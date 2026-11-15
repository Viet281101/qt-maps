# Unit 10 — Licensed Orthography Integration Plan

## Status and tracking

**Integration complete locally; one issue acceptance item deferred.** The owner
supplied a local, unmodified Sentinel-2 Level-2A scene with product ID and
checksum, satisfying the gate in `15_orthography-decision-gate.md`. Processing
stays local; neither the generator nor the application queries or downloads
imagery. The short Release performance observation remains deferred by the
active i915 benchmark freeze and is not claimed by functional visual review.

## Objective

Expose a validated, Hanoi-only local orthography layer in QtMaps while keeping
the existing vector and local-raster layers usable, accurately labelled, and
fully offline after provisioning. The approved source contract remains
authoritative in `15_orthography-decision-gate.md`.

## Fixed contract

- Sentinel-2 Level-2A RGB (`B04`, `B03`, `B02`), current Hanoi bounds only.
- z12--z15, 10 m source resolution, immutable ignored MBTiles package, and a
  100 MiB package cap.
- Persistent `Contains modified Copernicus Sentinel data [year]` attribution;
  retain OSM attribution whenever vector overlays remain visible.
- No remote imagery, runtime download, browser imagery, new dependency, or
  benchmark/soak/GPU work.

## Ordered slices

1. **Provisioning evidence — complete.** The supplied 2026-10-14 Sentinel-2C
   Level-2A scene is recorded in the ignored package manifest with its SHA-256,
   baseline 05.13, declared 12.59245% cloud cover, and RGB inputs.
2. **Deterministic generation — complete locally.** The local preparation
   script produced all 226 z12--z15 PNG tiles, then the offline packager wrote
   a 14,700,544-byte MBTiles package (below the 100 MiB cap) with checksum
   `6286ce7d3956d83d324619f879d1b11d7a05f59cfc833c22319700da63370e70`.
   Scenes, tiles, manifests, and packages remain ignored.
3. **Runtime boundary — implemented.** Add a lazy worker-thread package validator/source that
   never blocks the UI. Missing, corrupt, or incompatible data is a visible
   unavailable/error state; retain the current vector or recovery-raster layer
   and never relabel it as orthography.
4. **Selector and presentation — implemented.** Add an `Orthography (10 m, local)` choice,
   unavailable reason, persistent source attribution, and readable route, A/B,
   navigation, and connection UI across the supported viewports.
5. **Acceptance — implementation evidence complete; performance deferred.**
   Focused package/runtime/QML failure tests pass, as does the Qt 5.15.18
   Release test suite. Functional visual review captured the active layer at
   900x580, 1366x768, and 1920x1080 after constraining navigation camera
   updates to the approved z15 ceiling. This review is not a benchmark. The
   only remaining issue acceptance item is a separately approved short Release
   performance observation after the i915 freeze is lifted.

## Acceptance criteria

- A real approved package passes all validator checks and is selectable without
  restarting the app.
- Vector, existing local raster, and orthography labels/attributions are never
  conflated; unavailable state names the reason.
- Layer switches preserve readable route/navigation overlays and leave no
  worker, listener, or cache lifecycle leak.
- Generated scenes, tiles, MBTiles, manifests, credentials, and screenshots
  remain outside Git.

## Explicitly deferred

Nationwide coverage, street-level imagery, cloud-scene selection automation,
additional providers, runtime networking, and new performance measurement are
outside this unit. A full benchmark, soak, or i915 investigation needs separate
owner approval.
