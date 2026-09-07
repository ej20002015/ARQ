# ARQ Benchmark Results

This branch is updated by the weekly benchmark workflow. Raw Google Benchmark JSON
is retained with each workflow run as an artifact.

**Latest run:** 2026-09-07T08:26:57Z

| Context | Value |
|---|---|
| Commit | [fe95b335bd35](https://github.com/ej20002015/ARQ/commit/fe95b335bd352e952da7196bd9dd6446f2f2cbe2) |
| Workflow | [run 34100455899](https://github.com/ej20002015/ARQ/actions/runs/34100455899) |
| Branch | `master` |
| Runner | ubuntu-24.04 |
| Compiler | GCC 14.3.0 |
| CPU | Intel(R) Xeon(R) 6973P-C |
| Logical CPUs | 4 |
| Google Benchmark | v1.9.1 |

## Latest measurements

| Suite | Benchmark | Median CPU | Change | Median real | Throughput | CPU CV |
|---|---|---:|---:|---:|---:|---:|
| ARQUtils | `LoggerBenchmark/InfoLogging` | 5.51 µs | +4.59% | 5.55 µs | 182 k/s | 2.72% |

CPU-time changes are relative to the previous run with the same runner image,
compiler, CPU model, logical CPU count and Google Benchmark version. Negative values
are faster. No automatic regression threshold is applied while the baseline is being
established.

See [HISTORY.md](HISTORY.md) for all recorded runs and `history.json` for the
machine-readable data.
