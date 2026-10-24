# QtMaps Performance Instrumentation

## Measurement semantics

- Current schema: `qtmaps-perf-v2`. Existing `qtmaps-perf-v1` files remain valid historical evidence and are not rewritten.
- Frame source: each `QQuickWindow::frameSwapped` emission counts one completed swap. A one-second interval reports `completed swaps / actual interval seconds`; UI timer firings are never counted as frames.
- Zero-frame intervals: a visible, non-minimized window contributes a `0 FPS` sample. Hidden or minimized intervals display `0 FPS` but are excluded from the summary.
- Legacy summary: `average_fps` remains the arithmetic mean of retained one-second FPS samples and `one_percent_low_fps` remains the mean of the lowest `ceil(sample_count * 0.01)` interval samples. These v1-compatible fields include visible idle intervals and must not be used as the primary render-on-demand responsiveness report.
- Interaction windows: pan, zoom, camera-follow, or navigation automation explicitly opens a bounded interaction window. Only intervals between consecutive `QQuickWindow::frameSwapped` events while that window is active enter the interaction summary. The first swap in each separated window establishes the timing baseline and is not itself a frame-time sample. Idle gaps, hidden/minimized time, and time between interaction windows are excluded.
- Primary render-on-demand report: `interaction_average_fps` is `interaction frame intervals × 1000 / sum(frame times in ms)`. Frame-time p50/p95/p99 use the nearest-rank value from the retained interaction-only frame-time samples. Report FPS average plus p50/p95/p99 together; do not convert the legacy interval 1% low into a frame-time claim.
- Bound: at most 3,600 legacy FPS intervals and 3,600 interaction frame-time samples are retained. CSV process samples are streamed and not retained in memory.
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
schema_version,timestamp_utc,timestamp_local,timezone_offset,scenario,build_id,build_type,qt_version,renderer,driver,window_width_px,window_height_px,network_mode,dataset_id,dataset_coverage,sampling_interval_ms,warmup_seconds,app_pid,process_count,child_count,current_fps,average_fps,one_percent_low_fps,interaction_active,interaction_frame_count,interaction_average_fps,interaction_frame_time_p50_ms,interaction_frame_time_p95_ms,interaction_frame_time_p99_ms,total_rss_bytes,total_cpu_percent_one_core_100,error
```

The three legacy frame columns are retained in v2 for comparison with Unit 02 and the original Unit 01b runs. Interaction columns are cumulative since the post-warm-up reset; use the last complete CSV row for a run summary. `interaction_active` describes the sampling instant and is not used to filter the cumulative percentile fields.

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
