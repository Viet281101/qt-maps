# Unit 14 — glibc ABI and supported Linux target (analysis, 2026-11-24)

**Decision held:** Ubuntu **26.04 x86-64 remains the only committed clean-target Linux platform**. This analysis does not add 22.04 or 24.04 support. No GPU or application benchmark was run.

## Measured ABI floor of the 14A stage

The exact approved 14A stage is `generated/unit-14a-stage-20261122-r2`. Every file with ELF magic was inspected using `readelf --version-info`; 66 staged ELFs had GLIBC requirements. Symlink/copy aliases of the same MapLibre binary do not change the maximum.

| Staged component | Highest required symbol | Evidence |
| --- | --- | --- |
| QtMaps executable | `GLIBC_2.38` | `readelf --version-info app/qtmaps` |
| MapLibre Native Qt v3.0.0 runtime | **`GLIBC_2.43`** | `libQMapLibre.so.3.0.0` requires `atan2f@GLIBC_2.43` and `remainder@GLIBC_2.43`; also `GLIBCXX_3.4.32` and `CXXABI_1.3.15` |
| pyvalhalla/Valhalla runtime | `GLIBC_2.28` | Maximum among 57 staged Valhalla/wheel ELFs |
| Whole staged bundle | **glibc 2.43** | Maximum of all staged ELFs; system Qt/ICU dependency versions are additional constraints |

Reproduce from the repository root with `readelf --version-info generated/unit-14a-stage-20261122-r2/runtime/maplibre/lib/libQMapLibre.so.3.0.0` and `objdump -T ... | grep GLIBC_2.43`. The development host reports `ldd (Ubuntu GLIBC 2.43-2ubuntu2.7) 2.43`. The laptop 7480 host reports glibc **2.39** on Ubuntu 24.04.5; running `ldd` on the staged MapLibre library there reports `/lib/x86_64-linux-gnu/libm.so.6: version 'GLIBC_2.43' not found`, plus missing Qt 5 and `libicuuc.so.78` from that host. This directly proves the current 26.04 stage is not a direct 24.04 bundle. The successful 14A VM had its own Ubuntu 26.04 userland.

Ubuntu package records identify glibc [2.35 on 22.04](https://packages.ubuntu.com/jammy/libc6), [2.39 on 24.04](https://packages.ubuntu.com/noble/libc6), and [2.43 on 26.04](https://packages.ubuntu.com/resolute/libc6). The same records list distro Qt development packages as [5.15.3 on 22.04](https://packages.ubuntu.com/jammy/amd64/qtbase5-dev), [5.15.13 on 24.04](https://packages.ubuntu.com/noble/amd64/qtbase5-dev) and [5.15.18 on 26.04](https://packages.ubuntu.com/resolute/amd64/qtbase5-dev). Thus copying only the current app/MapLibre files onto either older distro cannot preserve the committed Qt 5.15.18 baseline or its full shared-library closure.

## Cost of an older build baseline (estimate, not a commitment)

| Proposed baseline | Engineering estimate | Required work and uncertainty |
| --- | ---: | --- |
| Ubuntu 24.04, retaining Qt 5.15.18 | **3–6 engineer-days** for a first validated bundle, then CI maintenance | Build/backport a complete matching Qt 5.15.18 kit and QML/Qt Location/SQLite plugins; rebuild MapLibre v3.0.0 and QtMaps on 24.04; select compatible ICU/libstdc++; package licenses and dependencies; repeat 14A fresh-target tests on 24.04 **and** 26.04. Actual elapsed time depends on source/toolchain access and failures in third-party dependencies. |
| Ubuntu 22.04, retaining Qt 5.15.18 | **5–10 engineer-days** for a first validated bundle, then CI maintenance | Same work with an older glibc, Qt 5.15.3 distro kit and older toolchain; expect additional compiler/ICU/build dependency adjustments. Requires its own clean VM and complete offline smoke. |
| Keep Ubuntu 26.04 only | **No backport work** for Unit 14 | Pin 26.04 in build/CI and target instructions. Revisit older distros only after owner approval and separate proof. |

The Valhalla wheel's staged glibc floor (2.28) is below both proposed older baselines, but that alone does **not** prove its service, Python dependencies, graph or routing behavior on them. A build with the system Qt 5.15.13/5.15.3 would be a **baseline change** requiring an explicit owner decision; it is not counted as a validated shortcut. Estimates exclude procurement/license review and any newly discovered C++ ABI or packaging issue.

## CI consequence

Do **not** run the Ubuntu 26.04-built bundle as a native executable on an `ubuntu-22.04` or `ubuntu-24.04` runner and infer target support. Its MapLibre library requires glibc 2.43. GitHub's [runner-images project](https://github.com/actions/runner-images/blob/main/README.md) lists an explicit `ubuntu-26.04` image, and its [November 2026 image release](https://github.com/actions/runner-images/releases) reports Ubuntu 26.04.1. Unit 14C should pin `runs-on: ubuntu-26.04`, verify `getconf GNU_LIBC_VERSION` and Qt 5.15.18 at job start, and run the 14A harness only in a matching 26.04 userland. Avoid the moving `ubuntu-latest` alias for release provenance. An older runner may host an isolated 26.04 container, but that would prove the container userland, **not** native 22.04/24.04 support.

**Owner decision still open:** whether a separately scoped 24.04 (or 22.04) build is worth the above cost after the Linux 26.04 CI path is stable. Until then, do not expand the published distro support claim.
