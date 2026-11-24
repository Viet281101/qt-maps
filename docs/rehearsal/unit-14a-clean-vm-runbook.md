# Unit 14A clean Ubuntu 26.04 VM operator runbook

Status: **clean Ubuntu 26.04.1 VM smoke passed on laptop 7480 on 2026-11-24**. See the [dated VM results and evidence](unit-14a-7480-vm-results.md). This is functional work only. Do not enable 3D acceleration, GPU passthrough, or benchmark tools while the i915 freeze remains.

On the development host, the 2026-11-22 stage was copied and installed inside a temporary bubblewrap namespace with no network interface and a writable ignored `generated/` tree. Checksums, Unit 12 activation, Unit 13 bundle/vector startup resolution, 9,094 real FTS5 rows, same-content rollback pointer mechanics and rejected missing-bundle activation passed. This was a host simulation, **not** the fresh Ubuntu image, real-route response or GUI proof. The later VM trial used a second approved bundle for distinct-version rollback.

## Laptop 7480 preflight (2026-11-23; not VM acceptance)

SSH inspection on `viet-anh-Latitude-7480` found Ubuntu 24.04.4, 181 GB free disk, VT-x flags, loaded `kvm_intel`/`kvm`, and a usable `/dev/kvm` (`KVM_GET_API_VERSION=12` as the target user). QEMU 8.2.2 was subsequently installed. Physical RAM is 8,011,244 kB (about 7.6 GiB), so the requested 8 GiB guest cannot be allocated safely alongside the host. The owner approved a **6 GiB, 2-vCPU** VM on this laptop (2 physical cores/4 threads), with host containers stopped before launch. OOM or swap-related failures are resource failures, not bundle failures. This trial had no such failure.

A separate Ubuntu 26.04 Docker container on that laptop had Qt 5.15.18, `--network none`, no default route, llvmpipe and the approved real-data stage with verified SHA-256. The installer passed activation, 9,094 FTS5 rows, same-content rollback and missing-bundle rejection. MapLibre showed the Hanoi vector map, and Valhalla 3.9.1 returned a non-fixture 7,294 m route with 14 maneuvers. The owner manually exercised the container app and reported the functions working; zoom felt slightly jerky under software rendering. This is qualitative feedback, not a performance result. The container had no clean VM snapshot or distinct-version rollback and does **not** close Unit 14A.

## Build and snapshot the VM

1. Obtain an authorized Ubuntu 26.04 Desktop ISO and verify its published checksum (the [official release directory](https://releases.ubuntu.com/26.04/) publishes both ISO and `SHA256SUMS`). On the host, create a dedicated 60 GB qcow2 disk, e.g. `qemu-img create -f qcow2 qtmaps-14a.qcow2 60G`. Install Ubuntu in QEMU/KVM with a standard virtual display without 3D acceleration and no passthrough. Baseline is 4 vCPU/8 GiB; the approved 7480 trial uses 2 vCPU/6 GiB. Leave 3D acceleration unchecked.
2. After the minimum desktop install, before copying QtMaps or data, power off and create the clean snapshot: `qemu-img snapshot -c clean qtmaps-14a.qcow2` (or a named virt-manager snapshot). Record ISO hash, Ubuntu build, VM settings and snapshot name. Do not take a snapshot while the guest is writing the disk.
3. With temporary network access if needed, provision **only** the candidate runtime packages below in the guest, record `dpkg-query -W`, then shut down and snapshot the provisioned base separately. The clean snapshot remains unchanged. After any failed attempt, revert to the clean snapshot before trying a corrected package set; log the missing library/plugin/font symptom and the added package. Never repair an already failed guest in place.
4. Copy the staged payload and `tools/unit14a-deliver.sh` by an approved shared folder, USB image or SCP. Disconnect the shared/network transport. Boot with virtual NIC detached or isolated, then inside the guest record `ip -4 route`, `ip -6 route`, `ip -br addr` and `ip -4 route show default`/`ip -6 route show default` (both default-route outputs must be empty). Do not change host networking.

The 7480 VM was launched with `-machine q35,accel=kvm -smp 2 -m 6144 -display gtk,gl=off -vga std -nic none`, a read-only 9p transfer with tag `qtmaps` and writable evidence share tagged `qtmaps_evidence`. After provisioning and before smoke, mount and inspect them in the guest:

```bash
sudo mkdir -p /mnt/qtmaps /mnt/evidence
sudo mount -t 9p -o trans=virtio,version=9p2000.L,ro qtmaps /mnt/qtmaps
sudo mount -t 9p -o trans=virtio,version=9p2000.L qtmaps_evidence /mnt/evidence
ip -br addr; ip -4 route; ip -6 route
(cd /mnt/qtmaps/new && sha256sum -c metadata/SHA256SUMS)
(cd /mnt/qtmaps/older && sha256sum -c metadata/SHA256SUMS)
bash /mnt/qtmaps/unit14a-deliver.sh install /mnt/qtmaps/new /home/qtmaps14a/QtMaps-14A
bash /mnt/qtmaps/unit14a-deliver.sh verify-rollback /mnt/qtmaps/older /home/qtmaps14a/QtMaps-14A
bash /mnt/qtmaps/unit14a-deliver.sh run /home/qtmaps14a/QtMaps-14A
```

Install is the single provisioning command for the prebuilt stage; the separate rollback and GUI commands are acceptance checks. During the actual 7480 run, thin `offline-install.sh`, `offline-rollback.sh` and `offline-run.sh` wrappers ran these commands while teeing logs to `/mnt/evidence`. Record hashes and network state **before** launching the GUI.

## Prepare on source, install with one VM command

On the dev host, after the Qt 5.15.18 Release build and owner-approved data selection, run `tools/unit14a-deliver.sh prepare STAGE APP MAPLIBRE_ROOT PYVALHALLA_SITE VALHALLA_CONFIG VECTOR_DIR RASTER_DIR SEARCH_DIR ORTHOGRAPHY_DIR GRAPH_TILES_DIR`. The script copies real packages into a self-contained stage, creates Unit 12 contracts, retains OSM/ODbL and Copernicus attribution and writes SHA-256 for every staged file. The source package manifests remain unchanged. Use explicit paths. Do not commit the stage, raw Sentinel scene or graph to Git.

On the isolated guest, run **one command**: `./unit14a-deliver.sh install /path/to/copied-stage /home/USER/QtMaps-14A`. The destination must not exist. It verifies hashes, absence of default IPv4/IPv6 routes, disk space, package validation, activation, startup metadata, 9,094 actual FTS5 places, rollback in a separate activation root and rejection of a missing bundle. It refuses overwrite. `./unit14a-deliver.sh run /home/USER/QtMaps-14A` launches the real app with llvmpipe (`LIBGL_ALWAYS_SOFTWARE=1`, `GALLIUM_DRIVER=llvmpipe`) and offline activation roots. Do not add `--routing-fixture-only` or override the Valhalla config: Unit 13 must select the active graph in its private config.

## Distinct-version rollback proof

The default stage is the newer real bundle (raster `hanoi-raster-20261115-v3-r4`, Sentinel `hanoi-orthography-20261115-s2-l2a-v2`). Prepare a second approved **older** real-data stage from the same vector, search and Hanoi graph, with raster `hanoi-raster-20261114-v3-r3`, Sentinel `hanoi-orthography-20261115-s2-l2a-v1`, and bundle ID `hanoi-unit14a-v1`. Both raster manifests bind to the vector v3-r3 manifest; both orthography packages retain Copernicus attribution. On the dev host, from the repo root:

```bash
NEW=generated/unit-14a-stage-20261122-r2
tools/unit14a-deliver.sh prepare generated/unit-14a-stage-v1-20261123 \
  "$NEW/app/qtmaps" "$NEW/runtime/maplibre" "$NEW/runtime/valhalla" \
  "$NEW/generated/unit-04/valhalla/valhalla.json" \
  data/packages/hanoi-20261114-v3-r3 \
  data/raster-packages/hanoi-raster-20261114-v3-r3 \
  data/search-packages/hanoi-search-20261115-v1-r1 \
  data/orthography-packages/hanoi-orthography-20261115-s2-l2a-v1 \
  "$NEW/data/graph/tiles" hanoi-unit14a-v1
```

Transfer both stages and verify their SHA-256 on the VM. After installing the newer stage, run `./unit14a-deliver.sh verify-rollback /path/to/older-stage /home/USER/QtMaps-14A` while offline. This activates old then new in an **isolated proof root**, verifies both distinct raster and orthography package IDs and current/previous pointers, rolls back to old, and resolves startup from old. Inspect `metadata/rollback-two-version-startup.json`; production activation remains on new. A second independently approved graph version is not available, so graph rollback remains a same-content pointer check and must be reported as such.

## Manual functional smoke (record pass/fail and screenshot)

1. Confirm offline evidence and Settings readiness. Open the map; verify local Hanoi vector and attribution. Switch to raster fallback, then Sentinel-2 orthography (z12–z15); record visible content, source and attribution. If llvmpipe cannot render, mark GUI proof blocked, not passed.
2. Search for two known Hanoi places; confirm result source says local OSM and no fixture fallback. Select A/B and request a route. Record Valhalla local readiness, graph ID/path, route geometry, nonzero distance and maneuvers. If it falls back to fixture, mark real route failed. Start simulated replay and confirm camera/maneuver progression and re-route behavior.
3. Exit and run the launcher again; ensure Settings, map, search and route still work offline. Inspect `metadata/startup.json` and `metadata/failure.log`. The installer tests same-content rollback mechanics; `verify-rollback` above tests a **different** approved real bundle version. Both use the same Hanoi graph, so graph rollback is only a same-content pointer check.
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

The host `ldd` inventory for app, Valhalla and MapLibre (with its library directory on `LD_LIBRARY_PATH`) resolves common C/C++, GL/X11/XCB, ICU/Harfbuzz/Freetype, Qt Core/Gui/Qml/Quick/Location/Positioning/Network/SQL and Valhalla bundled libraries. The candidate top-level Ubuntu packages are `libqt5location5`, `libqt5location5-plugins`, `libqt5positioning5`, `libqt5sql5-sqlite`, `qml-module-qtlocation`, `qml-module-qtpositioning`, `qml-module-qtquick2`, `qml-module-qtquick-window2`, `libgl1-mesa-dri`, `libgl1`, `fonts-noto-core`, `python3`, `iproute2` and `coreutils`, plus apt-resolved dependencies. On the clean Ubuntu 26.04.1 Desktop VM this set succeeded with 33 newly installed packages and no further missing library/plugin/font package; see the [exact list and ldd inventory](unit-14a-7480-vm-results.md). It is an observed sufficient set, not a universal minimum. Qt 5.15.18 was verified in the guest; no Qt runtime is bundled in this stage.
