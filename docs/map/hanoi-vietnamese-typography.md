# Hanoi Vietnamese Typography

## Selected font

The bounded Hanoi basemap uses **Noto Sans Regular** from the Debian
`fonts-noto-core` package, version `20201225-2build1`. The upstream project is
[Noto Fonts](https://github.com/notofonts/noto-fonts); the font is licensed
under the SIL Open Font License 1.1 (OFL-1.1). The reference font file is
`/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf`, SHA-256
`89c3c497f618fdaa0b2d1e98fef93582f28c71debd2c4a8cdf41f190ced2909d`.

The font file is a build prerequisite, not a runtime network dependency. The
package builder rasterizes only the needed local Mapbox-glyph ranges into the
ignored map package; the runtime requests those PBF files from its loopback
tile source. No font, glyph, sprite, or style endpoint is public.

## Reproduction

`make requirements` verifies `fonts-noto-core`, `python3-fonttools`, and
`python3-pil`. It does not install anything. `make requirements-apt` prints
the reviewed install command if a developer needs to restore the prerequisite.

`tools/data-pipeline/generate_hanoi_glyphs.py` uses the named font file at 24
pixels and emits exactly these Noto Sans ranges:

- `0-255` and `256-511` for Latin/Latin-1 and Latin Extended letters;
- `768-1023` for combining diacritics; and
- `7680-7935` for Vietnamese precomposed letters.

This covers Vietnamese road and place names without generating a broad font
catalog. The generated files are checksummed in each immutable map package;
the generator test verifies representative `Đ` and `ộ` glyph records.

## Label policy

- Road labels are line-placed only for motorway, trunk, primary, and secondary
  classes from zoom 14. They use a white halo, a 35-degree maximum angle, and
  six pixels of collision padding.
- Place labels include only city/town context from zoom 12. They prefer
  `name_vi`, then fall back to `name`, use eight pixels of collision padding,
  and scale from 12 to 16 pixels through zoom 15.
- POIs, new sprites, and unrestricted local-road labels remain out of scope.
  Existing route-derived QML labels remain a navigation overlay rather than a
  replacement for the bounded basemap labels.

## Verification boundary

The source contract is `qtmaps-map-package-v3` / `qtmaps-style-v3` with style
revision `qtmaps-hanoi-v3`; this prevents a v2 vector or raster package from
silently being used with the Noto label style. The owner-authorized
`hanoi-20261112-v3-r2` vector and `hanoi-raster-20261112-v3-r2` raster pair
passed a 1366×768 vector → raster → vector review. The 847 raster tiles are
256×256, and a rendered z16 tile visibly shows Vietnamese street names.

After recovering the current Xwayland authorization, the accelerated Intel HD
520 direct-GPU N=3 pan/zoom median passed every locked threshold: 59.06 FPS
average, 46.03 FPS p5, 28.14 FPS 1% low, 420.18 MiB PSS peak, and 34.16%
machine CPU peak. Sampler-overhead collection was intentionally skipped; a full
GPU matrix or soak remains separately controlled by the open i915 blocker.
