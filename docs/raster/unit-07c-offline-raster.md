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

The final 2026-10-31 matrix uses the locked Unit 06 thresholds, 15 seconds of
measurement after a 3-second warm-up, N=3, online and loopback-only offline,
and both `pan_zoom` and `navigation_replay`. Every table records duration,
fresh PSS sample count, sampler intervals, FPS average/p5/1% low, frame-time
p95, sampler p95, PSS average/peak, CPU average/peak, min/max, worst-run bound,
and per-threshold PASS/FAIL. The complete same-day A/B and explanation of the
older Unit 06 difference are in
`docs/benchmarks/unit-07bc-ab-comparison.md`.

Fallback navigation passes every median lock. Offline PSS average/peak is
386.23/488.12 MiB and online is 389.94/487.62 MiB. Machine CPU average/peak is
13.31/24.80% offline and 13.71/26.05% online. FPS average/p5/1% low is
59.86/51.16/37.97 offline and 59.74/50.61/38.32 online; frame-time p95 remains
19.55/19.76 ms. The worst-run bounds are retained in the comparison report.

Continuous raster pan/zoom uses 333.59/342.67 MiB PSS average/peak offline and
333.74/342.74 MiB online, versus 417.43/437.39 and 418.09/435.01 MiB for the
same-revision vector path. Ten raster/vector cycles return to a vector range of
203,032–217,272 KiB PSS and 277,200–291,440 KiB RSS from a 218,199/292,244 KiB
baseline, ending below baseline. Raster resources are false after every return,
so the worker and renderer are released and the 64 MiB raster budget is not
cumulative with MapLibre.

`generated/unit-07c/raster-connect.trace` records only Unix sockets and IPv4
`127.0.0.1`; the missing-package smoke exits cleanly without freezing. A real
Valhalla smoke returned the 7,294 m / 523.659 s / 14-maneuver route while the
raster layer was active. The final exact 1366×768 captures are
`generated/unit-07c/fallback-navigation-1366x768.png` and
`generated/unit-07c/fallback-rerouting-1366x768.png`. Both show the old route,
A marker, position puck, source state, attribution, and the persistent
`z16+ có độ phân giải thấp hơn` label; re-routing keeps the route visible while
Valhalla is deliberately paused by the review harness. These review, benchmark,
trace, and raster artifacts remain ignored and are not committed.

Repeat the complete render/package/visual/benchmark evidence after Unit 07D/#32.
