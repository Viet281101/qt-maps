# Unit 05D: Selected-route navigation evidence

Date: 2026-10-25. This report is intentionally short and records the current
navigation review gates.

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

## D3 review gate

Focused controller coverage includes start, maneuver boundaries, final arrival,
duplicate maneuver positions, invalid maneuver indices, zero duration, missing
routes, invalid speed, pause/resume, and remaining values. The QML test creates
the banner and verifies instruction, metric formatting, source, and arrival
state; the route screen contract verifies banner wiring and active-list sync.

The Release fixture startup reached `mapReady=true` over the local Hanoi map.
The existing D1 visual smoke showed the replay controls and source label; the
desktop automation surface did not expose the Qt window for an additional
click-through in this pass, so no stronger manual interaction claim is made.

Final D3 gate: Qt 5.15.13 Release `make test` passed 7/7 executables in 5.52
seconds, and `git diff --check` passed. The branch tracks no generated dataset,
graph, virtual environment, binary, or screenshot.

## Pre-D4 real-backend smoke and replay audit

With the Release build, `./tools/run-demo.sh --disable-instrumentation
--exit-after 8` ran without `--routing-fixture-only`. Selected log lines:

```text
probeMode= unit04-routing-valhalla
qml: routing mapReady=true error=0 errorString=
GET /status ... 200
GET /route ... 200 10211
Valhalla route applied: 7294 m, 523.659 s, 14 maneuvers
```

The process exited normally. These logs establish backend route application and
map readiness, not a manual start/pause/banner inspection. Valhalla also emitted
the previously known tile/traffic-extract warnings. Replay now uses a monotonic
elapsed-time anchor rather than accumulated tick steps; remaining route totals
are proportional estimates from progress along the replay polyline. Starting a
new request or changing either endpoint invalidates the old route and stops
replay. Focused tests include a blocked GUI tick and delayed canceled reply.

D4 is in progress after preliminary D1-D3 approval; E and F remain unstarted.
