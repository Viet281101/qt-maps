#!/usr/bin/env python3
"""Prepare bounded Hanoi RGB Web-Mercator tiles from one local Sentinel-2 L2A ZIP.

This command has no network path and does not modify the supplied SAFE archive.
It only accepts the B02, B03, and B04 10 m bands from one complete Level-2A
product and writes its generated output outside version control.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path, PurePosixPath
import shutil
import tempfile
import zipfile
import xml.etree.ElementTree as etree

import numpy
from PIL import Image


Image.MAX_IMAGE_PIXELS = None

BOUNDS = (105.78, 20.97, 105.90, 21.10)
BANDS = ("B04", "B03", "B02")
MIN_ZOOM = 12
MAX_ZOOM = 15
CONTRAST_LOW_PERCENTILE = 2
CONTRAST_HIGH_PERCENTILE = 85
EARTH_RADIUS = 6378137.0
ECCENTRICITY_SQUARED = 0.00669438
UTM_SCALE = 0.9996
UTM_ZONE = 48


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def required_member(names, suffix):
    matches = [name for name in names if name.endswith(suffix)]
    if len(matches) != 1:
        raise SystemExit(f"Expected exactly one {suffix}, found {len(matches)}")
    member = PurePosixPath(matches[0])
    if member.is_absolute() or ".." in member.parts:
        raise SystemExit(f"Unsafe SAFE archive member: {matches[0]}")
    return matches[0]


def namespace_free(root, name):
    element = root.find(f".//{{*}}{name}")
    return element.text.strip() if element is not None and element.text else ""


def parse_scene(archive):
    names = archive.namelist()
    metadata_member = required_member(names, "/MTD_MSIL2A.xml")
    tile_metadata_member = required_member(names, "/MTD_TL.xml")
    product = etree.fromstring(archive.read(metadata_member))
    tile = etree.fromstring(archive.read(tile_metadata_member))
    product_id = namespace_free(product, "PRODUCT_URI")
    if not product_id.endswith(".SAFE") or namespace_free(product, "PROCESSING_LEVEL") != "Level-2A":
        raise SystemExit("SAFE archive is not a Sentinel-2 Level-2A product")
    sensing_start = namespace_free(product, "DATATAKE_SENSING_START")
    baseline = namespace_free(product, "PROCESSING_BASELINE")
    cloud_cover = namespace_free(product, "Cloud_Coverage_Assessment")
    code = namespace_free(tile, "HORIZONTAL_CS_CODE")
    if code != "EPSG:32648":
        raise SystemExit(f"Expected Hanoi UTM CRS EPSG:32648, got {code or '<missing>'}")
    size = tile.find(".//{*}Size[@resolution='10']")
    position = tile.find(".//{*}Geoposition[@resolution='10']")
    if size is None or position is None:
        raise SystemExit("SAFE tile metadata lacks 10 m geocoding")
    geocoding = {
        "columns": int(namespace_free(size, "NCOLS")),
        "rows": int(namespace_free(size, "NROWS")),
        "ulx": float(namespace_free(position, "ULX")),
        "uly": float(namespace_free(position, "ULY")),
        "xdim": float(namespace_free(position, "XDIM")),
        "ydim": float(namespace_free(position, "YDIM")),
    }
    if geocoding["xdim"] != 10.0 or geocoding["ydim"] != -10.0:
        raise SystemExit("SAFE 10 m geocoding has unexpected pixel dimensions")
    return {
        "product_id": product_id[:-5],
        "acquisition_date": sensing_start[:10],
        "processing_baseline": baseline,
        "cloud_cover_percent": float(cloud_cover),
        "geocoding": geocoding,
        "bands": {band: required_member(names, f"_{band}_10m.jp2") for band in BANDS},
    }


def lonlat_to_utm48(longitude, latitude):
    """Vectorized WGS84 longitude/latitude to EPSG:32648 northing/easting."""
    longitude = numpy.asarray(longitude, dtype=numpy.float64)
    latitude = numpy.asarray(latitude, dtype=numpy.float64)
    eccentricity_prime_squared = ECCENTRICITY_SQUARED / (1.0 - ECCENTRICITY_SQUARED)
    latitude_radians = numpy.deg2rad(latitude)
    longitude_radians = numpy.deg2rad(longitude)
    origin = numpy.deg2rad((UTM_ZONE - 1) * 6 - 180 + 3)
    n = EARTH_RADIUS / numpy.sqrt(1.0 - ECCENTRICITY_SQUARED * numpy.sin(latitude_radians) ** 2)
    t = numpy.tan(latitude_radians) ** 2
    c = eccentricity_prime_squared * numpy.cos(latitude_radians) ** 2
    a = numpy.cos(latitude_radians) * (longitude_radians - origin)
    m = EARTH_RADIUS * (
        (1 - ECCENTRICITY_SQUARED / 4 - 3 * ECCENTRICITY_SQUARED**2 / 64 - 5 * ECCENTRICITY_SQUARED**3 / 256)
        * latitude_radians
        - (3 * ECCENTRICITY_SQUARED / 8 + 3 * ECCENTRICITY_SQUARED**2 / 32 + 45 * ECCENTRICITY_SQUARED**3 / 1024)
        * numpy.sin(2 * latitude_radians)
        + (15 * ECCENTRICITY_SQUARED**2 / 256 + 45 * ECCENTRICITY_SQUARED**3 / 1024)
        * numpy.sin(4 * latitude_radians)
        - (35 * ECCENTRICITY_SQUARED**3 / 3072) * numpy.sin(6 * latitude_radians)
    )
    easting = UTM_SCALE * n * (
        a + (1 - t + c) * a**3 / 6 + (5 - 18 * t + t**2 + 72 * c - 58 * eccentricity_prime_squared) * a**5 / 120
    ) + 500000.0
    northing = UTM_SCALE * (
        m + n * numpy.tan(latitude_radians) * (a**2 / 2 + (5 - t + 9 * c + 4 * c**2) * a**4 / 24
        + (61 - 58 * t + t**2 + 600 * c - 330 * eccentricity_prime_squared) * a**6 / 720)
    )
    return easting, northing


def longitude_to_x(longitude, zoom):
    return math.floor((longitude + 180.0) / 360.0 * (1 << zoom))


def latitude_to_y(latitude, zoom):
    return math.floor((1.0 - math.asinh(math.tan(math.radians(latitude))) / math.pi) / 2.0 * (1 << zoom))


def expected_tiles():
    west, south, east, north = BOUNDS
    for zoom in range(MIN_ZOOM, MAX_ZOOM + 1):
        for x in range(longitude_to_x(west, zoom), longitude_to_x(east, zoom) + 1):
            for y in range(latitude_to_y(north, zoom), latitude_to_y(south, zoom) + 1):
                yield zoom, x, y


def tile_longitude_latitude(zoom, x, y):
    pixels = (numpy.arange(256, dtype=numpy.float64) + 0.5) / 256.0
    world = float(1 << zoom)
    longitude = ((x + pixels) / world) * 360.0 - 180.0
    mercator_y = math.pi * (1.0 - 2.0 * (y + pixels) / world)
    latitude = numpy.rad2deg(numpy.arctan(numpy.sinh(mercator_y)))
    return numpy.meshgrid(longitude, latitude)


def source_window(scene):
    west, south, east, north = BOUNDS
    longitudes = numpy.array([west, west, east, east])
    latitudes = numpy.array([south, north, south, north])
    easting, northing = lonlat_to_utm48(longitudes, latitudes)
    geocoding = scene["geocoding"]
    columns = (easting - geocoding["ulx"]) / geocoding["xdim"]
    rows = (northing - geocoding["uly"]) / geocoding["ydim"]
    left, right = math.floor(columns.min()) - 2, math.ceil(columns.max()) + 2
    top, bottom = math.floor(rows.min()) - 2, math.ceil(rows.max()) + 2
    if left < 0 or top < 0 or right > geocoding["columns"] or bottom > geocoding["rows"]:
        raise SystemExit("Approved Hanoi bounds are not fully covered by the supplied Sentinel tile")
    return left, top, right, bottom


def load_band(archive, member, window):
    with archive.open(member) as source:
        image = Image.open(source)
        if image.size[0] < window[2] or image.size[1] < window[3]:
            raise SystemExit(f"Sentinel band dimensions do not match metadata: {member}")
        return numpy.asarray(image.crop(window), dtype=numpy.float32)


def bilinear(array, x, y):
    x = numpy.clip(x, 0, array.shape[1] - 1)
    y = numpy.clip(y, 0, array.shape[0] - 1)
    x0, y0 = numpy.floor(x).astype(numpy.int32), numpy.floor(y).astype(numpy.int32)
    x1, y1 = numpy.minimum(x0 + 1, array.shape[1] - 1), numpy.minimum(y0 + 1, array.shape[0] - 1)
    fx, fy = x - x0, y - y0
    return ((array[y0, x0] * (1 - fx) + array[y0, x1] * fx) * (1 - fy)
            + (array[y1, x0] * (1 - fx) + array[y1, x1] * fx) * fy)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scene", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--verify-only", action="store_true")
    return parser.parse_args()


def main():
    args = parse_args()
    scene_path = args.scene.resolve()
    if not scene_path.is_file() or scene_path.suffix != ".zip":
        raise SystemExit("--scene must be a local SAFE .zip file")
    with zipfile.ZipFile(scene_path) as archive:
        scene = parse_scene(archive)
        scene["sha256"] = sha256(scene_path)
        if args.verify_only:
            print(json.dumps(scene, indent=2, sort_keys=True))
            return
        output = args.output.resolve()
        if output.exists() and any(output.iterdir()):
            raise SystemExit(f"Refusing to overwrite non-empty output: {output}")
        window = source_window(scene)
        bands = {band: load_band(archive, scene["bands"][band], window) for band in BANDS}
    output.mkdir(parents=True, exist_ok=True)
    try:
        low_high = {
            band: numpy.percentile(values[values > 0],
                                   (CONTRAST_LOW_PERCENTILE, CONTRAST_HIGH_PERCENTILE))
            for band, values in bands.items()
        }
        geocoding = scene["geocoding"]
        for zoom, x, y in expected_tiles():
            longitude, latitude = tile_longitude_latitude(zoom, x, y)
            easting, northing = lonlat_to_utm48(longitude, latitude)
            source_x = (easting - geocoding["ulx"]) / geocoding["xdim"] - window[0]
            source_y = (northing - geocoding["uly"]) / geocoding["ydim"] - window[1]
            channels = []
            for band in BANDS:
                low, high = low_high[band]
                channel = numpy.clip((bilinear(bands[band], source_x, source_y) - low) * 255.0 / (high - low), 0, 255)
                channels.append(channel.astype(numpy.uint8))
            tile = Image.fromarray(numpy.dstack(channels), mode="RGB")
            path = output / str(zoom) / str(x) / f"{y}.png"
            path.parent.mkdir(parents=True, exist_ok=True)
            tile.save(path, "PNG", optimize=True)
        provenance = {
            "scene": scene,
            "bounds": list(BOUNDS),
            "zoom_range": [MIN_ZOOM, MAX_ZOOM],
            "resampling": "bilinear",
            "contrast_stretch": "per-band 2nd--85th percentile over Hanoi source window",
            "network_access": False,
        }
        (output.parent / f"{output.name}-provenance.json").write_text(
            json.dumps(provenance, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )
        print(f"tiles={sum(1 for _ in expected_tiles())} output={output}")
        print(f"product_id={scene['product_id']} sha256={scene['sha256']}")
    except BaseException:
        shutil.rmtree(output, ignore_errors=True)
        raise


if __name__ == "__main__":
    main()
