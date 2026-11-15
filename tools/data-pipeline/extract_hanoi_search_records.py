#!/usr/bin/env python3
"""Extract approved Hanoi named-point records from a local OSM PBF."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

BOUNDS = "105.78,20.97,105.90,21.10"
WEST, SOUTH, EAST, NORTH = (float(value) for value in BOUNDS.split(","))
CATEGORIES = ("place", "amenity", "tourism", "shop", "public_transport")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pbf", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--osmium", default="osmium")
    args = parser.parse_args()
    pbf = args.pbf.resolve()
    if not pbf.is_file():
        raise SystemExit(f"PBF is missing: {pbf}")
    if shutil.which(args.osmium) is None:
        raise SystemExit(f"osmium is required: {args.osmium}")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="qtmaps-search-") as temporary:
        bounded = Path(temporary) / "hanoi.osm.pbf"
        filtered = Path(temporary) / "named.osm.pbf"
        stream = Path(temporary) / "points.geojsonseq"
        subprocess.run([args.osmium, "extract", "-b", BOUNDS, str(pbf), "-o", str(bounded), "-O"], check=True)
        filters = [f"n/{category}" for category in CATEGORIES]
        subprocess.run([args.osmium, "tags-filter", str(bounded), *filters, "-o", str(filtered), "-O"], check=True)
        subprocess.run([args.osmium, "export", str(filtered), "--geometry-types", "point", "-u", "type_id", "-f", "geojsonseq", "-o", str(stream), "-O"], check=True)
        records = []
        for line in stream.read_text(encoding="utf-8").splitlines():
            encoded = line.lstrip("\x1e").strip()
            if not encoded:
                continue
            feature = json.loads(encoded)
            properties = feature.get("properties", {})
            name = properties.get("name")
            coordinates = feature.get("geometry", {}).get("coordinates", [])
            source_id = feature.get("id")
            if not source_id or not name or len(coordinates) != 2:
                continue
            longitude, latitude = coordinates
            if not SOUTH <= latitude <= NORTH or not WEST <= longitude <= EAST:
                continue
            aliases = [value for key in ("name:vi", "alt_name") if (value := properties.get(key)) and value != name]
            category = next((key for key in CATEGORIES if key in properties), "place")
            records.append({"source_id": source_id, "name": name, "aliases": aliases, "category": category, "latitude": latitude, "longitude": longitude})
        records.sort(key=lambda record: record["source_id"])
        args.output.write_text("".join(json.dumps(record, ensure_ascii=False, sort_keys=True) + "\n" for record in records), encoding="utf-8")
        print(f"records={len(records)} output={args.output}")


if __name__ == "__main__":
    main()
