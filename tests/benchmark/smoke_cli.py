#!/usr/bin/env python3
"""Run a short end-to-end Unit 06 startup scenario."""

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: smoke_cli.py APP REPOSITORY_ROOT")
    app = Path(sys.argv[1]).resolve()
    root = Path(sys.argv[2]).resolve()
    qml_runtime = root / "generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5"
    maplibre = root / "generated/unit-01b/maplibre-install"
    environment = os.environ.copy()
    environment.update(
        {
            "QT_QPA_PLATFORM": "offscreen",
            "QSG_RENDER_LOOP": "threaded",
            "LD_LIBRARY_PATH": str(maplibre / "lib"),
            "QT_PLUGIN_PATH": os.pathsep.join(
                [str(maplibre / "plugins"), str(qml_runtime / "plugins")]
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
