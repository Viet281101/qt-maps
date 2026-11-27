#!/usr/bin/env python3
"""Package a local real-data review candidate; never upload it to public CI."""

import argparse
import hashlib
import json
import os
import pathlib
import shutil
import subprocess


EVIDENCE = {
    "unit-14a-real-hanoi.md": "docs/rehearsal/unit-14a-7480-vm-results.md",
    "v1-a-national-graph.md": "docs/routing/v1-a-national-graph-20261126.md",
    "v1-b-real-route-replay.md": "docs/routing/v1-b-real-route-replay-20261126.md",
}


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--maplibre-licenses", type=pathlib.Path, required=True)
    parser.add_argument("--valhalla-metadata", type=pathlib.Path, required=True)
    args = parser.parse_args()
    repo = pathlib.Path(__file__).resolve().parents[2]
    output = args.output.resolve()
    if output.exists():
        parser.error("output must not exist")
    stage = args.stage.resolve()
    if not (stage / "metadata/SHA256SUMS").is_file():
        parser.error("stage checksum manifest missing")
    subprocess.run(["sha256sum", "-c", "metadata/SHA256SUMS"], cwd=stage,
                   stdout=subprocess.DEVNULL, check=True)
    root = output / "root"
    (root / "LICENSES/MapLibre").mkdir(parents=True)
    (root / "operator").mkdir()
    (root / "evidence").mkdir()
    shutil.copytree(stage, root / "stage", copy_function=os.link)
    shutil.copy2(repo / "tools/unit14a-deliver.sh", root / "install.sh")
    shutil.copy2(repo / "docs/packaging/linux-v1-internal-candidate.md",
                 root / "operator/README.md")
    shutil.copy2(repo / "LICENSE", root / "LICENSES/QtMaps-MIT.txt")
    shutil.copytree(args.maplibre_licenses, root / "LICENSES/MapLibre", dirs_exist_ok=True)
    shutil.copy2(args.valhalla_metadata / "licenses/COPYING",
                 root / "LICENSES/pyvalhalla-COPYING")
    shutil.copy2(args.valhalla_metadata / "sboms/auditwheel.cdx.json",
                 root / "LICENSES/pyvalhalla-auditwheel.cdx.json")
    for name, source in EVIDENCE.items():
        shutil.copy2(repo / source, root / "evidence" / name)
    (root / "LICENSES/NOTICE").write_text(
        "Internal real-data candidate only; not v1.0. Qt 5.15.18 and fonts "
        "come from Ubuntu 26.04 packages and their notices remain in "
        "/usr/share/doc. MapLibre and pyvalhalla texts are included. "
        "OSM/ODbL and Copernicus attribution is recorded in the data manifests. "
        "Exact transitive runtime notice review remains open before release.\n")
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo,
                                     text=True).strip()
    commit_time = subprocess.check_output(["git", "show", "-s", "--format=%ct", "HEAD"],
                                          cwd=repo, text=True).strip()
    manifest = {
        "kind": "internal-real-data-review-candidate",
        "release_status": "not-v1.0; benchmark-and-notice-gates-open",
        "target": "ubuntu-26.04-x86_64",
        "source_commit": commit,
        "qt": "5.15.18", "maplibre_native_qt": "3.0.0", "valhalla": "3.9.1",
        "stage_sha256s_sha256": digest(stage / "metadata/SHA256SUMS"),
        "app_sha256": digest(stage / "app/qtmaps"),
        "graph_source_pbf_sha256": "4e5243c3116dc089a298f4cd7218e1257adf4b48355e42936360df5ce3200d43",
        "review_files_sha256": {
            str(path.relative_to(root)): digest(path)
            for path in sorted((root / "evidence").iterdir())
        },
    }
    (root / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    archive = output / "qtmaps-v1-candidate-internal-linux-x86_64.tar.zst"
    with subprocess.Popen(["tar", "--sort=name", "--owner=0", "--group=0",
                           "--numeric-owner", "--mtime=@" + commit_time,
                           "-C", str(root), "-cf", "-", "."],
                          stdout=subprocess.PIPE) as tar:
        subprocess.run(["zstd", "-q", "-T2", "-o", str(archive)], stdin=tar.stdout,
                       check=True)
        tar.stdout.close()
        if tar.wait() != 0:
            raise SystemExit("tar failed")
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (output / "SHA256SUMS").write_text(
        f"{digest(archive)}  {archive.name}\n"
        f"{digest(output / 'manifest.json')}  manifest.json\n")
    print(archive, archive.stat().st_size, digest(archive))


if __name__ == "__main__":
    main()
