# Progress Tracker

This is the concise operational snapshot. The prior detailed outcome log is
preserved in Git history through `e0a8086`.

## Current Phase

- Units 03--05 are squash-merged into `main`; Unit 06 performance work is also
  present on `origin/main`.
- Unit 07 is a dependency-ordered stack whose five branches have remote backup
  copies. No Unit 07 pull request is open.
- Qt 5.15.18 is the production baseline. Qt 5.15.13 is comparison-only.

## Current Goal

Review PR #33, then prepare focused Unit 07 pull requests. Do not push to
`main`.

## Publication Status

- PR [#33](https://github.com/Viet281101/qt-maps/pull/33),
  `docs/qt-5-15-18-alignment` → `main`, was squash-merged as `4193f07`.
- PR [#34](https://github.com/Viet281101/qt-maps/pull/34),
  `chore/07a-repo-structure-maintenance` → `main`, was squash-merged as
  `4a61d46`.
- PR [#35](https://github.com/Viet281101/qt-maps/pull/35),
  `feat/07b-demo-shell-ui` → `main`, is open, assigned to `Viet281101`,
  linked to Issue #3, and is `In Progress` on Project #6 with P0 / Week 1
  fields.
- The Unit 07 remote backups are `origin/chore/07a-repo-structure-maintenance`,
  `origin/feat/07b-demo-shell-ui`, `origin/feat/07c-offline-raster`,
  `origin/feat/07d-hanoi-cartography`, and `origin/unit-07e-verify`.

## Unit 07 Status

| Unit | Branch / head | State |
| --- | --- | --- |
| 07A | `chore/07a-repo-structure-maintenance` / `4a61d46` | merged as PR #34 |
| 07B | `feat/07b-demo-shell-ui` / `8d8bde6` | PR #35 open; follows 07A |
| 07C | `feat/07c-offline-raster` / `a4227ba` | pushed; depends on 07B |
| 07D | `feat/07d-hanoi-cartography` / `bd854fa` | pushed; depends on 07C |
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

1. Review PR #35 without merging directly to `main`.
2. Create focused Unit 07 stack PRs after 07B, update the tracker with their live URLs
   and publication state, then verify the linked issue/milestone/Project metadata.

## Latest Verification

- On 2026-11-07, `make QT_EXPECTED_VERSION=5.15.18 test` passed all 13 CTest
  tests without a render benchmark.
- Remote `origin/main` is `e0a8086`; the remote currently exposes only `main`.
