#!/usr/bin/env python3
"""Create tiny synthetic package contracts and a routable graph for 14C.

The manifests follow the production validators, but pixels/vector bytes and
places are generated fixtures. Never describe this output as real Hanoi data.
"""

import argparse
import base64
import hashlib
import json
from pathlib import Path
import sqlite3
import subprocess
import sys


BOUNDS = [105.78, 20.97, 105.9, 21.1]
ATTRIBUTION = "© OpenStreetMap contributors, ODbL 1.0"
PLANETILER_SHA = "f310bd0413e2e4512b27f4046d418664e8e1d3bf31603c2a70e23de06c167e4d"
LAYERS = ["building", "landuse", "place", "transportation", "water"]
PNG = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+/lZkAAAAASUVORK5CYII="
)
OSM = """<?xml version="1.0" encoding="UTF-8"?>
<osm version="0.6" generator="QtMaps-CI-synthetic">
  <bounds minlat="21.020" minlon="105.840" maxlat="21.030" maxlon="105.860"/>
  <node id="1" lat="21.0250" lon="105.8450" version="1"/>
  <node id="2" lat="21.0250" lon="105.8500" version="1"/>
  <node id="3" lat="21.0250" lon="105.8550" version="1"/>
  <node id="4" lat="21.0270" lon="105.8500" version="1"/>
  <way id="10" version="1"><nd ref="1"/><nd ref="2"/><nd ref="3"/>
    <tag k="highway" v="residential"/><tag k="name" v="Synthetic East Road"/></way>
  <way id="11" version="1"><nd ref="2"/><nd ref="4"/>
    <tag k="highway" v="residential"/><tag k="name" v="Synthetic North Road"/></way>
</osm>
"""


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n")


def make_mbtiles(path, count, tile):
    path.parent.mkdir(parents=True, exist_ok=True)
    with sqlite3.connect(path) as db:
        db.execute(
            "CREATE TABLE tiles (zoom_level INTEGER, tile_column INTEGER, "
            "tile_row INTEGER, tile_data BLOB)"
        )
        db.executemany(
            "INSERT INTO tiles VALUES (?, ?, ?, ?)",
            [(12 if i == 0 else 16, 3251 if i == 0 else i, 2292 if i == 0 else 0, tile)
             for i in range(count)],
        )


def vector_package(root, pbf_sha):
    root.mkdir(parents=True)
    make_mbtiles(root / "tiles.mbtiles", 1, b"synthetic-vector-tile")
    style = {
        "version": 8,
        "sprite": "__QTMAPS_BASE_URL__/sprites/sprite",
        "glyphs": "__QTMAPS_BASE_URL__/glyphs/{fontstack}/{range}.pbf",
        "sources": {"qtmaps": {
            "type": "vector",
            "tiles": ["__QTMAPS_BASE_URL__/tiles/{z}/{x}/{y}.pbf"]}},
        "layers": [{"id": layer, "source": "qtmaps", "source-layer": layer,
                    "type": "line" if layer == "transportation" else "fill"}
                   for layer in LAYERS],
    }
    write_json(root / "style/style.json", style)
    for number in ("0-255", "256-511", "768-1023", "7680-7935", "8192-8447"):
        path = root / "glyphs/Noto Sans Regular" / (number + ".pbf")
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"synthetic-glyph")
    for name, value in {
        "sprite.json": b"{}\n", "sprite@2x.json": b"{}\n",
        "sprite.png": PNG, "sprite@2x.png": PNG,
    }.items():
        path = root / "sprites" / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(value)
    files = {
        str(path.relative_to(root)): {"sha256": sha(path), "size_bytes": path.stat().st_size}
        for path in root.rglob("*") if path.is_file()
    }
    manifest = {
        "schema_version": "qtmaps-map-package-v3",
        "data_version": "ci-synthetic-v1",
        "style_contract_version": "qtmaps-style-v3",
        "layer_schema": {"version": 2, "source_layers": LAYERS},
        "bounds": BOUNDS, "min_zoom": 12, "max_zoom": 15,
        "planetiler": {"version": "0.10.2", "sha256": PLANETILER_SHA},
        "source_pbf": {"name": "synthetic-ci.osm.pbf", "sha256": pbf_sha},
        "pipeline_config": {"revision": "ci-synthetic-v1", "sha256": pbf_sha},
        "resources": {"style_revision": "qtmaps-hanoi-v3",
                      "glyph_revision": "noto-sans-regular-v2",
                      "sprite_revision": "ci-synthetic-v1"},
        "attribution": ATTRIBUTION, "files": files,
    }
    write_json(root / "manifest.json", manifest)
    return manifest


def raster_package(root, vector_root, vector, pbf_sha):
    root.mkdir(parents=True)
    db = root / "raster.mbtiles"
    make_mbtiles(db, 847, PNG)
    manifest = {
        "schema_version": "qtmaps-raster-package-v1",
        "package_id": "ci-synthetic-raster-v1",
        "source_vector_package": {
            "package_schema_version": vector["schema_version"],
            "data_version": vector["data_version"],
            "manifest_sha256": sha(vector_root / "manifest.json"),
            "style_contract_version": vector["style_contract_version"],
            "style_revision": vector["resources"]["style_revision"],
            "layer_schema_version": vector["layer_schema"]["version"],
        },
        "source_pbf": {"name": "synthetic-ci.osm.pbf", "sha256": pbf_sha},
        "bounds": BOUNDS, "min_zoom": 12, "max_zoom": 16,
        "renderer": {"name": "MapLibre Native Qt", "version": "3.0.0"},
        "tiles": {"path": "raster.mbtiles", "format": "png", "tile_size": 256,
                  "count": 847, "size_bytes": db.stat().st_size, "sha256": sha(db)},
        "attribution": ATTRIBUTION,
    }
    write_json(root / "manifest.json", manifest)


def search_package(root, pbf_sha):
    root.mkdir(parents=True)
    db_path = root / "places.sqlite"
    with sqlite3.connect(db_path) as db:
        db.execute(
            "CREATE TABLE places (id INTEGER PRIMARY KEY, source_id TEXT UNIQUE NOT NULL, "
            "name TEXT NOT NULL, aliases TEXT NOT NULL, category TEXT NOT NULL, "
            "latitude REAL NOT NULL, longitude REAL NOT NULL)"
        )
        db.execute(
            "CREATE VIRTUAL TABLE places_fts USING fts5(name, aliases, content='places', "
            "content_rowid='id', tokenize='unicode61 remove_diacritics 2')"
        )
        db.execute(
            "INSERT INTO places (source_id,name,aliases,category,latitude,longitude) "
            "VALUES ('synthetic-1','Synthetic Hanoi Point','','place',21.025,105.850)"
        )
        db.execute("INSERT INTO places_fts(places_fts) VALUES ('rebuild')")
    write_json(root / "manifest.json", {
        "schema_version": "qtmaps-search-package-v1",
        "package_id": "ci-synthetic-search-v1",
        "attribution": ATTRIBUTION, "bounds": BOUNDS,
        # The validator requires this URL as a contract sentinel. The extra
        # field and artifact manifest identify the actual synthetic source.
        "source": {"url": "https://download.geofabrik.de/asia/vietnam-latest.osm.pbf",
                   "fixture_origin": "synthetic-ci.osm.pbf; no Geofabrik data",
                   "pbf_sha256": pbf_sha, "retrieved_on": "2026-11-24"},
        "places": {"path": "places.sqlite", "count": 1,
                   "size_bytes": db_path.stat().st_size, "sha256": sha(db_path)},
    })


def graph_package(root, valhalla_site):
    root.mkdir(parents=True)
    osm = root / "synthetic-ci.osm"
    pbf = root / "synthetic-ci.osm.pbf"
    osm.write_text(OSM)
    subprocess.run(["osmium", "cat", str(osm), "-o", str(pbf), "-O"], check=True)
    tiles = root / "tiles"
    tiles.mkdir()
    config = root / "valhalla.json"
    subprocess.run(
        [sys.executable, "-m", "valhalla.valhalla_build_config",
         "--mjolnir-tile-dir", str(tiles.resolve()), "--mjolnir-concurrency", "1",
         "-o", str(config)], check=True,
    )
    service_bin = valhalla_site / "valhalla/bin"
    subprocess.run(
        [str(service_bin / "valhalla_build_tiles"), "-c", str(config), str(pbf)],
        check=True, stdout=subprocess.DEVNULL,
    )
    if not list(tiles.rglob("*.gph")):
        raise SystemExit("synthetic Valhalla graph has no tiles")
    request = {"locations": [{"lat": 21.025, "lon": 105.845},
                             {"lat": 21.025, "lon": 105.855}], "costing": "auto"}
    response = subprocess.run(
        [str(service_bin / "valhalla_service"), str(config), "route",
         json.dumps(request)], check=True, capture_output=True, text=True,
    )
    route = json.loads(response.stdout)["trip"]
    if route["summary"]["length"] <= 0 or not route["legs"][0]["maneuvers"]:
        raise SystemExit("synthetic graph route/maneuver check failed")
    return pbf, config, tiles


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--valhalla-site", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("output directory already exists")
    args.output.mkdir(parents=True)
    pbf, config, tiles = graph_package(args.output / "graph-build", args.valhalla_site)
    pbf_sha = sha(pbf)
    vector_root = args.output / "vector"
    vector = vector_package(vector_root, pbf_sha)
    raster_package(args.output / "raster", vector_root, vector, pbf_sha)
    search_package(args.output / "search", pbf_sha)
    write_json(args.output / "fixture.json", {
        "kind": "synthetic-ci-fixture", "map_pixels": "synthetic",
        "vector_tile_bytes": "synthetic", "search_places": 1,
        "graph": "synthetic OSM-format road, routable",
        "orthography": "omitted; optional-unavailable path",
        "pbf_sha256": pbf_sha,
        "config": str(config.resolve()), "tiles": str(tiles.resolve()),
        "vector": str(vector_root.resolve()),
        "raster": str((args.output / "raster").resolve()),
        "search": str((args.output / "search").resolve()),
    })
    print("Synthetic CI fixture prepared:", args.output)
    print("Synthetic graph route passed; PBF SHA-256:", pbf_sha)


if __name__ == "__main__":
    main()
