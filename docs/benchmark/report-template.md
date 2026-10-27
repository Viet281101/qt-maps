# QtMaps Management Benchmark Report

- **Date/timezone:** TODO
- **Git commit:** TODO
- **Build:** Qt 5.15.13 Release, CMake/Ninja
- **Machine:** Dell Latitude 5480, i5-6300U, 2 physical cores / 4 logical
  CPUs, 16 GiB RAM, NVMe
- **MapLibre / Valhalla:** TODO / TODO
- **Dataset and disk size:** TODO
- **Sampling:** TODO ms CPU/RSS; TODO ms PSS; TODO-second warm-up; TODO
  repetitions

PSS is the primary RAM value; RSS is accompanying evidence. CPU is shown both
as Linux process-style (`100% = one logical CPU`) and machine-normalized
(`100% = all four logical CPUs`). Values are median [min, max] across runs.

## Results

| Scenario / network | FPS avg | FPS p5 | 1% low | Frame avg / p95 ms | PSS avg / peak MiB | RSS avg / peak MiB | CPU one-core avg / peak % | CPU all-core avg / peak % | Result |
| --- | ---: | ---: | ---: | --- | --- | --- | --- | --- | --- |
| Startup / online | N/A | N/A | N/A | N/A | TODO | TODO | TODO | TODO | TODO |
| Startup / offline | N/A | N/A | N/A | N/A | TODO | TODO | TODO | TODO | TODO |
| Idle / online | N/A | N/A | N/A | N/A | TODO | TODO | TODO | TODO | TODO |
| Idle / offline | N/A | N/A | N/A | N/A | TODO | TODO | TODO | TODO | TODO |
| Pan/zoom / online | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO |
| Pan/zoom / offline | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO |
| Route / online | N/A | N/A | N/A | N/A | TODO | TODO | TODO | TODO | TODO |
| Route / offline | N/A | N/A | N/A | N/A | TODO | TODO | TODO | TODO | TODO |
| Navigation replay / online | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO |
| Navigation replay / offline | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO | TODO |

Use `PASS`, `FAIL / không đạt`, or `UNAVAILABLE`. Never change the locked
threshold to make a measured result pass.

## Locked-threshold comparison

| Metric | Locked target | Measured median [min, max] | Result and analysis |
| --- | ---: | ---: | --- |
| Interactive average FPS | >= 30 | TODO | TODO |
| FPS p5 | >= 24 | TODO | TODO |
| 1% low | >= 20 | TODO | TODO |
| Average / p95 frame time | <= 33.3 / 50 ms | TODO | TODO |
| Idle CPU all-core average / peak | <= 5% / 15% | TODO | TODO |
| Interactive CPU all-core average / peak | <= 40% / 75% | TODO | TODO |
| Valhalla route CPU all-core peak | <= 65% | TODO | TODO |
| PSS average / peak | <= 550 / 640 MiB | TODO | TODO |
| RSS average / peak | <= 650 / 768 MiB | TODO | TODO |
| Sampler CPU / PSS overhead | <= 1 point / 10 MiB | TODO | TODO |
| Sampler-duration p95 | <= 50 ms | TODO | TODO |

## Report-only latency

| Scenario / network | Startup ms, median [min, max] | Route latency ms, median [min, max] |
| --- | ---: | ---: |
| Online | TODO | TODO |
| Offline | TODO | TODO |

## Management summary

Write three to five lines:

1. Overall demo readiness and which locked targets passed.
2. Any `FAIL / không đạt`, its user-visible impact, and the evidence-backed cause.
3. PSS as primary RAM plus the observed RSS/PSS difference and explanation.
4. Online/offline comparison and whether re-routing remained responsive.
5. Recommended next action without changing the approved threshold.

## Verification and limitations

- Full command and output directory: TODO
- External `ps`/`htop`/`pstree` cross-check and observed tolerance: TODO
- Missing permission, unavailable PSS, failed run, fallback, or fixture use: TODO
- Unverified items: TODO
