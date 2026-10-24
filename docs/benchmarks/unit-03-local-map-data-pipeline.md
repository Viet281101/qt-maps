# Unit 03 Local Map Data Pipeline Evidence

Date: 2026-10-24. This report covers the bounded Hanoi Unit 03 package and
continuous simulated-navigation baseline. It does not claim real routing,
production raster coverage, nationwide coverage, or a final user interface.

## Package and reproducibility

`make hanoi-package` consumes only the previously approved local Geofabrik PBF
and Planetiler jar. Planetiler ran with `download=false`, `-Xmx6g`, and two
workers. The published artifact is ignored by Git at
`data/packages/hanoi-20261024-v1/`; `data/packages/current` is a relative
symlink to it.

| Property | Value |
| --- | --- |
| Schema/data/style contract | `qtmaps-map-package-v1` / `hanoi-20261024-v1` / `qtmaps-style-v1` |
| Bounds and zoom | `105.78,20.97,105.90,21.10`; z12-z15 |
| Layers | `building`, `place`, `transportation` |
| PBF SHA-256 | `4e5243c3116dc089a298f4cd7218e1257adf4b48355e42936360df5ce3200d43` |
| Planetiler v0.10.2 SHA-256 | `f310bd0413e2e4512b27f4046d418664e8e1d3bf31603c2a70e23de06c167e4d` |
| MBTiles | 226 tiles; 14,675,968 bytes; SHA-256 `70a3da3dde0cd7e60efb3e84656484a88e25070d16be9324fb418a935c5160d3` |
| First generation | 96.923 s; peak RSS 2,620,056 KiB |
| Reproduction | 73.687 s; peak RSS 3,080,044 KiB; all six content hashes matched |

The two manifests intentionally differ only in per-run generation duration and
peak RSS. MBTiles, style, two glyph ranges, and sprite JSON/PNG have identical
SHA-256 values. The package is atomically published only after generation and
resource hashing complete. The validator checks the exact schema/style/layer
contract, approved bounds/zooms and Planetiler digest, every file checksum,
glyph/sprite presence, source-layer set, source PBF identity, and attribution.

## Runtime and failure behavior

`MbTilesTileSource` owns a worker thread containing both its read-only
`QSqlDatabase` connection and `QTcpServer`. It binds only to a random
`127.0.0.1` port and serves health, style, glyph, sprite, and XYZ-to-TMS vector
tile requests. `TileSource` keeps the implementation replaceable. Qt Test
covers valid startup, stop/restart, random port service, requested-port
collision, missing resource, corrupt checksum, incompatible style contract,
missing layer, invalid bounds, fallback selection/reason, deterministic route
sampling, and destructor-driven clean shutdown.

The forced vector-failure run selected the local diagnostic raster, displayed
the failure reason, reported `mapReady=true`, and retained its prominent
non-geographic warning. It did not fetch public OSM tiles. A `strace -f -e
connect` run of the real vector probe found one IP target only:
`inet_addr("127.0.0.1")`.

The real-data runtime check on Intel HD Graphics 520/Mesa direct rendering
showed `HÀ NỘI`, roads, buildings, the project position marker, OSM attribution,
45-degree tilt, and bearing-follow behavior. Qt Quick used the threaded render
loop and a 16.66 ms vsync animation driver. MapLibre logs that its plugin does
not consider the threaded loop optimal; both the visual check and benchmark
nonetheless use it consistently.

## Continuous navigation benchmark

Reference host: Dell Latitude 5480, i5-6300U (2 cores/4 logical CPUs), 16 GiB
RAM, Intel HD Graphics 520, Mesa 25.2.8, Qt 5.15.13, dynamically linked
MapLibre Native Qt v3.0.0, Release, 960x640. CPU uses 100% for one logical core.
RSS includes the complete QtMaps process; the tile service is in-process and
there are no child processes.

The committed simulated street fixture is 5.47 km through central Hanoi. Each
run follows it at 40 km/h with a 16 ms position timer, rotating camera, zoom 15,
and tilt 45 degrees. The 120-second measured segment covers approximately 1.33
km; it is not a routing-engine result. Ten warm-up seconds are excluded. Cold
runs use three new `XDG_CACHE_HOME` directories. Warm runs share a separate
cache after a 20-second preload. Every final run has 3,600 retained interaction
frames and one separately reported settling swap.

| Cache/run | FPS avg | p50 ms | p95 ms | p99 ms | >100 ms | Mean CPU | Mean RSS MiB | Peak RSS MiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Cold 1 | 60.009 | 16.658 | 17.878 | 19.216 | 0 | 17.352% | 207.373 | 208.898 |
| Cold 2 | 60.009 | 16.640 | 17.895 | 19.112 | 0 | 16.691% | 208.749 | 210.688 |
| Cold 3 | 60.016 | 16.622 | 18.008 | 19.345 | 0 | 17.593% | 207.751 | 209.969 |
| **Cold median** | **60.009** | **16.640** | **17.895** | **19.216** | **0** | **17.352%** | **207.751** | **209.969** |
| Warm 1 | 60.011 | 16.658 | 18.048 | 19.674 | 0 | 16.407% | 205.590 | 207.969 |
| Warm 2 | 60.011 | 16.645 | 17.872 | 19.314 | 0 | 16.919% | 201.901 | 204.207 |
| Warm 3 | 60.010 | 16.646 | 17.793 | 18.999 | 0 | 16.499% | 198.508 | 200.336 |
| **Warm median** | **60.011** | **16.646** | **17.872** | **19.314** | **0** | **16.499%** | **201.901** | **204.207** |

Both cache states pass the proposed p50 <= 17 ms and p95 <= 33 ms review
thresholds; p99 remains report-only. CSV uses `qtmaps-perf-v3`, so the final
settling interval cannot distort the primary percentiles. Raw CSV, logs,
caches, screenshot, package, PBF, jar, and reproduction output remain ignored.
The benchmark binary identified the tree as `930f6495ac25+working-tree`; its
only source changes were the marker and shutdown-order fix committed immediately
afterward as `4e377ec` (the pre-existing user-owned VS Code change kept the tree
dirty).

## Licensing, limitations, and recommendation

The package and visible map retain `© OpenStreetMap contributors, ODbL 1.0`.
Planetiler v0.10.2 is Apache-2.0 build tooling and is not shipped. MapLibre core
is BSD-2-Clause; the Qt binding is dynamically linked under the selected
LGPL-3.0 option. Qt 5.15.13 is dynamically linked under the applicable LGPLv3
terms. Distribution obligations and replacement/relinking instructions remain
in `THIRD_PARTY_NOTICES.md` and `docs/packaging/linux-dynamic-linking.md`.

Unit 03 supports the approved Qt 5.15.13 + MapLibre baseline. The local
diagnostic raster is a mechanics-only fallback, not real Hanoi raster data.
Remaining risks are MapLibre deployment packaging/private-header overlay,
long-duration stability, explicit cache bounds, real raster generation, and
coverage beyond the bounded z12-z15 package. The next demo-critical unit should
be routing: approve Valhalla and its graph-build limits, then expose route and
maneuver models. Final route-planning/navigation UI depends on those backend
contracts; a non-final map shell can proceed, but polishing it first would not
close the two-point route demo gap.
