# Progress Tracker

Update this file after every meaningful implementation change. Keep it below the automatic compaction thresholds in `context/ai-workflow-rules.md`; detailed historical evidence belongs in the linked specs and reports.

## Current Phase

- The 2026-11-03 management-demo date has passed. Units 03--05 are squash-merged
  through PRs #20--#22; the local Unit 07A--07D stack is ready for integration
  planning but has no PR or push.
- Unit 07 performance close-out is complete. Retain `0c20b97` for the Canvas
  route state; no more performance implementation is authorized in this round.
  The benchmark protocol and full evidence are authoritative in
  `docs/benchmarks/unit-07-final.md`.
- Default launches use Qt's Wayland plugin in the GNOME Wayland session. The
  xcb/XWayland evidence is material, but choosing xcb as a production default
  awaits the project owner; no override is implemented.
- Display-scale review (2026-11-05) measured Wayland at DPR 2 and xcb/XWayland
  at DPR 1 for the same 960x640 logical window (1920x1280 versus 960x640
  framebuffer/Canvas FBO). XWayland's 192 logical DPI inflated default
  point-size text, causing real layout overflow. Explicit existing Theme pixel
  tokens now cover controls, endpoint fields, and maneuver rows; raster retains one
  application-owned ODbL attribution. Evidence is ignored under `generated/review/20261105-final2/`; this is not a benchmark or platform decision.
- The active package remains v1. Issue #30 and the Issue #32 migration contract
  must merge atomically; v1 remains the rollback path.

## Current Goal

Prepare the completed Unit 07 stack for squash-merge integration, then resume
approved product features. Do not change rendering, locked thresholds, package
contracts, or the Wayland default without an owner decision.

## Completed Outcomes

### Context, tracking, and identity

- The project has a demo-first product definition, architecture, UI conventions, code standards, workflow rules, and dependency-ordered unit specs.
- GitHub Project #6 is named `QtMaps` and must not be renamed or replaced. The `qt-maps` repository must not be renamed without explicit owner approval and an impact review.
- PRs must link the canonical issue, milestone, and Project #6 item; their live metadata and fields must be verified after mutation.
- Repository PR, bug, feature, and engineering-task templates now capture scope,
  honest capability claims, tracking metadata, and verification evidence.
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

- The approved bounded Hanoi Planetiler dataset, licenses/attribution, Vietnamese
  labels, loopback-only access, and real Intel/Mesa GPU rendering were verified;
  no generated/downloaded artifacts entered Git.
- The signed MapLibre Native Qt v3.0.0 build on Qt 5.15.13 outperformed the
  Qt 5.13 comparison in the matched schema-v2 interaction run: 50.054 versus
  33.407 median FPS. Full build, resource, frame-time, CPU, and RSS evidence is
  in `docs/benchmarks/unit-01b-vector-feasibility.md`.

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

### Unit 05 — navigation and simulated re-routing

- PR #22 completed selected-route replay, 1x/2x/4x controls, maneuver guidance,
  arrival/restart, camera follow/recenter, route-owned progress, and simulated
  >40 m deviation/re-routing. The last valid route remains visible while a
  replacement request is pending; successful replies resume replay and errors
  remain visible without a fixture fallback. Live GPS/drag is not claimed.
- Controller/QML coverage includes lifecycle and boundaries, delayed ticks and
  canceled requests, guarded map clicks, endpoint changes, HTTP failure,
  minimum/demo layouts, keyboard flow, independent scrolling, and the endpoint
  form binding race. Authoritative behavior and evidence are in
  `docs/routing/unit-05-navigation.md` and the merged PR history.

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

- **07A:** `chore/07a-repo-structure-maintenance` at `cccfcb7`; local Release
  smoke, isolated review, and N=3 quick matrix passed.
- **07B:** stacked `feat/07b-demo-shell-ui` through `1ce45e4`; UI review fixes
  and evidence are in `docs/benchmarks/unit-07bc-ab-comparison.md`.
- **07C:** stacked `feat/07c-offline-raster` through `a4227ba`; the local
  z12--z16 raster package, lazy bounded caches, rejection contract, lifecycle
  checks, and final N=3 navigation gates are complete. See
  `context/feature-specs/07c_offline-raster-fallback.md` and the Unit 07B/07C
  report. No generated artifact is tracked.
- **07D:** `feat/07d-hanoi-cartography` through `bd854fa`; #29 style work is
  separate from #30 draft-v2 water/green data. Keep #30 and #32 atomic; do not
  migrate v1 or generate glyphs/fonts. Evidence:
  `docs/benchmarks/unit-07d-pre-glyph-v2.md`.
- The environment audit, visual fixes, and Qt 5.15.18 verification are complete;
  detailed evidence is in `docs/benchmarks/unit-07d-environment-abc.md`,
  `docs/benchmarks/unit-07-final.md`, and the linked generated directories.
- `905908e` is the demo snapshot; `0c20b97` reverts the mixed combined Canvas
  path and is the retained performance head. The full close-out records that
  pan/zoom remains below the unchanged 20-FPS 1%-low lock, while xcb navigation
  is close to A+hooks. This is a recorded limitation, not a configuration change.
## Next Up

1. Rebase 07A onto current `main`, open its integration PR, and after each
   squash merge rebase the next stacked branch before opening its PR.
2. Integrate 07B, then 07C. Keep #30 and #32 together in a later atomic
   integration decision; #31 remains deferred.
3. Resume owner-approved feature work only after the integration sequence.
   Keep Wayland as the default until the owner decides on xcb/XWayland.

## Open Questions and Risks

1. Nationwide graph generation is deferred; record feasibility only if peak RSS stays within the owner-set 10-12 GiB ceiling, and obtain an exact time/limit decision before scheduling the full build.
2. Is zoom 16 practical for Hanoi/Red River Delta after storage, generation-time, visual-quality, and performance measurements?
3. Which orthography source, license, attribution, geographic bounds, zoom range, and storage cap are acceptable?
4. How complete are Vietnam OSM `ref` tags for QL and DT/ĐT roads, and what fallback routing behavior is acceptable?
5. Which machine will run the management demo, and is a distributable package required?
6. Which UI theme tokens, icon source, minimum window size, shortcuts, contrast target, and map-aware light/dark behavior should be committed?
7. Long-duration MapLibre stability, explicit cache bounds, deployment packaging, and Qt 5.13/OpenSSL 3 HTTPS remain unverified.

## Environment and Working Commands

- Qt 5.15.18 current standard: `/usr/lib/qt5/bin/qmake` or `CMAKE_PREFIX_PATH=/usr`; plain `make test` uses this kit.
- Qt 5.13.2 comparison: `/home/viet/Qt/5.13.2/gcc_64/bin/qmake` or `CMAKE_PREFIX_PATH=/home/viet/Qt/5.13.2/gcc_64`; use the isolated override command in `context/code-standards.md`.
- Do not modify global `PATH`; `/usr/bin/qmake` currently resolves to package-managed Qt 5.15.18.

```bash
make requirements
make run
make debug-run
make debug-gdb
make check
make review
```

Authoritative benchmark evidence:

- `docs/benchmarks/unit-01b-vector-feasibility.md`
- `docs/benchmarks/unit-02-performance-results.md`
- `generated/benchmarks/20261027-233639-complete/report.md` (Unit 06 local run)
- `docs/benchmarks/unit-07bc-ab-comparison.md` (same-day Unit 07 A/B)

Licensing and packaging evidence:

- `THIRD_PARTY_NOTICES.md`
- `docs/packaging/linux-dynamic-linking.md`

## Latest Verification

- Unit 05 D4 evidence is authoritative in
  `docs/routing/unit-05-navigation.md`; its final `make check`, Release smoke,
  and isolated review build passed before PR #22 merged.
- Unit 06 on 2026-10-28: `make test` passed 10/10 CTest executables. The
  complete Release matrix is in `generated/benchmarks/20261027-233639-complete/`;
  the independent external scope/memory sample is in
  `generated/benchmarks/external-crosscheck-20261028/`.
- Unit 06 offline first-frame delay was traced to an unavailable X11 session-
  manager ICE socket, not map or Valhalla network access. Clearing that benchmark
  child variable produced 721 ms offline-first versus 778 ms online-second;
  forced PSS reads yielded 501.98 versus 500.74 MiB route peaks.
- Unit 07C final Release A/B uses N=3, 15+3 seconds, 500/2000 ms sampling,
  online/offline, and both `navigation_replay` and `pan_zoom`. All median locks
  pass; complete min/max and worst-run evidence is in the Unit 07B/07C report.
- Unit 07 final close-out (2026-11-05): HEAD `0c20b97` plus harness commits
  `768f020`/`1341c89` was measured N=5 at 5+60 seconds against a temporary
  A+hooks copy. HEAD pan/zoom 1%-low remains below the 20-FPS lock
  (18.83 online / 19.15 offline) and prior no-draw reference (~26); no more
  Canvas changes are planned. Evidence and limitations are in
  `docs/benchmarks/unit-07-final.md` and
  `generated/benchmarks/20261105-final-matrix/`.
- Navigation follow-up (2026-11-05) stopped at the owner-set 60-minute limit
  after one cooled 5+60-second Wayland online/offline round for A+hooks,
  905908e, 2aa4629, 0c20b97, and 905908e plus zoom-cache simplification.
  No N=3 aggregate, xcb comparison, or pan/zoom selection run was started.
  Raw evidence is retained in
  `generated/benchmarks/20261105-navigation-investigation/`; no configuration
  decision is justified by this incomplete matrix.
- Measurement-validity follow-up (2026-11-05) rebuilt original `905908e`
  without benchmark hooks or the 16 ms timer. On the same 5+60/N=3 online
  protocol it measured 14.14 FPS 1%-low / 45.37 ms p95 on Wayland, but 22.47 /
  20.56 on xcb. The historical 03/11 platform was not recorded, so xcb is an
  evidence-backed explanation rather than a confirmed historical match. Raw
  runs remain in `generated/benchmarks/20261105-validity/`; the timer and
  HEAD/A+hooks xcb matrices were not started after the total time limit.
- Platform follow-up (2026-11-05): an ordinary `make run` launch did not set
  `QT_QPA_PLATFORM` and loaded Qt's Wayland platform plugin in the GNOME
  Wayland session. A cooled online xcb/XWayland N=3 matrix is retained at
  `generated/benchmarks/20261105-xcb-matrix/`: navigation 1%-low/frame-p95
  was 24.94/22.14 ms (A+hooks), 20.52/21.35 ms (original `905908e`), and
  23.26/22.99 ms (HEAD `0c20b97`); pan/zoom was 21.75/28.03, 15.27/28.97,
  and 17.60/28.95 respectively. The online-only budget excluded optional
  offline reruns. The xcb effect is material but does not identify the
  unlogged 03/11 platform; no production platform override has been made.
- Display/layout follow-up (2026-11-06): `make QT_EXPECTED_VERSION=5.15.18
  test` passes 13/13 after each small layout/diagnostic commit. The FPS/RSS
  overlay and ODbL attribution now use existing pixel tokens: both are 36 px
  and 28 px logical high on Wayland and xcb (0 px difference). Captures are in
  `generated/review/20261106-overlay/`; Wayland expands requested 1366x768 to
  1530x799 logical, while xcb honors it, so raster pairs remain visual evidence.
