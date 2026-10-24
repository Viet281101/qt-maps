# Progress Tracker

Update this file after every meaningful implementation change. Keep it below the automatic compaction thresholds in `context/ai-workflow-rules.md`; detailed historical evidence belongs in the linked specs and reports.

## Current Phase

- Unit 01b is complete locally on `feat/01b-vector-feasibility-spike`, which branches from Unit 02 because it reuses instrumentation not yet merged into `main`.
- The bounded real-Hanoi comparison recommends Qt 5.15.13 plus MapLibre Native Qt v3.0.0 for the demo, with QtLocation raster as fallback. The project-wide baseline remains Qt 5.13.2 until owner approval.
- The Unit 01b branch has not been pushed or merged. Unit 02 remains associated with PR #19 and Issue #7.

## Current Goal

Deliver a stable management demo by 2026-11-03: polished map-first Release UI, visible two-point routing and navigation-style flow, and honest FPS/RAM/CPU evidence on the reference laptop.

## Completed Outcomes

### Context, tracking, and identity

- The project has a demo-first product definition, architecture, UI conventions, code standards, workflow rules, and dependency-ordered unit specs.
- GitHub Project #6 is named `QtMaps` and must not be renamed or replaced. The `qt-maps` repository must not be renamed without explicit owner approval and an impact review.
- PRs must link the canonical issue, milestone, and Project #6 item; their live metadata and fields must be verified after mutation.
- Context files now compact automatically when they exceed 250 lines or 24 KiB, become dominated by history, or duplicate authoritative facts. The pre-compaction completed log is retained in Git commit `0671f99`.

### Unit 01 — environment and rendering foundation

- Qt 5.13.2 remains available at `/home/viet/Qt/5.13.2/gcc_64`; its QtLocation/QtPositioning plugins and runtime dependencies were verified.
- CMake/Ninja/ccache Release builds work with GCC 13.3.0 and Qt 5-compatible CMake APIs. The root `Makefile` is a convenience wrapper only.
- The local diagnostic raster and `mapboxgl` style probes verified localhost provider loading and rendering mechanics without using public tile servers.
- Qt 5.13.2 uses build-time OpenSSL 1.1 APIs against runtime OpenSSL 3 and emits two unresolved-symbol warnings. HTTPS remains deferred; all current map/service endpoints use HTTP on loopback.
- Detailed scope and transferred gates are recorded in `context/feature-specs/01b_vector-feasibility-spike.md` and the Git history preceding Unit 02.

### Unit 02 — performance instrumentation

- Implemented completed-swap FPS, interval 1% low, process-tree RSS/CPU, `qtmaps-perf-v1` CSV logging, F3 overlay, warm-up exclusion, and bounded worker shutdown.
- Unit tests cover summaries, CPU/RSS aggregation, child-process churn, CSV handling, recording control, and error paths.
- Latest accepted Unit 02 build passed the Qt 5.13.2 Release test path; detailed diagnostic results are in `docs/benchmarks/unit-02-performance-results.md`.
- PR #19 references Issue #7, uses the `First Management Demo` milestone, and is `In Progress` in Project #6. It remains unmerged.

### Unit 01b — vector feasibility

- Generated a bounded Hanoi dataset from the owner-approved Geofabrik Vietnam PBF with Planetiler v0.10.2: bounds `105.78,20.97,105.90,21.10`, zooms 12-15, 226 tiles, 14,463,231 tile bytes, 62.94 seconds, 6 GiB heap limit, two workers, and 2,635,164 KiB peak RSS.
- Verified checksums, licenses, OSM/ODbL attribution, Vietnamese `HÀ NỘI` labels, bearing/tilt, and loopback-only network access. No PBF, jar, generated tiles, logs, screenshots, or CSV entered Git.
- Installed only the owner-approved minimal Qt 5.15.13 development packages after a dry run reported 0 upgrades, 32 new packages, 0 removals, and 1 held package. Qt 5.13.2 remains independently selectable.
- Built signed MapLibre Native Qt v3.0.0 commit `d929c783737120787b43417d9ef05da88da75dfd` with Qt 5.15.13, GCC 13.3.0, Release, OpenGL, and `make -j2`. Build: 16:01.44 wall time, 1,149,676 KiB peak RSS, 92 MiB build tree, 12 MiB install tree.
- GPU verification used real Mesa Intel HD Graphics 520 direct rendering, not llvmpipe. Qt Quick used the threaded render loop and a 16.66 ms/60 Hz vsync animation driver.
- Both renderers used the same real Hanoi data, localhost style/glyph/sprite resources, camera sequence, Unit 02 instrumentation, three-second warm-up, and 15-second recording.

| Renderer | Median avg FPS | Median interval 1% low | Median CPU | Median mean RSS |
| --- | ---: | ---: | ---: | ---: |
| Qt 5.13.2 `mapboxgl` | 24.362 | 5.000 | 6.600% | 177.087 MiB |
| Qt 5.15.13 MapLibre v3 | 33.769 | 5.000 | 10.440% | 209.778 MiB |

CPU uses 100% per logical core; the separately launched localhost server is excluded. The low one-second samples occurred during no-action/render-on-demand phases at low CPU and recovered during camera motion, so the current 1% low mainly reflects coarse sampling/frame pacing rather than sustained tile loading or GPU saturation. See `docs/benchmarks/unit-01b-vector-feasibility.md` for complete evidence.

## Active Decisions

### Committed

- Demo-first delivery through 2026-11-03; report real, simulated, fixture-backed, online, offline, degraded, and unavailable behavior honestly.
- Current application baseline: Qt 5.13.2, C++14, QML/Qt Quick 2.13, CMake with Ninja, and Qt 5-compatible CMake APIs.
- No public tile-server downloads. Real map artifacts come from the approved Geofabrik/Planetiler/local-service pipeline and stay outside Git.
- Reference machine: Dell Latitude 5480, i5-6300U, 16 GiB RAM, Intel HD Graphics 520, Mesa direct rendering.

### Proposed — owner approval required

- Migrate the demo baseline to Qt 5.15.13 plus dynamically linked MapLibre Native Qt v3.0.0.
- Retain QtLocation raster tiles as the fallback and the Qt 5.13.2 vector probe as rollback/comparison evidence.
- Require a continuous-motion navigation benchmark before treating UI frame pacing as stable.
- Use Geofabrik/Planetiler-generated map data, a loopback-only tile service or direct MBTiles access, Valhalla for routing, and SQLite FTS5 for offline search.

## In Progress

- Await owner review of Unit 01b and approval or rejection of the renderer/version migration, LGPL-compatible dynamic-link packaging approach, and continuous-motion performance gate.

## Next Up

1. Review Unit 02/PR #19 and the local Unit 01b commits; do not push or merge without owner approval.
2. After the renderer decision, start the minimum Unit 03 pipeline: reproducible bounded Hanoi generation and loopback service, renderer/backend contract, raster fallback, and continuous-navigation workload.
3. Build the demo shell, then the real two-point route, navigation replay, and fixed Release benchmark in that order.

## Open Questions and Risks

1. Approve Qt 5.15.13/MapLibre v3, dynamic-link packaging, and the continuous-motion gate, or keep Qt 5.13.2 as the demo baseline?
2. What graph-build time and peak-RAM thresholds should trigger reducing the initial nationwide routing scope?
3. Is zoom 16 practical for Hanoi/Red River Delta after storage, generation-time, visual-quality, and performance measurements?
4. Which orthography source, license, attribution, geographic bounds, zoom range, and storage cap are acceptable?
5. How complete are Vietnam OSM `ref` tags for QL and DT/ĐT roads, and what fallback routing behavior is acceptable?
6. Which machine will run the management demo, and is a distributable package required?
7. Which UI theme tokens, icon source, minimum window size, shortcuts, contrast target, and map-aware light/dark behavior should be committed?
8. Long-duration MapLibre stability, deployment packaging, continuous-motion frame pacing, and Qt 5.13/OpenSSL 3 HTTPS remain unverified.

## Environment and Working Commands

- Qt 5.13.2: `/home/viet/Qt/5.13.2/gcc_64/bin/qmake` or `CMAKE_PREFIX_PATH=/home/viet/Qt/5.13.2/gcc_64`.
- Qt 5.15.13: `/usr/lib/qt5/bin/qmake` or `CMAKE_PREFIX_PATH=/usr`.
- Do not modify global `PATH`; `/usr/bin/qmake` currently resolves to package-managed Qt 5.15.13.

```bash
make requirements
make build
make test
make run
```

Authoritative benchmark evidence:

- `docs/benchmarks/unit-01b-vector-feasibility.md`
- `docs/benchmarks/unit-02-performance-results.md`

## Latest Verification

- Unit 01b closeout commits: `1b1dcca`, `36335cd`, `e1d9560`, `0671f99`.
- Qt 5.13.2 Release `make test`: 2/2 passed.
- Isolated Qt 5.15.13 Release `ctest`: 2/2 passed.
- Unit 01b closeout left the worktree clean; the subsequent context-compaction documentation change is the only new scope. No push or merge was performed.
