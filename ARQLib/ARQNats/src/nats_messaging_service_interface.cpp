#include <ARQNats/nats_messaging_service_interface.h>

#include <ARQUtils/error.h>
#include <ARQUtils/id.h>
#include <ARQUtils/enum.h>
#include <ARQUtils/types.h>
#include <ARQCore/data_source_config.h>
#include <ARQUtils/logger.h>

using namespace ARQ::Enum::bitwise_operators;

namespace ARQ
{

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
		case NATS_CONNECTION_DISCONNECTED: return MessagingEvent::CONN_DISCONNECTED;
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

void natsErrorCB( natsConnection* nc, natsSubscription* subscription, natsStatus err, void* closure )
{
	NatsMessagingService& nms = *reinterpret_cast<NatsMessagingService*>( closure );
	nms.onNatsError( subscription, err );
}

void natsClosedCB( natsConnection* nc, void* closure )
{
	NatsMessagingService& nms = *reinterpret_cast<NatsMessagingService*>( closure );
	nms.onNatsClosed();
}

void natsDisconnectedCB( natsConnection* nc, void* closure )
{
	NatsMessagingService& nms = *reinterpret_cast<NatsMessagingService*>( closure );
	nms.onNatsDisconnected();
}

void natsReconnectedCB( natsConnection* nc, void* closure )
{
	NatsMessagingService& nms = *reinterpret_cast<NatsMessagingService*>( closure );
	nms.onNatsReconnected();
}

void natsDiscoveredServersCB( natsConnection* nc, void* closure )
{
	NatsMessagingService& nms = *reinterpret_cast<NatsMessagingService*>( closure );
	nms.onNatsDiscoveredServers();
}

void natsLameDuckCB( natsConnection* nc, void* closure )
{
	NatsMessagingService& nms = *reinterpret_cast<NatsMessagingService*>( closure );
	nms.onNatsLameDuck();
}

void onNatsMsg( natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure )
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
		if( rc = natsMsgHeader_Keys( msg, &headerKeys, &headerCount ); rc != NATS_OK && rc != NATS_NOT_FOUND )
			Log( Module::NATS ).error( "onNatsMsg() errored when processing message on topic [{}] - natsConnection_Subscribe() call failed with error: {}", subject, formatNatsError( rc ) );
		else if( rc != NATS_NOT_FOUND )
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
	ARQ_DO_IN_TRY( arqExc, errMsg );
		subHandler.onMsg( std::move( msgObj ) );
	ARQ_END_TRY_AND_CATCH( arqExc, errMsg );

	if( arqExc.what().size() )
		Log( Module::NATS ).error( arqExc, "onNatsMsg() errored when invoking user callback for message on topic [{}], with subHandler.desc() [] - subHandler.onMsg() call threw an ARQException", subject, subHandler.getDesc() );
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
	natsStatus rc;
	natsMsg* natsMsg;

	if( rc = natsMsg_Create( &natsMsg, topic.data(), nullptr, msg.data.getDataPtrAs<const char*>(), msg.data.size ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to publish nats message - natsMsg_Create() call failed with error: {}", formatNatsError( rc ) ) );

	if( msg.headers.size() )
	{
		for( const auto& [key, vals] : msg.headers )
		{
			for( const std::string& val : vals )
			{
				if( rc = natsMsgHeader_Add( natsMsg, key.c_str(), val.c_str() ); rc != NATS_OK )
					throw ARQException( std::format( "Failed to publish nats message - natsMsgHeader_Add() call failed with error: {}", formatNatsError( rc ) ) );
			}
		}
	}
	
	if( natsStatus rc = natsConnection_PublishMsg( m_natsConn, natsMsg ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to publish nats message - natsConnection_Publish() call failed with error: {}", formatNatsError( rc ) ) );

	Log( Module::NATS ).trace( "NatsMessagingService: Published message of size [{}] on topic [{}] with [{}] headers", msg.data.size, topic, msg.headers.size() );
}

std::unique_ptr<ISubscription> NatsMessagingService::subscribe( const std::string_view topicPattern, std::shared_ptr<ISubscriptionHandler> subHandler )
{
	natsStatus        rc;
	natsSubscription* sub;
	
	if( rc = natsConnection_Subscribe( &sub, m_natsConn, topicPattern.data(), onNatsMsg, subHandler.get() ); rc != NATS_OK )
		throw ARQException( std::format("Failed to create nats subscriber - natsConnection_Subscribe() call failed with error: {}", formatNatsError( rc ) ) );

	Log( Module::NATS ).info( "NatsMessagingService: Created subscription on topic [{}], with handler [{}], and nats subscription ID [{}]", topicPattern, subHandler->getDesc(), natsSubscription_GetID( sub ) );

	return std::make_unique<NatsSubscription>( sub );
}

void NatsMessagingService::registerEventCallback( const MessagingEventCallbackFunc& eventCallbackFunc )
{
	std::unique_lock<std::shared_mutex> ul( m_eventCallbacksMutex );
	m_eventCallbacks.push_back( eventCallbackFunc );
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
	Log( Module::NATS ).info( "Connecting to Nats for dsh [{}]", dsh );

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

	Log( Module::NATS ).info( "Finished connecting to Nats for dsh [{}]", dsh );
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

	const std::optional<MessagingEvent> e = natsStatus2MsgEvent( err );
	if( !e )
	{
		Log( Module::NATS ).debug( "onNatsError() callback returning early - nats status [{}] not an ARQ messaging event", natsStatus_GetText( err ) );
		return;
	}

	invokeUserEventCallbacks( *e, subID );
}

void NatsMessagingService::onNatsClosed()
{
	// TODO: Work out if we need to send this to all the subscribers as well or other callback will deal with that
	Log( Module::NATS ).error( "NatsMessagingService: Nats connection has been closed" );
	invokeUserEventCallbacks( MessagingEvent::CONN_CLOSED );
}

void NatsMessagingService::onNatsDisconnected()
{
	// TODO: Work out if we need to send this to all the subscribers as well or other callback will deal with that
	Log( Module::NATS ).error( "NatsMessagingService: Nats connection has been disconnected - will attempt to reconnect" );
	invokeUserEventCallbacks( MessagingEvent::CONN_DISCONNECTED );
}

void NatsMessagingService::onNatsReconnected()
{
	// TODO: Work out if we need to send this to all the subscribers as well or other callback will deal with that
	Log( Module::NATS ).info( "NatsMessagingService: Nats connection has been reconnected" );
	invokeUserEventCallbacks( MessagingEvent::CONN_RECONNECTED );
}

void NatsMessagingService::onNatsDiscoveredServers()
{
	Log( Module::NATS ).info( "NatsMessagingService: There are new nats servers" );
}

void NatsMessagingService::onNatsLameDuck()
{
	Log( Module::NATS ).info( "NatsMessagingService: Nats connection has gone into lame duck mode" );
}

void NatsMessagingService::invokeUserEventCallbacks( const MessagingEvent e, const std::optional<int64_t> subID )
{
	std::vector<MessagingEventCallbackFunc> eventCallbacks;
	{
		std::shared_lock<std::shared_mutex> sl( m_eventCallbacksMutex );
		eventCallbacks = m_eventCallbacks;
	}
	if( eventCallbacks.empty() )
	{
		Log( Module::NATS ).debug( "NatsMessagingService::invokeUserEventCallbacks() returning early - no user event callbacks registered" );
		return;
	}

	Log( Module::NATS ).debug( "NatsMessagingService::invokeUserEventCallbacks() invoking [{}] user messaging event callback functions", eventCallbacks.size() );
	for( const auto& cb : eventCallbacks )
	{
		if( !cb )
			continue;

		ARQ_DO_IN_TRY( arqExc, errMsg );
			cb( e, subID );
		ARQ_END_TRY_AND_CATCH( arqExc, errMsg );

		if( arqExc.what().size() )
			Log( Module::NATS ).error( arqExc, "NatsMessagingService::invokeUserEventCallbacks() errored when invoking user callback for messaging event [{}] - callback threw an ARQException", Enum::enum_name( e ) );
		else if( errMsg.size() )
			Log( Module::NATS ).error( "NatsMessagingService::invokeUserEventCallbacks() errored when invoking user callback for messaging event [{}] - callback threw an exception: {}", Enum::enum_name( e ), errMsg );
	}
}

/*
*********************************************
*    Implementation of NatsSubscription     *
*********************************************
*/

NatsSubscription::~NatsSubscription()
{
	if( m_natsSub )
		unsubscribe();
}

int64_t NatsSubscription::getID()
{
	checkPtr();
	return natsSubscription_GetID( m_natsSub );
}

std::string_view NatsSubscription::getTopic()
{
	checkPtr();
	return natsSubscription_GetSubject( m_natsSub );
}

bool NatsSubscription::isValid()
{
	checkPtr();
	return natsSubscription_IsValid( m_natsSub );
}

SubStats NatsSubscription::getStats()
{
	checkPtr();

	int32_t pendingMsgs, pendingBytes, maxPendingMsgs, maxPendingBytes;
	int64_t deliveredMsgs, droppedMsgs;

	if( const natsStatus rc = natsSubscription_GetStats( m_natsSub, &pendingMsgs, &pendingBytes, &maxPendingMsgs, &maxPendingBytes, &deliveredMsgs, &droppedMsgs ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to get nats subscription stats - natsSubscription_GetStats() call failed with error: {}", formatNatsError( rc ) ) );
	
	return SubStats {
		pendingMsgs,
		pendingBytes,
		maxPendingMsgs,
		maxPendingBytes,
		deliveredMsgs,
		droppedMsgs
	};
}

void NatsSubscription::unsubscribe()
{
	checkPtr();

	if( const natsStatus rc = natsSubscription_Unsubscribe( m_natsSub ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to unsubscribe nats subscription - natsSubscription_Unsubscribe() call failed with error: {}", formatNatsError( rc ) ) );

	m_natsSub = nullptr;
}

void NatsSubscription::drain( const std::chrono::milliseconds timeout )
{
	checkPtr();

	if( const natsStatus rc = natsSubscription_DrainTimeout( m_natsSub, timeout.count() ); rc != NATS_OK )
		throw ARQException( std::format( "Failed to drain nats subscription - natsSubscription_DrainTimeout() call failed with error: {}", formatNatsError( rc ) ) );

	m_natsSub = nullptr;
}

void NatsSubscription::checkPtr() const
{
	if( !m_natsSub )
		throw ARQException( "Nats subscription is nullptr - has this already be unsubscribed/drained?" );
}

}