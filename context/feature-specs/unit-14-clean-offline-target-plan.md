# Unit 14 — Offline delivery and Windows 11 feasibility (proposed)

Tracked by [#71](https://github.com/Viet281101/qt-maps/issues/71). Reviewers use Windows 11, but their delivery mode is unknown. Linux remains the committed product platform; Windows is a feasibility spike pending the owner's acceptance decision. Estimates are engineering days, excluding access and review delays. The i915 GPU/OpenGL/MapLibre benchmark freeze remains in force.

## 14A — Clean-target Linux offline (3–5 days)

- **Goal:** Build one operator script to provision and smoke-test the bounded demo in a fresh Ubuntu 26.04 VM or container with networking disabled. Confirm on a second physical Linux machine only if available.
- **Bundle inventory:** QtMaps Release executable; compatible Qt 5.15.18 libraries, QML imports and plugins (platform, SQLite, Qt Location raster, MapLibre Native Qt 3.0.0); compiler runtime, transitive ELF libraries, fonts and notices; pinned pyvalhalla environment with `valhalla_service` and loopback config; compatible vector/raster/search bundle and optional permitted orthography; separate Valhalla graph; Unit 12/13 activation metadata, hashes, CLI and launcher. Derive and record exact minimal system packages with `ldd` and a clean image; do not assume a list from the developer host. Keep immutable data separate from writable activation/config/log paths.
- **Completion:** Script checks hashes and disk space, activates approved local inputs, starts from activation roots. Fresh target records image, versions, offline condition and outbound-network check, real local route and maneuvers, FTS5 search, restart, rollback/error behavior, and vector/raster/optional orthography selection. GUI map/layer evidence needs a software-render-capable VM/display or physical machine; headless container service checks alone do not prove rendering. Fixture results cannot count as real-route proof.
- **Dependencies:** Unit 12/13 contracts, authorized data/runtime transfer, storage and graphical target. Script and smoke harness are future implementation.
- **Risks:** Qt/MapLibre ABI and plugin closure, VM graphics, data size, fonts and licensing.
- **Owner decisions:** Dataset redistribution, target image/access/storage, orthography inclusion, physical confirmation.

## 14B — Windows 11 feasibility and bundle (5–10 days once Windows is available)

| Component | Assessment and upstream evidence | Required proof |
| --- | --- | --- |
| Qt 5.15.18 | **Needs work.** Qt 5 [supports Windows deployment](https://doc.qt.io/archives/qt-5.15/windows-deployment.html). | Obtain exact Windows kit; start with MSVC x64, assess MinGW only as a separate ABI path; build/test with matching runtime. |
| MapLibre Native Qt 3.0.0 | **Needs work.** [v3.0.0 release](https://github.com/maplibre/maplibre-native-qt/releases/tag/v3.0.0) offers a Windows Qt 5.15.2 binary; [usage guide](https://github.com/maplibre/maplibre-native-qt/blob/main/docs/Usage.md) names OpenGL backend. | Build or prove exact Qt 5.15.18/toolchain compatibility, plugin load and real Windows graphics. Existing Qt 5.15.2 artifact alone is insufficient. |
| Valhalla | **Needs work.** [pyvalhalla 3.9.1](https://pypi.org/project/pyvalhalla/3.9.1/) publishes win_amd64 wheels and native executables. | Probe pinned wheel's `valhalla_service`, graph/config, DLL closure and lifecycle. Native vcpkg build is fallback. Current Linux `prctl`/signals need platform handling. |
| SQLite FTS5 / localhost tile service | **Needs work.** Qt 5 [QSQLITE](https://doc.qt.io/archives/qt-5.15/sql-driver.html#qsqlite) and [Qt Network](https://doc.qt.io/archives/qt-5.15/qtnetwork-index.html) are portable APIs. | Verify deployed SQLite has FTS5; test 127.0.0.1 dynamic port and Windows firewall prompt. |
| Unit 12/13 paths and diagnostics | **Needs work.** Qt [QStandardPaths](https://doc.qt.io/archives/qt-5.15/qstandardpaths.html) defines platform writable locations. | Test drive letters, spaces, separators, pointer atomicity/rollback and permissions. Linux shell scripts, `/proc` and process cleanup require platform handling. |

- **Goal and delivery:** Propose portable ZIP assembled with matching Qt 5 `windeployqt --qmldir`, then explicitly include MapLibre/Valhalla and other third-party DLLs, notices, startup scripts and separately provisioned data. Qt [warns](https://doc.qt.io/archives/qt-5.15/windows-deployment.html) that windeployqt omits third-party libraries. ZIP is simple to inspect/replace but has manual extraction and prerequisite burden. Installer adds shortcuts, prerequisite setup, uninstall and possible signing (+3–6 days after ZIP).
- **Completion:** Windows 11 VM or machine builds/tests non-render paths; clean account validates DLL/QML/plugin closure, search, local route, activation, restart, rollback and firewall behavior. GUI map/layer acceptance requires Windows graphics hardware or suitable VM. Linux development host cannot prove Windows rendering or installer behavior; Windows CI can prove compile and headless boundaries only.
- **Fallbacks:** WSL2 Linux bundle needs Windows setup and GUI/graphics integration (+2–5 days feasibility); a Linux demo video/report takes ~1–2 days but cannot satisfy native execution. Replacing map/routing technology is a separate architectural change, likely multiple weeks.
- **Risks:** Exact Qt/MapLibre ABI and OpenGL, Linux-specific code, missing DLLs and firewall policy.
- **Owner decisions:** Native execution requirement, Windows x64 VM/machine access, ZIP versus installer/signature, accepted fallback.

## 14C — CI (2–4 days after 14A harness; Windows may add 2–4 days)

- **Goal:** Pin Ubuntu and Windows GitHub Actions images and toolchain inputs. Linux job builds Release, tests, constructs the 14A bundle, then reuses its script/smoke harness in a fresh filesystem with networking disabled during smoke. Provision tools before isolation. Public CI uses small redistributable fixtures; real protected-data proof runs privately/local.
- **Completion:** PR shows Linux build/test and offline service/data smoke logs plus manifest/hashes. Windows x64 job builds, runs non-render/offscreen tests and checks clean-directory bundle dependency closure. Record skipped graphics checks explicitly. Implement Linux job first so 14A assertions are reused.
- **Dependencies:** 14A script/manifest; 14B compiler/Qt kit and licensable dependency source for Windows.
- **Risks:** Runner image drift, dependency availability, CI storage. Hosted CI virtual hardware does not establish project FPS/RAM/CPU; official Release/soak remains on reference laptop once i915 freeze lifts.
- **Owner decisions:** CI cost/artifact retention, public/private data handling, Windows job merge gate.

## Ranked risks and remaining product acceptance

| Severity | Risk | Response |
| --- | --- | --- |
| Critical | Review mode and deadline unknown; Windows native app may be required. | Owner sets acceptance mode and date before committing delivery scope. |
| High | Qt 5.15.18/MapLibre Windows ABI, OpenGL and Linux-specific code may block native app. | Windows kit spike; accepted WSL2 or video/report fallback if native is not mandatory. |
| High | Bundle may miss shared libraries, legal data or storage. | 14A clean-target inventory, hashes/licenses and offline smoke. |
| Medium | Windows Valhalla service, FTS5, paths and firewall differ. | Clean-account probes and route/search tests. |
| Medium | CI success may be mistaken for rendering or performance acceptance. | Label CI as software/bundle proof only. |

Nationwide graph generation from Vietnam PBF can remain on Linux, but a Windows deliverable must check graph compatibility, storage/path handling and runtime RAM. Real-route navigation logic is shared; Windows needs its own end-to-end check. The Linux `/proc` sampler and official reference-laptop benchmark/soak are separate acceptance work; Windows measurements would require new instrumentation and approved thresholds.

## Questions for the owner

1. Will reviewers run QtMaps on Windows 11 themselves, watch a live/remote demo, or read a report? Is native Windows execution an acceptance condition or a preference?
2. Is a Windows 11 x64 VM or physical test machine available, including graphics and firewall access?
3. Is portable ZIP acceptable, or is an installer/signature required?
4. What is the actual submission/review deadline?
5. Which datasets and runtime binaries may be transferred, who operates the target, and what storage/CI artifact limits apply?
