# Unit 01b Vector Feasibility Evidence

## Status and decision gate

Unit 01b is complete for the installed Qt 5.13.2 `mapboxgl` renderer and paused at the owner-controlled Qt 5.15/MapLibre installation gate. The real bounded-Hanoi dataset, local resource proof, camera checks, and three-run benchmark are complete. The required MapLibre Native Qt comparison cannot start because this machine has no supported Qt kit: Qt 5.13.2 is too old for MapLibre QtLocation, and the installed Qt 6.4.2 is below the current 6.5 floor.

No PBF, jar, generated tile, installer, log, screenshot, or CSV is tracked by Git. They are all under ignored `generated/` or `/tmp`. The project baseline remains Qt 5.13.2 until a separate migration decision is approved.

## Inputs and integrity

The owner approved the Geofabrik and Planetiler downloads on 2026-10-23. Both were saved below ignored `generated/unit-01b/`, verified before use, and remained absent from `git status`.

| Artifact | Exact size | Verified digest | Verification |
| --- | ---: | --- | --- |
| Geofabrik Vietnam PBF, resolved dated file `vietnam-261022.osm.pbf` | 331,337,283 bytes | MD5 `5f7388bb0dc809212fddf9cea071e513` | Geofabrik `.md5` sidecar: OK |
| Planetiler v0.10.2 `planetiler.jar` | 93,278,824 bytes | SHA-256 `f310bd0413e2e4512b27f4046d418664e8e1d3bf31603c2a70e23de06c167e4d` | Official release `.sha256` sidecar: OK |
| Qt Online Installer 4.11.0, downloaded but not executed | 81,242,039 bytes | SHA-256 `40b76bdf74f6a396341efb70ae2e754fcd878474babb6cd9d7f07eff12a85c62` | Official Qt mirror metadata: OK |

The Qt installer remains non-executable and has not been launched. No package was installed.

## Real Hanoi generation

The reproducible source-controlled schema is `tools/data-pipeline/unit01b-hanoi.yml`. It reads only the verified local PBF and emits buildings, highways, and city/town places for the feasibility probe. Planetiler's built-in OpenMapTiles profile was rejected because it would have fetched additional unapproved Natural Earth/water inputs.

```bash
/usr/bin/time -v -o generated/unit-01b/logs/planetiler-time.txt \
  java -Xmx6g -jar generated/unit-01b/tools/planetiler.jar generate-custom \
  --schema=tools/data-pipeline/unit01b-hanoi.yml \
  '--output=generated/unit-01b/site/tiles/{z}/{x}/{y}.pbf' \
  --bounds=105.78,20.97,105.90,21.10 \
  --minzoom=12 --maxzoom=15 --render_maxzoom=15 \
  --threads=2 --tile_compression=none \
  --tmpdir=generated/unit-01b/tmp --force
```

The run completed in 62.94 seconds with 146.43 user seconds, 4.35 system seconds, 239% CPU, 2,635,164 KiB peak RSS, zero swaps, and exit status 0. It emitted 226 PBF tiles, 14,463,231 tile bytes (15 MiB directory including metadata), and 420,058 features. Planetiler reported 177 snap-fix inputs and two duplicate multipolygon members as non-fatal data warnings. `/usr/bin/time` recorded zero socket messages, and the schema names no remote source.

## Runtime correctness and localhost isolation

The Release probe served `style.json`, tiles, project-owned glyphs, and sprite files from a Python static server bound to `127.0.0.1:8766`. The map reported `mapReady=true`, error 0, and used the requested local style. HTTP logs contain only successful 200/304 requests for those resources.

An initial trace found that the application's unrelated provider-construction diagnostic could trigger external DNS. Vector mode now skips that diagnostic. A fresh `strace` then showed every AF_INET connection targeting only `127.0.0.1:8766`; local AF_UNIX/DBus and netlink activity was not classified as network data access.

The rendered frame visibly showed real Hanoi buildings and roads, the project-owned marker, the Vietnamese label `HÀ NỘI` with diacritics, and `© OpenStreetMap contributors, ODbL 1.0`. Requested/observed camera values matched at 45° bearing/30° tilt and 315° bearing/45° tilt, followed by a zero reset; the plugin reported a 0–60° tilt range. The inspected screenshot remains outside Git at `/tmp/qtmaps-unit01b-real.png`.

## Three-run real-data benchmark

Machine: Dell Latitude 5480, Intel Core i5-6300U (2 cores/4 logical CPUs), 16 GiB RAM, Intel HD Graphics 520, Mesa 25.2.8. Build/runtime: Release, Qt 5.13.2, 960×640, default Qt Quick render loop, three-second warm-up, 15-second recording, one-second samples, deterministic pan/zoom, z12–z15 real bounded-Hanoi data, localhost-only resources. CPU uses the Linux process convention where 100% equals one fully occupied logical core.

The Unit 02 logger measured the app process tree. It contained one process and no child because the Python static server was launched manually outside the tree; therefore the primary CPU/RSS values below are app-only. Desktop applications already open on the reference machine were left untouched, so this is reproducible development-host evidence rather than a clean-room maximum-throughput result.

| Run | Average FPS | 1% low FPS | Mean CPU | Mean RSS | Max RSS |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 21.715 | 5.000 | 6.198% | 177.020 MiB | 177.824 MiB |
| 2 | 24.362 | 4.000 | 6.600% | 177.087 MiB | 177.879 MiB |
| 3 | 26.228 | 6.000 | 7.276% | 178.022 MiB | 178.488 MiB |
| **Median** | **24.362** | **5.000** | **6.600%** | **177.087 MiB** | **177.879 MiB** |

These figures are from real OSM-derived data, not the synthetic fixture. They demonstrate a working low-resource render-on-demand path, but miss the provisional 30 FPS planning target and have visibly weak 1% lows. They are not yet a renderer selection because MapLibre has not been measured.

## Synthetic fixture distinction

The earlier synthetic mechanics fixture remains available through `make vector-fixture`. Its geometry is generated, non-geographic, and used only to validate local MVT/style/glyph/sprite mechanics. Its one short smoke run (24.296 average FPS, 4.995 1% low, 5.829% CPU, 156.289 MiB RSS) is diagnostic and is not a project benchmark.

## Qt and MapLibre environment assessment

Installed kits and tools:

- `/home/viet/Qt/5.13.2/gcc_64/bin/qmake`: Qt 5.13.2.
- `/usr/lib/qt6/bin/qmake6`: Qt 6.4.2.
- GCC 13.3.0, CMake 3.28.3, and Ninja 1.11.1.
- No Qt 5.15 kit is installed. Current apt candidates provide Qt 5.15.13 base, declarative, location, positioning, and private development packages.

Official MapLibre Native Qt build documentation says Qt 6.5+ is fully supported. It also documents Qt 5.15 desktop support for the older line; Qt 5.6–5.14 support widgets but not QtLocation. Release v3.0.0 provides a Linux Qt 5.15.2 build, QML import `MapLibre 3.0`, and `qmaplibre_location_setup_plugins`. Current main/v4 no longer supports Qt 5.

The recommended comparison is therefore Qt 5.15.13 plus MapLibre Native Qt v3.0.0: it is the smallest migration from the current Qt 5 application and preserves the existing Qt 5 CMake/QML shape. The prebuilt v3 artifact targets Qt 5.15.2, so ABI, ICU, and `ldd` compatibility must be verified rather than assumed. Qt 6 should use at least 6.5 and would require a broader CMake/QML/application migration; the installed 6.4.2 is not a supported shortcut.

The Intel HD 520/Mesa stack exposes OpenGL 4.6 and already renders the current GL backend, so it is a plausible MapLibre target. The older integrated GPU makes shader compilation, tile upload, driver behavior, and sustained memory important risks. A Qt 6 RHI migration adds driver/backend variance without resolving those risks, so it is not the first comparison choice.

## Installation approval required

To continue with the minimum-migration comparison, the owner must approve and manually execute or explicitly authorize these package changes:

```bash
sudo apt update
sudo apt install qtbase5-dev qtbase5-private-dev \
  qtdeclarative5-dev qtdeclarative5-private-dev \
  qtlocation5-dev qtpositioning5-dev
```

Alternatively, the already downloaded Qt Online Installer can install an isolated Qt 5.15.x kit if the owner's Qt account/license exposes that archived release. It must not be executed before approval. After Qt is ready, download and verify the MapLibre v3.0.0 Qt 5.15.2 Linux release asset, create an isolated MapLibre probe without deleting this probe, and run the same three scenarios.

## Licenses and provisional conclusion

- Geofabrik data is derived from OpenStreetMap and governed by ODbL 1.0; visible `© OpenStreetMap contributors` attribution is present in the probe.
- Planetiler v0.10.2 is Apache-2.0 licensed.
- MapLibre Native core is BSD-2-Clause; its Qt QML bindings are offered under GPL-2.0, GPL-3.0, or LGPL-3.0; examples are MIT. Final distribution compliance must follow the selected binding license and linking/package model.

Vector rendering is technically viable on this laptop with the existing Qt 5.13 plugin, including local real data, Vietnamese labels, bearing, tilt, and modest memory/CPU. Its median 24.362 FPS and 5 FPS 1% low do not yet justify committing it as the production renderer. Provisional recommendation: compare MapLibre v3 on Qt 5.15, choose it only if correctness/stability and frame pacing materially improve, and retain project-generated localhost raster tiles as the lower-risk fallback. Do not change the whole project Qt version until that comparison is reviewed.

## Sources checked

- <https://download.geofabrik.de/asia/vietnam.html>
- <https://github.com/onthegomap/planetiler/releases/tag/v0.10.2>
- <https://github.com/onthegomap/planetiler/blob/v0.10.2/LICENSE>
- <https://github.com/maplibre/maplibre-native-qt/releases/tag/v3.0.0>
- <https://maplibre.org/maplibre-native-qt/docs/md_docs_2Building.html>
- <https://github.com/maplibre/maplibre-native-qt>
- <https://doc.qt.io/qt-6/get-and-install-qt-cli.html>
