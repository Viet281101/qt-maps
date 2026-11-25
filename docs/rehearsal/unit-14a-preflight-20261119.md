# Unit 14A first slice: read-only runtime preflight — 2026-11-19

`tools/offline-linux-preflight.sh` takes six explicit paths: Release app,
`valhalla_service`, map/search bundle directory, routing graph contract, Qt
prefix, and MapLibre install root. It checks executable/runtime presence,
Qt 5.15.18, JSON shape, and `ldd` resolution for the app, Valhalla and
MapLibre shared objects. Its output is a dependency inventory from the current
host, not the clean-target minimal package list. It does not copy, activate,
install, open a map, disable networking or run a benchmark.

The preflight passed on the existing local Unit 13 test fixture under
`generated/unit-13-functional-yb8b6ucb/` and the approved Qt/MapLibre runtime.
This fixture is not proof of a transferable real-data bundle. The Qt 5.15.18
Release test gate passed 25/25. No GPU benchmark was run.

Next: obtain the owner-authorized real bundle/graph and clean VM image,
record artifact hashes and licenses, then implement a single installer/smoke
workflow. A clean target must establish system dependencies and actual offline
map/search/layer/route behavior independently of this preflight.
