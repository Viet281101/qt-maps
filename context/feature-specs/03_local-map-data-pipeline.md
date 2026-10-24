# Unit 03: Local Map Data Pipeline

Read `AGENTS.md`, the six mandatory context files, Unit 01b evidence, and the approved baseline decision before implementation.

## Goal

Turn the bounded Hanoi feasibility assets into a reproducible, versioned, localhost-only map package and runtime contract for the Qt 5.15.13/MapLibre baseline. Prove tile-service lifecycle, style-resource compatibility, raster fallback selection, and continuous camera-follow performance without implementing routing or final UI.

## Minimum Scope

### 1. Versioned data pipeline

- Accept only an owner-approved local source PBF and pinned generator/config inputs; generation performs no network requests.
- Emit an immutable dataset package ID and machine-readable manifest containing source identity/checksum/date/license, generator name/version/checksum, config revision/hash, geographic bounds, zoom range, layer schema version, resource revisions, output checksums/sizes, and generation time/peak RAM.
- Keep raw PBF, generator binaries, temporary databases, tiles, packages, logs, and benchmark CSV outside Git. Source-control only pipeline configuration, validation code, compact fixtures, and documentation.
- Fail before publishing a package when required metadata, checksums, bounds, layers, attribution, or style resources are missing or inconsistent.

### 2. Tile packaging and service lifecycle

- Package vector tiles and their manifest as an immutable versioned artifact. MBTiles is the preferred production container; an exploded XYZ tree may remain a development fixture, not the final package contract.
- Bind the runtime endpoint to `127.0.0.1` only. QtMaps owns service start/readiness/failure/timeout/shutdown and includes the service in process-tree performance totals.
- Use a bounded startup timeout, explicit health/readiness result, collision-safe port selection/configuration, incremental capped logs, graceful stop, and bounded forced termination.
- The owner selected direct in-process MBTiles/SQLite access behind `TileSource`; no runtime server dependency is added. A future QProcess implementation remains possible behind that interface.

### 3. Style-resource contract

- A dataset package declares one compatible style contract covering style JSON, tile URL template, source-layer names, sprite JSON/PNG, glyph URL/ranges, attribution text, and contract version.
- Startup validation rejects a package whose manifest, style contract, tile layer schema, glyphs, or sprites do not match. Runtime URLs remain localhost-only and contain no public fallback URL or token.
- Vietnamese labels, roads, buildings, attribution, bearing, tilt, and the project marker remain visible with the approved real Hanoi fixture.

### 4. Raster fallback

- Keep a separate renderer/backend contract so a vector startup, package, GPU, or licensing failure can select a configured localhost raster source without changing external network policy.
- Unit 03 validates fallback state, user-visible reason, and successful local raster rendering. The existing diagnostic raster may verify mechanics only and must remain visibly labeled until a real project-generated raster package is available.
- Automatic fallback must be observable in state and logs; it must not silently hide a vector failure.

### 5. Continuous-navigation benchmark

- Use a committed compact route geometry fixture inside the bounded Hanoi dataset. It is simulated movement, not a route-engine result.
- Replay at a fixed documented speed and update interval with uninterrupted camera follow and bearing. Keep the route, speed, viewport, zoom/tilt, duration, render loop, vsync, dataset/package ID, and warm-up identical across runs.
- Mark the navigation interval as continuously interactive and report schema-v2 interaction FPS, frame-time p50/p95/p99, CPU, and RSS for three Release runs. CPU uses 100% per logical core and includes the owned tile-service process.
- Record medians and compare them with the provisional 30 FPS planning target. Do not invent a committed p95/p99 threshold; obtain owner approval after the first continuous-motion evidence.

## Acceptance Criteria

- [x] One command regenerates the bounded Hanoi vector package from already approved local inputs with no Internet access and emits a complete versioned manifest.
- [x] Repeating the pipeline with identical inputs produces matching content checksums or documents every intentionally nondeterministic field outside the content identity.
- [x] Validation detects a changed checksum, missing layer/resource, incompatible style-contract version, and out-of-bounds package.
- [x] No PBF, jar, MBTiles, tiles, package, log, screenshot, CSV, cache, or third-party build tree is tracked by Git.
- [x] QtMaps starts the approved localhost tile service, observes readiness within a bounded validation interval, renders the real Hanoi package, and terminates the service cleanly; startup failure and port conflict are tested.
- [x] Socket evidence contains no non-loopback map-resource request.
- [x] Style, glyph, sprite, layer schema, Vietnamese labels, and OSM/ODbL attribution pass the contract check and visible runtime check.
- [x] A forced vector-package/service failure selects the configured localhost raster fallback with a visible reason; no public tile server is contacted.
- [x] The fixed simulated route runs with continuous camera follow at the documented stable speed for three Release runs per cache state and records median interaction FPS, frame-time p50/p95/p99, total CPU, and total RSS including the tile service.
- [x] Qt Test/CTest covers manifest parsing/validation, lifecycle state transitions/bounded failures, fallback selection, and deterministic route replay logic.
- [x] Qt 5.15.13 Release build and CTest pass; Qt 5.13 comparison instructions remain documented but are not a Unit 03 release gate.
- [x] Evidence records licenses, commands, package identity, measured resource use, limitations, and the next owner decision.

## Not in Unit 03

- Nationwide or final zoom-16 production generation, final storage sizing, incremental updates, downloads, CDN/remote service hosting, or public tile-server fallback.
- Valhalla integration, real route calculation, maneuvers, map matching, rerouting, geocoding/search, live GPS, or final navigation UI.
- Final orthography source/layer, final visual style, theme tokens, installer/package publishing, auto-updater, signing, or distribution to the management machine.
- A 30-to-60-minute soak, cold-cache matrix, rapid layer-switch suite, or final performance thresholds beyond recording the first continuous-motion evidence.
- Removing Qt 5.13 comparison support or changing repository/GitHub Project identity.

## Approved Implementation Decisions

1. Use ignored `data/packages/hanoi-<yyyymmdd>-v1/` packages containing `manifest.json`, `tiles.mbtiles`, `style/`, `glyphs/`, and `sprites/`, plus an ignored `data/packages/current` pointer. Validate every manifest checksum before loading.
2. Serve through an in-process `TileSource` implementation bound to a random `127.0.0.1` port. The HTTP server and `QSqlDatabase` connection run on a worker thread; the interface must allow a later QProcess implementation.
3. Unit 03 fallback uses only the visibly labeled diagnostic localhost raster to prove detection, switching, and reason reporting. Real Hanoi raster data is deferred.
4. Benchmark an explicitly simulated approximately 5 km real-Hanoi street route at a stable approximately 40 km/h for 120 seconds, with rotating camera follow, 10-second warm-up, separate cold/warm-cache results, and three runs per cache state.
5. Record p50, p95, p99, and count of frames over 100 ms. Proposed review thresholds are p50 at most 17 ms and p95 at most 33 ms; p99 is report-only until the owner reviews baseline evidence.
