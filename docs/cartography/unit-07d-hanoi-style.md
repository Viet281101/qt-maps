# Unit 07D — Hanoi Product Cartography

## Increment #29: source-controlled class-aware style

The product template is `resources/map/hanoi-style.json`. It is independent of
the synthetic vector fixture and contains only the local
`__QTMAPS_BASE_URL__` token for tiles, glyphs, and sprites. The package builder
canonicalizes the JSON through `tools/data-pipeline/build_hanoi_style.py`,
records the template checksum, and copies the rendered bytes unchanged into the
immutable package. Two renders from the same template are byte-identical.

The road hierarchy uses the existing `transportation.class` attribute:

| Group | OSM highway classes | Visibility and intent |
| --- | --- | --- |
| Major | motorway, trunk, primary | z12+, strongest casing/fill, 1.2→6 px fill |
| Secondary | secondary, tertiary | z12+, 0.7→3.8 px fill |
| Local | residential, unclassified, living_street, service | z13+, 0.35→2 px fill |
| Paths | path, footway, cycleway, pedestrian, steps | z15, 0.8 px dashed line |

This replaces the former five-pixel near-white rule for every highway. Building
color/outline and label/halo tokens are explicit. A line-placed road-label
layer is present but has `visibility: none`; #31 may enable it only after a font
is approved and local Vietnamese glyphs exist. The existing tiny `HÀ NỘI`
diagnostic glyph remains temporary.

The template retains the active `qtmaps-style-v1` contract and only uses the
existing `building`, `place`, and `transportation` source layers. #32 owns any
future layer-schema or style-contract migration; the current checked package
and rollback artifacts are not modified.

Focused tests validate deterministic output, loopback-only URLs, attribution,
road class/width hierarchy, disabled pre-glyph labels, and independence of the
synthetic fixture.
