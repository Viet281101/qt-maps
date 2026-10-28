#!/usr/bin/env python3

import csv
import json
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/benchmark"))

import summarize_benchmarks as summary  # noqa: E402


class BenchmarkSummaryTest(unittest.TestCase):
    def test_nearest_rank_and_median_range(self):
        self.assertEqual(summary.nearest_rank([10, 20, 30, 40], 95), 40)
        self.assertEqual(
            summary.stats([1, 3, 2]), {"median": 2, "min": 1, "max": 3, "count": 3}
        )

    def test_process_metrics_prioritize_complete_pss(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "process.csv"
            fieldnames = [
                "row_type",
                "role",
                "rss_bytes",
                "pss_bytes",
                "pss_available",
                "cpu_percent_one_core_100",
                "cpu_percent_all_cores_100",
                "sampler_duration_ms",
                "threads",
                "process_count",
            ]
            rows = [
                {
                    "row_type": "tree_total",
                    "role": "total",
                    "rss_bytes": str(200 * summary.MIB),
                    "pss_bytes": str(150 * summary.MIB),
                    "pss_available": "1",
                    "cpu_percent_one_core_100": "40",
                    "cpu_percent_all_cores_100": "10",
                    "sampler_duration_ms": "5",
                    "threads": "10",
                    "process_count": "2",
                },
                {
                    "row_type": "tree_total",
                    "role": "total",
                    "rss_bytes": str(220 * summary.MIB),
                    "pss_bytes": "0",
                    "pss_available": "0",
                    "cpu_percent_one_core_100": "80",
                    "cpu_percent_all_cores_100": "20",
                    "sampler_duration_ms": "8",
                    "threads": "12",
                    "process_count": "2",
                },
                {
                    "row_type": "process",
                    "role": "valhalla",
                    "rss_bytes": str(100 * summary.MIB),
                    "pss_bytes": str(90 * summary.MIB),
                    "pss_available": "1",
                    "cpu_percent_all_cores_100": "15",
                },
            ]
            with path.open("w", newline="", encoding="utf-8") as output:
                writer = csv.DictWriter(output, fieldnames=fieldnames)
                writer.writeheader()
                writer.writerows(rows)

            metrics = summary.read_process_metrics(path)
            self.assertEqual(metrics["pss_average_mib"], 150)
            self.assertEqual(metrics["rss_average_mib"], 210)
            self.assertEqual(metrics["cpu_one_average"], 60)
            self.assertEqual(metrics["cpu_all_peak"], 20)
            self.assertEqual(metrics["valhalla_cpu_all_peak"], 15)

    def test_locked_threshold_failure_is_not_retuned(self):
        group = {
            "scenario": "idle",
            "metrics": {
                "pss_average_mib": summary.stats([560, 570, 580]),
                "pss_peak_mib": summary.stats([600, 610, 620]),
                "rss_average_mib": summary.stats([600, 610, 620]),
                "rss_peak_mib": summary.stats([700, 710, 720]),
                "sampler_duration_p95_ms": summary.stats([10, 11, 12]),
                "cpu_all_average": summary.stats([4, 6, 8]),
                "cpu_all_peak": summary.stats([10, 12, 14]),
            },
        }
        checks = summary.evaluate_thresholds(group)
        pss = next(check for check in checks if check["name"] == "PSS average MiB")
        cpu = next(check for check in checks if check["name"] == "Machine CPU average %")
        self.assertEqual(pss["threshold"], 550.0)
        self.assertEqual(pss["status"], "FAIL / không đạt")
        self.assertEqual(cpu["status"], "FAIL / không đạt")

    def test_sampler_overhead_uses_locked_cpu_and_pss_limits(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sampler-overhead.json"
            path.write_text(
                json.dumps(
                    {
                        "comparisons": [
                            {"cpu_overhead_points": 0.8, "pss_overhead_mib": 9.0},
                            {"cpu_overhead_points": 1.2, "pss_overhead_mib": 11.0},
                            {"cpu_overhead_points": 1.4, "pss_overhead_mib": 12.0},
                        ]
                    }
                ),
                encoding="utf-8",
            )
            overhead = summary.summarize_overhead(path)
            self.assertEqual(overhead["cpu_overhead_points"]["median"], 1.2)
            self.assertEqual(overhead["pss_overhead_mib"]["median"], 11.0)
            self.assertTrue(all(check["status"] == "FAIL / không đạt" for check in overhead["checks"]))

    def test_startup_uses_memory_captured_at_first_frame(self):
        with tempfile.TemporaryDirectory() as directory:
            run_dir = Path(directory)
            process_csv = run_dir / "metrics-processes.csv"
            process_csv.write_text("row_type,pss_bytes,rss_bytes\n", encoding="utf-8")
            (run_dir / "run.json").write_text(
                json.dumps(
                    {
                        "scenario": "startup",
                        "network_mode": "online",
                        "repetition": 1,
                        "run_id": "startup-online-r1",
                        "exit_code": 0,
                        "process_csv": process_csv.name,
                        "events": [
                            {
                                "event": "result",
                                "success": True,
                                "startup_ms": 800,
                                "pss_mib": 200,
                                "rss_mib": 300,
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )
            run = summary.summarize_run(run_dir / "run.json")
            self.assertEqual(run["metrics"]["pss_average_mib"], 200)
            self.assertEqual(run["metrics"]["rss_peak_mib"], 300)


if __name__ == "__main__":
    unittest.main()
