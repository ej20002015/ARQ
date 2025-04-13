#pragma once
#include <TMQCore/dll.h>

#ifndef SPDLOG_USE_STD_FORMAT
#define SPDLOG_USE_STD_FORMAT
#endif
#define FMT_UNICODE 0

#include <spdlog/spdlog.h>

#include <source_location>

namespace TMQ
{

class Logger
{
public:
	TMQCore_API Logger();
	TMQCore_API ~Logger();

	template<typename... Args>
	void critical( const std::source_location& loc, const std::format_string<Args...> fmt, Args&&... args )
	{ 
		m_logger->log( spdlog::source_loc{ loc.file_name(), loc.line(), loc.function_name() }, spdlog::level::level_enum::critical, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void critical( const std::format_string<Args...> fmt, Args&&... args )
	{
		m_logger->critical( fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void error( const std::source_location& loc, const std::format_string<Args...> fmt, Args&&... args )
	{ 
		m_logger->log( spdlog::source_loc{ loc.file_name(), loc.line(), loc.function_name() }, spdlog::level::level_enum::err, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void error( const std::format_string<Args...> fmt, Args&&... args )
	{
		m_logger->error( fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void warn( const std::source_location& loc, const std::format_string<Args...> fmt, Args&&... args )
	{
		m_logger->log( spdlog::source_loc{ loc.file_name(), loc.line(), loc.function_name() }, spdlog::level::level_enum::warn, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void warn( const std::format_string<Args...> fmt, Args&&... args )
	{
		m_logger->warn( fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void info( const std::source_location& loc, const std::format_string<Args...> fmt, Args&&... args )
	{
		m_logger->log( spdlog::source_loc{ loc.file_name(), static_cast<int>( loc.line() ), loc.function_name() }, spdlog::level::level_enum::info, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void info( const std::format_string<Args...> fmt, Args&&... args )
	{
		m_logger->info( fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void debug( const std::source_location& loc, const std::format_string<Args...> fmt, Args&&... args )
	{
		m_logger->log( spdlog::source_loc{ loc.file_name(), loc.line(), loc.function_name() }, spdlog::level::level_enum::debug, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void debug( const std::format_string<Args...> fmt, Args&&... args )
	{
		m_logger->debug( fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void trace( const std::source_location& loc, const std::format_string<Args...> fmt, Args&&... args )
	{
		m_logger->log( spdlog::source_loc{ loc.file_name(), loc.line(), loc.function_name() }, spdlog::level::level_enum::trace, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void trace( const std::format_string<Args...> fmt, Args&&... args )
	{
		m_logger->trace( fmt, std::forward<Args>( args )... );
	}

private:
	std::shared_ptr<spdlog::logger> m_logger;
};

enum class LogModule
{
	REFDATA
};

class Log
{
public:
	TMQCore_API static void init();
	TMQCore_API static void fini();

	Log( const std::source_location& loc = std::source_location::current() )
		: m_loc( loc )
	{}

	template<typename... Args>
	void critical( const std::format_string<Args...> fmt, Args&&... args )
	{ 
		s_globalLogger->critical( m_loc, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void error( const std::format_string<Args...> fmt, Args&&... args )
	{ 
		s_globalLogger->error( m_loc, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void warn( const std::format_string<Args...> fmt, Args&&... args )
	{ 
		s_globalLogger->warn( m_loc, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void info( const std::format_string<Args...> fmt, Args&&... args )
	{ 
		s_globalLogger->info( m_loc, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void debug( const std::format_string<Args...> fmt, Args&&... args )
	{ 
		s_globalLogger->debug( m_loc, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void trace( const std::format_string<Args...> fmt, Args&&... args )
	{ 
		s_globalLogger->trace( m_loc, fmt, std::forward<Args>( args )... );
	}

private:
	TMQCore_API static std::unique_ptr<Logger> s_globalLogger;

	std::source_location m_loc;
};

}