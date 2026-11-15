#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import sqlite3
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools/data-pipeline/build_hanoi_search_package.py"


class SearchPackageToolTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.pbf = self.root / "vietnam.osm.pbf"
        self.pbf.write_bytes(b"pbf-fixture")
        self.places = self.root / "places.jsonl"
        self.write_places([
            {"source_id": "node/1", "name": "Hoàn Kiếm", "aliases": ["Hồ Hoàn Kiếm"], "category": "place", "latitude": 21.0285, "longitude": 105.8542},
            {"source_id": "node/2", "name": "Thái Hà", "aliases": [], "category": "street", "latitude": 21.0116, "longitude": 105.8197},
        ])

    def tearDown(self): self.temporary.cleanup()

    def write_places(self, rows):
        self.places.write_text("".join(json.dumps(row, ensure_ascii=False) + "\n" for row in rows), encoding="utf-8")

    def command(self, output):
        return ["python3", str(TOOL), "--places", str(self.places), "--source-pbf", str(self.pbf), "--retrieved-on", "2026-11-14", "--date", "20261115", "--output-root", str(output)]

    def test_package_is_stable_and_searchable(self):
        first_root, second_root = self.root / "first", self.root / "second"
        subprocess.run(self.command(first_root), cwd=ROOT, check=True)
        subprocess.run(self.command(second_root), cwd=ROOT, check=True)
        first = first_root / "hanoi-search-20261115-v1-r1"
        second = second_root / "hanoi-search-20261115-v1-r1"
        self.assertEqual((first / "places.sqlite").read_bytes(), (second / "places.sqlite").read_bytes())
        manifest = json.loads((first / "manifest.json").read_text())
        self.assertEqual(manifest["places"]["count"], 2)
        self.assertEqual(manifest["source"]["pbf_sha256"], hashlib.sha256(self.pbf.read_bytes()).hexdigest())
        with sqlite3.connect(first / "places.sqlite") as database:
            result = database.execute("SELECT places.name FROM places_fts JOIN places ON places.id=places_fts.rowid WHERE places_fts MATCH ?", ("hoan AND kiem",)).fetchone()
        self.assertEqual(result[0], "Hoàn Kiếm")

    def test_out_of_bounds_record_is_rejected(self):
        self.write_places([{"source_id": "node/3", "name": "Elsewhere", "aliases": [], "latitude": 0, "longitude": 0}])
        result = subprocess.run(self.command(self.root / "bad"), cwd=ROOT, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("outside Hanoi bounds", result.stderr)


if __name__ == "__main__": unittest.main()
