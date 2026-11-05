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

| State | Platform | Scenario / mode | FPS avg / p5 / 1% low | Frame p95 ms | >50 ms frames | PSS avg / peak MiB | CPU avg / peak % | Sampler p95 ms |
| --- | --- | --- | --- | ---: | ---: | --- | --- | ---: |
| A+hooks | wayland | pan/zoom online | 56.82 [56.37,57.54] / 37.15 [36.73,40.47] / 20.20 [19.77,21.77] | 26.92 [24.71,27.22] | not instrumented | 442.13 [440.72,445.21] / 461.21 [457.93,462.00] | 30.28 [30.25,31.62] / 48.02 [45.41,56.02] | 94.38 [74.62,116.52] |
| A+hooks | wayland | pan/zoom offline | 56.48 [56.11,56.90] / 37.02 [36.87,38.50] / 20.22 [18.19,21.69] | 27.01 [25.97,27.12] | not instrumented | 445.62 [442.64,448.83] / 462.24 [460.28,466.54] | 31.20 [30.40,31.53] / 48.31 [44.35,62.50] | 107.62 [93.82,116.17] |
| HEAD | wayland | pan/zoom online | 54.67 [54.25,55.58] / 31.95 [31.77,33.35] / 18.83 [16.78,19.25] | 31.30 [29.99,31.48] | 17 [16,26] | 450.82 [448.44,452.68] / 469.00 [464.93,471.96] | 31.62 [31.23,32.15] / 58.33 [49.02,79.55] | 117.30 [79.06,117.48] |
| HEAD | wayland | pan/zoom offline | 54.80 [54.37,55.77] / 32.49 [31.73,34.06] / 19.15 [17.88,21.22] | 30.78 [29.36,31.48] | 17 [5,23] | 449.14 [448.57,454.91] / 468.01 [465.27,471.95] | 31.61 [29.89,32.91] / 49.79 [46.88,54.24] | 89.54 [67.69,124.38] |
| A+hooks | wayland | navigation online | 58.26 [58.04,58.29] / 46.54 [45.62,46.90] / 24.14 [23.59,25.73] | 21.49 [21.32,21.90] | not instrumented | 517.42 [514.48,520.04] / 555.75 [552.75,558.00] | 16.63 [15.62,16.70] / 52.34 [51.44,53.93] | 89.38 [81.16,90.39] |
| A+hooks | wayland | navigation offline | 58.09 [57.72,58.23] / 45.88 [44.08,46.54] / 24.41 [23.32,26.17] | 21.80 [21.49,22.68] | not instrumented | 517.48 [513.60,523.77] / 558.12 [553.35,562.17] | 16.22 [16.10,16.85] / 57.23 [50.71,60.20] | 87.62 [82.18,91.04] |
| HEAD | wayland | navigation online | 48.45 [48.28,48.86] / 25.90 [25.69,26.00] / 17.20 [15.49,17.49] | 38.62 [38.46,38.92] | 26 [20,38] | 512.49 [509.13,520.65] / 555.29 [551.32,558.88] | 19.11 [18.99,19.60] / 41.76 [35.71,49.15] | 67.62 [65.54,69.27] |
| HEAD | wayland | navigation offline | 48.82 [48.17,49.05] / 25.97 [25.75,26.38] / 17.45 [16.33,18.07] | 38.50 [37.91,38.84] | 20 [17,25] | 513.18 [512.86,517.35] / 553.05 [551.52,561.21] | 19.21 [18.86,19.35] / 40.82 [33.65,47.98] | 67.68 [64.05,71.34] |

The unchanged locks are FPS average/p5/1% low >= 30/24/20, frame p95 <= 50
ms, PSS average/peak <= 550/640 MiB, interactive CPU average/peak <= 40/75%,
and sampler p95 <= 50 ms.  On median values, all rows pass memory, CPU and
frame-p95 locks.  HEAD fails the 1%-low lock in both scenarios/modes; A+hooks
passes it.  All rows fail sampler-p95, recorded separately and not used to
block acquisition.  Per-run reports retain each individual threshold result.

## One-page platform summary

All values below are online 1%-low FPS; parentheses contain frame-p95 ms.
`—` means that scenario/platform combination was not measured, not zero.  The
Wayland A+hooks and HEAD rows are the final N=5 matrix; Wayland `905908e` is the
earlier validity N=3 navigation check.  The xcb rows are the separate cooled
N=3 matrix and run through XWayland.

| Scenario | Wayland: A+hooks | Wayland: `905908e` | Wayland: HEAD `0c20b97` | xcb: A+hooks | xcb: `905908e` | xcb: HEAD `0c20b97` |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Pan/zoom | 20.20 (26.92) | — | 18.83 (31.30) | 21.75 (28.03) | 15.27 (28.97) | 17.60 (28.95) |
| Navigation | 24.14 (21.49) | 14.14 (45.37) | 17.20 (38.62) | 24.94 (22.14) | 20.52 (21.35) | 23.26 (22.99) |

Interpretation is deliberately narrow: on xcb, HEAD navigation is close to
A+hooks (23.26 versus 24.94 FPS 1%-low) and better than the original demo;
HEAD pan/zoom is still below both the unchanged 20-FPS lock and A+hooks.  No
offline xcb row was measured.  A+hooks and `905908e` lack the `frame_long`
hook, so their >50-ms counts are unavailable.  The platform effect does not
establish which platform was used for the unlogged 03/11 run.

## Follow-up xcb matrix (online)

This measured follow-up uses `QT_QPA_PLATFORM=xcb` for every run while the
desktop session remains Wayland, so Qt is running through XWayland.  It is a
separate, cooled N=3 online matrix: 5 s warm-up, 60 s measurement, 60 s pause
between all A+hooks -> original demo `905908e` -> HEAD runs.  The runner had
to be given the system Qt 5.15.18 QML/plugin root; without it, the first
A+hooks attempt aborted before measurement with an incompatible 5.15.13/5.15.18
runtime mix.  That failed pre-measurement attempt is retained in
`generated/benchmarks/20261105-xcb-matrix/` but is not one of the three valid
runs below.  All valid runs used `powersave`; package temperature was 49--59 C
at start and 56--66 C at end, with observed CPU-frequency ranges recorded in
each run's environment JSON.

Values are median `[min, max]`.  CPU is machine-normalized.  A+hooks and the
original demo have no `frame_long` hook, so their `>50 ms` counts are not
available.  Sampler p95 is reported separately and does not block acquisition.

| State | Platform | Scenario / mode | FPS avg / p5 / 1% low | Frame p95 ms | >50 ms frames | PSS avg / peak MiB | CPU avg / peak % | Sampler p95 ms |
| --- | --- | --- | --- | ---: | ---: | --- | --- | ---: |
| A+hooks | xcb | navigation online | 57.39 [55.97,58.08] / 45.18 [41.90,45.91] / 24.94 [22.27,25.45] | 22.14 [21.78,23.87] | not instrumented | 506.93 [505.39,507.40] / 542.95 [539.45,546.20] | 15.66 [14.86,15.99] / 48.70 [48.02,53.96] | 81.96 [81.15,84.77] |
| Demo `905908e` original | xcb | navigation online | 58.39 [57.80,58.92] / 46.83 [43.29,51.58] / 20.52 [18.37,24.61] | 21.35 [19.39,23.10] | not instrumented | 503.50 [502.45,508.23] / 543.42 [540.34,544.55] | 15.68 [14.98,15.87] / 36.53 [34.72,39.72] | 36.39 [33.40,41.31] |
| HEAD `0c20b97` + hooks | xcb | navigation online | 58.13 [57.67,58.50] / 43.50 [38.94,45.18] / 23.26 [22.34,24.64] | 22.99 [22.13,25.68] | 8 [6,8] | 493.89 [491.18,498.97] / 530.09 [529.00,537.39] | 15.95 [15.20,17.11] / 35.78 [28.11,37.50] | 75.48 [73.90,80.04] |
| A+hooks | xcb | pan/zoom online | 56.34 [55.85,56.48] / 35.68 [34.42,36.33] / 21.75 [20.97,23.86] | 28.03 [27.52,29.06] | not instrumented | 414.53 [413.84,415.33] / 428.05 [426.28,430.48] | 27.81 [26.29,28.29] / 45.89 [45.11,64.52] | 112.67 [79.48,115.36] |
| Demo `905908e` original | xcb | pan/zoom online | 56.18 [56.05,56.31] / 34.52 [33.81,35.75] / 15.27 [15.05,15.68] | 28.97 [27.97,29.58] | not instrumented | 422.68 [422.60,424.38] / 439.27 [438.00,439.55] | 24.70 [24.62,24.72] / 38.66 [37.47,39.75] | 77.93 [76.18,90.55] |
| HEAD `0c20b97` + hooks | xcb | pan/zoom online | 55.84 [55.58,55.87] / 34.54 [34.06,35.58] / 17.60 [17.59,18.13] | 28.95 [28.11,29.36] | 25 [23,26] | 418.15 [417.17,419.79] / 429.35 [428.94,437.15] | 27.44 [26.92,28.44] / 86.96 [38.09,189.19] | 116.71 [74.76,128.37] |

For the same original `905908e` binary and protocol, the earlier validity
matrix measured Wayland navigation at 14.14 FPS 1%-low / 45.37 ms frame p95 /
543 MiB PSS average / 19.87% CPU average, versus xcb 22.47 / 20.56 ms /
502 MiB / 15.37%.  The new xcb N=3 result (20.52 FPS 1%-low, 21.35 ms p95)
is consistent with the prior xcb direction.  This is measured platform effect;
the unrecorded platform of the 03/11 historical run cannot be recovered.

## xcb visual review

`QT_QPA_PLATFORM=xcb make run` completed fixture-route smoke captures for
vector and forced local-raster fallback at 960x640 and 1366x768.  All four
captures report `mapReady=true`.  The xcb/XWayland images retain Vietnamese
text, route and A/B markers, and map detail without a visible blur or geometry
offset versus the corresponding Wayland review captures.  The session is GNOME
Wayland (`XDG_SESSION_TYPE=wayland`) with no Qt scale environment override;
the XWayland RANDR surface reports 3072x1728 at 96 DPI.  The images/logs remain
ignored generated evidence at `generated/review/20261105-xcb/`.  The older
Wayland window captures use its compositor scale path, so this visual review
does not compare pixels or make a performance claim.

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
viewport diagnostic.  It is explicitly a Wayland result.  The separate N=3
xcb/XWayland online matrix and its before/after runtime snapshots are in
`generated/benchmarks/20261105-xcb-matrix/`; it did not run offline because
the 45-minute measurement budget was consumed by the cooled online matrix.

**Inferred:** the Wayland-to-xcb change materially affects frame pacing, PSS,
CPU, and sampler timing on this machine; it can therefore explain a substantial
part of the apparent session-to-session navigation regression.  It does not by
itself prove that the 03/11 run was xcb, since that platform was not logged.
Projection remains the dominant residual Canvas cost; removing it requires
moving the overlay to QSG C++ (roughly 3--5 engineering days plus
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
exposure.  The default application launch does not force a QPA platform and
selects Wayland in this GNOME Wayland session; a production decision to force
xcb was not made or implemented.
