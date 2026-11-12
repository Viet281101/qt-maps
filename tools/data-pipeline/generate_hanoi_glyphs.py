#!/usr/bin/env python3
"""Generate the bounded local Noto Sans glyph ranges used by the Hanoi style."""

import argparse
from pathlib import Path

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont


FONT_STACK = "Noto Sans Regular"
PIXEL_SIZE = 24
PADDING = 3
RANGES = ((0, 256), (256, 512), (768, 1024), (7680, 7936))


def varint(value):
    encoded = bytearray()
    while value > 0x7F:
        encoded.append((value & 0x7F) | 0x80)
        value >>= 7
    encoded.append(value)
    return bytes(encoded)


def zigzag(value):
    return (value << 1) ^ (value >> 63)


def field_varint(field_number, value):
    return varint(field_number << 3) + varint(value)


def field_bytes(field_number, value):
    return varint((field_number << 3) | 2) + varint(len(value)) + value


def glyph_message(codepoint, character, font):
    left, top, right, bottom = font.getbbox(character, anchor="ls")
    width = max(0, right - left)
    height = max(0, bottom - top)
    bitmap = b""
    if width and height:
        image = Image.new("L", (width + PADDING * 2, height + PADDING * 2))
        ImageDraw.Draw(image).text(
            (PADDING - left, PADDING - top), character, font=font, fill=255, anchor="ls"
        )
        bitmap = image.tobytes()
        top = -top
    else:
        top = 0
    advance = round(font.getlength(character))
    message = field_varint(1, codepoint)
    if bitmap:
        message += field_bytes(2, bitmap)
    message += field_varint(3, width)
    message += field_varint(4, height)
    message += field_varint(5, zigzag(left))
    message += field_varint(6, zigzag(top))
    message += field_varint(7, advance)
    return message


def write_glyph_ranges(font_path, output):
    font_path = font_path.resolve()
    if not font_path.is_file():
        raise ValueError(f"Font does not exist: {font_path}")
    with TTFont(font_path, lazy=True) as font_file:
        character_map = font_file.getBestCmap()
    font = ImageFont.truetype(font_path, PIXEL_SIZE)
    font_dir = output / FONT_STACK
    font_dir.mkdir(parents=True, exist_ok=True)
    for start, end in RANGES:
        range_name = f"{start}-{end - 1}"
        fontstack = field_bytes(1, FONT_STACK.encode("utf-8"))
        fontstack += field_bytes(2, range_name.encode("ascii"))
        for codepoint in range(start, end):
            if codepoint in character_map:
                fontstack += field_bytes(3, glyph_message(codepoint, chr(codepoint), font))
        (font_dir / f"{range_name}.pbf").write_bytes(field_bytes(1, fontstack))


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def main():
    args = parse_args()
    write_glyph_ranges(args.font, args.output)
    print(f"fontstack={FONT_STACK}")
    print(f"output={args.output / FONT_STACK}")


if __name__ == "__main__":
    main()
