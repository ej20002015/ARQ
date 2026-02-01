#pragma once
#include <ARQUtils/dll.h>

#include <ARQUtils/core.h>
#include <ARQUtils/error.h>
#include <ARQUtils/json.h>
#include <ARQUtils/sys.h>

#include <source_location>
#include <map>
#include <shared_mutex>
#include <optional>
#include <atomic>

// Fwd declare
namespace spdlog { class logger; }
namespace spdlog::sinks { class sink; }

namespace ARQ
{

enum class LogLevel
{
	CRITICAL,
	ERRO,
	WARN,
	INFO,
	DEBUG,
	TRACE,
};

static constexpr LogLevel DEFAULT_LOG_LEVEL = LogLevel::INFO;
static constexpr auto     LOG_NAME          = "ARQLib";

struct LoggerConfig
{
	LogLevel    primarySinkLogLevel   = DEFAULT_LOG_LEVEL;
	LogLevel    secondarySinkLogLevel = DEFAULT_LOG_LEVEL;
	std::string primarySinkDest       = "stdout";
	std::string secondarySinkDest     = ( Sys::logDir() / "ARQLib.log" ).string();

	std::vector<std::shared_ptr<spdlog::sinks::sink>> customSinks;
};

class Logger
{
public:
	static ARQUtils_API Logger* globalInst();
	static ARQUtils_API void    setGlobalInst( Logger* const logger );

public:
	ARQUtils_API Logger( const LoggerConfig& cfg = LoggerConfig() );
	ARQUtils_API ~Logger();

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

	[[nodiscard]] ARQUtils_API bool shouldLog( const LogLevel level );

	ARQUtils_API LogLevel getLevel () const;
	ARQUtils_API LogLevel getLevel2() const;
	ARQUtils_API LogLevel getLevelForSink( const size_t sinkIndex ) const;
	ARQUtils_API void     setLevel ( const LogLevel level );
	ARQUtils_API void     setLevel2( const LogLevel level );
	ARQUtils_API void     setLevelForSink( const size_t sinkIndex, const LogLevel level );

	ARQUtils_API void flush();

private:
	std::shared_ptr<spdlog::sinks::sink> createSink( const std::string_view logDest, const LogLevel logLevel );

	ARQUtils_API void logInternal( const LogLevel level, const std::source_location& loc, const Module module, const JSON& contextArgs, std::string&& msg, const std::optional<std::reference_wrapper< const ARQException>> exception = std::nullopt );

private:
	ARQUtils_API static std::atomic<Logger*> s_globalLoggerInstance;

private:
	LoggerConfig m_cfg;

	std::shared_ptr<spdlog::logger>      m_spdLogger;
	std::shared_ptr<spdlog::sinks::sink> m_primarySink;
	std::shared_ptr<spdlog::sinks::sink> m_secondarySink;

	std::string m_procName;
	int32_t     m_procID;
	std::string m_exeModuleStr;
};

inline static bool s_initLoggerThread = true;

class Log
{
public:
	static ARQUtils_API Logger& logger();

	Log( const Module module, const JSON& contextArgs = JSON::object(), const std::source_location& loc = std::source_location::current() )
		: m_module( module )
		, m_loc( loc )
		, m_contextArgs( contextArgs )
	{
	}

	template<typename... Args>
	void critical( const std::format_string<Args...> fmt, Args&&... args )
	{
		if( Logger::globalInst() )
			Logger::globalInst()->log( LogLevel::CRITICAL, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void error( const std::format_string<Args...> fmt, Args&&... args )
	{
		if( Logger::globalInst() )
			Logger::globalInst()->log( LogLevel::ERRO, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void warn( const std::format_string<Args...> fmt, Args&&... args )
	{
		if( Logger::globalInst() )
			Logger::globalInst()->log( LogLevel::WARN, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void info( const std::format_string<Args...> fmt, Args&&... args )
	{
		if( Logger::globalInst() )
			Logger::globalInst()->log( LogLevel::INFO, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void debug( const std::format_string<Args...> fmt, Args&&... args )
	{
		if( Logger::globalInst() )
			Logger::globalInst()->log( LogLevel::DEBUG, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void trace( const std::format_string<Args...> fmt, Args&&... args )
	{
		if( Logger::globalInst() )
			Logger::globalInst()->log( LogLevel::TRACE, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}

	template<typename... Args>
	void critical( const ARQException& exception, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( Logger::globalInst() )
			Logger::globalInst()->logException( exception, LogLevel::CRITICAL, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void error( const ARQException& exception, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( Logger::globalInst() )
			Logger::globalInst()->logException( exception, LogLevel::ERRO, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void warn( const ARQException& exception, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( Logger::globalInst() )
			Logger::globalInst()->logException( exception, LogLevel::WARN, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void info( const ARQException& exception, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( Logger::globalInst() )
			Logger::globalInst()->logException( exception, LogLevel::INFO, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void debug( const ARQException& exception, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( Logger::globalInst() )
			Logger::globalInst()->logException( exception, LogLevel::DEBUG, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}
	template<typename... Args>
	void trace( const ARQException& exception, const std::format_string<Args...> fmt, Args&&... args )
	{
		if( Logger::globalInst() )
			Logger::globalInst()->logException( exception, LogLevel::TRACE, m_loc, m_module, m_contextArgs, fmt, std::forward<Args>( args )... );
	}

public:

	class Context
	{
	public:

		class WriteLock
		{
		public:
			ARQUtils_API ~WriteLock();
			WriteLock( const WriteLock& ) = delete;
			WriteLock& operator=( const WriteLock& ) = delete;

			ARQUtils_API JSON& operator->() { return *m_context; }
			ARQUtils_API JSON& operator*() { return *m_context; }

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
			ARQUtils_API ~ReadLock();
			ReadLock( const ReadLock& ) = delete;
			ReadLock& operator=( const ReadLock& ) = delete;

			ARQUtils_API const JSON& operator->() const { return *m_context; }
			ARQUtils_API const JSON& operator*()  const { return *m_context; }

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
				ARQUtils_API explicit Scoped( const JSON& contextArgs );
				ARQUtils_API ~Scoped();

				Scoped( const Scoped& ) = delete;
				Scoped& operator=( const Scoped& ) = delete;

			private:
				std::map<std::string, std::optional<JSON>> m_prevState;
			};

			[[nodiscard]] ARQUtils_API static ReadLock  read();
			[[nodiscard]] ARQUtils_API static WriteLock write();
		};

		class Thread
		{
		public:

			class Scoped
			{
			public:
				ARQUtils_API explicit Scoped( const JSON& contextArgs );
				ARQUtils_API ~Scoped();

				Scoped( const Scoped& ) = delete;
				Scoped& operator=( const Scoped& ) = delete;

			private:
				std::map<std::string, std::optional<JSON>> m_prevState;
			};

			[[nodiscard]] ARQUtils_API static ReadLock  read();
			[[nodiscard]] ARQUtils_API static WriteLock write();
		};

	private:

		ARQUtils_API static              JSON s_global;
		             static thread_local JSON t_thread;

		ARQUtils_API static std::shared_mutex s_globalMut;

		friend class Logger;
	};

private:
	Module               m_module;
	std::source_location m_loc;
	JSON                 m_contextArgs;
};

}
