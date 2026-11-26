#!/usr/bin/env python3
"""Build the approved Vietnam Valhalla graph with reproducible local evidence."""

import argparse
import hashlib
import json
import pathlib
import subprocess
import sys


EXPECTED_PBF_SHA256 = "4e5243c3116dc089a298f4cd7218e1257adf4b48355e42936360df5ce3200d43"


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pbf", type=pathlib.Path, required=True)
    parser.add_argument("--base-config", type=pathlib.Path, required=True)
    parser.add_argument("--builder", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        parser.error("output must be a new or empty directory")
    actual_hash = sha256(args.pbf)
    if actual_hash != EXPECTED_PBF_SHA256:
        parser.error("PBF SHA-256 mismatch: " + actual_hash)
    output.mkdir(parents=True, exist_ok=True)
    tiles = output / "tiles"
    tiles.mkdir()
    config = json.loads(args.base_config.read_text())
    config["mjolnir"]["tile_dir"] = str(tiles)
    config["mjolnir"]["concurrency"] = 2
    config["httpd"]["service"]["listen"] = "tcp://127.0.0.1:18012"
    config_path = output / "valhalla.json"
    config_path.write_text(json.dumps(config, indent=2) + "\n")
    evidence = {
        "source_pbf": str(args.pbf.resolve()),
        "source_sha256": actual_hash,
        "config_sha256": sha256(config_path),
        "builder": str(args.builder.resolve()),
        "builder_version": subprocess.check_output([str(args.builder), "--version"], text=True).strip(),
        "concurrency": 2,
    }
    (output / "build-inputs.json").write_text(json.dumps(evidence, indent=2) + "\n")
    with (output / "build.log").open("w") as log:
        result = subprocess.run(
            ["/usr/bin/time", "-v", "-o", str(output / "build.time"),
             str(args.builder), "--config", str(config_path), "--concurrency", "2", str(args.pbf)],
            stdout=log, stderr=subprocess.STDOUT, check=False,
        )
    evidence["exit_code"] = result.returncode
    evidence["graph_files"] = sum(1 for _ in tiles.rglob("*.gph"))
    evidence["graph_bytes"] = sum(p.stat().st_size for p in tiles.rglob("*") if p.is_file())
    (output / "build-result.json").write_text(json.dumps(evidence, indent=2) + "\n")
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
