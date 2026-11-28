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

The initial app run used the offscreen software backend and could not prove
visual camera behavior. A subsequent local-only direct-rendering functional
review extended `qtmaps_visual_review` to start the real Valhalla backend and
wait for a `Valhalla local` route before capturing the production QML. The
review completed with exit 0 and captured vector navigation at 900×580,
1366×768 and 1920×1080, plus raster navigation at 1366×768 and the existing
layer views. The final run additionally triggered a simulated deviation,
waited for Valhalla's replacement route and resumed replay, then advanced to
arrival. Inspection of the ignored frames under
`generated/v1-b-real-visual-20261126-reroute/` showed the route, progressed
puck, rotated map, replacement guidance, green arrival banner and legible
attribution without overlap.
The banner now explicitly says `Vị trí mô phỏng` even with the route panel
collapsed. The harness asserts that the real route source, active replay and
visible banner are present. It stops the Valhalla child before exit.

This is functional visual evidence, not FPS/CPU/RAM benchmark or soak evidence.
The long Hue–Da Nang replay and its replacement route were checked at the
controller boundary with a real nationwide Valhalla response. The GUI review
used a real Hanoi route from that nationwide graph to stay inside the installed
detailed map coverage. Position remains simulated, with no live GNSS claim.
The routing graph, activation pointer and raw screenshots remain outside Git.
