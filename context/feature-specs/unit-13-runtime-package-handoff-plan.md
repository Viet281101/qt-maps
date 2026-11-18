# Unit 13 — Runtime handoff for activated local packages

## Status

Owner approved implementation on 2026-11-17; implemented locally and isolated
review found startup, pointer-race and degraded-mode defects. Local fixes and
self-contained tests are in [PR #70](https://github.com/Viet281101/qt-maps/pull/70).
Initial validation is cancellable, and vector tile-source startup has a bounded,
responsive event loop with timeout-safe worker ownership. Independent review
remains necessary before merge. Evidence is in `docs/rehearsal/unit-13-runtime-handoff-20261117.md`.
No data generation, download, benchmark, Valhalla rebuild, or source
configuration rewrite is authorized.

## Objective

Make the owner-provisioned artifacts activated by Unit 12 the explicit runtime
inputs of QtMaps, while preserving every current command-line override and
accurately reporting a routing graph that is installed but not yet selected by
the running Valhalla configuration.

## Problem to solve

Unit 12 validates and atomically activates local map/search and routing-graph
contracts. The app currently accepts those pointers for readiness reporting,
but normal map/search command-line defaults and `valhalla.json` retain their
own paths. A routing graph package can therefore be valid without being the
graph used by a route request. Unit 13 closes this handoff gap; it must not
describe metadata activation as live routing activation before that happens.

## Owner-approved decisions
Use an app-owned ignored runtime copy of the approved loopback-only Valhalla
configuration, changing only `mjolnir.tile_dir` to the validated active graph.
Preserve the source config and explicit CLI overrides. Use the existing default
activation roots `data/package-activation` and `data/routing-activation`, with
CLI overrides; relative paths resolve from the process working directory. A
bundle without orthography is valid and starts with that layer unavailable.
The demo launcher must not inject component overrides that mask activation.
Explicit vector paths are validated independently and remain usable if bundle
metadata stalls; the separately activated graph is also checked independently.
A bundle is skipped when all component paths are overridden. Bundle/vector and
routing checks run in independent cancellable helpers; a stalled check cannot
discard the other result. Fixture-only routing does not read the routing graph.
An explicit `--valhalla-config` remains untouched and is passed through as-is;
if it does not name the active graph, readiness reports installed but unselected.

## Ordered implementation slices

1. Snapshot the active bundle pointer and validate its metadata once. Resolve
   vector, raster, search, and optional orthography paths from that snapshot;
   existing consumers independently validate each component, preserving degraded
   operation and vector/raster compatibility. Explicit CLI paths take priority.
2. Feed those resolved paths into the existing map, search, and Unit 11
   readiness boundaries without blocking the UI thread. Test active, missing,
   corrupt, and rollback-pointer states.
3. Implement only the owner-approved routing-config strategy. Verify that the
   actual config passed to Valhalla names the validated graph path; otherwise
   show an installed-but-not-selected state rather than a ready routing state.
4. Run a local-only functional route through the selected graph, then repeat
   the clean offline operator walkthrough. Do not run a benchmark under the
   i915 freeze.

## Completion criteria

The application can start from Unit 12 activation roots without duplicated
component paths, explicit overrides still win, and UI status distinguishes
missing, invalid, installed, selected, and unavailable routing states. A route
returned by Valhalla is evidenced to use the selected graph/config pair. All
generated runtime config and data remain outside Git.
