# ARQ Benchmark Results

This branch is updated by the weekly benchmark workflow. Raw Google Benchmark JSON
is retained with each workflow run as an artifact.

**Latest run:** 2026-09-14T09:05:07Z

| Context | Value |
|---|---|
| Commit | [66ba37ed526f](https://github.com/ej20002015/ARQ/commit/66ba37ed526f5c480b1db10f54710a35549647d4) |
| Workflow | [run 34825733760](https://github.com/ej20002015/ARQ/actions/runs/34825733760) |
| Branch | `master` |
| Runner | ubuntu-24.04 |
| Compiler | GCC 14.3.0 |
| CPU | AMD EPYC 9V74 80-Core Processor |
| Logical CPUs | 4 |
| Google Benchmark | v1.9.1 |

## Latest measurements

| Suite | Benchmark | Median CPU | Change | Median real | Throughput | CPU CV |
|---|---|---:|---:|---:|---:|---:|
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:32` | 11.8 ns | +21.14% | 11.8 ns | 84.6 M/s | 9.27% |
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:512` | 10.5 ns | +12.10% | 10.5 ns | 94.8 M/s | 2.18% |
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:8192` | 10.5 ns | +14.00% | 10.5 ns | 95.1 M/s | 2.82% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:32/UpdateSize:1` | 68.8 µs | +22.25% | 68.6 µs | 14.5 k/s | 1.41% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:512/UpdateSize:1` | 68.6 µs | +21.72% | 68.4 µs | 14.6 k/s | 0.76% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:8192/UpdateSize:1` | 67.8 µs | +22.02% | 67.6 µs | 14.7 k/s | 1.02% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:8192/UpdateSize:32` | 70.4 µs | +22.25% | 70.2 µs | 454 k/s | 1.79% |
| ARQUtils | `LoggerBenchmark/InfoLogging` | 5.97 µs | +22.50% | 5.97 µs | 168 k/s | 5.75% |

CPU-time changes are relative to the previous run with the same runner image,
compiler, CPU model, logical CPU count and Google Benchmark version. Negative values
are faster. No automatic regression threshold is applied while the baseline is being
established.

See [HISTORY.md](HISTORY.md) for all recorded runs and `history.json` for the
machine-readable data.
