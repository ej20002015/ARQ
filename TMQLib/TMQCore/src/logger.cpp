#include <TMQCore/logger.h>

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace TMQ
{

static constexpr auto LOGGER_NAME = "TMQLib";

Logger::Logger()
{
	// TODO: Number of log destinations, what type, and log level should be configurable
	std::vector<spdlog::sink_ptr> logSinks;
	logSinks.emplace_back( std::make_shared<spdlog::sinks::stdout_color_sink_mt>() );
	static constexpr size_t MAX_SIZE = 1024 * 1024 * 5; // 5 mebibytes
	static constexpr size_t FILE_COUNT = 5;
	logSinks.emplace_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>( R"(C:\tmp\TMQ.log)", MAX_SIZE, FILE_COUNT ) ); // TODO: get log path depending on OS

	// Format as structured json
	logSinks[0]->set_pattern( // Console
		"{\"time\": \"%Y-%m-%dT%H:%M:%S.%f%z\", "
		"\"name\": \"%n\", "
		"\"level\": \"%^%l%$\", "
		"\"process\": %P, "
		"\"thread\": %t, "
		"\"location\": %@, "
		"\"function\": %!, "
		"\"message\": \"%v\"}"
	);
	logSinks[1]->set_pattern( // File (doesn't include color info)
		"{\"time\": \"%Y-%m-%dT%H:%M:%S.%f%z\", "
		"\"name\": \"%n\", "
		"\"level\": \"%l\", "
		"\"process\": %P, "
		"\"thread\": %t, "
		"\"location\": %@, "
		"\"function\": %!, "
		"\"message\": \"%v\"}"
	);

	// Create a global thread pool with queue size of 8192 and 1 worker thread
	spdlog::init_thread_pool( 8192, 1 );
	m_logger = std::make_shared<spdlog::async_logger>( LOGGER_NAME, logSinks.begin(), logSinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block );
	m_logger->set_level( spdlog::level::trace );
	m_logger->flush_on( spdlog::level::err );
	spdlog::register_logger( m_logger );
}

Logger::~Logger()
{
	spdlog::drop( LOGGER_NAME );
}

std::unique_ptr<Logger> Log::s_globalLogger;

void Log::init()
{
	s_globalLogger = std::make_unique<Logger>();
}

void Log::fini()
{
	s_globalLogger.reset();
	spdlog::shutdown();
}

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

}
