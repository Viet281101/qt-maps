# Progress Tracker

Update this file after every meaningful implementation change.

## Current Phase

- Unit 01 environment and feasibility spike on `feat/01-env-and-spike`; validate the Release CMake/Ninja path and the smallest Qt Quick/QtLocation proofs before expanding the demo.

## Current Goal

- Deliver the strongest stable management demo possible by 2026-11-03: a polished map-first Release application, a visible two-point route and navigation-style flow, and honest FPS/RAM/CPU evidence on the reference laptop.

## Completed

- Surveyed the repository and converted the web-oriented context templates into QtMaps project documentation.
- Recorded the current compiler, build tools, GPU, Mesa, direct-rendering, and OpenGL baseline.
- Defined the first ten dependency-ordered units and detailed specs for units 01 and 02.
- Owner selected Qt 5.13.2 `gcc_64` through `aqtinstall` under `~/Qt` and committed the project to CMake with Ninja, subject to successful unit 01 validation.
- Owner selected Vietnam zoom 0-10 for national raster/vector coverage, Hanoi and the surrounding Red River Delta for zoom 11-16 detail, with zoom 16 explicitly provisional.
- Owner approved Ninja and ccache installation but required all `sudo apt` commands to be printed for manual execution.
- Installed `aqtinstall` 3.3.0 in `~/.local/share/aqtinstall-3.3.0`.
- Queried Qt 5.13.2 `gcc_64` successfully: the extra-module list matched the repository-derived preliminary list, and the base archives include `qtdeclarative` and `qtlocation`.
- Owner selected nationwide routing data from one uncropped Geofabrik Vietnam extract as the initial scope, to be confirmed by graph-build time and peak-RAM measurements.
- Installed the Qt 5.13.2 `gcc_64` base archives under `/home/viet/Qt/5.13.2/gcc_64`; `/home/viet/Qt` uses 563 MiB as reported by `du -sh`.
- Confirmed qmake 3.1 reports Qt 5.13.2, and confirmed the QtLocation and QtPositioning libraries and QML import directories exist.
- Confirmed geoservice plugins `osm` and `mapboxgl`, plus the position plugin `serialnmea`, are present on disk.
- Ran `ldd` on Qt Core, Quick, Location, Positioning, and the xcb platform plugin; none reported a `not found` dependency.
- Built and ran a temporary non-application SSL probe outside the repository with GCC 13.3.0. `QSslSocket::supportsSsl()` returned true, but Qt reported unresolved `EVP_PKEY_base_id` and `SSL_get_peer_certificate` symbols when using OpenSSL 3.0.13.
- Owner set a two-week deadline ending 2026-11-03 for the first management demo and directed work to prioritize the most convincing stable vertical slice over the previously estimated long-form sequence.
- Updated the project overview, AI workflow, progress tracker, and bilingual root documentation to describe the demo-first direction and distinguish complete, partial, simulated, and unavailable behavior. `README.md` is the professional English entry point and `README_VN.md` contains the Vietnamese version.
- Linked the public [QtMaps Project board](https://github.com/users/Viet281101/projects/6) and repository in both directions, set the repository homepage to the board, and added Qt/QML/OSM/offline-map/routing topics for discoverability.
- Created the [First Management Demo milestone](https://github.com/Viet281101/qt-maps/milestone/1), the [demo tracking issue](https://github.com/Viet281101/qt-maps/issues/1), eleven scoped sub-issues with acceptance criteria, P0/P1 labels, area labels, and Project fields for Priority and Demo Phase.
- Opened [PR #17](https://github.com/Viet281101/qt-maps/pull/17) to review the complete context and demo-first documentation baseline before merging it into `main`.
- Merged the context baseline from PR #17 into `main` and created `feat/01-env-and-spike` from that merge.
- Owner deferred Qt 5.13.2 HTTPS compatibility work as a low-priority open risk that does not block unit 01. Until revisited, tile and routing endpoints are assumed to use HTTP on localhost. Possible later approaches are a local tile proxy that fetches HTTPS upstream and serves HTTP locally, or, only as a last resort, a private OpenSSL 1.1 runtime because OpenSSL 1.1 is end-of-life. Do not modify system OpenSSL or install `libssl1.1`.
- Ran `ldd` on the installed `mapboxgl` and `osm` geoservice plugins and the `serialnmea` position plugin. None produced a `not found` line, so this check does not justify any runtime-library installation command.
- Confirmed the owner-installed build tools are available: Ninja 1.11.1 and ccache 4.9.1.
- Added the minimal Qt 5 CMake project and completed a Release build with GCC 13.3.0, Ninja 1.11.1, ccache 4.9.1, and C++14. The QML engine loaded a 960x640 Qt Quick 2.13 window, and `xwininfo` confirmed the X11 window existed.
- `QGeoServiceProvider::availableServiceProviders()` returned `mapboxgl`, `here`, `osm`, `itemsoverlay`, `esri`, and `mapbox` (order varied between runs). Constructing `osm` and `mapboxgl` with experimental providers allowed returned a non-null mapping manager for both; provider and mapping errors were zero with empty error strings. This proves provider loading only, not raster or vector rendering.
- Verified against the QtLocation `v5.13.2` source that the OSM plugin parameter is `osm.mapping.custom.host`; the plugin appends `%z/%x/%y.png` to that base URL. The same source confirms `osm.mapping.providersrepository.disabled`, `osm.mapping.prefetching_style=NoPrefetching`, and `osm.useragent`.
- Reviewed the current OpenStreetMap Foundation standard raster-tile policy. It requires HTTPS and prohibits pre-seeding/bulk downloads and offline tile archives, so `tile.openstreetmap.org` will not be used as the source of a curl-downloaded localhost fixture. No tile was downloaded or generated during this research step.
- Owner approved a locally generated diagnostic XYZ fixture for unit 01 and prohibited downloading tiles from `tile.openstreetmap.org` or any other public tile server in all cases, including small tests. Real map content is reserved for the unit 03 project pipeline using Geofabrik PBF, Planetiler, and a localhost tile server. The unit 01 fixture must be labeled as non-geographic diagnostic content.
- Added `tools/generate_diagnostic_tiles.py`, which uses only Python standard-library `math`, `struct`, and `zlib` support and performs no network access. It applies the documented Web Mercator XYZ formula around Hanoi and generated 25 bordered, coordinate-labeled diagnostic tiles at each zoom from 10 through 14 (125 PNG files, 624 KiB total). The output under `generated/diagnostic-tiles/` is ignored and not committed; it is not real geographic map data.

## In Progress

- [Issue #2](https://github.com/Viet281101/qt-maps/issues/2), Qt 5.13 Release build and raster-map validation, is the active delivery item. The Release CMake/Ninja Qt Quick and provider probe passes. The owner must now approve either the proposed locally generated diagnostic XYZ fixture or another explicitly licensed raster source before the localhost raster implementation continues.

## Next Up

Before the first demo, execute the shortest stable vertical slice. Existing unit specifications remain authoritative for their completed implementations, but their full completion gates may be deferred and recorded during this milestone.

1. **Render the first raster proof** — after owner approval, preserve the validated Release CMake/Ninja path and use a policy-compliant, tightly bounded tile fixture through HTTP localhost. The proposed first fixture is a locally generated XYZ coordinate grid, explicitly labeled as diagnostic rather than real map data.
2. **Create the demo shell** — render the first real raster map and add a coherent map-first QML layout with status, route, navigation, layer, and performance presentation areas.
3. **Complete the route vertical slice** — select two points, obtain a real route through the fastest viable approved integration, draw it, and show its summary and maneuvers. Clearly label any bounded fixture or online dependency used for the first demo.
4. **Add navigation presentation** — replay a route or simulated position, follow the current position, show the next maneuver, and expose an obvious exit/recenter action.
5. **Measure the demo** — show rendered-frame FPS and capture short Release-build CPU/RAM results for the application process tree with documented scope.
6. **Add breadth without destabilizing the flow** — attempt offline fallback, vector, orthography, and search in that priority only after the end-to-end demo remains reproducible.
7. **Freeze and rehearse** — fix demo blockers, prepare a deterministic scenario and fallback evidence, update the weekly report, and avoid risky dependency changes immediately before the demonstration.

After the demo, resume the ten-unit plan and close every deferred verification gate rather than treating the demonstration slice as production completion.

## Open Questions

### Environment Validation

1. The `osm` and `mapboxgl` providers load with non-null mapping managers and no provider/mapping error. Can `osm` render the localhost raster proof, and what style/URL requirements prevent or permit a bounded `mapboxgl` probe?
2. **Deferred, low priority; does not block unit 01:** Does Qt 5.13.2 complete a real HTTPS request reliably with OpenSSL 3 despite the two unresolved-symbol warnings? The current spike assumes HTTP localhost for every tile and routing endpoint. Later options are (a) a local tile server that fetches HTTPS upstream and re-serves over HTTP localhost, or (b) a private OpenSSL 1.1 runtime only as a last resort because OpenSSL 1.1 is end-of-life. Do not modify OpenSSL or install `libssl1.1` while this question is deferred.
3. What graph-build-time and peak-RAM thresholds should trigger reconsideration of the initial nationwide routing scope?
4. Does the owner approve a locally generated diagnostic XYZ tile grid for the first localhost raster mechanics proof, knowing that it verifies rendering/pan/zoom but is not real geographic map content, or should the owner provide/approve a raster provider whose license explicitly permits the bounded local fixture?

### Technical and Product

5. **Confirmed for the minimal provider probe:** Qt Quick/QtLocation compiles and runs with GCC 13.3.0 in Release mode. Raster rendering remains to be verified separately.
6. Can a maintained MapLibre Native Qt integration build and render reliably with Qt 5.13.2 on Intel HD Graphics 520, within the reference hardware budget?
7. Is zoom 16 practical for the detailed Hanoi/Red River Delta dataset after storage, generation-time, visual-quality, and performance measurements?
8. Which legally usable orthography source should be selected, and what license, attribution, region, zoom range, and storage cap apply?
9. How complete and consistent are Vietnam OSM `ref` tags for national roads (QL) and provincial roads (DT/ĐT), and what routing behavior is acceptable where tags are missing?
10. Which machine will management use to view the demo, and how does it differ from the reference laptop?
11. Is a distributable installer/package required, or is a documented local Release build sufficient?
12. What exact UI theme token values, icon source, minimum window size, panel dimensions, keyboard shortcuts, contrast target, and map-aware light/dark behavior should be committed?

## Architecture Decisions

### Committed

- Target the first management demo for 2026-11-03 and prioritize a stable, visually coherent vertical slice with measured performance until that date.
- Use Qt 5.13.2 with C++ and QML/Qt Quick 2.13 on Linux. APIs introduced by Qt 5.15 or Qt 6 are forbidden.
- Provision Qt 5.13.2 `gcc_64` through `aqtinstall` under `~/Qt`; the approved installation completed successfully.
- Use CMake with Ninja as the primary build system and revalidate the decision after unit 01. The validated Release commands are recorded under Unit 01 Working Commands.
- Use Qt 5-compatible CMake APIs only; Qt 6 APIs such as `qt_add_executable`, `qt_add_qml_module`, and `qt_standard_project_setup` are forbidden.
- Use Vietnam zoom 0-10 for national raster/vector coverage and Hanoi plus the surrounding Red River Delta for zoom 11-16 detail. Zoom 16 remains an explicit initial assumption pending evidence.
- Initially build routing data for all of Vietnam from one uncropped Geofabrik Vietnam extract. Confirm this decision with graph-build-time and peak-RAM measurements; consider cutting the region only if agreed thresholds are exceeded.
- Support raster, vector, and orthography layers; offline-first operation; two-point car routing; simulated navigation; performance measurement; and weekly reporting.
- Measure FPS and combined RAM/CPU for the app and child services in Release mode with fixed scenarios.
- Manage routing/tile-server child processes through `QProcess` and keep routing/I/O off the UI thread.
- Never download tiles from `tile.openstreetmap.org` or another public tile server. Generate only non-geographic diagnostic tiles in unit 01; obtain real map tiles exclusively from the project-owned Geofabrik PBF/Planetiler/localhost pipeline beginning in unit 03.

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
- Qt 5.13.2 compatibility with GCC 13 for Qt Quick/QtLocation and with system OpenSSL 3 for real online HTTPS. A console probe compiled, but OpenSSL 3 produced unresolved-symbol warnings; offline localhost HTTP is not blocked by this TLS question.
- Vietnam OSM national/provincial road-reference completeness.

## Environment Baseline

- Reference machine: Dell Latitude 5480; Intel Core i5-6300U (2 cores/4 threads); 16 GiB RAM; 512 GB NVMe SSD.
- Available: Linux, GCC/G++ 13.3.0, CMake 3.28.3, GNU Make 4.3, `aqtinstall` 3.3.0, and Qt 5.13.2 `gcc_64` under `/home/viet/Qt/5.13.2/gcc_64`.
- Not currently available on `PATH`: qmake/Qt and Clang. This is intentional for Qt; use its absolute path or `CMAKE_PREFIX_PATH`. Ninja 1.11.1 and ccache 4.9.1 are available on `PATH` after owner-executed installation.
- Graphics: Intel HD Graphics 520 (Skylake GT2), Mesa 25.2.8, direct rendering enabled, OpenGL up to 4.6.

## Session Notes

- No application code, library installation, Qt build, dataset download, push, or merge was performed during context setup.
- `README.md` intentionally remains at the repository root with only the project title; no `docs/ai-playbook.md` exists because the original file was not a playbook.
- The Release configure, build, and run commands are validated below. No automated test target exists yet; the current verification is the bounded runtime/provider probe and X11-window inspection.
- The successful module query returned `debug_info qtcharts qtdatavis3d qtlottie qtnetworkauth qtpurchasing qtscript qtvirtualkeyboard qtwebengine qtwebglplugin`.
- The successful base-archive query returned `icu qt3d qtbase qtconnectivity qtdeclarative qtgamepad qtgraphicaleffects qtimageformats qtlocation qtmultimedia qtquickcontrols qtquickcontrols2 qtremoteobjects qtscxml qtsensors qtserialbus qtserialport qtspeech qtsvg qttools qttranslations qtwayland qtwebchannel qtwebsockets qtwebview qtx11extras qtxmlpatterns`.
- The original install command hit certificate errors on randomly selected fallback mirrors twice. Retrying with `--base https://ftp.jaist.ac.jp/pub/qtproject/` completed with hash verification enabled and no extra modules.
- Qt SSL probe: build version `OpenSSL 1.1.1b  26 Feb 2019`; runtime version `OpenSSL 3.0.13 30 Jan 2024`; `supportsSsl: true`; unresolved symbols `EVP_PKEY_base_id` and `SSL_get_peer_certificate`. This does not yet prove HTTPS works.

## Unit 01 Working Commands

Configure the Release build without modifying `PATH` or shell startup files:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/home/viet/Qt/5.13.2/gcc_64 -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

Build and run:

```bash
cmake --build build --parallel
./build/qtmaps
```

The bounded provider-probe observation used:

```bash
timeout --signal=TERM 8s ./build/qtmaps
xwininfo -root -tree | rg -i 'QtMaps unit 01 probe|qtmaps'
```
