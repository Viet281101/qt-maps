#!/usr/bin/env python3
"""Summarize QtMaps Unit 06 raw benchmark runs as JSON and Markdown."""

import argparse
import csv
import json
import math
import statistics
from collections import defaultdict
from pathlib import Path


MIB = 1024 * 1024
THRESHOLDS = {
    "fps_average_min": 30.0,
    "fps_p5_min": 24.0,
    "fps_one_percent_low_min": 20.0,
    "frame_time_average_ms_max": 33.3,
    "frame_time_p95_ms_max": 50.0,
    "idle_cpu_all_average_max": 5.0,
    "idle_cpu_all_peak_max": 15.0,
    "interactive_cpu_all_average_max": 40.0,
    "interactive_cpu_all_peak_max": 75.0,
    "route_valhalla_cpu_all_peak_max": 65.0,
    "rss_average_mib_max": 650.0,
    "rss_peak_mib_max": 768.0,
    "pss_average_mib_max": 550.0,
    "pss_peak_mib_max": 640.0,
    "sampler_duration_p95_ms_max": 50.0,
    "sampler_cpu_overhead_points_max": 1.0,
    "sampler_memory_overhead_mib_max": 10.0,
}


def number(value):
    if value in (None, ""):
        return None
    try:
        parsed = float(value)
    except (TypeError, ValueError):
        return None
    return parsed if math.isfinite(parsed) else None


def nearest_rank(values, percentile):
    ordered = sorted(value for value in values if value is not None)
    if not ordered:
        return None
    rank = max(1, math.ceil(percentile * len(ordered) / 100.0))
    return ordered[min(rank - 1, len(ordered) - 1)]


def stats(values):
    present = [value for value in values if value is not None]
    if not present:
        return None
    return {
        "median": statistics.median(present),
        "min": min(present),
        "max": max(present),
        "count": len(present),
    }


def read_process_metrics(path):
    with path.open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    totals = [row for row in rows if row.get("row_type") == "tree_total"]
    processes = [row for row in rows if row.get("row_type") == "process"]
    if not totals:
        return {}

    rss = [number(row.get("rss_bytes")) / MIB for row in totals]
    complete_pss = [
        number(row.get("pss_bytes")) / MIB
        for row in totals
        if row.get("pss_available") == "1" and number(row.get("pss_bytes")) is not None
    ]
    cpu_one = [number(row.get("cpu_percent_one_core_100")) for row in totals]
    cpu_all = [number(row.get("cpu_percent_all_cores_100")) for row in totals]
    sampler = [number(row.get("sampler_duration_ms")) for row in totals]
    threads = [number(row.get("threads")) for row in totals]
    process_counts = [number(row.get("process_count")) for row in totals]
    valhalla_cpu_all = [
        number(row.get("cpu_percent_all_cores_100"))
        for row in processes
        if row.get("role") == "valhalla"
    ]
    valhalla_rss = [
        number(row.get("rss_bytes")) / MIB
        for row in processes
        if row.get("role") == "valhalla" and number(row.get("rss_bytes")) is not None
    ]
    valhalla_pss = [
        number(row.get("pss_bytes")) / MIB
        for row in processes
        if row.get("role") == "valhalla" and row.get("pss_available") == "1"
    ]
    return {
        "rss_average_mib": statistics.fmean(rss),
        "rss_peak_mib": max(rss),
        "pss_average_mib": statistics.fmean(complete_pss) if complete_pss else None,
        "pss_peak_mib": max(complete_pss) if complete_pss else None,
        "pss_complete_samples": len(complete_pss),
        "process_samples": len(totals),
        "cpu_one_average": statistics.fmean(cpu_one),
        "cpu_one_peak": max(cpu_one),
        "cpu_all_average": statistics.fmean(cpu_all),
        "cpu_all_peak": max(cpu_all),
        "sampler_duration_p95_ms": nearest_rank(sampler, 95.0),
        "threads_peak": max(threads),
        "process_count_peak": max(process_counts),
        "valhalla_cpu_all_peak": max(valhalla_cpu_all) if valhalla_cpu_all else None,
        "valhalla_rss_peak_mib": max(valhalla_rss) if valhalla_rss else None,
        "valhalla_pss_peak_mib": max(valhalla_pss) if valhalla_pss else None,
    }


def summarize_run(run_file):
    run = json.loads(run_file.read_text(encoding="utf-8"))
    process_path = run_file.parent / run["process_csv"]
    metrics = read_process_metrics(process_path) if process_path.exists() else {}
    events = run.get("events", [])
    results = [event for event in events if event.get("event") == "result"]
    result = results[-1] if results else {}
    route_events = [
        event
        for event in events
        if event.get("event") == "route_finished" and event.get("success")
    ]
    route_latencies = [number(event.get("latency_ms")) for event in route_events]
    metrics.update(
        {
            "startup_ms": number(result.get("startup_ms")),
            "fps_average": number(result.get("fps_average")),
            "fps_p5": number(result.get("fps_p5")),
            "fps_one_percent_low": number(result.get("fps_one_percent_low")),
            "frame_time_average_ms": number(result.get("frame_time_average_ms")),
            "frame_time_p95_ms": number(result.get("frame_time_p95_ms")),
            "frame_time_p99_ms": number(result.get("frame_time_p99_ms")),
            "route_latency_median_ms": statistics.median(route_latencies)
            if route_latencies
            else None,
            "route_latency_min_ms": min(route_latencies) if route_latencies else None,
            "route_latency_max_ms": max(route_latencies) if route_latencies else None,
        }
    )
    return {
        "scenario": run["scenario"],
        "network_mode": run["network_mode"],
        "repetition": run["repetition"],
        "run_id": run["run_id"],
        "exit_code": run["exit_code"],
        "success": run["exit_code"] == 0 and bool(result.get("success")),
        "metrics": metrics,
        "directory": str(run_file.parent),
    }


def aggregate_runs(runs):
    grouped = defaultdict(list)
    for run in runs:
        grouped[(run["scenario"], run["network_mode"])].append(run)
    output = []
    metric_names = sorted(
        {name for run in runs for name in run.get("metrics", {}) if name != "pss_complete_samples"}
    )
    for (scenario, network_mode), group_runs in sorted(grouped.items()):
        metrics = {
            name: stats([run["metrics"].get(name) for run in group_runs]) for name in metric_names
        }
        metrics = {name: value for name, value in metrics.items() if value is not None}
        output.append(
            {
                "scenario": scenario,
                "network_mode": network_mode,
                "run_count": len(group_runs),
                "successful_runs": sum(1 for run in group_runs if run["success"]),
                "metrics": metrics,
            }
        )
    return output


def threshold_result(name, metric, operator, threshold):
    if metric is None:
        return {"name": name, "status": "UNAVAILABLE", "threshold": threshold, "value": None}
    value = metric["median"]
    passed = value >= threshold if operator == "min" else value <= threshold
    return {
        "name": name,
        "status": "PASS" if passed else "FAIL / không đạt",
        "threshold": threshold,
        "value": value,
        "observed_min": metric["min"],
        "observed_max": metric["max"],
    }


def evaluate_thresholds(group):
    scenario = group["scenario"]
    metrics = group["metrics"]
    checks = [
        threshold_result(
            "PSS average MiB", metrics.get("pss_average_mib"), "max", THRESHOLDS["pss_average_mib_max"]
        ),
        threshold_result(
            "PSS peak MiB", metrics.get("pss_peak_mib"), "max", THRESHOLDS["pss_peak_mib_max"]
        ),
        threshold_result(
            "RSS average MiB", metrics.get("rss_average_mib"), "max", THRESHOLDS["rss_average_mib_max"]
        ),
        threshold_result(
            "RSS peak MiB", metrics.get("rss_peak_mib"), "max", THRESHOLDS["rss_peak_mib_max"]
        ),
        threshold_result(
            "Sampler duration p95 ms",
            metrics.get("sampler_duration_p95_ms"),
            "max",
            THRESHOLDS["sampler_duration_p95_ms_max"],
        ),
    ]
    if scenario in ("pan_zoom", "navigation_replay"):
        checks.extend(
            [
                threshold_result("Average FPS", metrics.get("fps_average"), "min", 30.0),
                threshold_result("FPS p5", metrics.get("fps_p5"), "min", 24.0),
                threshold_result(
                    "FPS 1% low", metrics.get("fps_one_percent_low"), "min", 20.0
                ),
                threshold_result(
                    "Average frame time ms", metrics.get("frame_time_average_ms"), "max", 33.3
                ),
                threshold_result(
                    "Frame-time p95 ms", metrics.get("frame_time_p95_ms"), "max", 50.0
                ),
                threshold_result(
                    "Machine CPU average %", metrics.get("cpu_all_average"), "max", 40.0
                ),
                threshold_result(
                    "Machine CPU peak %", metrics.get("cpu_all_peak"), "max", 75.0
                ),
            ]
        )
    elif scenario == "idle":
        checks.extend(
            [
                threshold_result(
                    "Machine CPU average %", metrics.get("cpu_all_average"), "max", 5.0
                ),
                threshold_result(
                    "Machine CPU peak %", metrics.get("cpu_all_peak"), "max", 15.0
                ),
            ]
        )
    elif scenario == "route":
        checks.append(
            threshold_result(
                "Valhalla machine CPU peak %",
                metrics.get("valhalla_cpu_all_peak"),
                "max",
                65.0,
            )
        )
    return checks


def summarize_overhead(path):
    if not path.exists():
        return {
            "cpu_overhead_points": None,
            "pss_overhead_mib": None,
            "rss_overhead_mib": None,
            "checks": [
                threshold_result(
                    "Sampler machine CPU overhead points", None, "max", 1.0
                ),
                threshold_result("Sampler PSS overhead MiB", None, "max", 10.0),
            ],
        }
    payload = json.loads(path.read_text(encoding="utf-8"))
    comparisons = payload.get("comparisons", [])
    output = {
        "cpu_overhead_points": stats(
            [number(item.get("cpu_overhead_points")) for item in comparisons]
        ),
        "pss_overhead_mib": stats(
            [number(item.get("pss_overhead_mib")) for item in comparisons]
        ),
        "rss_overhead_mib": stats(
            [number(item.get("rss_overhead_mib")) for item in comparisons]
        ),
        "raw": payload,
    }
    output["checks"] = [
        threshold_result(
            "Sampler machine CPU overhead points",
            output["cpu_overhead_points"],
            "max",
            THRESHOLDS["sampler_cpu_overhead_points_max"],
        ),
        threshold_result(
            "Sampler PSS overhead MiB",
            output["pss_overhead_mib"],
            "max",
            THRESHOLDS["sampler_memory_overhead_mib_max"],
        ),
    ]
    return output


def format_range(metric, digits=2):
    if not metric:
        return "Not measured"
    return (
        f"{metric['median']:.{digits}f} "
        f"[{metric['min']:.{digits}f}, {metric['max']:.{digits}f}]"
    )


def markdown_report(summary):
    metadata = summary.get("metadata", {})
    lines = [
        "# QtMaps Unit 06 Benchmark Report",
        "",
        "Values are median [min, max] across repeated Release runs. PSS is the primary RAM "
        "measure; RSS is shown alongside it. Locked thresholds are evaluated against the median "
        "of each per-run metric and are never retuned after measurement.",
        "",
        "## Machine and run metadata",
        "",
        f"- CPU: {metadata.get('cpu_model', 'Unknown')} — "
        f"{metadata.get('physical_cores', 'Unknown')} physical cores / "
        f"{metadata.get('logical_cpus', 'Unknown')} logical CPUs",
        f"- RAM: {metadata.get('ram_bytes', 0) / (1024 ** 3):.2f} GiB",
        f"- Kernel: {metadata.get('kernel', 'Unknown')}",
        f"- Qt / MapLibre / Valhalla: {metadata.get('qt_version', 'Unknown')} / "
        f"{metadata.get('maplibre_version', 'Unknown')} / "
        f"{metadata.get('valhalla_version', 'Unknown')}",
        f"- Git: {metadata.get('git_commit', 'Unknown')}",
        f"- Sampling: {metadata.get('fast_sampling_interval_ms', 'Unknown')} ms fast; "
        f"{metadata.get('pss_sampling_interval_ms', 'Unknown')} ms PSS",
        f"- Data on disk: tiles {metadata.get('tiles_bytes', 0) / MIB:.2f} MiB; "
        f"Valhalla graph {metadata.get('graph_bytes', 0) / MIB:.2f} MiB",
        "",
        "CPU is reported both as Linux process-style (`100% = one logical CPU`) and "
        "machine-normalized (`100% = all four logical CPUs` on the reference laptop).",
        "",
        "## Results",
        "",
        "| Scenario / mode | Runs | FPS avg | FPS p5 | 1% low | Frame avg / p95 ms | "
        "PSS avg / peak MiB | RSS avg / peak MiB | CPU one-core avg / peak % | "
        "CPU all-core avg / peak % | Status |",
        "| --- | ---: | ---: | ---: | ---: | --- | --- | --- | --- | --- | --- |",
    ]
    for group in summary["groups"]:
        metrics = group["metrics"]
        checks = group["thresholds"]
        failed = [check for check in checks if check["status"].startswith("FAIL")]
        unavailable = [check for check in checks if check["status"] == "UNAVAILABLE"]
        status = "FAIL / không đạt" if failed else ("UNAVAILABLE" if unavailable else "PASS")
        lines.append(
            f"| {group['scenario']} / {group['network_mode']} | "
            f"{group['successful_runs']}/{group['run_count']} | "
            f"{format_range(metrics.get('fps_average'))} | "
            f"{format_range(metrics.get('fps_p5'))} | "
            f"{format_range(metrics.get('fps_one_percent_low'))} | "
            f"{format_range(metrics.get('frame_time_average_ms'))} / "
            f"{format_range(metrics.get('frame_time_p95_ms'))} | "
            f"{format_range(metrics.get('pss_average_mib'))} / "
            f"{format_range(metrics.get('pss_peak_mib'))} | "
            f"{format_range(metrics.get('rss_average_mib'))} / "
            f"{format_range(metrics.get('rss_peak_mib'))} | "
            f"{format_range(metrics.get('cpu_one_average'))} / "
            f"{format_range(metrics.get('cpu_one_peak'))} | "
            f"{format_range(metrics.get('cpu_all_average'))} / "
            f"{format_range(metrics.get('cpu_all_peak'))} | {status} |"
        )

    lines.extend(["", "## Locked-threshold evaluation", ""])
    for group in summary["groups"]:
        lines.append(f"### {group['scenario']} / {group['network_mode']}")
        lines.append("")
        for check in group["thresholds"]:
            value = "Not measured" if check["value"] is None else f"{check['value']:.3f}"
            lines.append(
                f"- **{check['status']}** — {check['name']}: {value}; "
                f"locked threshold {check['threshold']}."
            )
        lines.append("")

    lines.extend(["### Sampler overhead", ""])
    for check in summary["sampler_overhead"]["checks"]:
        value = "Not measured" if check["value"] is None else f"{check['value']:.3f}"
        lines.append(
            f"- **{check['status']}** — {check['name']}: {value}; "
            f"locked threshold {check['threshold']}."
        )
    lines.append("")

    pss_values = [
        group["metrics"].get("pss_average_mib", {}).get("median")
        for group in summary["groups"]
    ]
    rss_values = [
        group["metrics"].get("rss_average_mib", {}).get("median")
        for group in summary["groups"]
    ]
    differences = [
        rss - pss
        for pss, rss in zip(pss_values, rss_values)
        if pss is not None and rss is not None
    ]
    lines.extend(["## PSS and RSS interpretation", ""])
    if differences:
        lines.append(
            f"Across the scenario groups, median average RSS exceeds PSS by "
            f"{statistics.median(differences):.2f} MiB. RSS counts each process's shared "
            "mappings in full, while PSS apportions those pages among sharing processes; "
            "therefore PSS is the primary tree-memory result."
        )
    else:
        lines.append(
            "PSS was unavailable, so no valid RSS/PSS difference can be calculated. "
            "The affected result is not presented as a passing RAM measurement."
        )
    lines.extend(
        [
            "",
            "## Report-only latency",
            "",
            "Startup and route latency are baseline evidence only; no pass/fail threshold is "
            "applied in Unit 06's first baseline.",
            "",
            "| Scenario / mode | Startup ms | Route latency ms |",
            "| --- | ---: | ---: |",
        ]
    )
    for group in summary["groups"]:
        metrics = group["metrics"]
        lines.append(
            f"| {group['scenario']} / {group['network_mode']} | "
            f"{format_range(metrics.get('startup_ms'))} | "
            f"{format_range(metrics.get('route_latency_median_ms'))} |"
        )
    return "\n".join(lines) + "\n"


def summarize_session(session_dir):
    session_dir = Path(session_dir).resolve()
    metadata_path = session_dir / "session-metadata.json"
    metadata = json.loads(metadata_path.read_text(encoding="utf-8")) if metadata_path.exists() else {}
    runs = [summarize_run(path) for path in sorted(session_dir.glob("runs/*/run.json"))]
    groups = aggregate_runs(runs)
    for group in groups:
        group["thresholds"] = evaluate_thresholds(group)
    overhead = summarize_overhead(session_dir / "sampler-overhead.json")
    summary = {
        "schema_version": "qtmaps-benchmark-summary-v1",
        "metadata": metadata,
        "locked_thresholds": THRESHOLDS,
        "runs": runs,
        "groups": groups,
        "sampler_overhead": overhead,
    }
    (session_dir / "summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    (session_dir / "report.md").write_text(markdown_report(summary), encoding="utf-8")
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("session_dir", type=Path)
    args = parser.parse_args()
    summary = summarize_session(args.session_dir)
    print(f"summary={args.session_dir / 'summary.json'} groups={len(summary['groups'])}")
    print(f"report={args.session_dir / 'report.md'}")


if __name__ == "__main__":
    main()
