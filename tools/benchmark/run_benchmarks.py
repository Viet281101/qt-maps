#!/usr/bin/env python3
"""Run the reproducible QtMaps Unit 06 benchmark matrix."""

import argparse
import datetime as dt
import json
import os
import platform
import re
import shlex
import statistics
import subprocess
import sys
import time
from pathlib import Path

from summarize_benchmarks import summarize_session


SCENARIOS = ("startup", "idle", "pan_zoom", "route", "navigation_replay")
DEFAULT_DURATIONS = {
    "startup": 1,
    "idle": 60,
    "pan_zoom": 60,
    "route": 1,
    "navigation_replay": 60,
}
EVENT_PREFIX = "QTMAPS_BENCHMARK_EVENT"


def command_output(command, cwd, default="Unknown"):
    try:
        result = subprocess.run(
            command,
            cwd=cwd,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=15,
        )
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return default
    return result.stdout.strip() or default


def directory_size(path):
    if not path.exists():
        return 0
    if path.is_file():
        return path.stat().st_size
    return sum(entry.stat().st_size for entry in path.rglob("*") if entry.is_file())


def cpu_metadata():
    model = "Unknown"
    physical_pairs = set()
    current = {}
    cpuinfo = Path("/proc/cpuinfo")
    if cpuinfo.exists():
        for line in cpuinfo.read_text(encoding="utf-8", errors="replace").splitlines() + [""]:
            if not line:
                if current:
                    model = current.get("model name", model)
                    physical_pairs.add((current.get("physical id", "0"), current.get("core id", "0")))
                    current = {}
                continue
            key, separator, value = line.partition(":")
            if separator:
                current[key.strip()] = value.strip()
    return model, len(physical_pairs) or None, os.cpu_count()


def memory_bytes():
    for line in Path("/proc/meminfo").read_text(encoding="utf-8").splitlines():
        if line.startswith("MemTotal:"):
            return int(line.split()[1]) * 1024
    return 0


def parse_maplibre_version(root):
    version_file = root / "generated/unit-01b/maplibre-install/lib/cmake/QMapLibre/QMapLibreConfigVersion.cmake"
    if not version_file.exists():
        return "Unknown"
    match = re.search(r'set\(PACKAGE_VERSION "([^"]+)"\)', version_file.read_text(encoding="utf-8"))
    return match.group(1) if match else "Unknown"


def graph_path(config_path):
    try:
        config = json.loads(config_path.read_text(encoding="utf-8"))
        value = config.get("mjolnir", {}).get("tile_dir")
        return Path(value) if value else None
    except (OSError, json.JSONDecodeError):
        return None


def collect_metadata(args, root, output_dir):
    model, physical_cores, logical_cpus = cpu_metadata()
    package = args.map_package.resolve()
    graph = graph_path(args.valhalla_config.resolve())
    git_commit = command_output(["git", "rev-parse", "HEAD"], root)
    dirty = subprocess.run(["git", "diff", "--quiet", "HEAD"], cwd=root).returncode != 0
    qmake = args.qt_prefix / "bin/qmake"
    qt_version = command_output([str(qmake), "-query", "QT_VERSION"], root)
    valhalla_version = command_output([str(args.valhalla_service), "--version"], root)
    return {
        "schema_version": "qtmaps-benchmark-session-v1",
        "created_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "output_directory": str(output_dir),
        "cpu_model": model,
        "physical_cores": physical_cores,
        "logical_cpus": logical_cpus,
        "ram_bytes": memory_bytes(),
        "kernel": platform.release(),
        "platform": platform.platform(),
        "qt_version": qt_version,
        "maplibre_version": parse_maplibre_version(root),
        "valhalla_version": valhalla_version,
        "git_commit": git_commit + ("+working-tree" if dirty else ""),
        "fast_sampling_interval_ms": args.interval,
        "pss_sampling_interval_ms": args.pss_interval,
        "warmup_seconds": args.warmup,
        "repetitions": args.repeat,
        "network_modes": args.mode,
        "scenarios": args.scenarios,
        "map_package": str(package),
        "tiles_bytes": directory_size(package),
        "valhalla_graph": str(graph) if graph else "Unknown",
        "graph_bytes": directory_size(graph) if graph else 0,
    }


def runtime_environment(root, platform_name=None):
    environment = os.environ.copy()
    qml_runtime = root / "generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5"
    maplibre = root / "generated/unit-01b/maplibre-install"
    for name, paths in {
        "LD_LIBRARY_PATH": [maplibre / "lib"],
        "QT_PLUGIN_PATH": [maplibre / "plugins", qml_runtime / "plugins"],
        "QML2_IMPORT_PATH": [maplibre / "qml", qml_runtime / "qml"],
    }.items():
        existing = environment.get(name)
        values = [str(path) for path in paths]
        if existing:
            values.append(existing)
        environment[name] = os.pathsep.join(values)
    environment.setdefault("QSG_RENDER_LOOP", "threaded")
    # A loopback-only network namespace also isolates abstract AF_UNIX sockets.
    # Do not let optional desktop accessibility/session-manager handshakes add a
    # one-second retry to benchmark first-frame timing. These are benchmark-child
    # settings only and do not affect normal application launches.
    environment["QT_ACCESSIBILITY"] = "0"
    environment["NO_AT_BRIDGE"] = "1"
    environment.pop("SESSION_MANAGER", None)
    if platform_name:
        environment["QT_QPA_PLATFORM"] = platform_name
    return environment


def offline_prefix():
    return [
        "unshare",
        "-Urn",
        "bash",
        "-c",
        'set -euo pipefail; ip link set lo up; exec "$@"',
        "qtmaps-offline",
    ]


def verify_offline_available(root, environment):
    result = subprocess.run(
        offline_prefix() + ["/bin/true"],
        cwd=root,
        env=environment,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        raise RuntimeError(
            "Offline mode requires an unprivileged user/network namespace with iproute2; "
            f"preflight failed: {result.stdout.strip()}"
        )


def parse_events(output):
    events = []
    for line in output.splitlines():
        marker = line.find(EVENT_PREFIX)
        if marker < 0:
            continue
        json_start = line.find("{", marker + len(EVENT_PREFIX))
        if json_start < 0:
            continue
        try:
            events.append(json.loads(line[json_start:]))
        except json.JSONDecodeError:
            events.append({"event": "parse_error", "line": line})
    return events


def read_external_snapshot():
    snapshots = {}
    for entry in Path("/proc").iterdir():
        if not entry.name.isdigit():
            continue
        try:
            stat = (entry / "stat").read_text(encoding="utf-8")
            closing = stat.rfind(")")
            fields = stat[closing + 2 :].split()
            parent_pid = int(fields[1])
            cpu_ticks = int(fields[11]) + int(fields[12])
            start_ticks = int(fields[19])
            rss_bytes = None
            with (entry / "status").open(encoding="utf-8") as status:
                for line in status:
                    if line.startswith("VmRSS:"):
                        rss_bytes = int(line.split()[1]) * 1024
                        break
            if rss_bytes is None:
                continue
            pid = int(entry.name)
            snapshots[pid] = {
                "pid": pid,
                "ppid": parent_pid,
                "ticks": cpu_ticks,
                "start": start_ticks,
                "rss": rss_bytes,
            }
        except (OSError, ValueError, IndexError):
            continue
    return snapshots


def external_tree_pids(root_pid, snapshots):
    if root_pid not in snapshots:
        return []
    included = {root_pid}
    changed = True
    while changed:
        changed = False
        for pid, snapshot in snapshots.items():
            if pid not in included and snapshot["ppid"] in included:
                included.add(pid)
                changed = True
    return sorted(included)


def read_external_pss(pids):
    total = 0
    for pid in pids:
        try:
            with Path(f"/proc/{pid}/smaps_rollup").open(encoding="utf-8") as source:
                for line in source:
                    if line.startswith("Pss:"):
                        total += int(line.split()[1]) * 1024
                        break
                else:
                    return None
        except (OSError, ValueError):
            return None
    return total


def external_sample_process(process, warmup, duration, interval, pss_interval):
    clock_ticks = os.sysconf(os.sysconf_names["SC_CLK_TCK"])
    logical_cpus = max(1, os.cpu_count() or 1)
    started = time.monotonic()
    previous_time = started
    previous = read_external_snapshot()
    last_pss_time = started - pss_interval
    cached_pss = None
    rows = []
    while process.poll() is None:
        time.sleep(max(0.0, previous_time + interval - time.monotonic()))
        sampled_at = time.monotonic()
        current = read_external_snapshot()
        pids = external_tree_pids(process.pid, current)
        if not pids:
            break
        if sampled_at - last_pss_time >= pss_interval:
            cached_pss = read_external_pss(pids)
            last_pss_time = sampled_at
        tick_delta = 0
        for pid in pids:
            current_process = current[pid]
            previous_process = previous.get(pid)
            if (
                previous_process
                and previous_process["start"] == current_process["start"]
                and current_process["ticks"] >= previous_process["ticks"]
            ):
                tick_delta += current_process["ticks"] - previous_process["ticks"]
        elapsed = sampled_at - previous_time
        if sampled_at - started >= warmup:
            cpu_one = tick_delta * 100.0 / clock_ticks / elapsed
            rows.append(
                {
                    "cpu_all": cpu_one / logical_cpus,
                    "rss_mib": sum(current[pid]["rss"] for pid in pids) / (1024 * 1024),
                    "pss_mib": cached_pss / (1024 * 1024) if cached_pss is not None else None,
                }
            )
        previous = current
        previous_time = sampled_at
        if sampled_at - started >= warmup + duration:
            break
    return rows


def overhead_command(args, root, enabled, output_path, total_seconds):
    command = [
        str(args.app),
        "--map-package",
        str(args.map_package),
        "--valhalla-service",
        str(args.valhalla_service),
        "--valhalla-config",
        str(args.valhalla_config),
        "--valhalla-port",
        str(args.valhalla_port),
        "--network-mode",
        "host-network-overhead-comparison",
        "--dataset-id",
        args.dataset_id,
        "--dataset-coverage",
        args.dataset_coverage,
    ]
    if enabled:
        command.extend(
            [
                "--benchmark",
                "idle",
                "--duration",
                str(args.overhead_duration),
                "--benchmark-warmup",
                str(args.overhead_warmup),
                "--benchmark-interval",
                str(args.interval),
                "--benchmark-pss-interval",
                str(args.pss_interval),
                "--benchmark-output",
                str(output_path),
                "--benchmark-run-id",
                "sampler-overhead-on",
            ]
        )
    else:
        command.extend(
            [
                "--unit03-local-map",
                "--routing-demo",
                "--disable-instrumentation",
                "--exit-after",
                str(total_seconds),
            ]
        )
    if args.fixture_only:
        command.append("--routing-fixture-only")
    return command


def summarize_external_rows(rows):
    if not rows:
        return {"cpu_all_average": None, "pss_average_mib": None, "rss_average_mib": None}
    pss = [row["pss_mib"] for row in rows if row["pss_mib"] is not None]
    return {
        "cpu_all_average": statistics.fmean(row["cpu_all"] for row in rows),
        "pss_average_mib": statistics.fmean(pss) if pss else None,
        "rss_average_mib": statistics.fmean(row["rss_mib"] for row in rows),
    }


def run_sampler_overhead(args, root, session_dir, environment):
    overhead_dir = session_dir / "sampler-overhead"
    overhead_dir.mkdir()
    total_seconds = args.overhead_warmup + args.overhead_duration
    comparisons = []
    for repetition in range(1, args.repeat + 1):
        results = {}
        for enabled in (False, True):
            label = "on" if enabled else "off"
            log_path = overhead_dir / f"r{repetition}-{label}.log"
            csv_path = overhead_dir / f"r{repetition}-{label}.csv"
            command = overhead_command(args, root, enabled, csv_path, total_seconds)
            with log_path.open("w", encoding="utf-8") as log:
                process = subprocess.Popen(
                    command,
                    cwd=root,
                    env=environment,
                    text=True,
                    stdout=log,
                    stderr=subprocess.STDOUT,
                )
                rows = external_sample_process(
                    process,
                    args.overhead_warmup,
                    args.overhead_duration,
                    args.interval / 1000.0,
                    args.pss_interval / 1000.0,
                )
                try:
                    exit_code = process.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    process.terminate()
                    exit_code = process.wait(timeout=5)
            results[label] = {"exit_code": exit_code, **summarize_external_rows(rows)}
        on = results["on"]
        off = results["off"]
        comparisons.append(
            {
                "repetition": repetition,
                "off": off,
                "on": on,
                "cpu_overhead_points": on["cpu_all_average"] - off["cpu_all_average"]
                if on["cpu_all_average"] is not None and off["cpu_all_average"] is not None
                else None,
                "pss_overhead_mib": on["pss_average_mib"] - off["pss_average_mib"]
                if on["pss_average_mib"] is not None and off["pss_average_mib"] is not None
                else None,
                "rss_overhead_mib": on["rss_average_mib"] - off["rss_average_mib"]
                if on["rss_average_mib"] is not None and off["rss_average_mib"] is not None
                else None,
            }
        )
        print(f"sampler-overhead-r{repetition}: complete", flush=True)
    payload = {
        "schema_version": "qtmaps-sampler-overhead-v1",
        "warmup_seconds": args.overhead_warmup,
        "duration_seconds": args.overhead_duration,
        "comparisons": comparisons,
    }
    (session_dir / "sampler-overhead.json").write_text(
        json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )


def run_once(args, root, session_dir, scenario, network_mode, repetition, environment):
    run_id = f"{scenario}-{network_mode}-r{repetition}"
    run_dir = session_dir / "runs" / run_id
    run_dir.mkdir(parents=True, exist_ok=False)
    aggregate_csv = run_dir / "metrics.csv"
    duration = args.duration if args.duration is not None else DEFAULT_DURATIONS[scenario]
    warmup = 0 if scenario == "startup" else args.warmup
    command = [
        str(args.app),
        "--benchmark",
        scenario,
        "--duration",
        str(duration),
        "--benchmark-warmup",
        str(warmup),
        "--benchmark-interval",
        str(args.interval),
        "--benchmark-pss-interval",
        str(args.pss_interval),
        "--benchmark-output",
        str(aggregate_csv),
        "--benchmark-run-id",
        run_id,
        "--network-mode",
        "host-network-local-services" if network_mode == "online" else "loopback-only-namespace",
        "--dataset-id",
        args.dataset_id,
        "--dataset-coverage",
        args.dataset_coverage,
        "--map-package",
        str(args.map_package),
        "--valhalla-service",
        str(args.valhalla_service),
        "--valhalla-config",
        str(args.valhalla_config),
        "--valhalla-port",
        str(args.valhalla_port),
    ]
    if args.fixture_only:
        command.append("--routing-fixture-only")
    executed = offline_prefix() + command if network_mode == "offline" else command
    timeout_seconds = args.timeout or max(45, warmup + duration + 30)
    try:
        result = subprocess.run(
            executed,
            cwd=root,
            env=environment,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=timeout_seconds,
        )
        output = result.stdout
        exit_code = result.returncode
        timed_out = False
    except subprocess.TimeoutExpired as error:
        output = (error.stdout or "") + (error.stderr or "")
        exit_code = 124
        timed_out = True
    (run_dir / "run.log").write_text(output, encoding="utf-8")
    events = parse_events(output)
    run = {
        "schema_version": "qtmaps-benchmark-run-v1",
        "run_id": run_id,
        "scenario": scenario,
        "network_mode": network_mode,
        "repetition": repetition,
        "duration_seconds": duration,
        "warmup_seconds": warmup,
        "exit_code": exit_code,
        "timed_out": timed_out,
        "command": shlex.join(executed),
        "aggregate_csv": aggregate_csv.name,
        "process_csv": "metrics-processes.csv",
        "events": events,
    }
    (run_dir / "run.json").write_text(
        json.dumps(run, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    result_events = [event for event in events if event.get("event") == "result"]
    succeeded = exit_code == 0 and result_events and result_events[-1].get("success")
    print(f"{run_id}: {'OK' if succeeded else 'FAILED'} exit={exit_code}", flush=True)
    return bool(succeeded)


def parse_args(root):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", type=Path, default=root / "build-qt515/qtmaps")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--scenarios", default=",".join(SCENARIOS))
    parser.add_argument("--mode", choices=("online", "offline", "both"), default="both")
    parser.add_argument("--repeat", type=int, default=3)
    parser.add_argument("--duration", type=int)
    parser.add_argument("--warmup", type=int, default=5)
    parser.add_argument("--interval", type=int, default=500)
    parser.add_argument("--pss-interval", type=int, default=2000)
    parser.add_argument("--timeout", type=int)
    parser.add_argument("--overhead-warmup", type=int, default=5)
    parser.add_argument("--overhead-duration", type=int, default=10)
    parser.add_argument("--skip-overhead", action="store_true")
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--fixture-only", action="store_true")
    parser.add_argument("--platform")
    parser.add_argument("--qt-prefix", type=Path, default=Path("/usr"))
    parser.add_argument("--map-package", type=Path, default=root / "data/packages/current")
    parser.add_argument(
        "--valhalla-service",
        type=Path,
        default=root
        / "generated/unit-04/valhalla-venv/lib/python3.12/site-packages/valhalla/bin/valhalla_service",
    )
    parser.add_argument(
        "--valhalla-config",
        type=Path,
        default=root / "generated/unit-04/valhalla/valhalla.json",
    )
    parser.add_argument("--valhalla-port", type=int, default=18002)
    parser.add_argument("--dataset-id", default="hanoi-20261024-v1")
    parser.add_argument(
        "--dataset-coverage", default="Hanoi bounds 105.78,20.97,105.90,21.10; z12-z15"
    )
    args = parser.parse_args()
    args.scenarios = [item.strip() for item in args.scenarios.split(",") if item.strip()]
    unknown = sorted(set(args.scenarios) - set(SCENARIOS))
    if unknown:
        parser.error(f"unknown scenarios: {', '.join(unknown)}")
    if (
        args.repeat < 1
        or args.warmup < 0
        or args.overhead_warmup < 0
        or args.overhead_duration < 1
        or args.interval < 100
        or args.pss_interval < args.interval
    ):
        parser.error("repeat >= 1, warmup >= 0, interval >= 100, and pss-interval >= interval")
    return args


def main():
    root = Path(__file__).resolve().parents[2]
    args = parse_args(root)
    args.app = args.app.resolve()
    args.map_package = args.map_package.resolve()
    args.valhalla_service = args.valhalla_service.resolve()
    args.valhalla_config = args.valhalla_config.resolve()
    if not args.skip_build:
        subprocess.run(["make", "build"], cwd=root, check=True)
    for required in (args.app, args.map_package, args.valhalla_service, args.valhalla_config):
        if not required.exists():
            raise SystemExit(f"required benchmark path does not exist: {required}")

    timestamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    session_dir = (args.output or root / "generated/benchmarks" / timestamp).resolve()
    session_dir.mkdir(parents=True, exist_ok=False)
    environment = runtime_environment(root, args.platform)
    modes = ("online", "offline") if args.mode == "both" else (args.mode,)
    if "offline" in modes:
        verify_offline_available(root, environment)
    metadata = collect_metadata(args, root, session_dir)
    (session_dir / "session-metadata.json").write_text(
        json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )

    all_succeeded = True
    if not args.skip_overhead:
        run_sampler_overhead(args, root, session_dir, environment)
    for network_mode in modes:
        for scenario in args.scenarios:
            for repetition in range(1, args.repeat + 1):
                all_succeeded = (
                    run_once(
                        args,
                        root,
                        session_dir,
                        scenario,
                        network_mode,
                        repetition,
                        environment,
                    )
                    and all_succeeded
                )
    summary = summarize_session(session_dir)
    threshold_failures = [
        check
        for group in summary["groups"]
        for check in group["thresholds"]
        if check["status"].startswith("FAIL")
    ]
    threshold_failures.extend(
        check
        for check in summary["sampler_overhead"]["checks"]
        if check["status"].startswith("FAIL")
    )
    print(f"report={session_dir / 'report.md'}")
    print(f"summary={session_dir / 'summary.json'}")
    if not all_succeeded:
        raise SystemExit("one or more benchmark runs failed")
    if threshold_failures:
        print(f"locked-threshold failures={len(threshold_failures)}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
