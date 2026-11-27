# Linux v1.0 completion plan

Status: execution in progress in [PR #82](https://github.com/Viet281101/qt-maps/pull/82), tracked by [#81](https://github.com/Viet281101/qt-maps/issues/81). V1-A and V1-B functional evidence is recorded; V1-C preparation is active. Ubuntu 26.04 x86-64 is the committed release target; Windows readiness does not gate v1.0. Estimates are engineering days, excluding graph runtime, owner review, hardware repair and distribution approval. The [Unit 14 plan](unit-14-clean-offline-target-plan.md) owns the release gate and artifact contract.

## Sequence and acceptance

| Step | Work and verifiable exit | Dependency | Estimate |
| --- | --- | --- | ---: |
| V1-A — Nationwide graph | Check the approved uncropped Vietnam PBF hash, free disk/RAM, Valhalla version/config and build host. Freeze a test matrix of at least three geographically separated routes before the build, including national and provincial roads. Build with bounded concurrency, recording elapsed time, peak RSS, graph size, input/config hashes and failures. Keep graph outside Git. On an offline service, verify the matrix, geometry and maneuvers, plus unreachable/out-of-coverage errors. | Existing Hanoi route contract; sufficient build resources. | 2–4 d plus measured build time |
| V1-B — Real-route navigation | Activate the V1-A graph through Unit 12/13. Drive selected-route replay and simulated deviation from successful **real** Valhalla responses, including a long route. Verify camera follow, rotation, maneuver progression, arrival, re-route and honest source/simulation labels. Add focused tests only for defects found; record screenshots/logs and Release build/test results. | V1-A graph; existing navigation controller. | 1–3 d |
| V1-C — Release preparation | Decide distribution rights, channel, artifact size/retention and notices. Reuse 14A packaging and 14C CI; add a reviewed release candidate workflow that ties a Linux executable bundle, manifest, SHA256SUMS, LICENSES/NOTICE, operator instructions and real-data acceptance report to one commit. Keep protected data in a separately authorized transfer. Verify extraction and offline smoke from the **candidate artifact** on clean Ubuntu 26.04 before publishing. | Owner distribution decisions; V1-A/B data versions. Can prepare in parallel. | 2–3 d |
| V1-D — Reference benchmark | **Blocked while the i915 freeze remains.** After the owner explicitly lifts it, run the locked Release FPS/frame-time and process-tree PSS/RSS/CPU scenarios with real local packages, nationwide graph and 30–60 minute soak on the reference Latitude 5480. Include the open orthography observation in [#15](https://github.com/Viet281101/qt-maps/issues/15). Publish raw/summary evidence and report every threshold as pass or fail. | Owner lifts freeze; V1-A/B; fixed benchmark procedure. | 1–3 d after unblock |
| V1-E — Tag and handoff | Recheck 14A real-data VM evidence, CI on the candidate commit, V1-A/B/V1-D results, notices, hashes and known limits. Review from `main`, tag `v1.0`, retain the CI-built artifact per approved policy and hand off the Linux runbook. Never use a fixture-only CI result as nationwide or GUI proof. | All gates above pass or an explicit owner-approved scope change. | 1 d |

V1-A is first. If its measured memory, storage or duration exceeds available resources, stop and choose a larger build host or a documented scope decision before continuing; do not silently crop the PBF. The local PBF is present at `generated/unit-01b/source/vietnam-latest.osm.pbf` (316 MB as observed on 2026-11-26); the dev machine showed 14 GiB total RAM, about 10 GiB available and 102 GiB free disk at planning time. These are preflight observations, **not** a graph capacity guarantee. Building the graph is software-only and does not authorize GPU work.

The V1-A route matrix is frozen before building the nationwide graph: Hanoi–Hai Phong (21.0278, 105.8342 to 20.8449, 106.6881), Hue–Da Nang (16.4637, 107.5909 to 16.0544, 108.2022), Ho Chi Minh City–Can Tho (10.7769, 106.7009 to 10.0452, 105.7469), and Thai Nguyen–Bac Kan (21.5942, 105.8482 to 22.1470, 105.8348). Inspect returned road names/classes for national and provincial road evidence; coordinates alone do not establish those classes. Also test an out-of-coverage location and record its explicit error. The graph build uses `tools/routing/build_nationwide_graph.py` with the approved PBF, the Unit 04 Valhalla 3.9.1 base config and two workers; generated config, logs and tiles stay under ignored `generated/`.

## Critical path and decisions

`V1-A → V1-B → V1-D → V1-E` is the sequential dependency chain. V1-C may start beside V1-A, but it must also finish before V1-E and cannot publish an artifact until distribution and acceptance evidence are settled. Unit 14A/14C remain completed; their Hanoi real-data VM and synthetic CI fixture prove different things. Bounded Hanoi search/orthography and simulated GPS can be stated as release limits. Native Windows support and the company-laptop trial remain after v1.0 and require a CI-produced Windows artifact if commissioned.

Owner decisions needed: (1) actual submission/review date; (2) authorized runtime/data redistribution and delivery channel, including Qt/MapLibre/Valhalla and OSM/Copernicus notices; (3) CI artifact size, cost and retention; (4) safe build host/resource ceiling if the nationwide graph exceeds the dev machine; (5) when the i915 freeze may be lifted. Do not infer approval from this plan. A failed locked performance threshold requires remediation or an explicit release-scope decision before tagging.

## Risks, highest first

| Risk | Mitigation |
| --- | --- |
| i915 freeze prevents the required reference benchmark and soak, so v1.0 cannot be tagged under the current gate. | Prepare V1-A/B/C now; run V1-D only after explicit owner clearance. |
| Nationwide graph build may exceed available RAM/disk/time or reveal route-quality gaps. | Measure preflight and build resources; keep original PBF and record failures before choosing a larger host. |
| Runtime/data redistribution rights and retention are undecided. | Inventory licenses/notices and prepare a candidate; publish only after owner decision. |
| Real-route replay may expose guidance or re-route defects not seen with Hanoi/fixtures. | Verify several real routes and separate service, UI and replay failures. |
| Ubuntu ABI or clean-target drift may break the release candidate. | Pin Ubuntu 26.04 and repeat archive extraction/offline smoke on a clean target. |
