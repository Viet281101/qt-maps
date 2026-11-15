#!/usr/bin/env python3
"""Build a bounded offline Hanoi FTS5 package without network access."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import sqlite3

BOUNDS = [105.78, 20.97, 105.90, 21.10]
MAX_BYTES = 20 * 1024 * 1024
ATTRIBUTION = "© OpenStreetMap contributors, ODbL 1.0"


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def records(path):
    seen = set()
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        try:
            record = json.loads(line)
            source_id = record["source_id"]
            name = record["name"].strip()
            latitude = float(record["latitude"])
            longitude = float(record["longitude"])
        except (KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
            raise SystemExit(f"Invalid place record at line {line_number}") from error
        if not source_id or not name or source_id in seen:
            raise SystemExit(f"Invalid or duplicate place record at line {line_number}")
        if not BOUNDS[1] <= latitude <= BOUNDS[3] or not BOUNDS[0] <= longitude <= BOUNDS[2]:
            raise SystemExit(f"Place record outside Hanoi bounds at line {line_number}")
        seen.add(source_id)
        yield source_id, name, " ".join(record.get("aliases", [])), record.get("category", "place"), latitude, longitude


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--places", type=Path, required=True)
    parser.add_argument("--source-pbf", type=Path, required=True)
    parser.add_argument("--retrieved-on", required=True)
    parser.add_argument("--date", default=datetime.date.today().strftime("%Y%m%d"))
    parser.add_argument("--revision", default="r1")
    parser.add_argument("--output-root", type=Path, default=Path("data/search-packages"))
    args = parser.parse_args()
    places, pbf = args.places.resolve(), args.source_pbf.resolve()
    if not places.is_file() or not pbf.is_file():
        raise SystemExit("Prepared places or source PBF is missing")
    package_id = f"hanoi-search-{args.date}-v1-{args.revision}"
    root = args.output_root.resolve(); package = root / package_id; staging = root / f".{package_id}.staging"
    if package.exists() or staging.exists():
        raise SystemExit(f"Refusing to overwrite package: {package}")
    staging.mkdir(parents=True)
    try:
        database_path = staging / "places.sqlite"
        database = sqlite3.connect(database_path)
        database.executescript("CREATE TABLE places (id INTEGER PRIMARY KEY, source_id TEXT UNIQUE NOT NULL, name TEXT NOT NULL, aliases TEXT NOT NULL, category TEXT NOT NULL, latitude REAL NOT NULL, longitude REAL NOT NULL); CREATE VIRTUAL TABLE places_fts USING fts5(name, aliases, content='places', content_rowid='id', tokenize='unicode61 remove_diacritics 2');")
        rows = list(records(places))
        database.executemany("INSERT INTO places(source_id,name,aliases,category,latitude,longitude) VALUES (?,?,?,?,?,?)", rows)
        database.execute("INSERT INTO places_fts(places_fts) VALUES ('rebuild')")
        database.commit(); database.execute("VACUUM"); database.close()
        size = database_path.stat().st_size
        if size > MAX_BYTES: raise SystemExit(f"Search package exceeds 20 MiB cap: {size}")
        manifest = {"schema_version":"qtmaps-search-package-v1","package_id":package_id,"bounds":BOUNDS,"attribution":ATTRIBUTION,"source":{"url":"https://download.geofabrik.de/asia/vietnam-latest.osm.pbf","pbf_name":pbf.name,"pbf_sha256":digest(pbf),"retrieved_on":args.retrieved_on},"places":{"path":"places.sqlite","count":len(rows),"size_bytes":size,"sha256":digest(database_path)},"generation":{"network_access":False,"pipeline_revision":"unit61-v1"}}
        (staging / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        os.replace(staging, package)
        current_tmp = root / ".current.tmp"; current_tmp.unlink(missing_ok=True); current_tmp.symlink_to(package.name, target_is_directory=True); os.replace(current_tmp, root / "current")
    except BaseException:
        shutil.rmtree(staging, ignore_errors=True); raise


if __name__ == "__main__":
    main()
