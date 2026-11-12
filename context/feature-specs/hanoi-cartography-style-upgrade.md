# Hanoi Cartography Follow-up Plan

## Status and purpose

Unit 07D delivered the source-controlled, class-aware Hanoi v1 style.
Increment A is implemented on a dedicated branch from current `main`; it covers
the bounded-data context and atomic package-contract migration. Increment B
remains planned. Both preserve offline operation, attribution, package
integrity, and performance evidence.

The factual baseline is `docs/map/hanoi-vector-style-audit.md`. Implement each
increment on a dedicated branch from then-current `main`; do not revive the
archived `unit-07e-verify` branch or mix this work with i915 investigation.

## Increment A: atomic contextual-package migration (#30 + #32)

- Activate bounded `water` and selected `landuse`/park polygons using only the
  approved local OSM PBF and documented tag allowlists.
- Keep `transportation.class` and the existing class-aware style; do not add a
  duplicate `transportation_name` layer.
- Version the source-layer set and style contract together. Update
  `SOURCE_LAYERS`, resource/package revisions, `MapPackageManifest`, and
  positive/negative tests without weakening checksum or loopback URL checks.
- Rebuild ignored vector and raster packages reproducibly from approved local
  inputs only after confirming the local inputs and available disk space. This
  is complete for `hanoi-20261111-v2-r2` and
  `hanoi-raster-20261111-v2-r2`; a v1/v2 mismatch is a visible rejection,
  never a fallback.
- Owner-authorized fixed-camera/runtime review and focused direct-GPU N=3
  pan/zoom plus real-Valhalla navigation evidence are complete. A full Release
  matrix and soak remain out of scope until separately authorized.

## Increment B: Vietnamese typography and bounded labels (#31, in progress)

- Noto Sans Regular (OFL-1.1) is selected; its source, package version,
  checksum, and local packaging method are in `docs/map/hanoi-vietnamese-typography.md`.
- Generate only the documented Latin/combining/Vietnamese ranges reproducibly;
  do not use the handcrafted `HÀ NỘI` diagnostic glyph set for product labels.
- Enable street labels from `transportation.name` for bounded major/secondary
  classes at zoom 14, and city/town labels preferring `name_vi`, then `name`.
- Keep route-derived QML labels distinct from basemap labels and reassess their
  value after general labels become available.
- Defer POIs and sprites unless their product value, licensing, zoom rules and
  density limits are approved.

## Acceptance criteria

- Water and selected green/landuse areas are visually distinct in the bounded
  local package without public tile access.
- The existing road hierarchy and selected route remain legible at supported
  zooms and viewports.
- Vietnamese labels are licensed, locally packaged, collision-bounded and
  reproducible.
- Package validation rejects incompatible contract/layer/resource combinations
  and accepts a clean reproducible migrated package.
- Routing, navigation replay, offline fallback, attribution and shutdown remain
  intact. Performance claims use only owner-authorized current measurements.

## Non-goals

- No nationwide high-detail package, dark theme, satellite/orthography work or
  full POI product.
- No renderer replacement, Qt 6 migration, GPU configuration change or i915
  investigation.
