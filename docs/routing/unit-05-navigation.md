# Unit 05D: Selected-route navigation evidence

Date: 2026-10-25. This report is intentionally short and records the current
navigation review gates.

## D1 selected-route replay

`RouteNavigationController` consumes the current backend or recorded-fixture
`Route`; it does not use or modify the Unit 03 5.47 km benchmark fixture. The
reported route duration defines 1x replay, with selectable 2x/4x multipliers.
The 33 ms timer advances only lightweight in-memory interpolation on the GUI
thread; it performs no network, filesystem, process, or blocking work.

The route screen exposes start, pause/resume, stop, explicit recenter, speed
selection, a distinct position marker, camera follow, heading, and 45-degree tilt. The visible source
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
A later real-Valhalla click-through exercised start, pause/resume, stop, the
guidance banner, maneuver synchronization, and the D4 flow described below. The
follow-up recenter control remained explicitly visible while replay was active
and paused.

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

## D4 simulated re-routing

The simulation chooses a position more than 40 m from the nearest polyline
segment. The existing Valhalla client cancels and generation-guards replaced
requests; the request uses that position and the existing destination. A
successful response replaces the route and resumes replay. A backend error stops
replay and is shown explicitly, without a fixture fallback. This is simulated
deviation, not live GPS or marker dragging.

Focused Release tests cover positions at 39/41 m, simulated offset, outgoing
coordinates, a delayed replaced request, successful replay resumption, and an
HTTP 503 failure. These use a fake loopback backend. A real-Valhalla UI
click-through started the 7,294 m/14-maneuver route, paused and resumed replay,
then simulated a deviation. Valhalla returned a replacement 7,622 m route in
560.93 seconds with 16 maneuvers; replay resumed, the banner and maneuver list
updated, the explicit recenter and stop actions remained available, and stop
exited navigation.
E and F remain unstarted; work stops for the D4 review gate.

After the recenter follow-up, the final Qt 5.15.13 Release `make test` passed 7/7
executables in 6.63 seconds; `git diff --check` passed.
