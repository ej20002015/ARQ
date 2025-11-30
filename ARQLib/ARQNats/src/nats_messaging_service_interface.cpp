#include <ARQNats/nats_messaging_service_interface.h>

#include <ARQUtils/error.h>
#include <ARQUtils/id.h>
#include <ARQUtils/enum.h>
#include <ARQUtils/types.h>
#include <ARQCore/data_source_config.h>
#include <ARQCore/logger.h>

using namespace Enum::bitwise_operators;

namespace ARQ
{

#define DO_IN_TRY() try {

#define END_TRY_AND_CATCH( arqException, errorMsg) } \
catch( const ARQException& e ) {                     \
	arqException = e; }                              \
catch( const std::exception& e ) {                   \
	errorMsg = e.what(); }                           \
catch( ... ) {                                       \
	errorMsg = "Unknown exception"; }                \

static std::string formatNatsError( natsStatus rc )
{
	return std::format( "nats.c error: code={}, error={}, message={}", static_cast<uint32_t>( rc ), natsStatus_GetText( rc ), nats_GetLastError( &rc ) );
}

static std::optional<MessagingEvent> natsStatus2MsgEvent( const natsStatus ns )
{
	switch( ns )
	{
		case NATS_CONNECTION_CLOSED:       return MessagingEvent::CONN_CLOSED;
		case NATS_NO_SERVER:               return MessagingEvent::NO_SERVER;
		case NATS_STALE_CONNECTION:        return MessagingEvent::CONN_STALE;
		case NATS_CONNECTION_DISCONNECTED: return MessagingEvent::CONN_DISCONNECT;
		case NATS_SLOW_CONSUMER:           return MessagingEvent::SLOW_SUBSCRIBER;
		case NATS_DRAINING:                return MessagingEvent::DRAINING;
		default:
			return std::nullopt;
	}
}

/*
*********************************************
*  Implementation of NatsMessagingService   *
*********************************************
*/

// ---------------------------- free function callbacks ----------------------------

static void natsErrorCB( natsConnection* nc, natsSubscription* subscription, natsStatus err, void* closure )
{
	NatsMessagingService& nms = *reinterpret_cast<NatsMessagingService*>( closure );
	nms.onNatsError( subscription, err );
}

static void natsClosedCB( natsConnection* nc, void* closure )
{
	NatsMessagingService& nms = *reinterpret_cast<NatsMessagingService*>( closure );
	nms.onNatsClosed();
}

static void natsDisconnectedCB( natsConnection* nc, void* closure )
{
	NatsMessagingService& nms = *reinterpret_cast<NatsMessagingService*>( closure );
	nms.onNatsDisconnected();
}

static void natsReconnectedCB( natsConnection* nc, void* closure )
{
	NatsMessagingService& nms = *reinterpret_cast<NatsMessagingService*>( closure );
	nms.onNatsReconnected();
}

static void natsDiscoveredServersCB( natsConnection* nc, void* closure )
{
	NatsMessagingService& nms = *reinterpret_cast<NatsMessagingService*>( closure );
	nms.onNatsDiscoveredServers();
}

static void natsLameDuckCB( natsConnection* nc, void* closure )
{
	NatsMessagingService& nms = *reinterpret_cast<NatsMessagingService*>( closure );
	nms.onNatsLameDuck();
}

static void onNatsMsg( natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure )
{
	ISubscriptionHandler& subHandler = *reinterpret_cast<ISubscriptionHandler*>( closure );

	Message msgObj;

	// Grab message data
	const char*  dataPtr  = natsMsg_GetData( msg );
	const size_t dataSize = natsMsg_GetDataLength( msg );
	msgObj.data = Buffer( reinterpret_cast<const uint8_t*>( dataPtr ), dataSize );

	// Grab message topic (subject in nats terminology)
	const char* subject = natsMsg_GetSubject( msg );
	msgObj.topic = subject;

	Log( Module::NATS ).trace( "onNatsMsg() callback triggered for message on topic [{}], with subHandler.getDesc [{}]", subject, subHandler.getDesc() );

	// Get headers (if enabled for handler)
	if( !( subHandler.getSubOptions() & SubscriptionOptions::DISABLE_HEADERS ) )
	{
		HeaderMap headerMap;

		natsStatus   rc;
		const char** headerKeys;
		int32_t      headerCount;
		if( rc = natsMsgHeader_Keys( msg, &headerKeys, &headerCount ); rc != NATS_OK )
			Log( Module::NATS ).error( "onNatsMsg() errored when processing message on topic [{}] - natsConnection_Subscribe() call failed with error: {}", subject, formatNatsError( rc ) );
		else
		{
			ARQDefer {
				free( headerKeys );
			});

			for( size_t i = 0; i < headerCount; i++ )
			{
				const char* key = headerKeys[i];

				const char** vals;
				int32_t      valCount;
				if( rc = natsMsgHeader_Values( msg, key, &vals, &valCount ); rc != NATS_OK )
					Log( Module::NATS ).error( "onNatsMsg() errored when processing message on topic [{}] - natsMsgHeader_Values() call failed with error: {}", subject, formatNatsError( rc ) );
				else
				{
					for( size_t j = 0; j < valCount; j++ )
						headerMap[key].emplace_back( vals[j] );

					free( vals );
				}
			}
		}

		msgObj.headers = std::move( headerMap );
	}

	// Invoke callback

	ARQException arqException;
	std::string errMsg;

	DO_IN_TRY();
	subHandler.onMsg( std::move( msgObj ) );
	END_TRY_AND_CATCH( arqException, errMsg );

	if( arqException.what().size() )
		Log( Module::NATS ).error( arqException, "onNatsMsg() errored when invoking user callback for message on topic [{}], with subHandler.desc() [] - subHandler.onMsg() call threw an ARQException", subject, subHandler.getDesc() );
	else if( errMsg.size() )
		Log( Module::NATS ).error( "onNatsMsg() errored when invoking user callback for message on topic [{}], with subHandler.desc() [] - subHandler.onMsg() call threw an exception: {}", subject, subHandler.getDesc(), errMsg );

	// Cleanup

	natsMsg_Destroy( msg );
}

// ------------------------ Constructor/Destructor -------------------------

NatsMessagingService::NatsMessagingService( const std::string_view dsh )
	: m_dsh( dsh )
{
	connect( dsh );
}

NatsMessagingService::~NatsMessagingService()
{
	disconnect();
}

// ---------------------------- IMessagingService implementation ----------------------------

void NatsMessagingService::publish( const std::string_view topic, const Message& msg )
{
	if( natsStatus rc = natsConnection_Publish( m_natsConn, topic.data(), msg.data.getDataPtrAs<const char*>(), msg.data.size ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to publish nats message - natsConnection_Publish() call failed with error: {}", formatNatsError( rc ) ) );

	Log( Module::NATS ).trace( "NatsMessagingService: Published message of size [{}] on topic [{}]", msg.data.size, topic );
}

std::unique_ptr<ISubscription> NatsMessagingService::subscribe( const std::string_view topicPattern, std::shared_ptr<ISubscriptionHandler> subHandler )
{
	natsStatus        rc;
	natsSubscription* sub;
	
	if( rc = natsConnection_Subscribe( &sub, m_natsConn, topicPattern.data(), onNatsMsg, subHandler.get() ); rc != NATS_OK )
		throw ARQException( std::format("Failed to create nats subscriber - natsConnection_Subscribe() call failed with error: {}", formatNatsError( rc ) ) );

	{
		std::unique_lock<std::shared_mutex> ul( m_natsSubID2HandlerAndTopicMutex );
		m_natsSubID2HandlerAndTopic.emplace( natsSubscription_GetID( sub ), std::make_pair( subHandler, topicPattern ) );
	}

	Log( Module::NATS ).info( "NatsMessagingService: Created subscription on topic [{}], with handler [{}], and nats subscription ID [{}]", topicPattern, subHandler->getDesc(), natsSubscription_GetID( sub ) );

	return std::make_unique<NatsSubscription>( sub );
}

GlobalStats NatsMessagingService::getStats() const
{
	natsStatus      rc;
	natsStatistics* stats;
	uint64_t        inMsgs, inBytes, outMsgs, outBytes, reconnects;

	if( rc = natsStatistics_Create( &stats ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to get nats stats - natsStatistics_Create() call failed with error: {}", formatNatsError( rc ) ) );
	else
	{
		ARQDefer {
			natsStatistics_Destroy( stats );
		});

		if( rc = natsConnection_GetStats( m_natsConn, stats ); rc != NATS_OK )
			throw ARQException( std::format( "Failed to get nats stats - natsConnection_GetStats() call failed with error: {}", formatNatsError( rc ) ) );
		else if( rc = natsStatistics_GetCounts( stats, &inMsgs, &inBytes, &outMsgs, &outBytes, &reconnects ) )
			throw ARQException( std::format( "Failed to get nats stats - natsStatistics_GetCounts() call failed with error: {}", formatNatsError( rc ) ) );

		return GlobalStats {
			inMsgs,
			inBytes,
			outMsgs,
			outBytes,
			reconnects
		};
	}
}

// ---------------------------- Connection ----------------------------

void NatsMessagingService::connect( const std::string_view dsh )
{
	natsStatus rc;

	natsOptions* natsOptions;
	if( rc = natsOptions_Create( &natsOptions ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to create NatsMessagingService - natsOptions_Create() call failed with error: {}", formatNatsError( rc ) ) );

	ARQDefer{
		natsOptions_Destroy( natsOptions );
	});

	// Connection urls

	static constexpr size_t MAX_SERVERS = 10;
	std::array<const char*, MAX_SERVERS> serverUrls;
	std::array<std::string, MAX_SERVERS> serverUrlsStrs; // Just to store the string buffers
	size_t i = 0;
	const DataSourceConfig& dsc = DataSourceConfigManager::inst().get( dsh );
	for( const auto& [_, connProps] : dsc.connPropsMap )
	{
		if( i >= MAX_SERVERS )
			throw ARQException( std::format( "Failed to create NatsMessagingService - number of conn props in dsh [{}] exceeds maximum allowed for nats of {}", dsh, MAX_SERVERS ) );

		serverUrlsStrs[i] = std::format( "{}:{}", connProps.hostname, connProps.port );
		serverUrls[i] = serverUrlsStrs[i].c_str();
		++i;
	}
	if( rc = natsOptions_SetServers( natsOptions, serverUrls.data(), i ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to create NatsMessagingService - natsOptions_SetServers() call failed with error: {}", formatNatsError( rc ) ) );

	// Misc options

	if( rc = natsOptions_SetAllowReconnect( natsOptions, true ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to create NatsMessagingService - natsOptions_SetAllowReconnect() call failed with error: {}", formatNatsError( rc ) ) );

	if( rc = natsOptions_SetName( natsOptions, std::format( "ARQNats.Session-{}", ID::getSessionID() ).c_str() ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to create NatsMessagingService - natsOptions_SetName() call failed with error: {}", formatNatsError( rc ) ) );

	// Set callbacks

	if( rc = natsOptions_SetErrorHandler( natsOptions, natsErrorCB, reinterpret_cast<void*>( this ) ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to create NatsMessagingService - natsOptions_SetErrorHandler() call failed with error: {}", formatNatsError( rc ) ) );

	if( rc = natsOptions_SetClosedCB( natsOptions, natsClosedCB, reinterpret_cast<void*>( this ) ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to create NatsMessagingService - natsOptions_SetClosedCB() call failed with error: {}", formatNatsError( rc ) ) );

	if( rc = natsOptions_SetDisconnectedCB( natsOptions, natsDisconnectedCB, reinterpret_cast<void*>( this ) ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to create NatsMessagingService - natsOptions_SetDisconnectedCB() call failed with error: {}", formatNatsError( rc ) ) );

	if( rc = natsOptions_SetReconnectedCB( natsOptions, natsReconnectedCB, reinterpret_cast<void*>( this ) ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to create NatsMessagingService - natsOptions_SetReconnectedCB() call failed with error: {}", formatNatsError( rc ) ) );

	if( rc = natsOptions_SetDiscoveredServersCB( natsOptions, natsDiscoveredServersCB, reinterpret_cast<void*>( this ) ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to create NatsMessagingService - natsOptions_SetDiscoveredServersCB() call failed with error: {}", formatNatsError( rc ) ) );

	if( rc = natsOptions_SetLameDuckModeCB( natsOptions, natsLameDuckCB, reinterpret_cast<void*>( this ) ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to create NatsMessagingService - natsOptions_SetLameDuckModeCB() call failed with error: {}", formatNatsError( rc ) ) );

	// Connect

	if( natsStatus rc = natsConnection_Connect( &m_natsConn, natsOptions ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to create NatsMessagingService - natsConnection_Connect() call failed with error: {}", formatNatsError( rc ) ) );
}

void NatsMessagingService::disconnect()
{
	natsConnection_Destroy( m_natsConn );
}

// ---------------------------- Callbacks ----------------------------

void NatsMessagingService::onNatsError( natsSubscription* subscription, natsStatus err )
{
	const int64_t subID = natsSubscription_GetID( subscription );

	Log( Module::NATS ).debug( "onNatsError() callback triggered with nats status [{}] for subscription with ID [{}]", natsStatus_GetText( err ), subID );

	std::shared_ptr<ISubscriptionHandler> subHandler;
	std::string                           topic;
	{
		std::shared_lock<std::shared_mutex> sl( m_natsSubID2HandlerAndTopicMutex );
		if( auto it = m_natsSubID2HandlerAndTopic.find( subID ); it != m_natsSubID2HandlerAndTopic.end() )
		{
			const auto& [handler, topicStr] = it->second;
			subHandler = handler.lock();
			topic      = topicStr;
		}
	}

	if( subHandler )
	{
		const std::optional<MessagingEvent> e = natsStatus2MsgEvent( err );
		if( e )
		{
			ARQException arqException;
			std::string errMsg;

			DO_IN_TRY();
			subHandler->onEvent( SubscriptionEvent{ *e, topic } );
			END_TRY_AND_CATCH( arqException, errMsg );

			if( arqException.what().size() )
				Log( Module::NATS ).error( arqException, "onNatsError() errored when invoking user callback for subscription event [{}], with subHandler.desc() [] - subHandler.onEvent() call threw an ARQException", Enum::enum_name( *e ), subHandler->getDesc() );
			else if( errMsg.size() )
				Log( Module::NATS ).error( "onNatsError() errored when invoking user callback for subscription event [{}], with subHandler.desc() [] - subHandler.onEvent() call threw an exception: {}", Enum::enum_name( *e ), subHandler->getDesc(), errMsg );
		}
	}
	else
		Log( Module::NATS ).error( "onNatsError() errored so cannot forward event [{}] - couldn't get subscription handler", natsStatus_GetText( err ) );
}

void NatsMessagingService::onNatsClosed()
{
	// TODO: Work out if we need to send this to all the subscribers as well or other callback will deal with that
	Log( Module::NATS ).error( "NatsMessagingService: Nats connection has been closed" );
}

void NatsMessagingService::onNatsDisconnected()
{
	// TODO: Work out if we need to send this to all the subscribers as well or other callback will deal with that
	Log( Module::NATS ).error( "NatsMessagingService: Nats connection has been disconnected - will attempt to reconnect" );
}

void NatsMessagingService::onNatsReconnected()
{
	// TODO: Work out if we need to send this to all the subscribers as well or other callback will deal with that
	Log( Module::NATS ).info( "NatsMessagingService: Nats connection has been reconnected" );
}

void NatsMessagingService::onNatsDiscoveredServers()
{
	// TODO: Work out if we need to send this to all the subscribers as well or other callback will deal with that
	Log( Module::NATS ).info( "NatsMessagingService: There are new nats servers" );
}

void NatsMessagingService::onNatsLameDuck()
{
	// TODO: Work out if we need to send this to all the subscribers as well or other callback will deal with that
	Log( Module::NATS ).info( "NatsMessagingService: Nats connection has gone into lame duck mode" );
}

/*
*********************************************
*    Implementation of NatsSubscription     *
*********************************************
*/

int64_t NatsSubscription::getID()
{
	return natsSubscription_GetID( m_natsSub );
}

std::string_view NatsSubscription::getTopic()
{
	return natsSubscription_GetSubject( m_natsSub );
}

bool NatsSubscription::isValid()
{
	return natsSubscription_IsValid( m_natsSub );
}

SubStats NatsSubscription::getStats()
{
	// TODO
	return SubStats();
}

void NatsSubscription::unsubscribe()
{
	// TODO
	/*{
		std::unique_lock<std::shared_mutex> ul( m_natsSubID2HandlerMutex );
		m_natsSubID2Handler.emplace( natsSubscription_GetID( sub ), subHandler );
	}*/
}

void NatsSubscription::drainAndUnsubscribe()
{
	// TODO
}

void NatsSubscription::blockOnDrainAndUnsubscribe()
{
	// TODO
}

}