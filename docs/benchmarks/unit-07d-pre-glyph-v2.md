# Unit 07D pre-glyph v2 review

## Default-safety and migration plan

The default `data/packages/current` pointer still resolves to
`hanoi-20261024-v1`, and the unmodified runtime still enforces
`qtmaps-map-package-v1` / `qtmaps-style-v1`. A normal fixture-backed Release
smoke reached `mapReady=true` with that package. The draft v2 package used in
this review was passed explicitly with `--map-package`; the default pointer was
not changed.

Increment #30 and the map-package contract migration portion of #32 must merge
as one atomic integration. Until that integration, v2 is review-only. This
keeps every `main` revision able to start the default demo with its matching
manifest, schema, and style.

## Method and environment caveat

Both current-day rows below ran on the same Dell Latitude 5480, Release build,
MapLibre Native Qt 3.0.0, Valhalla 3.9.1, Linux 7.0.0-42, and Qt 5.15.18. Each
row is N=3, 15 measured seconds after a 3-second warm-up, with a 500 ms fast
sampler and a 2,000 ms fresh-PSS sampler. Online means host networking with
local services; offline means a loopback-only network namespace. Values are
`median [min, max]`.

The host upgraded from Qt 5.15.13/Linux 6.8 to Qt 5.15.18/Linux 7.0 before this
review. The locked MapLibre and QML 5.15.13 runtime aborted when mixed with the
new system Qt, so the current-day A/B uses an isolated, matching Qt 5.15.18 QML
runtime. Historical Unit 06 and 07C results remain useful context but are not a
same-environment performance comparison. The current-day 07C control is the
causal comparator for v2.

The v2 package is `hanoi-20261101-v2-style29`: 226 z12-z15 tiles, 15,007,744
bytes MBTiles, the #29 product style, and #30 water/selected-green layers. It
was generated with the pinned local Planetiler 0.10.2 jar and approved local
Geofabrik PBF with network access disabled.

## Current-day v2 result

| Scenario / mode | Duration + warm-up | Fresh PSS samples (rows) | FPS avg / p5 / 1% low | Frame p95 ms | Sampler p95 ms | PSS avg / peak MiB | CPU avg / peak % | Overall |
| --- | ---: | ---: | --- | ---: | ---: | --- | --- | --- |
| navigation_replay / offline | 15 s + 3 s | 8 [7, 8] (31 [31, 32]) | 44.61 [43.68, 44.84] / 21.55 [21.45, 22.00] / 11.36 [9.88, 12.08] | 46.41 [45.46, 46.61] | 51.99 [38.49, 70.82] | 478.22 [470.61, 480.39] / 565.01 [561.76, 565.02] | 23.94 [22.81, 24.98] / 44.82 [33.70, 50.49] | FAIL (3/3) |
| navigation_replay / online | 15 s + 3 s | 8 [7, 8] (31 [31, 33]) | 44.22 [41.95, 44.93] / 21.87 [20.86, 22.32] / 14.79 [12.99, 17.46] | 45.72 [44.80, 47.93] | 53.16 [46.79, 61.98] | 480.53 [470.01, 484.13] / 569.15 [564.41, 571.15] | 23.71 [22.01, 23.95] / 43.91 [32.74, 44.42] | FAIL (3/3) |
| pan_zoom / offline | 15 s + 3 s | 7 [7, 7] (31 [31, 31]) | 51.84 [51.82, 53.20] / 26.51 [26.05, 26.68] / 10.97 [10.66, 13.70] | 37.72 [37.48, 38.39] | 90.06 [85.42, 102.33] | 445.82 [434.08, 447.52] / 465.56 [458.27, 473.89] | 34.45 [34.00, 35.12] / 58.44 [54.01, 59.24] | FAIL (3/3) |
| pan_zoom / online | 15 s + 3 s | 7 [7, 8] (31 [31, 31]) | 53.17 [52.20, 54.65] / 26.71 [25.64, 27.03] / 12.92 [11.23, 12.96] | 37.43 [37.00, 39.00] | 76.63 [60.07, 118.08] | 445.43 [435.55, 451.93] / 469.32 [460.26, 476.29] | 34.27 [33.87, 34.53] / 53.24 [50.42, 55.39] | FAIL (3/3) |

Locked thresholds are FPS average/p5/1% low at least 30/24/20; frame average
and p95 at most 33.3/50 ms; PSS average/peak at most 550/640 MiB; RSS
average/peak at most 650/768 MiB; machine-normalized CPU average/peak at most
40/75%; and sampler p95 at most 50 ms.

| Scenario / mode | FPS avg | FPS p5 | 1% low | Frame avg | Frame p95 | PSS avg | PSS peak | RSS avg | RSS peak | CPU avg | CPU peak | Sampler p95 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| navigation_replay / offline | PASS | FAIL | FAIL | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | FAIL |
| navigation_replay / online | PASS | FAIL | FAIL | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | FAIL |
| pan_zoom / offline | PASS | PASS | FAIL | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | FAIL |
| pan_zoom / online | PASS | PASS | FAIL | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | FAIL |

The generated report records every threshold value and all per-run ranges at
`generated/benchmarks/20261101-unit07d-v2-style29-qt51518/report.md`.

## Comparison

The table below shows medians. Unit 06 used 60 seconds + 5 seconds and about 30
fresh-PSS/120 fast samples per run. Historical 07C and both current-day rows
used 15 seconds + 3 seconds and 7-8 fresh-PSS/31-33 fast samples per run.

| State / scenario / mode | FPS avg / p5 / 1% low | Frame p95 | PSS avg / peak MiB | CPU avg / peak % | Result |
| --- | --- | ---: | --- | --- | --- |
| Unit 06 / navigation / offline | 60.03 / 51.61 / 42.82 | 19.38 | 496.54 / 527.11 | 8.43 / 38.81 | PASS |
| Unit 06 / navigation / online | 59.98 / 51.76 / 44.43 | 19.32 | 505.69 / 541.62 | 8.49 / 37.72 | PASS |
| Unit 06 / pan_zoom / offline | 59.96 / 49.49 / 39.33 | 20.21 | 421.76 / 432.78 | 16.88 / 27.39 | PASS |
| Unit 06 / pan_zoom / online | 60.04 / 49.58 / 40.73 | 20.17 | 419.21 / 430.59 | 17.51 / 26.55 | PASS |
| Historical 07C / navigation / offline | 59.63 / 49.42 / 29.34 | 20.23 | 447.57 / 539.78 | 18.54 / 37.58 | PASS |
| Historical 07C / navigation / online | 59.67 / 49.14 / 32.21 | 20.35 | 438.30 / 537.14 | 17.95 / 39.82 | PASS |
| Historical 07C / pan_zoom / offline | 60.12 / 49.33 / 40.23 | 20.27 | 417.43 / 437.39 | 16.84 / 28.33 | PASS |
| Historical 07C / pan_zoom / online | 60.14 / 48.51 / 39.43 | 20.53 | 418.09 / 435.01 | 16.87 / 28.61 | PASS |
| Current 07C control / navigation / offline | 44.25 / 23.19 / 14.08 | 43.12 | 484.95 / 581.74 | 23.05 / 44.30 | FAIL |
| Current 07C control / navigation / online | 46.73 / 22.13 / 13.80 | 44.98 | 487.65 / 573.59 | 22.67 / 31.94 | FAIL |
| Current 07C control / pan_zoom / offline | 54.48 / 27.47 / 10.82 | 36.40 | 448.80 / 480.52 | 29.67 / 47.82 | FAIL |
| Current 07C control / pan_zoom / online | 52.75 / 26.28 / 10.32 | 38.05 | 452.86 / 480.81 | 29.86 / 43.62 | FAIL |
| Current v2 / navigation / offline | 44.61 / 21.55 / 11.36 | 46.41 | 478.22 / 565.01 | 23.94 / 44.82 | FAIL |
| Current v2 / navigation / online | 44.22 / 21.87 / 14.79 | 45.72 | 480.53 / 569.15 | 23.71 / 43.91 | FAIL |
| Current v2 / pan_zoom / offline | 51.84 / 26.51 / 10.97 | 37.72 | 445.82 / 465.56 | 34.45 / 58.44 | FAIL |
| Current v2 / pan_zoom / online | 53.17 / 26.71 / 12.92 | 37.43 | 445.43 / 469.32 | 34.27 / 53.24 | FAIL |

The current-day control proves that the large FPS/sampler regression already
exists on 07C after the host Qt/kernel change. Relative to that matched control,
v2 reduces median PSS average by 3.0-7.1 MiB and peak by 4.4-16.7 MiB. It adds
about 4.4-4.8 CPU points during pan/zoom and loses 1.65 navigation p5 FPS in the
offline row; the online navigation p5 is nearly unchanged. This review does not
accept v2 as performance-passing: the matched v2 matrix still fails the locked
1% low and sampler gates, and navigation p5 also fails.

## Fixed-camera visual review

The paired captures use exactly the same coordinates/zoom and a logical
1366x768 X11 review surface. They use llvmpipe only for deterministic capture;
the benchmark above used the real Intel HD Graphics 520/Mesa renderer. The
fixture route overlay and shell are intentionally identical in each pair.

| Location | Camera | Before (07C v1) | After (#29 + #30 v2) |
| --- | --- | --- | --- |
| Ho Guom | 21.0288, 105.8521, z15 | `generated/unit-07d/fixed-camera/before-07c-ho-guom-1366x768.png` | `generated/unit-07d/fixed-camera/after-07d-ho-guom-1366x768.png` |
| Ho Tay | 21.0550, 105.8200, z14 | `generated/unit-07d/fixed-camera/before-07c-ho-tay-1366x768.png` | `generated/unit-07d/fixed-camera/after-07d-ho-tay-1366x768.png` |
| Song Hong | 21.0400, 105.8800, z14 | `generated/unit-07d/fixed-camera/before-07c-song-hong-1366x768.png` | `generated/unit-07d/fixed-camera/after-07d-song-hong-1366x768.png` |

The after captures make Ho Guom, Ho Tay, the Red River, secondary water bodies,
and selected parks/green areas visually distinct. The Song Hong camera also
shows the approved package boundary at the east edge; v2 does not claim data
beyond 105.90 E.

All packages, benchmark CSV/JSON, temporary review builds, toolchain extracts,
and screenshots remain ignored generated artifacts and are not committed.
