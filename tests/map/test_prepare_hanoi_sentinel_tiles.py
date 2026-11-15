#!/usr/bin/env python3
"""Verify SAFE ZIP metadata gating without using imagery or a network."""
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools/data-pipeline/prepare_hanoi_sentinel_tiles.py"
ROOT_NAME = "S2C_MSIL2A_20261014T032711_N0513_R018_T48QWJ_20261014T082606.SAFE"

PRODUCT = """<n1:Level-2A_User_Product xmlns:n1=\"urn:test\">
<n1:PRODUCT_URI>S2C_MSIL2A_20261014T032711_N0513_R018_T48QWJ_20261014T082606.SAFE</n1:PRODUCT_URI>
<n1:PROCESSING_LEVEL>Level-2A</n1:PROCESSING_LEVEL>
<n1:DATATAKE_SENSING_START>2026-10-14T03:27:11.025Z</n1:DATATAKE_SENSING_START>
<n1:PROCESSING_BASELINE>05.13</n1:PROCESSING_BASELINE>
<n1:Cloud_Coverage_Assessment>12.59245</n1:Cloud_Coverage_Assessment></n1:Level-2A_User_Product>"""

TILE = """<n1:Level-2A_Tile_ID xmlns:n1=\"urn:test\"><n1:HORIZONTAL_CS_CODE>EPSG:32648</n1:HORIZONTAL_CS_CODE>
<n1:Size resolution=\"10\"><n1:NCOLS>10980</n1:NCOLS><n1:NROWS>10980</n1:NROWS></n1:Size>
<n1:Geoposition resolution=\"10\"><n1:ULX>499980</n1:ULX><n1:ULY>2400000</n1:ULY><n1:XDIM>10</n1:XDIM><n1:YDIM>-10</n1:YDIM></n1:Geoposition></n1:Level-2A_Tile_ID>"""


class SentinelTilePreparationTest(unittest.TestCase):
    def write_scene(self, root, level="Level-2A"):
        scene = root / "scene.zip"
        product = PRODUCT.replace("Level-2A</n1:PROCESSING_LEVEL>", f"{level}</n1:PROCESSING_LEVEL>")
        prefix = f"{ROOT_NAME}/GRANULE/L2A_T48QWJ_A010999_20261014T034028"
        with zipfile.ZipFile(scene, "w") as archive:
            archive.writestr(f"{ROOT_NAME}/MTD_MSIL2A.xml", product)
            archive.writestr(f"{prefix}/MTD_TL.xml", TILE)
            for band in ("B02", "B03", "B04"):
                archive.writestr(f"{prefix}/IMG_DATA/R10m/T48QWJ_20261014T032711_{band}_10m.jp2", b"fixture")
        return scene

    def test_verify_only_records_required_l2a_provenance(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            completed = subprocess.run(["python3", str(TOOL), "--scene", str(self.write_scene(root)),
                                        "--output", str(root / "output"), "--verify-only"],
                                       cwd=ROOT, check=True, capture_output=True, text=True)
        scene = json.loads(completed.stdout)
        self.assertEqual(scene["product_id"], ROOT_NAME[:-5])
        self.assertEqual(scene["processing_baseline"], "05.13")
        self.assertEqual(scene["geocoding"]["columns"], 10980)
        self.assertEqual(sorted(scene["bands"]), ["B02", "B03", "B04"])

    def test_non_l2a_scene_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            completed = subprocess.run(["python3", str(TOOL), "--scene", str(self.write_scene(root, "Level-1C")),
                                        "--output", str(root / "output"), "--verify-only"],
                                       cwd=ROOT, capture_output=True, text=True)
        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("Level-2A", completed.stderr)


if __name__ == "__main__":
    unittest.main()
