#!/usr/bin/env python3
"""Sample Linux process RSS and CPU from /proc without external packages."""

import argparse
import csv
import os
from pathlib import Path
import statistics
import time


def parse_process(value):
    label, separator, pid_text = value.partition("=")
    if not separator or not label or not pid_text.isdigit():
        raise argparse.ArgumentTypeError("process must use LABEL=PID")
    return label, int(pid_text)


def read_cpu_ticks(pid):
    stat = Path(f"/proc/{pid}/stat").read_text(encoding="utf-8")
    fields_after_name = stat[stat.rfind(")") + 2 :].split()
    user_ticks = int(fields_after_name[11])
    system_ticks = int(fields_after_name[12])
    return user_ticks + system_ticks


def read_rss_kib(pid):
    with Path(f"/proc/{pid}/status").open(encoding="utf-8") as status:
        for line in status:
            if line.startswith("VmRSS:"):
                return int(line.split()[1])
    raise RuntimeError(f"VmRSS is unavailable for PID {pid}")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--process", action="append", type=parse_process, required=True)
    parser.add_argument("--duration", type=float, required=True)
    parser.add_argument("--interval", type=float, default=1.0)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def summarize(rows, labels):
    for label in labels + ["combined"]:
        rss_values = [row[f"{label}_rss_kib"] for row in rows]
        cpu_values = [row[f"{label}_cpu_percent"] for row in rows]
        print(
            f"{label}: samples={len(rows)} "
            f"rss_kib_avg={statistics.fmean(rss_values):.1f} "
            f"rss_kib_min={min(rss_values)} rss_kib_max={max(rss_values)} "
            f"cpu_percent_avg={statistics.fmean(cpu_values):.3f} "
            f"cpu_percent_min={min(cpu_values):.3f} "
            f"cpu_percent_max={max(cpu_values):.3f}"
        )


def main():
    args = parse_args()
    if args.duration <= 0 or args.interval <= 0:
        raise SystemExit("duration and interval must be positive")

    processes = dict(args.process)
    clock_ticks = os.sysconf(os.sysconf_names["SC_CLK_TCK"])
    previous_time = time.monotonic()
    previous_ticks = {label: read_cpu_ticks(pid) for label, pid in processes.items()}
    started = previous_time
    rows = []

    while True:
        target = previous_time + args.interval
        time.sleep(max(0.0, target - time.monotonic()))
        sampled_at = time.monotonic()
        elapsed = sampled_at - previous_time
        row = {"elapsed_seconds": sampled_at - started}
        combined_rss = 0
        combined_cpu = 0.0

        for label, pid in processes.items():
            ticks = read_cpu_ticks(pid)
            rss_kib = read_rss_kib(pid)
            cpu_percent = (ticks - previous_ticks[label]) / clock_ticks / elapsed * 100.0
            row[f"{label}_rss_kib"] = rss_kib
            row[f"{label}_cpu_percent"] = cpu_percent
            previous_ticks[label] = ticks
            combined_rss += rss_kib
            combined_cpu += cpu_percent

        row["combined_rss_kib"] = combined_rss
        row["combined_cpu_percent"] = combined_cpu
        rows.append(row)
        previous_time = sampled_at
        if sampled_at - started >= args.duration:
            break

    args.output.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = list(rows[0])
    with args.output.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"output={args.output} duration_seconds={rows[-1]['elapsed_seconds']:.3f}")
    summarize(rows, list(processes))


if __name__ == "__main__":
    main()
