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
replay and is shown explicitly, without a fixture fallback; the last valid route
remains available for retry. This is simulated deviation, not live GPS or
marker dragging.

Focused Release tests cover positions at 39/41 m, simulated offset, outgoing
coordinates, a delayed replaced request, successful replay resumption, and an
HTTP 503 failure. These use a fake loopback backend. A real-Valhalla UI
click-through started the 7,294 m/14-maneuver route, paused and resumed replay,
then simulated a deviation. Valhalla returned a replacement 7,622 m route in
560.93 seconds with 16 maneuvers; replay resumed, the banner and maneuver list
updated, the explicit recenter and stop actions remained available, and stop
exited navigation.
A repeat click-through after the failed-reroute preservation fix replaced the
same initial route with a 7,307 m/522.622 s route with 14 maneuvers and resumed
replay normally.
E and F remain unstarted; work stops for the D4 review gate.

After the recenter and failed-reroute preservation follow-ups, the final Qt
5.15.13 Release `make test` passed 7/7 executables in 6.88 seconds;
`git diff --check` passed.

## PR #22 merge-gate click-through (2026-10-25)

This pass used the Qt 5.15.13 Release build and the local Hanoi map package.
The success scenarios below used the real managed Valhalla 3.9.1 service, not
the recorded route fixture.

- At 900x580, the initial Hoàn Kiếm–Thái Hà request returned 7,294 m,
  523.659 s, and 14 maneuvers. Starting replay showed the guidance banner while
  leaving the connection pill below it. Clicking the map while replay was
  running left the two endpoint fields, route summary, visible route, and
  running guidance unchanged. Pause and the explicit recenter action remained
  operable.
- At 900x580, the route-control area scrolled to expose the deviation action,
  while the maneuver list retained its own viewport and scrolled from the
  current early maneuvers to maneuvers 13 and 14. A real-Valhalla deviation
  request succeeded with a replacement route of 7,647 m, 548.184 s, and 15
  maneuvers; replay resumed with updated banner/list content.
- At 1366x768, the active banner, connection pill, controls, and maneuver list
  were simultaneously visible without overlap. Replay and the same explicit
  controls remained usable at this size.
- For a short route selected on the map, real Valhalla returned 336 m,
  45.638 s, and 4 maneuvers. At 4x replay the UI reached `Đã đến đích`, showed
  `Đi lại từ đầu` instead of `Tạm dừng`, and invoking it restarted the same
  route with active guidance from the beginning.

The HTTP 503 scenario was observed separately with the deterministic fake
loopback backend from `qtmaps_valhalla_runtime_tests`; it was not attributed to
the real Valhalla process. The initial failed request loaded the explicitly
labeled fixture fallback. After replay started, the simulated deviation
received `503 Service Unavailable`; replay stopped, the error was visible, and
the prior 7.29 km summary and maneuver list remained available without a second
fixture substitution. Automated coverage additionally compares the preserved
polyline, maneuver count, distance, duration, and source.

Screenshots were used only for local visual inspection and remain untracked.
No FPS/CPU navigation measurement was performed in this pass; Unit 05E and F
remain unstarted.

Final post-fix gate: Qt 5.15.13 Release `make test` passed all 7/7 CTest
executables in 7.46 seconds, and `git diff --check origin/main...HEAD` passed.
All six acceptance criteria in Issue #6 remain satisfied; the deferred E/F
process-tree measurement is not one of those criteria.
