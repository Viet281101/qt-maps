#!/usr/bin/env python3

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools/data-pipeline/build_hanoi_style.py"
TEMPLATE = ROOT / "resources/map/hanoi-style.json"


def load_tool():
    spec = importlib.util.spec_from_file_location("build_hanoi_style", TOOL)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class HanoiStyleTest(unittest.TestCase):
    def test_product_style_is_deterministic_local_and_class_aware(self):
        tool = load_tool()
        first = tool.render_style(TEMPLATE)
        second = tool.render_style(TEMPLATE)
        self.assertEqual(first, second)
        self.assertNotIn("http://", first)
        self.assertNotIn("https://", first)

        style = json.loads(first)
        self.assertEqual(style["metadata"]["qtmaps:style_revision"], "qtmaps-hanoi-v2")
        self.assertEqual(
            style["sources"]["qtmaps"]["tiles"],
            ["__QTMAPS_BASE_URL__/tiles/{z}/{x}/{y}.pbf"],
        )
        layers = {layer["id"]: layer for layer in style["layers"]}
        self.assertIn("road-major-fill", layers)
        self.assertEqual(layers["water"]["source-layer"], "water")
        self.assertEqual(layers["green-areas"]["source-layer"], "landuse")
        self.assertIn("road-secondary-fill", layers)
        self.assertIn("road-local-fill", layers)
        self.assertEqual(layers["road-labels-prepared"]["layout"]["visibility"], "none")
        self.assertEqual(layers["hanoi-label"]["layout"]["visibility"], "none")
        self.assertLess(
            layers["road-local-fill"]["paint"]["line-width"]["stops"][-1][1],
            layers["road-major-fill"]["paint"]["line-width"]["stops"][-1][1],
        )

    def test_cli_output_is_byte_stable(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            first = root / "first"
            second = root / "second"
            subprocess.run(
                [sys.executable, str(TOOL), "--output", str(first)],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )
            subprocess.run(
                [sys.executable, str(TOOL), "--output", str(second)],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )
            self.assertEqual(
                (first / "style.json").read_bytes(),
                (second / "style.json").read_bytes(),
            )

    def test_synthetic_fixture_does_not_generate_the_product_style(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "resources"
            subprocess.run(
                [
                    sys.executable,
                    str(ROOT / "tools/generate_vector_fixture.py"),
                    "--resources-only",
                    "--output",
                    str(output),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )
            self.assertFalse((output / "style.json").exists())
            self.assertFalse((output / "tiles").exists())
            self.assertTrue((output / "fonts/QtMapsSans/0-255.pbf").is_file())
            self.assertTrue((output / "sprite.png").is_file())


if __name__ == "__main__":
    unittest.main()
