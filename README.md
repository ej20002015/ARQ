# ARQ Benchmark Results

This branch is updated by the weekly benchmark workflow. Raw Google Benchmark JSON
is retained with each workflow run as an artifact.

**Latest run:** 2026-10-26T09:08:47Z

| Context | Value |
|---|---|
| Commit | [66ba37ed526f](https://github.com/ej20002015/ARQ/commit/66ba37ed526f5c480b1db10f54710a35549647d4) |
| Workflow | [run 40296688138](https://github.com/ej20002015/ARQ/actions/runs/40296688138) |
| Branch | `master` |
| Runner | ubuntu-24.04 |
| Compiler | GCC 14.3.0 |
| CPU | AMD EPYC 9V74 80-Core Processor |
| Logical CPUs | 4 |
| Google Benchmark | v1.9.1 |

## Latest measurements

| Suite | Benchmark | Median CPU | Change | Median real | Throughput | CPU CV |
|---|---|---:|---:|---:|---:|---:|
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:32` | 9.46 ns | -19.98% | 9.46 ns | 106 M/s | 0.72% |
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:512` | 7.71 ns | -26.88% | 7.71 ns | 130 M/s | 0.38% |
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:8192` | 8.76 ns | -16.66% | 8.76 ns | 114 M/s | 0.65% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:32/UpdateSize:1` | 54.6 µs | -20.66% | 54.4 µs | 18.3 k/s | 2.27% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:512/UpdateSize:1` | 53.8 µs | -21.60% | 53.6 µs | 18.6 k/s | 1.01% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:8192/UpdateSize:1` | 53.8 µs | -20.69% | 53.6 µs | 18.6 k/s | 0.28% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:8192/UpdateSize:32` | 55.2 µs | -21.69% | 55 µs | 580 k/s | 3.15% |
| ARQUtils | `LoggerBenchmark/InfoLogging` | 5.51 µs | -7.68% | 5.51 µs | 182 k/s | 17.61% |

CPU-time changes are relative to the previous run with the same runner image,
compiler, CPU model, logical CPU count and Google Benchmark version. Negative values
are faster. No automatic regression threshold is applied while the baseline is being
established.

See [HISTORY.md](HISTORY.md) for all recorded runs and `history.json` for the
machine-readable data.
