# Unit 07C — Bounded Offline Raster Fallback

## Decision and outcome

The owner approved source option A on 2026-10-30: render raster tiles locally
from the already approved Geofabrik Vietnam PBF through the versioned QtMaps
vector package. QtMaps must never fetch a public tile endpoint. The outcome is
a reproducible, bounded Hanoi fallback that remains usable without Internet and
does not add memory to the default vector path.

## Data contract

- Coverage is `105.78,20.97,105.90,21.10`, inside the routing graph bbox
  `105.20,20.45,106.20,21.45`. It contains both fixed demo pairs
  (Hoàn Kiếm–Thái Hà and Hoàn Kiếm–Ba Đình) with more than 10% margin on every
  side.
- Zooms 12–15 cover the full bbox. Zoom 16 covers the same bounded demo
  corridor for navigation clarity. The exact generated tile count and measured
  byte size are written to the ignored raster manifest and evidence document.
- The raster manifest records its schema, package id, vector package schema and
  data version, style contract and revision, layer-schema version, source PBF
  checksum, bounds, zooms, tile format/size/count, renderer version, generation
  time, attribution, and SHA-256/size of the MBTiles file.
- Runtime accepts only the compiled map-package/style contract. A mismatch,
  missing tile database, checksum error, unsafe path, wrong attribution, bounds,
  or zoom range is rejected explicitly.
- Unit 07D/#32 must regenerate raster after the new schema/style package lands;
  its migration verification includes the raster build command and a negative
  old-raster compatibility test. Generated raster is ignored and never
  committed.

## Runtime behavior

- Vector is the default. Raster can be selected manually and starts
  automatically only when vector package validation/startup fails. Both trigger
  paths have focused tests.
- Raster startup, manifest hashing, SQLite access, and tile reads run off the UI
  thread. Missing/corrupt data produces a visible recoverable state without
  freezing routing controls or the window.
- The raster source, worker thread, database, and encoded-tile LRU are created
  lazily. The LRU maximum is 64 MiB and is destroyed when returning to vector.
  MapLibre is recreated on layer changes so old vector and raster render caches
  do not accumulate.
- UI states distinguish `Bản đồ vector`, `Raster ngoại tuyến`, loading, automatic
  fallback, and failure. Attribution `© OpenStreetMap contributors, ODbL` is
  always visible.
- Raster stops at z16. At higher camera zoom the renderer overzooms the z16
  tiles and the UI labels the layer `độ phân giải thấp hơn`.

## Verification and evidence

1. Deterministic manifest/package tests cover acceptance, style/package
   mismatch, checksum, bounds/zoom, cache eviction, manual activation,
   automatic fallback, cancellation/shutdown, and UI state.
2. Build the ignored package twice from the same rendered input and compare
   manifest/MBTiles hashes. Record the exact z12–z16 tile count, measured size,
   source PBF checksum, and command.
3. Release smoke covers vector default, manual raster, forced vector failure,
   missing raster, no external connections, routing/replay/re-routing, and
   clean shutdown.
4. Run the fallback navigation benchmark separately with the unchanged Unit 06
   thresholds. Report PSS average/peak and CPU average/peak plus the worst run;
   vector measurements must show no raster worker/cache allocation.
5. Run `make check` after each increment and isolated `make review` before phase
   handoff. Store screenshots and benchmark output only under ignored paths.

## Increment plan

1. Contract and deterministic package tooling.
2. Lazy asynchronous runtime source with a 64 MiB tested cache.
3. Manual/automatic switching, visible UI states, Release smoke, size/network
   evidence, and separate fallback benchmark.
