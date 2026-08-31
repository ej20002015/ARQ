#include <ARQUtils/logger.h>

#include <benchmark/benchmark.h>

#define SPDLOG_USE_STD_FORMAT
#include <spdlog/sinks/null_sink.h>

#include <cstdint>
#include <memory>

using namespace ARQ;

class LoggerBenchmark : public benchmark::Fixture
{
public:
    void SetUp( const benchmark::State& ) override
    {
        LoggerConfig cfg;
        cfg.primarySinkDest   = "none";
        cfg.secondarySinkDest = "none";
        cfg.customSinks       = { std::make_shared<spdlog::sinks::null_sink_mt>() };
        cfg.logName           = "LoggerBenchmark";

        m_previousLogger = Logger::getGlobalInstPtr();
        m_logger         = std::make_unique<Logger>( cfg );
        Logger::setGlobalInst( m_logger.get() );
    }

    void TearDown( const benchmark::State& ) override
    {
        m_logger->flush();
        Logger::setGlobalInst( m_previousLogger );
        m_logger.reset();
    }

private:
    Logger*                 m_previousLogger = nullptr;
    std::unique_ptr<Logger> m_logger;
};

BENCHMARK_F( LoggerBenchmark, InfoLogging )( benchmark::State& state )
{
    uint64_t messageNumber = 0;
    for( auto _ : state )
        Log( Module::CORE ).info( "Hello this is a benchmark: num {0}", messageNumber++ );

    state.SetItemsProcessed( state.iterations() );
}

BENCHMARK_MAIN();
