#include <ARQCore/service_base.h>

#include <ARQUtils/error.h>
#include <ARQUtils/enum.h>
#include <ARQUtils/str.h>

#include <csignal>
#include <regex>

namespace ARQ
{

ServiceRunner* ServiceRunner::s_instPtr = nullptr;

ServiceRunner::ServiceRunner( ServiceBase& service )
	: m_service( service )
	, m_cfgWrangler( service.serviceDescription(), service.serviceName() )
{
}

int ServiceRunner::tryRunImpl( int argc, char* argv[] )
{
	ARQ_DO_IN_TRY( arqExc, errMsg );
		return runImpl( argc, argv );
	ARQ_END_TRY_AND_CATCH( arqExc, errMsg );
	if( arqExc.what().size() )
	{
		if( Logger::globalInst() )
			Log( Module::EXE ).critical( arqExc, "Exiting after unhandled exception thrown in service runner" );
		else
			std::cerr << "Exiting after unhandled exception thrown in service runner - " << arqExc.what();
	}
	else if( errMsg.size() )
	{
		if( Logger::globalInst() )
			Log( Module::EXE ).critical( "Exiting after unhandled exception thrown in service runner - what: ", errMsg );
		else
			std::cerr << "Exiting after unhandled exception thrown in service runner - what: " << errMsg;
	}

	return SvcExitCodes::UNKNOWN_ERROR;
}

int ServiceRunner::runImpl( int argc, char* argv[] )
{
	m_service.m_healthy   = true;
	m_service.m_shouldRun = true;

	registerSignalHandlers();

	// Register configuration options and init library

	addConfigOptions();
	LibGuard guard( argc, argv, m_cfgWrangler );
	
	logConfig();

	// Set up and start admin HTTP server

	setUpAdminServer();

	if( !runAdminServer() )
	{
		Log( Module::EXE ).critical( "Exiting after failure to start and run admin HTTP server" );
		return SvcExitCodes::ADMIN_SRV_ERROR;
	}

	// Start service

	if( !startupService() )
	{
		Log( Module::EXE ).critical( "Exiting after failure to start up service" );
		shutdownAdminServer();
		return SvcExitCodes::STARTUP_ERROR;
	}

	// Run service

	const bool runSuccess = runService();
	if( !runSuccess )
		Log( Module::EXE ).critical( "Failure during main service run loop - exiting" );

	// Shutdown service

	if( !shutdownService() )
	{
		Log( Module::EXE ).critical( "Exiting after failure to shut down service" );
		shutdownAdminServer();
		return SvcExitCodes::SHUTDOWN_ERROR;
	}

	shutdownAdminServer();

	m_service.m_healthy = false;

	return runSuccess ? SvcExitCodes::SUCCESS : SvcExitCodes::RUNTIME_ERROR;
}

void ServiceRunner::registerSignalHandlers()
{
	std::signal( SIGINT, ServiceRunner::handleSignal  );
	std::signal( SIGTERM, ServiceRunner::handleSignal );
}

void ServiceRunner::addConfigOptions()
{
	Log( Module::EXE ).debug( "Adding service configuration options..." );

	// Add common config options

	m_cfgWrangler.add( m_service.m_baseConfig.adminPort, "--svc.adminServer.port", "Set the port used by the http admin server", "ARQ_svc_adminServer_port" );

	// Add service-specific config options

	m_service.registerConfigOptions( m_cfgWrangler );

	Log( Module::EXE ).debug( "Finished adding service configuration options" );
}

void ARQ::ServiceRunner::logConfig()
{
	const std::string configDump = m_cfgWrangler.dump();
	Log( Module::EXE ).info( "Service Config: {}", configDump );

	// Also log each item indivdually to make it more human readable
	std::stringstream ss( configDump );
	std::string line;
	while( std::getline( ss, line ) )
	{
		line.erase( std::remove( line.begin(), line.end(), '\"' ), line.end() );
		line.erase( std::remove( line.begin(), line.end(), '\'' ), line.end() );
		line = std::regex_replace( line, std::regex( R"(\\\\)" ), R"(\)" );

		if( !line.empty() )
			Log( Module::EXE ).info( "   {}", line );
	}
}

void ServiceRunner::setUpAdminServer()
{
	Log( Module::EXE ).debug( "Configuring admin HTTP server..." );

	// Set up logging and error handling for admin server

	m_adminServer.set_logger( [] ( const http::Request& req, const http::Response& res )
	{
		if( Str::contains( req.path, "/health/" ) ) // Health checks are very frequent, log at trace level
			Log( Module::HTTP ).trace( "{} {} - Status: {}", req.method, req.path, res.status );
		else
			Log( Module::HTTP ).debug( "{} {} - Status: {}", req.method, req.path, res.status );
	} );

	m_adminServer.set_error_handler( [] ( const http::Request& req, http::Response& res )
	{
		if( res.status == 404 )
		{
			Log( Module::HTTP ).debug( "404 Not Found: {} {}", req.method, req.path );
			res.set_content( "404 Not Found", "text/plain" );
		}
		else if( res.status == 503 ) // For the health checks
		{
			Log( Module::HTTP ).debug( "503 Server Unavailable {} on {} {}", res.status, req.method, req.path );
			res.set_content( "503 Server Unavailable", "text/plain" );
		}
		else if( res.status >= 500 )
		{
			Log( Module::HTTP ).error( "Internal Server Error {} on {} {}", res.status, req.method, req.path );
			res.set_content( "Internal Server Error", "text/plain" );
		}
	} );

	// Health endpoints

	m_adminServer.Get( "/health/live", [this]( const http::Request& req, http::Response& res )
	{
		res.status = m_service.m_healthy ? 200 : 503;
		res.body   = m_service.m_healthy ? "Service is healthy" : "Service is not healthy";
	} );

	m_adminServer.Get( "/health/ready", [this]( const http::Request& req, http::Response& res )
	{
		res.status = m_service.m_ready ? 200 : 503;
		res.body   = m_service.m_ready ? "Service is ready to receive traffic" : "Service is not ready to receive traffic";
	} );

	m_adminServer.Get( "/health/startup", [this]( const http::Request& req, http::Response& res )
	{
		res.status = m_service.m_started ? 200 : 503;
		res.body   = m_service.m_started ? "Service has started successfully" : "Service has not finished starting up";
	} );

	// Log level endpoints

	m_adminServer.Get( "/admin/logLevel", [this]( const http::Request& req, http::Response& res )
	{
		const LogLevel level = Log::logger().getLevel();
		res.set_content( std::format( "LogLevel: {}", Enum::enum_name( level ) ), "text/plain" );
		res.status = 200;
	} );

	m_adminServer.Get( "/admin/logLevel2", [this] ( const http::Request& req, http::Response& res )
	{
		const LogLevel level = Log::logger().getLevel2();
		res.set_content( std::format( "LogLevel2: {}", Enum::enum_name( level ) ), "text/plain" );
		res.status = 200;
	} );

	const auto setLevel = [this] ( const http::Request& req, http::Response& res, bool primaryLevel )
	{
		const std::string_view logLevelStr = primaryLevel ? "LogLevel" : "LogLevel2";

		std::string levelStr = req.body;
		std::optional<LogLevel> level = Enum::enum_cast<LogLevel>( Str::toUpper( levelStr ) );
		if( !level )
		{
			res.status = 400;
			res.set_content( std::format( "Invalid log level given in request: \"{}\"", levelStr ), "text/plain" );
		}
		else
		{
			Log( Module::EXE ).info( "Received {} change request via admin endpoint - changing to {}", logLevelStr, Enum::enum_name( *level ) );
			primaryLevel ? Log::logger().setLevel( *level ) : Log::logger().setLevel2( *level );

			res.set_content( std::format( "{} updated to: {}", logLevelStr, Enum::enum_name( *level ) ), "text/plain" );
			res.status = 200;
		}
	};

	m_adminServer.Post( "/admin/logLevel", [&setLevel] ( const http::Request& req, http::Response& res )
	{
		setLevel( req, res, true );
	} );

	m_adminServer.Post( "/admin/logLevel2", [&setLevel] ( const http::Request& req, http::Response& res )
	{
		setLevel( req, res, false );
	} );

	// Configuration dump endpoint

	m_adminServer.Get( "/admin/config", [this]( const http::Request& req, http::Response& res )
	{
		res.set_content( m_cfgWrangler.dump(), "text/plain" );
		res.status = 200;
	} );
	
	// Allow service to register custom admin endpoints

	m_service.registerCustomAdminEndpoints( m_adminServer );

	Log( Module::EXE ).debug( "Finished configuring admin HTTP server" );
}

bool ServiceRunner::runAdminServer()
{
	Log( Module::EXE ).debug( "Starting up admin HTTP server..." );
	
	const int32_t port = m_service.m_baseConfig.adminPort;

	if( !m_adminServer.bind_to_port( "0.0.0.0", port ) )
	{
		Log( Module::EXE ).error( "Failed to bind admin HTTP server to port {}", port );
		return false;
	}

	m_adminServerThread = std::thread( [this, port] ()
	{
		Log( Module::EXE ).info( "Starting admin HTTP server on port {}", port );
		m_adminServer.listen_after_bind();
	} );

	Log( Module::EXE ).debug( "Finished starting up admin HTTP server" );

	return true;
}

void ServiceRunner::shutdownAdminServer()
{
	Log( Module::EXE ).debug( "Shutting down admin HTTP server..." );

	m_adminServer.stop();
	if( m_adminServerThread.joinable() )
		m_adminServerThread.join();

	Log( Module::EXE ).debug( "Finished shutting down admin HTTP server" );
}

bool ServiceRunner::startupService()
{
	Log( Module::EXE ).info( "Starting up service..." );

	ARQ_DO_IN_TRY( arqExc, errMsg );
		m_service.onStartup();
	ARQ_END_TRY_AND_CATCH( arqExc, errMsg );
	if( arqExc.what().size() )
	{
		Log( Module::EXE ).error( arqExc, "Exception thrown when starting up service" );
		return false;
	}
	else if( errMsg.size() )
	{
		Log( Module::EXE ).error( "Exception thrown when starting up service - what: ", errMsg );
		return false;
	}

	m_service.m_ready = m_service.m_started = true;

	Log( Module::EXE ).info( "Finished starting up service" );

	return true;
}

bool ServiceRunner::runService()
{
	Log( Module::EXE ).info( "Entering main service run loop" );

	ARQ_DO_IN_TRY( arqExc, errMsg );
		m_service.run();
	ARQ_END_TRY_AND_CATCH( arqExc, errMsg );
	if( arqExc.what().size() )
	{
		Log( Module::EXE ).error( arqExc, "Unhandled exception thrown when running service" );
		return false;
	}
	else if( errMsg.size() )
	{
		Log( Module::EXE ).error( "Unhandled exception thrown when running service - what: ", errMsg );
		return false;
	}

	Log( Module::EXE ).info( "Exited main service run loop" );

	return true;
}

bool ServiceRunner::shutdownService()
{
	Log( Module::EXE ).info( "Shutting down service..." );

	m_service.m_ready = false;

	ARQ_DO_IN_TRY( arqExc, errMsg );
		m_service.onShutdown();
	ARQ_END_TRY_AND_CATCH( arqExc, errMsg );
	if( arqExc.what().size() )
	{
		Log( Module::EXE ).error( arqExc, "Exception thrown when shutting down service" );
		return false;
	}
	else if( errMsg.size() )
	{
		Log( Module::EXE ).error( "Exception thrown when shutting down service - what: ", errMsg );
		return false;
	}

	Log( Module::EXE ).info( "Finished shutting down service" );

	return true;
}

void ServiceRunner::handleSignal( int signum )
{
	if( s_instPtr )
		s_instPtr->m_service.m_shouldRun = false;
}

}