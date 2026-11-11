#!/usr/bin/env python3
"""Run a short end-to-end Unit 06 startup scenario."""

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path


def qmake_query(qmake: Path, key: str) -> str:
    result = subprocess.run(
        [str(qmake), "-query", key],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode != 0:
        raise SystemExit(f"qmake query failed for {key}: {result.stderr.strip()}")
    value = result.stdout.strip()
    if not value:
        raise SystemExit(f"qmake returned an empty value for {key}")
    return value


def selected_qt_runtime(environment: dict[str, str]) -> tuple[Path, Path]:
    qt_prefix = Path(environment.get("QTMAPS_QT_PREFIX", "/usr")).expanduser().resolve()
    qmake = qt_prefix / "bin" / "qmake"
    if not qmake.is_file() or not qmake.stat().st_mode & 0o111:
        raise SystemExit(f"qmake is not executable under QTMAPS_QT_PREFIX: {qmake}")
    expected_version = environment.get("QTMAPS_QT_EXPECTED_VERSION", "5.15.18")
    actual_version = qmake_query(qmake, "QT_VERSION")
    if actual_version != expected_version:
        raise SystemExit(
            f"expected Qt {expected_version} from {qmake}, found {actual_version}"
        )
    archdata = Path(qmake_query(qmake, "QT_INSTALL_ARCHDATA"))
    plugin_root = Path(qmake_query(qmake, "QT_INSTALL_PLUGINS"))
    if not (archdata / "qml").is_dir():
        raise SystemExit(f"Qt QML runtime is missing: {archdata / 'qml'}")
    if not plugin_root.is_dir():
        raise SystemExit(f"Qt plugin root is missing: {plugin_root}")
    return archdata, plugin_root


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: smoke_cli.py APP REPOSITORY_ROOT")
    app = Path(sys.argv[1]).resolve()
    root = Path(sys.argv[2]).resolve()
    qml_runtime, qt_plugin_root = selected_qt_runtime(os.environ)
    maplibre = Path(
        os.environ.get(
            "QTMAPS_MAPLIBRE_INSTALL", root / "generated/unit-01b/maplibre-install"
        )
    ).expanduser().resolve()
    environment = os.environ.copy()
    environment.update(
        {
            "QT_QPA_PLATFORM": "offscreen",
            "QSG_RENDER_LOOP": "threaded",
            "QTMAPS_ASSERT_SOFTWARE_BACKEND": "1",
            "LD_LIBRARY_PATH": str(maplibre / "lib"),
            "QT_PLUGIN_PATH": os.pathsep.join(
                [str(maplibre / "plugins"), str(qt_plugin_root)]
            ),
            "QML2_IMPORT_PATH": os.pathsep.join(
                [str(maplibre / "qml"), str(qml_runtime / "qml")]
            ),
        }
    )
    with tempfile.TemporaryDirectory(prefix="qtmaps-benchmark-smoke-") as directory:
        output = Path(directory) / "startup.csv"
        command = [
            str(app),
            "--benchmark",
            "startup",
            "--duration",
            "1",
            "--benchmark-warmup",
            "0",
            "--benchmark-output",
            str(output),
            "--benchmark-run-id",
            "ctest-startup-smoke",
            "--routing-fixture-only",
            "--map-package",
            str(root / "data/packages/current"),
        ]
        result = subprocess.run(
            command,
            cwd=root,
            env=environment,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=30,
        )
        if result.returncode != 0:
            print(result.stdout)
            raise SystemExit(f"startup scenario exited {result.returncode}")
        if "QTMAPS_CTEST_BACKEND QT_QUICK_BACKEND=software" not in result.stdout:
            print(result.stdout)
            raise SystemExit("qtmaps did not confirm QT_QUICK_BACKEND=software")
        events = []
        for line in result.stdout.splitlines():
            marker = line.find("QTMAPS_BENCHMARK_EVENT")
            json_start = line.find("{", marker) if marker >= 0 else -1
            if json_start >= 0:
                events.append(json.loads(line[json_start:]))
        event_names = {event.get("event") for event in events}
        required = {"map_ready", "first_rendered_map_frame", "startup_memory", "result"}
        if not required.issubset(event_names):
            raise SystemExit(f"missing benchmark events: {sorted(required - event_names)}")
        result_event = [event for event in events if event.get("event") == "result"][-1]
        if not result_event.get("success"):
            raise SystemExit(f"startup result failed: {result_event}")
        process_output = output.with_name("startup-processes.csv")
        if not output.exists() or not process_output.exists():
            raise SystemExit("benchmark CSV outputs were not created")
        header = process_output.read_text(encoding="utf-8").splitlines()[0]
        for column in ("pss_bytes", "rss_bytes", "cpu_percent_all_cores_100"):
            if column not in header:
                raise SystemExit(f"process CSV is missing {column}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
