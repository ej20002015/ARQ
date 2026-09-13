# ARQ Benchmark Results

This branch is updated by the weekly benchmark workflow. Raw Google Benchmark JSON
is retained with each workflow run as an artifact.

**Latest run:** 2026-09-13T20:03:21Z

| Context | Value |
|---|---|
| Commit | [66ba37ed526f](https://github.com/ej20002015/ARQ/commit/66ba37ed526f5c480b1db10f54710a35549647d4) |
| Workflow | [run 34779350955](https://github.com/ej20002015/ARQ/actions/runs/34779350955) |
| Branch | `master` |
| Runner | ubuntu-24.04 |
| Compiler | GCC 14.3.0 |
| CPU | AMD EPYC 9V74 80-Core Processor |
| Logical CPUs | 4 |
| Google Benchmark | v1.9.1 |

## Latest measurements

| Suite | Benchmark | Median CPU | Change | Median real | Throughput | CPU CV |
|---|---|---:|---:|---:|---:|---:|
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:32` | 9.76 ns | new baseline | 9.76 ns | 102 M/s | 2.13% |
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:512` | 9.41 ns | new baseline | 9.41 ns | 106 M/s | 0.38% |
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:8192` | 9.22 ns | new baseline | 9.23 ns | 108 M/s | 0.24% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:32/UpdateSize:1` | 56.3 µs | new baseline | 56.1 µs | 17.8 k/s | 1.11% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:512/UpdateSize:1` | 56.4 µs | new baseline | 56.2 µs | 17.7 k/s | 3.21% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:8192/UpdateSize:1` | 55.6 µs | new baseline | 55.4 µs | 18 k/s | 1.56% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:8192/UpdateSize:32` | 57.6 µs | new baseline | 57.5 µs | 555 k/s | 2.36% |
| ARQUtils | `LoggerBenchmark/InfoLogging` | 4.87 µs | new baseline | 4.87 µs | 205 k/s | 5.45% |

CPU-time changes are relative to the previous run with the same runner image,
compiler, CPU model, logical CPU count and Google Benchmark version. Negative values
are faster. No automatic regression threshold is applied while the baseline is being
established.

See [HISTORY.md](HISTORY.md) for all recorded runs and `history.json` for the
machine-readable data.
