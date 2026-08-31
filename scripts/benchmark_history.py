#!/usr/bin/env python3

"""Update the private ARQ benchmark history from Google Benchmark JSON files."""

from __future__ import annotations

import argparse
import json
import math
import statistics
from collections import defaultdict
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1
TIME_UNIT_TO_NS = {
    "ns": 1.0,
    "us": 1_000.0,
    "ms": 1_000_000.0,
    "s": 1_000_000_000.0,
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--results-dir", type=Path, required=True)
    parser.add_argument("--history-dir", type=Path, required=True)
    parser.add_argument("--repository", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--branch", required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--run-attempt", required=True)
    parser.add_argument("--run-url", required=True)
    parser.add_argument("--timestamp", required=True)
    parser.add_argument("--runner-image", required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--cpu-model", required=True)
    parser.add_argument("--summary-output", type=Path)
    return parser.parse_args()


def read_json(path: Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as stream:
        value = json.load(stream)

    if not isinstance(value, dict):
        raise ValueError(f"Expected a JSON object in {path}")
    return value


def suite_name(path: Path) -> str:
    name = path.stem
    if name.startswith("b_"):
        name = name[2:]
    if name.endswith("_results"):
        name = name[: -len("_results")]
    return name


def finite_number(entry: dict[str, Any], key: str) -> float | None:
    value = entry.get(key)
    if isinstance(value, (int, float)) and math.isfinite(value):
        return float(value)
    return None


def time_in_ns(value: float | None, unit: str) -> float | None:
    if value is None:
        return None
    multiplier = TIME_UNIT_TO_NS.get(unit)
    if multiplier is None:
        raise ValueError(f"Unsupported Google Benchmark time unit: {unit}")
    return value * multiplier


def aggregate(
    entries: list[dict[str, Any]], aggregate_name: str
) -> dict[str, Any] | None:
    return next(
        (
            entry
            for entry in entries
            if entry.get("run_type") == "aggregate"
            and entry.get("aggregate_name") == aggregate_name
        ),
        None,
    )


def median_number(entries: list[dict[str, Any]], key: str) -> float | None:
    values = [value for entry in entries if (value := finite_number(entry, key)) is not None]
    return statistics.median(values) if values else None


def coefficient_of_variation(entries: list[dict[str, Any]], key: str) -> float | None:
    values = [value for entry in entries if (value := finite_number(entry, key)) is not None]
    if len(values) < 2:
        return None
    mean = statistics.mean(values)
    if mean == 0:
        return None
    return statistics.stdev(values) / mean


def extract_benchmarks(path: Path) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    document = read_json(path)
    raw_benchmarks = document.get("benchmarks")
    if not isinstance(raw_benchmarks, list):
        raise ValueError(f"Missing benchmarks array in {path}")

    grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for entry in raw_benchmarks:
        if not isinstance(entry, dict):
            continue
        run_name = entry.get("run_name") or entry.get("name")
        if isinstance(run_name, str):
            grouped[run_name].append(entry)

    extracted: list[dict[str, Any]] = []
    suite = suite_name(path)
    for run_name, entries in sorted(grouped.items()):
        iterations = [entry for entry in entries if entry.get("run_type") == "iteration"]
        median = aggregate(entries, "median")
        cv = aggregate(entries, "cv")
        representative = median or (iterations[0] if iterations else None)
        if representative is None:
            continue

        unit = representative.get("time_unit")
        if not isinstance(unit, str):
            raise ValueError(f"Missing time unit for {suite}/{run_name}")

        cpu_time = finite_number(median, "cpu_time") if median else median_number(iterations, "cpu_time")
        real_time = finite_number(median, "real_time") if median else median_number(iterations, "real_time")
        cpu_cv = finite_number(cv, "cpu_time") if cv else coefficient_of_variation(iterations, "cpu_time")
        items_per_second = (
            finite_number(median, "items_per_second")
            if median
            else median_number(iterations, "items_per_second")
        )

        extracted.append(
            {
                "suite": suite,
                "name": run_name,
                "median_cpu_time_ns": time_in_ns(cpu_time, unit),
                "median_real_time_ns": time_in_ns(real_time, unit),
                "cpu_cv": cpu_cv,
                "items_per_second": items_per_second,
            }
        )

    context = document.get("context")
    return extracted, context if isinstance(context, dict) else {}


def load_history(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {"schema_version": SCHEMA_VERSION, "runs": []}

    history = read_json(path)
    if history.get("schema_version") != SCHEMA_VERSION:
        raise ValueError(f"Unsupported benchmark history schema in {path}")
    if not isinstance(history.get("runs"), list):
        raise ValueError(f"Missing runs array in {path}")
    return history


def environment_key(run: dict[str, Any]) -> tuple[Any, ...]:
    return (
        run.get("runner_image"),
        run.get("compiler"),
        run.get("cpu_model"),
        run.get("num_cpus"),
        run.get("benchmark_library_version"),
    )


def benchmark_key(benchmark: dict[str, Any]) -> tuple[Any, ...]:
    return benchmark.get("suite"), benchmark.get("name")


def previous_benchmark(
    runs: list[dict[str, Any]], current_run: dict[str, Any], current_benchmark: dict[str, Any]
) -> dict[str, Any] | None:
    current_environment = environment_key(current_run)
    current_key = benchmark_key(current_benchmark)
    for run in reversed(runs):
        if environment_key(run) != current_environment:
            continue
        for benchmark in run.get("benchmarks", []):
            if isinstance(benchmark, dict) and benchmark_key(benchmark) == current_key:
                return benchmark
    return None


def escape_markdown(value: Any) -> str:
    return str(value).replace("|", "\\|").replace("\n", " ")


def format_time(value_ns: Any) -> str:
    if not isinstance(value_ns, (int, float)) or not math.isfinite(value_ns):
        return "—"
    if value_ns >= 1_000_000_000:
        return f"{value_ns / 1_000_000_000:.3g} s"
    if value_ns >= 1_000_000:
        return f"{value_ns / 1_000_000:.3g} ms"
    if value_ns >= 1_000:
        return f"{value_ns / 1_000:.3g} µs"
    return f"{value_ns:.3g} ns"


def format_rate(value: Any) -> str:
    if not isinstance(value, (int, float)) or not math.isfinite(value):
        return "—"
    for divisor, suffix in ((1_000_000_000, "G/s"), (1_000_000, "M/s"), (1_000, "k/s")):
        if abs(value) >= divisor:
            return f"{value / divisor:.3g} {suffix}"
    return f"{value:.3g} /s"


def format_cv(value: Any) -> str:
    if not isinstance(value, (int, float)) or not math.isfinite(value):
        return "—"
    return f"{value * 100:.2f}%"


def format_change(current: dict[str, Any], previous: dict[str, Any] | None) -> str:
    if previous is None:
        return "new baseline"
    current_value = current.get("median_cpu_time_ns")
    previous_value = previous.get("median_cpu_time_ns")
    if not isinstance(current_value, (int, float)) or not isinstance(previous_value, (int, float)):
        return "—"
    if not math.isfinite(current_value) or not math.isfinite(previous_value) or previous_value == 0:
        return "—"
    change = (current_value - previous_value) / previous_value * 100
    return f"{change:+.2f}%"


def commit_link(repository: str, commit: str) -> str:
    return f"[{commit[:12]}](https://github.com/{repository}/commit/{commit})"


def result_table(
    previous_runs: list[dict[str, Any]], current_run: dict[str, Any]
) -> list[str]:
    lines = [
        "| Suite | Benchmark | Median CPU | Change | Median real | Throughput | CPU CV |",
        "|---|---|---:|---:|---:|---:|---:|",
    ]
    for benchmark in current_run["benchmarks"]:
        previous = previous_benchmark(previous_runs, current_run, benchmark)
        lines.append(
            "| {suite} | `{name}` | {cpu} | {change} | {real} | {rate} | {cv} |".format(
                suite=escape_markdown(benchmark["suite"]),
                name=escape_markdown(benchmark["name"]),
                cpu=format_time(benchmark.get("median_cpu_time_ns")),
                change=format_change(benchmark, previous),
                real=format_time(benchmark.get("median_real_time_ns")),
                rate=format_rate(benchmark.get("items_per_second")),
                cv=format_cv(benchmark.get("cpu_cv")),
            )
        )
    return lines


def render_latest(
    repository: str, previous_runs: list[dict[str, Any]], current_run: dict[str, Any]
) -> str:
    lines = [
        "# ARQ Benchmark Results",
        "",
        "This branch is updated by the weekly benchmark workflow. Raw Google Benchmark JSON",
        "is retained with each workflow run as an artifact.",
        "",
        f"**Latest run:** {escape_markdown(current_run['timestamp'])}",
        "",
        "| Context | Value |",
        "|---|---|",
        f"| Commit | {commit_link(repository, current_run['commit'])} |",
        f"| Workflow | [run {current_run['run_id']}]({current_run['run_url']}) |",
        f"| Branch | `{escape_markdown(current_run['branch'])}` |",
        f"| Runner | {escape_markdown(current_run['runner_image'])} |",
        f"| Compiler | {escape_markdown(current_run['compiler'])} |",
        f"| CPU | {escape_markdown(current_run['cpu_model'])} |",
        f"| Logical CPUs | {escape_markdown(current_run.get('num_cpus', 'unknown'))} |",
        f"| Google Benchmark | {escape_markdown(current_run.get('benchmark_library_version', 'unknown'))} |",
        "",
        "## Latest measurements",
        "",
        *result_table(previous_runs, current_run),
        "",
        "CPU-time changes are relative to the previous run with the same runner image,",
        "compiler, CPU model, logical CPU count and Google Benchmark version. Negative values",
        "are faster. No automatic regression threshold is applied while the baseline is being",
        "established.",
        "",
        "See [HISTORY.md](HISTORY.md) for all recorded runs and `history.json` for the",
        "machine-readable data.",
        "",
    ]
    return "\n".join(lines)


def render_history(repository: str, runs: list[dict[str, Any]]) -> str:
    comparisons: dict[tuple[Any, ...], dict[str, Any]] = {}
    rows: list[str] = []
    for run in runs:
        environment = environment_key(run)
        for benchmark in run.get("benchmarks", []):
            if not isinstance(benchmark, dict):
                continue
            key = environment + benchmark_key(benchmark)
            previous = comparisons.get(key)
            rows.append(
                "| {date} | {commit} | {suite} | `{name}` | {cpu} | {change} | {rate} | {cv} | [run]({url}) |".format(
                    date=escape_markdown(run.get("timestamp", "unknown")),
                    commit=commit_link(repository, str(run.get("commit", ""))),
                    suite=escape_markdown(benchmark.get("suite", "unknown")),
                    name=escape_markdown(benchmark.get("name", "unknown")),
                    cpu=format_time(benchmark.get("median_cpu_time_ns")),
                    change=format_change(benchmark, previous),
                    rate=format_rate(benchmark.get("items_per_second")),
                    cv=format_cv(benchmark.get("cpu_cv")),
                    url=run.get("run_url", ""),
                )
            )
            comparisons[key] = benchmark

    return "\n".join(
        [
            "# Complete ARQ Benchmark History",
            "",
            "Rows are ordered newest first. Change comparisons use compatible runner and",
            "toolchain context; `new baseline` indicates that no compatible earlier run exists.",
            "",
            "| Date | Commit | Suite | Benchmark | Median CPU | Change | Throughput | CPU CV | Workflow |",
            "|---|---|---|---|---:|---:|---:|---:|---|",
            *reversed(rows),
            "",
        ]
    )


def write_text(path: Path, value: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value, encoding="utf-8", newline="\n")


def main() -> None:
    args = parse_args()
    result_files = sorted(args.results_dir.glob("*_results.json"))
    if not result_files:
        raise FileNotFoundError(f"No benchmark JSON files found in {args.results_dir}")

    benchmarks: list[dict[str, Any]] = []
    contexts: list[dict[str, Any]] = []
    for result_file in result_files:
        extracted, context = extract_benchmarks(result_file)
        benchmarks.extend(extracted)
        contexts.append(context)

    if not benchmarks:
        raise ValueError("The benchmark JSON files contained no measurements")

    first_context = contexts[0] if contexts else {}
    current_run = {
        "run_id": args.run_id,
        "run_attempt": args.run_attempt,
        "timestamp": args.timestamp,
        "commit": args.commit,
        "branch": args.branch,
        "run_url": args.run_url,
        "runner_image": args.runner_image,
        "compiler": args.compiler,
        "cpu_model": args.cpu_model,
        "num_cpus": first_context.get("num_cpus"),
        "mhz_per_cpu": first_context.get("mhz_per_cpu"),
        "benchmark_library_version": first_context.get("library_version"),
        "benchmarks": sorted(benchmarks, key=benchmark_key),
    }

    history_path = args.history_dir / "history.json"
    history = load_history(history_path)
    previous_runs = [
        run
        for run in history["runs"]
        if isinstance(run, dict) and str(run.get("run_id")) != args.run_id
    ]
    previous_runs.sort(key=lambda run: str(run.get("timestamp", "")))
    history["runs"] = [*previous_runs, current_run]

    args.history_dir.mkdir(parents=True, exist_ok=True)
    write_text(args.history_dir / "README.md", render_latest(args.repository, previous_runs, current_run))
    write_text(args.history_dir / "HISTORY.md", render_history(args.repository, history["runs"]))
    write_text(history_path, json.dumps(history, indent=2, sort_keys=True) + "\n")

    if args.summary_output:
        write_text(
            args.summary_output,
            "\n".join(
                [
                    "## Weekly benchmark results",
                    "",
                    *result_table(previous_runs, current_run),
                    "",
                    f"[Recorded history branch](https://github.com/{args.repository}/tree/benchmark-results)",
                    "",
                ]
            ),
        )


if __name__ == "__main__":
    main()
