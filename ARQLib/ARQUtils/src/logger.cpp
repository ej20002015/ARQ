#include <ARQUtils/logger.h>

#include <ARQUtils/os.h>
#include <ARQUtils/time.h>
#include <ARQUtils/enum.h>
#include <ARQUtils/str.h>

#define SPDLOG_USE_STD_FORMAT
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <iostream>

namespace ARQ
{

LogLevel spdlogLevelToARQ( const spdlog::level::level_enum level )
{
	switch( level )
	{
		case spdlog::level::critical: return LogLevel::CRITICAL; break;
		case spdlog::level::err:      return LogLevel::ERRO;     break;
		case spdlog::level::warn:     return LogLevel::WARN;     break;
		case spdlog::level::info:     return LogLevel::INFO;     break;
		case spdlog::level::debug:    return LogLevel::DEBUG;    break;
		case spdlog::level::trace:    return LogLevel::TRACE;    break;
		default:
			ARQ_ASSERT( false );
			return LogLevel::CRITICAL;
	}
}

spdlog::level::level_enum ARQLevelToSpdlog( const LogLevel level )
{
	switch( level )
	{
		case LogLevel::CRITICAL: return spdlog::level::critical; break;
		case LogLevel::ERRO:     return spdlog::level::err;      break;
		case LogLevel::WARN:     return spdlog::level::warn;     break;
		case LogLevel::INFO:     return spdlog::level::info;     break;
		case LogLevel::DEBUG:    return spdlog::level::debug;    break;
		case LogLevel::TRACE:    return spdlog::level::trace;    break;
		default:
			ARQ_ASSERT( false );
			return spdlog::level::critical;
	}
}

std::atomic<Logger*> Logger::s_globalLoggerInstance = nullptr;

Logger* ARQ::Logger::globalInst()
{
	return s_globalLoggerInstance.load();
}

void ARQ::Logger::setGlobalInst( Logger* const logger )
{
	s_globalLoggerInstance.store( logger );
}

Logger::Logger( const LoggerConfig& cfg )
	: m_cfg( cfg )
{
	try
	{
		m_procName     = OS::procName();
		m_procID       = OS::procID();
		m_exeModuleStr = Str::toUpper( std::filesystem::path( m_procName ).stem().string() );

		std::vector<spdlog::sink_ptr> logSinks;
		m_primarySink   = createSink( m_cfg.primarySinkDest,   m_cfg.primarySinkLogLevel   );
		m_secondarySink = createSink( m_cfg.secondarySinkDest, m_cfg.secondarySinkLogLevel );
		if( m_primarySink )
			logSinks.push_back( m_primarySink );
		if( m_secondarySink )
			logSinks.push_back( m_secondarySink );
		logSinks.insert( logSinks.end(), m_cfg.customSinks.begin(), m_cfg.customSinks.end() );

		// Create a global thread pool with queue size of 8192 and 1 worker thread
		if( spdlog::thread_pool() == nullptr )
			spdlog::init_thread_pool( 8192, 1 );

		m_spdLogger = std::make_shared<spdlog::async_logger>( LOG_NAME, logSinks.begin(), logSinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block );
		m_spdLogger->flush_on( ARQLevelToSpdlog( LogLevel::ERRO ) );
		m_spdLogger->set_level( ARQLevelToSpdlog( LogLevel::TRACE ) ); // Set to lowest level, individual sinks will filter
		spdlog::register_logger( m_spdLogger );
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
	spdlog::drop( LOG_NAME );
}

bool Logger::shouldLog( const LogLevel level )
{
	return std::any_of( m_spdLogger->sinks().begin(), m_spdLogger->sinks().end(),
						[level] ( spdlog::sink_ptr& sinkPtr ) { return sinkPtr->should_log( ARQLevelToSpdlog( level ) ); } );
}

LogLevel Logger::getLevel() const
{
	return spdlogLevelToARQ( m_primarySink->level() );
}

LogLevel Logger::getLevel2() const
{
	return spdlogLevelToARQ( m_secondarySink->level() );
}

LogLevel Logger::getLevelForSink( const size_t sinkIndex ) const
{
	const size_t numSinks = m_spdLogger->sinks().size();
	if( sinkIndex >= numSinks )
		throw ARQException( std::format( "Logger has {} sinks - no such sink at index {}", numSinks, sinkIndex ) );

	if( m_spdLogger->sinks()[sinkIndex] )
		return spdlogLevelToARQ( m_spdLogger->sinks()[sinkIndex]->level() );
	else
		throw ARQException( std::format( "Logger sink at index {} is a nullptr ", sinkIndex ) );
}

void Logger::setLevel( const LogLevel level )
{
	m_primarySink->set_level( ARQLevelToSpdlog( level ) );
}

void Logger::setLevel2( const LogLevel level )
{
	m_secondarySink->set_level( ARQLevelToSpdlog( level ) );
}

void Logger::setLevelForSink( const size_t sinkIndex, const LogLevel level )
{
	const size_t numSinks = m_spdLogger->sinks().size();
	if( sinkIndex >= numSinks )
		throw ARQException( std::format( "Logger has {} sinks - no such sink at index {}", numSinks, sinkIndex ) );

	if( m_spdLogger->sinks()[sinkIndex] )
		m_spdLogger->sinks()[sinkIndex]->set_level( ARQLevelToSpdlog( level ) );
	else
		throw ARQException( std::format( "Logger sink at index {} is a nullptr ", sinkIndex ) );
}

void Logger::flush()
{
	m_spdLogger->flush();
}

std::shared_ptr<spdlog::sinks::sink> Logger::createSink( const std::string_view logDest, const LogLevel logLevel )
{
	std::shared_ptr<spdlog::sinks::sink> sink;

	if( logDest == "stdout" )
	{
		sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		sink->set_pattern( "%^%v%$" );
	}
	else if( logDest == "stderr" )
	{
		sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
		sink->set_pattern( "%^%v%$" );
	}
	else if( logDest == "none" )
	{
		sink = nullptr;
	}
	else
	{
		// Assume it's a file path
		static constexpr size_t MAX_SIZE = 1024 * 1024 * 5; // 5 mebibytes
		static constexpr size_t FILE_COUNT = 5;

		sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>( logDest.data(), MAX_SIZE, FILE_COUNT);
		sink->set_pattern( "%v" ); // File doesn't include colour info
		
	}

	if( sink )
		sink->set_level( ARQLevelToSpdlog( logLevel ) );

	return sink;
}

void Logger::logInternal( const LogLevel level, const std::source_location& loc, const Module module, const JSON& contextArgs, std::string&& msg, const std::optional<std::reference_wrapper<const ARQException>> exception )
{
	try
	{
		// Work out context
		JSON ctx;
		{
			std::shared_lock<std::shared_mutex> sl( Log::Context::s_globalMut );
			ctx = Log::Context::s_global;
		}
		ctx.update( Log::Context::t_thread, true );
		ctx.update( contextArgs, true );

		// Create the full log entry (use OrderedJSON to preserve order of keys)
		OrderedJSON logEntry;
		logEntry["timestamp"] = Time::DateTime::nowUTC().fmtISO8601();
		logEntry["level"] = Enum::enum_name( level );
		logEntry["proc_id"] = m_procID;
		logEntry["proc_name"] = m_procName;
		logEntry["thread_id"] = OS::threadID();
		logEntry["thread_name"] = OS::threadName();
		logEntry["module"] = module == Module::EXE ? m_exeModuleStr : Enum::enum_name( module );
		logEntry["source"] = {
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
				{ "stack", fmtStacktrace( exception->get().stack() ) }
			};
		}
		if( !ctx.empty() )
			logEntry["context"] = std::move( ctx );
		logEntry["message"] = std::move( msg );

		// Dispatch to spdlog
		m_spdLogger->log( spdlog::source_loc{}, ARQLevelToSpdlog( level ), logEntry.dump() );
	}
	catch( const ARQException& e )
	{
		// Log formatting/JSON error to stderr to avoid crashing application
		std::cerr << fmtInternalErrStr( std::format( "Logger internal error: {0}", e.what() ) ) << std::endl;
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

Logger& Log::logger()
{
	ARQ_ASSERT( Logger::globalInst() );
	if( !Logger::globalInst() )
		throw ARQException( "Global logger not initialised" );

	return *Logger::globalInst();
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
	auto getLock  = [] () { s_globalMut.lock(); };
	auto freeLock = [] () { s_globalMut.unlock(); };
	return WriteLock( &s_global, getLock, freeLock );
}

Log::Context::ReadLock Log::Context::Global::read()
{
	auto getLock  = [] () { s_globalMut.lock_shared(); };
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
	std::unique_lock<std::shared_mutex> sl( s_globalMut );
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
	std::unique_lock<std::shared_mutex> sl( s_globalMut );
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
