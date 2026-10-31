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

The template declares draft revision `qtmaps-hanoi-v2-draft`. Packages built
before #32 will therefore fail the current v1 runtime contract by design; #32
owns the coordinated style/layer schema migration and package regeneration.
The current checked package and rollback artifacts are not modified.

Focused tests validate deterministic output, loopback-only URLs, attribution,
road class/width hierarchy, disabled pre-glyph labels, and independence of the
synthetic fixture.

## Increment #30: bounded contextual polygons

`tools/data-pipeline/unit07d-hanoi-v2.yml` is the draft layer-schema v2 input.
It is deliberately separate from the active v1 schema until #32 migrates the
manifest/runtime contract and rebuilds the immutable package. Both schemas use
only the approved local Geofabrik-derived PBF; neither adds a remote source.

The added polygon rules are deliberately small whitelists:

| Source layer | OSM tags | Min zoom | Minimum size |
| --- | --- | ---: | ---: |
| `water` | `natural=water`; `waterway=riverbank`; `landuse=reservoir|basin` | 12 | 4 tile pixels |
| `landuse` | `leisure=park|garden|nature_reserve`; `landuse=grass|recreation_ground|forest`; `natural=wood` | 13 | 6 tile pixels |

Each feature exposes only a bounded `class` value derived from the matched tag.
Names are not duplicated into these layers because no accepted label rule uses
them yet. The existing `place` layer continues to carry `name_vi` and `name`;
#31 owns the visible Vietnamese-first fallback. District/neighborhood expansion
and every POI remain deferred. The product style consumes the two layers as
pale-blue water and pale-green areas, below buildings and roads so the blue
route with its white outline remains the dominant navigation feature. The
global package maximum remains z15.

The local Planetiler 0.10.2 `verify` command accepts the draft schema. A direct
ignored build used the pinned jar and approved PBF with bounds
`105.78,20.97,105.90,21.10`, z12-z15, two threads, a 6 GiB heap cap, uncompressed
MVT, and both download/refresh disabled. It completed in 61.56 seconds with
3,186,956 KiB peak RSS. The output contains the same 226 tiles as v1.

| Layer | v1 encoded instances | Draft v2 encoded instances | Change |
| --- | ---: | ---: | ---: |
| `building` | 178,812 | 178,812 | 0 |
| `place` | 4 | 4 | 0 |
| `transportation` | 241,185 | 241,185 | 0 |
| `landuse` | 0 | 3,417 | +3,417 |
| `water` | 0 | 2,469 | +2,469 |

These are encoded tile feature instances, so a source feature crossing a tile or
zoom boundary can be counted more than once. Total instances grow by 1.40%.
MBTiles grows from 14,675,968 to 15,007,744 bytes: +331,776 bytes (+2.26%). At
z15, the tiles containing representative coordinates for Hồ Gươm, Hồ Tây, and
the Red River contain 4, 3, and 11 water instances respectively. The PBF also
contains named `natural=water` objects for Hồ Hoàn Kiếm/Hồ Tây and named Red
River ways; the polygon layer is the visual context, not a new label source.
The ignored measurement output is under `generated/unit-07d/`; #32 repeats the
build through the versioned package command and records reproducibility.
