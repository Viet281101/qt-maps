#!/usr/bin/env python3
"""Validate and canonicalize the source-controlled Hanoi product style."""

import argparse
import json
from pathlib import Path


BASE_URL_TOKEN = "__QTMAPS_BASE_URL__"
ATTRIBUTION = "© OpenStreetMap contributors, ODbL 1.0"


def render_style(template_path):
    style = json.loads(template_path.read_text(encoding="utf-8"))
    if style.get("version") != 8:
        raise ValueError("Hanoi product style must use Mapbox style version 8")
    source = style.get("sources", {}).get("qtmaps", {})
    expected_tiles = [f"{BASE_URL_TOKEN}/tiles/{{z}}/{{x}}/{{y}}.pbf"]
    if source.get("tiles") != expected_tiles:
        raise ValueError("Hanoi style must use the local qtmaps tile URL token")
    if source.get("attribution") != ATTRIBUTION:
        raise ValueError("Hanoi style attribution is missing or incompatible")
    if style.get("sprite") != f"{BASE_URL_TOKEN}/sprites/sprite":
        raise ValueError("Hanoi style sprite URL must use the local URL token")
    if style.get("glyphs") != f"{BASE_URL_TOKEN}/glyphs/{{fontstack}}/{{range}}.pbf":
        raise ValueError("Hanoi style glyph URL must use the local URL token")
    serialized = json.dumps(style, ensure_ascii=False, indent=2) + "\n"
    if "http://" in serialized or "https://" in serialized:
        raise ValueError("Hanoi style must not contain a public asset URL")
    return serialized


def main():
    repository = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--template",
        type=Path,
        default=repository / "resources/map/hanoi-style.json",
    )
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    destination = args.output / "style.json"
    destination.write_text(render_style(args.template.resolve()), encoding="utf-8")
    print(f"style={destination}")


if __name__ == "__main__":
    main()
