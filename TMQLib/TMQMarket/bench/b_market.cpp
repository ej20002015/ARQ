#include <benchmark/benchmark.h>
#include <TMQMarket/market.h>

#include <random>

constexpr int NUM_ENTRIES = 100;
TMQ::Market market;

// Populate MarketStore with some initial data
static void SetupMarket() {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(1.0, 100.0);

    for (int i = 0; i < NUM_ENTRIES; ++i) {
        std::string id = "FX_" + std::to_string(i);
        market.setFxPair(id, TMQ::MktType::FXPair(dist(rng), dist(rng), dist(rng)));

        id = "EQ_" + std::to_string(i);
        market.setEquity(id, TMQ::MktType::Equity(dist(rng), dist(rng), dist(rng), dist(rng), dist(rng)));
    }
}

// Benchmark FX Pair Reads
static void Benchmark_FX_Read(benchmark::State& state) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> idDist(0, NUM_ENTRIES - 1);

    for (auto _ : state) {
        std::string id = "FX_" + std::to_string(idDist(rng));
        benchmark::DoNotOptimize(market.getFxPair(id));
    }
}
BENCHMARK(Benchmark_FX_Read);

// Benchmark FX Pair Writes
static void Benchmark_FX_Write(benchmark::State& state) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(1.0, 100.0);

    for (auto _ : state) {
        std::string id = "FX_" + std::to_string(state.iterations() % NUM_ENTRIES);
        market.setFxPair(id, TMQ::MktType::FXPair(dist(rng), dist(rng), dist(rng)));
    }
}
BENCHMARK(Benchmark_FX_Write);

int main(int argc, char** argv) {
    SetupMarket();
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    return 0;
}
