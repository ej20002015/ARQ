#pragma once
#include <ARQCore/dll.h>

#include <ARQUtils/core.h>
#include <ARQUtils/error.h>
#include <ARQUtils/json.h>
#include <ARQUtils/sys.h>

#include <source_location>
#include <map>
#include <shared_mutex>
#include <optional>


// Fwd declare
namespace spdlog { class logger; }
namespace spdlog::sinks { class sink; }

namespace ARQ
{

enum class LogLevel
{
	CRITICAL,
	ERROR,
	WARN,
	INFO,
	DEBUG,
	TRACE,

	_SIZE
};

static constexpr const char* const LOG_LEVEL_STRS[static_cast<size_t>( LogLevel::_SIZE )] = { "CRITICAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE" };

static constexpr auto DEFAULT_LOGGER_NAME = "ARQLib";

struct LoggerConfig
{
	std::string loggerName = DEFAULT_LOGGER_NAME;

	bool disableConsoleLogger = false;
	bool disableFileLogger    = false;

	LogLevel consoleLoggerLevel = LogLevel::INFO;
	LogLevel fileLoggerLevel    = LogLevel::DEBUG;

	LogLevel flushLevel         = LogLevel::ERROR;

	std::filesystem::path fileLoggerDir = Sys::logDir();

	std::vector<std::shared_ptr<spdlog::sinks::sink>> customSinks;
};

class Logger
{
public:
	ARQCore_API Logger( const LoggerConfig& cfg = LoggerConfig() );
	ARQCore_API ~Logger();

	template<typename... Args>
	void log( const LogLevel level, const std::source_location& loc, const Module module, const JSON& contextArgs, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( !shouldLog( level ) )
			return;

		std::string msg = std::format( fmt, std::forward<Args>( args )... );
		logInternal( level, loc, module, contextArgs, std::move( msg ) );
	}

	template<typename... Args>
	void logException( const ARQException& exception, const LogLevel level, const std::source_location& loc, const Module module, const JSON& contextArgs, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( !shouldLog( level ) )
			return;

		std::string msg = std::format( fmt, std::forward<Args>( args )... );
		logInternal( level, loc, module, contextArgs, std::move( msg ), exception );
	}

	[[nodiscard]] bool shouldLog( const LogLevel level );

	void setLevel( const LogLevel level );

	void flush();

private:
	void logInternal( const LogLevel level, const std::source_location& loc, const Module module, const JSON& contextArgs, std::string&& msg, const std::optional<std::reference_wrapper< const ARQException>> exception = std::nullopt );

private:
	LoggerConfig m_cfg;

	std::shared_ptr<spdlog::logger> m_logger;

	std::string m_procName;
	int32_t m_procID;
};

inline static bool s_initLoggerThread = true;

class Log
{
public:
	ARQCore_API static void init( const LoggerConfig& cfg = LoggerConfig() );
	ARQCore_API static void fini();

	[[nodiscard]] ARQCore_API static bool shouldLog( const LogLevel level );

	ARQCore_API static void setLevel( const LogLevel level );

	ARQCore_API static void flush();

	Log( const Module module, const JSON& contextArgs = JSON::object(), const std::source_location& loc = std::source_location::current() )
		: m_module( module )
		, m_loc( loc )
		, m_contextArgs( contextArgs )
	{
	}

	template<typename... Args>
	void critical( const std::format_string<Args...> fmt, Args&&... args )
	{
		if( s_globalLogger )
			s_globalLogger->log( LogLevel::CRITICAL, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void error( const std::format_string<Args...> fmt, Args&&... args )
	{
		if( s_globalLogger )
			s_globalLogger->log( LogLevel::ERROR, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void warn( const std::format_string<Args...> fmt, Args&&... args )
	{
		if( s_globalLogger )
			s_globalLogger->log( LogLevel::WARN, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void info( const std::format_string<Args...> fmt, Args&&... args )
	{
		if( s_globalLogger )
			s_globalLogger->log( LogLevel::INFO, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void debug( const std::format_string<Args...> fmt, Args&&... args )
	{
		if( s_globalLogger )
			s_globalLogger->log( LogLevel::DEBUG, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void trace( const std::format_string<Args...> fmt, Args&&... args )
	{
		if( s_globalLogger )
			s_globalLogger->log( LogLevel::TRACE, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void critical( const ARQException& exception, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( s_globalLogger )
			s_globalLogger->logException( exception, LogLevel::CRITICAL, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void error( const ARQException& exception, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( s_globalLogger )
			s_globalLogger->logException( exception, LogLevel::ERROR, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void warn( const ARQException& exception, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( s_globalLogger )
			s_globalLogger->logException( exception, LogLevel::WARN, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void info( const ARQException& exception, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( s_globalLogger )
			s_globalLogger->logException( exception, LogLevel::INFO, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void debug( const ARQException& exception, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( s_globalLogger )
			s_globalLogger->logException( exception, LogLevel::DEBUG, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void trace( const ARQException& exception, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( s_globalLogger )
			s_globalLogger->logException( exception, LogLevel::TRACE, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}

public:

	class Context
	{
	public:

		class WriteLock
		{
		public:
			ARQCore_API ~WriteLock();
			WriteLock( const WriteLock& ) = delete;
			WriteLock& operator=( const WriteLock& ) = delete;

			ARQCore_API JSON& operator->() { return *m_context; }
			ARQCore_API JSON& operator*() { return *m_context; }

		private:
			WriteLock( JSON* const context, const std::function<void( void )>& onConstruction, const std::function<void( void )>& onDestruction );

		private:
			JSON* const m_context;
			std::function<void( void )> m_onDestruction;

			friend class Context;
		};

		class ReadLock
		{
		public:
			ARQCore_API ~ReadLock();
			ReadLock( const ReadLock& ) = delete;
			ReadLock& operator=( const ReadLock& ) = delete;

			ARQCore_API const JSON& operator->() const { return *m_context; }
			ARQCore_API const JSON& operator*()  const { return *m_context; }

		private:
			ReadLock( const JSON* const context, const std::function<void( void )>& onConstruction, const std::function<void( void )>& onDestruction );

		private:
			const JSON* const m_context;
			std::function<void( void )> m_onDestruction;

			friend class Context;
		};

	public:

		class Global
		{
		public:

			class Scoped
			{
			public:
				ARQCore_API explicit Scoped( const JSON& contextArgs );
				ARQCore_API ~Scoped();

				Scoped( const Scoped& ) = delete;
				Scoped& operator=( const Scoped& ) = delete;

			private:
				std::map<std::string, std::optional<JSON>> m_prevState;
			};

			[[nodiscard]] ARQCore_API static ReadLock  read();
			[[nodiscard]] ARQCore_API static WriteLock write();
		};

		class Thread
		{
		public:

			class Scoped
			{
			public:
				ARQCore_API explicit Scoped( const JSON& contextArgs );
				ARQCore_API ~Scoped();

				Scoped( const Scoped& ) = delete;
				Scoped& operator=( const Scoped& ) = delete;

			private:
				std::map<std::string, std::optional<JSON>> m_prevState;
			};

			[[nodiscard]] ARQCore_API static ReadLock  read();
			[[nodiscard]] ARQCore_API static WriteLock write();
		};

	private:

		ARQCore_API static              JSON s_global;
		            static thread_local JSON t_thread;

		ARQCore_API static std::shared_mutex s_globalMut;

		friend class Logger;
	};

private:
	ARQCore_API static std::unique_ptr<Logger> s_globalLogger;

	Module m_module;
	std::source_location m_loc;
	JSON m_contextArgs;
};

}