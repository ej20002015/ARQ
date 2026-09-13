#include <ARQMarket/market.h>

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace
{

using namespace ARQ::MD;
using namespace ARQ::Time;

std::string makeFXRateID( const std::size_t index )
{
    return "FX/" + std::to_string( index );
}

Record<FXRate> makeFXRate( const std::size_t index, const std::uint64_t timestamp, const double mid )
{
    Record<FXRate> record;
    record.header.id            = makeFXRateID( index );
    record.header.asofTs        = DateTime( Microseconds( timestamp ) );
    record.header.lastUpdatedTs = record.header.asofTs;
    record.data.mid             = mid;
    record.data.bid             = mid - 0.0001;
    record.data.ask             = mid + 0.0001;
    return record;
}

RecordCollection makeFXRates( const std::size_t count, const std::uint64_t timestamp, const double mid )
{
    RecordCollection records;
    auto& fxRates = records.get<Record<FXRate>>();
    fxRates.reserve( count );

    for( std::size_t index = 0; index < count; ++index )
        fxRates.push_back( makeFXRate( index, timestamp, mid ) );

    return records;
}

class MarketBenchmark : public benchmark::Fixture
{
public:
    void SetUp( const benchmark::State& state ) override
    {
        const auto marketSize = static_cast<std::size_t>( state.range( 0 ) );
        RecordCollection baseline = makeFXRates( marketSize, 1, 1.0 );
        m_market.update( std::move( baseline ) );
        m_readID = makeFXRateID( marketSize / 2 );
    }

protected:
    Market      m_market;
    std::string m_readID;
};

BENCHMARK_DEFINE_F( MarketBenchmark, UpdateSnapshot )( benchmark::State& state )
{
    const auto updateSize = static_cast<std::size_t>( state.range( 1 ) );
    const RecordCollection updateTemplate = makeFXRates( updateSize, 2, 1.1 );

    for( auto _ : state )
    {
        state.PauseTiming();
        RecordCollection updates = updateTemplate;
        state.ResumeTiming();

        m_market.update( std::move( updates ) );
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed( state.iterations() * static_cast<std::int64_t>( updateSize ) );
}

BENCHMARK_REGISTER_F( MarketBenchmark, UpdateSnapshot )
    ->Args( { 32, 1 } )
    ->Args( { 512, 1 } )
    ->Args( { 8192, 1 } )
    ->Args( { 8192, 32 } )
    ->ArgNames( { "MarketSize", "UpdateSize" } );

BENCHMARK_DEFINE_F( MarketBenchmark, AcquireSnapshotAndReadFXRate )( benchmark::State& state )
{
    for( auto _ : state )
    {
        const std::shared_ptr<const MarketSnapshot> snapshot = m_market.snapshot();
        const ARQ::OptConstRef<Record<FXRate>>       record   = snapshot->get<FXRate>( m_readID );

        benchmark::DoNotOptimize( snapshot.get() );
        benchmark::DoNotOptimize( record->data.mid );
    }

    state.SetItemsProcessed( state.iterations() );
}

BENCHMARK_REGISTER_F( MarketBenchmark, AcquireSnapshotAndReadFXRate )
    ->Arg( 32 )
    ->Arg( 512 )
    ->Arg( 8192 )
    ->ArgName( "MarketSize" );

} // namespace

BENCHMARK_MAIN();
