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
minimal local QML runtime. The verified screen therefore uses Qt Quick core
primitives only; no package installation or unapproved UI dependency was
added. Final visual design remains out of scope.
