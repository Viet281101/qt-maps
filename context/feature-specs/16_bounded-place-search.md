# Unit 16 — Bounded Place Search

## Goal

Let a demo user find one of the supported Hanoi fixture locations and place it
into the route origin or destination without blocking the Qt Quick UI.

## Scope for this increment

- A source-controlled, local fixture catalogue containing Hoàn Kiếm, Thái Hà,
  and Ba Đình — the locations already accepted by `RoutingController`.
- Debounced query state exposed to QML: idle, loading, ready, empty, and error.
- A compact result popover in the existing map search control.
- Explicit selection of a result as origin or destination; the existing route
  fields and controller remain the single endpoint authority.
- Clear Vietnamese UI labels that this is a bounded demo fixture.
- Deterministic C++ and QML tests for matching, empty input, stale queries,
  selection, and UI states.

## Deferred

- SQLite FTS5 schema, generator, address/POI coverage beyond the three fixture
  locations, ranking from a geographic index, and a persisted geocoding
  database. These remain proposed under the architecture's Search entry.
- Remote geocoding, network calls, or new services.

## Acceptance

1. Typing a supported name returns a labeled local fixture result without
   blocking the UI thread.
2. A result can populate either route endpoint and invalidates the old route
   through the existing `RoutingController` behavior.
3. Empty, loading, no-result, and failure states are visible and truthful.
4. The shell remains usable at 900x580, 1366x768, and 1920x1080.
5. The full SQLite FTS5 route is documented as deferred rather than claimed.
