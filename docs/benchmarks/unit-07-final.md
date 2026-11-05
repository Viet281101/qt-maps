# Unit 07 final Canvas-route benchmark

## Scope and method

This closes the Canvas route-overlay pan/zoom investigation at `1341c89`
(product code remains the retained performance state `0c20b97`: geographic
bbox culling plus display-only, zoom-bucket polyline simplification).  The
comparison is deliberately labelled **A+hooks**, not A: it is a temporary
copy of historical source `e0a8086` with benchmark liveness hooks only.

All 40 runs are retained: two states, `pan_zoom` and `navigation_replay`,
online/offline, N=5, 5 s warm-up and 60 s measured.  Each repetition ran
A+hooks then HEAD for each scenario/mode.  The selected platform was
`QT_QPA_PLATFORM=wayland` for every run.  Session metadata records Wayland,
`ubuntu:GNOME`, `powersave` on CPUs 0--3, frequency and every exposed thermal
zone before and after each run.  The compositor name is recorded as
unavailable because `wmctrl` is not installed; no package was installed to
fill that gap.  The harness now logs pre-exposure events and only starts
warm-up after `window_exposed`.

No run was excluded and the locked thresholds were not edited.  The runner
records the requested before/after temperature/frequency snapshots, but did
not impose a separate cooling delay between runs; therefore it must not be
read as a cooled-between-every-run protocol.

Values are median `[min, max]` over five runs.  CPU is machine-normalized.
`>50 ms` is the event count from the frame-long hook.

## Final results

| State | Scenario / mode | FPS avg / p5 / 1% low | Frame p95 ms | >50 ms frames | PSS avg / peak MiB | CPU avg / peak % | Sampler p95 ms |
| --- | --- | --- | ---: | ---: | --- | --- | ---: |
| A+hooks | pan/zoom online | 56.82 [56.37,57.54] / 37.15 [36.73,40.47] / 20.20 [19.77,21.77] | 26.92 [24.71,27.22] | not instrumented | 442.13 [440.72,445.21] / 461.21 [457.93,462.00] | 30.28 [30.25,31.62] / 48.02 [45.41,56.02] | 94.38 [74.62,116.52] |
| A+hooks | pan/zoom offline | 56.48 [56.11,56.90] / 37.02 [36.87,38.50] / 20.22 [18.19,21.69] | 27.01 [25.97,27.12] | not instrumented | 445.62 [442.64,448.83] / 462.24 [460.28,466.54] | 31.20 [30.40,31.53] / 48.31 [44.35,62.50] | 107.62 [93.82,116.17] |
| HEAD | pan/zoom online | 54.67 [54.25,55.58] / 31.95 [31.77,33.35] / 18.83 [16.78,19.25] | 31.30 [29.99,31.48] | 17 [16,26] | 450.82 [448.44,452.68] / 469.00 [464.93,471.96] | 31.62 [31.23,32.15] / 58.33 [49.02,79.55] | 117.30 [79.06,117.48] |
| HEAD | pan/zoom offline | 54.80 [54.37,55.77] / 32.49 [31.73,34.06] / 19.15 [17.88,21.22] | 30.78 [29.36,31.48] | 17 [5,23] | 449.14 [448.57,454.91] / 468.01 [465.27,471.95] | 31.61 [29.89,32.91] / 49.79 [46.88,54.24] | 89.54 [67.69,124.38] |
| A+hooks | navigation online | 58.26 [58.04,58.29] / 46.54 [45.62,46.90] / 24.14 [23.59,25.73] | 21.49 [21.32,21.90] | not instrumented | 517.42 [514.48,520.04] / 555.75 [552.75,558.00] | 16.63 [15.62,16.70] / 52.34 [51.44,53.93] | 89.38 [81.16,90.39] |
| A+hooks | navigation offline | 58.09 [57.72,58.23] / 45.88 [44.08,46.54] / 24.41 [23.32,26.17] | 21.80 [21.49,22.68] | not instrumented | 517.48 [513.60,523.77] / 558.12 [553.35,562.17] | 16.22 [16.10,16.85] / 57.23 [50.71,60.20] | 87.62 [82.18,91.04] |
| HEAD | navigation online | 48.45 [48.28,48.86] / 25.90 [25.69,26.00] / 17.20 [15.49,17.49] | 38.62 [38.46,38.92] | 26 [20,38] | 512.49 [509.13,520.65] / 555.29 [551.32,558.88] | 19.11 [18.99,19.60] / 41.76 [35.71,49.15] | 67.62 [65.54,69.27] |
| HEAD | navigation offline | 48.82 [48.17,49.05] / 25.97 [25.75,26.38] / 17.45 [16.33,18.07] | 38.50 [37.91,38.84] | 20 [17,25] | 513.18 [512.86,517.35] / 553.05 [551.52,561.21] | 19.21 [18.86,19.35] / 40.82 [33.65,47.98] | 67.68 [64.05,71.34] |

The unchanged locks are FPS average/p5/1% low >= 30/24/20, frame p95 <= 50
ms, PSS average/peak <= 550/640 MiB, interactive CPU average/peak <= 40/75%,
and sampler p95 <= 50 ms.  On median values, all rows pass memory, CPU and
frame-p95 locks.  HEAD fails the 1%-low lock in both scenarios/modes; A+hooks
passes it.  All rows fail sampler-p95, recorded separately and not used to
block acquisition.  Per-run reports retain each individual threshold result.

## Route visibility and interpretation

The final N=5 matrix intentionally left render tracing off, so it does not
perturb its timings.  The immediately preceding HEAD step-b diagnostic
(`generated/benchmarks/20261104-step-b-smoke/`) measured viewport-route
vertices at median **0**, max **0** over 42 overlay paints.  This is measured
diagnostic evidence, not an N=5 result; it explains why this pan/zoom trace
mostly sees a route outside the viewport.

Measured earlier paint-stage tracing attributes 91.9% of direct Canvas work to
`fromCoordinate` projection.  The no-draw reference was about 26 FPS 1%-low;
HEAD pan/zoom remains below its 20-FPS lock and below that reference.  A+hooks
also varies by session and is not a claim about historical A without hooks.
The exposure-gated harness records pre-exposure events instead of dropping
them, which avoids treating a pre-exposure pause as a sampled rendering stall.

## Status and remaining options

**Measured:** the N=5 table above, each run's environment JSON, and the
viewport diagnostic.  Complete generated evidence is in
`generated/benchmarks/20261105-final-matrix/`.

**Inferred:** projection remains the dominant residual Canvas cost; removing
it requires moving the overlay to QSG C++ (roughly 3--5 engineering days plus
projection/visual-regression risk), or bundling the QML MapLibre module
(roughly 1--2 days plus runtime/package validation).  Neither option was
implemented.

**Not achieved / not measured:** A+hooks did not contain the later
`frame_long` event hook, so its >50-ms frame count is unavailable and cannot be
reconstructed from aggregate FPS.  Final matrix route-vertex counts were not
re-measured with tracing enabled.  No-draw was not re-run in this final matrix;
the cited ~26 FPS is a prior reference.  The requested cooling pause was not
implemented by the runner.  There is no evidence here to diagnose the old
2,996-ms no-draw exposure-adjacent stall beyond its timing relative to
exposure.
