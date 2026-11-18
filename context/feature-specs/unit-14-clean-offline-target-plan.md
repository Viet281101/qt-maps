# Unit 14 — Clean-target offline delivery verification

## Status

Proposed follow-up to Unit 13. This is a decision and verification plan, not
authorization to install software, change network settings, download or generate
data, copy protected datasets, or run performance benchmarks. Resolve Unit 13's
review findings and complete its review and merge before starting this unit.

## Objective and motivation

Prove that an operator can provision and run the existing bounded offline demo
on a separate, clean supported Linux target without relying on the development
machine's ignored paths, pre-existing packages, or network access. Unit 13's
local walkthrough proves runtime handoff on the reference machine; it does not
establish portability or clean-target delivery. The outcome must state the
actual target and coverage and distinguish real data, simulation and fixtures.

## Decisions and prerequisites

- The owner selects and authorizes an isolated clean Linux target, access method,
  destination paths, and how the offline condition will be established and
  independently verified without disrupting existing networks or user data.
- Inventory the approved Qt 5.15.18, MapLibre runtime, Valhalla executable and
  config, SQLite/FTS5 support, fonts, system libraries, and licenses. Record the
  required origin and transfer method. Installation, transfers and dependency
  changes require the applicable owner authorization before execution.
- Identify the exact owner-provided map/search bundle and independent routing
  graph, versions, coverage, manifests, checksums, attribution and redistribution
  permissions. Orthography is optional and needs explicit permission if used.
- Agree on storage budget, expected supported configuration, who performs each
  operation, and cleanup/rollback boundaries. Do not modify source datasets or
  commit generated packages, credentials, configuration or target-specific data.

## Ordered verification slices, after prerequisites are approved

1. Produce a portable inventory and an operator checklist from the existing
   Unit 12 and Unit 13 contracts. Record missing runtime dependencies and paths;
   do not assume the development machine's `generated/` tree exists on target.
2. Verify and explicitly transfer only approved runtime artifacts and local
   bundles to the selected target. Record version, provenance, size, checksum,
   license and attribution for every required artifact, without repackaging or
   rebuilding unless separately approved.
3. Establish and document the approved offline condition. From a clean target,
   activate the map/search bundle and separate graph through the existing CLI;
   start from activation roots without component-path duplication. Capture the
   actual config/graph pairing and verify override, missing/corrupt and rollback
   behavior without damaging the operator's data.
4. Exercise vector and raster, optional orthography or its honest unavailable
   state, offline search, a real local route and explicitly simulated navigation.
   Record visible errors, coverage, attribution and any fixture fallback. Repeat
   startup after a restart and verify no external network dependency is needed.
5. Publish a concise dated functional walkthrough with exact commands, target
   capabilities, evidence and limitations. Record defects as follow-up work;
   do not characterize a partial, fixture-backed or failed run as offline proof.

## Completion criteria and boundaries

A repeatable clean-target checklist and functional evidence demonstrate the
above operator flow using only authorized local inputs and accurately report
missing or degraded states. Unit 14 does not claim nationwide routing, full
Vietnam map coverage, real GNSS, a new installer, hosting, or performance
acceptance. The i915 benchmark freeze remains in force. Nationwide routing,
real-route navigation acceptance and performance evidence remain separate
work with their own decisions and gates.
