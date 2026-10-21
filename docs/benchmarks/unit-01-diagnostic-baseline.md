# Unit 01 Diagnostic Raster Baseline

> **Not management-reportable.** These measurements use locally generated colored XYZ diagnostic tiles, not real geographic map data. They validate the measurement tooling and the QtLocation localhost path only.

## Environment

- Date: 2026-10-21 (Asia/Bangkok)
- Build: Release, CMake 3.28.3, Ninja 1.11.1, GCC 13.3.0, ccache 4.9.1
- Application viewport: 960x640 on X11
- Machine: Dell Latitude 5480, Intel Core i5-6300U (2 cores, 4 logical CPUs), 16 GiB RAM
- Graphics: Intel HD Graphics 520 (Skylake GT2), Mesa 25.2.8, direct rendering enabled, OpenGL 4.6
- Power: AC online; battery full at 100%
- CPU governor: `powersave` on CPUs 0-3
- Network/data: HTTP loopback only; generated non-geographic XYZ fixture at zooms 10-14
- Process scope: `qtmaps` plus the separately launched Python static HTTP server; the server is not yet an application-owned child process

Pre-measurement temperatures were 53 C from `x86_pkg_temp` and 58 C from the `Package id 0` hwmon sensor. After idle they were 53 C and 57 C. After the retained interaction run they were 54 C and 53 C. Sensor disagreement is preserved rather than normalized.

## Method

`tools/sample_process_metrics.py` sampled `/proc/<pid>/stat` and `/proc/<pid>/status` once per second. CPU percent is calculated from the delta of user plus system clock ticks divided by wall time; 100% represents one fully occupied logical CPU. RSS is the current `VmRSS` value in KiB. Generated CSV files are ignored and are not committed.

The static server command was:

```bash
python3 -m http.server 8765 --bind 127.0.0.1 --directory generated/diagnostic-tiles
```

The application command was:

```bash
./build/qtmaps
```

## Results

### Idle, 60.009 seconds, 60 samples

| Scope | RSS average (KiB) | RSS min (KiB) | RSS max (KiB) | CPU average | CPU min | CPU max |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| QtMaps | 139016.0 | 139016 | 139016 | 0.000% | 0.000% | 0.000% |
| HTTP server | 20012.0 | 20012 | 20012 | 0.017% | 0.000% | 1.000% |
| Combined | 159028.0 | 159028 | 159028 | 0.017% | 0.000% | 1.000% |

### Symmetric pan/zoom, 30.005 seconds, 30 samples

The repeated interaction sequence was: pan right/down, pan left/up at the same zoom, zoom in by one level, then zoom out by one level. The server recorded five requests, all HTTP 200, with no 404 or server exception.

| Scope | RSS average (KiB) | RSS min (KiB) | RSS max (KiB) | CPU average | CPU min | CPU max |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| QtMaps | 144504.3 | 140364 | 144664 | 0.533% | 0.000% | 2.999% |
| HTTP server | 20626.0 | 20568 | 20628 | 0.000% | 0.000% | 0.000% |
| Combined | 165130.3 | 160932 | 165292 | 0.533% | 0.000% | 2.999% |

## FPS Status and Limitations

FPS was not captured. The static Qt Quick scene renders on demand and this unit does not yet contain a rendered-frame counter. Adding reliable `frameSwapped`-based instrumentation is part of unit 02; a refresh-rate assumption or screenshot count would not be an FPS measurement.

Two earlier interactive attempts were discarded because their asymmetric pan/zoom order accumulated center movement beyond the bounded fixture and triggered 404 retries. Their CPU/RSS values are not baseline results. The retained symmetric run is the only interactive result above.

These figures must not be compared with project performance targets or presented to management. A real benchmark requires project-produced map data, application-owned service lifecycle, unit 02 frame instrumentation, and the fixed scenarios defined in the benchmark plan.
