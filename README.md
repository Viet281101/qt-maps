# QtMaps

QtMaps is a Linux desktop mapping application for Vietnam built with Qt 5.15.13, dynamically linked MapLibre Native Qt v3.0.0, C++, and QML/Qt Quick. Qt 5.13.2 is retained for comparison evidence. The project focuses on local and offline operation, car routing, and a navigation experience inspired by familiar modern mapping applications.

## Project Requirements

- Render a digital map of Vietnam using OpenStreetMap data.
- Support raster tiles, vector maps, and orthography layers where suitable licensed data is available.
- Operate with or without an Internet connection after the required local datasets have been provisioned.
- Calculate a car route between two points, preferring suitable national and provincial road classes when supported by the underlying OSM data.
- Display route geometry, a trip summary, and turn-by-turn maneuvers.
- Provide a simulated navigation mode with camera following, heading-based rotation, a maneuver banner, and re-routing.
- Measure rendered FPS and total RAM and CPU usage for the application and its child services in a Release build.
- Track progress and publish weekly reports through GitHub Projects and repository documentation.

Voice guidance, real-time traffic, mobile and web delivery, user accounts, and production-grade hosted services are outside the current scope.

## First Demo Goal

The first management demo is due on **November 3, 2026**. During the two-week preparation window, the project prioritizes a stable, visually coherent end-to-end demo:

1. Start a Release build and display a real map centered on the agreed Vietnam demo region.
2. Present a clear map-first interface with online/offline, layer, route, and performance states.
3. Select two points, calculate and draw a car route, and display its summary and maneuvers.
4. Enter a navigation-style view, replay simulated movement, and show the next maneuver.
5. Display FPS and provide reproducible CPU and RAM measurements from the reference laptop.
6. Add offline fallback, vector rendering, orthography, and search in that priority order once the core flow is stable.

Nationwide high-detail coverage, a complete data pipeline, installer packaging, long soak tests, and production hardening may be deferred when they threaten the core demo. Features backed by simulation, fixtures, the Internet, limited geographic coverage, or incomplete integrations must be identified accurately during the demonstration.

## Technical Direction

| Component           | Current direction                                                     |
| ------------------- | --------------------------------------------------------------------- |
| UI and runtime      | Qt 5.15.13, MapLibre Native Qt v3.0.0, C++, QML, and Qt Quick          |
| Build system        | CMake and Ninja using Qt 5 CMake APIs                                 |
| Raster rendering    | Diagnostic localhost fallback validated; real raster data deferred    |
| Vector rendering    | Approved MapLibre Native Qt v3.0.0 over validated local MBTiles       |
| Map data            | Versioned Hanoi package from the approved Geofabrik PBF/Planetiler    |
| Routing             | A local Valhalla child process is the proposed approach               |
| Offline search      | A project-built SQLite FTS5 index is the proposed approach            |
| Position simulation | QtPositioning with route or NMEA replay                               |
| Performance         | Rendered-frame FPS and process-tree CPU/RAM measurements              |

A proposed technology is not committed until it has been built and exercised successfully with Qt 5.15.13 on the reference machine.

## Build and run

The validated Release workflow selects system Qt 5.15.13 explicitly and does not change the global `PATH`:

```bash
make requirements
make build
make test
```

The current MapLibre development install and Qt QML/runtime plugins are ignored local dependencies. This successful local smoke command runs the checksummed Hanoi package for 15 seconds; the Unit 03 evidence wraps it in the network namespace used for the disconnected check:

```bash
LD_LIBRARY_PATH="$PWD/generated/unit-01b/maplibre-install/lib" \
QT_PLUGIN_PATH="$PWD/generated/unit-01b/maplibre-install/plugins:$PWD/generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5/plugins" \
QML2_IMPORT_PATH="$PWD/generated/unit-01b/maplibre-install/qml:$PWD/generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5/qml" \
QSG_RENDER_LOOP=threaded \
./build-qt515/qtmaps --unit03-local-map --navigation-benchmark \
  --map-package data/packages/current --exit-after 15 \
  --network-mode network-namespace-offline \
  --dataset-id hanoi-20261024-v1 \
  --dataset-coverage "Hanoi bounds 105.78,20.97,105.90,21.10; z12-z15"
```

The recorded 120-second benchmark uses the same command and runtime variables, replacing `--exit-after 15` with:

```bash
--benchmark-output /tmp/qtmaps-unit03.csv \
--benchmark-warmup 10 --benchmark-duration 120 --benchmark-interval 1000 \
--scenario unit03-navigation
```

Recreate the ignored, versioned package only from the approved local inputs:

```bash
make hanoi-package \
  HANOI_PBF=generated/unit-01b/source/vietnam-latest.osm.pbf \
  PLANETILER_JAR=generated/unit-01b/tools/planetiler.jar
```

Exact disconnected verification and benchmark procedures are recorded in [the Unit 03 evidence](docs/benchmarks/unit-03-local-map-data-pipeline.md).

## Current status and limitations

- Qt 5.15.13 plus dynamically linked MapLibre Native Qt v3.0.0 is the production baseline. Qt 5.13.2 is retained only for explicit comparison.
- The checksummed local vector package currently covers bounded Hanoi data at zoom 12–15. It is not nationwide coverage.
- PR #20 is merged. Its visible 5.47 km path and 40 km/h movement remain deterministic Unit 03 navigation fixtures, not output from a routing engine.
- The Unit 04 route-planning flow now runs a managed, loopback-only Valhalla 3.9.1 service and returns real route geometry, distance, duration, and Vietnamese maneuvers for the bounded greater-Hanoi graph. Recorded fixtures remain an explicitly labeled fallback.
- Raster fallback is a local diagnostic image that proves failure detection and switching; it is not real Hanoi raster coverage.
- Navigation replay over the selected route, re-routing, full QtMaps-plus-Valhalla process-tree benchmarking, nationwide graph generation, offline search, orthography, final product UI, installer packaging, and long-duration stability validation are not implemented yet.

## Implementation Principles

- Use only APIs compatible with Qt 5.15.13; do not use Qt 6-only APIs.
- CMake must use `find_package(Qt5 ...)`, `CMAKE_AUTOMOC`/`CMAKE_AUTORCC`, `qt5_add_resources`, and `add_executable`.
- Never perform blocking network, routing, database, filesystem, or process waits on the UI thread.
- Do not commit generated datasets, caches, routing graphs, MBTiles, or third-party artifacts to Git.
- Management-facing results must come from a Release build and state the process scope, dataset, duration, viewport, and network mode.
- Complete a stable vertical slice before expanding feature breadth.

## Planned Data Coverage

- Nationwide raster and vector coverage at zoom levels 0–10.
- Hanoi and the surrounding Red River Delta at zoom levels 11–16; zoom 16 remains subject to storage and performance evidence.
- Initial routing coverage from one uncropped Geofabrik Vietnam extract, to be reassessed using graph-build time and peak RAM measurements.
- Orthography limited to an approved demo region and zoom range using a suitably licensed source.

## Initial Performance Targets

These values are planning targets, not measured results:

- At least 30 average FPS during interactive scenarios.
- Idle CPU at or below 25% of one logical CPU.
- Interactive CPU at or below 150% under Linux process-level reporting.
- Combined resident memory for QtMaps and its child services at or below 2 GiB after 30 minutes.

Actual results are recorded using [the benchmark template](docs/benchmarks/benchmark-template.md).

## Repository Structure

```text
app/                 Entry point and lifecycle
qml/                 Screens, controls, overlays, and theme
src/core/            Domain types, configuration, and lifecycle abstractions
src/map/             Map engines, layers, and tile access
src/routing/         Routing client, route models, and polyline decoder
src/search/          Offline search and models
src/perf/            FPS, process metrics, and CSV logging
tools/data-pipeline/ Reproducible data preparation
tests/               Qt Test and Qt Quick Test suites
docs/                Benchmarks and weekly reports
context/             Authoritative scope and project decisions
```

Qt 5.13.2 comparison and MapLibre runtime-path details are documented in [the code standards](context/code-standards.md) and [Unit 01b evidence](docs/benchmarks/unit-01b-vector-feasibility.md).

Third-party attribution and the approved dynamic-link distribution plan are recorded in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [the Linux packaging guide](docs/packaging/linux-dynamic-linking.md).
