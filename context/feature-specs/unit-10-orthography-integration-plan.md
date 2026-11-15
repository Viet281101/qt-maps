# Unit 10 — Licensed Orthography Integration Plan

## Status and tracking

**Planned; blocked on provisioning.** This unit delivers issue #15 after the
owner provides either an authorized Copernicus Data Space session or a local,
unmodified Sentinel-2 Level-2A scene with product ID and checksum. It must not
query, download, or process imagery before that gate is satisfied.

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

1. **Provisioning evidence.** Record the authorized local scene/product ID,
   SHA-256, acquisition date, processing baseline, cloud criterion, and RGB
   preparation command outside Git. Abort if the selected scene or preliminary
   package exceeds the approved coverage or storage limit.
2. **Deterministic generation.** Use the existing offline packager on the
   prepared complete z12--z15 tile tree. Validate its manifest, package hash,
   PNG coverage, provenance, attribution, and cap; generated imagery remains
   ignored.
3. **Runtime boundary.** Add a lazy worker-thread package validator/source that
   never blocks the UI. Missing, corrupt, or incompatible data is a visible
   unavailable/error state; retain the current vector or recovery-raster layer
   and never relabel it as orthography.
4. **Selector and presentation.** Add an `Orthography (10 m, local)` choice,
   unavailable reason, persistent source attribution, and readable route, A/B,
   navigation, and connection UI across the supported viewports.
5. **Acceptance.** Add focused package/runtime/QML failure tests, run the
   Release review gate, and perform an owner-authorized functional visual review
   at 900x580, 1366x768, and 1920x1080. This review is not a benchmark.

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
