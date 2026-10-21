# UI Context

## Visual Direction

QtMaps uses a map-first, familiar navigation layout inspired by common desktop map applications without copying branded assets. The map occupies the window, while floating controls and panels preserve maximum geographic context. The interface must remain readable over raster, vector, and orthography imagery and must expose offline, loading, degraded, and error states clearly.

Exact colors, fonts, radii, spacing, icon set, component dimensions, and accessibility contrast targets have not been selected. They remain TODO items in `progress-tracker.md`; do not invent final values during implementation.

## QML Theme Singleton

Reusable visual tokens belong in a QML singleton, planned as `qml/theme/Theme.qml`. Components consume semantic properties rather than duplicating literals.

| Category | Planned semantic properties | Current value |
| --- | --- | --- |
| Color | `mapOverlay`, `surface`, `surfaceElevated`, `textPrimary`, `textSecondary`, `accent`, `border`, `error`, `warning`, `success`, `route`, `routeOutline` | TODO |
| Typography | `fontFamily`, `fontSizeCaption`, `fontSizeBody`, `fontSizeTitle`, `fontWeightRegular`, `fontWeightEmphasis` | TODO; prefer an available system font unless a licensed bundled font is approved |
| Spacing | `spaceXs`, `spaceSm`, `spaceMd`, `spaceLg`, `spaceXl` | TODO |
| Radius | `radiusControl`, `radiusPanel`, `radiusPill` | TODO |
| Elevation | `shadowLow`, `shadowHigh` or equivalent Qt 5.13-compatible properties | TODO |
| Motion | `durationFast`, `durationNormal`, `easingStandard` | TODO; respect measured render cost |

Theme implementation must use Qt 5.13-compatible QML and must not depend on APIs or graphical effects introduced later.

## Layout Patterns

- **Map canvas:** fills the application content area and remains visible behind floating UI.
- **Search control:** a floating search field near the top edge, with results in a bounded overlay that does not resize the map.
- **Layer control:** a floating button opens a compact selector for Raster, Vector, and Orthography; unavailable layers show a reason rather than disappearing.
- **Route planning:** origin and destination controls plus route summary and maneuver preview occupy a side panel or adaptive floating panel.
- **Maneuver list:** a scrollable panel keeps the active maneuver visibly distinct and allows the map to remain usable.
- **Navigation banner:** a prominent top banner shows the next maneuver and distance while navigation simulation is active.
- **Map actions:** zoom, recenter, and navigation controls are grouped consistently and remain reachable without covering essential route content.
- **Status:** online/offline state, missing data, service startup, and benchmark recording have concise visible indicators.

Desktop placement, breakpoints, minimum window size, keyboard behavior, and final dimensions remain TODO until the initial QML shell is measured.

## Component Conventions

- Create focused QML components for the search box, layer selector, route panel, maneuver row/list, navigation banner, map actions, status indicator, and performance overlay.
- Controls expose semantic states such as normal, hover, pressed, focused, disabled, loading, warning, and error where relevant.
- Preserve keyboard focus visibility and meaningful tab order. Mouse-only interaction is not sufficient for ordinary controls.
- Use text labels or tooltips for ambiguous icons. The icon source remains an open design choice and requires license review.
- Panels need an opaque or sufficiently legible surface over every map layer; final opacity and contrast values require visual testing.
- Loading and failure states must not freeze or replace the entire map when an unaffected local layer remains usable.

## Map and Navigation Presentation

- The selected route must remain distinguishable from roads and imagery at supported zoom levels.
- Origin, destination, current simulated position, and selected search result use distinct semantic marker roles.
- Camera following and map rotation must have an obvious exit/recenter action.
- Re-routing must preserve the last valid route until a replacement succeeds or clearly report that no route is available.
- Attribution required by map and imagery licenses remains visible and readable.
- The performance overlay must be switchable and must avoid obscuring primary navigation information.

## Pending UI Decisions

The following are intentionally unresolved and tracked in `progress-tracker.md`: token values, icon source, minimum window size, side-panel placement and sizing, keyboard shortcuts, contrast target, and whether light/dark map-aware variants are required.
