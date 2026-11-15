#!/usr/bin/env python3
"""Exercise the local-only Hanoi OSM PBF extractor with a compact fixture."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
EXTRACTOR = ROOT / "tools/data-pipeline/extract_hanoi_search_records.py"


class HanoiSearchExtractorTest(unittest.TestCase):
    def test_keeps_approved_named_nodes_and_aliases(self):
        source = """<osm version=\"0.6\">
          <node id=\"1\" lat=\"21.028\" lon=\"105.852\"><tag k=\"place\" v=\"square\"/><tag k=\"name\" v=\"Hồ Hoàn Kiếm\"/><tag k=\"name:vi\" v=\"Hồ Gươm\"/><tag k=\"alt_name\" v=\"Hoan Kiem Lake\"/></node>
          <node id=\"2\" lat=\"21.030\" lon=\"105.850\"><tag k=\"amenity\" v=\"cafe\"/><tag k=\"name\" v=\"Cà phê mẫu\"/></node>
          <node id=\"3\" lat=\"21.030\" lon=\"105.850\"><tag k=\"highway\" v=\"bus_stop\"/><tag k=\"name\" v=\"Không lập chỉ mục\"/></node>
        </osm>"""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            osm = root / "fixture.osm"
            pbf = root / "fixture.osm.pbf"
            output = root / "places.jsonl"
            osm.write_text(source, encoding="utf-8")
            subprocess.run(["osmium", "cat", str(osm), "-o", str(pbf)], check=True)
            subprocess.run([sys.executable, str(EXTRACTOR), "--pbf", str(pbf), "--output", str(output)], check=True)
            records = [json.loads(line) for line in output.read_text(encoding="utf-8").splitlines()]
        self.assertEqual([record["name"] for record in records], ["Hồ Hoàn Kiếm", "Cà phê mẫu"])
        self.assertEqual(records[0]["aliases"], ["Hồ Gươm", "Hoan Kiem Lake"])
        self.assertEqual(records[0]["category"], "place")
        self.assertTrue(records[0]["source_id"].startswith("n"))


if __name__ == "__main__":
    unittest.main()
