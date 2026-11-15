# Progress Tracker

This file is the concise operational snapshot. Detailed implementation evidence
remains in the cited pull requests and Git history; generated data and raw
benchmark artifacts remain ignored.

## Current phase

- Units 03--08 are merged. Qt 5.15.18 is the production baseline; Qt 5.15.13
  is comparison-only.
- The active GitHub milestone is `Post-demo stabilization` with no due date.
- [PR #53](https://github.com/Viet281101/qt-maps/pull/53) merged as `15d198a`
  on 2026-11-15 and closed #16. Project #6 records both items as `Done`.
- The management-demo vertical slice is accepted: #9 and #1 are closed and
  `Done` on Project #6. #15 (licensed orthography, `In Progress`, P1) awaits
  authorized source imagery; #58 (offline SQLite FTS5 search spike, `In
  Progress`, P1) is the active unblocked delivery item.

## Completed product baseline

- Local Hanoi vector and validated raster packages render through the MapLibre
  and loopback boundaries. The app keeps one readable app-owned OSM attribution
  pill, retains route/puck/map items across layer switches, and rejects
  incompatible raster/vector packages.
- Fixture-backed routing and navigation provide endpoint entry, route geometry,
  maneuvers, simulated movement, re-routing presentation, bounded camera
  follow, and explicit simulation labels. The navigation banner is the sole
  owner of the active instruction while replay is running.
- Bounded local place search supplies Hoàn Kiếm, Thái Hà, and Ba Đình with
  debounced idle/loading/ready/empty states and explicit A/B endpoint actions.
  It is visibly labelled as fixture data; SQLite FTS5, wider coverage, and all
  remote geocoding remain deferred.
- The local-only `qtmaps_visual_review` harness captures production QML in
  vector and forced-raster modes at 900x580, 1366x768, and 1920x1080. Its
  latest #53 review drives the actual search input and verifies the popover,
  connection pill, route, and navigation framing do not overlap.

## Current verified evidence

- The #58 QSQLITE FTS5 spike creates and queries an external-content Vietnamese
  fixture schema locally; `make QT_EXPECTED_VERSION=5.15.18 test` passed 19/19.
- `make QT_EXPECTED_VERSION=5.15.18 review` passed for PR #53: formatting,
  isolated Release build, and 17/17 tests.
- Direct-GPU visual captures completed on the Intel HD 520 for the supported
  viewports and vector/raster states. This is functional-layout evidence, not
  a performance benchmark.
- Current ignored compatible packages are `hanoi-20261114-v3-r3` and
  `hanoi-raster-20261115-v3-r4`; each has the v2 glyph revision and matching
  v3 manifests.
- Environment documentation was synchronized on 2026-11-15: system Qt 5.15.18
  is the production baseline, while Qt 5.15.13 remains comparison-only. The
  checked `make QT_EXPECTED_VERSION=5.15.18 requirements` preflight passes.

## Next delivery plan

1. **#9 rehearsal and #1 handoff — completed.** PR #54 merged as `6e11910` on
   2026-11-15; #9 and #1 are closed and their Project #6 items are `Done`.
   The clean-start preflight, route/search/navigation operator checklist, and
   clean exit passed. Evidence and management-facing limits are in
   `docs/rehearsal/demo-handoff.md` and
   `docs/reports/weekly-report-2026-11-15.md`; no benchmark was added.
2. **#15 — approved orthography contract (P1).** The owner approved the
   Sentinel-2 Level-2A 10 m regional RGB source, Hanoi-only bounds, z12–z15,
   100 MiB package cap, required attribution, offline-only runtime, and explicit
   unavailable/failure UX on 2026-11-15. PR #55 merged as `37534a7`. The exact
   contract and provisioning gate are in
   `context/feature-specs/15_orthography-decision-gate.md`. The first offline
   package/manifest contract now has a compact synthetic fixture and focused
   tests; it records complete coverage and provenance but does not process real
   imagery. PR #57 merged as `631d713`. Wait for an authorized Copernicus
   session/account or approved local L2A scene before querying, downloading, or
   processing real imagery.
3. **#58 — FTS5 search spike (P1).** Validate the Qt SQLite FTS5 capability
   with a compact Vietnamese fixture and record the offline package contract;
   do not replace the current fixture search, download a source dataset, or add
   remote geocoding. The proposed schema and acceptance are in
   `context/feature-specs/58_offline-search-fts5-spike.md`.

## Risks and decisions requiring the owner

1. The i915 render-benchmark freeze remains in effect. The completed matrices
   and direct-GPU visual review do not authorize a new benchmark, soak,
   MapLibre/OpenGL investigation, or GPU configuration change.
2. Orthography provisioning needs an authorized Copernicus Data Space
   session/account or a local Sentinel-2 L2A scene; credentials and raw source
   data remain outside Git.
3. SQLite FTS5 remains proposed until #58 records target-runtime capability
   evidence and a later owner decision selects a licensed local source.

## Working rules

- Before each code commit, run `make QT_EXPECTED_VERSION=5.15.18 test`; run
  `make QT_EXPECTED_VERSION=5.15.18 review` before review-ready handoff.
- Use Qt 5 CMake APIs with CMake/Ninja. Do not use Qt 6 APIs, force-push,
  rewrite pushed history, push directly to `main`, or commit generated data,
  build trees, raw benchmarks, credentials, or user-owned files.
- Each PR uses its canonical Issue, active milestone, Project #6, matching
  Priority/Demo Phase, assignment to `Viet281101`, and live-state verification
  after GitHub mutations.
- `tools/run-demo.sh --print-env` selects the approved Qt 5.15.18 payload. The
  explicit `maplibre-qt51518` rebuild remains owner-authorized work only.
