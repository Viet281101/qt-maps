# Unit 04A Route Model and Fixture Evidence

Date: 2026-10-24. This evidence covers only the backend-independent route
contract, polyline6 decoder, Valhalla response parser, and test fixtures. No
Valhalla binary, routing graph, route UI, or navigation behavior was added.

## Contract

`Route` contains a decoded `QGeoCoordinate` polyline, total distance in meters,
total duration in seconds, source (`Fixture` or `Valhalla`), and ordered
maneuvers. Each maneuver retains the Valhalla numeric type, instruction, first
street name, distance/time, shape index, and resolved coordinate. Parser errors
are explicit values; no exception or silent fallback crosses the boundary.

`Polyline6Decoder` uses precision 6 and rejects empty/truncated input,
out-of-range characters, oversized components/deltas, invalid coordinates, and
geometry with fewer than two points. The response parser currently requires
Valhalla `kilometers` units and converts all distances to meters.

## Fixtures

| Fixture | Geometry | Summary | Maneuvers |
| --- | ---: | ---: | ---: |
| `hanoi_hoan_kiem_to_thai_ha.json` | 7 points | 5.475 km / 720 s | 7 |
| `hanoi_hoan_kiem_to_ba_dinh.json` | 5 points | 2.600 km / 390 s | 5 |

Both JSON files follow the Valhalla `trip.summary`, `trip.legs[].shape`, and
`trip.legs[].maneuvers[]` response structure and preserve Vietnamese street
text. They are hand-authored from bounded real Hanoi coordinates and are
explicitly marked `backend_calculated: false`; their street path and timing are
UI/model fixtures, not a claim that Valhalla calculated or validated a legal
driving route.

## Verification

`make test` configured and rebuilt the Qt 5.15.13 Release tree without a new
compiler warning. CTest passed 4/4 executables in 0.64 seconds, including the
new `qtmaps_routing_tests`. Tests cover positive/negative polyline deltas, both
fixtures, Vietnamese names, source labeling, invalid encodings/deltas,
unsupported units, and negative/out-of-route maneuver indices.

`git diff --check` passed. No dataset, graph, executable, third-party build
tree, log, cache, screenshot, or CSV was added. The next increment, only after
owner review, is the Valhalla availability/dependency survey and the minimal
fixture-backed route UI; Unit 04A itself performs no runtime I/O.
