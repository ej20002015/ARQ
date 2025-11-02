#include <ARQCore/refdata_command_manager.h>

#include <ARQUtils/enum.h>
#include <ARQCore/logger.h>

namespace ARQ
{

void RefDataCommandManager::init()
{
	// Nothing to do atm but will have stuff in a bit
}

void RefDataCommandManager::start()
{
	m_running.store( true );
	m_commandCheckerThread = std::thread( &RefDataCommandManager::checkInFlightCommands, this );
}

void RefDataCommandManager::stop()
{
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

	// TODO: ACtually send to kafka broker

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

// TODO: Set up nats callback that invokes this
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

		std::this_thread::sleep_for( m_checkerInterval );
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

}