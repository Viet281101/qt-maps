# Unit 12 — Local package delivery decision plan

## Status

Owner-approved implementation plan tracked by
[#65](https://github.com/Viet281101/qt-maps/issues/65). Unit 11 is present on
the local `main` baseline. This plan authorizes only the local import and
activation path described here; it does not authorize a downloader, hosted
distribution, package regeneration, or changes to protected datasets.

Slice 1 is implemented locally: `OfflinePackageContract` reads and rejects
unsafe/incomplete versioned map/search bundle and routing-graph activation
metadata without changing existing package schemas or loading data. Its focused
Qt 5.15.18 Release test passed on 2026-11-17. Slice 2 (staged import and atomic
activation) is now implemented locally: `OfflinePackageInstaller` validates a
candidate through the existing vector/raster/search/orthography validators,
then switches a local `current` pointer atomically and retains `previous` for
explicit rollback. It links operator-provided local data rather than copying,
regenerating, or downloading it. Focused activation/rollback tests passed on
2026-11-17. Slice 3 is implemented locally: the routing-graph contract checks
that its separately provisioned graph directory exists and is non-empty, then
activates a separate `routing-current.json` pointer and reports the active
graph path. It does not alter the running Valhalla configuration or route
requests. Focused tests passed on 2026-11-17. Slice 4 (readiness UI and
operator workflow) is implemented locally: Unit 11 readiness now checks the
separate routing activation root on its existing worker thread and shows an
optional Offline routing item with recovery guidance. The application accepts
`--routing-activation-root`; no runtime routing behavior changes. The full
Qt 5.15.18 Release suite passed 25/25 on 2026-11-17.

The local functional walkthrough is recorded in
`docs/rehearsal/unit-12-offline-walkthrough-20261117.md`: owner-provisioned
map/search and routing artifacts were validated and activated through the
explicit CLI, then a local Valhalla route returned 7,294 m and 14 maneuvers.
This is not a benchmark or clean-target transfer result.

## Local implementation completion

Unit 12 is complete locally: its versioned contracts, explicit activation CLI,
integrity checks, insufficient-storage rejection, independent map/search and
routing rollback pointers, readiness UI, operator checklist, focused tests,
and local-only walkthrough are complete. A repeat on a separate clean Linux
target remains deployment evidence, not outstanding implementation work.

## Objective

Decide how an operator obtains, verifies, installs, updates, and recovers the
offline vector, raster, search, orthography, and routing data needed by QtMaps.
Unit 11 already reports readiness; Unit 12 should turn that report into a
repeatable, documented provisioning path without hiding data provenance.

## Owner-approved delivery decision

1. **Distribution:** the owner supplies local files. QtMaps has no hosted,
   downloadable, installer-managed, or automatic-update payload.
2. **Package composition:** a map/search bundle contains the exact compatible
   vector and raster pair plus the search package. Orthography remains an
   optional owner-provisioned component: it can be imported only when its
   manifest, attribution, and license record are present. The Valhalla routing
   graph is a separate local artifact, activated independently.
3. **Licensing:** each imported artifact must retain its source, retrieval
   date, coverage, license/attribution, checksum, and generator metadata.
   Unit 12 does not redistribute Sentinel-2 imagery or any other protected
   data; the operator remains responsible for supplying only authorized files.
4. **Integrity and lifecycle:** import stages a candidate outside the active
   location, runs the existing validators/checksums and cross-package
   compatibility checks, then atomically activates it. Retain exactly one
   prior valid map/search bundle for explicit rollback. Updates are manual,
   explicit imports; cleanup never removes the active or rollback bundle.
   Every manifest declares its installed size and required free space, so the
   importer can reject insufficient storage without inventing a global quota.
5. **Acceptance target:** a clean Linux target installs owner-supplied local
   artifacts, starts the full supported demo with networking disabled, and
   keeps missing, corrupt, incompatible, or absent optional components visible.

## Ordered implementation slices after approval

1. Record a versioned map/search bundle contract and a separate routing-graph
   activation contract. Both reference existing validators and checksums; do
   not replace their schemas. **Start here.**
2. Add an explicit local map/search import path with staged validation, atomic
   activation, and one recoverable prior version. Preserve attribution and
   provenance; do not copy or regenerate datasets.
3. Add separate routing-graph discovery/activation and report its readiness
   without coupling graph replacement to map/search rollback.
4. Connect each outcome to Unit 11 readiness with actionable error text and an
   operator checklist for a clean offline start.
5. Verify valid, missing, corrupt, incompatible, interrupted, insufficient-
   storage, and rollback scenarios using small fixtures; then run the Qt
   5.15.18 Release review and functional offline walkthrough.

## Boundaries

- No new runtime network fetch, public tile server, credentials in Git, or
  automatic data acquisition is implied by this plan.
- Keep data and generated bundles outside Git. Do not regenerate protected
  datasets or edit third-party artifacts by hand.
- Keep the active i915 benchmark freeze. A performance measurement requires
  separate approval and a documented scope.
- Do not change #65's live GitHub status until its canonical implementation PR
  is created and linked; verify the resulting live state after that mutation.

## Completion criteria

The approved package path can be repeated from a clean local installation,
integrity and compatibility failures remain visible, a previous valid bundle
can be restored, and the full demo starts with the network disabled. Record
exact commands, evidence, disk use, licensing, and unresolved limits.

## Clean offline operator checklist

1. Obtain only owner-provisioned map/search and routing-graph artifacts; verify
   their license, attribution, provenance, checksum, and declared size.
2. Validate a candidate map/search bundle before activation; activate it only
   through the explicit local import path and retain `previous` for rollback.
3. Validate and activate the routing graph separately; pass its activation root
   with `--routing-activation-root`.
4. Start QtMaps with networking disabled and inspect Settings: vector is
   required; raster, search, orthography, and routing accurately show ready,
   missing, or invalid state.
5. If a candidate fails or the demo regresses, use the retained previous bundle
   or graph activation pointer; do not edit manifests, generated data, or
   third-party artifacts by hand.
