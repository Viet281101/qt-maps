# V1-C — separate Valhalla runtime build, 2026-11-28

**Result:** a local, unshipped Valhalla 3.9.1 `valhalla_service` build without
data tools routed on the existing Vietnam national graph and matched the pinned
wheel for the same request. It also served a real route to the QtMaps GUI in a
networkless Ubuntu 26.04 software-rendered container. Its resolved library
closure did not contain `libspatialite`, `librttopo` or GEOS. This narrows the
runtime notice problem; it does not approve a binary upload or replace the
release candidate.

The exploratory build used Ubuntu 26.04/glibc 2.43 in Docker, with networking
disabled for the checks. Source revisions were Valhalla 3.9.1
`bafb69902220615a48307d5c790fb6c943802ba4` and prime_server 0.7.2
`77e61628d78e97ce59eea6e1eeb254d1e68edcb6`. CMake/Ninja built
prime_server, then the `valhalla_service` target with
`ENABLE_DATA_TOOLS=OFF`, `ENABLE_TOOLS=ON`, `ENABLE_SERVICES=ON`,
`ENABLE_HTTP=ON`, `ENABLE_PYTHON_BINDINGS=OFF` and `ENABLE_GEOTIFF=OFF`.

`readelf -d` listed nine direct `DT_NEEDED` entries, including
`libprime_server.so.0`, `libprotobuf-lite.so.32`, `libcurl.so.4`, `libz.so.1`
and `liblz4.so.1`. `ldd` resolved every dependency in the Ubuntu 26.04 build
image; its 49-line closure had no `libspatialite`, `librttopo` or GEOS entry.
This is an observation about these binaries in this image, not a complete
license review. The locally exported binaries have SHA-256:

| File | SHA-256 |
| --- | --- |
| `valhalla_service` | `74328cb7de04f9b1f04375391d10af867092b1762d92fdc1376c868c102e7705` |
| `libprime_server.so.0.7.2` | `9e3ae67e78d6f24e5fa9003cf77fc2311a013258053420c0be8db584758ba3f1` |

With `/graph/tiles` mounted read-only from the approved 734-tile national
graph, `valhalla_service` returned a route for 21.025,105.845 to
21.025,105.855. Both this build and the pinned pyvalhalla wheel returned
1.315 km, 133.593 s and six maneuvers. The run used `--network none` and
`--cap-drop ALL`. Optional traffic/tile-extract lookup warned because no
extract was configured; the graph route succeeded.

## Offline QtMaps GUI substitution smoke

A local test copied only the new `valhalla_service` and `libprime_server` into
an installation extracted from the exact internal real-data archive; the
archive itself was not changed. Ubuntu 26.04 packages supplied the remaining
runtime libraries. The original archive hash, stage hashes, installer, FTS5
and two real routes passed before substitution. With `--network none`,
`--cap-drop ALL`, 6 GiB limit and Xvfb/llvmpipe, `ldd` on the substituted
service resolved every library and listed neither `librttopo` nor
`libspatialite`. Its CLI route returned 1.315 km/133.593 s/six maneuvers.

QtMaps then selected the installed national graph, reported map readiness,
and applied a 7,294 m/523.659 s/14-maneuver Valhalla route. Visual review of
the 1440×900 screenshot showed the vector map, route line, guidance panel and
OSM attribution. The ignored local evidence is under
`generated/v1-c-valhalla-runtime-prototype/gui-run/`; the screenshot SHA-256 is
`c4816e00ba88d051587d624d4e494fc963422b6d26c4cfe5e8f2b9574514e775`.
The GUI test did not repeat navigation replay, re-route or arrival with the
substituted service. The candidate still contains the original wheel files,
so this smoke does not establish the dependency closure of a replacement
distribution archive.

The build recipe, logs, test configuration, response and exported binaries
remain only under the ignored local directory
`generated/v1-c-valhalla-runtime-prototype/` in the main workspace. No
replacement binary or graph was committed or uploaded.

Before substituting this runtime in V1-C, package the exact service and
prime_server binary with their source notices, enumerate all other shipped
shared objects and applicable notices, and remove the unused wheel from the
public artifact. Then repeat offline archive smoke and QtMaps navigation
checks on that replacement archive. The graph-building wheel can stay
internal. The final artifact must still be tied to its reviewed source
commit. V1-D remains gated by the owner's i915 benchmark freeze.
