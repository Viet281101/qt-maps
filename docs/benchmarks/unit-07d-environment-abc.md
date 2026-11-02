# Unit 07D environment-controlled A/B/C benchmark

## Decision

The Unit 06 source baseline (A) does not pass the unchanged full-protocol gate
in the current host environment. Its application FPS, frame-time, memory, and
CPU metrics pass, but sampler p95 exceeds the locked 50 ms limit in all four
rows. The independent sampler A/B test also fails its CPU-overhead lock. Per the
owner's decision tree, code optimization, commit bisect, and v2 layer bisect
stop here. This result does not establish a code regression between A, B, and C.

## Host and renderer

- Physical host: Dell Latitude 5480, Intel Core i5-6300U, 16 GiB RAM; DMI reports
  `Dell Inc. / Latitude 5480`; `systemd-detect-virt` reports `none`, and PID 1 is
  in `0::/init.scope`.
- OS/kernel: Ubuntu 26.04.1 LTS, `7.0.0-42-generic`.
- Runtime: Qt 5.15.18, MapLibre Native Qt 3.0.0, Valhalla 3.9.1.
- Session: Wayland; `QT_QPA_PLATFORM`, `LIBGL_ALWAYS_SOFTWARE`, and
  `GALLIUM_DRIVER` unset.
- GPU: `/dev/dri/card1` and `/dev/dri/renderD128` use `i915`; direct rendering
  and acceleration are enabled. `glxinfo -B` and `QSG_INFO=1` both identify
  `Mesa Intel(R) HD Graphics 520 (SKL GT2)`, OpenGL 4.6, Mesa 26.0.8. This is not
  llvmpipe/software rendering.
- Power: AC online. All four recorded governors were `powersave`.
- Before every run the harness recorded power, governor, load/uptime, GL, DRI,
  runtime versions, and background processes. Browser/IDE/build work was absent;
  the Codex UI renderer and GPU subprocesses were suspended during each timed
  window and resumed afterward.

The historical Unit 06 evidence used kernel 6.8.0-147, glibc 2.39, and Qt
5.15.13. The current session uses kernel 7.0.0-42, glibc 2.43, Qt 5.15.18, and
Mesa 26.0.8. A therefore controls for source but not for this runtime/OS drift.
The OS upgrade also left no installed Qt 5.15.18 `qml-module-qtquick2` or
`libqt5sql5-sqlite`; the repository's retained modules are Qt 5.15.13. A direct
Qt 5.15.18 CTest run therefore correctly fails the two runtime-loading tests
instead of silently mixing patch levels. The remaining 11 CTest targets and the
style/schema Python tests pass. Installing or replacing host packages was not
attempted during this audit.

## Protocol and states

- Full Unit 06 protocol: 5 s warm-up + 60 s measured duration, N=3, interleaved
  A -> B -> C for each repetition and mode. This is the only protocol used for
  PASS/FAIL below.
- Sampler: 500 ms fast tree sample, 2,000 ms fresh PSS sample. Each run contains
  121-122 tree samples and 28 fresh PSS samples.
- A: `e0a8086982d4f55cb10a78a9d5995ef5050938ba`, `main` immediately after PR #23.
- B: `a4227ba`, end of Unit 07C, vector mode.
- C: current pre-glyph v2 review state, package v2 + style #29, explicitly
  selected; the default package remains v1.
- CPU `1c` means Linux process normalization (100% = one logical CPU). CPU
  `all` means machine normalization (100% = all four logical CPUs).

Values are median `[min, max]` over three runs.

## Smoothness and sampling

| State | Scenario / mode | FPS avg | FPS p5 | FPS 1% low | Frame p95 ms | Sampler p95 ms | Locked result |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| A | navigation / online | 59.04 [58.72, 59.23] | 45.15 [44.36, 46.24] | 29.74 [28.48, 30.54] | 22.15 [21.62, 22.54] | 60.88 [52.23, 63.13] | FAIL sampler; all others PASS |
| A | navigation / offline | 59.09 [58.47, 59.32] | 45.71 [44.16, 45.80] | 29.13 [28.77, 30.50] | 21.88 [21.83, 22.64] | 57.20 [54.58, 58.84] | FAIL sampler; all others PASS |
| A | pan/zoom / online | 58.18 [58.15, 59.03] | 38.96 [38.00, 40.84] | 20.56 [19.33, 24.03] | 25.67 [24.47, 26.32] | 74.05 [71.80, 88.42] | FAIL sampler; all others PASS by median |
| A | pan/zoom / offline | 58.45 [58.37, 58.76] | 39.38 [38.89, 41.20] | 22.15 [21.47, 23.63] | 25.40 [24.27, 25.70] | 95.06 [83.45, 117.42] | FAIL sampler; all others PASS |
| B | navigation / online | 58.64 [56.69, 59.26] | 34.05 [33.81, 34.76] | 24.69 [23.65, 25.35] | 29.37 [28.75, 29.58] | 35.48 [35.38, 36.66] | FAIL PSS avg; all others PASS |
| B | navigation / offline | 59.11 [58.93, 59.16] | 34.36 [34.10, 34.94] | 24.36 [22.41, 25.20] | 29.10 [28.62, 29.33] | 36.39 [35.30, 38.33] | FAIL PSS avg; all others PASS |
| B | pan/zoom / online | 58.10 [57.13, 58.63] | 25.90 [25.44, 27.51] | 16.58 [16.30, 16.68] | 38.61 [36.35, 39.30] | 69.03 [59.59, 69.08] | FAIL 1% low + sampler; all others PASS |
| B | pan/zoom / offline | 58.11 [58.07, 58.16] | 27.32 [26.01, 28.53] | 14.74 [13.91, 16.68] | 36.61 [35.05, 38.45] | 79.29 [64.70, 80.85] | FAIL 1% low + sampler; all others PASS |
| C | navigation / online | 58.22 [51.31, 58.85] | 33.68 [26.96, 35.60] | 23.64 [16.32, 25.53] | 29.69 [28.09, 37.09] | 36.20 [35.60, 40.36] | PASS by median; one low outlier retained |
| C | navigation / offline | 58.31 [58.00, 59.23] | 33.66 [33.52, 34.36] | 23.65 [23.11, 25.66] | 29.71 [29.11, 29.84] | 37.34 [36.59, 38.47] | PASS |
| C | pan/zoom / online | 57.64 [56.82, 57.91] | 30.57 [28.02, 31.16] | 15.08 [13.69, 15.66] | 32.71 [32.09, 35.69] | 71.26 [66.70, 79.44] | FAIL 1% low + sampler; all others PASS |
| C | pan/zoom / offline | 56.88 [56.61, 57.48] | 28.00 [27.98, 30.33] | 13.61 [12.58, 14.44] | 35.72 [32.97, 35.74] | 91.81 [62.95, 106.61] | FAIL 1% low + sampler; all others PASS |

## Memory and CPU

| State | Scenario / mode | PSS avg MiB | PSS peak MiB | CPU 1c avg / peak % | CPU all avg / peak % |
| --- | --- | ---: | ---: | ---: | ---: |
| A | navigation / online | 520.15 [514.77, 525.50] | 562.76 [558.90, 564.07] | 54.50 [54.35, 54.92] / 185.02 [142.86, 185.26] | 13.63 [13.59, 13.73] / 46.26 [35.71, 46.32] |
| A | navigation / offline | 525.43 [524.40, 532.74] | 568.63 [563.54, 575.53] | 55.47 [53.96, 56.85] / 203.59 [171.95, 222.45] | 13.87 [13.49, 14.21] / 50.90 [42.99, 55.61] |
| A | pan/zoom / online | 449.35 [443.21, 450.52] | 467.77 [457.94, 471.03] | 129.20 [122.15, 132.92] / 217.30 [193.83, 219.12] | 32.30 [30.54, 33.23] / 54.32 [48.46, 54.78] |
| A | pan/zoom / offline | 444.42 [440.33, 446.54] | 465.98 [453.28, 466.52] | 128.81 [128.19, 130.09] / 207.55 [197.95, 400.00] | 32.20 [32.05, 32.52] / 51.89 [49.49, 100.00] |
| B | navigation / online | 551.65 [548.81, 553.26] | 593.81 [593.60, 595.22] | 76.64 [76.08, 78.10] / 156.94 [132.11, 190.10] | 19.16 [19.02, 19.53] / 39.24 [33.03, 47.53] |
| B | navigation / offline | 551.20 [550.66, 553.97] | 590.47 [589.50, 592.78] | 76.64 [76.50, 76.95] / 131.05 [120.41, 139.17] | 19.16 [19.12, 19.24] / 32.76 [30.10, 34.79] |
| B | pan/zoom / online | 485.69 [484.96, 488.87] | 504.64 [504.26, 508.17] | 103.49 [101.12, 104.83] / 165.24 [152.78, 175.00] | 25.87 [25.28, 26.21] / 41.31 [38.19, 43.75] |
| B | pan/zoom / offline | 482.72 [482.18, 487.39] | 501.60 [499.75, 502.12] | 106.96 [99.55, 107.72] / 189.98 [163.68, 212.12] | 26.74 [24.89, 26.93] / 47.50 [40.92, 53.03] |
| C | navigation / online | 540.30 [536.75, 552.85] | 567.87 [567.79, 637.48] | 84.48 [80.35, 85.83] / 126.25 [122.00, 198.40] | 21.12 [20.09, 21.46] / 31.56 [30.50, 49.60] |
| C | navigation / offline | 542.69 [536.93, 544.24] | 572.20 [569.57, 573.00] | 86.22 [85.65, 86.88] / 199.20 [137.45, 227.36] | 21.55 [21.41, 21.72] / 49.80 [34.36, 56.84] |
| C | pan/zoom / online | 474.93 [471.23, 477.08] | 488.34 [485.21, 490.05] | 123.28 [122.31, 124.03] / 219.11 [185.11, 272.73] | 30.82 [30.58, 31.01] / 54.78 [46.28, 68.18] |
| C | pan/zoom / offline | 474.57 [474.20, 475.44] | 488.22 [487.83, 488.87] | 127.50 [123.95, 134.62] / 238.94 [225.43, 251.53] | 31.88 [30.99, 33.66] / 59.74 [56.36, 62.88] |

Locked limits are unchanged: FPS avg >=30, p5 >=24, 1% low >=20;
frame average <=33.3 ms, frame p95 <=50 ms; PSS average <=550 MiB,
PSS peak <=640 MiB; RSS average <=650 MiB, RSS peak <=768 MiB;
interactive machine CPU average <=40%, peak <=75%; sampler p95 <=50 ms.
Every unlisted row/check passes by its N=3 median. The worst observed run is
retained in each cell's min/max range rather than discarded.

## Sampler overhead

With the same host controls, three paired 5 s warm-up + 10 s measurements gave
CPU overhead of 1.071, 1.523, and 1.617 machine-percentage points: median 1.523,
**FAIL** against <=1.0. PSS overhead was 1.110 MiB median
[-0.827, 2.723], **PASS** against <=10 MiB. This independently corroborates
that the current sampler/host combination cannot reproduce the original gate.

## UI fixes completed while performance work is stopped

- Disabled search placeholder now reads `Tìm địa điểm (sắp có)`.
- Route, travelled segment, A/B markers, and navigation puck render in a single
  top Canvas overlay above MapLibre road layers.
- Demo camera/pan is clamped to the approved data bounds, including viewport
  corners; the map raises zoom when the viewport itself is wider than coverage.
- Route-derived street pills are distance-filtered and capped by zoom, and pills
  near A/B are suppressed.
- The water polygon outline was removed to avoid seam-like connector lines.
- The hard-coded Hanoi label is hidden until #31 collision-aware labels exist.

No font was downloaded, no glyph PBF was generated, and #31 plus package
migration #32 remain stopped pending owner direction.
