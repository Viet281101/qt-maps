# Progress Tracker

Update this file after every meaningful implementation change.

## Current Phase

- Unit 02 performance instrumentation is complete on `feat/02-perf-instrumentation`, owner review is complete, and [PR #19](https://github.com/Viet281101/qt-maps/pull/19) is open for merge. Do not begin unit 01b on this branch.

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
- Added a root `Makefile` as a thin wrapper around the authoritative CMake/Ninja workflow. `make requirements` verifies Qt 5.13.2, CMake, Ninja, ccache, G++, and Python 3 for the standard-library-only helper scripts; `make build` completed the validated Release configure/build path successfully. Python provisioning packages are intentionally not represented as application requirements because QtMaps is a Qt/C++ application and the current helper scripts have no third-party Python dependency.
- `QGeoServiceProvider::availableServiceProviders()` returned `mapboxgl`, `here`, `osm`, `itemsoverlay`, `esri`, and `mapbox` (order varied between runs). Constructing `osm` and `mapboxgl` with experimental providers allowed returned a non-null mapping manager for both; provider and mapping errors were zero with empty error strings. This proves provider loading only, not raster or vector rendering.
- Verified against the QtLocation `v5.13.2` source that the OSM plugin parameter is `osm.mapping.custom.host`; the plugin appends `%z/%x/%y.png` to that base URL. The same source confirms `osm.mapping.providersrepository.disabled`, `osm.mapping.prefetching_style=NoPrefetching`, and `osm.useragent`.
- Reviewed the current OpenStreetMap Foundation standard raster-tile policy. It requires HTTPS and prohibits pre-seeding/bulk downloads and offline tile archives, so `tile.openstreetmap.org` will not be used as the source of a curl-downloaded localhost fixture. No tile was downloaded or generated during this research step.
- Owner approved a locally generated diagnostic XYZ fixture for unit 01 and prohibited downloading tiles from `tile.openstreetmap.org` or any other public tile server in all cases, including small tests. Real map content is reserved for the unit 03 project pipeline using Geofabrik PBF, Planetiler, and a localhost tile server. The unit 01 fixture must be labeled as non-geographic diagnostic content.
- Added `tools/generate_diagnostic_tiles.py`, which uses only Python standard-library `math`, `struct`, and `zlib` support and performs no network access. It applies the documented Web Mercator XYZ formula around Hanoi and generates 49 bordered, coordinate-labeled diagnostic tiles at each zoom from 10 through 14 (245 PNG files, 1.2 MiB total). The output under `generated/diagnostic-tiles/` is ignored and not committed; it is not real geographic map data.
- Added the unit 01 diagnostic raster view using the Qt 5.13.2 OSM parameters `osm.mapping.custom.host=http://127.0.0.1:8765/`, `osm.mapping.providersrepository.disabled=true`, `osm.mapping.prefetching_style=NoPrefetching`, and a diagnostic `osm.useragent`. The view labels itself as not a real geographic map and supports mouse gestures plus deterministic arrow-key pan and `+`/`-` zoom controls.
- Served the fixture with `python3 -m http.server 8765 --bind 127.0.0.1 --directory generated/diagnostic-tiles`. A clean zoom-12 load generated 20 localhost requests and zooming to approximately 13 generated 15 more; all 35 returned HTTP 200. Keyboard pan changed the captured window hash from `fea04a14...d552390` to `958e89cb...561c68` and produced additional localhost requests when the viewport moved beyond the deliberately bounded fixture; those out-of-coverage requests returned the expected 404.
- Verified from the QtLocation `v5.13.2` source that the Mapbox GL plugin accepts `mapboxgl.mapping.additional_style_urls`, `mapboxgl.access_token`, `mapboxgl.mapping.cache.memory`, `mapboxgl.mapping.cache.directory`, `mapboxgl.mapping.cache.size`, and `mapboxgl.mapping.use_fbo`. An `http:` additional-style URL is explicitly marked non-HTTPS by that implementation.
- Added a separate `--mapboxgl-probe` mode with a committed localhost style containing only a background layer. The plugin reached `mapReady=true`, selected the `User provided style`, fetched `/style.json` once from `127.0.0.1:8766` with HTTP 200, rendered the background, and identified the Mesa Intel HD Graphics 520 GPU. It also warned that threaded rendering is not optimal. This proves local HTTP style loading and background rendering only; vector tile rendering and MapLibre Native feasibility remain unverified until unit 03 provides project vector tiles and a matching style, including any required sprites/glyphs.
- Added a reusable `/proc` sampler and captured an exploratory Release baseline for the 960x640 diagnostic raster plus its manually launched localhost HTTP server. Idle for 60 seconds averaged 139016 KiB RSS and 0.000% CPU for QtMaps, 20012 KiB and 0.017% for the server, and 159028 KiB and 0.017% combined. A valid symmetric pan/zoom run for 30 seconds averaged 144504.3 KiB and 0.533% for QtMaps, 20626.0 KiB and 0.000% for the server, and 165130.3 KiB and 0.533% combined; combined maxima were 165292 KiB and 2.999%. CPU percentages use one logical CPU as 100%. This is diagnostic-tool validation only and must not be reported to management.
- FPS was not captured: the on-demand Qt Quick scene has no existing frame-swap counter, and adding production FPS instrumentation belongs to unit 02. Two earlier interaction attempts were rejected because asymmetric pan/zoom moved beyond the bounded fixture and caused 404 retries; only the symmetric run with five HTTP 200 responses and zero 404/errors is retained as the interaction baseline.
- On 2026-10-21 the owner accepted unit 01 as **complete for its environment portion**. Verified scope is Qt 5.13.2 provisioning and dependency inspection, CMake/Ninja Release build, provider loading, diagnostic localhost raster mechanics, and exploratory RSS/CPU tooling. Two incomplete evidence tracks were explicitly transferred rather than waived: real rendered-frame FPS to unit 02, and local vector plus MapLibre Native feasibility to `context/feature-specs/01b_vector-feasibility-spike.md`.
- Added the five-part unit 01b vector-feasibility spec. It requires a project-generated local MVT/style/glyph/sprite proof, bearing/tilt verification, dense-urban three-run performance evidence using unit 02 instrumentation, and explicit MapLibre Native switch criteria/build estimation. No dataset or tool was downloaded and no package was installed for this documentation step.
- Owner confirmed the unit 01 branch was merged. Local `main` was fast-forwarded to merge commit `afe5c38`, and `feat/02-perf-instrumentation` was created from that commit on 2026-10-22.
- Implemented bounded completed-swap FPS metrics from Qt 5.13 `QQuickWindow::frameSwapped`, including visible zero-frame intervals, hidden/minimized exclusion, reset, average FPS, and an explicitly defined interval-based 1% low.
- Implemented the `qtmaps-perf-v1` CSV logger on a dedicated worker thread. It discovers the QtMaps PID and readable descendants from Linux `/proc`, sums RSS bytes, calculates CPU deltas with 100% equal to one logical CPU, handles PID churn and unreadable entries, validates existing CSV headers, escapes fields, flushes every row, surfaces non-fatal errors, and shuts down cleanly.
- Added a switchable F3 performance overlay, manual start/stop control, blank-window probe, deterministic raster pan/zoom mode, benchmark CLI metadata, warm-up exclusion, and lazy worker startup so `--disable-instrumentation` does not start the sampler thread.
- Added `qtmaps_perf_tests`. `make test` completed a Release build and passed 1/1 CTest target on 2026-10-22. Coverage includes bounded/zero-frame summaries, CPU/RSS aggregation, a real controlled child entering and leaving the process tree, CSV escaping and one-header append behavior, recording start/stop, and an unwritable path.
- **BUILD PASS (Unit 02):** `make test` used the validated Qt 5.13.2/CMake/Ninja/ccache Release path and passed after the final implementation change. `git diff --check` also passed.
- Recorded short instrumentation-on/off validation runs in `docs/benchmarks/unit-02-performance-results.md`. External-sampler overhead was +2.375 CPU percentage points and +2.963 MiB average RSS for the blank window, and +2.375 CPU points and +2.147 MiB for diagnostic raster pan/zoom. These short diagnostic results are not management-facing product benchmarks.
- Completed three fresh-cache raster runs per render loop. Median default-loop values were 3.618 completed-swap FPS, 3.000 interval 1% low, 3.167% CPU, and 155.076 MiB average RSS; basic-loop medians were 3.741 FPS, 3.000 interval 1% low, 3.399% CPU, and 151.414 MiB average RSS. The on-demand scripted workload does not justify changing the Qt default render loop. Local fixture requests returned HTTP 200 at zooms 12 and 13.
- Enabled CMake compile-command export and pointed VS Code C/C++ IntelliSense at `build/compile_commands.json` on 2026-10-23. This supplies the selected Qt 5.13.2 include paths, compiler flags, project headers, and target definitions without hard-coding machine-specific Qt subdirectories in `includePath`; the existing project cSpell additions were preserved.
- Added a repository-level pull-request tracking gate on 2026-10-23. Every new PR must reference its canonical issue without prematurely closing incomplete acceptance criteria, use the active milestone, join Project #6 without duplication, align Priority and Demo Phase with the issue, and have its live metadata verified after creation.
- Applied the tracking gate to PR #19 on 2026-10-23: its description references Issue #7, its milestone is `First Management Demo`, and its Project #6 item is `In Progress` with Priority `P0` and Demo Phase `Week 2`. Issue #7 is also `In Progress` and remains open because the real navigation-scenario acceptance evidence is not yet complete.

## In Progress

- No implementation is in progress after the Unit 02 completion gate. Owner review is complete and PR #19 is open for merge; generated CSV files remain outside source control.

## Next Up

Before the first demo, execute the shortest stable vertical slice. Existing unit specifications remain authoritative for their completed implementations, but their full completion gates may be deferred and recorded during this milestone.

Immediate feature-unit backlog order:

1. **Merge Unit 02:** merge PR #19 after its required issue, milestone, and Project metadata have been verified, preserving the recorded limitations.
2. **Unit 01b — Local vector and MapLibre Native feasibility:** use the Unit 02 instrumentation to prove or disprove the Qt 5.13.2 `mapboxgl` local vector path, bearing/tilt, dense-urban performance, and MapLibre switch criteria.
3. Resume the demo vertical slice and unit 03 real-data pipeline using the owner-reviewed outcomes.

The demo delivery sequence remains:

1. **Preserve the raster proof** — preserve the validated Release CMake/Ninja path and the policy-compliant, tightly bounded diagnostic fixture through HTTP localhost until project-generated real data is available.
2. **Create the demo shell** — render the first real raster map and add a coherent map-first QML layout with status, route, navigation, layer, and performance presentation areas.
3. **Complete the route vertical slice** — select two points, obtain a real route through the fastest viable approved integration, draw it, and show its summary and maneuvers. Clearly label any bounded fixture or online dependency used for the first demo.
4. **Add navigation presentation** — replay a route or simulated position, follow the current position, show the next maneuver, and expose an obvious exit/recenter action.
5. **Measure the demo** — show rendered-frame FPS and capture short Release-build CPU/RAM results for the application process tree with documented scope.
6. **Add breadth without destabilizing the flow** — attempt offline fallback, vector, orthography, and search in that priority only after the end-to-end demo remains reproducible.
7. **Freeze and rehearse** — fix demo blockers, prepare a deterministic scenario and fallback evidence, update the weekly report, and avoid risky dependency changes immediately before the demonstration.

After the demo, resume the ten-unit plan and close every deferred verification gate rather than treating the demonstration slice as production completion.

## Open Questions

### Environment Validation

1. **Transferred to unit 01b:** `osm` renders the localhost diagnostic raster, and `mapboxgl` loads/renders a localhost HTTP background style. Whether `mapboxgl` renders local vector tiles with a matching local style, glyphs, and sprites remains unverified. The exact vector source/glyph/sprite URL rules for Qt 5.13.2 are also open.
2. **Deferred, low priority; does not block unit 01:** Does Qt 5.13.2 complete a real HTTPS request reliably with OpenSSL 3 despite the two unresolved-symbol warnings? The current spike assumes HTTP localhost for every tile and routing endpoint. Later options are (a) a local tile server that fetches HTTPS upstream and re-serves over HTTP localhost, or (b) a private OpenSSL 1.1 runtime only as a last resort because OpenSSL 1.1 is end-of-life. Do not modify OpenSSL or install `libssl1.1` while this question is deferred.
3. What graph-build-time and peak-RAM thresholds should trigger reconsideration of the initial nationwide routing scope?
4. **Resolved for instrumentation semantics:** Unit 02 now measures completed swaps and an interval-based 1% low, excludes warm-up and hidden/minimized intervals from summaries, and records process-tree RSS/CPU. The current fixture is render-on-demand and remains non-geographic; its short results are tooling evidence, not management-reportable map performance. Dense active-motion evidence transfers to unit 01b with real local vector data.

### Technical and Product

5. **Resolved for unit 01 environment scope:** Qt Quick/QtLocation compiles and runs with GCC 13.3.0 in Release mode, and the `osm` plugin renders the diagnostic localhost raster.
6. **Transferred to unit 01b:** Can the installed Qt 5.13.2 `mapboxgl` plugin render the complete local vector resource set, honor `Map.bearing`/`Map.tilt`, and meet dense-urban performance needs? If not, can a maintained MapLibre Native Qt integration build and render reliably with Qt 5.13.2 on Intel HD Graphics 520 within an acceptable integration budget?
7. **Unit 01b approval gate:** Which exact Planetiler version/distribution and runtime, Hanoi generation bound, MVT schema/style, glyph/font and sprite sources, loopback tile server, and MapLibre Native repository/tag should be approved? Nothing in this list has been downloaded or installed.
8. Is zoom 16 practical for the detailed Hanoi/Red River Delta dataset after storage, generation-time, visual-quality, and performance measurements?
9. Which legally usable orthography source should be selected, and what license, attribution, region, zoom range, and storage cap apply?
10. How complete and consistent are Vietnam OSM `ref` tags for national roads (QL) and provincial roads (DT/ĐT), and what routing behavior is acceptable where tags are missing?
11. Which machine will management use to view the demo, and how does it differ from the reference laptop?
12. Is a distributable installer/package required, or is a documented local Release build sufficient?
13. What exact UI theme token values, icon source, minimum window size, panel dimensions, keyboard shortcuts, contrast target, and map-aware light/dark behavior should be committed?

## Architecture Decisions

### Committed

- Target the first management demo for 2026-11-03 and prioritize a stable, visually coherent vertical slice with measured performance until that date.
- Use Qt 5.13.2 with C++ and QML/Qt Quick 2.13 on Linux. APIs introduced by Qt 5.15 or Qt 6 are forbidden.
- Provision Qt 5.13.2 `gcc_64` through `aqtinstall` under `~/Qt`; the approved installation completed successfully.
- Use CMake with Ninja as the primary build system. Unit 01 validated the Release path; the exact commands are recorded under Unit 01 Working Commands.
- Use Qt 5-compatible CMake APIs only; Qt 6 APIs such as `qt_add_executable`, `qt_add_qml_module`, and `qt_standard_project_setup` are forbidden.
- Use Vietnam zoom 0-10 for national raster/vector coverage and Hanoi plus the surrounding Red River Delta for zoom 11-16 detail. Zoom 16 remains an explicit initial assumption pending evidence.
- Initially build routing data for all of Vietnam from one uncropped Geofabrik Vietnam extract. Confirm this decision with graph-build-time and peak-RAM measurements; consider cutting the region only if agreed thresholds are exceeded.
- Support raster, vector, and orthography layers; offline-first operation; two-point car routing; simulated navigation; performance measurement; and weekly reporting.
- Measure FPS and combined RAM/CPU for the app and child services in Release mode with fixed scenarios.
- Manage routing/tile-server child processes through `QProcess` and keep routing/I/O off the UI thread.
- Never download tiles from `tile.openstreetmap.org` or another public tile server. Generate only non-geographic diagnostic tiles in unit 01; obtain real map tiles exclusively from the project-owned Geofabrik PBF/Planetiler/localhost pipeline. Unit 01b may produce a bounded spike dataset only after explicit owner approval; unit 03 owns the production pipeline.

### Proposed

- Evaluate the installed QtLocation `mapboxgl` plugin for the bounded local-vector path in unit 01b. Consider MapLibre Native with a Qt-compatible integration only if the documented switch criteria are triggered; neither vector backend is committed yet. Use QtLocation `osm` as the verified diagnostic raster path.
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

The convenience wrapper exposes the validated workflow without replacing CMake:

```bash
make requirements
make build
make run
```

Use `make help` for the diagnostic-fixture and localhost style-server targets. Override machine-specific values when required, for example `make QT_PREFIX=/path/to/Qt/5.13.2/gcc_64 BUILD_DIR=build-release build`.

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

Generate and serve the non-geographic diagnostic fixture:

```bash
python3 tools/generate_diagnostic_tiles.py
python3 -m http.server 8765 --bind 127.0.0.1 --directory generated/diagnostic-tiles
```
