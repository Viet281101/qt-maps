# Unit 12 local offline walkthrough — 2026-11-17

## Scope

This is functional local evidence on the reference development machine. It is
not a benchmark and does not replace a later clean-target transfer check.

## Activated owner-provisioned artifacts

- Map/search bundle: `hanoi-demo-20261117-r1`, 69,787,060 bytes declared;
  vector, raster, search, and licensed orthography are referenced by local
  pointers under `data/local-bundles/hanoi-demo-20261117/`.
- Routing graph: `hanoi-routing-20261024-r1`, 175,042,264 bytes declared;
  its source PBF SHA-256 is
  `04d37da6a99d95364a5aecbbd643fa56467c7659bfee84bc0a6f7b9865e9ae73`.

## Procedure and result

1. `qtmaps --install-local-bundle data/local-bundles/hanoi-demo-20261117
   --bundle-activation-root data/package-activation` validated and activated
   the bundle.
2. `qtmaps --activate-routing-graph
   data/routing-activation/hanoi-routing-20261024.json
   --routing-activation-root data/routing-activation` validated and activated
   the graph contract.
3. QtMaps was started headlessly using only the active local component pointers,
   `--network-mode offline-local`, the local Valhalla executable, and its
   loopback-only configuration. Valhalla reached `/status`, then returned a
   Hanoi car route of 7,294 m in 523.659 s with 14 maneuvers.

The Valhalla log warns about intentionally absent tile/traffic extracts; it
still returned HTTP 200 for status and route. No public-tile, geocoding, or
package-download path was invoked.

The temporary untracked activation pointers used for this walkthrough were
removed after the check so they cannot be mistaken for source-controlled
artifacts. An operator repeats the recorded activation commands with their
own authorized local bundle and graph contract.
