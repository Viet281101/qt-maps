# Unit 07C — Bounded Offline Raster Fallback

## Source and license

- Source: Geofabrik Vietnam OpenStreetMap extract retrieved on 2026-10-23 from
  <https://download.geofabrik.de/asia/vietnam.html> after owner approval.
- Source file: `generated/unit-01b/source/vietnam-latest.osm.pbf` (ignored),
  331,337,283 bytes, SHA-256
  `4e5243c3116dc089a298f4cd7218e1257adf4b48355e42936360df5ce3200d43`.
- License: Open Database License 1.0. The product always displays
  `© OpenStreetMap contributors, ODbL 1.0`.
- Renderer: dynamically linked MapLibre Native Qt 3.0.0 using the validated
  `hanoi-20261024-v1` vector package and `qtmaps-hanoi-v1` style. Rendering and
  serving use loopback only; no public raster/vector tile endpoint is contacted.

## Coverage and measured package

The package covers `105.78,20.97,105.90,21.10`. The fixed demo endpoints span
`105.8197..105.8542 / 21.0116..21.0369`; their 10% expanded envelope is
`105.81625..105.85765 / 21.00907..21.03943`, so every side remains inside the
raster bounds. Both recorded route geometries are also inside. The raster is a
strict subset of the Valhalla graph bbox
`105.20,20.45,106.20,21.45`.

| Zoom | Tiles |
| ---: | ---: |
| 12 | 4 |
| 13 | 12 |
| 14 | 42 |
| 15 | 168 |
| 16 | 621 |
| **Total** | **847** |

The 2026-10-31 provisional pre-07D render took 543.626 seconds. Its immutable
MBTiles is 27,054,080 bytes (25.80 MiB), well below the 256 MiB review cap, with
SHA-256 `26a18c4ba04ad4862a1a50e23ca19ca84ea02a23e0ae6665df060ffca5c2d4e7`.
Two independent packaging runs produced identical manifest and MBTiles bytes;
the manifest SHA-256 is
`f2cfd074c36ebf50c506dcd3dbf43b4f4fbfb281bcc1789d048d75ec6076919c`.

The current vector input ends at z15, so the provisional z16 raster is rendered
from overzoomed z15 vector content. It provides 256 px tiles along the full demo
bounds but not new source detail. The UI labels views above z16 as lower
resolution. Unit 07D/#32 must rebuild the vector package and then regenerate
this raster; the runtime rejects the old style/package contract after that
migration.

## Reproduction

The commands perform no download and generated outputs remain ignored:

```bash
make hanoi-raster-render \
  MAP_PACKAGE=data/packages/current \
  RASTER_TILES=generated/unit-07c/rendered-tiles

make hanoi-raster-package \
  MAP_PACKAGE=data/packages/current \
  RASTER_TILES=generated/unit-07c/rendered-tiles
```

`build_hanoi_raster_package.py` requires exactly the contracted z/x/y PNG set,
checks each image is 256×256, writes tiles in deterministic order, records the
source vector manifest and PBF identity, and refuses to overwrite an immutable
package. The `data/raster-packages/current` symlink selects the local result.

## Runtime and cache behavior

- Vector remains the default. Manual selection and automatic vector-start
  failure are the only raster activation conditions; each is tested.
- The raster worker, read-only SQLite connection, loopback server, and Qt OSM
  custom-host renderer are created on first activation. The 64 MiB CPU-side
  budget comprises 8 MiB server-encoded PNG LRU, 8 MiB Qt compressed cache,
  16 MiB decoded-texture cache, and 32 MiB reserved for mandatory visible tiles
  at the supported maximum 1920×1080 viewport. Disk caching and prefetch are
  disabled. Returning to vector releases both renderer and server caches.
  GPU/driver allocations are not directly capped by Qt's cache API and are
  covered by process PSS/RSS measurements.
- Manifest hashing, database startup, and tile reads run on the worker thread.
  Missing or incompatible data changes the visible state without blocking the
  UI or silently using another source.
- Switching recreates the MapLibre map. Raster mode uses a Qt Quick route
  overlay so the opaque raster style cannot cover the selected route, A/B
  markers, travelled segment, or navigation puck during cold-start fallback.
- Review-only screenshots are under `generated/unit-07c/`; raster data,
  screenshots, and benchmark artifacts are not committed.

The provisional z16 images contain only z15 source detail. At camera zoom 16
and above, the persistent map badge reads `Nguồn chi tiết đến z15 · z16+ có độ
phân giải thấp hơn`.

## Release verification

The 2026-10-31 quick matrix used the locked Unit 06 thresholds, 15 seconds of
measurement after a 3-second warm-up, and three runs per network mode. Values
below are medians; the parenthesized value is the worst run for that metric.
CPU is normalized across all four logical CPUs.

| Source / mode | PSS average MiB | PSS peak MiB | CPU average % | CPU peak % |
| --- | ---: | ---: | ---: | ---: |
| Vector / offline | 451.15 (453.01) | 544.42 (545.68) | 18.05 (18.16) | 40.84 (40.84) |
| Vector / online | 446.02 (460.79) | 536.75 (544.68) | 17.25 (17.73) | 29.88 (38.38) |
| Raster / offline | 426.97 (427.83) | 512.62 (514.01) | 16.92 (17.22) | 34.62 (35.65) |
| Raster / online | 413.50 (415.43) | 512.67 (514.79) | 17.04 (17.88) | 40.18 (50.00) |

All groups pass. In particular, fallback navigation remains 123.03 MiB below
the 550 MiB median-average PSS lock in its worse median mode. The default vector
run does not create the raster worker/cache, while the fallback run does not
retain the vector tile engine, so the 64 MiB cache is not cumulative.

`generated/unit-07c/raster-connect.trace` records only Unix sockets and IPv4
`127.0.0.1`; the missing-package smoke exits cleanly without freezing. A real
Valhalla smoke returned the 7,294 m / 523.659 s / 14-maneuver route while the
raster layer was active. Exact 900×580, 1366×768, and 1920×1080 navigation
captures under `generated/unit-07c/review/` show the route, A marker, position
puck, banner, source state, and always-visible attribution. These review,
benchmark, trace, and raster artifacts remain ignored and are not committed.

Repeat the complete render/package/visual/benchmark evidence after Unit 07D/#32.
