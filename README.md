# ARQ Benchmark Results

This branch is updated by the weekly benchmark workflow. Raw Google Benchmark JSON
is retained with each workflow run as an artifact.

**Latest run:** 2026-08-31T16:05:01Z

| Context | Value |
|---|---|
| Commit | [a7c6a9416a35](https://github.com/ej20002015/ARQ/commit/a7c6a9416a3585fa19e5cb52f54177a9b08560c9) |
| Workflow | [run 33411868840](https://github.com/ej20002015/ARQ/actions/runs/33411868840) |
| Branch | `master` |
| Runner | ubuntu-24.04 |
| Compiler | GCC 14.3.0 |
| CPU | Intel(R) Xeon(R) 6973P-C |
| Logical CPUs | 4 |
| Google Benchmark | v1.9.1 |

## Latest measurements

| Suite | Benchmark | Median CPU | Change | Median real | Throughput | CPU CV |
|---|---|---:|---:|---:|---:|---:|
| ARQUtils | `LoggerBenchmark/InfoLogging` | 5.27 µs | new baseline | 5.3 µs | 190 k/s | 2.21% |

CPU-time changes are relative to the previous run with the same runner image,
compiler, CPU model, logical CPU count and Google Benchmark version. Negative values
are faster. No automatic regression threshold is applied while the baseline is being
established.

See [HISTORY.md](HISTORY.md) for all recorded runs and `history.json` for the
machine-readable data.
