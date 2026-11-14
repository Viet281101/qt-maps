# UI Context

## Visual Direction

QtMaps uses a map-first, familiar navigation layout inspired by common desktop map applications without copying branded assets. The map occupies the window, while floating controls and panels preserve maximum geographic context. The interface must remain readable over raster, vector, and orthography imagery and must expose offline, loading, degraded, and error states clearly.

Unit 07B commits a dark floating-control theme for the management demo. Text
must meet WCAG AA 4.5:1 contrast on its actual panel/control background, ordinary
pointer targets are at least 36 px high, and keyboard focus uses a visible
high-contrast outline. The map cartography remains light; a later dark-map theme
is not implied by the dark control surfaces.

Unit 02 may introduce a minimal `qml/theme/Theme.qml` singleton so the performance overlay consumes semantic tokens. Values used by that diagnostic overlay are provisional implementation tokens, not the final visual-system decision, and must remain isolated behind the singleton.

## QML Theme Singleton

Reusable visual tokens belong in a QML singleton, planned as `qml/theme/Theme.qml`. Components consume semantic properties rather than duplicating literals.

| Category | Planned semantic properties | Current value |
| --- | --- | --- |
| Color | `mapOverlay`, `surface`, `surfaceElevated`, `surfaceInput`, `textPrimary`, `textSecondary`, `accent`, `border`, `error`, `warning`, `success`, `route`, `routeOutline`, `routeTravelled`, `markerOrigin`, `markerDestination` | Unit 07B dark demo palette in `Theme.qml`; route starts at `#1A73E8` with white outline, travelled route is grey, and A/B markers use `#1E8E3E` / `#D93025` |
| Typography | `fontFamily`, `fontSizeCaption`, `fontSizeBody`, `fontSizeTitle`, `fontSizeDisplay`, `fontWeightRegular`, `fontWeightEmphasis` | System Noto Sans; no bundled font until the licensed cartography-font gate |
| Spacing | `spaceXs`, `spaceSm`, `spaceMd`, `spaceLg`, `spaceXl` | 4 / 8 / 12 / 16 / 24 px |
| Radius | `radiusControl`, `radiusPanel`, `radiusPill` | 8 / 12 / 999 px |
| Elevation | `shadowLow`, `shadowHigh` or equivalent Qt 5.15-compatible properties | TODO |
| Motion | `durationFast`, `durationNormal`, `easingStandard` | TODO; respect measured render cost |

Theme implementation must use Qt 5.15.13-compatible QML and must not depend on Qt 6-only APIs or graphical effects.

## Layout Patterns

- **Map canvas:** fills the application content area and remains visible behind floating UI.
- **Search control:** a floating search field near the top edge, with results in a bounded overlay that does not resize the map.
- **Layer control:** a floating button opens a compact selector for Raster, Vector, and Orthography; unavailable layers show a reason rather than disappearing.
- **Route planning:** origin and destination controls plus route summary and maneuver preview occupy a side panel or adaptive floating panel.
- **Maneuver list:** a scrollable panel keeps the active maneuver visibly distinct and allows the map to remain usable.
- **Navigation banner:** a prominent top banner shows the next maneuver and distance while navigation simulation is active.
- **Map actions:** zoom, recenter, and navigation controls are grouped consistently and remain reachable without covering essential route content.
- **Status:** online/offline state, missing data, service startup, and benchmark recording have concise visible indicators.
- **Layer state:** the layer menu distinguishes local vector, manually selected offline raster, automatic raster fallback, loading, and package failure. Raster warns that views above z16 are overzoomed at lower resolution; switching never hides attribution or route/navigation overlays.

The minimum supported window is 900x580. At that size the route panel scrolls
independently, navigation automatically collapses it, and the banner/status
overlays must not intersect. Demo review also covers 1366x768 and 1920x1080.
The supported maximum is 1920x1080 so the bounded raster visible-texture
working-set calculation remains valid on the reference configuration.

At widths through 960 px, the route panel uses a 300 px compact width. Search
text clips/elides inside its surface; connection status occupies the next
available top-right row beneath the layer control, and moves below the expanded
layer menu. This prevents status/search and status/menu overlap without hiding
the connection source.

## Component Conventions

- Create focused QML components for the search box, layer selector, route panel, maneuver row/list, navigation banner, map actions, status indicator, and performance overlay.
- Controls expose semantic states such as normal, hover, pressed, focused, disabled, loading, warning, and error where relevant.
- Preserve keyboard focus visibility and meaningful tab order. Mouse-only interaction is not sufficient for ordinary controls.
- Use text labels or tooltips for ambiguous icons. The icon source remains an open design choice and requires license review.
- Panels need an opaque or sufficiently legible surface over every map layer; final opacity and contrast values require visual testing.
- Loading and failure states must not freeze or replace the entire map when an unaffected local layer remains usable.

## Map and Navigation Presentation

- The selected route must remain distinguishable from roads and imagery at supported zoom levels.
- Qt 5.15 `MapPolyline` does not expose cap/join controls in QML. The route therefore relies on the MapLibre renderer's joins, while circular A/B markers cover its endpoint caps; visual review at every supported viewport is the acceptance check.
- Origin, destination, current simulated position, and selected search result use distinct semantic marker roles.
- Camera following and map rotation must have an obvious exit/recenter action.
- Re-routing must preserve the last valid route until a replacement succeeds or clearly report that no route is available.
- During re-routing, keep the navigation camera, travelled segment, current-position puck, and last valid route. The destination marker remains part of the route even when camera-follow framing places it outside the viewport.
- Attribution required by map and imagery licenses remains visible and readable.
- The performance overlay must be switchable and must avoid obscuring primary navigation information.
- A performance value that has not been sampled is shown with an em dash; the UI must never present a synthetic zero RSS measurement.
- The connection pill uses an everyday Vietnamese label while retaining the source identity, for example `Valhalla (cục bộ)`, and stays below the navigation banner at every review viewport.

## Pending UI Decisions

The icon source and any light/dark map-aware variant remain unresolved. Unit 07B
uses text/Unicode directional symbols only; introducing an icon asset set still
requires license review. F3 is the committed performance-detail shortcut.
