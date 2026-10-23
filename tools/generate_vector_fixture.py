#!/usr/bin/env python3
"""Generate a synthetic local MVT/style/glyph/sprite mechanics fixture.

The output contains no OpenStreetMap or other geographic source data. It uses
Hanoi coordinates only to exercise XYZ addressing in the Qt 5.13 Mapbox GL
plugin. It is not a substitute for the approved real-data Unit 01b benchmark.
Only the Python standard library is used and the script performs no network I/O.
"""

import argparse
import json
import math
from pathlib import Path
import struct
import zlib


HANOI_LATITUDE = 21.0278
HANOI_LONGITUDE = 105.8342
EXTENT = 4096

PIXEL_GLYPHS = {
    "A": ("01110", "10001", "10001", "11111", "10001", "10001", "10001"),
    "À": ("00100", "01000", "01110", "10001", "11111", "10001", "10001", "10001"),
    "H": ("10001", "10001", "10001", "11111", "10001", "10001", "10001"),
    "I": ("11111", "00100", "00100", "00100", "00100", "00100", "11111"),
    "N": ("10001", "11001", "11001", "10101", "10011", "10011", "10001"),
    "O": ("01110", "10001", "10001", "10001", "10001", "10001", "01110"),
    "Ộ": ("00100", "01110", "10001", "10001", "10001", "10001", "01110", "00100"),
}


def varint(value):
    result = bytearray()
    while value > 0x7F:
        result.append((value & 0x7F) | 0x80)
        value >>= 7
    result.append(value)
    return bytes(result)


def zigzag(value):
    return (value << 1) ^ (value >> 31)


def field_varint(number, value):
    return varint(number << 3) + varint(value)


def field_bytes(number, value):
    return varint((number << 3) | 2) + varint(len(value)) + value


def packed_varints(values):
    return b"".join(varint(value) for value in values)


def command(command_id, count):
    return (count << 3) | command_id


def geometry_point(x, y):
    return [command(1, 1), zigzag(x), zigzag(y)]


def geometry_line(points):
    geometry = [command(1, 1), zigzag(points[0][0]), zigzag(points[0][1])]
    geometry.append(command(2, len(points) - 1))
    previous_x, previous_y = points[0]
    for x, y in points[1:]:
        geometry.extend((zigzag(x - previous_x), zigzag(y - previous_y)))
        previous_x, previous_y = x, y
    return geometry


def geometry_polygon(points):
    geometry = geometry_line(points)
    geometry.append(command(7, 1))
    return geometry


def string_value(value):
    return field_bytes(1, value.encode("utf-8"))


def feature(feature_id, tags, geometry_type, geometry):
    message = field_varint(1, feature_id)
    message += field_bytes(2, packed_varints(tags))
    message += field_varint(3, geometry_type)
    message += field_bytes(4, packed_varints(geometry))
    return message


def vector_tile(tile_x, tile_y):
    keys = ["kind", "name"]
    values = ["building", "road", "label", "HANOI", "Synthetic Road"]
    offset = ((tile_x * 97 + tile_y * 53) % 300) - 150

    features = [
        feature(
            1,
            [0, 0, 1, 3],
            3,
            geometry_polygon(
                [
                    (900 + offset, 950),
                    (3000 + offset, 950),
                    (3000 + offset, 2850),
                    (900 + offset, 2850),
                ]
            ),
        ),
        feature(
            2,
            [0, 1, 1, 4],
            2,
            geometry_line([(0, 1900 + offset), (1500, 1800), (3000, 2100), (4096, 1950)]),
        ),
        feature(
            3,
            [0, 1, 1, 4],
            2,
            geometry_line([(2100, 0), (1950, 1400), (2200, 2800), (2050, 4096)]),
        ),
        feature(4, [0, 2, 1, 3], 1, geometry_point(2048, 1650)),
    ]

    layer = field_bytes(1, b"qtmaps")
    for item in features:
        layer += field_bytes(2, item)
    for key in keys:
        layer += field_bytes(3, key.encode("utf-8"))
    for value in values:
        layer += field_bytes(4, string_value(value))
    layer += field_varint(5, EXTENT)
    layer += field_varint(15, 2)
    return field_bytes(3, layer)


def web_mercator_tile(latitude, longitude, zoom):
    latitude = max(-85.05112878, min(85.05112878, latitude))
    count = 1 << zoom
    x = math.floor((longitude + 180.0) / 360.0 * count)
    latitude_radians = math.radians(latitude)
    y = math.floor(
        (1.0 - math.asinh(math.tan(latitude_radians)) / math.pi) / 2.0 * count
    )
    return x, y


def png_chunk(chunk_type, payload):
    checksum = zlib.crc32(chunk_type)
    checksum = zlib.crc32(payload, checksum) & 0xFFFFFFFF
    return struct.pack(">I", len(payload)) + chunk_type + payload + struct.pack(">I", checksum)


def rgba_png(width, height, pixels):
    rows = bytearray()
    stride = width * 4
    for row in range(height):
        rows.append(0)
        rows.extend(pixels[row * stride : (row + 1) * stride])
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", header)
        + png_chunk(b"IDAT", zlib.compress(bytes(rows), level=9))
        + png_chunk(b"IEND", b"")
    )


def write_sprite(output):
    size = 16
    pixels = bytearray(size * size * 4)
    for y in range(size):
        for x in range(size):
            distance = math.hypot(x - 7.5, y - 7.5)
            color = (239, 68, 68, 255) if distance <= 6.5 else (0, 0, 0, 0)
            offset = (y * size + x) * 4
            pixels[offset : offset + 4] = bytes(color)
    (output / "sprite.png").write_bytes(rgba_png(size, size, pixels))
    (output / "sprite.json").write_text(
        json.dumps({"marker": {"x": 0, "y": 0, "width": 16, "height": 16, "pixelRatio": 1}}, indent=2) + "\n",
        encoding="utf-8",
    )


def glyph_bitmap(character, scale=2, padding=3):
    pattern = PIXEL_GLYPHS[character]
    core_width = len(pattern[0]) * scale
    core_height = len(pattern) * scale
    bitmap_width = core_width + padding * 2
    bitmap_height = core_height + padding * 2
    bitmap = bytearray(bitmap_width * bitmap_height)
    for row, bits in enumerate(pattern):
        for column, bit in enumerate(bits):
            if bit == "1":
                for dy in range(scale):
                    for dx in range(scale):
                        x = padding + column * scale + dx
                        y = padding + row * scale + dy
                        bitmap[y * bitmap_width + x] = 255
    return bytes(bitmap), core_width, core_height


def glyph_message(codepoint, character):
    if character == " ":
        bitmap, width, height, advance = b"", 0, 0, 6
    else:
        bitmap, width, height = glyph_bitmap(character)
        advance = width + 2
    message = field_varint(1, codepoint)
    if bitmap:
        message += field_bytes(2, bitmap)
    message += field_varint(3, width)
    message += field_varint(4, height)
    message += field_varint(5, zigzag(0))
    message += field_varint(6, zigzag(height))
    message += field_varint(7, advance)
    return message


def write_glyphs(output):
    characters = sorted(set(" AHINOÀỘ"), key=ord)
    ranges = {}
    for character in characters:
        start = ord(character) // 256 * 256
        ranges.setdefault(start, []).append(character)
    for start, range_characters in ranges.items():
        range_name = f"{start}-{start + 255}"
        fontstack = field_bytes(1, b"QtMapsSans") + field_bytes(2, range_name.encode("ascii"))
        for character in range_characters:
            fontstack += field_bytes(3, glyph_message(ord(character), character))
        glyphs = field_bytes(1, fontstack)
        path = output / "fonts" / "QtMapsSans" / f"{range_name}.pbf"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(glyphs)


def write_style(output, real_data=False):
    source_layers = {
        "building": "building" if real_data else "qtmaps",
        "roads": "transportation" if real_data else "qtmaps",
        "labels": "place" if real_data else "qtmaps",
    }
    attribution = (
        "© OpenStreetMap contributors, ODbL 1.0"
        if real_data
        else "Synthetic QtMaps mechanics fixture; not geographic data"
    )
    style = {
        "version": 8,
        "name": "QtMaps bounded Hanoi real-data vector probe" if real_data else "QtMaps synthetic local vector mechanics fixture",
        "sprite": "http://127.0.0.1:8766/sprite",
        "glyphs": "http://127.0.0.1:8766/fonts/{fontstack}/{range}.pbf",
        "sources": {
            "qtmaps": {
                "type": "vector",
                "tiles": ["http://127.0.0.1:8766/tiles/{z}/{x}/{y}.pbf"],
                "minzoom": 12,
                "maxzoom": 15,
                "attribution": attribution,
            }
        },
        "layers": [
            {"id": "background", "type": "background", "paint": {"background-color": "#dbeafe"}},
            {
                "id": "buildings",
                "type": "fill",
                "source": "qtmaps",
                "source-layer": source_layers["building"],
                **({} if real_data else {"filter": ["==", "kind", "building"]}),
                "paint": {"fill-color": "#94a3b8", "fill-outline-color": "#475569"},
            },
            {
                "id": "roads",
                "type": "line",
                "source": "qtmaps",
                "source-layer": source_layers["roads"],
                **({} if real_data else {"filter": ["==", "kind", "road"]}),
                "paint": {"line-color": "#f8fafc", "line-width": 5},
            },
            {
                "id": "labels",
                "type": "symbol",
                "source": "qtmaps",
                "source-layer": source_layers["labels"],
                "filter": ["all", ["==", "class", "city"], ["==", "name", "Hà Nội"]]
                if real_data
                else ["==", "kind", "label"],
                "layout": {
                    "icon-image": "marker",
                    "icon-size": 0.75,
                    "text-field": "HÀ NỘI" if real_data else "{name}",
                    "text-font": ["QtMapsSans"],
                    "text-size": 16,
                    "text-offset": [0, 1.2],
                },
                "paint": {"text-color": "#0f172a", "text-halo-color": "#ffffff", "text-halo-width": 1},
            },
        ],
    }
    (output / "style.json").write_text(json.dumps(style, indent=2) + "\n", encoding="utf-8")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("generated/vector-fixture"))
    parser.add_argument("--min-zoom", type=int, default=12)
    parser.add_argument("--max-zoom", type=int, default=15)
    parser.add_argument("--radius", type=int, default=2)
    parser.add_argument(
        "--real-style-only",
        action="store_true",
        help="write local resources for Planetiler OpenMapTiles layers without synthetic tiles",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    total = 0
    coverage = []
    for zoom in range(args.min_zoom, args.max_zoom + 1) if not args.real_style_only else []:
        center_x, center_y = web_mercator_tile(HANOI_LATITUDE, HANOI_LONGITUDE, zoom)
        count = 1 << zoom
        for y in range(max(0, center_y - args.radius), min(count, center_y + args.radius + 1)):
            for x_offset in range(-args.radius, args.radius + 1):
                x = (center_x + x_offset) % count
                path = args.output / "tiles" / str(zoom) / str(x) / f"{y}.pbf"
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(vector_tile(x, y))
                total += 1
        coverage.append({"zoom": zoom, "center_x": center_x, "center_y": center_y, "radius": args.radius})
    write_style(args.output, real_data=args.real_style_only)
    write_sprite(args.output)
    write_glyphs(args.output)
    manifest = {
        "id": "unit01b-hanoi-real-v1" if args.real_style_only else "synthetic-vector-mechanics-v1",
        "geographic_data": args.real_style_only,
        "network_access_during_generation": False,
        "tile_count": total,
        "coverage": coverage,
        "notes": (
            "Style resources for bounded real OpenStreetMap-derived Planetiler output; tiles are generated separately."
            if args.real_style_only
            else "Synthetic MVT geometry at Hanoi XYZ addresses; not real map data or an official benchmark dataset."
        ),
    }
    (args.output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(
        f"output={args.output} total_tiles={total} "
        f"mode={'real-style-only' if args.real_style_only else 'synthetic'}"
    )


if __name__ == "__main__":
    main()
