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

### Fully disconnected Release verification

PR #20 was rechecked with the Release binary in a new user/network namespace.
Only the loopback interface was brought up; the namespace had no default route
or non-loopback interface. The successful command was:

```bash
unshare -Urn bash -c '
set -euo pipefail
ip link set lo up
export LD_LIBRARY_PATH="$PWD/generated/unit-01b/maplibre-install/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$PWD/generated/unit-01b/maplibre-install/plugins:$PWD/generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
export QML2_IMPORT_PATH="$PWD/generated/unit-01b/maplibre-install/qml:$PWD/generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5/qml${QML2_IMPORT_PATH:+:$QML2_IMPORT_PATH}"
export QSG_RENDER_LOOP=threaded
strace -f -e trace=connect -s 256 -o /tmp/qtmaps-pr20-offline-connect.strace \
  ./build-qt515/qtmaps --unit03-local-map --navigation-benchmark \
  --map-package data/packages/current --exit-after 15 \
  --network-mode network-namespace-offline \
  --dataset-id hanoi-20261024-v1 \
  --dataset-coverage "Hanoi bounds 105.78,20.97,105.90,21.10; z12-z15"
'
```

The app reported `mapReady=true`, error 0, and Mesa Intel HD Graphics 520. A
captured 960x640 frame showed the bounded Hanoi vector map with Vietnamese
`HÀ NỘI` text, roads, building footprints, the simulated-position marker, and
the visible `© OpenStreetMap contributors, ODbL 1.0` attribution. The trace
contained AF_INET connections only to the in-process service at
`127.0.0.1:<random-port>`; AF_UNIX connections were desktop IPC. The package
style contains no `http://`, `https://`, public tile-server URL, or remote
source URL. Its tile, glyph, and sprite entries all use the
`__QTMAPS_BASE_URL__` token, which the validated service replaces with its
random loopback endpoint.

## Continuous navigation benchmark

Reference host: Dell Latitude 5480, i5-6300U (2 cores/4 logical CPUs), 16 GiB
RAM, Intel HD Graphics 520, Mesa 25.2.8, Qt 5.15.13, dynamically linked
MapLibre Native Qt v3.0.0, Release, 960x640. CPU uses 100% for one logical core.
RSS includes the complete QtMaps process; the tile service is in-process and
there are no child processes. Therefore the current CPU/RSS figures cover the
Qt application and its in-process tile service, but no external service. When
Valhalla is added, the benchmark must be repeated over the full process tree so
the routing child process is included.

The committed simulated street fixture is 5.47 km through central Hanoi. Each
run follows it at 40 km/h with a 16 ms position timer, rotating camera, zoom 15,
and tilt 45 degrees. The 120-second measured segment covers approximately 1.33
km; it is not a routing-engine result. Ten warm-up seconds are excluded. Cold
runs use three new `XDG_CACHE_HOME` directories. Warm runs share a separate
cache after a 20-second preload. Final runs retain 7,168-7,179 interaction
frames (the complete approximately 119.5-second post-warm-up window) and one
separately reported settling swap. An earlier 3,600-frame result set was
rejected during commit review because the original buffer retained only the
last approximately 60 seconds; those raw files remain ignored historical
diagnostics and are not used below.

| Cache/run | FPS avg | p50 ms | p95 ms | p99 ms | >100 ms | Mean CPU | Mean RSS MiB | Peak RSS MiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Cold 1 | 60.011 | 16.642 | 18.284 | 19.811 | 0 | 17.548% | 208.934 | 211.480 |
| Cold 2 | 60.011 | 16.642 | 17.943 | 19.318 | 0 | 17.569% | 209.044 | 211.562 |
| Cold 3 | 60.011 | 16.645 | 17.969 | 19.322 | 0 | 17.575% | 207.706 | 209.496 |
| **Cold median** | **60.011** | **16.642** | **17.969** | **19.322** | **0** | **17.569%** | **208.934** | **211.480** |
| Warm 1 | 60.010 | 16.641 | 17.837 | 19.138 | 0 | 16.850% | 203.677 | 206.199 |
| Warm 2 | 60.012 | 16.648 | 17.944 | 19.248 | 0 | 17.102% | 202.294 | 204.152 |
| Warm 3 | 60.012 | 16.646 | 17.922 | 19.249 | 0 | 17.451% | 199.132 | 201.363 |
| **Warm median** | **60.012** | **16.646** | **17.922** | **19.248** | **0** | **17.102%** | **202.294** | **204.152** |

Both cache states pass the proposed p50 <= 17 ms and p95 <= 33 ms review
thresholds; p99 remains report-only. CSV uses `qtmaps-perf-v3`, so the final
settling interval cannot distort the primary percentiles. Raw CSV, logs,
caches, screenshot, package, PBF, jar, and reproduction output remain ignored.
The approximately 60 FPS result is limited by the 60 Hz vsync path; it is not
evidence of unused rendering capacity. CPU and RSS are the primary capacity
indicators for this baseline, with frame-time percentiles used to detect
interaction stalls.
The reviewed benchmark binary identified the tree as
`6e26a48bdb30+working-tree`; `6e26a48` contains the full-window buffer and
offline-contract fixes. The suffix is caused by the pre-existing user-owned VS
Code settings change, which was excluded from every commit.

## Licensing, limitations, and recommendation

The package and visible map retain `© OpenStreetMap contributors, ODbL 1.0`.
Planetiler v0.10.2 is Apache-2.0 build tooling and is not shipped. MapLibre core
is BSD-2-Clause; the Qt binding is dynamically linked under the selected
LGPL-3.0 option. Qt 5.15.13 is dynamically linked under the applicable LGPLv3
terms. Distribution obligations and replacement/relinking instructions remain
in `THIRD_PARTY_NOTICES.md` and `docs/packaging/linux-dynamic-linking.md`.

Unit 03 supports the approved Qt 5.15.13 + MapLibre baseline. The local
diagnostic raster is a mechanics-only fallback, not real Hanoi raster data.
Development tests currently select the Qt 5.15 QSQLITE plugin from the ignored
Unit 01b runtime tree through `QT_RUNTIME_PLUGIN_PATH`; `make test` now fails
early with an actionable message when that plugin root is absent. A release
bundle must include the matching `plugins/sqldrivers/libqsqlite.so`.
Remaining risks are MapLibre deployment packaging/private-header overlay,
long-duration stability, explicit cache bounds, real raster generation, and
coverage beyond the bounded z12-z15 package. The next demo-critical unit should
be routing: approve Valhalla and its graph-build limits, then expose route and
maneuver models. Final route-planning/navigation UI depends on those backend
contracts; a non-final map shell can proceed, but polishing it first would not
close the two-point route demo gap.
