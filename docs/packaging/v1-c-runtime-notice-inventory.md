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
Its CycloneDX SBOM lists 44 entries (38 unique component names, including
repeated entries) and zero
components with a `licenses` field. It names dependencies such as OpenSSL,
libcurl, PROJ, libtiff, libxml2, ZeroMQ and LuaJIT, but does not by itself
provide the exact license texts or notices for each bundled binary. The
current draft `LICENSES/NOTICE` says this review is open. A public runtime
archive must wait until the exact binary-to-notice mapping is reviewed and
included; a synthetic-data label alone does not close this gap.

The exact `pyvalhalla.libs` directory also contains seven files whose component
names do not appear among the SBOM's 38 unique names: `libfreexl`, `libgeos`,
`libgeos_c`, `libprime_server`, `librttopo`, `libspatialite` and `libsqlite3`.
The two GEOS files represent one component family. This comparison is based on
the filenames in the pinned wheel and the SBOM component `name` fields; it
does not establish the origin or applicable license of these binaries. Review
their exact provenance and notices alongside the RPM-derived components before
enabling any public binary upload.

### Directly linked library requiring distribution review — 2026-11-28

`readelf -d` on the **staged** `runtime/valhalla/valhalla/bin/valhalla_service`
lists `librttopo-099fd867.so.1.1.0` directly in `DT_NEEDED`, as well as
`libspatialite`, `libprime_server` and `libgeos_c`. The staged `librttopo`,
`libspatialite` and `libprime_server` hashes match the pinned local wheel.
The [librttopo upstream mirror README](https://gitlab.com/rttopo/rttopo/-/raw/master/README.md)
states GNU GPL version 2 or later. The upstream project pages identify
[GEOS as LGPL](https://libgeos.org/),
[FreeXL](https://www.gaia-gis.it/fossil/freexl/index) and
[SpatiaLite](https://www.gaia-gis.it/fossil/libspatialite/home) as offering
MPL 1.1/GPL 2+/LGPL 2.1+ alternatives, and
[SQLite](https://www.sqlite.org/copyright.html) as public domain. The
[prime_server upstream COPYING](https://raw.githubusercontent.com/kevinkreiser/prime_server/master/COPYING)
contains a permissive source/binary redistribution notice. These pages
establish upstream project claims, not the exact build flags, selected
license path or complete notices of the packaged wheel. In particular, the
SpatiaLite [GCP module note](https://www.gaia-gis.it/fossil/libspatialite/info/4b3c909885)
describes an additional GPL condition if that optional module is enabled;
the wheel build configuration has not been established.

**Public runtime binary upload remains disabled.** Before enabling it, review
the exact `librttopo` and other bundled library versions, source and notices,
the combined distribution's license obligations, and whether a differently
built Valhalla runtime can avoid unwanted dependencies. Owner permission to
bundle a runtime is not a substitute for that review. No claim of legal
compliance or incompatibility is made from the `DT_NEEDED` observation alone.

A separate Valhalla **runtime** build without data tools has now passed a
national-graph CLI route check. The [Valhalla 3.9.1 CMake source](https://raw.githubusercontent.com/valhalla/valhalla/3.9.1/CMakeLists.txt)
finds SpatiaLite under `ENABLE_DATA_TOOLS`, while `valhalla_service` is
included in the tool targets. The [local prototype report](v1-c-valhalla-runtime-prototype-20261128.md)
records the exact source revisions, flags, binary hashes, dependency closure,
matching route result and local GUI substitution smoke. The archive tested
still contains the original wheel, so it is **not** a replacement distribution
candidate. The internal graph-building wheel could remain separate. Any
shipped replacement must remove unused wheel files, pass offline extraction
and navigation smoke, and have complete binary notices reviewed.
