# V1-B real-route replay, initial functional evidence

On 2026-11-26, Unit 12 activated the nationwide graph contract
`vietnam-20261022-valhalla391-r1` under an ignored local activation root.
The Release application started with that root and the approved Unit 04
Valhalla 3.9.1 source config. Unit 13 wrote a private temporary config;
comparison of parsed JSON confirmed only `mjolnir.tile_dir` changed, from
the Hanoi tiles to the activated nationwide tiles. The application reported
`selected: true`; its Valhalla child passed `/status`, answered HTTP 200 to
the application's route request, and the routing controller applied a real
7,294 m route with 14 maneuvers. The process was stopped after the check.

The opt-in `realNationwideRouteReplayOptIn` Qt Test called the same local
Valhalla 3.9.1 binary with the nationwide config for Hue–Da Nang. It parsed
the real response through the production `ValhallaRouteParser`, selected it
in `RouteNavigationController` with source `Valhalla local`, started replay,
advanced halfway, and confirmed maneuver progression. A simulated deviation
was more than 40 m from the route. Valhalla returned a replacement route from
that position to the original destination; replay on that replacement reached
arrival at the destination. The opt-in test passed in 401 ms. The ordinary
Qt 5.15.18 Release suite passed 25/25 with that test skipped when graph
environment variables are absent.

The app run used the offscreen software backend to avoid GPU benchmark activity.
MapLibre reported no OpenGL context, so this run does not prove visual camera
follow, rotation, route rendering, or banner layout. Those remain V1-B
functional visual acceptance work. Position remains explicitly simulated;
there is no live GNSS claim. The routing graph, activation pointer and runtime
config remain outside Git.
