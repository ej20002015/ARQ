//#include <benchmark/benchmark.h>
//#include <ARQMarket/market.h>
//
//#include <random>
//
//using namespace ARQ;
//
//constexpr int NUM_ENTRIES = 100;
//Mkt::Market market;
//
//// Populate MarketStore with some initial data
//static void SetupMarket()
//{
//    std::mt19937 rng( std::random_device{}() );
//    std::uniform_real_distribution<double> dist( 1.0, 100.0 );
//
//    for( int i = 0; i < NUM_ENTRIES; ++i )
//    {
//        const std::string rateID = "FX_" + std::to_string(i);
//        ARQ::MDEntities::FXRate rate;
//        rate.ID = rateID;
//        rate.mid = dist( rng );
//        rate.ask = dist( rng );
//        rate.bid = dist( rng );
//        market.setFXRate( rate );
//
//        const std::string eqID = "EQ_" + std::to_string( i );
//        ARQ::MDEntities::EQPrice eq;
//        eq.ID = eqID;
//        eq.ask = dist( rng );
//        eq.bid = dist( rng );
//        eq.open = dist( rng );
//        eq.close = dist( rng );
//        market.setEQPrice( eq );
//    }
//}
//
//static void BenchmarkFXRead( benchmark::State& state )
//{
//    std::mt19937 rng( std::random_device{}() );
//    std::uniform_int_distribution<int> idDist ( 0, NUM_ENTRIES - 1 );
//
//    for( auto _ : state )
//    {
//        const std::string id = "FX_" + std::to_string( idDist( rng ) );
//        benchmark::DoNotOptimize( market.getFXRate( id ) );
//    }
//}
//BENCHMARK(BenchmarkFXRead);
//
//static void BenchmarkFXWrite( benchmark::State& state )
//{
//    std::mt19937 rng (std::random_device{}() );
//    std::uniform_int_distribution<int> idDist( 0, NUM_ENTRIES - 1 );
//    std::uniform_real_distribution<double> dist( 1.0, 100.0 );
//
//    for( auto _ : state )
//    {
//        const std::string id = "FX_" + std::to_string( state.iterations() % NUM_ENTRIES );
//        ARQ::MDEntities::FXRate rate;
//        rate.ID = id;
//        rate.mid = dist( rng );
//        rate.ask = dist( rng );
//        rate.bid = dist( rng );
//        market.setFXRate( rate );
//    }
//}
//BENCHMARK( BenchmarkFXWrite );

int main(int argc, char** argv)
{
    /*SetupMarket();
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();*/
    return 0;
}
