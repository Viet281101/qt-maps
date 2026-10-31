#!/usr/bin/env python3

import json
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCHEMA = ROOT / "tools/data-pipeline/unit07d-hanoi-v2.yml"
STYLE = ROOT / "resources/map/hanoi-style.json"


def feature_block(schema, layer_id):
    pattern = rf"(?ms)^  - id: {re.escape(layer_id)}\n(.*?)(?=^  - id: |\Z)"
    match = re.search(pattern, schema)
    if match is None:
        raise AssertionError(f"missing schema layer: {layer_id}")
    return match.group(1)


class HanoiV2SchemaTest(unittest.TestCase):
    def test_context_layers_are_bounded_whitelists(self):
        schema = SCHEMA.read_text(encoding="utf-8")
        self.assertIn("version: 2", schema)
        self.assertNotIn("url:", schema)

        landuse = feature_block(schema, "landuse")
        self.assertIn("geometry: polygon", landuse)
        self.assertIn("min_zoom: 13", landuse)
        self.assertIn("leisure: [park, garden, nature_reserve]", landuse)
        self.assertIn("landuse: [grass, recreation_ground, forest]", landuse)
        self.assertIn("natural: wood", landuse)
        self.assertNotIn("__any__", landuse)

        water = feature_block(schema, "water")
        self.assertIn("geometry: polygon", water)
        self.assertIn("min_zoom: 12", water)
        self.assertIn("natural: water", water)
        self.assertIn("waterway: riverbank", water)
        self.assertIn("landuse: [reservoir, basin]", water)
        self.assertNotIn("__any__", water)

    def test_product_style_consumes_each_draft_v2_context_layer(self):
        style = json.loads(STYLE.read_text(encoding="utf-8"))
        layers = {layer["id"]: layer for layer in style["layers"]}
        self.assertEqual(layers["green-areas"]["source-layer"], "landuse")
        self.assertEqual(layers["water"]["source-layer"], "water")
        self.assertEqual(layers["green-areas"]["minzoom"], 13)
        self.assertEqual(layers["water"]["minzoom"], 12)
        self.assertNotEqual(
            layers["green-areas"]["paint"]["fill-color"],
            style["metadata"]["qtmaps:theme"]["land"],
        )
        self.assertNotEqual(
            layers["water"]["paint"]["fill-color"],
            style["metadata"]["qtmaps:theme"]["land"],
        )

    def test_no_unapproved_context_or_poi_layer_is_present(self):
        schema = SCHEMA.read_text(encoding="utf-8")
        layer_ids = re.findall(r"(?m)^  - id: ([a-z_]+)$", schema)
        self.assertEqual(
            layer_ids,
            ["building", "landuse", "place", "transportation", "water"],
        )
        self.assertNotIn("poi", layer_ids)
        self.assertNotIn("amenity:", schema)


if __name__ == "__main__":
    unittest.main()
