# QtMaps Project Overview

## Overview

QtMaps is a Linux desktop mapping exercise for Vietnam, built by one developer with Qt 5.13.2, C++, QML, and Qt Quick 2.13. It is intended to demonstrate offline-first map rendering, car routing on national and provincial roads, and a Google Maps-like navigation view on modest integrated-graphics hardware. Development is organized into small, verifiable increments, with the immediate priority being a convincing first management demo by 2026-11-03.

## Goals

1. Render Vietnam map data with raster tiles, vector data, and a legally sourced orthography layer, with automatic offline fallback where an online source is supported.
2. Calculate a car route between two points, display the route and maneuver list, and simulate navigation with camera following, map rotation, GPS replay, and re-routing.
3. Run the agreed demo flows on the reference laptop and publish reproducible Release-build FPS, total RAM, and total CPU measurements for the application and its child services.
4. Maintain a weekly progress record that can be summarized for management without reconstructing work from commit history.

## First Management Demo Milestone

The first management demo is due on 2026-11-03. Until that milestone, work is demo-first: establish a stable, visually coherent end-to-end flow before expanding coverage or completing lower-visibility infrastructure.

Priority order for this milestone:

1. A reproducible Release build that opens a polished, map-first Qt Quick shell and renders a real map centered on the agreed Vietnam demo region.
2. A visible two-point driving-route flow with route geometry, summary, maneuver presentation, and clear loading/error states.
3. A navigation-style presentation with camera following or a clearly identified simulated route replay, next-maneuver banner, and current-position feedback.
4. A switchable on-screen performance overlay and a short, reproducible record of FPS, total RAM, and total CPU on the reference laptop.
5. Add offline behavior and additional raster, vector, or orthography layer capability in that order only when the preceding vertical slice remains stable.

For this milestone, breadth, nationwide high-detail coverage, a complete data-generation pipeline, packaging, exhaustive automated tests, long soak tests, and production hardening may be deferred when they threaten the demonstrable vertical slice. Deferred work remains part of the overall project unless the owner explicitly removes it. The demo must identify real, simulated, fixture-backed, online, offline, degraded, and unavailable behavior accurately; an incomplete capability must not be presented as complete.

## Core User Flow

1. The user starts QtMaps and the application starts the required local services or reports a recoverable startup error.
2. The map opens at the configured demo region and remains usable from local data without internet access.
3. The user switches among raster, vector, and orthography layers.
4. The user searches for or selects an origin and destination.
5. QtMaps calculates a car route that prefers the configured national and provincial road classes and shows the geometry and maneuver list.
6. The user starts navigation simulation; the camera follows the simulated position, rotates with travel direction, advances through maneuvers, and requests a new route after a simulated deviation.
7. The developer runs a fixed benchmark scenario and records FPS, RAM, CPU, build information, dataset information, and online/offline mode.

## Features

### Owner-Decided Coverage

- Raster and vector data cover Vietnam at zoom levels 0 through 10.
- Higher-detail demo data covers Hanoi and its surrounding Red River Delta area at zoom levels 11 through 16. Zoom 16 is an initial assumption and remains subject to storage, generation-time, visual-quality, and performance evidence.
- Routing data initially covers all of Vietnam using one uncropped Geofabrik Vietnam extract. This is an owner decision that must be validated with measured graph-build time and peak RAM; consider a regional cut only if those measurements exceed the agreed thresholds.
- These coverage bounds were decided by the project owner; they are not inferred implementation defaults.

### Map Layers

- Raster tile rendering with local/offline operation and an online fallback only when configured and available.
- Vector map rendering, subject to the MapLibre Native Qt 5.13.2 feasibility spike.
- Orthography rendering for a bounded demo area and zoom range from a source whose license permits the intended use.

### Routing and Navigation

- Two-point car routing using a local routing service, with Valhalla currently proposed.
- Road costing configured to prefer appropriate `trunk`, `primary`, and `secondary` roads; behavior depends on actual Vietnam OSM coverage and tags.
- Route geometry, maneuver list, navigation banner, camera following, map rotation, simulated GPS/NMEA playback, and re-routing.
- Polyline decoding must use the precision declared by the selected engine: Valhalla uses precision 6; OSRM would use precision 5.

### Search

- Offline place and address search using a proposed project-built SQLite FTS5 index.

### Offline Operation

- Local map, routing, and search datasets.
- Graceful fallback to available local data when the network is unavailable.
- Explicit status and actionable errors when required local data or a service is missing.

### Performance and Reporting

- On-screen FPS instrumentation based on rendered frames.
- CSV logging for FPS, total resident memory, and total CPU use, including child routing and tile-server processes.
- Fixed Release-build scenarios: idle; urban pan/zoom; long-route navigation; layer changes; online versus offline; and a 30-to-60-minute soak test.
- GitHub Projects issues grouped by work area, weekly milestones, and a weekly summary sourced from `context/progress-tracker.md`.

## Scope

### In Scope

- Linux desktop implementation with Qt 5.13.2, C++, QML, and Qt Quick 2.13.
- The three map layer types, offline-first behavior, two-point car routing, offline search, navigation simulation, and re-routing described above.
- A reproducible local data pipeline for the bounded datasets selected for the demo.
- National raster/vector coverage at zoom 0-10, detailed Hanoi/Red River Delta coverage initially up to zoom 16, and nationwide routing coverage from one uncropped Geofabrik Vietnam extract. The routing scope remains subject to measured graph-build time and peak RAM.
- Performance instrumentation and benchmark reports on the reference laptop.
- Packaging only if the packaging requirement is confirmed in `context/progress-tracker.md`.

### Out of Scope

- Voice guidance.
- Real-time traffic, live incident feeds, and traffic-aware routing.
- Writing a routing algorithm or routing engine from scratch.
- Mobile platforms, web delivery, and operating systems other than Linux.
- Global map coverage or an unrestricted offline orthography dataset.
- Production-grade hosted services, multi-user accounts, and authentication.

## Success Criteria

1. A Release build completes using the real build command selected during unit 01 and starts on the reference machine.
2. After the required local datasets are provisioned, the core map, route, search, and simulated-navigation demo can run with the network disabled.
3. Raster, vector, and orthography layers can each be selected and visibly rendered for their agreed demo coverage; limitations are documented.
4. A route between two selected points is drawn with maneuvers, and navigation simulation demonstrates camera following, rotation, maneuver progression, and re-routing.
5. Each fixed scenario produces a benchmark record containing FPS, total RAM, total CPU, build mode, child-process scope, dataset, and test duration.
6. **Proposed, to be finalized after the unit 01 baseline:** interactive scenarios average at least 30 FPS; idle CPU averages at most 25% of one logical CPU; interactive CPU averages at most 150% in process-level Linux reporting; and combined resident memory for QtMaps and its child services remains at or below 2 GiB after a 30-minute run. These are planning targets, not committed requirements.
7. `context/progress-tracker.md` and the latest weekly report accurately describe completed work, next work, risks, and the newest benchmark results.
