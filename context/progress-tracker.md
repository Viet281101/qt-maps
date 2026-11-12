# Progress Tracker

This is the concise operational snapshot. Detailed completed-work narration is
preserved in Git history through `e0a8086` and the cited pull requests.

## Current Phase

- Units 03--07 are squash-merged into `main`; Unit 06 performance work is
  included. Qt 5.15.18 is the production baseline; Qt 5.15.13 is comparison
  only.
- The active milestone is `Post-demo stabilization` with no due date.
- Unit 08 is in progress on `codex/unit-08-camera-controller` (draft PR #49,
  `Refs #40`, Project #6 `In Progress` / P1 / After Demo).

## Current Goal

Complete Unit 08's bounded-data camera controller before starting #39. The
controller now owns overview, user-gesture completion, maneuver focus,
route-fit, navigation follow, and automated pan/zoom normalization. Route-fit
math uses the documented viewport padding in C++ and rejects invalid or
degenerate input without moving the map. `MapCanvas` has no direct
focus-coordinate assignment bypassing the controller.

The branch also contains the small-viewport text-containment repair for shared
action buttons and the navigation-banner status row. It elides and bounds text
instead of painting labels beneath controls.

## Unit 08 Verification

- On 2026-11-12, the GUI-less `qtmaps_diagnostics_tests` passed after adding
  deterministic route-fit, padding, idempotence, and degenerate-input coverage.
- The offscreen software-backend `qtmaps_routing_qml_tests` passed after wiring
  QML route-fit and focus through the controller.
- The owner authorized a direct-GPU startup on 2026-11-12. With the explicit
  Qt 5.15.18 MapLibre payload, fixture routing reached `mapReady=true` on the
  Intel HD 520; this was not a benchmark or i915 configuration change.
- The remaining Unit 08 acceptance item is fixed-viewport interactive pan/zoom
  review. Automation has no native input surface for an actual drag/pinch, so
  it must be completed visibly on the desktop before merge.

## Publication Status

- PRs #33--#38, #42, #44--#47 are squash-merged. #47 finalized Unit 07 and
  closed #32; Project #6 records their completed items as `Done`.
- Unit 07 backups remain at
  `origin/chore/07a-repo-structure-maintenance`,
  `origin/feat/07b-demo-shell-ui`, `origin/feat/07c-offline-raster`,
  `origin/feat/07d-hanoi-cartography`, and `origin/unit-07e-verify`.
- Draft PR #49 contains the Unit 08 work. Do not mark #40 done or merge it
  until its remaining visual review is accepted.

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

1. Fixed-viewport direct-GPU interactive pan/zoom review for Unit 08 on the
   visible desktop.
2. Any privileged i915 investigation or GPU configuration change.
3. Any benchmark expansion beyond the completed #32 matrix, including a soak.

## Next Up

1. Complete the visible interactive review, then update PR #49 and #40
   truthfully.
2. Start #39 only after the Unit 08 controller contract is accepted.

## Current Environment Evidence

- The approved ignored Qt 5.15.18 private-header overlay is at
  `generated/unit-01b/qt51518-private/`. The explicit
  `maplibre-qt51518` target is not part of ordinary CTest/review flows; it
  validates prerequisites before running and currently needs a future
  owner-authorized rebuild to investigate the vendor `<cstdint>` include issue.
- `tools/run-demo.sh --print-env` selects the Qt 5.15.18 payload. The normal
  software-backend suite last passed 16/16 before Unit 08; the stale
  `build-unit07e-qt51518` tree is obsolete and must not be rebuilt or tested.
- Current ignored valid packages are `hanoi-20261112-v3-r2` and
  `hanoi-raster-20261112-v3-r2`. They have 847 256x256 raster tiles, matching
  v3 manifests, Vietnamese labels, and a completed 1366x768 vector-raster-
  vector review. Keep generated packages and raw evidence out of Git.
