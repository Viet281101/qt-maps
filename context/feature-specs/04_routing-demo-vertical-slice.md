# Unit 04: Routing Demo Vertical Slice

## Goal

Deliver the smallest honest end-to-end Hanoi routing and guidance flow for the
2026-11-03 demo. Establish a fixture-backed vertical slice first, then replace
its route source with a bounded localhost Valhalla backend without expanding
into search, production UI, nationwide graph generation, or unrelated map work.

## Ordered increments

### A. Route contract and fixtures

- Define route geometry, total distance/time, and maneuver type, instruction,
  street name, distance/time, shape index, and coordinate.
- Decode Valhalla polyline6 with explicit malformed-input errors.
- Parse representative Valhalla response JSON into the route contract.
- Keep one or two compact, hand-authored Hanoi fixtures clearly labeled as
  fixtures rather than backend-calculated routes.
- Stop for owner review after the Release build, Qt Test, evidence, and commits.

### B. Local Valhalla backend

- Check apt, official release/container, and source-build options before adding
  dependencies. If source build is required, report dependencies and estimate
  before proceeding.
- Build a Hanoi graph first with at most two threads and record `/usr/bin/time
  -v`, peak RSS, and graph size. Nationwide feasibility is a later observation,
  not a blocker; 10-12 GiB peak RSS is the review ceiling.
- Own a loopback-only child service, bounded readiness/health/shutdown, and an
  asynchronous route client with timeout and visible fixture fallback.
- Stop for an owner decision if the Hanoi graph is not working by day three.

### C-F. Demo UI, guidance, measurement, and reproducibility

- Add minimal origin/destination selection, route action, geometry, summary,
  maneuver list, and explicit fixture/backend/offline state.
- Replace the Unit 03 movement fixture with selected route geometry, camera
  follow, heading, next-maneuver distance, and adjustable replay speed.
  Re-routing is optional and must remain visibly incomplete if deferred.
- Benchmark the Release app plus Valhalla process tree; CPU/RSS are primary and
  FPS/frame time are responsiveness checks.
- Record a reproducible MapLibre v3 build/checksum and a clean-shell launch
  command that has been exercised successfully.

## Constraints

- Qt 5.15.13, Qt 5 CMake APIs, C++14, local/offline operation, and at most two
  graph-build threads.
- No routing graph, PBF, binary, cache, benchmark CSV, or third-party build tree
  enters Git. No public tile service or non-loopback routing endpoint is used.
- Each increment requires a Release build, `make test`, `git diff --check`,
  compact evidence, a progress-tracker update, and purpose-specific commits.
- No search, orthography, installer, nationwide graph, final UI polish, or other
  capability outside the requested vertical slice.

## Increment A acceptance

- [x] Route and maneuver value contract implemented in `src/routing/`.
- [x] Polyline6 decoding covers valid Hanoi geometry and malformed boundaries.
- [x] Two Valhalla-shaped, fixture-labeled Hanoi responses parse successfully.
- [x] Invalid units, shape indices, coordinate deltas, and malformed encodings
  fail explicitly.
- [x] Qt 5.15.13 Release build and all four CTest executables pass.
- [x] No backend, UI, data download, graph generation, push, or merge performed.
