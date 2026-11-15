#!/usr/bin/env python3
"""Package validated local z12-z15 Hanoi RGB tiles without network access.

This packager does not query Copernicus or transform a Sentinel scene.  It
only turns a complete, already-prepared RGB tile set into an immutable MBTiles
package and records the source provenance needed by the runtime gate.
"""

import argparse
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import sqlite3
import struct


BOUNDS = [105.78, 20.97, 105.90, 21.10]
MIN_ZOOM = 12
MAX_ZOOM = 15
MAX_PACKAGE_BYTES = 100 * 1024 * 1024
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
RGB_BANDS = ["B04", "B03", "B02"]


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
        raise SystemExit(f"Orthography tile must be 256x256: {path} is {width}x{height}")


def iso_date(value):
    try:
        return datetime.date.fromisoformat(value).isoformat()
    except ValueError as error:
        raise argparse.ArgumentTypeError(f"Expected YYYY-MM-DD, got {value!r}") from error


def sha256_value(value):
    if len(value) != 64 or any(character not in "0123456789abcdef" for character in value):
        raise argparse.ArgumentTypeError("Expected a lowercase SHA-256 hexadecimal value")
    return value


def percentage(value):
    try:
        parsed = float(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("Expected a percentage from 0 through 100") from error
    if not 0.0 <= parsed <= 100.0:
        raise argparse.ArgumentTypeError("Expected a percentage from 0 through 100")
    return parsed


def compact_date(value):
    try:
        return datetime.datetime.strptime(value, "%Y%m%d").strftime("%Y%m%d")
    except ValueError as error:
        raise argparse.ArgumentTypeError(f"Expected YYYYMMDD, got {value!r}") from error


def revision(value):
    if not re.fullmatch(r"v[1-9][0-9]*", value):
        raise argparse.ArgumentTypeError("Expected a revision such as v1")
    return value


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tiles", type=Path, required=True,
                        help="Directory containing z/x/y.png")
    parser.add_argument("--output-root", type=Path,
                        default=Path("data/orthography-packages"))
    parser.add_argument("--date", type=compact_date,
                        default=datetime.date.today().strftime("%Y%m%d"))
    parser.add_argument("--revision", type=revision, default="v1")
    parser.add_argument("--source-product-id", action="append", required=True)
    parser.add_argument("--source-sha256", action="append", type=sha256_value, required=True)
    parser.add_argument("--acquisition-date", type=iso_date, required=True)
    parser.add_argument("--processing-baseline", required=True)
    parser.add_argument("--cloud-cover-max-percent", type=percentage, required=True)
    parser.add_argument("--resampling", choices=("nearest", "bilinear", "cubic", "lanczos"),
                        required=True)
    return parser.parse_args()


def source_scenes(args):
    if len(args.source_product_id) != len(args.source_sha256):
        raise SystemExit("Each source product ID requires one matching source SHA-256")
    scenes = sorted(zip(args.source_product_id, args.source_sha256))
    if len({product_id for product_id, _ in scenes}) != len(scenes):
        raise SystemExit("Source product IDs must be unique")
    return [{"product_id": product_id, "sha256": digest} for product_id, digest in scenes]


def main():
    args = parse_args()
    tiles_root = args.tiles.resolve()
    if not tiles_root.is_dir():
        raise SystemExit("Prepared RGB tiles are missing")
    expected = list(expected_tiles())
    expected_paths = {Path(str(z)) / str(x) / f"{y}.png" for z, x, y in expected}
    actual_paths = {
        path.relative_to(tiles_root)
        for path in tiles_root.rglob("*")
        if path.is_file()
    }
    missing = sorted(expected_paths - actual_paths)
    extra = sorted(actual_paths - expected_paths)
    if missing or extra:
        raise SystemExit(
            f"Orthography tile coverage mismatch: missing={len(missing)} extra={len(extra)}"
        )
    for relative in sorted(expected_paths):
        validate_png(tiles_root / relative)

    package_id = f"hanoi-orthography-{args.date}-s2-l2a-{args.revision}"
    output_root = args.output_root.resolve()
    package = output_root / package_id
    staging = output_root / f".{package_id}.staging"
    if package.exists() or staging.exists():
        raise SystemExit(f"Refusing to overwrite existing package/staging path: {package}")
    scenes = source_scenes(args)
    staging.mkdir(parents=True)
    database_path = staging / "orthography.mbtiles"
    try:
        database = sqlite3.connect(database_path)
        database.executescript(
            "PRAGMA journal_mode=OFF; PRAGMA synchronous=OFF; PRAGMA page_size=4096;"
            "CREATE TABLE metadata (name TEXT PRIMARY KEY, value TEXT NOT NULL);"
            "CREATE TABLE tiles (zoom_level INTEGER NOT NULL, tile_column INTEGER NOT NULL,"
            " tile_row INTEGER NOT NULL, tile_data BLOB NOT NULL);"
        )
        attribution = f"Contains modified Copernicus Sentinel data {args.acquisition_date[:4]}"
        metadata = {
            "name": package_id,
            "format": "png",
            "bounds": ",".join(str(value) for value in BOUNDS),
            "minzoom": str(MIN_ZOOM),
            "maxzoom": str(MAX_ZOOM),
            "attribution": attribution,
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

        size_bytes = database_path.stat().st_size
        if size_bytes > MAX_PACKAGE_BYTES:
            raise SystemExit(
                f"Orthography package exceeds 100 MiB budget: {size_bytes} bytes"
            )
        manifest = {
            "schema_version": "qtmaps-orthography-package-v1",
            "package_id": package_id,
            "bounds": BOUNDS,
            "min_zoom": MIN_ZOOM,
            "max_zoom": MAX_ZOOM,
            "source": {
                "mission": "Copernicus Sentinel-2",
                "product_level": "L2A",
                "rgb_bands": RGB_BANDS,
                "acquisition_date": args.acquisition_date,
                "processing_baseline": args.processing_baseline,
                "cloud_cover_max_percent": args.cloud_cover_max_percent,
                "resampling": args.resampling,
                "scenes": scenes,
            },
            "tiles": {
                "path": database_path.name,
                "format": "png",
                "tile_size": 256,
                "count": len(expected),
                "size_bytes": size_bytes,
                "sha256": sha256(database_path),
            },
            "attribution": attribution,
            "generation": {
                "network_access": False,
                "generated_on": args.date,
                "pipeline_revision": "unit15-v1-package-contract",
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
        print(f"size_bytes={size_bytes}")
        print(f"sha256={manifest['tiles']['sha256']}")
    except BaseException:
        shutil.rmtree(staging, ignore_errors=True)
        raise


if __name__ == "__main__":
    main()
