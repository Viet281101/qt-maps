# Unit 04B: Local Valhalla backend evidence

Recorded on 2026-10-24 on the Dell Latitude 5480 reference machine (Intel
i5-6300U, 16 GiB RAM, Ubuntu 24.04). All source data, virtualenv files, logs,
and graph tiles are ignored under `generated/unit-04/`; no routing request or
build step used a non-loopback service.

## Provisioned tools

- `osmium-tool` 1.16.0 and `libboost-program-options1.83.0` were installed by
  the owner after the reviewed apt dry-run: 2 new packages, 0 upgraded, 0
  removed, 3,989 KiB installed. The trailing masked `packagekit.service`
  notification did not change the successful dpkg state.
- `pyvalhalla==3.9.1` was installed only in
  `generated/unit-04/valhalla-venv` with Python 3.12 and
  `pip install --require-hashes -r tools/routing/requirements-valhalla.txt`.
  The accepted wheel is 37,532,140 bytes with SHA-256
  `7b49c5e5f86cefd555b473aaaf34df3305889d30aa889b8abd3b4c2acc622271`;
  the resulting virtualenv occupies 272 MiB.
- The wheel contains native tools at
  `generated/unit-04/valhalla-venv/lib/python3.12/site-packages/valhalla/bin/`:
  `valhalla_build_tiles` (6,354,632 bytes) and `valhalla_service` (9,820,408
  bytes). `valhalla_build_config` is at `.../valhalla-venv/bin/`.
- The wheel's `valhalla_build_tiles` and `valhalla_service` console wrappers
  do not locate their native executable unless the internal directory is on
  `PATH`. QtMaps therefore records and launches the native executable path
  directly and does not change the user's global `PATH`.

`ldd` resolved every `valhalla_service` dependency on this host. The service
is a native direct child, not a Python wrapper process, so later process-tree
measurement includes QtMaps plus this native child. The ignored Python
virtualenv remains a runtime dependency because it supplies that executable
and its libraries; producing a redistributable bundle is installer work and
remains out of scope.

## Hanoi extract

Input: approved Geofabrik Vietnam PBF, replication timestamp
`2026-10-22T20:21:33Z`, 331,337,283 bytes, SHA-256
`4e5243c3116dc089a298f4cd7218e1257adf4b48355e42936360df5ce3200d43`.
The bbox `105.20,20.45,106.20,21.45` covers greater Hanoi and a buffer around
the QL1A/2/3/5/6/18 approaches. `smart` retained complete ways and completed
multipolygon plus turn-restriction relations:

```bash
/usr/bin/time -v -o generated/unit-04/logs/osmium-extract.time \
  osmium extract --bbox 105.20,20.45,106.20,21.45 \
  --strategy smart --option types=multipolygon,restriction --set-bounds \
  --output generated/unit-04/data/hanoi-buffer.osm.pbf \
  generated/unit-01b/source/vietnam-latest.osm.pbf
```

Result: 41,468,052 bytes, SHA-256
`04d37da6a99d95364a5aecbbd643fa56467c7659bfee84bc0a6f7b9865e9ae73`,
20.57 seconds wall time, and 3,906,660 KiB peak RSS. The larger actual object
envelope (`104.0279057,20.2439886,106.7530801,21.7733285`) is expected from
completed ways/relations outside the header bbox.

## Graph build and service

`valhalla_build_config` generated an ignored config with the graph directory,
empty elevation/admin/timezone inputs, `use_admin_db=false`, two workers, and
`httpd.service.listen=tcp://127.0.0.1:18002`. The measured build was:

```bash
/usr/bin/time -v -o generated/unit-04/logs/valhalla-build.time \
  generated/unit-04/valhalla-venv/lib/python3.12/site-packages/valhalla/bin/valhalla_build_tiles \
  --config generated/unit-04/valhalla/valhalla.json --concurrency 2 \
  generated/unit-04/data/hanoi-buffer.osm.pbf
```

Result: 64.83 seconds wall time, 699,660 KiB peak RSS, and 35 graph files
occupying 175,042,264 bytes. `valhalla_service .../valhalla.json 2` listened
only on `127.0.0.1:18002`; `/status` reported Valhalla 3.9.1. A warm service
snapshot after the route sample used 352,212 KiB RSS.

The Qt runtime implementation owns this executable with `QProcess`, probes
`/status` asynchronously with a bounded startup timeout, requests a graceful
SIGINT shutdown through an application-level coordinator, then uses a forced
kill after a two-second bound before allowing the application to exit. On
Linux the child installs `PR_SET_PDEATHSIG=SIGTERM` before exec. The service
exposes explicit stopped/starting/ready/stopping/error states. The route client accepts only literal HTTP loopback
endpoints, uses asynchronous `QNetworkAccessManager`, has cancellation and a
bounded request timeout, and requests `costing=auto`, kilometre units, and
`vi-VN`. Focused Qt Tests cover lifecycle, readiness, clean stop, unsafe
configuration, application-level graceful and forced shutdown, parent-death
cleanup, request contract, loopback enforcement, success, and timeout.

### Shutdown verification

The application exit path now disables implicit last-window quit, asks the
coordinator to stop routing, and calls `QCoreApplication::quit()` only after
the child reaches `Stopped`. A fake child that ignores `SIGINT` verifies the
two-second hard-kill fallback without blocking the GUI thread.

The real Release app was also started with the bounded Hanoi package and
native Valhalla service, then the QtMaps PID was sent `SIGKILL`. Before the
kill, Valhalla was its direct child and owned `127.0.0.1:18002`. Linux
`PR_SET_PDEATHSIG=SIGTERM` removed the child and released the port in 31 ms.
The inspected PIDs were 14740 for QtMaps and 14767 for `valhalla_service`;
these are transient verification identifiers, not runtime configuration.

## Real routes and latency

Twenty alternating warm requests (10 per route) produced a 56.138 ms overall
median. Per-route medians were 61.210 ms for Hoàn Kiếm–Thái Hà and 54.037 ms
for Hoàn Kiếm–Ba Đình; the observed range was 41.378–93.495 ms. This is local
service latency, not the deferred navigation/process-tree benchmark.

| Route | Straight line | Valhalla route | Ratio | Time | Maneuvers |
| --- | ---: | ---: | ---: | ---: | ---: |
| Hoàn Kiếm–Thái Hà | 4.044 km | 7.294 km | 1.80x | 523.659 s | 14 |
| Hoàn Kiếm–Ba Đình | 2.295 km | 4.262 km | 1.86x | 392.156 s | 11 |

Both ratios are plausible for the Hanoi street network. Valhalla 3.9.1
returned native Vietnamese instructions such as `Rẽ phải vào Phố Đinh Tiên
Hoàng`; no application-side translation is needed. The full responses were
recorded on 2026-10-24 in the two compact test fixtures with backend/version/
dataset metadata. Visual alignment against the local vector map is completed
in Unit 04C rather than claimed from numeric checks alone.

## Licensing and packaging boundary

Valhalla and its `pyvalhalla` distribution are MIT-licensed. `osmium-tool` is
GPL-3.0 and is used only to prepare the ignored PBF extract; it is not linked,
bundled, or distributed with QtMaps. The OSM-derived input/graph remains
subject to ODbL attribution and data obligations. Exact notices and runtime
packaging consequences are also recorded in `THIRD_PARTY_NOTICES.md`.
