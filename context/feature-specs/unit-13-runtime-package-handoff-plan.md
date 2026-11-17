# Unit 13 — Runtime handoff for activated local packages

## Status

Proposed follow-up to Unit 12. It is planning-only: no data generation,
download, benchmark, Valhalla rebuild, or production configuration rewrite is
authorized by this document.

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

## Owner decision gate

Before implementation, approve one routing-config strategy:

1. **Recommended:** create an ignored, app-owned runtime copy of an approved
   loopback-only Valhalla config, changing only `mjolnir.tile_dir` to the
   validated active routing graph path. The source config remains untouched.
2. Require the operator to provide a Valhalla config that already targets the
   active graph, and expose an explicit mismatch state when it does not.

Also confirm the supported activation-root location and whether a map/search
bundle with no optional orthography remains valid for normal startup.

## Ordered implementation slices after approval

1. Add a pure resolver for an active bundle pointer that returns validated
   vector, raster, search, and optional orthography paths. Explicit CLI paths
   retain priority over the resolved bundle paths.
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
