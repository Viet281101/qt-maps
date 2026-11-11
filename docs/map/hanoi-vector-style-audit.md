# Hanoi Cartography Follow-up Audit

Audit date: 2026-11-11

## Verdict

Unit 07D already replaced the former synthetic five-pixel road presentation
with a source-controlled, class- and zoom-aware Hanoi product style. The active
runtime contract remains `qtmaps-style-v1` with three source layers, so water,
landuse context and general Vietnamese basemap labels are intentionally not yet
visible. There is no evidence that a renderer defect is needed to explain the
remaining omissions.

## Current production state

- `resources/map/hanoi-style.json` is the canonical product template. The
  package builder validates and canonicalizes it with
  `tools/data-pipeline/build_hanoi_style.py`; the synthetic fixture provides
  only diagnostic resources and does not define the product style.
- The active v1 package contains `building`, `transportation`, and `place`.
  `transportation.class` drives major, secondary, local and path styling with
  zoom-aware widths. The template uses only local URL tokens and preserves OSM
  attribution.
- A line-placed road-label layer exists but is disabled until #31 supplies a
  licensed Vietnamese-capable font and reproducible glyph ranges. The small
  `HÀ NỘI` glyph fixture is diagnostic-only.
- Route-derived street labels are presentation overlays in
  `qml/components/MapCanvas.qml` at zoom 15 and above. They are not general
  basemap labels.

## Remaining gap

The bounded local data has no active `water` or `landuse` source layers, so the
map cannot distinguish those areas from ordinary land. A draft v2 Planetiler
configuration exists, but it is deliberately inactive until the package/style
contract migration is accepted. Activating it without the matching manifest,
validator, vector package and raster-package regeneration would violate the
runtime contract.

## Follow-up sequence

1. **#30 + #32, one atomic migration:** activate bounded water and landuse
   layers, version the source-layer/style contract together, update validation,
   rebuild ignored vector/raster packages, and record reproducibility and
   negative compatibility evidence.
2. **#31, separate typography and labels:** select and document a licensed
   Vietnamese font, generate glyph ranges reproducibly, then enable bounded
   road/place labels with collision and zoom rules.
3. **Visual/performance evidence:** capture fixed camera reviews and run the
   locked Release matrix only after explicit owner approval lifts the i915
   render-benchmark freeze. Historical results are not current evidence.

## Non-goals

- No public tile, style, font or sprite endpoint.
- No nationwide high-detail expansion, dark map, satellite layer, or POI
  product in these follow-ups.
- No renderer replacement or Qt 6 migration.
