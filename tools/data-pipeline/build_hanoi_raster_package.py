#!/usr/bin/env python3
"""Package a complete local z12-z16 Hanoi PNG tile set without network access."""

import argparse
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import sqlite3
import struct


BOUNDS = [105.78, 20.97, 105.90, 21.10]
MIN_ZOOM = 12
MAX_ZOOM = 16
ATTRIBUTION = "© OpenStreetMap contributors, ODbL 1.0"
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def longitude_to_x(longitude, zoom):
    return math.floor((longitude + 180.0) / 360.0 * (1 << zoom))


def latitude_to_y(latitude, zoom):
    radians = math.radians(latitude)
    return math.floor(
        (1.0 - math.asinh(math.tan(radians)) / math.pi) / 2.0 * (1 << zoom)
    )


def expected_tiles():
    west, south, east, north = BOUNDS
    for zoom in range(MIN_ZOOM, MAX_ZOOM + 1):
        for x in range(longitude_to_x(west, zoom), longitude_to_x(east, zoom) + 1):
            for y in range(latitude_to_y(north, zoom), latitude_to_y(south, zoom) + 1):
                yield zoom, x, y


def validate_png(path):
    with path.open("rb") as stream:
        header = stream.read(24)
    if len(header) != 24 or not header.startswith(PNG_SIGNATURE):
        raise SystemExit(f"Not a PNG tile: {path}")
    width, height = struct.unpack(">II", header[16:24])
    if (width, height) != (256, 256):
        raise SystemExit(f"Raster tile must be 256x256: {path} is {width}x{height}")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tiles", type=Path, required=True,
                        help="Directory containing z/x/y.png")
    parser.add_argument("--vector-package", type=Path, required=True)
    parser.add_argument("--output-root", type=Path, default=Path("data/raster-packages"))
    parser.add_argument("--date", default=datetime.date.today().strftime("%Y%m%d"))
    parser.add_argument("--revision", default="v2")
    return parser.parse_args()


def main():
    args = parse_args()
    tiles_root = args.tiles.resolve()
    vector_root = args.vector_package.resolve()
    vector_manifest_path = vector_root / "manifest.json"
    if not tiles_root.is_dir() or not vector_manifest_path.is_file():
        raise SystemExit("Rendered tiles or vector manifest is missing")
    vector_manifest = json.loads(vector_manifest_path.read_text(encoding="utf-8"))
    expected = list(expected_tiles())
    expected_paths = {Path(str(z)) / str(x) / f"{y}.png" for z, x, y in expected}
    actual_paths = {path.relative_to(tiles_root) for path in tiles_root.rglob("*.png")}
    missing = sorted(expected_paths - actual_paths)
    extra = sorted(actual_paths - expected_paths)
    if missing or extra:
        raise SystemExit(
            f"Raster tile coverage mismatch: missing={len(missing)} extra={len(extra)}"
        )
    for relative in sorted(expected_paths):
        validate_png(tiles_root / relative)

    package_id = f"hanoi-raster-{args.date}-{args.revision}"
    output_root = args.output_root.resolve()
    package = output_root / package_id
    staging = output_root / f".{package_id}.staging"
    if package.exists() or staging.exists():
        raise SystemExit(f"Refusing to overwrite existing package/staging path: {package}")
    staging.mkdir(parents=True)
    database_path = staging / "raster.mbtiles"
    try:
        database = sqlite3.connect(database_path)
        database.executescript(
            "PRAGMA journal_mode=OFF; PRAGMA synchronous=OFF; PRAGMA page_size=4096;"
            "CREATE TABLE metadata (name TEXT PRIMARY KEY, value TEXT NOT NULL);"
            "CREATE TABLE tiles (zoom_level INTEGER NOT NULL, tile_column INTEGER NOT NULL,"
            " tile_row INTEGER NOT NULL, tile_data BLOB NOT NULL);"
        )
        metadata = {
            "name": package_id,
            "format": "png",
            "bounds": ",".join(str(value) for value in BOUNDS),
            "minzoom": str(MIN_ZOOM),
            "maxzoom": str(MAX_ZOOM),
            "attribution": ATTRIBUTION,
            "type": "baselayer",
            "version": "1",
        }
        database.executemany("INSERT INTO metadata VALUES (?, ?)", sorted(metadata.items()))
        for zoom, x, y in expected:
            tms_y = (1 << zoom) - 1 - y
            database.execute(
                "INSERT INTO tiles VALUES (?, ?, ?, ?)",
                (zoom, x, tms_y, (tiles_root / str(zoom) / str(x) / f"{y}.png").read_bytes()),
            )
        database.execute(
            "CREATE UNIQUE INDEX tile_index ON tiles (zoom_level, tile_column, tile_row)"
        )
        database.commit()
        database.execute("VACUUM")
        database.close()

        source_pbf = vector_manifest.get("source_pbf", {})
        resources = vector_manifest.get("resources", {})
        layer_schema = vector_manifest.get("layer_schema", {})
        manifest = {
            "schema_version": "qtmaps-raster-package-v1",
            "package_id": package_id,
            "source_vector_package": {
                "package_schema_version": vector_manifest.get("schema_version"),
                "data_version": vector_manifest.get("data_version"),
                "manifest_sha256": sha256(vector_manifest_path),
                "style_contract_version": vector_manifest.get("style_contract_version"),
                "style_revision": resources.get("style_revision"),
                "layer_schema_version": layer_schema.get("version"),
            },
            "source_pbf": {
                "name": source_pbf.get("name"),
                "sha256": source_pbf.get("sha256"),
            },
            "bounds": BOUNDS,
            "min_zoom": MIN_ZOOM,
            "max_zoom": MAX_ZOOM,
            "renderer": {"name": "MapLibre Native Qt", "version": "3.0.0"},
            "tiles": {
                "path": database_path.name,
                "format": "png",
                "tile_size": 256,
                "count": len(expected),
                "size_bytes": database_path.stat().st_size,
                "sha256": sha256(database_path),
            },
            "attribution": ATTRIBUTION,
            "generation": {
                "network_access": False,
                "generated_on": args.date,
                "pipeline_revision": "unit07d-v2",
            },
        }
        (staging / "manifest.json").write_text(
            json.dumps(manifest, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        os.replace(staging, package)
        current_tmp = output_root / ".current.tmp"
        current = output_root / "current"
        current_tmp.unlink(missing_ok=True)
        current_tmp.symlink_to(package.name, target_is_directory=True)
        os.replace(current_tmp, current)
        print(f"package={package}")
        print(f"tiles={len(expected)}")
        print(f"size_bytes={manifest['tiles']['size_bytes']}")
        print(f"sha256={manifest['tiles']['sha256']}")
    except Exception:
        shutil.rmtree(staging, ignore_errors=True)
        raise


if __name__ == "__main__":
    main()
