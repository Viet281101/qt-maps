# Linux Dynamic-Link Packaging Plan

Status: approved architecture direction; implementation and distributable release are not part of the pre-Unit-03 work. This checklist is technical compliance guidance, not legal advice. Obtain legal review before external distribution.

## Selected model

QtMaps uses dynamically linked Qt 5.15.13 and MapLibre Native Qt v3.0.0 shared libraries. The application executable, Qt/MapLibre shared objects, plugins, QML modules, map dataset packages, and notices remain separable. Do not statically link Qt or the LGPL-selected MapLibre Qt/QML bindings into a release artifact without a separate owner and legal review.

## Proposed bundle layout

```text
qtmaps/
  bin/qtmaps
  lib/                         Qt, MapLibre and required shared objects
  plugins/geoservices/         MapLibre/Qt Location runtime plugin
  plugins/platforms/           required Qt platform plugin
  plugins/sqldrivers/          required SQLite plugin
  qml/                         required Qt/MapLibre QML modules
  share/qtmaps/data/           separately versioned map packages
  share/qtmaps/licenses/       full license texts and dependency notices
  share/qtmaps/sources/        source instructions/offers and build metadata
  THIRD_PARTY_NOTICES.md
```

Use a relative, documented runtime search configuration that still permits library replacement. Do not change the user's global `PATH`, lock the bundle with signatures/DRM that prevent a modified LGPL library from running, or impose terms that remove LGPL relinking/reverse-engineering rights.

## Release checklist

1. Build only from the approved Qt 5.15.13 and signed MapLibre v3.0.0 identities; record compiler, flags, patches, submodule commits, and checksums.
2. Confirm `ldd` for the executable, Qt plugins, QML plugins, and MapLibre libraries has no missing dependency and contains no accidental Qt 5.13 or build-tree path.
3. Keep the executable dynamically linked. Verify that replacing the packaged LGPL shared libraries with ABI-compatible rebuilt versions is technically possible and document the commands/search paths needed to run that modified combination.
4. Include `THIRD_PARTY_NOTICES.md`, the complete LGPL-3.0 and GPL-3.0 texts required by the selected LGPL terms, BSD-2-Clause notices for MapLibre core, Qt module/third-party notices, and every transitive notice emitted by the exact build.
5. Provide the complete corresponding source for distributed LGPL libraries, including project-applied modifications and build scripts, or a legally reviewed written offer/instructions valid for the required period. Preserve MapLibre's exact v3.0.0 Qt binding and core source identities.
6. Ensure installer/EULA terms do not prohibit the reverse engineering needed to debug or relink LGPL components. Provide installation information sufficient to run a relinked combination.
7. Keep Planetiler outside the runtime package. If it is redistributed, add Apache-2.0, its NOTICE file, bundled dependency notices, and any bundled-JRE terms.
8. Keep map datasets separately versioned. Display `© OpenStreetMap contributors`, identify ODbL 1.0, link the copyright/license page, and ship source/checksum/date/license/share-alike metadata with each distributed derived database.
9. Generate a final component inventory/SBOM from the actual staging directory; compare it with this notice and fail packaging on an unknown binary, missing license, missing source offer, or static Qt/MapLibre linkage.
10. Run the offline demo from the staged bundle on a clean Linux user account before release, including vector startup, localhost raster fallback, library/plugin discovery, and service shutdown.

## Development selection without global PATH changes

Baseline configure/build/test:

```bash
cmake -S . -B build-qt515 -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/usr \
  -DQTMAPS_MIN_QT_VERSION=5.15.13 \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
cmake --build build-qt515 --parallel
ctest --test-dir build-qt515 --output-on-failure
```

The current MapLibre proof uses an ignored local install prefix. Select it only for the launched process with `LD_LIBRARY_PATH`, `QT_PLUGIN_PATH`, and `QML2_IMPORT_PATH`; do not export these in shell startup files. Unit 03 must replace the feasibility paths with a reproducible development/runtime dependency layout before packaging begins.

Qt 5.13 comparison remains isolated through the command in `context/code-standards.md` and is not copied into the Qt 5.15 release staging directory.
