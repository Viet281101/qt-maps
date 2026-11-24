# Unit 14A — clean Ubuntu 26.04 VM on laptop 7480

**Result, 2026-11-24:** functional offline smoke passed on a clean Ubuntu 26.04.1 Desktop VM with the approved real Hanoi data. This proves bounded Hanoi behavior, not nationwide routing, live GPS, release packaging or performance. The earlier container trial is supplemental only.

## Image and exact sequence

- Host: `viet-anh-Latitude-7480`, Ubuntu 24.04.4, KVM API 12, QEMU 8.2.2; containers stopped. Owner-approved deviation: **2 vCPU/6 GiB RAM/60 GiB qcow2** because the host has only 7.6 GiB RAM. QEMU used `-display gtk,gl=off -vga std`, no 3D/GPU passthrough. No OOM/swap failure was observed.
- ISO: Ubuntu 26.04.1 Desktop, SHA-256 `601e30fbf5d97759367c632e2c33630665039b7e2158fd068403da3ccf1bda1f`, matched the [official SHA256SUMS](https://releases.ubuntu.com/26.04/SHA256SUMS). Guest: Ubuntu 26.04.1 LTS, kernel 7.0.0-30-generic.
- Installed the essential/minimal Desktop selection with no network. Shut down and created snapshot `clean` **before installing any extra package** (2026-11-23 22:42:55). First provisioning attempt installed candidate packages but its inventory helper failed on `comm` sort order. Shut down, reverted to `clean`, fixed the helper, and repeated; no repair on the dirty guest. The second attempt passed. Snapshot `provisioned` was created at 22:59:03, before data transfer. [Snapshot list and `qemu-img check`](unit-14a-evidence/vm-image.txt) after final shutdown show no image errors.
- Host launcher: `~/QtMaps-14A-vm/run-vm.sh offline` with `-machine q35,accel=kvm -smp 2 -m 6144 -nic none`, a read-only 9p transfer and a writable evidence share. Guest commands: `bash /mnt/qtmaps/offline-install.sh`, `bash /mnt/qtmaps/offline-rollback.sh`, `bash /mnt/qtmaps/offline-run.sh`; wrappers call `tools/unit14a-deliver.sh install/verify-rollback/run` with paths in the [runbook](unit-14a-clean-vm-runbook.md). The app was also relaunched after host/guest reboot.
- [Pre-smoke network log](unit-14a-evidence/network.txt): only loopback `127.0.0.1/8` and `::1/128`; no IPv4/IPv6 route or default. Both stages passed in-guest `sha256sum -c`: [current](unit-14a-evidence/new-hashes.txt), [older](unit-14a-evidence/older-hashes.txt), [installed manifest](unit-14a-evidence/SHA256SUMS).

## Runtime, dependencies, sizes and attribution

Guest Qt Core `5.15.18+dfsg-1ubuntu1`; Qt Location/QML `5.15.18+dfsg-1`; MapLibre Native Qt **v3.0.0** (source tag and generated CMake config); pyvalhalla/Valhalla service **3.9.1**. Stage includes the Release executable, MapLibre library/plugin/QML, Valhalla service with wheel libraries, local config, Unit 12/13 contracts and real data. Qt runtime, Qt Location/SQLite plugins, llvmpipe, fonts and Python are Ubuntu packages, **not bundled**. `ldd` on the app, MapLibre plugin, Valhalla service and all staged shared libraries had zero `not found` lines ([full inventory](unit-14a-evidence/ldd-all-runtime.txt)); GUI launch also exercised plugins and fonts.

Top-level packages requested with `apt-get install --no-install-recommends`: `libqt5location5 libqt5location5-plugins libqt5positioning5 libqt5sql5-sqlite qml-module-qtlocation qml-module-qtpositioning qml-module-qtquick2 qml-module-qtquick-window2 libgl1-mesa-dri libgl1 fonts-noto-core python3 iproute2 coreutils`. Apt added **33 packages**, including transitive Qt/XCB/XKB libraries ([exact list](unit-14a-evidence/packages-added.tsv)). No further runtime library, QML import or font package was needed. This is the **observed sufficient set for this minimal Desktop image**, not a proven universal minimum. The first failure was the inventory helper, not a missing runtime dependency ([capture](unit-14a-evidence/provision-first-attempt-comm-error.png)).

| Component | Current stage bytes | Installed bytes |
| --- | ---: | ---: |
| Hanoi vector v3-r3 | 15,581,641 | 15,581,641 |
| Hanoi raster fallback v3-r4 | 38,442,131 | 38,442,131 |
| Hanoi FTS5 search, 9,094 places | 1,061,600 | 1,061,600 |
| Sentinel-2 L2A z12–z15 v2 | 14,701,688 | 14,701,688 |
| Hanoi Valhalla graph | 175,042,264 | 175,042,573, including contract |
| MapLibre runtime | 11,830,486 | 11,830,486 |
| Valhalla runtime | 251,213,002 | 251,213,002 |
| Entire stage/install | **509,179,106** | **509,180,200** |

Installed sizes: [component inventory](unit-14a-evidence/installed-by-component.tsv). Older real-data stage: **487,195,877 bytes**. SHA-256 of each stage's `metadata/SHA256SUMS`: current `8d253d8889869413eb4c054dd0394a260bc03a3657fa58c35776881119e4a9e0`; older `6be03a2ba491db149125e8817aa038ff0f9ee5cb72e735fd942b1c2b8469c2c6`. qcow2 virtual size: 64,424,509,440 bytes; host disk use after run: 7.46 GiB.

Vector, raster, search and graph manifests retain `© OpenStreetMap contributors, ODbL 1.0`; the orthography manifest retains `Contains modified Copernicus Sentinel data 2026`, scene ID and source hash. The app displays those attributions in the screenshots. Qt/font copyright records come from Ubuntu package metadata; MapLibre source has `LICENSES/`; pyvalhalla wheel metadata has `licenses/COPYING` and an auditwheel SBOM. **The transferable stage lacks a consolidated runtime `LICENSES/NOTICE` directory.** Add it and confirm redistribution channels before publishing a CI release. This owner-approved internal transfer does not establish public redistribution rights.

| License input | Verified local source / status |
| --- | --- |
| QtMaps executable | Repository `LICENSE` (MIT). |
| Qt 5.15.18 and Ubuntu system/Qt plugin libraries | Ubuntu 26.04 package `/usr/share/doc/<package>/copyright`; exact added packages and versions are in `packages-added.tsv`. Qt packages contain component-specific licenses, so a single blanket SPDX label would be inaccurate. |
| Noto font | Ubuntu `/usr/share/doc/fonts-noto-core/copyright` (package version `20201225-2build1`). |
| MapLibre Native Qt v3.0.0 | Local source `generated/unit-01b/maplibre-native-qt/LICENSES/` lists MIT, BSD-2-Clause, LGPL-3.0-only, GPL-2.0-only and GPL-3.0-only texts; applicable notices require review before redistribution. |
| pyvalhalla 3.9.1 / Valhalla service | Local wheel metadata `pyvalhalla-3.9.1.dist-info/METADATA` declares MIT and points to `licenses/COPYING`; bundled wheel libraries are listed in `sboms/auditwheel.cdx.json`. |
| OSM-derived packages/graph and Sentinel imagery | Included package manifests carry ODbL and Copernicus attribution, respectively; owner approved this internal transfer. |

## Smoke ledger

| Check | Result | Evidence |
| --- | --- | --- |
| One-command install, activation, valid startup, FTS5 count | **Pass** | [install log](unit-14a-evidence/install.log), [startup JSON](unit-14a-evidence/startup.json): 9,094 real rows |
| Offline before smoke | **Pass** | [network preflight](unit-14a-evidence/network.txt), QEMU `-nic none` |
| Vector map, raster fallback, Sentinel orthography and attribution | **Pass** | [vector](unit-14a-evidence/vector-route.png), [raster](unit-14a-evidence/raster.png), [orthography](unit-14a-evidence/orthography.png) |
| Local OSM FTS5 search for Hoàn Kiếm and Thái Hà | **Pass** | [Hoàn Kiếm](unit-14a-evidence/search-local.png), [Thái Hà](unit-14a-evidence/search-thai-ha.png) |
| Real Valhalla route and maneuvers, no fixture | **Pass** | [app log](unit-14a-evidence/app-run-1.log): active graph, `probeMode=unit04-routing-valhalla`, HTTP 200, 7,294 m/14 maneuvers; alternate 6,021 m/11 maneuvers |
| Navigation replay, maneuver progression and arrival | **Pass** | [progress](unit-14a-evidence/navigation-progress.png), [arrival](unit-14a-evidence/navigation-complete.png); simulated position |
| Simulated off-route re-route | **Pass** | [re-route screenshot](unit-14a-evidence/navigation-reroute.png), [restart log](unit-14a-evidence/app-run-2.log): new start coordinate, HTTP 200, 6,204 m/12 maneuvers |
| Restart after host/guest reboot | **Pass** | [screenshot](unit-14a-evidence/restart-route.png), [app log](unit-14a-evidence/app-run-2.log): active graph and route restored |
| Missing bundle rejected, active pointer preserved | **Pass** | [failure log](unit-14a-evidence/failure.log); installer checked no `failure/current` pointer |
| Rollback between distinct real-data versions and restart resolution | **Pass** | [rollback log](unit-14a-evidence/rollback.log), [startup JSON](unit-14a-evidence/rollback-two-version-startup.json): newer raster r4/Sentinel v2 to older raster r3/Sentinel v1, isolated activation root |

Both stages use the same Hanoi graph, so graph rollback is only a same-content pointer check. Valhalla logs optional tile/traffic-extract warnings while routes from loose graph tiles succeed. Search and imagery remain Hanoi-bounded. All GUI captures used llvmpipe; **no FPS/RAM/CPU benchmark was run**. A second physical Linux target and public-license-ready release archive remain separate work.
