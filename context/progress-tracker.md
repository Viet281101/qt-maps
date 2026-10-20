# Progress Tracker

Update this file after every meaningful implementation change.

## Current Phase

- Unit 01 research; application code and installation have not started.

## Current Goal

- Inventory the Qt 5.13.2 modules available through `aqtinstall`, present all installation commands for approval/manual execution, then perform the environment and feasibility spike.

## Completed

- Surveyed the repository and converted the web-oriented context templates into QtMaps project documentation.
- Recorded the current compiler, build tools, GPU, Mesa, direct-rendering, and OpenGL baseline.
- Defined the first ten dependency-ordered units and detailed specs for units 01 and 02.
- Owner selected Qt 5.13.2 `gcc_64` through `aqtinstall` under `~/Qt` and committed the project to CMake with Ninja, subject to successful unit 01 validation.
- Owner selected Vietnam zoom 0-10 for national raster/vector coverage, Hanoi and the surrounding Red River Delta for zoom 11-16 detail, with zoom 16 explicitly provisional.
- Owner approved Ninja and ccache installation but required all `sudo apt` commands to be printed for manual execution.

## In Progress

- Read-only environment and Qt module research. No installation is authorized until the module list is presented and the owner approves proceeding.

## Next Up

Each unit is intended to fit one to three four-hour sessions. Do not begin a unit until its dependencies and required owner approvals are satisfied.

1. **01 env-and-spike** — establish Qt 5.13.2 `gcc_64` through `aqtinstall`, validate CMake with Ninja, render a blank QML window and minimal raster map, capture an initial FPS/RAM baseline, and assess MapLibre Native feasibility.
2. **02 perf-instrumentation** — implement the FPS overlay and CSV logger for total application-process-tree RAM/CPU with reproducible metadata.
3. **03 raster-map-offline** — make the raster layer usable from bounded local data with explicit online/offline state and fallback behavior.
4. **04 routing-service** — integrate a local proposed Valhalla process and return route geometry plus maneuvers without blocking the UI thread.
5. **05 route-on-map** — select endpoints and render the route, summary, and maneuver list on the raster map.
6. **06 navigation-mode** — replay simulated GPS/NMEA, follow and rotate the camera, advance maneuvers, and demonstrate re-routing.
7. **07 vector-layer** — integrate the vector path approved by unit 01 and render the route/navigation UI against local vector data.
8. **08 orthography-layer** — add a licensed, attributed orthography source for the approved demo region and zoom range.
9. **09 search** — build and query the proposed SQLite FTS5 offline geocoding index and connect results to map selection.
10. **10 packaging-and-final-benchmark** — package only if required, run all fixed Release scenarios, complete the soak test, and publish the final benchmark/report artifacts.

The order keeps measurement available before feature growth, establishes an offline raster fallback before dependent route UI, and validates navigation before introducing the higher-risk vector and orthography datasets. The MapLibre go/no-go investigation remains in unit 01 so unit 07 does not inherit an untested assumption.

## Open Questions

### Required Before Installation

1. Does `aqt list-qt linux desktop --modules 5.13.2 gcc_64` show the required QtLocation and QtPositioning modules, and what exact module names must be requested? The owner must review the output before approving the Qt download/install command.
2. After installation, are the QtLocation `osm` and `mapboxgl` plugins actually present and loadable, and which system libraries does `ldd` report missing?
3. Which routing-data scope should be used: Northern Vietnam or all of Vietnam?

### Technical and Product

4. Does Qt 5.13.2 compile cleanly with GCC 13.3.0, or is a compatible compiler/toolchain adjustment required?
5. Does Qt 5.13.2 networking work with the system OpenSSL 3 environment for HTTPS tile access, or is an OpenSSL 1.1 compatibility arrangement required? This affects online HTTPS only; offline localhost HTTP remains a separate viable path.
6. Can a maintained MapLibre Native Qt integration build and render reliably with Qt 5.13.2 on Intel HD Graphics 520, within the reference hardware budget?
7. Is zoom 16 practical for the detailed Hanoi/Red River Delta dataset after storage, generation-time, visual-quality, and performance measurements?
8. Which legally usable orthography source should be selected, and what license, attribution, region, zoom range, and storage cap apply?
9. How complete and consistent are Vietnam OSM `ref` tags for national roads (QL) and provincial roads (DT/ĐT), and what routing behavior is acceptable where tags are missing?
10. Which machine will management use to view the demo, and how does it differ from the reference laptop?
11. Is a distributable installer/package required, or is a documented local Release build sufficient?
12. What exact UI theme token values, icon source, minimum window size, panel dimensions, keyboard shortcuts, contrast target, and map-aware light/dark behavior should be committed?

## Architecture Decisions

### Committed

- Use Qt 5.13.2 with C++ and QML/Qt Quick 2.13 on Linux. APIs introduced by Qt 5.15 or Qt 6 are forbidden.
- Provision Qt 5.13.2 `gcc_64` through `aqtinstall` under `~/Qt`, but do not install it until the owner reviews the module list and approves the command.
- Use CMake with Ninja as the primary build system and revalidate the decision after unit 01. The exact working build command remains **TBD after unit 01**.
- Use Qt 5-compatible CMake APIs only; Qt 6 APIs such as `qt_add_executable`, `qt_add_qml_module`, and `qt_standard_project_setup` are forbidden.
- Use Vietnam zoom 0-10 for national raster/vector coverage and Hanoi plus the surrounding Red River Delta for zoom 11-16 detail. Zoom 16 remains an explicit initial assumption pending evidence. Routing coverage is Northern Vietnam or all of Vietnam, with the final choice still open.
- Support raster, vector, and orthography layers; offline-first operation; two-point car routing; simulated navigation; performance measurement; and weekly reporting.
- Measure FPS and combined RAM/CPU for the app and child services in Release mode with fixed scenarios.
- Manage routing/tile-server child processes through `QProcess` and keep routing/I/O off the UI thread.

### Proposed

- Prefer MapLibre Native with a Qt-compatible integration for vector rendering and navigation; use QtLocation `osm` as the raster path. MapLibre compatibility requires a one-day spike before commitment.
- Use Geofabrik Vietnam PBF, Planetiler-generated vector MBTiles, raster tiles, and either Martin/mbtileserver or direct `QSqlDatabase` MBTiles access.
- Use Valhalla locally for routing, maneuvers, and map matching, with car costing oriented toward `trunk`, `primary`, and `secondary` roads.
- Build offline geocoding with SQLite FTS5.
- Use C++14 as the project language level, subject to toolchain confirmation in unit 01.
- Planning performance thresholds are an average of at least 30 FPS, idle CPU at most 25%, interactive CPU at most 150%, and process-tree resident memory at most 2 GiB after 30 minutes on the reference laptop. Final thresholds wait for the baseline.

### Investigation Required

- Legal orthography source and bounded coverage.
- MapLibre Native compatibility with Qt 5.13.2 and the reference GPU/toolchain.
- Qt 5.13.2 compatibility with GCC 13 and system OpenSSL 3 for online HTTPS; offline localhost HTTP is not blocked by this TLS question.
- Vietnam OSM national/provincial road-reference completeness.

## Environment Baseline

- Reference machine: Dell Latitude 5480; Intel Core i5-6300U (2 cores/4 threads); 16 GiB RAM; 512 GB NVMe SSD.
- Available: Linux, GCC/G++ 13.3.0, CMake 3.28.3, GNU Make 4.3.
- Not currently available on `PATH`: qmake/Qt, Ninja, Clang, ccache. Ninja and ccache are approved for owner-executed installation.
- Graphics: Intel HD Graphics 520 (Skylake GT2), Mesa 25.2.8, direct rendering enabled, OpenGL up to 4.6.

## Session Notes

- No application code, library installation, Qt build, dataset download, push, or merge was performed during context setup.
- `README.md` intentionally remains at the repository root with only the project title; no `docs/ai-playbook.md` exists because the original file was not a playbook.
- The exact build and test commands remain **TBD after unit 01**.
