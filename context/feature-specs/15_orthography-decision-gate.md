# Issue 15 — Licensed Orthography Decision Gate

## Status

**Approved on 2026-11-15; provisioning still blocked.** The owner approved the
candidate and limits below. No imagery has been downloaded or generated: a
registered Copernicus Data Space account or an approved local source scene is
still required.

## Candidate and recommendation

Use a locally generated RGB package from **Copernicus Sentinel-2 Level-2A**
surface-reflectance data, limited to the current Hanoi demo bounds:
`105.78,20.97,105.90,21.10`.

This is the recommended candidate because Sentinel data is available on a free,
full, and open basis and the legal notice permits reproduction, distribution,
adaptation, and combination with other data. The input access path requires a
registered Copernicus Data Space account; credentials and raw downloads remain
outside Git. Sentinel-2 has 10 m RGB bands, so this layer is **regional
orthography only**, not street-level satellite imagery. It must never be
presented as more detailed than its source resolution.

The owner accepted the 10 m limitation on 2026-11-15. A future street-detail
requirement still needs a separately licensed high-resolution provider and a
new decision gate; do not substitute browser/consumer imagery or cache a public
tile service.

## Required attribution and provenance

- Display persistently in the map UI and package manifest:
  `Contains modified Copernicus Sentinel data [acquisition year]`.
- Record source product IDs, acquisition date(s), processing baseline, cloud
  criterion, RGB band mapping, bounds, resampling method, generator revision,
  and SHA-256 values in the ignored generated manifest.
- Preserve the existing OSM attribution when vector overlays or route labels
  remain visible; each source retains its own attribution.
- Use the official [Copernicus Sentinel legal notice](https://sentinels.copernicus.eu/documents/247904/690755/Sentinel_Data_Legal_Notice)
  and [Copernicus Data Space terms](https://dataspace.copernicus.eu/terms-and-conditions)
  as the compliance record. The [Sentinel-2 product description](https://dataspace.copernicus.eu/data-collections/copernicus-sentinel-missions/sentinel-2)
  documents the 10 m, 20 m, and 60 m bands and Level-2A surface reflectance.

## Bounded data contract

| Property | Approved limit |
| --- | --- |
| Coverage | Current Hanoi map package bounds only: `105.78,20.97,105.90,21.10` |
| Product | Sentinel-2 Level-2A, visible RGB composite from the 10 m bands |
| Zoom range | z12–z15; do not synthesize additional source detail above z15 |
| Package form | Ignored, immutable local MBTiles/package with a checksummed manifest |
| Storage budget | At most 100 MiB for the generated package; stop and re-scope if exceeded |
| Network mode | Offline after provisioning; no runtime third-party tile requests |
| Runtime budget | Reuse the proven lazy, bounded local-raster lifecycle; measure any changed memory behavior before claiming equivalence |

The current local raster reference contains 847 tiles and occupies 38,440,960
bytes. It is only a sizing reference, not an orthography estimate or evidence
that the new package will meet the 100 MiB cap.

## Product and fallback behavior

- Add an explicit `Orthography (10 m, local)` selector only after a validated
  package exists. Until then, the layer selector shows it as unavailable with
  the reason `Chưa có gói ảnh trực giao được cấp phép`.
- Keep route polylines, A/B markers, current simulated position, navigation
  banner, and attribution readable over imagery.
- A missing, corrupt, or incompatible orthography package is a visible error;
  the application remains on its current vector/raster layer. It must not fetch
  remote imagery or silently fall back while labeling the result orthography.
- Vector remains the default. Local raster remains the existing recovery path,
  not an orthography substitute.

## Provisioning gate

Before a product query, source-data download, or processing of real imagery,
provide one of the following without committing credentials:

1. An authorized Copernicus Data Space session/account for scene selection and
   download; or
2. A local, unmodified Sentinel-2 Level-2A scene path with its product ID and
   checksum.

Then implement as a separate vertical slice: deterministic generator and
manifest contract first; then lazy local source, selector/UI states,
attribution, and focused tests; finally an owner-authorized functional visual
review. Do not run a new benchmark, soak, GPU/i915 investigation, or download
data until authorized access is available. Compact synthetic fixtures may test
the contract but must never be presented as orthography evidence.

## Deterministic package contract

`tools/data-pipeline/build_hanoi_orthography_package.py` is the first,
offline-only increment. It accepts only a complete, already prepared z12–z15
`z/x/y.png` RGB tile tree, rejects missing or extra tiles, then writes an
ignored immutable `orthography.mbtiles` package and a checksummed
`qtmaps-orthography-package-v1` manifest. It never queries Copernicus,
downloads data, or processes a raw Sentinel scene.

The manifest records the fixed bounds, zoom range, PNG tile format, 100 MiB
storage cap, required attribution, Sentinel-2 L2A RGB mapping (`B04`, `B03`,
`B02`), acquisition date, processing baseline, cloud-cover criterion,
resampling method, and one SHA-256 per product ID. Its compact test fixture is
synthetic and exists only to verify packaging, coverage, provenance, and
byte-stability; it is not imagery evidence. Runtime loading, the selector,
and on-map attribution remain deliberately out of scope until a real licensed
package is provisioned.
