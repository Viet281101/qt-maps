# Unit 14B — Windows 11 readiness inventory

Status: documentation and source inspection for [#79](https://github.com/Viet281101/qt-maps/issues/79) only. Native Windows is optional and outside the Linux v1.0 gate. The company-laptop trial is deferred until a CI-produced Windows artifact exists at v1.0 or later. **No Windows build, Windows VM, company policy check or graphics test has been performed.**

## Porting inventory and next proof

| Area | Evidence today | Work before a Windows CI artifact |
| --- | --- | --- |
| Qt baseline | The [Qt 5.15.18 LTS announcement](https://www.qt.io/blog/commercial-lts-qt-5.15.18-released) describes subscription-gated installers/source. Public [Qt 5.15.2](https://www.qt.io/blog/qt-5.15.2-released) had installers. Access to a licensed 5.15.18 Windows kit is unconfirmed. | Confirm kit/license and MSVC or MinGW toolchain. Keep 5.15.18 unless the owner approves and tests a 5.15.2 deviation. |
| MapLibre Native Qt | The [v3.0.0 release](https://github.com/maplibre/maplibre-native-qt/releases/tag/v3.0.0) lists a Windows binary built with Qt 5.15.2; the [usage guide](https://github.com/maplibre/maplibre-native-qt/blob/main/docs/Usage.md) describes OpenGL rendering. Compatibility with the 5.15.18 baseline is unproven. | Build v3.0.0 against the selected Windows Qt kit; check plugin/DLL closure and render on an approved Windows target. |
| Valhalla | [pyvalhalla 3.9.1](https://pypi.org/project/pyvalhalla/3.9.1/) lists `win_amd64` wheels. `ValhallaServiceManager.cpp` uses a Linux-only parent-death hook and a Unix SIGINT branch; other platforms call `QProcess::terminate()`. | Prove wheel installation, service startup/shutdown, graph path and route on Windows CI/VM. Parent/child cleanup after crashes is unverified. |
| Benchmark/process monitoring | `src/perf/ProcessTreeSampler.cpp` includes `<unistd.h>` and reads `/proc`; `app/main.cpp` reads `/proc/.../smaps_rollup`. These are Linux implementations and block or degrade a Windows build/report. | Isolate the Linux sampler in CMake or implement a Windows sampler before Windows build. Define Windows metrics separately; Linux thresholds do not transfer. |
| Bundle and activation | `tools/unit14a-deliver.sh` hardcodes `/usr/lib/x86_64-linux-gnu/qt5` and uses Bash. `OfflinePackageInstaller.cpp` uses Qt paths and `QFile::rename` for current/previous pointers. | Add a Windows packaging/install path; verify drive letters, writable user-profile roots, atomic pointer swaps, rollback and restart on NTFS. The Qt API use alone does not prove Windows behavior. |
| SQLite, tiles and loopback | Map/search use Qt `QSQLITE` and FTS5; tile servers use `QTcpServer` on `QHostAddress::LocalHost`; Valhalla config requires `tcp://127.0.0.1:<port>`. | Verify deployed SQLite plugin has FTS5, DLL closure, loopback binding, and company firewall/Defender behavior. The [Qt deployment guide](https://doc.qt.io/archives/qt-5.15/windows-deployment.html) covers `windeployqt` for Qt dependencies; third-party runtime closure still needs inspection. |

No application code is changed in 14B. The first Windows implementation PR should start with a non-render compile and process-monitor boundary, then a CI-built ZIP and offscreen data/service tests. A real Windows display and company policy still require an approved Windows environment; a Linux cross-build cannot certify them. No date is committed.

## Company laptop trial, after CI Windows artifact

The [Unit 14 plan](../../context/feature-specs/unit-14-clean-offline-target-plan.md#later-company-laptop-trial-1520-minutes-only-after-ci-windows-artifact-exists) contains the 15–20 minute smoke and result record. Before transfer, the owner/IT must confirm extract/run rights, unsigned `.exe` policy (SmartScreen, AppLocker, Defender), antivirus DLL scanning, allowed USB or other transfer, writable user-profile path, disk space for offline data, GPU/driver/OpenGL capability, and localhost firewall prompts. Do not weaken company policy. A portable ZIP under the user profile may avoid admin installation but may still be blocked by application control. If blocked, record the policy failure and use an approved Linux demo or video/report; do not mark native Windows as passed.

Trial record: `date | Windows build/driver | CI tag/commit and artifact SHA-256 | dataset/graph version | open/search/layers/real route/replay/restart/rollback pass-fail-blocked | error/policy | screenshot path | operator`. Use the CI artifact, not a developer-built executable.

## Owner decisions still open

Submission/review deadline; ZIP versus installer/signing; runtime and dataset redistribution rights; CI artifact size/cost/retention; whether a future Windows job is a merge gate; licensed Qt 5.15.18 Windows kit/toolchain access. These decisions do not delay Linux 14A/14C or v1.0 under the current plan.
