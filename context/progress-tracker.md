# Progress Tracker

Update this file after every meaningful implementation change. Keep it below the automatic compaction thresholds in `context/ai-workflow-rules.md`; detailed historical evidence belongs in the linked specs and reports.

## Current Phase

- Unit 03 was squash-merged through PR #20 as `1358c3c`.
- Unit 04 was squash-merged through PR #21 as `d4a07bb`. Unit 05 is under review in PR #22 from `feat/05-navigation`. Feature freeze is 2026-11-01, with 2026-11-02 through 2026-11-03 reserved for bug fixes, rehearsal, and reporting.
- The owner approved Qt 5.15.13 plus dynamically linked MapLibre Native Qt v3.0.0 as the application baseline, retaining Qt 5.13.2 for comparison and localhost raster as fallback.
- Performance schema v3 is complete: interaction-only FPS and frame-time p50/p95/p99 exclude the final settling swap, which is reported separately; legacy CSV fields remain for historical comparison.

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
- PR #19 was squash-merged into `main` as `39f6b0f`; Issue #7 remains `In Progress` for the follow-on navigation measurements.

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

The schema-v2 interaction-only rerun produced these three-run medians: Qt 5.13 `mapboxgl` 33.407 FPS with frame-time p50/p95/p99 of 16.535/295.523/319.129 ms; Qt 5.15 MapLibre 50.054 FPS with 16.549/19.035/350.803 ms. CPU medians were 6.271% and 12.660%; mean-RSS medians were 176.684 MiB and 209.021 MiB. The long p99 tail occurs inside the explicit 450 ms response window, while idle phases between actions are excluded.

### Unit 03 — local map data pipeline

- `make hanoi-package` reproducibly emits the ignored checksummed `hanoi-20261024-v1` package and `current` pointer from approved local inputs. Two generations produced identical MBTiles/style/glyph/sprite hashes; the 14,675,968-byte MBTiles contains 226 z12-z15 tiles.
- The in-process worker-thread MBTiles/SQLite service binds a random loopback port behind `TileSource`; strict startup validation, lifecycle/port-conflict/error tests, visible diagnostic-raster fallback, and loopback-only socket trace pass.
- Real-data runtime verification shows Vietnamese labels, buildings/roads, marker, attribution, bearing, and 45-degree tilt. The 5.47 km route is explicitly simulated; each 120-second run covers approximately 1.33 km at 40 km/h.
- Commit review found and fixed a 3,600-frame buffer that had truncated the original 120-second statistics plus a missing style tile-URL contract check. The final runs retain 7,168-7,179 frames and reject non-local style URLs.
- Corrected three-run schema-v3 medians are: cold 60.011 FPS, p50/p95/p99 16.642/17.969/19.322 ms, 17.569% CPU, 208.934 MiB mean RSS; warm 60.012 FPS, 16.646/17.922/19.248 ms, 17.102% CPU, 202.294 MiB mean RSS. Both have zero frames over 100 ms and pass the proposed p50/p95 review thresholds. Full evidence: `docs/benchmarks/unit-03-local-map-data-pipeline.md`.
- PR #20 Release verification also passed inside a network namespace with only loopback enabled: `mapReady=true`, the captured Hanoi view retained Vietnamese labels, roads, buildings, marker, and attribution, and `strace -f -e connect` found no non-loopback IP target.
- PR #20 was squash-merged into `main` as `1358c3c`, closing the Unit 03 branch review. Repository merge settings continue to allow squash merge only.

### Unit 04A — route model and fixtures

- Added a backend-independent route contract with polyline, total meters/seconds, source label, and ordered maneuver type/instruction/street/distance/time/position fields.
- Added strict Valhalla polyline6 decoding and response parsing with explicit errors for malformed geometry, units, numeric bounds, and maneuver indices.
- Valhalla response units are normalized at the parser boundary: omitted units default to kilometres, explicit kilometres/miles convert to metres, and duration remains seconds; focused Qt Tests cover each case.
- Added two fixture-labeled Valhalla-shaped Hanoi responses: Hoàn Kiếm–Thái Hà (5.475 km) and Hoàn Kiếm–Ba Đình (2.600 km). They use real bounded coordinates but are hand-authored, not backend-calculated driving routes.
- Evidence: `docs/routing/unit-04-route-model-fixtures.md`. Qt 5.15.13 Release `make test` passes 4/4.

### Unit 04B — bounded local Valhalla backend

- Owner-installed `osmium-tool` 1.16.0 cut bbox `105.20,20.45,106.20,21.45` with the smart complete-way/relation strategy: 41,468,052 bytes in 20.57 seconds with 3,906,660 KiB peak RSS.
- The checksum-pinned `pyvalhalla` 3.9.1 wheel lives only under ignored `generated/unit-04/`. Its native builder produced a 175,042,264-byte/35-file Hanoi graph with two threads in 64.83 seconds and 699,660 KiB peak RSS.
- Native `valhalla_service` binds only `127.0.0.1`; the managed `QProcess` lifecycle has bounded asynchronous readiness and graceful-interrupt/forced-kill shutdown. The asynchronous client enforces loopback, `auto`, kilometres, `vi-VN`, timeout, cancellation, and explicit errors.
- Twenty alternating real-route requests had 56.138 ms overall median latency. Hoàn Kiếm–Thái Hà is 7.294 km/523.659 s and Hoàn Kiếm–Ba Đình is 4.262 km/392.156 s; native Vietnamese maneuvers are available.
- The two fixtures are now recorded Valhalla 3.9.1 responses with version/date/dataset metadata. Detailed evidence: `docs/routing/unit-04-valhalla-local-backend.md`.

### Unit 04C — fixture-first routing UI

- Added a map-first Qt Quick screen with map-click and alias/coordinate endpoint selection, calculate action, route polyline, A/B markers, summary, Vietnamese maneuver list, maneuver focus, source/connectivity/error state, attribution, and the existing F3 overlay.
- The first Release smoke intentionally used the recorded fixture only. `mapReady=true`; the 7.294 km Hoàn Kiếm–Thái Hà route visually followed the local Hanoi vector roads and displayed all 14 maneuvers with explicit `fixture/mô phỏng` and `offline — fixture` labels.
- The render-facing controller flow is covered by Qt Test. Current minimal runtime lacks Qt Quick Controls/Layouts, so the screen uses dependency-free Qt Quick core primitives. Evidence: `docs/routing/unit-04-routing-ui.md`.
- The same controller is now connected to the managed backend: it validates the exact loopback listen config, switches the visible source to `Valhalla local`, reports exact missing service/config paths without silently loading a fixture, and refuses unsupported fallback endpoints. Release smoke showed the real route polyline and Vietnamese maneuvers; timed/window exit removed both app/service processes and released port 18002.
- Route replies are generation-bound and canceled when endpoints change. Shutdown is coordinated asynchronously with a two-second hard-kill bound and Linux parent-death signal. The UI now has persistent maneuver selection, keyboard actions/tab order, panel-aware bounds/collapse, stronger route contrast, zoom-15 street labels, opaque markers, and compact source health colors.
- Three 15-second idle/render-on-demand Release diagnostics consistently counted one Valhalla child and two total processes; mean process-tree RSS was 423.4-429.9 MiB and mean CPU 2.20-2.53%. This verifies accounting after UI polish, not the deferred full interactive Unit 04E benchmark.
- PR #21 references Issues #4 and #5, is assigned to `Viet281101`, and uses the `First Management Demo` milestone plus the `feature`, `demo`, `priority:P0`, `area:routing`, `area:map`, and `area:ui` labels. The PR and both issues are in Project #6 with Status `In Progress`, Priority `P0`, and Demo Phase `Week 1`.

### Unit 05D1 — selected-route replay

- Added a route-owned replay controller without changing the Unit 03 5.47 km
  benchmark fixture. It accepts the current Valhalla or recorded-fixture route,
  preserves the source label, interpolates position/heading on a non-blocking
  timer, and supports 1x/2x/4x, pause/resume, stop/reset, and arrival.
- The route screen now provides replay controls, a distinct current-position
  marker, camera follow/recenter, bearing rotation, and 45-degree tilt. Focused lifecycle,
  interpolation, speed, pause, stop, arrival, and missing-route tests pass.

### Unit 05D2 — maneuver guidance

- Replay progress now selects the next maneuver from cumulative route geometry,
  reports distance to that maneuver plus remaining route distance/time, and
  keeps the maneuver list synchronized while navigation is active.
- A source-labeled navigation banner presents the current instruction, street,
  remaining values, and an explicit arrival state. Focused controller tests pass;
  D3 boundary and QML coverage remain the active review-gate work.

### Unit 05D3 — navigation review gate

- Controller tests cover route start, turn boundaries, route end/arrival,
  duplicate maneuver positions, invalid maneuver indices, zero replay duration,
  missing routes, invalid speeds, pause/resume, and remaining values.
- QML tests instantiate the banner and verify maneuver, distance, remaining,
  source, and arrival presentation. Keyboard order now includes the start action.
- D1-D3 received preliminary owner approval. E/F remain unstarted.

### Unit 05 pre-D4 follow-up

- D1-D3 received preliminary approval and branch `feat/05-navigation` was
  pushed without a PR. Replay now derives position from a monotonic elapsed-time
  anchor, and a new route request invalidates the old route and replay.
- Tests cover a delayed GUI tick, endpoint changes during replay, and a delayed
  canceled request. A non-fixture Release smoke applied a real Valhalla route;
  its limited log evidence is in `docs/routing/unit-05-navigation.md`.

### Unit 05D4 — simulated re-routing

- A button simulates an offset; the nearest distance to the full polyline must
  exceed 40 m before requesting Valhalla from that position to the destination.
  Replay stops while the last valid route remains visible, replaced requests
  are canceled, and only successful replies replace the route and resume
  navigation. Backend errors are visible without a fixture fallback and retain
  the last valid route for retry. Live GPS/drag is not claimed.
- Tests cover the threshold, outgoing endpoints, replacement of a delayed
  request, successful replay resumption, and backend failure. A real-Valhalla
  UI click-through also verified pause/resume, banner updates, re-routing,
  automatic replay resumption, recenter, and stop. D4 awaits owner review; E
  and F remain unstarted.

### Unit 05 PR #22 merge-gate fixes

- Map endpoint selection is disabled while replay is active and while a
  re-route request keeps the last valid route visible. A QML interaction test
  clicks the map during replay and verifies that endpoints, route geometry,
  route availability, and navigation state do not change; it also verifies
  that endpoint selection is restored after navigation stops.
- At the 900x580 minimum and 1366x768 demo sizes, the route-control stack now
  scrolls independently so the maneuver list retains at least 120 px and can
  scroll its full model. The connection pill moves below the active navigation
  banner; QML geometry tests cover both window sizes and their non-overlap.

## Active Decisions

### Committed

- Demo-first delivery through 2026-11-03; report real, simulated, fixture-backed, online, offline, degraded, and unavailable behavior honestly.
- Current application baseline: Qt 5.15.13, dynamically linked MapLibre Native Qt v3.0.0, C++14, QML/Qt Quick, CMake with Ninja, and Qt 5-compatible CMake APIs. Qt 5.13.2 is comparison-only.
- No public tile-server downloads. Real map artifacts come from the approved Geofabrik/Planetiler/local-service pipeline and stay outside Git.
- Reference machine: Dell Latitude 5480, i5-6300U, 16 GiB RAM, Intel HD Graphics 520, Mesa direct rendering.
- Valhalla is the committed Unit 04 routing direction, subject to the availability, Hanoi graph-build, lifecycle, and route-client validation gates in the Unit 04 spec.

### Proposed — owner approval required

- SQLite FTS5 remains an unapproved offline-search direction.

## In Progress

- Unit 05 D4 is implemented in PR #22 and stopped at the owner review gate. E
  and F have not started.

## Next Up

1. Owner review of D4 and its verification evidence.
2. Do not start E or F before the D4 review gate is released.

## Open Questions and Risks

1. Nationwide graph generation is deferred; record feasibility only if peak RSS stays within the owner-set 10-12 GiB ceiling, and obtain an exact time/limit decision before scheduling the full build.
2. Is zoom 16 practical for Hanoi/Red River Delta after storage, generation-time, visual-quality, and performance measurements?
3. Which orthography source, license, attribution, geographic bounds, zoom range, and storage cap are acceptable?
4. How complete are Vietnam OSM `ref` tags for QL and DT/ĐT roads, and what fallback routing behavior is acceptable?
5. Which machine will run the management demo, and is a distributable package required?
6. Which UI theme tokens, icon source, minimum window size, shortcuts, contrast target, and map-aware light/dark behavior should be committed?
7. Long-duration MapLibre stability, explicit cache bounds, deployment packaging, and Qt 5.13/OpenSSL 3 HTTPS remain unverified.

## Environment and Working Commands

- Qt 5.15.13 baseline: `/usr/lib/qt5/bin/qmake` or `CMAKE_PREFIX_PATH=/usr`; plain `make test` uses this kit.
- Qt 5.13.2 comparison: `/home/viet/Qt/5.13.2/gcc_64/bin/qmake` or `CMAKE_PREFIX_PATH=/home/viet/Qt/5.13.2/gcc_64`; use the isolated override command in `context/code-standards.md`.
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

Licensing and packaging evidence:

- `THIRD_PARTY_NOTICES.md`
- `docs/packaging/linux-dynamic-linking.md`

## Latest Verification

- PR #20 closure gate on 2026-10-24: Qt 5.15.13 Release rebuilt without a new compiler warning; `make test` passed 3/3, `git diff --check` passed, all internal links across 23 tracked Markdown files resolved, and `git ls-files` found no prohibited generated/downloaded artifact. Before its squash merge, the PR was mergeable/clean with CodeRabbit success and the approved tracking metadata.
- **BUILD PASS:** default Qt 5.15.13 Release `make test` in `build-qt515`: 3/3 executables passed, including manifest corruption, service lifecycle/bounded failure/port conflict, fallback, and navigation-route coverage; `ldd` resolves Qt Core from the system Qt 5.15 runtime.
- Last pre-Unit03 isolated Qt 5.13.2 comparison: 2/2 passed and `ldd` resolved Qt Core from `/home/viet/Qt/5.13.2/gcc_64`; Unit 03 is intentionally not gated or rerun on the comparison kit.
- Unit 03 corrected benchmark: three cold plus three warm-cache real-data runs reported `mapReady=true`, 7,168-7,179 interaction frames, no CSV errors, and only loopback IP connections.
- PR #20 offline recheck used the Release binary with no external network interface or route; style, glyph, sprite, and tile requests resolved only through the random in-process `127.0.0.1` endpoint.
- `README.md` and `README_VN.md` now document only the validated Qt 5.15.13/MapLibre workflow, current run/benchmark/package commands, and the bounded data, simulated-route, diagnostic-raster, and unimplemented-feature limitations.
- `.gitignore` explicitly rejects PBF, jar, MBTiles, PMTiles, benchmark CSV/log, cache directories, and screenshot directories regardless of where a local tool creates them; none of these artifact classes is tracked.
- Unit 04A Qt 5.15.13 Release verification: `make test` passed 4/4 executables in 0.64 seconds with no new compiler warning; `git diff --check` passed. Only source, tests, compact JSON fixtures, context, and evidence are changed.
- Unit conversion follow-up: omitted units default to kilometers, explicit kilometers/miles normalize to meters, and seconds remain unchanged; Release `make test` passes 4/4. Branch backup matched `origin/feat/04-routing` at `6aa1c8e` with no PR.
- Valhalla survey evidence: `docs/routing/valhalla-availability-survey.md`. Ubuntu apt/release binaries are unavailable; the official 37.5 MB `pyvalhalla` 3.9.1 wheel is preferred over the 156.3 MiB compressed container or a 328-package source-build dependency set.
- Unit 04B/C closure verification on 2026-10-25: all 6 CTest executables pass; stale delayed replies, active maneuver state, QML keyboard/selection contracts, application-level graceful/forced shutdown, and Linux parent-death cleanup are covered. The real service is one counted direct child on `127.0.0.1:18002`; Alt+F4 and timed exit leave no process/listener, while `kill -9` releases both within 90 ms. The demo script also succeeds from `/tmp`, and missing Valhalla files are logged/shown without fixture fallback.
- PR #21 verification on 2026-10-25: the Qt 5.15.13 Release build passed 5/5 CTest executables, `git diff --check origin/main...HEAD` passed, and the PR is mergeable. CodeRabbit succeeded on the preceding routing head; the latest docs-only tracking head had no check result reported at final verification. Live metadata confirms the issue references, assignee, milestone, six labels, and Project #6 fields described above. The pre-existing `.vscode/settings.json` change remains user-owned and excluded from every commit.
- Unit 05 D3 review gate on 2026-10-25: Qt 5.15.13 Release `make test`
  passed 7/7 executables in 5.52 seconds. Focused navigation and QML tests pass,
  fixture startup reached `mapReady=true`, and `git diff --check` passed. No
  generated data, graph, virtual environment, binary, or screenshot is tracked.
- Unit 05 D4 review gate on 2026-10-25: Qt 5.15.13 Release `make test` passed
  7/7 executables in 6.88 seconds after the recenter and failed-reroute
  preservation follow-ups, and `git diff --check` passed. The real
  Valhalla click-through started, paused, and resumed replay, then replaced the
  7,294 m/14-maneuver route with a 7,622 m/16-maneuver route after simulated
  deviation. Replay, banner, maneuver selection, the explicit recenter/stop
  actions, and stop behavior all updated correctly. The fake-backend tests
  remain the deterministic failure and race coverage, including preservation
  of the last valid route after an HTTP 503 reroute failure.
  A post-fix real-Valhalla repeat replaced the initial route with a 7,307 m,
  522.622 s, 14-maneuver route and resumed replay normally.
