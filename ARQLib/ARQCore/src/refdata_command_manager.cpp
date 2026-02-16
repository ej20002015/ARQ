#include <ARQCore/refdata_command_manager.h>

#include <ARQCore/messaging_service.h>

#include <ARQUtils/enum.h>
#include <ARQUtils/logger.h>

namespace ARQ::RD
{

void CommandManager::init( const Config& config )
{
	m_config           = config;
	m_messagingService = MessagingServiceFactory::create( m_config.messagingServiceDSH );
	m_streamProducer   = StreamingServiceFactory::createProducer( m_config.streamingServiceDSH, StreamProducerOptions( "RD::CommandManager", StreamProducerOptions::Preset::LowLatency ) );
	m_subTopicPattern  = SUB_TOPIC_PFX + ID::getSessionID().toString();
	m_subHandler       = std::make_shared<SubHandler>( *this );
}

void CommandManager::start()
{
	m_subscription = m_messagingService->subscribe( m_subTopicPattern, m_subHandler );
	m_running.store( true );
	m_commandCheckerThread = std::thread( &CommandManager::checkInFlightCommands, this );
}

void CommandManager::stop()
{
	m_subscription->drain();
	m_running.store( false );
	if( m_commandCheckerThread.joinable() )
		m_commandCheckerThread.join();
}

void CommandManager::registerOnResponseCallback( const CommandCallback& callback )
{
	std::unique_lock<std::shared_mutex> ul( m_globalCallbacksMut );
	m_globalCallbacks.push_back( callback );
}

void CommandManager::checkInFlightCommands()
{
	while( m_running )
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
				CommandResponse resp = {
					.corrID  = corrID,
					.status  = CommandResponse::TIMEOUT,
					.message = "Command has timed out"
				};

				onCommandResponse( resp );
			}
		}

		std::this_thread::sleep_for( m_config.checkerInterval );
	}
}

ID::UUID CommandManager::sendCommandImpl( Buffer&& buf, const std::string key, const std::string_view cmdName, const std::string_view cmdEntity, const std::string_view cmdAction, const CommandCallback& callback, const Time::Milliseconds timeout )
{
	ID::UUID corrID = ID::UUID::create();

	JSON logContext = {
		"Command", {
			{ "CorrID",      corrID.toString() },
			{ "Domain",      "RefData" },
			{ "CommandType", cmdName }
		}
	};

	Log::Context::Thread::Scoped logCtx( logContext );
	Log( Module::REFDATA ).debug( "RD::CommandManager: Sending command of type {} with CorrID={}", cmdName, corrID );

	StreamProducerMessage msg = formStreamMsg(
		std::move( buf ),
		key,
		corrID,
		cmdName,
		cmdEntity,
		cmdAction
	);

	// Register in-flight command before sending to avoid race
	createInFlightCommand( corrID, callback, timeout );

	m_streamProducer->send( msg, [=, this] ( const StreamProducerMessageMetadata& messageMetadata, std::optional<StreamError> error )
	{
		Log::Context::Thread::Scoped logCtx( logContext );

		if( !error )
		{
			Log( Module::REFDATA ).debug( "RD::CommandManager: Successfully sent command message to streaming topic {}: MessageID={}, Partition={}, Offset={}",
				messageMetadata.topic,
				messageMetadata.messageID ? std::to_string( *messageMetadata.messageID ) : "N/A",
				messageMetadata.partition,
				messageMetadata.offset ? std::to_string( *messageMetadata.offset ) : "N/A" );
		}
		else
		{
			std::string errMsg = std::format( "RD::CommandManager: Failed to send command message to streaming topic {}: {}", messageMetadata.topic, error->message );
			Log( Module::REFDATA ).error( "{}", errMsg );
			onCommandResponse( CommandResponse{ .corrID = corrID, .status = CommandResponse::ERRO, .message = errMsg } );
		}
	} );

	return corrID;
}

StreamProducerMessage CommandManager::formStreamMsg( Buffer&& buf, const std::string_view key, const ID::UUID& corrID, const std::string_view cmdName, const std::string_view cmdEntity, const std::string_view cmdAction )
{
	StreamProducerMessage msg;
	msg.topic = getStreamTopic( cmdEntity );
	msg.data  = SharedBuffer( std::move( buf ) ); // Get kafka to own the buffer
	msg.key   = key;

	msg.headers["ARQ_CorrID"]        = corrID.toString();
	msg.headers["ARQ_Type"]          = std::string( cmdName );
	msg.headers["ARQ_CmdAction"]     = std::string( cmdAction );
	msg.headers["ARQ_ResponseTopic"] = m_subTopicPattern;

	return msg;
}

std::string CommandManager::getStreamTopic( const std::string_view cmdEntity ) const
{
	return std::format( "ARQ.RefData.Commands.{}", cmdEntity );
}

void RD::CommandManager::createInFlightCommand( const ID::UUID& corrID, const CommandCallback& callback, const Time::Milliseconds timeout )
{
	Time::DateTime now = Time::DateTime::nowUTC();

	InFlightCommand command = {
		.callback    = callback,
		.startTime   = now,
		.timeoutTime = now + timeout
	};

	{
		std::unique_lock<std::shared_mutex> ul( m_inFlightCommandsMut );
		m_inFlightCommands.insert( std::make_pair( corrID, std::move( command ) ) );
	}
}

void CommandManager::onCommandResponse( const CommandResponse& resp )
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

	if( !command )
	{
		Log( Module::REFDATA ).warn( "RD::CommandManager: Received response for unknown command with CorrID={} - ignoring", resp.corrID );
		return;
	}

	const Time::Microseconds timeTaken = Time::DateTime::nowUTC() - command->startTime;
	const std::string logMsg = std::format( "RD::CommandManager: Received response with status={}, message={} for command with CorrID={} in {}ms - invoking callback(s)", Enum::enum_name( resp.status ), resp.message ? *resp.message : "NULL", resp.corrID, timeTaken / 1000.0 );
	switch( resp.status )
	{
		case CommandResponse::SUCCESS:
			Log( Module::REFDATA ).debug( "{}", logMsg ); break;
		default:
			Log( Module::REFDATA ).error( "{}", logMsg ); break;
	}

	ARQ_DO_IN_TRY( arqExc, errMsg );
		command->callback( resp );
	ARQ_END_TRY_AND_CATCH( arqExc, errMsg );

	if( arqExc.what().size() )
		Log( Module::NATS ).error( arqExc, "RD::CommandManager: Exception thrown in callback for CorrID={}", resp.corrID );
	else if( errMsg.size() )
		Log( Module::NATS ).error( "RD::CommandManager: Exception thrown in callback for CorrID={} - what: ", resp.corrID, errMsg );
		
	std::vector<CommandCallback> globalCallbacks;
	{
		std::shared_lock<std::shared_mutex> sl( m_globalCallbacksMut );
		globalCallbacks = m_globalCallbacks;
	}

	for( const auto& cb : m_globalCallbacks )
	{
		ARQ_DO_IN_TRY( arqExc, errMsg );
			cb( resp );
		ARQ_END_TRY_AND_CATCH( arqExc, errMsg );

		if( arqExc.what().size() )
			Log( Module::NATS ).error( arqExc, "RD::CommandManager: Exception thrown in global callback for CorrID={}", resp.corrID );
		else if( errMsg.size() )
			Log( Module::NATS ).error( "RD::CommandManager: Exception thrown in global callback for CorrID={} - what: ", resp.corrID, errMsg );
	}
}

void CommandManager::SubHandler::onMsg( Message&& msg )
{
	Log( Module::REFDATA ).debug( "RD::CommandManager: Received response message on topic {}", msg.topic );

	CommandResponse resp;

	ARQ_DO_IN_TRY( arqExc, errMsg );
		resp = m_owner.m_config.serialiser->deserialise<CommandResponse>( msg.data );
	ARQ_END_TRY_AND_CATCH( arqExc, errMsg );

	if( arqExc.what().size() )
		Log( Module::NATS ).error( arqExc, "RD::CommandManager: Exception thrown when deserialising message on topic [{}] to a CommandResponse object", msg.topic );
	else if( errMsg.size() )
		Log( Module::NATS ).error( "RD::CommandManager: Exception thrown when deserialising message on topic [{}] to a CommandResponse object - what: ", msg.topic, errMsg );

	m_owner.onCommandResponse( resp );
}

}