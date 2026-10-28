# QtMaps Management Benchmark Report

- **Date/timezone:** 2026-10-28, Asia/Bangkok
- **Git commit:** `4fa777e` (unrelated workspace edits present)
- **Build/machine:** Qt 5.15.13 Release; Dell Latitude 5480, i5-6300U (2 physical / 4 logical CPUs), 15.50 GiB RAM
- **MapLibre / Valhalla / data:** 3.0.0 / 3.9.1; Hanoi tiles 14.00 MiB, graph 166.93 MiB
- **Sampling:** 500 ms CPU/RSS; 2,000 ms PSS; 5-second warm-up; 3 repetitions

PSS is the primary RAM value; RSS accompanies it. CPU below is machine-normalized (`100% = all four logical CPUs`). The table shows medians; per-run minimum/maximum ranges are retained in the linked generated report.

## Results

| Scenario / network | FPS avg / p5 / 1% low | PSS avg / peak MiB | RSS avg / peak MiB | CPU avg / peak % | Result |
| --- | --- | --- | --- | --- | --- |
| Startup / online | N/A | 140.44 / 140.44 | 222.98 / 227.25 | 24.16 / 24.16 | PASS |
| Startup / offline | N/A | 141.00 / 141.00 | 219.43 / 219.43 | 23.40 / 23.40 | PASS |
| Idle / online | N/A | 354.64 / 354.71 | 434.94 / 435.01 | 0.84 / 7.25 | PASS |
| Idle / offline | N/A | 360.77 / 360.82 | 441.26 / 441.31 | 0.81 / 2.00 | PASS |
| Pan/zoom / online | 60.03 / 49.44 / 40.16 | 417.45 / 429.87 | 499.57 / 510.28 | 17.41 / 33.07 | PASS |
| Pan/zoom / offline | 60.00 / 49.23 / 39.62 | 420.06 / 433.77 | 501.04 / 515.33 | 17.29 / 29.08 | PASS |
| Route / online | N/A | 409.00 / 502.88 | 536.93 / 583.14 | 8.81 / 13.56 | PASS |
| Route / offline | N/A | 414.06 / 503.58 | 542.35 / 583.99 | 11.97 / 12.96 | PASS |
| Navigation / online | 59.98 / 51.86 / 43.06 | 507.98 / 541.31 | 590.15 / 621.68 | 8.60 / 53.20 | PASS |
| Navigation / offline | 59.98 / 51.97 / 43.98 | 506.43 / 544.39 | 588.36 / 624.62 | 8.50 / 49.25 | PASS |

## Locked-threshold comparison

All locked targets passed. The closest primary-RAM result is navigation PSS average: median **506.43–507.98 MiB** versus the **550 MiB** limit; its accompanying RSS average is **588.36–590.15 MiB** versus 650 MiB. Interactive FPS p5 median >=49.23 (target 24; worst run 48.93), 1% low median >=39.62 (target 20; worst run 38.20), and frame p95 median <=20.31 ms (target 50 ms). Idle CPU peak median <=7.25% (target 15%), though one online run reached 23.81%. Navigation CPU peak median <=53.20% (target 75%; worst run 56.96%). Valhalla route CPU peak median <=10.48% (target 65%).

Sampler overhead passed: CPU **0.761** points `[0.738, 0.785]` versus <=1; PSS **4.740 MiB** `[3.565, 7.233]` versus <=10; sampler p95 median <=30.27 ms versus <=50 (worst run 44.7 ms).

## Report-only latency

Startup spans roughly 0.7–1.2 s across all 30 runs; the dedicated startup scenario median is online **855 ms** `[738, 886]` and offline **857 ms** `[697, 959]`. Route latency median is about 53 ms (online **53 ms** `[52, 53]`; offline **55 ms** `[48, 59]`), but the slowest individual route in a run is 192–235 ms, likely the first request/graph-page warm-up; this attribution is not yet independently verified.

## Management summary

The demo benchmark passes every locked RAM, CPU, FPS, frame-time and sampler-overhead target on the reference laptop. RSS exceeds primary PSS by 82.03 MiB because RSS counts shared mappings in full. Online and offline are expected to be similar because map data and Valhalla run locally. Valhalla accounts for about 217 MiB PSS while idle and about 360 MiB after route calculation; the remaining PSS belongs to the app. Pan/zoom PSS moved by no more than 2 MiB versus the prior table; route completion now forces a fresh PSS sample.

## Verification and limitations

- Command/output: `make benchmark`; `generated/benchmarks/20261028-184033/`.
- External cross-check: `ps` RSS matched sidecar within 0.004%; `smaps_rollup` PSS within 0.019%.
- PSS remains 2-second sampling during long scenarios; first-frame and completion checkpoints force a fresh PSS read.
- Startup and route each have only 1–3 PSS samples per run, so their PSS averages are less reliable than the long idle/pan/navigation scenarios.
- `idle-online-r1` has a single 23.81% machine-CPU sample at 65.034 s: `qtmaps` used 95.238% on the one-core scale while Valhalla used 0%; adjacent samples were 0.50% and 1.00%. It is an isolated app-process spike; its internal cause is not identified by the collected `/proc` data.
- No failed run, fallback, unavailable PSS, or threshold failure occurred.
