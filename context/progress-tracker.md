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
- The management-demo vertical slice is accepted: #9's clean-start rehearsal
  passed and #1 is ready to close from its recorded evidence. #15 (licensed
  orthography, `Todo`, P1) is the next open delivery item.

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

- `make QT_EXPECTED_VERSION=5.15.18 review` passed for PR #53: formatting,
  isolated Release build, and 17/17 tests.
- Direct-GPU visual captures completed on the Intel HD 520 for the supported
  viewports and vector/raster states. This is functional-layout evidence, not
  a performance benchmark.
- Current ignored compatible packages are `hanoi-20261114-v3-r3` and
  `hanoi-raster-20261115-v3-r4`; each has the v2 glyph revision and matching
  v3 manifests.

## Next delivery plan

1. **#9 rehearsal and #1 handoff — completed.** PR #54 merged as `6e11910` on
   2026-11-15; #9 and #1 are closed and their Project #6 items are `Done`.
   The clean-start preflight, route/search/navigation operator checklist, and
   clean exit passed. Evidence and management-facing limits are in
   `docs/rehearsal/demo-handoff.md` and
   `docs/reports/weekly-report-2026-11-15.md`; no benchmark was added.
2. **#15 — orthography decision gate (P1).** Before code or data work, record
   a candidate source, license, attribution text, bounded Hanoi coverage,
   zoom range, storage estimate, offline behavior, and fallback UX. Obtain an
   owner decision before downloading/generating any imagery. Implement as a
   separate vertical slice only after that gate passes. The proposed Sentinel-2
   Level-2A candidate and its approval conditions are in
   `context/feature-specs/15_orthography-decision-gate.md`.
3. **Deferred search expansion.** Specify and spike the SQLite FTS5 schema,
   generator provenance, query normalization/ranking, and package lifecycle
   after the demo-handoff work. It must remain offline-only and not imply
   remote geocoding.

## Risks and decisions requiring the owner

1. The i915 render-benchmark freeze remains in effect. The completed matrices
   and direct-GPU visual review do not authorize a new benchmark, soak,
   MapLibre/OpenGL investigation, or GPU configuration change.
2. Orthography has no approved source, license, or storage budget. #15 cannot
   proceed beyond planning without an explicit source decision.
3. SQLite FTS5 is still proposed architecture, not an implemented capability.

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
