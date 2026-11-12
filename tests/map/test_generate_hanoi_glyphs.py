#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools/data-pipeline/generate_hanoi_glyphs.py"
FONT = Path("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf")


def load_tool():
    spec = importlib.util.spec_from_file_location("generate_hanoi_glyphs", TOOL)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def read_varint(data, offset):
    value = 0
    shift = 0
    while True:
        byte = data[offset]
        offset += 1
        value |= (byte & 0x7F) << shift
        if not byte & 0x80:
            return value, offset
        shift += 7


def fields(data):
    offset = 0
    while offset < len(data):
        key, offset = read_varint(data, offset)
        field = key >> 3
        wire_type = key & 7
        if wire_type == 0:
            value, offset = read_varint(data, offset)
        elif wire_type == 2:
            length, offset = read_varint(data, offset)
            value = data[offset : offset + length]
            offset += length
        else:
            raise AssertionError(f"unexpected wire type {wire_type}")
        yield field, value


def glyph_fields(path, codepoint):
    outer = dict(fields(path.read_bytes()))
    for field, glyph in fields(outer[1]):
        if field != 3:
            continue
        values = dict(fields(glyph))
        if values[1] == codepoint:
            return values
    raise AssertionError(f"missing glyph {codepoint}")


class HanoiGlyphsTest(unittest.TestCase):
    def test_generates_bounded_noto_ranges_with_vietnamese_glyphs(self):
        tool = load_tool()
        self.assertTrue(FONT.is_file(), f"missing required font: {FONT}")
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "glyphs"
            tool.write_glyph_ranges(FONT, output)
            glyph_dir = output / tool.FONT_STACK
            expected = {"0-255.pbf", "256-511.pbf", "768-1023.pbf", "7680-7935.pbf"}
            self.assertEqual({path.name for path in glyph_dir.iterdir()}, expected)
            for name in expected:
                self.assertGreater((glyph_dir / name).stat().st_size, 1000)
            self.assertIn(tool.FONT_STACK.encode("utf-8"), (glyph_dir / "0-255.pbf").read_bytes())
            self.assertIn(b"\x08" + tool.varint(ord("Đ")), (glyph_dir / "256-511.pbf").read_bytes())
            self.assertIn(b"\x08" + tool.varint(ord("ộ")), (glyph_dir / "7680-7935.pbf").read_bytes())
            glyph = glyph_fields(glyph_dir / "256-511.pbf", ord("Đ"))
            self.assertEqual(len(glyph[2]), (glyph[3] + 6) * (glyph[4] + 6))


if __name__ == "__main__":
    unittest.main()
