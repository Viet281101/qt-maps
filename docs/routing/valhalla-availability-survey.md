# Valhalla Availability Survey

Date: 2026-10-24. Scope: read-only availability and dependency survey for Unit
04B on Ubuntu 24.04 x86_64. No Valhalla package/image was installed or pulled,
no PBF was cut, and no graph was built.

## Host and existing inputs

- Dell Latitude 5480, i5-6300U (2 cores/4 threads), 15 GiB RAM plus 4 GiB
  swap; 115 GiB free in the project filesystem.
- Docker 29.9.2 is installed and usable without sudo. No Valhalla image is
  cached. No Podman, Apptainer, Valhalla binary, or `osmium` binary is installed.
- The approved 316 MiB Vietnam PBF remains outside Git at the Unit 01b path;
  SHA-256 is `4e5243c3116dc089a298f4cd7218e1257adf4b48355e42936360df5ce3200d43`.

## Prebuilt choices

| Choice | Version/size | Dependencies and assessment |
| --- | --- | --- |
| Ubuntu apt | No Valhalla package in Noble repositories | Not available; only `osmium-tool` 1.16.0 is packaged. |
| GitHub release | 3.9.1, published 2026-10-06; no binary assets | Source release only. |
| Official `pyvalhalla` wheel | 3.9.1; Linux x86_64 wheel 37,532,140 bytes; SHA-256 `7b49c5e5f86cefd555b473aaaf34df3305889d30aa889b8abd3b4c2acc622271` | Official attested manylinux 2.28 wheel. This host has Python 3.12 and compatible glibc. No mandatory Python dependency; the wheel includes selected graph tools and service executable wrappers. Installable in an ignored virtualenv without sudo. |
| Official scripted container | `ghcr.io/valhalla/valhalla-scripted:3.9.1`; amd64 manifest `sha256:31d96eb75f73e5f5b3dc40702edfda16c4e1793a384a8235bcf00dd3eed1b9bc`; approximately 156.3 MiB compressed | Docker is ready and the image can build/serve a mounted local PBF with `server_threads=2`. The service runs under the Docker daemon rather than as a direct QtMaps descendant, which makes lifecycle and complete process-tree accounting less faithful. |
| PyPI package named `valhalla` | 0.1.4 | Unrelated validation library; rejected. |

## Source-build fallback

Valhalla 3.9.1's official Linux dependency script requests autoconf,
automake, ccache, Clang/clang-tidy, CMake, GCC/G++, curl, Git, jq, lcov,
Boost, curl/CZMQ/ZMQ/GDAL/GEOS/LuaJIT/LZ4/protobuf/spatialite/SQLite
development packages, libtool, locales, osmium-tool, GNU parallel, pkgconf,
Python development/requests/Shapely/pip packages, spatialite-bin, unzip, and
zlib. It also builds prime_server 0.12.0 from source for service support.

An apt simulation of that untrimmed official list would add 328 packages (0
upgraded, 0 removed). On the two-core reference machine, a Release source build
limited to `-j2` is estimated at 1.5-3 hours plus dependency provisioning and
several GiB of source/build storage. Main risks are the very broad dependency
surface, prime_server build, Ubuntu 24.04/t64 compatibility, and spending the
three-day routing budget on toolchain work. Source build is therefore the last
fallback, not the recommended first attempt.

## PBF-cut prerequisite

`apt-get -s install osmium-tool` reports exactly two new packages:
`osmium-tool` 1.16.0 and `libboost-program-options1.83.0`; 0 upgrades, 0
removals, and 1 held package. Their combined installed size is approximately
3.9 MiB. Real installation requires sudo and owner approval.

## Recommendation and approval gate

Use pinned `pyvalhalla==3.9.1` in an ignored
`generated/unit-04/valhalla-venv/`, verify the published wheel SHA-256, and
invoke its native tools with the virtualenv's explicit Python path. This keeps
`valhalla_service` as a direct app child for lifecycle and future process-tree
metrics, avoids a source build, and is substantially smaller than the
container. Use apt `osmium-tool` only to cut the approved PBF, then build the
Hanoi graph with at most two threads and `/usr/bin/time -v`.

Approval is required before installing the two apt packages. The wheel has not
yet been downloaded or installed; after approval it will remain outside Git and
be checksum-verified before use.

Primary references:

- <https://github.com/valhalla/valhalla/releases/tag/3.9.1>
- <https://pypi.org/project/pyvalhalla/3.9.1/>
- <https://github.com/valhalla/valhalla/blob/3.9.1/docker/README.md>
- <https://valhalla.github.io/valhalla/building/>
- <https://github.com/valhalla/valhalla/blob/3.9.1/scripts/install-linux-deps.sh>
