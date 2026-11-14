#!/usr/bin/env python3
"""Validate the source-controlled bounded Hanoi v2 package inputs."""

import json
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCHEMA = ROOT / "tools/data-pipeline/unit07d-hanoi-v2.yml"
STYLE = ROOT / "resources/map/hanoi-style.json"
BUILDER = ROOT / "tools/data-pipeline/build_hanoi_package.py"


class HanoiV2SchemaTest(unittest.TestCase):
    def test_schema_has_only_bounded_context_layers(self):
        schema = SCHEMA.read_text(encoding="utf-8")
        self.assertIn("version: 2", schema)
        self.assertNotIn("url:", schema)
        self.assertEqual(
            re.findall(r"(?m)^  - id: ([a-z_]+)$", schema),
            ["building", "landuse", "place", "transportation", "water"],
        )
        self.assertNotIn("  - id: poi", schema)
        self.assertNotIn("amenity:", schema)
        for allowed_tag in (
            "leisure: [park, garden, nature_reserve]",
            "landuse: [grass, recreation_ground, forest]",
            "natural: wood",
            "natural: water",
            "waterway: riverbank",
            "landuse: [reservoir, basin]",
        ):
            self.assertIn(allowed_tag, schema)
        self.assertNotIn("__any__", schema.split("  - id: landuse", 1)[1].split("  - id: place", 1)[0])
        self.assertNotIn("__any__", schema.split("  - id: water", 1)[1])

    def test_style_and_builder_share_the_v2_contract(self):
        style = json.loads(STYLE.read_text(encoding="utf-8"))
        layers = {layer["id"]: layer for layer in style["layers"]}
        self.assertEqual(style["metadata"]["qtmaps:style_revision"], "qtmaps-hanoi-v3")
        self.assertEqual(layers["water"]["source-layer"], "water")
        self.assertEqual(layers["green-areas"]["source-layer"], "landuse")
        self.assertEqual(layers["water"]["minzoom"], 12)
        self.assertEqual(layers["green-areas"]["minzoom"], 13)
        builder = BUILDER.read_text(encoding="utf-8")
        self.assertIn('SCHEMA_VERSION = "qtmaps-map-package-v3"', builder)
        self.assertIn('STYLE_CONTRACT_VERSION = "qtmaps-style-v3"', builder)
        self.assertIn("LAYER_SCHEMA_VERSION = 2", builder)
        self.assertIn('sprites_dir / "sprite@2x.json"', builder)
        self.assertIn('sprites_dir / "sprite@2x.png"', builder)
        self.assertIn('generate_hanoi_glyphs.py', builder)
        self.assertIn('FONT_STACK = "Noto Sans Regular"', builder)
        self.assertIn('"glyph_revision": "noto-sans-regular-v2"', builder)
        self.assertIn('SOURCE_LAYERS = ["building", "landuse", "place", "transportation", "water"]', builder)
        self.assertIn('repository / "tools/data-pipeline/unit07d-hanoi-v2.yml"', builder)


if __name__ == "__main__":
    unittest.main()
