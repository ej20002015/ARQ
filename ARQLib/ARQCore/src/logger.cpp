#include <ARQCore/logger.h>

#include <ARQUtils/os.h>
#include <ARQUtils/time.h>

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <iostream>

namespace ARQ
{

spdlog::level::level_enum ARQLevelToSpdlog( const LogLevel level )
{
	switch( level )
	{
		case LogLevel::CRITICAL: return spdlog::level::critical; break;
		case LogLevel::ERROR:    return spdlog::level::err;      break;
		case LogLevel::WARN:     return spdlog::level::warn;     break;
		case LogLevel::INFO:     return spdlog::level::info;     break;
		case LogLevel::DEBUG:    return spdlog::level::debug;    break;
		case LogLevel::TRACE:    return spdlog::level::trace;    break;
		default:
			ARQ_ASSERT( false );
			return spdlog::level::critical;
	}
}

Logger::Logger( const LoggerConfig& cfg )
	: m_cfg( cfg )
{
	try
	{
		m_procName = OS::procName();
		m_procID   = OS::procID();

		std::vector<spdlog::sink_ptr> logSinks;
		if( !m_cfg.disableConsoleLogger )
		{
			spdlog::sink_ptr consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			consoleSink->set_level( ARQLevelToSpdlog( m_cfg.consoleLoggerLevel ) );
			consoleSink->set_pattern( "%^%v%$" );
			logSinks.push_back( consoleSink );
		}
		if( !m_cfg.disableFileLogger )
		{
			static constexpr size_t MAX_SIZE = 1024 * 1024 * 5; // 5 mebibytes
			static constexpr size_t FILE_COUNT = 5;
			if( !std::filesystem::exists( m_cfg.fileLoggerDir ) )
				std::filesystem::create_directories( m_cfg.fileLoggerDir );
			const std::filesystem::path filepath = m_cfg.fileLoggerDir / std::format( "{0}.log", m_cfg.loggerName );
			spdlog::sink_ptr fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>( filepath.string(), MAX_SIZE, FILE_COUNT );
			fileSink->set_level( ARQLevelToSpdlog( m_cfg.fileLoggerLevel ) );
			fileSink->set_pattern( "%v" ); // File doesn't include colour info
			logSinks.push_back( fileSink );
		}
		if( m_cfg.customSinks.size() )
			logSinks.insert( logSinks.end(), m_cfg.customSinks.begin(), m_cfg.customSinks.end() );
		if( logSinks.empty() )
			std::cout << "WARNING: No sinks have been added to the logger";

		// Create a global thread pool with queue size of 8192 and 1 worker thread
		if( s_initLoggerThread )
		{
			spdlog::init_thread_pool( 8192, 1 );
			s_initLoggerThread = false;
		}

		m_logger = std::make_shared<spdlog::async_logger>( m_cfg.loggerName, logSinks.begin(), logSinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block );
		m_logger->flush_on( ARQLevelToSpdlog( m_cfg.flushLevel ) );
		spdlog::register_logger( m_logger );
	}
	catch( ARQException& e )
	{
		e.what() = std::format( "Logger internal error - failed to create logger: {0}", e.what() );
		std::cerr << fmtInternalErrStr( e.what() ) << std::endl;
		throw;
	}
	catch( const std::exception& e )
	{
		std::string errMsg = std::format( "Logger internal error - failed to create logger: {0}", e.what() );
		std::cerr << fmtInternalErrStr( errMsg ) << std::endl;
		throw ARQException( errMsg );
	}
	catch( ... )
	{
		static constexpr auto errMsg = "Logger internal error: Unknown Exception";
		std::cerr << fmtInternalErrStr( errMsg ) << std::endl;
		throw ARQException( errMsg );
	}
}

Logger::~Logger()
{
	spdlog::drop( m_cfg.loggerName );
}

bool Logger::shouldLog( const LogLevel level )
{
	return m_logger->should_log( ARQLevelToSpdlog( level ) );
}

void Logger::setLevel( const LogLevel level )
{
	m_logger->set_level( ARQLevelToSpdlog( level ) );
}

void Logger::flush()
{
	m_logger->flush();
}

void Logger::logInternal( const LogLevel level, const std::source_location& loc, const Module module, const JSON& contextArgs, std::string&& msg, const std::optional<std::reference_wrapper<const ARQException>> exception )
{
	try
	{
		// Work out context
		JSON ctx;
		{
			std::shared_lock<std::shared_mutex> l( Log::Context::s_globalMut );
			ctx = Log::Context::s_global;
		}
		ctx.update( Log::Context::t_thread, true );
		ctx.update( contextArgs, true );

		// Create the full log entry (use OrderedJSON to preserve order of keys)
		OrderedJSON logEntry;
		logEntry["timestamp"]      = Time::DateTime::nowUTC().fmtISO8601();
		logEntry["level"]          = LOG_LEVEL_STRS[static_cast<size_t>( level )];
		logEntry["proc_id"]        = m_procID;
		logEntry["proc_name"]      = m_procName;
		logEntry["thread_id"]      = OS::threadID();
		logEntry["thread_name"]    = OS::threadName();
		logEntry["module"]         = MODULE_STRS[static_cast<size_t>( module )];
		logEntry["source"]         = {
			{ "file",     loc.file_name() },
			{ "line",     loc.line() },
			{ "function", loc.function_name() }
		};
		if( exception.has_value() )
		{
			logEntry["exception"] = {
				{ "where", {
					{ "file",     exception->get().where().file_name() },
					{ "line",     exception->get().where().line() },
					{ "function", exception->get().where().function_name() }
				} },
				{ "what",  exception->get().what() },
			};
		}
		if( !ctx.empty() )
			logEntry["context"]    = std::move( ctx );
		logEntry["message"]        = std::move( msg );

		// Dispatch to spdlog
		m_logger->log( spdlog::source_loc{}, ARQLevelToSpdlog( level ), logEntry.dump() );
	}
	catch( const ARQException& e )
	{
		// Log formatting/JSON error to stderr to avoid crashing application
		std::cerr << fmtInternalErrStr( std::format( "Logger internal error: {0}",  e.what() ) ) << std::endl;
	}
	catch( const std::exception& e )
	{
		std::cerr << fmtInternalErrStr( std::format( "Logger internal error: {0}", e.what() ) ) << std::endl;
	}
	catch( ... )
	{
		std::cerr << fmtInternalErrStr( "Logger internal error: Unknown Exception" ) << std::endl;
	}
}

// TODO: Get rid of this guard!!!!!
static struct LoggerGuard
{
	LoggerGuard()
	{
		Log::init();
	}

	~LoggerGuard()
	{
		Log::fini();
	}
} lg;

std::unique_ptr<Logger> Log::s_globalLogger;

void Log::init( const LoggerConfig& cfg )
{
	s_globalLogger = std::make_unique<Logger>( cfg );
}

void Log::fini()
{
	//s_globalLogger.reset();
	spdlog::shutdown();
	s_initLoggerThread = true;
}

bool Log::shouldLog( const LogLevel level )
{
	if( s_globalLogger )
		return s_globalLogger->shouldLog( level );

	return false;
}

void Log::setLevel( const LogLevel level )
{
	if( s_globalLogger )
		s_globalLogger->setLevel( level );
}

void Log::flush()
{
	if( s_globalLogger )
		s_globalLogger->flush();
}

JSON Log::Context::s_global = JSON::object();
thread_local JSON Log::Context::t_thread = JSON::object();
std::shared_mutex Log::Context::s_globalMut;

Log::Context::WriteLock::WriteLock( JSON* const context, const std::function<void( void )>& onConstruction, const std::function<void( void )>& onDestruction )
	: m_context( context )
	, m_onDestruction( onDestruction )
{
	if( onConstruction )
		onConstruction();
}

Log::Context::WriteLock::~WriteLock()
{
	if( m_onDestruction )
		m_onDestruction();
}

Log::Context::ReadLock::ReadLock( const JSON* const context, const std::function<void( void )>& onConstruction, const std::function<void( void )>& onDestruction )
	: m_context( context )
	, m_onDestruction( onDestruction )
{
	if( onConstruction )
		onConstruction();
}

Log::Context::ReadLock::~ReadLock()
{
	if( m_onDestruction )
		m_onDestruction();
}

Log::Context::WriteLock Log::Context::Global::write()
{
	auto getLock =  [] () { s_globalMut.lock(); };
	auto freeLock = [] () { s_globalMut.unlock(); };
	return WriteLock( &s_global, getLock, freeLock );
}

Log::Context::ReadLock Log::Context::Global::read()
{
	auto getLock =  [] () { s_globalMut.lock_shared(); };
	auto freeLock = [] () { s_globalMut.unlock_shared(); };
	return ReadLock( &s_global, getLock, freeLock );
}

Log::Context::WriteLock Log::Context::Thread::write()
{
	return WriteLock( &t_thread, std::function<void( void )>(), std::function<void( void )>() );
}

Log::Context::ReadLock Log::Context::Thread::read()
{
	return ReadLock( &t_thread, std::function<void( void )>(), std::function<void( void )>() );
}

Log::Context::Global::Scoped::Scoped( const JSON& contextArgs )
{
	if( !contextArgs.is_object() )
		throw ARQException( "Attempting to create Log::Context::Global::Scoped with non-object JSON" );

	std::unique_lock<std::shared_mutex>( s_globalMut );
	JSON& targetJson = s_global;

	for( const auto& [key, newValue] : contextArgs.items() )
	{
		auto it = targetJson.find( key );
		if( it != targetJson.end() )
			m_prevState[key] = it.value();
		else
			m_prevState[key] = std::nullopt;

		targetJson[key] = newValue;
	}
}

Log::Context::Global::Scoped::~Scoped()
{
	std::unique_lock<std::shared_mutex>( s_globalMut );
	JSON& targetJson = s_global;

	for( const auto& [key, prevValue] : m_prevState )
	{
		if( prevValue.has_value() )
			targetJson[key] = prevValue.value();
		else
			targetJson.erase( key );
	}
}

Log::Context::Thread::Scoped::Scoped( const JSON& contextArgs )
{
	if( !contextArgs.is_object() )
		throw ARQException( "Attempting to create Log::Context::Thread::Scoped with non-object JSON" );

	JSON& targetJson = t_thread;

	for( const auto& [key, newValue] : contextArgs.items() )
	{
		auto it = targetJson.find( key );
		if( it != targetJson.end() )
			m_prevState[key] = it.value();
		else
			m_prevState[key] = std::nullopt;

		targetJson[key] = newValue;
	}
}

Log::Context::Thread::Scoped::~Scoped()
{
	JSON& targetJson = t_thread;

	for( const auto& [key, prevValue] : m_prevState )
	{
		if( prevValue.has_value() )
			targetJson[key] = prevValue.value();
		else
			targetJson.erase( key );
	}
}

}
