# Hanoi Cartography Follow-up Audit

Audit date: 2026-11-11

## Verdict

Unit 07D replaced the former synthetic five-pixel road presentation with a
source-controlled, class- and zoom-aware Hanoi product style. The active source
and runtime contract now require `qtmaps-style-v2` with five bounded source
layers. The ignored v2 vector package is generated from the approved local PBF;
the matching raster package has been regenerated and runtime-tested. An
existing v1 package is rejected explicitly.
There is no evidence that a renderer defect is needed to explain the remaining
typography/label omission.

## Current production state

- `resources/map/hanoi-style.json` is the canonical product template. The
  package builder validates and canonicalizes it with
  `tools/data-pipeline/build_hanoi_style.py`; the synthetic fixture provides
  only diagnostic resources and does not define the product style.
- The v2 package contract contains `building`, `landuse`, `place`,
  `transportation`, and `water`. `landuse` and `water` use documented polygon
  allowlists only; POIs and extra name attributes are not added.
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

The checked-in v2 schema and style are coupled to the manifest validators. The
local `hanoi-20261111-v2-r2` vector package was generated with the pinned
Planetiler JAR, two threads, a 6 GiB heap cap, and no network access; it
contains all five declared vector layers. Its matching
`hanoi-raster-20261111-v2-r2` package contains 847 256×256 tiles bound to the
same vector manifest. A 1366×768 vector → raster → vector runtime review
completed without a HiDPI sprite request failure and released raster resources
on return to vector.

## Follow-up sequence

1. **#30 + #32, one active atomic migration:** activate bounded water and
   landuse layers, version the source-layer/style contract together, update
   validation, rebuild ignored vector/raster packages, and record
   reproducibility and negative compatibility evidence.
2. **#31, separate typography and labels:** select and document a licensed
   Vietnamese font, generate glyph ranges reproducibly, then enable bounded
   road/place labels with collision and zoom rules.
3. **Visual/performance evidence:** owner-authorized focused v2 direct-GPU
   pan/zoom and real-Valhalla navigation runs now pass their locked thresholds.
   A full Release matrix or soak remains outside this authorization while the
   i915 blocker is open.

## Non-goals

- No public tile, style, font or sprite endpoint.
- No nationwide high-detail expansion, dark map, satellite layer, or POI
  product in these follow-ups.
- No renderer replacement or Qt 6 migration.
