# Pre-Unit 06: Repository Structure Maintenance

## Purpose

Perform a bounded, behavior-preserving cleanup on a dedicated branch created
from the updated `main` branch before starting Unit 06. This work must not add
product scope, replace the feature-based architecture, or silently change the
map, routing, navigation, fixture fallback, or benchmark behavior.

The branch must be created before any implementation, commit, or push for this
maintenance work. Unit 05E/F remain planned work and resume after the cleanup is
reviewed and merged; Unit 06 must receive its own approved specification before
implementation.

## Confirmed findings

1. The root `resources.qrc` is not wrong merely because of its location. The
   ownership problem is that one bundle contains both QML screens and recorded
   routing responses currently stored under `tests/fixtures/`. Moving that file
   unchanged into `qml/` would create misleading cross-directory references.
2. `tests/fixtures/routing/` is not test-only: `RoutingController` loads those
   responses as runtime fallback resources. Runtime recordings should move to a
   runtime-owned location such as `fixtures/routing/` or `resources/routing/`;
   genuinely test-only inputs should remain under `tests/fixtures/`.
3. `fixtures/mapboxgl/style.json` is a tracked Unit 01 diagnostic artifact with
   no current source, build, test, or resource reference. It is a deletion
   candidate after one final reference and smoke check; Git history remains the
   recovery path.
4. `qml/RoutingProbe.qml` is now the effective product shell and has grown to
   roughly 700 lines. Meanwhile, `qml/Main.qml` remains a Unit 01 diagnostic
   screen and the other top-level `*Probe.qml` files are unit diagnostics.
5. `qtmaps_map_probe` combines production map sources/controllers with
   diagnostic replay/camera controllers. The target name and source placement
   no longer communicate the production boundary accurately.
6. The current feature-first layout (`src/map`, `src/routing`, and peers) is
   appropriate for the repository's size. Do not introduce `domain/`,
   `infrastructure/`, or similar architectural layers during this cleanup.

## Bounded implementation sequence

### 1. Establish resource ownership

- Split QML resources from routing runtime recordings instead of relocating the
  existing monolithic file unchanged.
- Keep resource aliases stable unless a coordinated source and test update is
  part of the same verified change.
- Move recorded routing responses out of the test-only namespace and preserve
  the explicit fixture/simulated source labels shown to users.
- Remove the legacy Mapbox GL diagnostic style only after confirming that no
  current target or documented smoke path needs it.

### 2. Establish the product QML entry point

- Make `qml/Main.qml` the unambiguous product entry point.
- Move retained diagnostic screens under `qml/probes/` and keep command-line
  probe selection working only where it is still useful.
- Split the current routing shell along visible responsibilities, initially
  targeting a route/control panel, map canvas, navigation controls/banner, and
  connection/source status component. Prefer a few cohesive components over
  one component per visual element.

### 3. Officialize the map target

- Rename the production target from `qtmaps_map_probe` to `qtmaps_map`.
- Move diagnostic-only controllers, including camera/replay probe machinery,
  into a clearly named diagnostics location or target where practical.
- Keep `MapSourceController`, `TileSource`, and concrete tile sources in the
  production map boundary.

## Review gates

- Keep each concern reviewable and avoid mixing behavior or feature changes
  into the structural work.
- Run `make check` after each meaningful change and `make review` before merge.
- Smoke-test the default product entry point, retained diagnostic entry points,
  fixture fallback, real local Valhalla routing, navigation replay, re-routing,
  and clean shutdown.
- Confirm that the QML resource aliases and runtime fallback paths work from a
  clean Release build.
- Update `context/architecture.md`, affected user/developer documentation, and
  this tracker if the final implementation differs from the boundaries above.
- Record any material CPU/RSS/FPS difference caused by the QML split. Unit 05E
  remains the authoritative full 120-second measurement step after maintenance.

## Explicit non-goals

- No new end-user features or Unit 06 implementation.
- No `domain/` or `infrastructure/` layers.
- No change to Qt 5.15.13, MapLibre Native Qt v3.0.0, CMake/Ninja, or Qt 5 APIs.
- No fixture presented as a live backend result and no expansion of demo claims.
