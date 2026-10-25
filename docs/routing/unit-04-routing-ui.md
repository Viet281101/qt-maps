# Unit 04C: Routing UI evidence

Date: 2026-10-24. The first verified slice deliberately used the recorded
Valhalla fixture before connecting the same controller to the live service.

## Fixture-first flow

The map-first screen keeps the local Hanoi vector map visible behind a compact
route panel. It supports:

- alternating origin/destination selection by clicking the map;
- simple aliases (`Hoàn Kiếm`, `Thái Hà`, `Ba Đình`) or `lat, lon` input;
- a calculate action, route polyline, A/B markers, distance/time summary, and
  scrollable Vietnamese maneuver list;
- clicking a maneuver to center and zoom to its coordinate;
- persistent selected-maneuver styling, keyboard traversal, and route street
  labels from zoom 15;
- panel-aware route fitting, a panel collapse button, and a compact one-line
  green/yellow/red source summary;
- explicit `fixture/mô phỏng` source and `offline — fixture` state; and
- visible OSM/ODbL attribution plus the existing F3 performance overlay.

The fixture controller refuses unsupported arbitrary fixture endpoints rather
than drawing a route for mismatched points. Focused Qt Test covers the path
`set endpoints -> calculate -> render-facing path/model -> focus maneuver`,
invalid/unsupported selection state, and route invalidation after a new map
point.

## Manual Release verification

The successful local command was:

```bash
LD_LIBRARY_PATH="$PWD/generated/unit-01b/maplibre-install/lib" \
QT_PLUGIN_PATH="$PWD/generated/unit-01b/maplibre-install/plugins:$PWD/generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5/plugins" \
QML2_IMPORT_PATH="$PWD/generated/unit-01b/maplibre-install/qml:$PWD/generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5/qml" \
QSG_RENDER_LOOP=threaded \
./build-qt515/qtmaps --unit03-local-map --routing-demo \
  --map-package data/packages/current --exit-after 30 \
  --network-mode localhost-only --dataset-id hanoi-20261024-v1 \
  --dataset-coverage "Hanoi bounds 105.78,20.97,105.90,21.10; z12-z15"
```

The app reported `mapReady=true`, MapLibre used Mesa Intel HD Graphics 520,
and the default Hoàn Kiếm–Thái Hà fixture displayed 7.29 km / 9 minutes with
14 Vietnamese maneuvers. Visual inspection showed the blue polyline tracking
the rendered Hanoi street network, both markers, the source/offline badge, and
attribution. The local screenshot was retained only under ignored
`generated/unit-04/`; it is not a source artifact.

Qt Quick Controls and Layouts runtime modules are not present in the current
minimal local QML runtime. The two actions therefore use the tested Qt Quick
core `KeyboardActionButton`, with tab focus plus Enter/Space activation; no
package installation or unapproved UI dependency was added.

## Managed-backend connection

Verified on 2026-10-25 with the same Release command after removing
`--routing-fixture-only`. QtMaps started the native Valhalla 3.9.1 process as
its direct child, validated that the generated config exactly matched
`tcp://127.0.0.1:18002`, waited asynchronously for `/status`, then requested
the default route. The UI changed to `Nguồn: Valhalla local` and `Valhalla
local — loopback`, retained the 7.29 km/9 minute summary and 14 Vietnamese
maneuvers, and visibly drew the route on the Hanoi roads.

A process snapshot confirmed the ownership boundary: QtMaps PID 6238 used
219,280 KiB RSS and its direct `valhalla_service` child PID 6249 used 227,144
KiB RSS. These are diagnostic snapshots, not the deferred Unit 04E benchmark.
After the timed app exit, neither process remained and port 18002 was no
longer listening. The application now reaches that result through the
asynchronous shutdown coordinator documented in the backend evidence rather
than a blocking destructor wait.

Missing service/config files now produce `Valhalla chưa chạy: thiếu <absolute
path>` in both the log and visible UI, without loading a fixture. The fixture
is available only in explicit fixture mode or as a visibly labeled fallback
after an actual route-request failure. Unsupported arbitrary endpoints do not
receive a misleading fixture route.
The runtime Qt Test uses a managed loopback child to verify fixture-to-backend
transition, request completion, visible source state, and shutdown without
requiring the generated graph in routine CTest.

## UI-polish and route inspection

Release visual verification confirmed the direct Valhalla child is included
in the process tree (`process_count=2`, `child_count=1` in every sample). Three
15-second static-screen diagnostics after the contrast/label changes produced:

| Run | Mean sampled FPS | Mean CPU | Mean RSS | Valhalla children |
| --- | ---: | ---: | ---: | ---: |
| 1 | 1.63 | 2.53% | 423.7 MiB | 1 |
| 2 | 1.56 | 2.20% | 429.9 MiB | 1 |
| 3 | 1.50 | 2.20% | 423.4 MiB | 1 |

The low FPS is expected for an idle, render-on-demand screen and is not an
interaction benchmark. The full Unit 04E scenario remains unstarted.

Two visually suspicious bends were inspected at zoom 16.5. The northern bend
is a real sequence following the one-way circulation from Đinh Tiên Hoàng via
Lê Thái Tổ to Tràng Thi around Hoàn Kiếm; the southern hook is the real final
approach through Thái Hà and two right turns into Trung Liệt. Both align with
the rendered streets and Valhalla maneuver indices; neither is a polyline
decode/rendering error. Marker bodies are now opaque and sit above route
labels, so no street label remains readable underneath A/B.

Manual keyboard verification alternated Thái Hà and Ba Đình while repeatedly
requesting routes, then ended on Ba Đình with the matching real 4.26 km /
7-minute route and 11 maneuvers. The delayed fake-server test is the
deterministic proof that an in-flight older reply is never applied.

`tools/run-demo.sh` was launched from `/tmp`; it changed to the repository
root, used absolute data/backend paths, rendered the real route, and exited
cleanly through Alt+F4 with no remaining child or listener. A missing-service
override exited normally while logging the exact missing absolute path and
displaying no fixture route.
