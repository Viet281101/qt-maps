# Unit 12 — Local package delivery decision plan

## Status

Proposed planning unit tracked by [#65](https://github.com/Viet281101/qt-maps/issues/65),
after Unit 11 review and merge. This document does not
authorize distribution, download, package regeneration, or installation work.

## Objective

Decide how an operator obtains, verifies, installs, updates, and recovers the
offline vector, raster, search, orthography, and routing data needed by QtMaps.
Unit 11 already reports readiness; Unit 12 should turn that report into a
repeatable, documented provisioning path without hiding data provenance.

## Decision gate for the owner

Before implementation, approve:

1. Distribution model: owner-provisioned local files, a separate downloadable
   bundle, or an installer-managed payload. State who hosts and maintains it.
2. Package set and version pairing: which components are required, which are
   optional, how the raster/vector compatibility pair is selected, and whether
   a routing graph belongs to the same bundle.
3. License and attribution review for every source, generated artifact, and
   redistribution channel, especially Sentinel-2 orthography.
4. Storage, integrity, update, rollback, and cleanup limits, including the
   expected disk budget on the reference laptop.
5. Offline acceptance scenario and supported installation target. Any
   download path must be separately approved; the application must never
   silently fetch map data.

## Ordered implementation slices after approval

1. Record a versioned bundle contract that references existing package
   validators and checksums; do not replace their schemas.
2. Add an explicit local import/install path with atomic activation and a
   recoverable previous version. Preserve package attribution and provenance.
3. Connect install outcomes to Unit 11 readiness, with actionable error text
   and an operator checklist for a clean offline start.
4. Verify valid, missing, corrupt, incompatible, interrupted, and rollback
   scenarios using small fixtures; then run the Qt 5.15.18 Release review and
   functional offline walkthrough.

## Boundaries

- No new runtime network fetch, public tile server, credentials in Git, or
  automatic data acquisition is implied by this plan.
- Keep data and generated bundles outside Git. Do not regenerate protected
  datasets or edit third-party artifacts by hand.
- Keep the active i915 benchmark freeze. A performance measurement requires
  separate approval and a documented scope.
- Do not implement this plan or mark its issue `In Progress` before the owner
  resolves the decision gate.

## Completion criteria

The approved package path can be repeated from a clean local installation,
integrity and compatibility failures remain visible, a previous valid bundle
can be restored, and the full demo starts with the network disabled. Record
exact commands, evidence, disk use, licensing, and unresolved limits.
