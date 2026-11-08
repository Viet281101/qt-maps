# Unit 07B/07C Same-Day A/B Benchmark

## Method

This 2026-10-31 comparison ran on the same Dell Latitude 5480 (i5-6300U,
2 cores/4 logical CPUs, 15.50 GiB RAM), Qt 5.15.13, MapLibre Native Qt 3.0.0,
and Valhalla 3.9.1. Every row is Release, N=3, 15 seconds measured after a
3-second warm-up, with a 500 ms fast sampler and a 2,000 ms fresh-PSS sampler.
`online` means host networking with local services; `offline` means a network
namespace with loopback only. Both modes use the same local map and routing
artifacts.

The four states are main before Unit 07 (`e0a8086`), end of 07B (`1ce45e4`),
end of 07C vector, and end of 07C raster fallback. The same review-only
benchmark patch continuously requests frames during `pan_zoom` and
`navigation_replay` in all four builds. Without that drive, Qt Location's
event-driven raster renderer correctly stops swapping between discrete replay
commands and the harness incorrectly counts the intentional pause as a slow
frame. This patch does not change normal product rendering.

Values are `median [min, max]`. The worst run is therefore the lower bracket
for FPS and the upper bracket for frame time, sampler time, memory, and CPU.
`PASS (3/3)` means all runs completed and every median locked-threshold
comparison for that row passed. The locks are FPS average/p5/1% low at least
30/24/20; frame average/p95 at most 33.3/50 ms; PSS average/peak at most
550/640 MiB; RSS average/peak at most 650/768 MiB; CPU average/peak at most
40/75%; and sampler p95 at most 50 ms. The generated reports enumerate every
individual threshold comparison. A worst-run outlier does not silently change
the pre-approved median gate.

## Main before Unit 07

| Scenario / mode | Duration + warm-up | Fresh PSS samples (rows) | Sampler fast / PSS | FPS avg / p5 / 1% low | Frame p95 ms | Sampler p95 ms | PSS avg / peak MiB | CPU avg / peak % | Status |
| --- | ---: | ---: | ---: | --- | ---: | ---: | --- | --- | --- |
| navigation_replay / offline | 15 s + 3 s | 8 [8, 8] (31 [31, 31]) | 500 / 2000 ms | 60.15 [60.08, 60.16] / 53.95 [53.91, 54.11] / 39.58 [39.14, 39.64] | 18.51 [18.48, 18.54] | 25.71 [24.54, 28.02] | 426.94 [424.97, 431.33] / 536.90 [534.32, 542.22] | 11.58 [11.17, 11.64] / 41.75 [32.81, 49.39] | PASS (3/3) |
| navigation_replay / online | 15 s + 3 s | 8 [7, 8] (31 [31, 31]) | 500 / 2000 ms | 60.11 [60.01, 60.16] / 53.79 [53.73, 53.85] / 39.54 [34.38, 41.88] | 18.59 [18.57, 18.61] | 28.57 [23.99, 29.73] | 444.92 [441.58, 447.14] / 533.23 [532.38, 540.99] | 11.18 [10.74, 11.67] / 35.07 [33.43, 43.33] | PASS (3/3) |
| pan_zoom / offline | 15 s + 3 s | 8 [7, 8] (31 [31, 31]) | 500 / 2000 ms | 60.16 [60.15, 60.21] / 49.97 [49.53, 50.19] / 42.20 [38.51, 42.87] | 20.01 [19.92, 20.19] | 26.88 [26.43, 28.36] | 403.62 [403.36, 406.41] / 424.82 [423.80, 428.71] | 16.37 [16.17, 16.56] / 24.50 [22.68, 24.51] | PASS (3/3) |
| pan_zoom / online | 15 s + 3 s | 8 [7, 8] (31 [31, 31]) | 500 / 2000 ms | 60.14 [60.11, 60.14] / 49.61 [48.69, 50.33] / 41.88 [39.71, 41.97] | 20.16 [19.87, 20.54] | 25.48 [25.11, 26.02] | 405.86 [401.12, 406.89] / 425.13 [423.76, 427.53] | 16.75 [16.03, 16.94] / 25.00 [24.00, 31.67] | PASS (3/3) |

## End of Unit 07B

| Scenario / mode | Duration + warm-up | Fresh PSS samples (rows) | Sampler fast / PSS | FPS avg / p5 / 1% low | Frame p95 ms | Sampler p95 ms | PSS avg / peak MiB | CPU avg / peak % | Status |
| --- | ---: | ---: | ---: | --- | ---: | ---: | --- | --- | --- |
| navigation_replay / offline | 15 s + 3 s | 8 [7, 8] (31 [31, 31]) | 500 / 2000 ms | 60.02 [60.02, 60.22] / 51.88 [51.21, 52.38] / 41.36 [39.90, 44.79] | 19.28 [19.09, 19.53] | 29.84 [26.06, 29.97] | 442.11 [427.71, 445.75] / 534.20 [533.69, 535.93] | 12.41 [12.01, 12.48] / 34.64 [32.56, 36.07] | PASS (3/3) |
| navigation_replay / online | 15 s + 3 s | 7 [7, 8] (31 [31, 31]) | 500 / 2000 ms | 60.21 [60.20, 60.22] / 52.54 [52.00, 52.91] / 44.17 [42.90, 44.71] | 19.03 [18.90, 19.23] | 25.73 [23.89, 28.01] | 439.13 [426.60, 448.53] / 532.16 [528.25, 537.41] | 12.19 [12.07, 12.33] / 34.36 [33.50, 37.15] | PASS (3/3) |
| pan_zoom / offline | 15 s + 3 s | 7 [7, 8] (31 [31, 31]) | 500 / 2000 ms | 60.13 [60.12, 60.14] / 48.95 [48.49, 49.98] / 40.23 [35.68, 42.57] | 20.43 [20.01, 20.62] | 27.53 [26.14, 47.13] | 405.24 [404.88, 406.00] / 424.19 [421.47, 426.33] | 15.85 [14.88, 16.10] / 24.90 [22.04, 27.11] | PASS (3/3) |
| pan_zoom / online | 15 s + 3 s | 8 [8, 8] (31 [31, 33]) | 500 / 2000 ms | 60.13 [60.12, 60.15] / 49.80 [49.49, 49.94] / 40.42 [39.84, 40.94] | 20.08 [20.02, 20.21] | 32.86 [27.90, 43.74] | 407.36 [405.97, 409.04] / 430.00 [425.00, 430.23] | 15.86 [15.75, 19.50] / 28.50 [25.61, 97.22] | PASS (3/3) |

The 97.22% worst-run CPU peak above exceeds the 75% peak lock, while the
pre-approved median peak is 28.50% and passes. It is retained as an explicit
outlier rather than hidden or used to retune the gate.

## End of Unit 07C — vector

| Scenario / mode | Duration + warm-up | Fresh PSS samples (rows) | Sampler fast / PSS | FPS avg / p5 / 1% low | Frame p95 ms | Sampler p95 ms | PSS avg / peak MiB | CPU avg / peak % | Status |
| --- | ---: | ---: | ---: | --- | ---: | ---: | --- | --- | --- |
| navigation_replay / offline | 15 s + 3 s | 8 [7, 8] (31 [31, 31]) | 500 / 2000 ms | 59.63 [59.39, 59.86] / 49.42 [48.66, 49.55] / 29.34 [24.26, 34.12] | 20.23 [20.18, 20.55] | 28.70 [26.47, 29.67] | 447.57 [441.06, 450.73] / 539.78 [538.00, 544.94] | 18.54 [17.76, 18.57] / 37.58 [27.83, 41.58] | PASS (3/3) |
| navigation_replay / online | 15 s + 3 s | 8 [7, 8] (31 [31, 31]) | 500 / 2000 ms | 59.67 [59.61, 59.76] / 49.14 [49.07, 50.62] / 32.21 [31.82, 33.56] | 20.35 [19.76, 20.38] | 29.59 [25.65, 30.91] | 438.30 [434.35, 445.23] / 537.14 [531.56, 547.71] | 17.95 [17.90, 18.44] / 39.82 [26.00, 40.42] | PASS (3/3) |
| pan_zoom / offline | 15 s + 3 s | 8 [7, 8] (31 [31, 31]) | 500 / 2000 ms | 60.12 [60.11, 60.16] / 49.33 [49.23, 50.12] / 40.23 [38.08, 41.76] | 20.27 [19.95, 20.31] | 28.76 [24.39, 36.02] | 417.43 [415.07, 419.81] / 437.39 [432.92, 440.89] | 16.84 [16.76, 17.42] / 28.33 [27.72, 30.00] | PASS (3/3) |
| pan_zoom / online | 15 s + 3 s | 7 [7, 8] (32 [31, 33]) | 500 / 2000 ms | 60.14 [60.13, 60.17] / 48.51 [47.74, 49.32] / 39.43 [37.84, 39.52] | 20.53 [20.28, 20.67] | 35.95 [25.30, 45.80] | 418.09 [413.14, 419.04] / 435.01 [428.81, 439.15] | 16.87 [16.83, 18.90] / 28.61 [24.05, 62.50] | PASS (3/3) |

## End of Unit 07C — raster fallback

| Scenario / mode | Duration + warm-up | Fresh PSS samples (rows) | Sampler fast / PSS | FPS avg / p5 / 1% low | Frame p95 ms | Sampler p95 ms | PSS avg / peak MiB | CPU avg / peak % | Status |
| --- | ---: | ---: | ---: | --- | ---: | ---: | --- | --- | --- |
| navigation_replay / offline | 15 s + 3 s | 8 [7, 8] (32 [32, 32]) | 500 / 2000 ms | 59.86 [59.20, 59.89] / 51.16 [51.07, 51.17] / 37.97 [36.19, 38.00] | 19.55 [19.54, 19.58] | 25.29 [24.27, 28.42] | 386.23 [381.21, 399.75] / 488.12 [487.06, 490.16] | 13.31 [12.90, 13.47] / 24.80 [23.03, 25.90] | PASS (3/3) |
| navigation_replay / online | 15 s + 3 s | 8 [7, 8] (32 [32, 32]) | 500 / 2000 ms | 59.74 [59.41, 59.85] / 50.61 [50.47, 51.64] / 38.32 [36.48, 38.44] | 19.76 [19.36, 19.81] | 26.43 [25.34, 26.53] | 389.94 [384.83, 398.23] / 487.62 [487.41, 487.77] | 13.71 [13.43, 14.04] / 26.05 [24.31, 50.00] | PASS (3/3) |
| pan_zoom / offline | 15 s + 3 s | 8 [8, 8] (32 [32, 32]) | 500 / 2000 ms | 60.13 [60.12, 60.14] / 52.96 [52.93, 53.53] / 39.15 [39.15, 44.75] | 18.88 [18.68, 18.89] | 21.56 [20.27, 21.84] | 333.59 [332.21, 333.94] / 342.67 [341.01, 343.14] | 5.97 [5.80, 6.21] / 12.50 [11.48, 18.29] | PASS (3/3) |
| pan_zoom / online | 15 s + 3 s | 8 [8, 8] (32 [32, 33]) | 500 / 2000 ms | 60.13 [60.13, 60.19] / 53.83 [53.04, 53.85] / 39.37 [36.32, 42.89] | 18.57 [18.57, 18.85] | 22.49 [20.10, 23.19] | 333.74 [333.29, 342.21] / 342.74 [341.66, 350.60] | 5.84 [5.64, 6.71] / 11.52 [11.50, 34.09] | PASS (3/3) |

## Interpretation of the Unit 06 difference

The old Unit 06 navigation report used 60 measured seconds after a 5-second
warm-up and about 30 fresh PSS samples/120 fast rows per run. Its PSS averages
were 496.54 MiB offline and 505.69 MiB online, while peaks were 527.11 and
541.62 MiB. This A/B uses 15 measured seconds, 3 seconds warm-up, and 7–8 fresh
PSS samples/31–32 rows. On the unchanged pre-07 code, it produces 426.94 and
444.92 MiB averages but nearly unchanged 536.90/533.23 MiB peaks.

The same-day control therefore disproves a 70 MiB code optimization in 07B.
The lower average is a windowing effect: navigation builds the Valhalla working
set during the run; the 60-second Unit 06 window spends much more time near the
post-route high-water mark, while the 15-second review window includes a larger
fraction of the lower initial phase. Conversely, route calculation, one
re-route, and continuous rendering are amortized over only 15 seconds, so CPU
average rises from about 8.5% to 11–12% on the unchanged control. CPU peak is a
single 500 ms sample and is noisy: the same-day main median is 35.07–41.75%,
with worst runs 43.33–49.39%, overlapping the old Unit 06 median 37.72–38.81%
and worst run up to 58.67%. There is no supported claim that 07B reduced the
CPU peak.

## Raster cache and lifecycle evidence

The raster limit is not merely compressed PNG bytes: 8 MiB is the server-side
encoded LRU, 8 MiB is Qt's compressed tile cache, 16 MiB is Qt's decoded
texture cache, and 32 MiB is reserved for the unavoidable visible-tile working
set at the supported 1920x1080 maximum. Disk cache and prefetch are disabled.
GPU/driver allocations cannot be hard-capped by the Qt API, so the complete
process tree is measured as the enforcement backstop.

During continuous pan/zoom, raster PSS average/peak is 333.59/342.67 MiB
offline and 333.74/342.74 MiB online; RSS average/peak is 414.50/421.66 and
414.41/421.65 MiB. The same-revision vector values are 417.43/437.39 and
418.09/435.01 MiB PSS, with 499.79/517.31 and 500.77/516.10 MiB RSS. Raster is
therefore not added on top of a retained vector engine.

The review-only ten-cycle probe starts at 218,199 KiB PSS / 292,244 KiB RSS.
After every return to vector, `rasterResourcesAllocated` is false; vector PSS
stays within 203,032–217,272 KiB and RSS within 277,200–291,440 KiB, ending at
207,940/282,108 KiB. Raster phases stay within 166,100–187,868 KiB PSS and
240,268–262,036 KiB RSS. There is no monotonic growth, the raster worker is
released on all ten returns, and the final vector footprint is below the
initial baseline.

## Evidence locations

- Full generated reports: `generated/benchmarks/20261031-ab-v2-main-pre07/`,
  `generated/benchmarks/20261031-ab-v2-07b-final/`,
  `generated/benchmarks/20261031-ab-final-07c-vector/`, and
  `generated/benchmarks/20261031-ab-final-07c-raster/`.
- Lifecycle log: `generated/unit-07c/review-toggle-final.log`.
- 1366x768 raster screenshots:
  `generated/unit-07c/fallback-navigation-1366x768.png` and
  `generated/unit-07c/fallback-rerouting-1366x768.png`.

Generated benchmark data and screenshots remain ignored and are not committed.
