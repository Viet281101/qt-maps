# Unit 08: Bounded Camera Controller

## Purpose

Unit 08 makes camera behavior deterministic for the bounded Hanoi package.
It resolves Issue #40 before the route-overlay replacement in #39 so map pan,
route fitting, navigation follow, and explicit recentering have one owner and
cannot create callback feedback loops.

## Scope

- Define one controller that accepts a requested camera state and returns the
  permitted center, zoom, bearing, and pitch.
- Bound the camera to the active package's declared Hanoi coverage plus a
  documented viewport-aware edge allowance; never invent coverage outside it.
- Define zoom min/max, initial/recenter camera, route-fit padding, and
  navigation-follow camera policy in one source-controlled contract.
- Preserve an explicit user gesture as the highest-priority request until the
  gesture ends; do not animate a clamp response back through QML callbacks.
- Expose a small QML-facing intent API: `initial`, `user`, `recenter`,
  `routeFit`, and `navigationFollow`.
- Add deterministic C++ unit coverage for bounds, zoom clamp, antimeridian-free
  Hanoi geometry, route fit, empty/invalid routes, and intent precedence.

## Contract and ownership

`MapCanvas.qml` requests camera *intent* only. A C++ controller owns
normalization and emits one resulting camera state; MapLibre receives that
state once. The controller must not read back provider camera changes and issue
another command merely because a value was normalized.

The source of truth is the active validated map package manifest. A missing,
invalid, or incompatible package leaves the existing fail-closed map state in
place; it must not silently use broad Vietnam or world bounds.

### Camera constants

The controller reads the Hanoi extent from `MapPackageContract`: west `105.78`,
south `20.97`, east `105.90`, and north `21.10`. Its overview is the extent
midpoint (`21.035`, `105.84`), zoom `14`, bearing `0`, and tilt `0`. Every
camera request is clamped to zoom `12` through `18`, bearing `[0, 360)`, and
tilt `0` through `60` degrees. Vector source tiles end at z15 and raster source
tiles at z16; the validated local MapLibre renderer intentionally overzooms both
sources through the shared z18 display ceiling.

Route fitting receives an antimeridian-free latitude/longitude bounding box
and the current map viewport in Qt Quick logical pixels. `MapCanvas` supplies
left padding of `panelWidth + 32 px` while the route panel is visible (or
`16 px` while collapsed), right padding `48 px`, and top/bottom padding
`32 px`. The pure controller calculates the Web-Mercator fit and center shift;
it rejects an empty, degenerate, non-finite, or non-positive usable viewport
without changing the map. It clamps the final zoom before calculating the
padded center shift, then projects the usable viewport by bearing to constrain
the center. The resulting camera is normalized once before QML applies it.

| Intent | Trigger | Required behavior |
| --- | --- | --- |
| `initial` | package becomes ready | use documented Hanoi overview camera |
| `user` | pan/zoom/rotate gesture | clamp only at gesture completion |
| `recenter` | explicit user control | restore overview or active follow target |
| `routeFit` | valid route accepted | fit route with stable UI-safe padding |
| `navigationFollow` | replay advances | keep position visible without fighting gestures |

## Invariants

1. Every emitted camera keeps the usable (uncovered) viewport inside the
   permitted package extent after accounting for bearing and padding; every
   emitted zoom is within the contract range.
2. A user gesture cannot be overwritten by route or navigation updates while
   it is active.
3. Invalid geometry, a degenerate route, or an unavailable package produces a
   defined no-op/fallback result, never NaN coordinates or an unbounded jump.
4. Route fitting and recentering are idempotent for unchanged inputs.
5. The controller has no render-loop, network, tile-server, or GPU dependency.

## Acceptance criteria

- The camera contract is documented with exact Hanoi bounds, min/max zoom,
  overview camera, and route-fit padding units.
- C++ tests cover ordinary, edge, degenerate, non-finite, and invalid inputs without a
  window or MapLibre runtime.
- QML integration has one command path per intent and no bidirectional camera
  binding/callback loop.
- Existing routing, offline fallback, navigation replay, labels, and package
  validation remain unchanged outside camera normalization.
- `make check` and `make review` pass.
- A fixed-viewport visual pan/zoom review and any GPU comparison occur only
  with separate owner authorization; no performance conclusion is implied by
  unit tests.

## Verification Evidence

On 2026-11-12, the owner accepted direct-GPU fixture-route review at 900x580,
1366x768, and 1920x1080 after exercising pan, zoom, rotation, route-fit,
recenter, and navigation follow. The 900x580 review exposed top-overlay
crowding; the compact panel/status stacking repair was implemented and accepted
in a repeated 900x580 review. `make test` and the isolated Release `make review`
passed 16/16. These are functional-layout checks, not benchmark evidence.

The 1920x1080 runtime also exposed a pre-existing missing local glyph request
for `8192-8447.pbf`; it is tracked separately as #50 and does not change the
Unit 08 camera contract.

## Non-goals

- Replacing the Canvas route overlay (#39), changing route geometry, or adding
  search/orthography features.
- Nationwide coverage, GPS positioning, live traffic, or camera persistence.
- Qt 6 APIs, renderer replacement, i915 configuration changes, GPU benchmark
  expansion, or a soak test.

## Delivery order

1. Record exact bounds and camera constants beside package metadata.
2. Implement the pure controller and unit tests.
3. Wire QML intents without changing route overlay behavior.
4. Run non-GPU gates, review the diff, then request scoped visual/GPU approval.
5. Start #39 only after the controller contract is accepted.
