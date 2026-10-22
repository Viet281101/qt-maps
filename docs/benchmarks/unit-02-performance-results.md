# Unit 02 Performance Instrumentation Result

## Scope and identity

| Field | Value |
| --- | --- |
| Date and timezone | 2026-10-22, Asia/Bangkok (UTC+07:00) |
| Build | Release; base commit `afe5c38e631c+working-tree` |
| Build/test | `make build`; `ctest --test-dir build --output-on-failure` |
| Qt | 5.13.2 `gcc_64` from `/home/viet/Qt/5.13.2/gcc_64` |
| Window | 960 x 640 |
| Dataset | `unit01-diagnostic-v1`; locally generated non-geographic XYZ grid, z10-z14 |
| Network | Localhost only; HTTP fixture requests returned 200 |
| Sampling | 1,000 ms; two-second warm-up excluded |
| Process scope | QtMaps and discoverable descendants; child count was zero. The manually launched fixture server is outside the application-owned process tree. |
| CPU units | Linux process percentage; 100% is one fully utilized logical CPU |

These are short diagnostic-tool validation runs, not management-facing product benchmarks. The scripted pan/zoom changes the map twice per second, so the low FPS figures describe this render-on-demand fixture workload and must not be interpreted as the display's maximum frame rate.

## Instrumentation overhead

The same external `/proc` sampler measured the app with instrumentation disabled and enabled for eight seconds after a two-second startup delay.

| Scenario | Instrumentation | Avg CPU % | Peak CPU % | Avg RSS MiB | Peak RSS MiB | Overhead versus off |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| Blank window | Off | 0.000 | 0.000 | 135.891 | 135.891 | Baseline |
| Blank window | On | 2.375 | 3.000 | 138.854 | 138.938 | +2.375 CPU points; +2.963 MiB average RSS |
| Diagnostic raster pan/zoom | Off | 0.874 | 3.991 | 153.041 | 153.047 | Baseline |
| Diagnostic raster pan/zoom | On | 3.249 | 6.998 | 155.188 | 155.305 | +2.375 CPU points; +2.147 MiB average RSS |

The overlay refresh itself causes approximately one or two blank-window swaps per second; that cost is intentionally included in the enabled result.

## Default versus basic render loop

Three fresh-cache Release runs per render loop used the same five-second recorded pan/zoom sequence after a two-second warm-up. Values below are medians of the three run summaries.

| Render loop | Avg completed-swap FPS | 1% low interval FPS | Avg CPU % | Avg RSS MiB | Peak RSS MiB |
| --- | ---: | ---: | ---: | ---: | ---: |
| Default | 3.618 | 3.000 | 3.167 | 155.076 | 155.180 |
| Basic (`QSG_RENDER_LOOP=basic`) | 3.741 | 3.000 | 3.399 | 151.414 | 151.445 |

The difference is too small and the diagnostic workload is too short to justify changing the default render loop. Keep the Qt default for now and repeat this comparison during the dense local-vector spike.

## Verification conclusion

- Release build passed and the Qt Test suite passed.
- `qtmaps-perf-v1` CSV files contained timezone-aware UTC/local timestamps, scenario/build/runtime metadata, completed-swap FPS, process-tree RSS/CPU, and child counts.
- Controlled test coverage proved a live child PID enters and leaves the sampled tree.
- CSV open errors and malformed/missing process data remain non-fatal to the application.
- The diagnostic raster runs generated localhost HTTP 200 responses at zooms 12 and 13 without a public tile source.
