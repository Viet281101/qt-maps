# Progress Tracker

Update this file after every meaningful implementation change.

## Current Phase

- Context setup; application code has not started.

## Current Goal

- Complete unit 01, the environment and feasibility spike, after the owner approves an installation approach and build-system direction.

## Completed

- Surveyed the repository and converted the web-oriented context templates into QtMaps project documentation.
- Recorded the current compiler, build tools, GPU, Mesa, direct-rendering, and OpenGL baseline.
- Defined the first ten dependency-ordered units and detailed specs for units 01 and 02.

## In Progress

- Documentation review and owner decisions required before unit 01.

## Next Up

Each unit is intended to fit one to three four-hour sessions. Do not begin a unit until its dependencies and required owner approvals are satisfied.

1. **01 env-and-spike** — establish Qt 5.13, select the real build system, render a blank QML window and minimal raster map, capture an initial FPS/RAM baseline, and assess MapLibre Native feasibility.
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

### Required Before Unit 01

1. How should Qt 5.13 be installed when Ubuntu apt provides Qt 5.15: `aqtinstall`, the Qt online installer, or a source build? Unit 01 must compare reproducibility, required modules, disk/time cost, licensing/account requirements, and compatibility. Install nothing before owner approval.
2. Which primary build system should the project use: qmake or CMake? CMake is proposed because version 3.28.3 is already available, it integrates well with generated builds and future CI, and qmake is absent; however, actual Qt 5.13 and candidate map-engine integration must be checked before the owner decides. The build command is **TBD after unit 01**.
3. May Ninja and ccache be installed with `sudo` to reduce build time? Neither is currently available.
4. Which bounded geographic region and zoom range should be used for the first raster demo and later offline datasets?

### Technical and Product

5. Does Qt 5.13 compile cleanly with GCC 13.3.0, or is a compatible compiler/toolchain adjustment required?
6. Does Qt 5.13 networking work with the system OpenSSL 3 environment for HTTPS tile access, or is a supported compatibility arrangement required?
7. Can a maintained MapLibre Native Qt integration build and render reliably with Qt 5.13 on Intel HD Graphics 520, within the reference hardware budget?
8. Which legally usable orthography source should be selected, and what license, attribution, region, zoom range, and storage cap apply?
9. How complete and consistent are Vietnam OSM `ref` tags for national roads (QL) and provincial roads (DT/ĐT), and what routing behavior is acceptable where tags are missing?
10. Which machine will management use to view the demo, and how does it differ from the reference laptop?
11. Is a distributable installer/package required, or is a documented local Release build sufficient?
12. What exact UI theme token values, icon source, minimum window size, panel dimensions, keyboard shortcuts, contrast target, and map-aware light/dark behavior should be committed?

## Architecture Decisions

### Committed

- Use Qt 5.13 with C++ and QML/Qt Quick 2.13 on Linux. APIs introduced by Qt 5.15 or Qt 6 are forbidden.
- Preserve the repository's actual build system; none exists yet, so the owner will select qmake or CMake after unit 01 evidence.
- Support raster, vector, and orthography layers; offline-first operation; two-point car routing; simulated navigation; performance measurement; and weekly reporting.
- Measure FPS and combined RAM/CPU for the app and child services in Release mode with fixed scenarios.
- Manage routing/tile-server child processes through `QProcess` and keep routing/I/O off the UI thread.

### Proposed

- Prefer MapLibre Native with a Qt-compatible integration for vector rendering and navigation; use QtLocation `osm` as the raster path. MapLibre compatibility requires a one-day spike before commitment.
- Use Geofabrik Vietnam PBF, Planetiler-generated vector MBTiles, raster tiles, and either Martin/mbtileserver or direct `QSqlDatabase` MBTiles access.
- Use Valhalla locally for routing, maneuvers, and map matching, with car costing oriented toward `trunk`, `primary`, and `secondary` roads.
- Build offline geocoding with SQLite FTS5.
- Use C++14 as the project language level, subject to toolchain confirmation in unit 01.
- Use CMake as the primary build system because it is present and CI-friendly, subject to unit 01 integration evidence and owner approval.
- Planning performance thresholds are an average of at least 30 FPS, idle CPU at most 25%, interactive CPU at most 150%, and process-tree resident memory at most 2 GiB after 30 minutes on the reference laptop. Final thresholds wait for the baseline.

### Investigation Required

- Legal orthography source and bounded coverage.
- MapLibre Native compatibility with Qt 5.13 and the reference GPU/toolchain.
- Qt 5.13 compatibility with GCC 13 and system OpenSSL 3.
- Vietnam OSM national/provincial road-reference completeness.

## Environment Baseline

- Reference machine: Dell Latitude 5480; Intel Core i5-6300U (2 cores/4 threads); 16 GiB RAM; 512 GB NVMe SSD.
- Available: Linux, GCC/G++ 13.3.0, CMake 3.28.3, GNU Make 4.3.
- Not currently available on `PATH`: qmake/Qt, Ninja, Clang, ccache.
- Graphics: Intel HD Graphics 520 (Skylake GT2), Mesa 25.2.8, direct rendering enabled, OpenGL up to 4.6.

## Session Notes

- No application code, library installation, Qt build, dataset download, push, or merge was performed during context setup.
- `README.md` intentionally remains at the repository root with only the project title; no `docs/ai-playbook.md` exists because the original file was not a playbook.
- The exact build and test commands remain **TBD after unit 01**.
