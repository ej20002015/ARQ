#include <ARQCore/refdata_command_manager.h>

#include <ARQCore/messaging_service.h>

#include <ARQUtils/enum.h>
#include <ARQCore/logger.h>

namespace ARQ
{

void RefDataCommandManager::init( const Config& config )
{
	m_config = config;
	m_messagingService = MessagingServiceFactory::create( m_config.messagingServiceDSH );
	m_subTopicPattern = SUB_TOPIC_PFX + ID::getSessionID().toString();
	m_subHandler = std::make_shared<SubHandler>( this );
}

void RefDataCommandManager::start()
{
	m_subscription = m_messagingService->subscribe( m_subTopicPattern, m_subHandler );
	m_running.store( true );
	m_commandCheckerThread = std::thread( &RefDataCommandManager::checkInFlightCommands, this );
}

void RefDataCommandManager::stop()
{
	m_subscription->blockOnDrainAndUnsubscribe();
	m_running.store( false );
	if( m_commandCheckerThread.joinable() )
		m_commandCheckerThread.join();
}

void RefDataCommandManager::registerOnResponseCallback( const RefDataCommandCallback& callback )
{
	std::unique_lock<std::shared_mutex> ul( m_globalCallbacksMut );
	m_globalCallbacks.push_back( callback );
}

ID::UUID RefDataCommandManager::upsertCurrencies( const std::vector<RDEntities::Currency>& currencies, const RefDataCommandCallback& callback, const Time::Milliseconds timeout )
{
	Time::DateTime now = Time::DateTime::nowUTC();
	ID::UUID corrID = ID::UUID::create();

	// TODO: Actually send to kafka broker

	InFlightCommand command = {
		.callback = callback,
		.startTime = now,
		.timeoutTime = now + timeout
	};

	{
		std::unique_lock<std::shared_mutex> ul( m_inFlightCommandsMut );
		m_inFlightCommands.insert( std::make_pair( std::move( corrID ), std::move( command ) ) );
	}

	Log( Module::REFDATA ).debug( "RefDataCommandManager: Created upsertCurrencies command with CorrID={}", corrID );
	return corrID;
}

void RefDataCommandManager::checkInFlightCommands()
{
	if( m_running )
	{
		std::map<ID::UUID, InFlightCommand> inFlightCommands;
		{
			std::shared_lock<std::shared_mutex> sl( m_inFlightCommandsMut );
			inFlightCommands = m_inFlightCommands;
		}

		for( const auto& [corrID, inFlightCommand] : inFlightCommands )
		{
			if( Time::DateTime::nowUTC() > inFlightCommand.timeoutTime )
			{
				RefDataCommandResponse resp = {
					.corrID = corrID,
					.status = RefDataCommandResponse::TIMEOUT,
					.message = "Command has timed out"
				};

				onCommandResponse( resp );
			}
		}

		std::this_thread::sleep_for( m_config.checkerInterval );
	}
}

void RefDataCommandManager::onCommandResponse( const RefDataCommandResponse& resp )
{
	std::optional<InFlightCommand> command;
	{
		std::unique_lock<std::shared_mutex> ul( m_inFlightCommandsMut );
		const auto it = m_inFlightCommands.find( resp.corrID );
		if( it != m_inFlightCommands.end() )
		{
			command = std::move( it->second );
			m_inFlightCommands.erase( it );
		}
	}

	if( command.has_value() )
	{
		const Time::Microseconds timeTaken = Time::DateTime::nowUTC() - command->startTime;
		const std::string logMsg = std::format( "RefDataCommandManager: Received response with status={}, message={} for command with CorrID={} in {}ms - invoking callback(s)", Enum::enum_name( resp.status ), resp.message ? *resp.message : "NULL", resp.corrID, timeTaken / 1000.0 );
		switch( resp.status )
		{
			case RefDataCommandResponse::SUCCESS:
				Log( Module::REFDATA ).debug( "{}", logMsg ); break;
			default:
				Log( Module::REFDATA ).error( "{}", logMsg ); break;
		}

		try
		{
			command->callback( resp );
		}
		catch( const ARQException& e )
		{
			Log( Module::REFDATA ).error( e, "RefDataCommandManager: Exception thrown in callback for CorrID={}", resp.corrID );
		}
		catch( const std::exception& e )
		{
			Log( Module::REFDATA ).error( "RefDataCommandManager: Exception thrown in callback for CorrID={} - what: ", resp.corrID, e.what() );
		}
		catch( ... )
		{
			Log( Module::REFDATA ).error( "RefDataCommandManager: Unknown exception thrown in callback for CorrID={}", resp.corrID );
		}
		
		{
			std::shared_lock<std::shared_mutex> sl( m_globalCallbacksMut );
			for( const auto& cb : m_globalCallbacks )
			{
				try
				{
					cb( resp );
				}
				catch( const ARQException& e )
				{
					Log( Module::REFDATA ).error( e, "RefDataCommandManager: Exception thrown in global callback for CorrID={}", resp.corrID );
				}
				catch( const std::exception& e )
				{
					Log( Module::REFDATA ).error( "RefDataCommandManager: Exception thrown in global callback for CorrID={} - what: ", resp.corrID, e.what() );
				}
				catch( ... )
				{
					Log( Module::REFDATA ).error( "RefDataCommandManager: Unknown exception thrown in global callback for CorrID={}", resp.corrID );
				}
			}
		}
	}
	else
		Log( Module::REFDATA ).error( "RefDataCommandManager: Received response for unknown command with CorrID={} - ignoring", resp.corrID );
}

void RefDataCommandManager::SubHandler::onMsg( Message&& msg )
{
	Log( Module::REFDATA ).debug( "RefDataCommandManager: Received response message on topic {}", msg.topic );

	RefDataCommandResponse resp;

	try
	{
		resp = m_owner.m_config.serialiser->deserialise<RefDataCommandResponse>( msg.data );
	}
	catch( const ARQException& e )
	{
		Log( Module::REFDATA ).error( e, "RefDataCommandManager: Exception thrown when deserialising message on topic [{}] to a RefDataCommandResponse object", msg.topic );
		return;
	}
	catch( const std::exception& e )
	{
		Log( Module::REFDATA ).error( "RefDataCommandManager: Exception thrown when deserialising message on topic [{}] to a RefDataCommandResponse object - what: ", msg.topic, e.what() );
		return;
	}
	catch( ... )
	{
		Log( Module::REFDATA ).error( "RefDataCommandManager: Unknown exception thrown when deserialising message on topic [{}] to a RefDataCommandResponse object", msg.topic );
		return;
	}

	m_owner.onCommandResponse( resp );
}

void RefDataCommandManager::SubHandler::onEvent( SubscriptionEvent&& event )
{
	// TODO
}

}