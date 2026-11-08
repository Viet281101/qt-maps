# Progress Tracker

This is the concise operational snapshot. The prior detailed outcome log is
preserved in Git history through `e0a8086`.

## Current Phase

- Units 03--05 are squash-merged into `main`; Unit 06 performance work is also
  present on `origin/main`.
- Unit 07 is a dependency-ordered stack whose five branches have remote backup
  copies. Unit 07B was squash-merged as PR #35.
- Qt 5.15.18 is the production baseline. Qt 5.15.13 is comparison-only.

## Current Goal

Repair the official demo launcher's Qt runtime selection without opening a
window or restarting render/benchmark work. Do not open a 07E PR.

## Publication Status

- PR [#33](https://github.com/Viet281101/qt-maps/pull/33),
  `docs/qt-5-15-18-alignment` → `main`, was squash-merged as `4193f07`.
- PR [#34](https://github.com/Viet281101/qt-maps/pull/34),
  `chore/07a-repo-structure-maintenance` → `main`, was squash-merged as
  `4a61d46`.
- PR [#35](https://github.com/Viet281101/qt-maps/pull/35),
  `feat/07b-demo-shell-ui` → `main`, was squash-merged as `e294167`.
- PR [#36](https://github.com/Viet281101/qt-maps/pull/36),
  `chore/ctest-software-backend` → `main`, was squash-merged as `0fc791d`;
  it forces software Qt Quick rendering for the CTest smoke and QML tests.
- PR [#37](https://github.com/Viet281101/qt-maps/pull/37),
  `feat/07c-offline-raster-pr` → `main`, was squash-merged as `90366bd`.
- PR [#38](https://github.com/Viet281101/qt-maps/pull/38),
  `feat/07d-style-only-pr` → `main`, is open for review; it references #28
  and #29. The current milestone is expired and no replacement is approved.
- The Unit 07 remote backups are `origin/chore/07a-repo-structure-maintenance`,
  `origin/feat/07b-demo-shell-ui`, `origin/feat/07c-offline-raster`,
  `origin/feat/07d-hanoi-cartography`, and `origin/unit-07e-verify`.

## Unit 07 Status

| Unit | Branch / head | State |
| --- | --- | --- |
| 07A | `chore/07a-repo-structure-maintenance` / `4a61d46` | merged as PR #34 |
| 07B | `feat/07b-demo-shell-ui` / `e294167` | merged as PR #35 |
| 07C | `feat/07c-offline-raster-pr` / `90366bd` | merged as PR #37; old branch remains backup |
| 07D | `feat/07d-style-only-pr` / `aa7f868` | PR #38 open; old branch remains backup |
| 07E | `unit-07e-verify` / `50fc763` | pushed; depends on 07D |

`chore/fix-benchmark-qt-runtime` is a separate local follow-up above 07E and
is not part of the requested Unit 07 backup push.

## i915 Render-Benchmark Freeze

Render benchmarks are frozen while the open i915 stability blocker is resolved.
Do not run the application, MapLibre/OpenGL paths, GPU benchmarks, or GPU
configuration experiments without explicit owner approval. Retained render
measurements are historical evidence only and must not be used as current
performance or stability conclusions. Non-render build and test verification
remains allowed.

## Working Rules

- Before each commit run `make QT_EXPECTED_VERSION=5.15.18 test`.
- Use Qt 5 CMake APIs and CMake/Ninja. Do not use Qt 6 CMake APIs.
- Do not force-push, rewrite pushed history, push to `main`, or delete local
  branches/tags.
- Do not commit generated datasets, raw benchmark data, build directories, or
  binaries. Preserve user-owned untracked paths.
- Every PR uses the repository template, a canonical issue, the applicable
  milestone and Project #6 (`QtMaps`) fields, and is assigned to `Viet281101`.

## Decisions Waiting for Owner

1. New milestone name and due date after the expired `First Management Demo`
   milestone; the owner requested a proposal but must approve creation.
2. Any privileged i915 investigation, GPU configuration change, or restart of
   render benchmarking.
3. Atomic integration decision for the 07D cartography work and its associated
   package migration.

## Next Up

1. The one authorized 07D software-backend CTest passed. Await owner review of
   PR #38 before a squash merge; do not open 07E first.
2. Do not restart rendering or benchmark work.
3. Follow up separately on default-disabled instrumentation/performance-badge
   behavior; do not fold it into 07C.

## Latest Verification

- On 2026-11-08, a clean `ef30610` build reached an incompatible-Qt abort
  before Scene Graph initialization because `tools/run-demo.sh` prepended the
  stale `generated/unit-01b/qml-runtime` plugin root and Qt loaded its
  `sqldrivers/libqsqlite.so`. The runtime-selection repair must use the active
  Qt 5.15.18 kit and reject a MapLibre payload built against another Qt patch
  release. No further window, OpenGL, or benchmark run is authorized here.

- The 07C merge commit `5ef2e4a` resolved the `CMakeLists.txt`/`Makefile`
  conflicts by retaining 5.15.13 defaults and dropping the smoke's
  `QTMAPS_QML_RUNTIME`/`QTMAPS_MAPLIBRE_INSTALL` overrides. That made the
  smoke fall back to the 5.15.13 generated QML runtime while loading the
  5.15.18 MapLibre install, causing the incompatible-Qt abort. A minimal
  follow-up `2b467bf` restores the `origin/main` 5.15.18 defaults and runtime
  overrides. A fresh `build-07c-qt51518-rerun` configured and built against
  Qt 5.15.18; `readelf` found no binary RPATH/RUNPATH. The QML test's
  `generated/unit-01b/qml-runtime` import path is inherited unchanged from
  `origin/main`; its plugins have no RPATH/RUNPATH or 5.15.13 strings.
- On 2026-11-08, the one authorized software-backend CTest run passed all
  11/11 tests in 11.24 s. The 30-second smoke test passed in 1.05 s and its
  log is `generated/ctest/20261108-07c-rerun.log`. No app was launched
  manually, no standalone MapLibre/OpenGL path or benchmark ran, and no
  additional CTest rerun is authorized.
- On 2026-11-08, style-only 07D's one authorized software-backend CTest run
  passed all 12/12 tests in 11.32 s. The 30-second smoke passed in 1.00 s; the
  added `qtmaps_hanoi_style_tests` passed in 0.34 s. Its log is
  `generated/ctest/20261108-07d.log`; no manual app, MapLibre/OpenGL path, or
  benchmark ran.
- Remote `origin/main` is `0fc791d` (the squash merge of PR #36).
