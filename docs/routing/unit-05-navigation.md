# Unit 05D: Selected-route navigation evidence

Date: 2026-10-25. This report is intentionally short and grows only through
the D3 review gate.

## D1 selected-route replay

`RouteNavigationController` consumes the current backend or recorded-fixture
`Route`; it does not use or modify the Unit 03 5.47 km benchmark fixture. The
reported route duration defines 1x replay, with selectable 2x/4x multipliers.
The 33 ms timer advances only lightweight in-memory interpolation on the GUI
thread; it performs no network, filesystem, process, or blocking work.

The route screen exposes start, pause/resume, stop, speed selection, a distinct
position marker, camera follow, heading, and 45-degree tilt. The visible source
label remains `Valhalla` or `Fixture`. Focused Release tests cover missing route,
interpolation, 2x progress, pause retention, resume/arrival, stop/reset, and an
unsupported speed. The D1 visual fixture smoke rendered the new controls and
source label over the real local Hanoi map.

## D2 maneuver guidance

Replay progress is mapped to cumulative polyline distance and maneuver shape
indices. The active banner shows the next instruction, street name, distance to
that maneuver, total distance/time remaining, arrival, and the retained route
source. The maneuver list follows the same active index during replay. Focused
Release controller and QML smoke tests pass.

The full D3 boundary/QML review evidence remains pending. D4 re-routing, E
measurement, and F follow-ups have not started.
