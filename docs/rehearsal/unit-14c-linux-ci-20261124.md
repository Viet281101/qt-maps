# Unit 14C — Linux CI and offline fixture harness

Status: [GitHub Actions run #45550719417](https://github.com/Viet281101/qt-maps/actions/runs/45550719417) passed on 2026-11-25 for [#77](https://github.com/Viet281101/qt-maps/issues/77). This workflow does not publish a v1.0 release or claim real-data/GUI/performance acceptance.

## Job contract

`.github/workflows/unit14c-linux.yml` pins `ubuntu-26.04` and checks Ubuntu 26.04, glibc 2.43 and Qt 5.15.18 before building. It fetches the Qt Location 5.15.18 source archive by pinned SHA-256, reconstructs the required private-header overlay, fetches MapLibre Native Qt v3.0.0 by checked commit, and builds it against that Qt kit. It builds QtMaps Release with CMake/Ninja and runs the CTest suite **excluding the benchmark CLI smoke** while the i915 freeze remains. Qt Quick is configured for offscreen/software use; no FPS, RAM or CPU results are emitted as acceptance evidence.

`tools/ci/make_unit14_fixture.py` generates only synthetic map bytes, tiny PNG raster, one FTS5 place and a **routable synthetic OSM-format road graph**; it omits orthography. Its `fixture.json` and the packaged manifest label every output as a fixture. The search manifest retains the Geofabrik URL solely because the current application validator uses it as a schema sentinel; `source.fixture_origin` records that no Geofabrik data was used. The application's OSM attribution field is likewise a fixed contract value, not a claim that the generated pixels or search place are real OSM data. The artifact NOTICE repeats this distinction. No protected Hanoi package, graph or Sentinel scene enters public CI.

`tools/unit14a-deliver.sh` is reused for stage creation and offline installation. The backwards-compatible additions allow absent optional orthography, a supplied graph ID/source SHA-256 and an FTS5 count checked against the package manifest instead of hard-coded 9,094. The real 14A stage still has 9,094 manifest rows and its prior invocation remains valid.

The job builds a fixture archive with a manifest, SHA256SUMS and selected runtime license texts, then extracts it inside a fresh Ubuntu 26.04 container launched with `--network none --cap-drop ALL`. `tools/ci/smoke_unit14c_archive.sh` checks network isolation, archive and stage hashes, ELF closure, installer activation, optional orthography absence, missing-bundle rejection, restart resolution, FTS5 count and a Valhalla route with maneuvers from the synthetic graph. It does **not** render a map. The real-data/layer/replay proof remains the [14A VM report](unit-14a-7480-vm-results.md).

The job retains only text `manifest.json` and `SHA256SUMS` for one day as PR evidence. The binary archive remains inside the CI job and is not uploaded or released while runtime/data distribution and release retention are undecided. A v1.0 Linux release workflow remains gated by nationwide routing, real-route navigation, licensed notices, reference-laptop benchmark/soak after the i915 freeze and owner review. Windows CI remains optional after v1.0.

## Local software-only verification on 2026-11-24

- The Qt private overlay generator reproduced all **163** locally prepared headers byte-for-byte. The fixture generator created ~196 KiB of source packages/graph and its one-shot Valhalla route returned nonzero distance with maneuvers.
- On laptop 7480, the fixture stage was installed inside an Ubuntu 26.04.1 container with `--network none`. The same installer accepted the one-row FTS5 package, activated the synthetic graph, rejected a missing bundle and passed same-content pointer rollback. A separately packaged archive was extracted in a clean, networkless container: hashes and app/MapLibre/service `ldd` closure passed, startup resolved after restart, orthography was absent as declared, and the installed synthetic graph returned a route with maneuvers. No GUI or benchmark ran.
- The local Release build completed. Five focused software-only CTests passed: search, package contract, FTS5, route navigation and Valhalla runtime. A broader `make test` invocation was interrupted after `qtmaps_benchmark_cli_smoke` timed out; that test starts the app in a benchmark scenario and is deliberately excluded from the CI workflow. The failed/aborted invocation is **not** counted as a passing full suite.

## Hosted result on 2026-11-25

Run #45550719417 built MapLibre v3.0.0 and QtMaps Release with Qt 5.15.18 on Ubuntu 26.04. **24/24 non-benchmark CTests passed.** The synthetic graph builder returned a route; the archived bundle passed checksum and ELF dependency checks, installer activation, controlled missing-bundle failure, restart resolution, and a route with maneuvers inside a fresh container started with `--network none --cap-drop ALL`. Optional orthography absence was verified. Only the 583-byte text provenance artifact was uploaded for one day; no binary bundle or protected Hanoi data was published. The run is CI proof of software-only packaging, not a GUI, real-Hanoi or performance measurement.

Two earlier hosted attempts exposed missing `lrelease` and Python `numpy` in the test interpreter; the workflow now installs `qttools5-dev-tools` and runs CTest with the distro Python packages. This result makes the 14C PR review-ready. Keep #77 open until its PR merges; v1.0 release remains gated separately by the Unit 14 plan.
