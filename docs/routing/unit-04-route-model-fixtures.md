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
geometry with fewer than two points. Missing response units follow Valhalla's
default `kilometers`; explicit `kilometers` and `miles` are converted to meters.
Valhalla `time` values remain seconds in the route model, and unsupported units
fail explicitly.

## Fixtures

| Fixture | Geometry | Summary | Maneuvers |
| --- | ---: | ---: | ---: |
| `hanoi_hoan_kiem_to_thai_ha.json` | 320 points | 7.294 km / 523.659 s | 14 |
| `hanoi_hoan_kiem_to_ba_dinh.json` | 233 points | 4.262 km / 392.156 s | 11 |

Both JSON files follow the Valhalla `trip.summary`, `trip.legs[].shape`, and
`trip.legs[].maneuvers[]` response structure and preserve Vietnamese street
text. The initial hand-authored files were replaced on 2026-10-24 with full
responses recorded from local Valhalla 3.9.1 on the bounded Hanoi graph. Their
metadata records `backend_calculated: true`, version/date, dataset, and
requested endpoints. They remain offline runtime fixtures when the managed
backend is unavailable; the Unit 04B evidence records the backend validation.

## Verification

Unit 04A originally passed 4/4 CTest executables. After recording the real
responses, the same parser tests assert the larger backend-calculated geometry,
summary, Vietnamese names, metadata, default kilometres, explicit
miles-to-metres conversion, unchanged seconds, invalid encodings/deltas,
unsupported units, and negative/out-of-route maneuver indices. Unit 04B adds a
fifth runtime-focused Qt Test executable.

`git diff --check` passed. No dataset, graph, executable, third-party build
tree, log, cache, screenshot, or CSV was added. The next increment, only after
owner review, is the Valhalla availability/dependency survey and the minimal
fixture-backed route UI; Unit 04A itself performs no runtime I/O.
