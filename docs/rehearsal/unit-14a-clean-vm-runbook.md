# Unit 14A clean Ubuntu 26.04 VM operator runbook

Status: script and staging validated on the development host; **clean VM execution and visual/real-route smoke are pending**. This is functional work only. Do not enable 3D acceleration, GPU passthrough, or benchmark tools while the i915 freeze remains.

On the development host, the 2026-11-22 stage was copied and installed inside a temporary bubblewrap namespace with no network interface and a writable ignored `generated/` tree. Checksums, Unit 12 activation, Unit 13 bundle/vector startup resolution, 9,094 real FTS5 rows, same-content rollback pointer mechanics and rejected missing-bundle activation passed. This is a host simulation, **not** a fresh Ubuntu image, real-route response or GUI proof. A true prior-version rollback requires a second approved bundle.

## Build and snapshot the VM

1. Obtain an authorized Ubuntu 26.04 Desktop ISO and verify its published checksum. On the host, create a dedicated 60 GB qcow2 disk, e.g. `qemu-img create -f qcow2 qtmaps-14a.qcow2 60G`. Install Ubuntu in QEMU/KVM with 4 vCPU, 8 GiB RAM, a standard virtual display without 3D acceleration and no passthrough. `virt-manager` may be used with the same settings. Leave 3D acceleration unchecked.
2. After the minimum desktop install, before copying QtMaps or data, power off and create the clean snapshot: `qemu-img snapshot -c clean qtmaps-14a.qcow2` (or a named virt-manager snapshot). Record ISO hash, Ubuntu build, VM settings and snapshot name. Do not take a snapshot while the guest is writing the disk.
3. With temporary network access if needed, provision the candidate runtime packages below in the guest, record `dpkg-query -W`, then shut down and snapshot the provisioned base separately. The clean snapshot remains unchanged. To test missing dependencies, return to clean before installing packages.
4. Copy the staged payload and `tools/unit14a-deliver.sh` by an approved shared folder, USB image or SCP. Disconnect the shared/network transport. Boot with virtual NIC detached or isolated, then inside the guest record `ip -4 route`, `ip -6 route`, `ip -br addr` and `ip -4 route show default`/`ip -6 route show default` (both default-route outputs must be empty). Do not change host networking.

## Prepare on source, install with one VM command

On the dev host, after the Qt 5.15.18 Release build and owner-approved data selection, run `tools/unit14a-deliver.sh prepare STAGE APP MAPLIBRE_ROOT PYVALHALLA_SITE VALHALLA_CONFIG VECTOR_DIR RASTER_DIR SEARCH_DIR ORTHOGRAPHY_DIR GRAPH_TILES_DIR`. The script copies real packages into a self-contained stage, creates Unit 12 contracts, retains OSM/ODbL and Copernicus attribution and writes SHA-256 for every staged file. The source package manifests remain unchanged. Use explicit paths. Do not commit the stage, raw Sentinel scene or graph to Git.

On the isolated guest, run **one command**: `./unit14a-deliver.sh install /path/to/copied-stage /home/USER/QtMaps-14A`. The destination must not exist. It verifies hashes, absence of default IPv4/IPv6 routes, disk space, package validation, activation, startup metadata, 9,094 actual FTS5 places, rollback in a separate activation root and rejection of a missing bundle. It refuses overwrite. `./unit14a-deliver.sh run /home/USER/QtMaps-14A` launches the real app with llvmpipe (`LIBGL_ALWAYS_SOFTWARE=1`, `GALLIUM_DRIVER=llvmpipe`) and offline activation roots. Do not add `--routing-fixture-only` or override the Valhalla config: Unit 13 must select the active graph in its private config.

## Manual functional smoke (record pass/fail and screenshot)

1. Confirm offline evidence and Settings readiness. Open the map; verify local Hanoi vector and attribution. Switch to raster fallback, then Sentinel-2 orthography (z12–z15); record visible content, source and attribution. If llvmpipe cannot render, mark GUI proof blocked, not passed.
2. Search for two known Hanoi places; confirm result source says local OSM and no fixture fallback. Select A/B and request a route. Record Valhalla local readiness, graph ID/path, route geometry, nonzero distance and maneuvers. If it falls back to fixture, mark real route failed. Start simulated replay and confirm camera/maneuver progression and re-route behavior.
3. Exit and run the launcher again; ensure Settings, map, search and route still work offline. Inspect `metadata/startup.json` and `metadata/failure.log`. The installer tests rollback pointer mechanics in an isolated root using the same real bundle twice; a true previous-version recovery remains unverified until a second approved version exists. Do not present this same-content test as a rollback of changed data.
4. Record date, VM image/snapshot, Ubuntu/Qt/MapLibre/Valhalla versions, package IDs/checksums, `ip` outputs, each check's pass/fail, screenshot paths and limitations. The screenshot review is functional evidence, not an FPS/RAM/CPU measurement.

## Size and dependency inventory from the development host

The 2026-11-22 real-data stage contains 393 hashed files and 509,179,106 bytes (~486 MiB) by `du -sb`, excluding Ubuntu/Qt system runtime and filesystem overhead. Components were measured individually by `du -sb`:

| Component | Bytes |
| --- | ---: |
| Vector Hanoi v3-r3 | 15,581,641 |
| Raster Hanoi v3-r4 | 38,442,131 |
| Search FTS5 (9,094 places) | 1,061,600 |
| Sentinel-2 L2A z12–z15 v2 | 14,701,688 |
| Valhalla Hanoi graph tiles | 175,042,264 |
| MapLibre runtime copy | 11,830,486 |
| Valhalla service and bundled wheel libraries | 251,213,002 |
| QtMaps executable | 1,249,016 |

The host `ldd` inventory for app, Valhalla and MapLibre (with its library directory on `LD_LIBRARY_PATH`) resolves the common C/C++, GL/X11/XCB, ICU/Harfbuzz/Freetype, Qt Core/Gui/Qml/Quick/Location/Positioning/Network/SQL and Valhalla bundled libraries. Candidate top-level Ubuntu packages for a minimal guest are `libqt5location5`, `libqt5location5-plugins`, `libqt5positioning5`, `libqt5sql5-sqlite`, `qml-module-qtlocation`, `qml-module-qtpositioning`, `qml-module-qtquick2`, `qml-module-qtquick-window2`, `libgl1-mesa-dri`, `libgl1`, `fonts-noto-core`, `python3`, `iproute2` and `coreutils`, plus apt-resolved dependencies. This is a **candidate**, not a verified minimal set: inspect `ldd` and missing QML imports on the clean image and record the exact added packages. Qt 5.15.18 must be verified in that image; no Qt runtime is bundled in the current stage. Headless `ldd` cannot establish llvmpipe/Qt plugin loading.
