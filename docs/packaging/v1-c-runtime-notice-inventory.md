# V1-C runtime notice inventory (review pending)

Observed against the exact local runtime assembled for the 2026-11-27
candidate. This is an inventory for release review, not a declaration that
distribution requirements have been satisfied.

| Component | Present evidence | Remaining check |
| --- | --- | --- |
| QtMaps executable | Repository MIT `LICENSE` copied into `LICENSES/QtMaps-MIT.txt`. | Confirm the final executable/source commit matches the archive manifest. |
| Qt 5.15.18, Qt Location/QML plugins and Noto | Supplied by Ubuntu 26.04 packages, not copied into the stage; `/usr/share/doc/<package>/copyright` remains on the target. | Record exact target package versions and required user-facing source/relink information for a release. |
| MapLibre Native Qt v3.0.0 | Local `LICENSES/` has BSD-2-Clause, GPL-2.0-only, GPL-3.0-only, LGPL-3.0-only and MIT texts; all are copied into the candidate. | Match the built library/plugin and transitive components to applicable notices and chosen license path. |
| Valhalla/pyvalhalla 3.9.1 | `licenses/COPYING` and `sboms/auditwheel.cdx.json` copied into the candidate. | Resolve notices for native wheel dependencies before binary publication. |
| OSM and Copernicus data | Package manifests include source/hash and ODbL/Copernicus attribution. | Keep the real data and graph on the approved internal channel, separate from public CI. |

The pinned pyvalhalla wheel contains 50 native files under `pyvalhalla.libs`.
Its CycloneDX SBOM lists 44 components (including repeated entries) and zero
components with a `licenses` field. It names dependencies such as OpenSSL,
libcurl, PROJ, libtiff, libxml2, ZeroMQ and LuaJIT, but does not by itself
provide the exact license texts or notices for each bundled binary. The
current draft `LICENSES/NOTICE` says this review is open. A public runtime
archive must wait until the exact binary-to-notice mapping is reviewed and
included; a synthetic-data label alone does not close this gap.
