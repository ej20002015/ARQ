# ARQ Benchmark Results

This branch is updated by the weekly benchmark workflow. Raw Google Benchmark JSON
is retained with each workflow run as an artifact.

**Latest run:** 2026-10-05T10:48:27Z

| Context | Value |
|---|---|
| Commit | [66ba37ed526f](https://github.com/ej20002015/ARQ/commit/66ba37ed526f5c480b1db10f54710a35549647d4) |
| Workflow | [run 37297471012](https://github.com/ej20002015/ARQ/actions/runs/37297471012) |
| Branch | `master` |
| Runner | ubuntu-24.04 |
| Compiler | GCC 14.3.0 |
| CPU | AMD EPYC 7763 64-Core Processor |
| Logical CPUs | 4 |
| Google Benchmark | v1.9.1 |

## Latest measurements

| Suite | Benchmark | Median CPU | Change | Median real | Throughput | CPU CV |
|---|---|---:|---:|---:|---:|---:|
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:32` | 10.8 ns | -0.24% | 10.8 ns | 92.3 M/s | 0.41% |
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:512` | 9.83 ns | +1.24% | 9.83 ns | 102 M/s | 0.98% |
| ARQMarket | `MarketBenchmark/AcquireSnapshotAndReadFXRate/MarketSize:8192` | 9.87 ns | -1.98% | 9.87 ns | 101 M/s | 0.39% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:32/UpdateSize:1` | 72.4 µs | -3.48% | 72.2 µs | 13.8 k/s | 46.69% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:512/UpdateSize:1` | 74 µs | -0.40% | 73.8 µs | 13.5 k/s | 1.08% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:8192/UpdateSize:1` | 73.5 µs | -0.20% | 73.3 µs | 13.6 k/s | 3.99% |
| ARQMarket | `MarketBenchmark/UpdateSnapshot/MarketSize:8192/UpdateSize:32` | 75.4 µs | -1.07% | 75.2 µs | 424 k/s | 3.63% |
| ARQUtils | `LoggerBenchmark/InfoLogging` | 6.23 µs | +1.09% | 6.25 µs | 161 k/s | 11.51% |

CPU-time changes are relative to the previous run with the same runner image,
compiler, CPU model, logical CPU count and Google Benchmark version. Negative values
are faster. No automatic regression threshold is applied while the baseline is being
established.

See [HISTORY.md](HISTORY.md) for all recorded runs and `history.json` for the
machine-readable data.
