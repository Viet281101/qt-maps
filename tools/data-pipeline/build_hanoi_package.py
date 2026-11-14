#!/usr/bin/env python3
"""Build an immutable, checksummed Hanoi MBTiles package without network access."""

import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import resource
import shutil
import subprocess
import sys
import time

from build_hanoi_style import render_style


BOUNDS = [105.78, 20.97, 105.90, 21.10]
MIN_ZOOM = 12
MAX_ZOOM = 15
SCHEMA_VERSION = "qtmaps-map-package-v3"
STYLE_CONTRACT_VERSION = "qtmaps-style-v3"
LAYER_SCHEMA_VERSION = 2
ATTRIBUTION = "© OpenStreetMap contributors, ODbL 1.0"
PLANETILER_VERSION = "0.10.2"
PLANETILER_SHA256 = "f310bd0413e2e4512b27f4046d418664e8e1d3bf31603c2a70e23de06c167e4d"
SOURCE_LAYERS = ["building", "landuse", "place", "transportation", "water"]
FONT_PATH = Path("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf")
FONT_STACK = "Noto Sans Regular"


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pbf", type=Path, required=True)
    parser.add_argument("--planetiler-jar", type=Path, required=True)
    parser.add_argument("--output-root", type=Path, default=Path("data/packages"))
    parser.add_argument("--date", default=datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%d"))
    parser.add_argument("--revision", default="v3")
    parser.add_argument("--heap", default="6g")
    parser.add_argument("--threads", type=int, default=2)
    return parser.parse_args()


def main():
    args = parse_args()
    repository = Path(__file__).resolve().parents[2]
    pbf = args.pbf.resolve()
    jar = args.planetiler_jar.resolve()
    font_path = FONT_PATH.resolve()
    for path, label in ((pbf, "PBF"), (jar, "Planetiler jar"), (font_path, "Noto Sans font")):
        if not path.is_file():
            raise SystemExit(f"{label} does not exist: {path}")
    jar_sha256 = sha256(jar)
    if jar_sha256 != PLANETILER_SHA256:
        raise SystemExit(
            f"Planetiler checksum mismatch: expected {PLANETILER_SHA256}, got {jar_sha256}"
        )
    if args.threads < 1 or args.threads > 2:
        raise SystemExit("--threads must remain between 1 and 2 on the reference machine")

    package_id = f"hanoi-{args.date}-{args.revision}"
    output_root = args.output_root.resolve()
    package = output_root / package_id
    staging = output_root / f".{package_id}.staging"
    if package.exists() or staging.exists():
        raise SystemExit(f"Refusing to overwrite existing package/staging path: {package}")
    staging.mkdir(parents=True)
    started = time.monotonic()

    try:
        source_schema_path = repository / "tools/data-pipeline/unit07d-hanoi-v2.yml"
        source_schema = source_schema_path.read_text(encoding="utf-8")
        source_schema = source_schema.replace(
            "local_path: generated/unit-01b/source/vietnam-latest.osm.pbf",
            f"local_path: '{pbf}'",
        )
        schema_path = staging / "planetiler-schema.yml"
        schema_path.write_text(source_schema, encoding="utf-8")

        tiles = staging / "tiles.mbtiles"
        command = [
            "java", f"-Xmx{args.heap}", "-jar", str(jar), "generate-custom",
            f"--schema={schema_path}", f"--output={tiles}",
            f"--bounds={','.join(str(value) for value in BOUNDS)}",
            f"--minzoom={MIN_ZOOM}", f"--maxzoom={MAX_ZOOM}",
            f"--render_maxzoom={MAX_ZOOM}", f"--threads={args.threads}",
            "--tile_compression=none", "--download=false", "--refresh_sources=false",
            f"--tmpdir={staging / 'tmp'}", "--force",
        ]
        subprocess.run(command, cwd=repository, check=True)
        generation_peak_rss_kib = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss

        resources = staging / "resources"
        subprocess.run(
            [sys.executable, str(repository / "tools/generate_vector_fixture.py"),
             "--resources-only", "--output", str(resources)],
            cwd=repository, check=True,
        )

        style_dir = staging / "style"
        glyphs_dir = staging / "glyphs"
        sprites_dir = staging / "sprites"
        style_dir.mkdir()
        subprocess.run(
            [
                sys.executable,
                str(repository / "tools/data-pipeline/generate_hanoi_glyphs.py"),
                "--font",
                str(font_path),
                "--output",
                str(glyphs_dir),
            ],
            cwd=repository,
            check=True,
        )
        sprites_dir.mkdir()
        shutil.copy2(resources / "sprite.json", sprites_dir / "sprite.json")
        shutil.copy2(resources / "sprite.png", sprites_dir / "sprite.png")
        # v2 deliberately has an empty sprite, but MapLibre requests DPR-specific
        # endpoints on HiDPI displays. Non-empty future sprites need a true 2x asset.
        shutil.copy2(resources / "sprite.json", sprites_dir / "sprite@2x.json")
        shutil.copy2(resources / "sprite.png", sprites_dir / "sprite@2x.png")
        style_template_path = repository / "resources/map/hanoi-style.json"
        rendered_style = render_style(style_template_path)
        style_revision = json.loads(rendered_style)["metadata"]["qtmaps:style_revision"]
        (style_dir / "style.json").write_text(rendered_style, encoding="utf-8")
        shutil.rmtree(resources)
        shutil.rmtree(staging / "tmp", ignore_errors=True)
        schema_path.unlink()

        files = {}
        for path in sorted(staging.rglob("*")):
            if path.is_file():
                relative = path.relative_to(staging).as_posix()
                files[relative] = {"sha256": sha256(path), "size_bytes": path.stat().st_size}

        manifest = {
            "schema_version": SCHEMA_VERSION,
            "data_version": package_id,
            "style_contract_version": STYLE_CONTRACT_VERSION,
            "layer_schema": {"version": LAYER_SCHEMA_VERSION, "source_layers": SOURCE_LAYERS},
            "bounds": BOUNDS,
            "min_zoom": MIN_ZOOM,
            "max_zoom": MAX_ZOOM,
            "planetiler": {"version": PLANETILER_VERSION, "sha256": jar_sha256},
            "source_pbf": {"name": pbf.name, "sha256": sha256(pbf)},
            "pipeline_config": {
                "revision": "unit07d-v3-typography",
                "sha256": sha256(source_schema_path),
                "style_template_sha256": sha256(style_template_path),
            },
            "resources": {
                "style_revision": style_revision,
                "glyph_revision": "noto-sans-regular-v2",
                "glyph_font": {
                    "family": FONT_STACK,
                    "license": "OFL-1.1",
                    "sha256": sha256(font_path),
                },
                "sprite_revision": "qtmaps-empty-v1",
            },
            "attribution": ATTRIBUTION,
            "generation": {
                "network_access": False,
                "threads": args.threads,
                "heap_limit": args.heap,
                "duration_seconds": round(time.monotonic() - started, 3),
                "peak_rss_kib": generation_peak_rss_kib,
            },
            "files": files,
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
        print(f"manifest={package / 'manifest.json'}")
        print(f"duration_seconds={manifest['generation']['duration_seconds']}")
    except Exception:
        shutil.rmtree(staging, ignore_errors=True)
        raise


if __name__ == "__main__":
    main()
