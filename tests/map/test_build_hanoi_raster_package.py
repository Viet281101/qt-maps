#!/usr/bin/env python3

import hashlib
import json
import math
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest
import zlib


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools/data-pipeline/build_hanoi_raster_package.py"
BOUNDS = [105.78, 20.97, 105.90, 21.10]


def png_tile():
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    raw = b"".join(b"\x00" + b"\xf2\xef\xe9" * 256 for _ in range(256))
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", 256, 256, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw, 9))
        + chunk(b"IEND", b"")
    )


def longitude_to_x(longitude, zoom):
    return math.floor((longitude + 180.0) / 360.0 * (1 << zoom))


def latitude_to_y(latitude, zoom):
    radians = math.radians(latitude)
    return math.floor((1.0 - math.asinh(math.tan(radians)) / math.pi) / 2.0 * (1 << zoom))


class RasterPackageToolTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.tiles = self.root / "tiles"
        payload = png_tile()
        west, south, east, north = BOUNDS
        for zoom in range(12, 17):
            for x in range(longitude_to_x(west, zoom), longitude_to_x(east, zoom) + 1):
                for y in range(latitude_to_y(north, zoom), latitude_to_y(south, zoom) + 1):
                    path = self.tiles / str(zoom) / str(x) / f"{y}.png"
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(payload)
        self.vector = self.root / "vector"
        self.vector.mkdir()
        (self.vector / "manifest.json").write_text(
            json.dumps(
                {
                    "schema_version": "qtmaps-map-package-v1",
                    "data_version": "hanoi-test-v1",
                    "style_contract_version": "qtmaps-style-v1",
                    "layer_schema": {"version": 1},
                    "resources": {"style_revision": "qtmaps-hanoi-v1"},
                    "source_pbf": {"name": "vietnam-latest.osm.pbf", "sha256": "a" * 64},
                },
                sort_keys=True,
            ),
            encoding="utf-8",
        )

    def tearDown(self):
        self.temporary.cleanup()

    def build(self, output):
        subprocess.run(
            [
                "python3", str(TOOL), "--tiles", str(self.tiles),
                "--vector-package", str(self.vector), "--output-root", str(output),
                "--date", "20261030", "--revision", "test",
            ],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
        return output / "hanoi-raster-20261030-test"

    def test_build_is_complete_and_byte_stable(self):
        first = self.build(self.root / "first")
        second = self.build(self.root / "second")
        first_manifest = json.loads((first / "manifest.json").read_text(encoding="utf-8"))
        self.assertEqual(first_manifest["tiles"]["count"], 847)
        self.assertEqual(first_manifest["min_zoom"], 12)
        self.assertEqual(first_manifest["max_zoom"], 16)
        self.assertEqual((first / "manifest.json").read_bytes(),
                         (second / "manifest.json").read_bytes())
        self.assertEqual((first / "raster.mbtiles").read_bytes(),
                         (second / "raster.mbtiles").read_bytes())

    def test_missing_tile_is_rejected(self):
        missing = next(self.tiles.rglob("*.png"))
        missing.unlink()
        completed = subprocess.run(
            [
                "python3", str(TOOL), "--tiles", str(self.tiles),
                "--vector-package", str(self.vector), "--output-root", str(self.root / "bad"),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("coverage mismatch", completed.stderr)


if __name__ == "__main__":
    unittest.main()
