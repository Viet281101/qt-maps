# Architecture Context

## Decision Status Vocabulary

- **Committed**: explicitly required or approved and safe to implement against.
- **Proposed**: a current direction that still requires owner approval or implementation evidence.
- **Spike required**: feasibility must be established before committing to the choice.

## Stack

| Layer | Technology | Status | Role |
| --- | --- | --- | --- |
| Platform | Linux desktop | Committed | Development and target platform |
| UI/runtime | Qt 5.13.2, QML, Qt Quick 2.13 | Committed | Application lifecycle and user interface |
| Language | C++ with C++14 as the proposed project level | Qt/C++ committed; language level proposed | Native application and service integration |
| Build system | CMake with Ninja | Committed; minimal Release path validated, revalidate after unit 01 | Primary configure/generate/build path; exact commands are recorded in `progress-tracker.md` and `code-standards.md` |
| Raster map | QtLocation `osm` plugin if present and compatible | Proposed; verify in unit 01 | Minimal raster display and fallback path |
| Vector map | MapLibre Native with a Qt-compatible integration | Proposed; spike required | Vector rendering and navigation presentation |
| Map source data | Geofabrik Vietnam OSM PBF | Proposed | Input for map, routing, and search datasets |
| Vector pipeline | Planetiler to MBTiles | Proposed | Generate vector tiles |
| Tile access | Direct MBTiles through `QSqlDatabase`, or Martin/mbtileserver child process | Proposed | Serve or read local tiles |
| Routing | Valhalla child process | Proposed | Offline route, maneuvers, and map matching |
| Search | SQLite FTS5 index | Proposed | Offline geocoding/search |
| Position simulation | QtPositioning and route/NMEA replay | Proposed | Simulated GPS for navigation |
| Performance | Frame signal instrumentation plus CSV process logger | Committed capability; implementation pending | Capture FPS, RAM, and CPU for the app process tree |

No engine or library marked Proposed or Spike required becomes committed merely by appearing in this table.

## System Boundaries

- `app/` — application entry point, startup/shutdown coordination, and top-level dependency wiring.
- `qml/` — QML screens, overlays, controls, and the theme singleton; no blocking I/O or routing work.
- `src/core/` — shared domain types, configuration, error types, and lifecycle abstractions.
- `src/map/` — map-engine adapters, layer selection, tile access, and map-facing models.
- `src/routing/` — routing process client, request/response translation, route models, and polyline decoding.
- `src/search/` — offline index access and search models.
- `src/perf/` — frame metrics, process-tree sampling, CSV output, and benchmark metadata.
- `tools/data-pipeline/` — reproducible commands and scripts that transform source datasets; generated data does not belong in source control.
- `tests/` — Qt Test and Qt Quick Test suites and their fixtures.
- `docs/benchmarks/` — reference hardware and benchmark procedure/result templates.
- `docs/reports/` — weekly report templates and completed reports.
- `context/` — product, architecture, standards, workflow, specs, and current project state.

These directories describe intended ownership. Application-code directories are created only when their implementation unit begins.

## Storage Model

- **MBTiles**: proposed container for local vector and raster tiles. Direct SQLite access versus a local server remains an implementation decision.
- **Routing graph**: generated Valhalla graph tiles, versioned by source extract and build configuration outside Git.
- **Geocoding database**: proposed SQLite database with FTS5 tables generated from selected source data.
- **Orthography files or tiles**: bounded by the agreed region, zoom range, license, and attribution requirements.
- **Disk cache**: bounded cache for fetched or decoded tiles; limits and invalidation policy must be measured and documented before commitment.
- **Configuration and logs**: user-writable configuration, operational logs, and benchmark CSV files stored outside packaged read-only assets.

Large generated datasets, caches, routing graphs, and third-party artifacts must not be committed to Git.

## Services and Process Model

- QtMaps owns the lifecycle of any routing or tile-server child process through `QProcess`.
- Child services start only when required, expose readiness/failure state, and shut down when the application exits.
- Startup must use bounded timeouts and actionable errors; it must never wait indefinitely on the UI thread.
- Routing, tile I/O, search, process monitoring, and service output parsing must not block the UI thread.
- Service endpoints default to loopback-only access unless a later requirement explicitly changes this boundary.
- Thread and worker counts must be configurable and conservatively limited for the two-core/four-thread reference CPU.
- Performance totals include the main process and every child service started by it.

## Invariants

1. Production code must not require an API introduced after Qt 5.13, and QML imports must not declare a version newer than 2.13 for Qt Quick modules.
2. The UI thread must never perform blocking routing, network, database, filesystem, or child-process waits.
3. The agreed demo must operate fully offline after its required datasets have been provisioned.
4. Only Release builds may produce performance results presented as project benchmarks.
5. In-memory tile and decoded-resource caches must have explicit, testable bounds.
6. The application must own and cleanly terminate the child services it starts.
7. Generated data, vendor internals, and third-party artifacts are not hand-edited as application source.
8. Architecture entries retain their decision status until evidence is recorded and the project owner approves a change.
9. Build files must use Qt 5 CMake integration only: `find_package(Qt5 ...)`, `CMAKE_AUTOMOC`/`CMAKE_AUTORCC`, `qt5_add_resources`, and `add_executable`. Qt 6-only APIs such as `qt_add_executable`, `qt_add_qml_module`, and `qt_standard_project_setup` are forbidden.

## Environment Provisioning Decisions

- Provision Qt 5.13.2 `gcc_64` with `aqtinstall` under `~/Qt`; this method, version, architecture, and destination were selected by the project owner.
- Before installing Qt, run `aqt list-qt linux desktop --modules 5.13.2 gcc_64`, present the module list to the owner, and wait for explicit installation approval.
- Qt Quick, QtLocation, QtPositioning, and the `osm`/`mapboxgl` geoservice plugins remain unverified until the downloaded installation is inspected and loaded.
- After installation, use `ldd` to identify missing system libraries rather than assuming the archive is self-contained.
- Qt 5.13.2 compatibility with OpenSSL 1.1 versus the host's OpenSSL 3 remains a spike question for online HTTPS. Offline localhost HTTP does not require TLS, so this risk does not by itself block the offline path.
- The owner approved installation of Ninja and ccache, but the agent must only print the required `sudo apt` command; the owner runs it manually.

## Current Environment Baseline

- Linux development machine: Dell Latitude 5480, Intel Core i5-6300U, 16 GiB RAM, and 512 GB NVMe SSD.
- GCC/G++ 13.3.0, CMake 3.28.3, and GNU Make 4.3 are available.
- qmake/Qt and Clang are not available on `PATH`; Qt is intentionally addressed by absolute path or `CMAKE_PREFIX_PATH`. Owner-installed Ninja 1.11.1 and ccache 4.9.1 are available on `PATH`.
- Intel HD Graphics 520 (Skylake GT2), Mesa 25.2.8, direct rendering enabled, and OpenGL up to 4.6.
- Qt 5.13.2 installation under `/home/viet/Qt/5.13.2/gcc_64` is complete and owner-approved. The owner manually installed Ninja and ccache; no agent-run `sudo` command was used.
