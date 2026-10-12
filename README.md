# ARQ Benchmark Results

This branch is updated by the weekly benchmark workflow. Raw Google Benchmark JSON
is retained with each workflow run as an artifact.

**Latest run:** 2026-10-12T11:05:09Z

| Context | Value |
|---|---|
| Commit | [66ba37ed526f](https://github.com/ej20002015/ARQ/commit/66ba37ed526f5c480b1db10f54710a35549647d4) |
| Workflow | [run 38239781957](https://github.com/ej20002015/ARQ/actions/runs/38239781957) |
| Branch | `master` |
| Runner | ubuntu-24.04 |
| Compiler | GCC 14.3.0 |
| CPU | AMD EPYC 9V45 96-Core Processor |
| Logical CPUs | 4 |
| Google Benchmark | v1.9.1 |

## Latest measurements

| Suite | Benchmark | Median CPU | Change | Median real | Throughput | CPU CV |
|---|---|---:|---:|---:|---:|---:|
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:32` | 13.3 ns | +1.57% | 13.3 ns | 75.2 M/s | 1.61% |
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:512` | 12.7 ns | +2.55% | 12.7 ns | 78.7 M/s | 1.03% |
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:8192` | 13 ns | +2.07% | 13 ns | 77.1 M/s | 1.02% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:32/UpdateSize:1` | 33.6 µs | -0.78% | 33.5 µs | 29.7 k/s | 3.39% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:512/UpdateSize:1` | 35 µs | -7.42% | 34.9 µs | 28.6 k/s | 51.58% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:8192/UpdateSize:1` | 34.9 µs | -4.13% | 34.7 µs | 28.6 k/s | 29.09% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:8192/UpdateSize:32` | 36.1 µs | -6.66% | 35.9 µs | 886 k/s | 31.20% |
| ARQUtils | `LoggerBenchmark/InfoLogging` | 3.63 µs | -0.12% | 3.63 µs | 276 k/s | 11.36% |

CPU-time changes are relative to the previous run with the same runner image,
compiler, CPU model, logical CPU count and Google Benchmark version. Negative values
are faster. No automatic regression threshold is applied while the baseline is being
established.

See [HISTORY.md](HISTORY.md) for all recorded runs and `history.json` for the
machine-readable data.
