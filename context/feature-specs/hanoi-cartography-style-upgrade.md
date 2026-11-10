# Hanoi Cartography and Vector Style Upgrade

## Status and purpose

This is approved backlog planning, not active implementation. Improve the
bounded Hanoi map from a renderer-feasibility style into a legible local
navigation basemap while preserving offline operation, attribution, package
integrity, and performance evidence.

The evidence and corrections behind this scope are in
`docs/map/hanoi-vector-style-audit.md`. Implement on a dedicated branch from the
then-current `main`; do not mix the work into Unit 06 PR #23 or the Pre-06
repository-maintenance epic without an explicit owner decision.

## Increment A: maintainable style source

- Move the real Hanoi style definition out of the synthetic fixture generator
  into a source-controlled product style template.
- Keep the package builder responsible for deterministic local URL-token
  substitution, checksums, resource revisions, and immutable output.
- Preserve the synthetic vector fixture and its minimal diagnostic resources as
  an independent test path.
- Define light map tokens for land, water, green space, buildings, road casing,
  road fill by class, labels, halos, route contrast, and attribution contrast.

## Increment B: minimum contextual tile schema

- Add water polygons and selected landuse/park polygons using only the approved
  local OSM PBF and documented tags.
- Keep the existing `transportation` attributes and use `class` for road
  hierarchy. Add schema attributes only when a style rule consumes them.
- Expand `place` classes for district/neighborhood context only after defining
  collision, zoom, language fallback, and density rules.
- Do not add `transportation_name` merely to duplicate existing road names.
- Defer POIs until their product value, categories, icon source/license, zoom
  rules, and density limits are approved.

## Increment C: road hierarchy and labels

- Use class- and zoom-aware casing/fill widths so local roads do not dominate
  the map and primary roads remain recognizable.
- Render water and selected green/landuse areas with contrast that leaves the
  blue route plus white outline clearly visible.
- Add general street labels from `transportation.name` with line placement and
  bounded zoom/density rules.
- Prefer `name_vi`, then `name`, for place labels when both are available.
- Keep route-derived QML street labels distinct from basemap labels and review
  whether they remain useful after the basemap gains names.

## Increment D: Vietnamese typography and optional icons

- Select a redistributable font with Vietnamese coverage and record its license,
  source, version, checksum, attribution, and packaging method.
- Generate only the required glyph ranges reproducibly; do not rely on the
  handcrafted `HÀ NỘI` diagnostic glyph set for product labels.
- Add sprite icons only for approved features. A POI layer is not justified by
  an existing marker sprite alone.

## Increment E: contract migration and evidence

- Bump `qtmaps-style-v1` and layer-schema version 1 together when the accepted
  source-layer set changes; define compatibility/failure behavior explicitly.
- Update `SOURCE_LAYERS`, package/resource revisions, `MapPackageManifest`, and
  positive/negative tests without weakening checksum or loopback URL checks.
- Rebuild the ignored immutable Hanoi package and verify reproducibility from
  the approved local inputs. Generated MBTiles, glyphs, sprites, screenshots,
  and benchmark outputs remain outside Git.
- Capture before/after screenshots at fixed camera states and check label
  legibility, collision, road/route distinction, attribution, and 900x580 plus
  1366x768 layouts.
- Run `make check`, `make review`, and the locked Unit 06 Release benchmark
  matrix. Report any threshold failure without changing the threshold.

## Acceptance criteria

- Water and selected green/landuse areas are visually distinct from ordinary
  land using the bounded local package with no public tile access.
- Road classes remain recognizable across supported zooms without a near-white
  blanket effect; the selected route stays visually dominant.
- Vietnamese place and street labels are readable, licensed, locally packaged,
  and bounded by collision/zoom rules.
- Package validation rejects incompatible contract/layer/resource combinations
  and accepts a clean reproducible v2 package.
- Default routing, navigation replay, re-routing, offline mode, attribution,
  fallback behavior, and clean shutdown remain intact.
- Documentation records the schema, style source, font/license, build command,
  visual evidence, benchmark comparison, and known omissions.

## Non-goals

- No nationwide high-detail package in this work.
- No public tile/style/font/sprite endpoint.
- No dark theme, satellite/orthography implementation, or full POI product
  unless separately approved.
- No renderer replacement and no Qt 6 migration.
