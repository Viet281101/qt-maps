#!/usr/bin/env python3

import json
import math
from pathlib import Path
import sqlite3
import struct
import subprocess
import tempfile
import unittest
import zlib


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools/data-pipeline/build_hanoi_orthography_package.py"
BOUNDS = [105.78, 20.97, 105.90, 21.10]


def png_tile():
    def chunk(kind, data):
        return (
            struct.pack(">I", len(data))
            + kind
            + data
            + struct.pack(">I", zlib.crc32(kind + data))
        )

    raw = b"".join(b"\x00" + b"\x4a\x8c\x52" * 256 for _ in range(256))
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
    return math.floor(
        (1.0 - math.asinh(math.tan(radians)) / math.pi) / 2.0 * (1 << zoom)
    )


class OrthographyPackageToolTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.tiles = self.root / "tiles"
        payload = png_tile()
        west, south, east, north = BOUNDS
        for zoom in range(12, 16):
            for x in range(longitude_to_x(west, zoom), longitude_to_x(east, zoom) + 1):
                for y in range(latitude_to_y(north, zoom), latitude_to_y(south, zoom) + 1):
                    path = self.tiles / str(zoom) / str(x) / f"{y}.png"
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(payload)

    def tearDown(self):
        self.temporary.cleanup()

    def command(self, output):
        return [
            "python3", str(TOOL), "--tiles", str(self.tiles), "--output-root", str(output),
            "--date", "20261115", "--source-product-id", "S2A_TEST_A",
            "--source-sha256", "a" * 64, "--acquisition-date", "2026-11-14",
            "--processing-baseline", "05.11", "--cloud-cover-max-percent", "10",
            "--resampling", "cubic",
        ]

    def build(self, output):
        subprocess.run(self.command(output), cwd=ROOT, check=True, capture_output=True, text=True)
        return output / "hanoi-orthography-20261115-s2-l2a-v1"

    def test_build_is_complete_byte_stable_and_provenanced(self):
        first = self.build(self.root / "first")
        second = self.build(self.root / "second")
        manifest = json.loads((first / "manifest.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["tiles"]["count"], 226)
        self.assertEqual(
            manifest["tiles"]["size_bytes"],
            (first / "orthography.mbtiles").stat().st_size,
        )
        self.assertEqual(
            manifest["attribution"], "Contains modified Copernicus Sentinel data 2026"
        )
        self.assertEqual(manifest["source"]["rgb_bands"], ["B04", "B03", "B02"])
        self.assertEqual(
            manifest["source"]["scenes"],
            [{"product_id": "S2A_TEST_A", "sha256": "a" * 64}],
        )
        self.assertFalse(manifest["generation"]["network_access"])
        self.assertEqual((first / "manifest.json").read_bytes(), (second / "manifest.json").read_bytes())
        self.assertEqual((first / "orthography.mbtiles").read_bytes(),
                         (second / "orthography.mbtiles").read_bytes())
        with sqlite3.connect(first / "orthography.mbtiles") as database:
            metadata = dict(database.execute("SELECT name, value FROM metadata"))
        self.assertEqual(metadata["minzoom"], "12")
        self.assertEqual(metadata["maxzoom"], "15")

    def test_missing_tile_is_rejected(self):
        next(self.tiles.rglob("*.png")).unlink()
        completed = subprocess.run(
            self.command(self.root / "bad"), cwd=ROOT, capture_output=True, text=True
        )
        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("coverage mismatch", completed.stderr)

    def test_non_png_tile_is_rejected(self):
        extra = self.tiles / "12" / "3251" / "1803.jpg"
        extra.write_bytes(b"not-a-tile")
        completed = subprocess.run(
            self.command(self.root / "extra"), cwd=ROOT, capture_output=True, text=True
        )
        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("coverage mismatch", completed.stderr)

    def test_package_name_components_are_restricted(self):
        command = self.command(self.root / "bad-name")
        command.extend(["--revision", "../escape"])
        completed = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("Expected a revision such as v1", completed.stderr)

    def test_provenance_pairing_is_required(self):
        command = self.command(self.root / "bad-provenance")
        command.extend(["--source-product-id", "S2A_TEST_B"])
        completed = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("matching source SHA-256", completed.stderr)


if __name__ == "__main__":
    unittest.main()
