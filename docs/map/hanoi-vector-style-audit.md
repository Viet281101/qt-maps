# Hanoi Cartography Follow-up Audit

Audit date: 2026-11-14

## Verdict

Unit 07D replaced the former synthetic five-pixel road presentation with a
source-controlled, class- and zoom-aware Hanoi product style. The active source
and runtime contract now require `qtmaps-style-v3` with five bounded source
layers and local Vietnamese-capable glyph resources. The ignored v3 vector
package is generated from the approved local PBF; the v2 raster package is
intentionally rejected until a matching v3 raster is owner-authorized. An
existing v1 or v2 package is rejected explicitly.
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
- Noto Sans Regular (OFL-1.1) supplies reproducible local Vietnamese glyph
  ranges. Road labels are bounded to major/secondary classes from zoom 14;
  city/town labels prefer `name_vi`, then `name`. The small `HÀ NỘI` fixture is
  diagnostic-only and is not used by the production package.
- Route-derived street labels are presentation overlays in
  `qml/components/MapCanvas.qml` at zoom 15 and above. They are not general
  basemap labels.

## Validated package lineage

The checked-in v3 style/glyph contract is coupled to the manifest validators.
The superseded `hanoi-20261112-v3-r2` vector package contains four Noto glyph
ranges and is deliberately rejected by the v2 glyph revision. The current
`hanoi-20261114-v3-r3` vector package was generated with the pinned Planetiler
JAR, two threads, a 6 GiB heap cap, and no network access; it contains all five
declared vector layers and the fifth `8192-8447` range for U+2013 EN DASH. Its
matching `hanoi-raster-20261114-v3-r3` package has 847 256×256 tiles rendered
locally by MapLibre. A fixed z15 MapLibre capture shows `Đường Long Biên – Xuân
Quan` with its EN DASH; vector startup and a forced-raster fallback both reached
`mapReady=true` without a glyph-resource error.

## Follow-up sequence

1. **#30:** complete. Bounded water and landuse layers are in the accepted
   local package contract.
2. **#31, typography and labels:** complete and squash-merged as PR #46.
3. **#32, verification:** the owner-authorized v3-r2 direct-GPU Release matrix
   completed all 30 combinations of five scenarios, two modes, and three runs;
   every locked threshold passed. A soak remains outside this evidence.

## Non-goals

- No public tile, style, font or sprite endpoint.
- No nationwide high-detail expansion, dark map, satellite layer, or POI
  product in these follow-ups.
- No renderer replacement or Qt 6 migration.
