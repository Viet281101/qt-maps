# Progress Tracker

This is the concise operational snapshot. The prior detailed outcome log is
preserved in Git history through `e0a8086`.

## Current Phase

- Units 03--05 are squash-merged into `main`; Unit 06 performance work is also
  present on `origin/main`.
- Unit 07 is a local, dependency-ordered stack. Its branches have not yet been
  pushed and no Unit 07 pull request is open.
- Qt 5.15.18 is the production baseline. Qt 5.15.13 is comparison-only.

## Current Goal

Publish the Qt 5.15.18 documentation alignment and make a remote backup of the
Unit 07 branch stack through focused pull requests. Do not push to `main`.

## Unit 07 Status

| Unit | Branch / head | State |
| --- | --- | --- |
| 07A | `chore/07a-repo-structure-maintenance` / `cccfcb7` | first stack branch; local only |
| 07B | `feat/07b-demo-shell-ui` / `1ce45e4` | depends on 07A; local only |
| 07C | `feat/07c-offline-raster` / `a4227ba` | depends on 07B; local only |
| 07D | `feat/07d-hanoi-cartography` / `bd854fa` | depends on 07C; local only |
| 07E | `unit-07e-verify` / `50fc763` | depends on 07D; local only |

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

1. Commit, push, and open the Qt 5.15.18 alignment documentation PR.
2. Push the five Unit 07 branches in dependency order as remote backups.
3. Create focused stack PRs, update the tracker with their live URLs and
   publication state, then verify the linked issue/milestone/Project metadata.

## Latest Verification

- On 2026-11-07, `make QT_EXPECTED_VERSION=5.15.18 test` passed all 13 CTest
  tests without a render benchmark.
- Remote `origin/main` is `e0a8086`; the remote currently exposes only `main`.
