# Unit 01b Vector Feasibility Evidence

## Final status and recommendation

Unit 01b is complete locally. Both the installed Qt 5.13.2 `mapboxgl` plugin and MapLibre Native Qt v3.0.0 on Qt 5.15.13 rendered the same real bounded-Hanoi data, local style, project-owned glyphs and sprite through `127.0.0.1`. Both retained the requested bearing/tilt values and displayed `HÀ NỘI` with diacritics and visible OpenStreetMap attribution.

Recommend **MapLibre Native Qt v3.0.0 on Qt 5.15.13 for the first demo vector renderer**. Its median average FPS exceeded the provisional 30 FPS target and was 38.6% above the Qt 5.13 plugin, while its additional CPU and RSS remained modest on the reference laptop. Keep project-generated localhost raster tiles as the fallback. A project-wide Qt migration still requires owner approval before Unit 03; this spike does not silently change the current Qt 5.13 baseline.

No PBF, jar, generated tile, third-party source/build/install tree, `.deb`, log, screenshot, or CSV is tracked by Git. All such artifacts are under ignored `generated/` or `/tmp`.

## Inputs and integrity

| Artifact | Exact size / identity | Verified integrity |
| --- | --- | --- |
| Geofabrik Vietnam PBF, resolved dated file `vietnam-261022.osm.pbf` | 331,337,283 bytes | Geofabrik MD5 `5f7388bb0dc809212fddf9cea071e513`: OK |
| Planetiler v0.10.2 `planetiler.jar` | 93,278,824 bytes | Official SHA-256 `f310bd0413e2e4512b27f4046d418664e8e1d3bf31603c2a70e23de06c167e4d`: OK |
| MapLibre Native Qt v3.0.0 source | Signed release tag commit `d929c783737120787b43417d9ef05da88da75dfd`; 48 recursive submodules | Git checkout and submodule commits recorded locally |
| Qt Location 5.15.13 source | Official Qt archive | Official `md5sums.txt` value `1aa90c62a969c1911e53a64f52284bec`: OK |
| Qt Online Installer 4.11.0, retained but not executed | 81,242,039 bytes | Official SHA-256 `40b76bdf74f6a396341efb70ae2e754fcd878474babb6cd9d7f07eff12a85c62`: OK |

## Real Hanoi generation

The source-controlled `tools/data-pipeline/unit01b-hanoi.yml` reads only the verified local PBF and emits buildings, highways, and city/town places. Planetiler's built-in OpenMapTiles profile was rejected because it would have downloaded extra unapproved Natural Earth/water inputs.

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

The run completed in 62.94 seconds with 239% CPU, 2,635,164 KiB peak RSS, zero swaps, and exit status 0. It emitted 226 PBF tiles, 14,463,231 tile bytes, and 420,058 features. The 177 snap-fix inputs and two duplicate multipolygon members were non-fatal data warnings. The schema names no remote source and `/usr/bin/time` recorded zero socket messages.

## Apt dry-run and Qt selection

The approved development-package simulation used `apt install --dry-run --no-install-recommends` with:

```text
qtbase5-dev qtbase5-private-dev
qtdeclarative5-dev qtdeclarative5-private-dev
qtlocation5-dev qtpositioning5-dev
```

Result: **0 upgraded, 32 newly installed, 0 removed, 1 not upgraded**; 11.1 MB download and 66.6 MB installed. The 26 dependencies were Qt runtime/dev tools plus the required `libegl-dev`, `libglu1-mesa-dev`, and `libxext-dev`. No metapackage or recommended package was installed.

The six requested development packages are installed at Qt 5.15.13. Four minimal QML runtime packages and the SQLite driver separately simulated as 5 new packages, 0 upgrades, and 0 removals. Their PolicyKit prompt was not confirmed, so their repository `.deb` payloads were extracted under ignored `generated/unit-01b/qml-runtime` for this isolated probe; they were not installed system-wide.

Both Qt installations remain usable without changing global `PATH`:

```bash
# Existing Qt 5.13.2 project baseline
/home/viet/Qt/5.13.2/gcc_64/bin/qmake -v
cmake -S . -B build -DCMAKE_PREFIX_PATH=/home/viet/Qt/5.13.2/gcc_64

# System Qt 5.15.13 comparison kit
/usr/lib/qt5/bin/qmake -v
cmake -S . -B build-qt515 -DCMAKE_PREFIX_PATH=/usr \
  -DQt5_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt5
```

## GPU, render loop, and vsync

`glxinfo -B` reported direct rendering, accelerated Intel Mesa, device `Mesa Intel(R) HD Graphics 520 (SKL GT2)`, Mesa 25.2.8, and OpenGL 4.6. It did not report llvmpipe. `QSG_INFO=1` confirmed the threaded Qt Quick render loop, Intel/Mesa GL renderer, and vsync animation timing of 16.66 ms (nominal 60 Hz) for both comparison runs. The existing Qt 5.13 figures therefore do not require a software-rendering rerun.

## MapLibre build

MapLibre Native Qt v3.0.0 was configured as a Release Qt 5 desktop build with the OpenGL backend and widgets disabled, then built exactly with `make -j2`. Ubuntu does not package `qtlocation5-private-dev`; the build used 163 private Qt Location/Positioning headers from the verified official Qt 5.15.13 source in an isolated include overlay, without copying files into `/usr`.

```text
Compiler: GCC 13.3.0, C++17
Qt: 5.15.13 system development libraries
Build: 16:01.44 wall, 187% CPU, 1,149,676 KiB peak RSS, zero swaps
Size: 1.1 GiB source/submodules, 92 MiB build tree, 12 MiB install prefix
Runtime: Qt Core/Gui/Network/Sql/Location/Positioning/Quick, ICU 74, OpenGL,
         MapLibre core/location/geoservices/QML plugins, SQLite driver
```

`ldd` reported no missing dependency. The installed MapLibre prefix contains a 10,332,416-byte core library, 232,072-byte location library, geoservices plugin, and QML plugin.

## Correctness and localhost isolation

Both Release probes reported `mapReady=true`, error 0. HTTP logs contain only successful 200/304 requests for the local style, sprite, glyph ranges, and z12/z13 tiles. Post-fix `strace -f -e connect` showed every AF_INET connection targeting only `127.0.0.1:8766`; local AF_UNIX/DBus and netlink activity is not external network access.

Both rendered real Hanoi buildings and roads, the project marker, `HÀ NỘI`, and `© OpenStreetMap contributors, ODbL 1.0`. For MapLibre, requested/observed camera values matched at 45°/30° and 315°/45°, then reset to zero; its tilt range was 0–60°. Screenshots remain outside Git at `/tmp/qtmaps-unit01b-real.png` and `/tmp/qtmaps-unit01b-maplibre.png`.

## Three-run real-data benchmarks

Machine: Dell Latitude 5480, Intel Core i5-6300U (2 cores/4 logical CPUs), 16 GiB RAM, Intel HD 520/Mesa 25.2.8. Each Release run used a 960×640 window, default threaded Qt Quick render loop, 60 Hz vsync, three-second warm-up, 15-second recording, one-second samples, deterministic pan/zoom, and the same z12–z15 real Hanoi dataset over localhost. CPU uses 100% for one fully occupied logical core.

The Unit 02 logger measured the app process tree. Each contained one process and no child because the same Python static server ran separately; CPU/RSS below are app-only for both renderers. Other desktop apps remained open, so these are reproducible development-host comparisons rather than clean-room maximums.

| Renderer/run | Average FPS | 1% low FPS | Mean CPU | Mean RSS | Max RSS |
| --- | ---: | ---: | ---: | ---: | ---: |
| Qt 5.13 `mapboxgl` 1 | 21.715 | 5.000 | 6.198% | 177.020 MiB | 177.824 MiB |
| Qt 5.13 `mapboxgl` 2 | 24.362 | 4.000 | 6.600% | 177.087 MiB | 177.879 MiB |
| Qt 5.13 `mapboxgl` 3 | 26.228 | 6.000 | 7.276% | 178.022 MiB | 178.488 MiB |
| **Qt 5.13 median** | **24.362** | **5.000** | **6.600%** | **177.087 MiB** | **177.879 MiB** |
| MapLibre/Qt 5.15 1 | 33.769 | 4.000 | 10.440% | 207.737 MiB | 209.652 MiB |
| MapLibre/Qt 5.15 2 | 33.225 | 5.000 | 10.058% | 209.778 MiB | 210.781 MiB |
| MapLibre/Qt 5.15 3 | 37.706 | 5.000 | 11.222% | 210.255 MiB | 211.758 MiB |
| **MapLibre/Qt 5.15 median** | **33.769** | **5.000** | **10.440%** | **209.778 MiB** | **210.781 MiB** |

MapLibre raised median average FPS by 9.407 FPS (38.6%), at a cost of 3.840 CPU percentage points and 32.691 MiB mean RSS. Both remain far below the provisional 150% interactive CPU and 2 GiB memory limits.

### Meaning of the 5 FPS 1% low

The Unit 02 metric is an interval-based percentile, not a frame-time percentile. With only 14–15 one-second samples, `ceil(samples × 0.01)` is one, so the reported 1% low is the single smallest one-second completed-swap rate.

The Qt 5.13 runs repeatedly fell to 4–6 swaps/s during three no-action phases in the 12-step automation cycle, with only 2–4% CPU, then returned to 29–62 swaps/s on pan/zoom phases. MapLibre showed the same low sample and low CPU during the corresponding render-on-demand phase while improving active-phase throughput. This evidence attributes the low primarily to on-demand frame pacing plus the coarse interval metric, not sustained tile loading or GPU saturation. A continuously animated navigation scenario should be added in Unit 03 before treating this value as perceived stutter.

## Licenses, risks, and fallback

- Geofabrik data is OpenStreetMap-derived and governed by ODbL 1.0; visible `© OpenStreetMap contributors` attribution is present.
- Planetiler v0.10.2 is Apache-2.0 licensed.
- MapLibre Native core is BSD-2-Clause; Qt QML bindings are GPL-2.0/GPL-3.0/LGPL-3.0 options; examples are MIT. Distribution must follow the selected binding license and linking/package model.
- MapLibre v3 depends on Qt Location private API; Ubuntu requires the documented private-header overlay. Qt 5.15 and MapLibre v3 are maintenance branches while current MapLibre targets Qt 6.5+, creating future upgrade work.
- The 15-second comparison does not replace a 30–60 minute soak, navigation animation, rapid layer switching, or cold-cache benchmark.
- Project-generated raster tiles served only on localhost remain the recovery path if vector startup, driver, packaging, or licensing issues block the demo.

## Decision requested before Unit 03

Approve Qt 5.15.13 plus MapLibre Native Qt v3.0.0 as the project/demo vector baseline, approve the LGPL-3.0-compatible dynamic-linking/package approach for review, and retain Qt 5.13 only as the known-working rollback until the Qt 5.15 build and deployment path is integrated. Also approve adding a continuously animated navigation benchmark to Unit 03 while keeping the existing interval metric for historical comparison.

## Sources checked

- <https://download.geofabrik.de/asia/vietnam.html>
- <https://github.com/onthegomap/planetiler/releases/tag/v0.10.2>
- <https://github.com/onthegomap/planetiler/blob/v0.10.2/LICENSE>
- <https://github.com/maplibre/maplibre-native-qt/releases/tag/v3.0.0>
- <https://maplibre.org/maplibre-native-qt/docs/md_docs_2Building.html>
- <https://github.com/maplibre/maplibre-native-qt>
- <https://download.qt.io/archive/qt/5.15/5.15.13/submodules/>
