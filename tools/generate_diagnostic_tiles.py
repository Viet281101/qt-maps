#!/usr/bin/env python3
"""Generate non-geographic XYZ diagnostic tiles around a Web Mercator point.

This fixture contains only colored grids and Z/X/Y labels. It is NOT real map
data and must not be presented as a geographic map.

For longitude ``lon`` and latitude ``lat`` in degrees at zoom ``z``:

    n = 2 ** z
    x = floor((lon + 180) / 360 * n)
    y = floor((1 - asinh(tan(lat * pi / 180)) / pi) / 2 * n)

The script writes standard XYZ paths: ``<output>/<z>/<x>/<y>.png``.
It uses only the Python standard library and performs no network access.
"""

import argparse
import math
from pathlib import Path
import struct
import zlib


TILE_SIZE = 256
HANOI_LATITUDE = 21.0278
HANOI_LONGITUDE = 105.8342

GLYPHS = {
    "0": ("111", "101", "101", "101", "111"),
    "1": ("010", "110", "010", "010", "111"),
    "2": ("111", "001", "111", "100", "111"),
    "3": ("111", "001", "111", "001", "111"),
    "4": ("101", "101", "111", "001", "001"),
    "5": ("111", "100", "111", "001", "111"),
    "6": ("111", "100", "111", "101", "111"),
    "7": ("111", "001", "010", "010", "010"),
    "8": ("111", "101", "111", "101", "111"),
    "9": ("111", "101", "111", "001", "111"),
    "/": ("001", "001", "010", "100", "100"),
    "X": ("101", "101", "010", "101", "101"),
    "Y": ("101", "101", "010", "010", "010"),
    "Z": ("111", "001", "010", "100", "111"),
}


def web_mercator_tile(latitude, longitude, zoom):
    """Return the XYZ tile containing a WGS84 coordinate."""
    latitude = max(-85.05112878, min(85.05112878, latitude))
    tile_count = 1 << zoom
    x = math.floor((longitude + 180.0) / 360.0 * tile_count)
    latitude_radians = math.radians(latitude)
    y = math.floor(
        (1.0 - math.asinh(math.tan(latitude_radians)) / math.pi)
        / 2.0
        * tile_count
    )
    return x, y


def png_chunk(chunk_type, payload):
    checksum = zlib.crc32(chunk_type)
    checksum = zlib.crc32(payload, checksum) & 0xFFFFFFFF
    return struct.pack(">I", len(payload)) + chunk_type + payload + struct.pack(">I", checksum)


def set_pixel(pixels, x, y, color):
    if 0 <= x < TILE_SIZE and 0 <= y < TILE_SIZE:
        offset = (y * TILE_SIZE + x) * 3
        pixels[offset : offset + 3] = bytes(color)


def fill_rectangle(pixels, left, top, right, bottom, color):
    for y in range(max(0, top), min(TILE_SIZE, bottom)):
        for x in range(max(0, left), min(TILE_SIZE, right)):
            set_pixel(pixels, x, y, color)


def draw_text(pixels, text, left, top, scale, color):
    cursor = left
    for character in text:
        glyph = GLYPHS[character]
        for row, bits in enumerate(glyph):
            for column, bit in enumerate(bits):
                if bit == "1":
                    fill_rectangle(
                        pixels,
                        cursor + column * scale,
                        top + row * scale,
                        cursor + (column + 1) * scale,
                        top + (row + 1) * scale,
                        color,
                    )
        cursor += 4 * scale


def write_tile(path, zoom, x, y):
    background = (
        80 + (x * 37 + zoom * 11) % 120,
        80 + (y * 53 + zoom * 17) % 120,
        80 + ((x + y) * 29 + zoom * 23) % 120,
    )
    pixels = bytearray(background * (TILE_SIZE * TILE_SIZE))

    for position in range(0, TILE_SIZE, 32):
        fill_rectangle(pixels, position, 0, position + 1, TILE_SIZE, (255, 255, 255))
        fill_rectangle(pixels, 0, position, TILE_SIZE, position + 1, (255, 255, 255))

    border = 4
    fill_rectangle(pixels, 0, 0, TILE_SIZE, border, (0, 0, 0))
    fill_rectangle(pixels, 0, TILE_SIZE - border, TILE_SIZE, TILE_SIZE, (0, 0, 0))
    fill_rectangle(pixels, 0, 0, border, TILE_SIZE, (0, 0, 0))
    fill_rectangle(pixels, TILE_SIZE - border, 0, TILE_SIZE, TILE_SIZE, (0, 0, 0))

    label = f"Z{zoom}/X{x}/Y{y}"
    scale = 3
    label_width = len(label) * 4 * scale
    fill_rectangle(pixels, 8, 10, min(TILE_SIZE - 8, 16 + label_width), 35, (255, 255, 255))
    draw_text(pixels, label, 12, 15, scale, (0, 0, 0))

    raw_rows = bytearray()
    stride = TILE_SIZE * 3
    for row in range(TILE_SIZE):
        raw_rows.append(0)
        start = row * stride
        raw_rows.extend(pixels[start : start + stride])

    header = struct.pack(">IIBBBBB", TILE_SIZE, TILE_SIZE, 8, 2, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n"
    png += png_chunk(b"IHDR", header)
    png += png_chunk(b"IDAT", zlib.compress(bytes(raw_rows), level=9))
    png += png_chunk(b"IEND", b"")

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("generated/diagnostic-tiles"))
    parser.add_argument("--min-zoom", type=int, default=10)
    parser.add_argument("--max-zoom", type=int, default=14)
    parser.add_argument("--radius", type=int, default=3, help="Tile radius; 3 generates a 7x7 set per zoom")
    return parser.parse_args()


def main():
    args = parse_args()
    total = 0
    for zoom in range(args.min_zoom, args.max_zoom + 1):
        center_x, center_y = web_mercator_tile(HANOI_LATITUDE, HANOI_LONGITUDE, zoom)
        tile_count = 1 << zoom
        zoom_total = 0
        for y in range(max(0, center_y - args.radius), min(tile_count, center_y + args.radius + 1)):
            for x_offset in range(-args.radius, args.radius + 1):
                x = (center_x + x_offset) % tile_count
                write_tile(args.output / str(zoom) / str(x) / f"{y}.png", zoom, x, y)
                zoom_total += 1
                total += 1
        print(f"z={zoom} center={center_x}/{center_y} tiles={zoom_total}")
    print(f"output={args.output} total_tiles={total} diagnostic_only=true")


if __name__ == "__main__":
    main()
