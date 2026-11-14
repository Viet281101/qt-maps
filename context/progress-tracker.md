# Progress Tracker

This is the concise operational snapshot. Detailed completed-work narration is
preserved in Git history through `e0a8086` and the cited pull requests.

## Current Phase

- Units 03--07 are squash-merged into `main`; Unit 06 performance work is
  included. Qt 5.15.18 is the production baseline; Qt 5.15.13 is comparison
  only.
- The active milestone is `Post-demo stabilization` with no due date.
- Unit 08 is complete on `codex/unit-08-camera-controller` and ready for PR
  review (#49, `Closes #40`, Project #6 `In Progress` / P1 / After Demo).

## Current Goal

Unit 08's bounded-data camera controller is ready to merge before starting #39. The
  controller now owns overview, user-gesture completion, maneuver focus,
  route-fit, navigation follow, and automated pan/zoom normalization. Route-fit
  clamps zoom before computing its padded center shift, protects against
  non-finite input, applies bearing-aware usable-viewport bounds, and rejects
  invalid, negatively padded, or out-of-package input without moving the map.
  Route refresh waits for all active gesture signals to finish. `MapCanvas` has no direct focus-coordinate
  assignment bypassing the controller.

The branch also contains the small-viewport text-containment repair for shared
action buttons, navigation-banner status, search/status stacking, and the
compact route panel. It elides and bounds text instead of painting labels
beneath controls.

- On 2026-11-14, the follow-up UI-polish increment replaced the undersized
  search glyph with a centered QML magnifier, shortened the explicitly
  unavailable-search label, made the performance panel hidden-by-default and
  safely stack above attribution when toggled with F3, and gives maneuver
  instructions two readable lines before a secondary street label. Route-ready
  status is now a compact single line. The Release UI was manually reviewed at
  900x580, 1366x768, and 1920x1080; this is functional-layout evidence, not a
  benchmark.

- A second 2026-11-14 density pass made the sidebar toggle a panel-edge tab
  with a 22 px chevron, standardised the core planning UI on system Noto Sans,
  reduced controls to the 36 px minimum target, and compacted the required OSM
  attribution to a readable 24 px pill. The attribution remains visible for
  license compliance.

- A final 2026-11-14 PR-review repair computes route-fit dimensions and its
  padded center offset in bearing-aware screen space, so a rotated long route
  remains inside its usable viewport. The maneuver secondary label now shows
  the actual street name, collapsing maneuvers returns their reserved height to
  the route controls, and remaining layer/navigation/map labels use Noto Sans.
  Deterministic tests cover the 90-degree route fit, street-name binding, and
  recovered collapsed-list space; the Qt 5.15.18 suite passed 16/16.

## Unit 08 Verification

- On 2026-11-12, the GUI-less `qtmaps_diagnostics_tests` passed after adding
  deterministic route-fit, padding, idempotence, and degenerate-input coverage.
- The offscreen software-backend `qtmaps_routing_qml_tests` passed after wiring
  QML route-fit and focus through the controller.
- The owner authorized a direct-GPU startup on 2026-11-12. With the explicit
  Qt 5.15.18 MapLibre payload, fixture routing reached `mapReady=true` on the
  Intel HD 520; this was not a benchmark or i915 configuration change.
- The default `Makefile`, `tools/run-demo.sh`, software smoke runtime, and
  benchmark runtime now select the Qt 5.15.18 MapLibre payload at
  `generated/unit-01b/maplibre-install-qt51518`; an explicit environment
  override remains available for controlled comparison only.
- The owner accepted direct-GPU fixture review at 900x580, 1366x768, and
  1920x1080 after testing pan, zoom, rotation, route-fit, recenter, and
  navigation follow. The 900x580 overlay crowding repair was re-reviewed and
  accepted. This is functional-layout evidence only, not a benchmark.
- The 1920x1080 review exposed a missing local glyph request for `8192-8447.pbf`.
  It is pre-existing package work tracked separately in #50; do not conflate it
  with the Unit 08 camera change.

## Publication Status

- PRs #33--#38, #42, #44--#47 are squash-merged. #47 finalized Unit 07 and
  closed #32; Project #6 records their completed items as `Done`.
- Unit 07 backups remain at
  `origin/chore/07a-repo-structure-maintenance`,
  `origin/feat/07b-demo-shell-ui`, `origin/feat/07c-offline-raster`,
  `origin/feat/07d-hanoi-cartography`, and `origin/unit-07e-verify`.
- PR #49 contains the accepted Unit 08 work and is ready for review. #40 closes
  on its squash merge; keep its Project item In Progress until then.

## Unit 07 Summary

| Unit | State |
| --- | --- |
| 07A | merged as PR #34 (`4a61d46`) |
| 07B | merged as PR #35 (`e294167`) |
| 07C | merged as PR #37 (`90366bd`); branch retained as backup |
| 07D | merged as PR #38 (`ef30610`); branch retained as backup |
| 07E | archived backup; do not revive or open a PR |

## i915 Render-Benchmark Freeze

The i915 stability blocker remains open. The owner authorized the v2, v3, and
full #32 matrices on 2026-11-12 only. Do not run an application window,
MapLibre/OpenGL, GPU benchmark/configuration, i915 work, a benchmark expansion,
or soak without a new explicit owner instruction. The completed full-matrix raw
artifacts remain ignored at
`generated/benchmarks/20261112-hanoi-v3-r2-full-matrix-gpu`.

## Working Rules

- Before each commit run `make QT_EXPECTED_VERSION=5.15.18 test`.
- Use Qt 5 CMake APIs with CMake/Ninja; never use Qt 6 CMake APIs.
- If dependencies are missing, run `make requirements`; only the owner runs
  privileged package restore/install commands shown by `make requirements-apt`.
- Do not force-push, rewrite pushed history, push to `main`, delete local
  branches/tags, or commit generated datasets, raw benchmark data, build trees,
  binaries, credentials, or user-owned untracked paths.
- Every PR uses the repository template, canonical issue, active milestone,
  Project #6 fields, and assignment to `Viet281101`.

## Decisions Waiting for Owner

1. Any privileged i915 investigation or GPU configuration change.
2. Any benchmark expansion beyond the completed #32 matrix, including a soak.

## Next Up

1. Squash-merge PR #49 after PR review, which closes #40.
2. Address #50's missing bounded glyph range before making further map-label
   quality claims, then start #39.

## Current Environment Evidence

- The approved ignored Qt 5.15.18 private-header overlay is at
  `generated/unit-01b/qt51518-private/`. The explicit
  `maplibre-qt51518` target is not part of ordinary CTest/review flows; it
  validates prerequisites before running and currently needs a future
  owner-authorized rebuild to investigate the vendor `<cstdint>` include issue.
- `tools/run-demo.sh --print-env` selects the Qt 5.15.18 payload by default.
  The normal software-backend suite last passed 16/16 after the Unit 08 review
  fixes; the stale
  `build-unit07e-qt51518` tree is obsolete and must not be rebuilt or tested.
- Current ignored valid packages are `hanoi-20261112-v3-r2` and
  `hanoi-raster-20261112-v3-r2`. They have 847 256x256 raster tiles, matching
  v3 manifests, Vietnamese labels, and a completed 1366x768 vector-raster-
  vector review. Keep generated packages and raw evidence out of Git.
