#!/usr/bin/env python3
"""Probe the frozen nationwide route matrix with the local Valhalla binary."""

import argparse
import json
import pathlib
import subprocess
import sys


ROUTES = (
    ("hanoi-haiphong", (21.0278, 105.8342), (20.8449, 106.6881)),
    ("hue-danang", (16.4637, 107.5909), (16.0544, 108.2022)),
    ("hcmc-cantho", (10.7769, 106.7009), (10.0452, 105.7469)),
    ("thainguyen-backan", (21.5942, 105.8482), (22.1470, 105.8348)),
    ("thudaumot-dian-provincial", (10.9804, 106.6519), (10.9077, 106.7695)),
)


def request(binary, config, start, end):
    payload = {"locations": [{"lat": start[0], "lon": start[1]},
                             {"lat": end[0], "lon": end[1]}],
               "costing": "auto", "units": "kilometers", "directions_options": {"language": "vi-VN"}}
    result = subprocess.run([str(binary), str(config), "route", json.dumps(payload)],
                            capture_output=True, text=True, timeout=120, check=False)
    return result.returncode, json.loads(result.stdout) if result.stdout.strip() else {}, result.stderr[-2000:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--service", type=pathlib.Path, required=True)
    parser.add_argument("--config", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    report = []
    failures = 0
    for name, start, end in ROUTES + (("out-of-coverage", (0.0, 0.0), (0.01, 0.01)),):
        code, response, stderr = request(args.service, args.config, start, end)
        trip = response.get("trip", {})
        legs = trip.get("legs", [])
        maneuvers = sum(len(leg.get("maneuvers", [])) for leg in legs)
        geometry = all(bool(leg.get("shape")) for leg in legs) and bool(legs)
        success = bool(legs) and maneuvers > 0 and geometry and trip.get("summary", {}).get("length", 0) > 0
        expected = name != "out-of-coverage"
        if success != expected:
            failures += 1
        report.append({"route": name, "exit_code": code, "success": success,
                       "length_km": trip.get("summary", {}).get("length"),
                       "maneuvers": maneuvers, "geometry": geometry,
                       "road_names": sorted({road for leg in legs for m in leg.get("maneuvers", [])
                                             for road in m.get("street_names", [])}),
                       "error": None if success else response.get("error") or response.get("error_code") or stderr})
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
