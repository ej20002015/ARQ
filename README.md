# ARQ Benchmark Results

This branch is updated by the weekly benchmark workflow. Raw Google Benchmark JSON
is retained with each workflow run as an artifact.

**Latest run:** 2026-10-19T09:04:22Z

| Context | Value |
|---|---|
| Commit | [66ba37ed526f](https://github.com/ej20002015/ARQ/commit/66ba37ed526f5c480b1db10f54710a35549647d4) |
| Workflow | [run 39226907282](https://github.com/ej20002015/ARQ/actions/runs/39226907282) |
| Branch | `master` |
| Runner | ubuntu-24.04 |
| Compiler | GCC 14.3.0 |
| CPU | INTEL(R) XEON(R) PLATINUM 8573C |
| Logical CPUs | 4 |
| Google Benchmark | v1.9.1 |

## Latest measurements

| Suite | Benchmark | Median CPU | Change | Median real | Throughput | CPU CV |
|---|---|---:|---:|---:|---:|---:|
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:32` | 22 ns | new baseline | 22 ns | 45.4 M/s | 1.85% |
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:512` | 20.3 ns | new baseline | 20.3 ns | 49.2 M/s | 1.42% |
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:8192` | 21.2 ns | new baseline | 21.2 ns | 47.3 M/s | 1.22% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:32/UpdateSize:1` | 64.2 µs | new baseline | 64 µs | 15.6 k/s | 2.61% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:512/UpdateSize:1` | 64.7 µs | new baseline | 64.6 µs | 15.5 k/s | 2.34% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:8192/UpdateSize:1` | 62.8 µs | new baseline | 62.7 µs | 15.9 k/s | 2.97% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:8192/UpdateSize:32` | 66.7 µs | new baseline | 66.5 µs | 480 k/s | 1.26% |
| ARQUtils | `LoggerBenchmark/InfoLogging` | 9 µs | new baseline | 9.04 µs | 111 k/s | 6.93% |

CPU-time changes are relative to the previous run with the same runner image,
compiler, CPU model, logical CPU count and Google Benchmark version. Negative values
are faster. No automatic regression threshold is applied while the baseline is being
established.

See [HISTORY.md](HISTORY.md) for all recorded runs and `history.json` for the
machine-readable data.
