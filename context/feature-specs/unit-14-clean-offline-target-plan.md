# Unit 14 — Linux offline delivery, CI, Windows readiness (proposed)

Tracked by [#71](https://github.com/Viet281101/qt-maps/issues/71) and planning [PR #72](https://github.com/Viet281101/qt-maps/pull/72). Linux is the v1.0 acceptance platform. Native Windows 11 is desirable, **not a v1.0 gate**. The owner will try the company Windows 11 laptop only after project completion and CI release v1.0 or later, using a CI-produced Windows artifact if one exists. Company policy may prevent that trial. No Windows target or date is committed. The i915 freeze remains: no GPU/OpenGL/MapLibre benchmark or soak now. Estimates below are engineering days, excluding external access and review delays.

## Order: 14A now → 14C Linux → v1.0; 14B readiness alongside spare capacity

### 14A — Clean-target Linux offline (3–5 days; first priority)

- **Goal:** One script installs and activates an authorized bundle on a fresh Ubuntu 26.04 VM or container, then runs an offline smoke. A second physical Linux machine is optional final confirmation.
- **Bundle inventory:** QtMaps Release executable; Qt 5.15.18 libraries, QML imports and platform/SQLite/Qt Location/MapLibre 3.0.0 plugins; compiler and transitive ELF libraries, fonts and notices; pinned pyvalhalla environment with `valhalla_service` and loopback config; compatible vector/raster/search packages and optional permitted orthography; separate Valhalla graph; Unit 12/13 manifests, checksums, activation CLI and launcher. Derive minimal system packages from `ldd` plus clean-image checks. Separate immutable data from writable activation/config/logs.
- **Completion:** Script checks hashes/disk space, activates local inputs and launches via activation roots. Fresh target evidence records image/version, disabled network, local search, real route/maneuvers, restart, rollback and failure states. Map/layer GUI proof requires a suitable display; headless service checks alone are insufficient. Record fixture or unavailable states honestly.
- **Dependencies/risks:** Unit 12/13 contracts, legal dataset/runtime transfer, storage, display and shared-library closure; VM rendering may not work reliably. No GPU benchmark is part of this unit.
- **Owner decisions:** Exact approved data/runtime transfer, target image/access/storage, orthography inclusion and whether optional physical confirmation is possible.

### 14C — CI and release provenance (Linux required, 2–4 days after 14A)

- **Goal:** A pinned Linux GitHub Actions job builds Release, tests, assembles the 14A bundle and reuses the same installer/smoke assertions in a fresh filesystem, offline during smoke. Provision dependencies before network isolation. Public CI uses redistributable small fixtures; protected real datasets need a private/local proof linked in the release evidence. An optional Windows build/non-render/bundle job can be a later Unit after v1.0 and is **not a merge gate by default**.
- **Completion:** PR job reports build/test, checksummed manifest, dependency closure, offline service/data smoke and explicit graphics omissions. Release workflow publishes only after the v1.0 gate below and identifies source commit/toolchain. No CI runner provides official FPS/RAM/CPU evidence; those measurements remain on the reference laptop after the i915 freeze lifts.
- **Dependencies/risks:** 14A script and inventory; CI image drift, storage, artifact licensing and private-data access. The CI Linux bundle cannot silently replace the full real-data offline proof with fixtures.
- **Owner decisions:** Public/private artifact policy, retention/storage cost and whether a post-v1.0 Windows CI job should ever become a merge gate.

### 14B — Windows readiness (lightweight, no date or v1.0 dependency)

- **Goal:** Record porting differences and avoid new Linux-only assumptions. When adjacent Linux work touches portable boundaries, isolate easy `prctl`/signal, `/proc`, shell-path and filesystem assumptions if software-only tests can verify the change. No Windows executable or Windows hardware proof is promised in Unit 14.
- **Completion:** A short inventory identifies platform-specific code and unresolved Qt/MapLibre/Valhalla, FTS5, activation-path, localhost and firewall checks. Any actual code change belongs to a separately scoped implementation PR with tests. Defer Windows CI bundle creation and native graphics validation until after v1.0, using CI output only for the company-laptop trial.
- **Dependencies/risks:** No Windows machine needed for inventory; Linux cannot prove Windows ABI, plugin/DLL closure, OpenGL, policy or installer behavior. Timebox to 0.5–1.5 days of documentation/inspection, with no v1.0 gate.
- **Owner decisions:** Later ZIP versus installer, permissible runtime/data distribution and whether to commission Windows CI after v1.0.

## Qt and MapLibre Windows evidence and decision gate

- The [Qt 5.15.2 release](https://www.qt.io/blog/qt-5.15.2-released) describes installer availability. The [Qt 5.15.18 LTS announcement](https://www.qt.io/blog/commercial-lts-qt-5.15.18-released) says online/offline installers and LTS source access are for valid subscription holders. Thus 5.15.18 is a prebuilt option **if licensing/access is confirmed**; do not assume the public Windows installer supplies it. The [Qt open-source download page](https://www.qt.io/development/download-open-source) does not specify which 5.15 patch binaries this account can install.
- [MapLibre Native Qt v3.0.0 release](https://github.com/maplibre/maplibre-native-qt/releases/tag/v3.0.0) explicitly offers a Windows binary built against **Qt 5.15.2**. Its [usage guide](https://github.com/maplibre/maplibre-native-qt/blob/main/docs/Usage.md) says the Qt integration uses OpenGL. Neither source proves that binary works with 5.15.18.
- For a future CI Windows artifact, preferred first check is an authorized Windows Qt 5.15.18 kit plus a matching MapLibre v3.0.0 build. Building Qt 5.15.18 from source requires access to its licensed LTS source and a Windows toolchain; Qt publishes no dependable elapsed-time estimate, so **effort remains unestimated until toolchain/license access and a CI spike**. Using 5.15.2 would diverge from the committed baseline and needs an explicit owner-approved comparison/deviation, security/license review and full CI/target proof; it is not an automatic shortcut. Qt [deployment guidance](https://doc.qt.io/archives/qt-5.15/windows-deployment.html) says `windeployqt` collects Qt dependencies, not all third-party DLLs. [pyvalhalla 3.9.1](https://pypi.org/project/pyvalhalla/3.9.1/) has win_amd64 wheels; service/process behavior still needs Windows proof.

## Proposed CI release v1.0 gate

Tag `v1.0` only from a reviewed `main` commit after: (1) 14A clean-target **real-data** offline proof with package provenance and honest bounded coverage; (2) Linux CI Release build, tests, bundle manifest and offline software smoke pass on the tagged commit; (3) nationwide Valhalla graph from the approved uncropped Vietnam PBF with build-time/peak-RAM evidence and routes over national/provincial roads; (4) simulated navigation and re-route verified end to end from a real engine route; (5) i915 freeze lifted by the owner, then locked Release FPS/frame-time/PSS/RSS/CPU scenarios and 30–60 minute soak on the reference laptop, with threshold outcomes reported truthfully; (6) licenses, attribution, version/checksum and release limitations published. A failed threshold is not silently called a pass; owner must decide the remediation or explicitly revise the release claim/scope before tagging.

Known limits can be documented without blocking v1.0: bounded Hanoi search and orthography coverage, simulated rather than live GPS, optional imagery absence where license excludes transfer, Windows native support and Windows CI, and a second physical Linux target. Nationwide routing, real-route navigation, clean Linux offline delivery, Linux CI, and reference benchmark evidence **cannot** be waived merely by listing them as limits under the current product goals.

Proposed release output: `qtmaps-v1.0-linux-x86_64-qt5.15.18.tar.zst` (or `.tar.gz` if tooling dictates), `qtmaps-v1.0-manifest.json`, `SHA256SUMS`, `LICENSES/NOTICE`, operator instructions and a dated acceptance report, all tied to tag/commit, OS/ABI, runtime versions, coverage and data provenance. Do not publish protected datasets in public CI: distribute authorized data separately with their own manifest/hash and required storage. If a later Windows CI workflow exists, name its separately tested output `qtmaps-v1.0-windows-x64-qt<verified-version>.zip`; absence of it does not block Linux v1.0. Keep immutable versioned release assets and CI build logs for a documented retention period; exact days, size and costs require owner approval. The company-laptop trial must use a retained CI artifact at v1.0 or later, never a manual dev build.

## Later company-laptop trial (15–20 minutes, only after CI Windows artifact exists)

Preflight with company IT/owner: permission to copy/extract and run unsigned `.exe` (SmartScreen, AppLocker/Defender), antivirus DLL scanning, USB or approved transfer route, write access under user profile, offline-data disk space, GPU/driver/OpenGL capability, and policy for loopback tile server and Valhalla ports/firewall prompts. Do not change policy without authorization. A portable ZIP extracted under the user profile may avoid admin installation but cannot bypass application control; keep listeners bound to `127.0.0.1` and document ports. If execution is blocked, use approved remote/Linux live demo or recorded video/report and record that native trial did not pass.

Smoke: verify CI artifact hash and version; open app (2 min), offline search (2), switch vector/raster and permitted orthography or record unavailable (3), request a real local route/maneuvers (3), start simulated replay/re-route (4), restart from activation roots (2), exercise documented rollback and reopen (3). Log `date | laptop/Windows build | CI tag/commit/artifact SHA-256 | dataset/graph version | each step pass/fail/blocked | error/policy | screenshot path | operator`. Owner must confirm IT permission, transfer route, storage, accepted delivery format and whether to proceed when a policy prompt occurs. Linux dev machine cannot validate Windows policy, graphics, DLLs, paths or this smoke.

## Ranked risks and remaining work

| Level | Risk | Response |
| --- | --- | --- |
| Critical | v1.0 performance/soak evidence is blocked by i915 freeze. | Owner decides when to lift freeze; preserve benchmark gate and release claim. |
| High | Clean Linux bundle may miss runtime libraries or authorized data. | 14A fresh-target inventory, checksums, license and offline smoke. |
| High | Nationwide graph generation and real-route navigation may exceed RAM/time or reveal integration defects. | Measure graph build and test actual routes separately from 14A. |
| Medium | Linux CI may prove only fixtures or lose release artifacts through retention. | Separate real-data evidence; pin sources and approve retention. |
| Low for v1.0 | Windows native build or corporate policy may block later trial. | Timeboxed readiness, optional Windows CI, ZIP/WSL2 or video fallback after v1.0. |

Nationwide routing work is Linux-first; a later Windows bundle must separately check graph format, paths, storage and RAM. Real-route navigation semantics are shared but need a later Windows smoke. Linux `/proc` performance instrumentation does not transfer to Windows; any Windows performance report needs separate instrumentation and thresholds.

## Open owner questions

1. What is the actual v1.0 submission/review deadline?
2. For a later Windows artifact, is portable ZIP sufficient or is an installer/signature required?
3. Which datasets and Qt/MapLibre/Valhalla runtimes may be distributed, and by which channel?
4. What are acceptable CI artifact size, storage cost and retention period?
5. Should a future Windows CI job ever be a merge gate, or only an optional release check?
6. Is a licensed Qt 5.15.18 Windows kit available for CI, and who can authorize its use? If not, should a 5.15.2 deviation be evaluated after v1.0?
