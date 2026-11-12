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

Complete #31's Vietnamese typography and bounded-label contract from current
`main`, without reusing the stale 07E branch or expanding the owner-approved
render/benchmark scope.

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
  `feat/07d-style-only-pr` → `main`, was squash-merged as `ef30610`.
- PR [#42](https://github.com/Viet281101/qt-maps/pull/42),
  `codex/qt51518-runtime-integration` → `main`, was squash-merged as
  `1157c866`.
- PR [#44](https://github.com/Viet281101/qt-maps/pull/44),
  `docs/hanoi-cartography-upgrade-plan` → `main`, was squash-merged as
  `1c7a05d`.
- PR [#45](https://github.com/Viet281101/qt-maps/pull/45),
  `codex/hanoi-v2-contract` → `main`, was squash-merged as `8fdfd3f`. Its
  Project #6 item is `Done`; #30 is closed/Done and #32 remains In Progress
  for its broader verification scope.
- PR [#46](https://github.com/Viet281101/qt-maps/pull/46),
  `codex/hanoi-vietnamese-labels` → `main`, is a draft linked to #31/#32,
  assigned to Viet281101, and tracked In Progress/P1/After Demo in Project #6.
  It must remain draft until a matching v3 raster and graphical review are
  explicitly authorized and recorded.
- The active follow-up milestone is `Post-demo stabilization`; it has no due
  date until the owner confirms the next delivery target.
- The Unit 07 remote backups are `origin/chore/07a-repo-structure-maintenance`,
  `origin/feat/07b-demo-shell-ui`, `origin/feat/07c-offline-raster`,
  `origin/feat/07d-hanoi-cartography`, and `origin/unit-07e-verify`.

## Unit 07 Status

| Unit | Branch / head | State |
| --- | --- | --- |
| 07A | `chore/07a-repo-structure-maintenance` / `4a61d46` | merged as PR #34 |
| 07B | `feat/07b-demo-shell-ui` / `e294167` | merged as PR #35 |
| 07C | `feat/07c-offline-raster-pr` / `90366bd` | merged as PR #37; old branch remains backup |
| 07D | `feat/07d-style-only-pr` / `aa7f868` | merged as PR #38; old branch remains backup |
| 07E | `unit-07e-verify` / `50fc763` | archived backup; do not revive or open a PR |

`chore/fix-benchmark-qt-runtime` is a separate local follow-up above 07E and
is not part of the requested Unit 07 backup push.

## i915 Render-Benchmark Freeze

The i915 stability blocker remains open. The owner explicitly authorized the
v2 checks on 2026-11-11 and the v3 label render/runtime review on 2026-11-12.
Do not run further application, MapLibre/OpenGL, GPU benchmark, GPU
configuration, or i915-investigation work without another explicit owner
instruction. Historical measurements remain historical; current scoped GPU
measurements apply only to their stated scenarios.

## Working Rules

- Before each commit run `make QT_EXPECTED_VERSION=5.15.18 test`.
- If configure or build reports a missing library or development dependency,
  run `make requirements` first. If it reports a missing package, use
  `make requirements-apt` to print the reviewed restore and `apt-mark manual`
  commands; review and run those privileged commands manually before retrying.
- Use Qt 5 CMake APIs and CMake/Ninja. Do not use Qt 6 CMake APIs.
- Do not force-push, rewrite pushed history, push to `main`, or delete local
  branches/tags.
- Do not commit generated datasets, raw benchmark data, build directories, or
  binaries. Preserve user-owned untracked paths.
- Every PR uses the repository template, a canonical issue, the applicable
  milestone and Project #6 (`QtMaps`) fields, and is assigned to `Viet281101`.

## Decisions Waiting for Owner

1. Any privileged i915 investigation or GPU configuration change.
2. Any benchmark expansion beyond the recorded v2 pan/zoom and real-Valhalla
   navigation scenarios, including a full soak or all-scenario Release matrix.

## Next Up

1. Complete #31 typography/labels on `codex/hanoi-vietnamese-labels` and
   retain a fail-closed v3 package/raster boundary until graphical review.
2. Keep #32 verification and instrumentation follow-up separate.
3. Do not broaden GPU work while the i915 blocker is unresolved.

## Latest Verification

- The approved ignored Qt 5.15.18 private-header overlay is at
  `generated/unit-01b/qt51518-private/`. The separate
  `maplibre-qt51518` Makefile target is intended to configure a fresh Release
  MapLibre install only when explicitly invoked; it is not part of normal
  CTest/check/review flows. Its generated-path and Qt-version preconditions are
  checked before the recipe executes. On 2026-11-09, `make -n` printed the
  approved configure/build/install sequence and rejected a missing overlay;
  the one authorized software-backend CTest pass was 11/11 in 11.42 s
  (`generated/ctest/20261109-maplibre-qt51518-target.log`). No MapLibre build
  or window/OpenGL/benchmark run occurred.
- The Qt 5.15.18 MapLibre rebuild later reached compilation under GCC 15.2.0
  and exposed a non-self-sufficient vendor header missing `<cstdint>`. The
  isolated target defaults to `-include cstdint` and Ninja keep-going (`-k 0`)
  so a future owner-authorized build can collect independent compiler failures;
  neither workaround changes QtMaps application compilation.
- `make requirements` verifies only normal QtMaps build dependencies.
  `make maplibre-requirements` adds the private-Qt/ICU checks required solely
  by the explicit MapLibre rebuild target. Their `*-apt` companions only print
  reviewed restore and `apt-mark manual` commands, so package installation
  remains an owner action.

- On 2026-11-08, a clean `ef30610` build reached an incompatible-Qt abort
  before Scene Graph initialization because `tools/run-demo.sh` prepended the
  stale `generated/unit-01b/qml-runtime` plugin root and Qt loaded its
  `sqldrivers/libqsqlite.so`. The runtime-selection repair must use the active
  Qt 5.15.18 kit and reject a MapLibre payload built against another Qt patch
  release. No further window, OpenGL, or benchmark run is authorized here.
- On 2026-11-10, integration retained the rebuild and launcher changes. The
  Qt 5.15.18 payload's `libQMapLibre.so.3.0.0` has only the `5.15.18` marker,
  its four MapLibre libraries/plugins resolve Qt and ICU from `/usr`, and
  `tools/run-demo.sh --print-env` succeeds with that payload. The pipeline
  check was changed to process substitution so Bash `pipefail` does not turn a
  successful early `grep -q` match into a `strings` SIGPIPE failure.
- The one permitted CTest run on 2026-11-10 is logged at
  `generated/ctest/20261110-qt51518-runtime-integration.log`: 11/13 passed.
  The stale `build-unit07e-qt51518` tree's smoke executable aborts from a Qt
  5.15.13/5.15.18 mix, and its obsolete `qtmaps_hanoi_v2_schema_tests` entry
  names a removed source file. Do not rerun or rebuild without owner approval.
- On 2026-11-10, the smoke and routing-QML CTest paths were changed to derive
  the QML runtime and plugin roots from the selected Qt 5.15.18 qmake rather
  than `generated/unit-01b/qml-runtime`. A Ninja reconfigure and app-only
  rebuild of `build-qt515` succeeded; its software-backend CTest pass was
  13/13 in 11.64 s (`generated/ctest/20261110-qt51518-runtime-smoke-fix-ctest.log`).
- On 2026-11-11, normal QtMaps requirements were split from the private-Qt/ICU
  requirements of the explicit MapLibre rebuild target. `make requirements`,
  `make maplibre-requirements`, a dry-run `make maplibre-qt51518`, and the
  software-backend CTest suite all passed; CTest was 13/13 in 11.72 s. No
  MapLibre rebuild, application window, OpenGL path, or benchmark ran.
- The isolated `make review` gate then passed after formatting the adjacent
  software-backend assertion in `app/main.cpp`; no behavior changed.

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

## Documentation Planning

- Issue #43 records the evidence-backed Hanoi cartography audit and staged
  upgrade plan for the scoped follow-up sequence (#30--#32). PR #44 references
  it and is `In Progress` in Project #6 under `Post-demo stabilization`. It
  adds no runtime, package, rendering, or benchmark behavior; the i915 freeze
  remains in effect.
- On 2026-11-11, PR #44 was corrected to use the merged 07D product-style
  baseline: source-controlled class-aware roads are complete; the remaining
  implementation is atomic #30/#32 package migration followed by #31
  typography/labels. Its next dependency is the runtime fix in PR #42.
- On 2026-11-11, after #42 and #44 were squash-merged, work began on the
  atomic #30 + #32 migration from `origin/main`. The tracked source schema,
  product style, vector/raster validators, and test fixtures now use the v2
  contract (`building`, `landuse`, `place`, `transportation`, `water`).
  `java -jar ... verify` accepted the schema and the approved local PBF/JAR
  generated ignored `data/packages/hanoi-20261111-v2-r1` with no network,
  two threads, a 6 GiB heap cap, 59.289 s duration, and 2,454,948 KiB peak
  RSS. The first generated v2 package exposed a stale builder layer-schema
  value; it was retained as ignored diagnostic output, the builder was fixed,
  and only `v2-r1` became `current`.
- On 2026-11-11, following explicit owner authorization, the v2 renderer
  exposed a HiDPI 512×512 capture while the package contract requires 256×256.
  The renderer now accepts only an exact integral 2×--4× capture and smoothly
  downscales it to 256×256; other dimensions remain a hard failure. Rendering
  produced 847/847 256×256 local PNGs in 230.734 s. The builder and manifest
  now include required `sprite@2x.json` and `sprite@2x.png` resources so
  MapLibre no longer issues HiDPI sprite 404s.
- The ignored current vector package is
  `data/packages/hanoi-20261111-v2-r2`; its style and tiles are byte-identical
  to v2-r1, with the required HiDPI sprite resources added. The ignored current
  raster package is `data/raster-packages/hanoi-raster-20261111-v2-r2`: 847
  PNG tiles, 15,831,040 bytes, SHA-256
  `de4b199a708ce5ca1d1607919e3c2609bfa8298c6252f1aba0bba2a144fc0f08`, bound
  to vector manifest SHA-256
  `41370119febcdb3acc70134073837602a5647fbae6e6d60282f8a3c6c8d3bee9`.
  A 1366×768 vector → raster → vector review completed with the r2 packages,
  retained/released raster resources correctly, and had no sprite 404.
- The benchmark runner previously selected a stale Qt 5.15.13 runtime. It now
  queries the selected Qt prefix's qmake for QML/plugin paths, with a dedicated
  regression test. Xvfb is llvmpipe and its failing output is not performance
  evidence. On the physical display, Mesa Intel HD 520 acceleration was
  confirmed. Direct-GPU N=3 medians passed every locked threshold for v2
  pan/zoom (59.00 FPS average, 44.33 FPS p5, 117.09 MiB PSS peak, 12.53% CPU
  peak) and real-Valhalla navigation replay (59.25 FPS average, 52.46 FPS p5,
  485.54 MiB PSS peak, 21.63% CPU peak). This is focused current evidence, not
  a full Release matrix or soak; sampler-overhead collection was not run.
- The post-fix `make QT_EXPECTED_VERSION=5.15.18 test`, format check, and
  `git diff --check` must pass again after the final review commit.
- On 2026-11-12, #45 was squash-merged as `8fdfd3f`; #30 was closed and moved
  to Done in Project #6. #31 then moved to In Progress on its dedicated branch.
  It upgrades the contract to v3, uses local Noto Sans Regular (OFL-1.1), and
  emits only four required glyph ranges. The ignored
  `data/packages/hanoi-20261112-v3` vector package built offline in 62.458 s
  with two threads and a 6 GiB heap cap; it has the verified Noto font SHA-256
  `89c3c497f618fdaa0b2d1e98fef93582f28c71debd2c4a8cdf41f190ced2909d`.
  Non-render `make ... test` passed 16/16 in 12.40 s. The v2 raster package is
  correctly incompatible with this new contract.
- On 2026-11-12, after the owner authorized v3 rendering, a first glyph package
  was rejected visually: its PBF metrics included MapLibre's implicit 3px
  border, so the parser discarded glyphs. The generator and a structural test
  were fixed; only `hanoi-20261112-v3-r2` and
  `hanoi-raster-20261112-v3-r2` are valid evidence. The completed r2 render
  has 847/847 256×256 tiles, SHA-256
  `5055a3c30d9b68800beb0ce9fe6a9d70370c787eeb4573477076866c4333069d`, and a
  1366×768 vector → raster → vector runtime review passed. A z16 tile visibly
  shows Vietnamese road labels. A direct-GPU N=3 pan/zoom attempt could not
  connect to `DISPLAY=:0` without Xauthority and is excluded from performance
  claims; Xvfb is visual evidence only.
