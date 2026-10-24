# QtMaps Performance Instrumentation

## Measurement semantics

- Schema: `qtmaps-perf-v1`.
- Frame source: each `QQuickWindow::frameSwapped` emission counts one completed swap. A one-second interval reports `completed swaps / actual interval seconds`; UI timer firings are never counted as frames.
- Zero-frame intervals: a visible, non-minimized window contributes a `0 FPS` sample. Hidden or minimized intervals display `0 FPS` but are excluded from the summary.
- Summary: average FPS is the arithmetic mean of retained one-second FPS samples. The reported 1% low is the arithmetic mean of the lowest `ceil(sample_count * 0.01)` interval-FPS samples. It is an interval-throughput statistic, not a frame-time percentile.
- Bound: at most 3,600 FPS intervals are retained. CSV process samples are streamed and not retained in memory.
- Warm-up: the default is three seconds. `/proc` baselines are updated during warm-up, but no CSV row is written. Frame summaries reset when warm-up completes.
- Sampling: the default process/CSV interval is 1,000 ms; values below 100 ms are rejected in favor of 100 ms.
- RAM: `total_rss_bytes` is the sum of `VmRSS` for the QtMaps PID and every readable descendant present at the sample.
- CPU: deltas of `/proc/<pid>/stat` user plus system ticks are divided by `SC_CLK_TCK` and elapsed monotonic seconds. `100%` means one fully utilized logical CPU. New processes need one baseline sample, while exited or unreadable processes disappear from the current scope.

## Threading and lifecycle

`FrameMetrics` receives render-completion signals on the GUI thread and performs only counter and bounded-vector work there. Linux `/proc` enumeration, parsing, directory creation, and CSV writes run on the `performance-process-sampler` thread. `PerformanceMonitor` owns that thread, requests a clean stop, flushes the file, quits the event loop, and waits up to three seconds during shutdown.

Read failures are skipped per process. A missing root QtMaps PID or a CSV open/write error stops recording and surfaces an error in the overlay; it does not terminate the map session.

## CSV schema

Columns, in order:

```text
schema_version,timestamp_utc,timestamp_local,timezone_offset,scenario,build_id,build_type,qt_version,renderer,driver,window_width_px,window_height_px,network_mode,dataset_id,dataset_coverage,sampling_interval_ms,warmup_seconds,app_pid,process_count,child_count,current_fps,average_fps,one_percent_low_fps,total_rss_bytes,total_cpu_percent_one_core_100,error
```

Text fields follow RFC 4180-style escaping: a field containing a comma, quote, carriage return, or newline is quoted, and embedded quotes are doubled. A new file receives one header. Appending to an existing file is allowed only when its header exactly matches the schema.

## Commands

Build and test with the established Release path:

```bash
make build
ctest --test-dir build --output-on-failure
```

Generate and serve the local non-geographic fixture in another terminal, then record a deterministic raster run:

```bash
make diagnostic-tiles
make serve-diagnostic-tiles
./build/qtmaps --automated-pan-zoom \
  --benchmark-output /tmp/qtmaps-raster.csv \
  --benchmark-warmup 3 --benchmark-duration 15 \
  --scenario diagnostic-raster-pan-zoom
```

Record a blank-window run by adding `--blank-probe`. Use `--disable-instrumentation --exit-after 15` for an instrumentation-off run measured by an external sampler. Generated CSV files belong outside source control.
