# Unit 06: Process-Tree Benchmark

## Goal

Produce reproducible Release-build benchmarks for QtMaps, Valhalla, and every
owned descendant process. One command runs the fixed scenarios, records raw
per-process and process-tree evidence, and emits management-ready Markdown and
machine-readable JSON summaries without changing normal application behavior.

## Locked acceptance thresholds

These thresholds were approved and locked before Unit 06 measurements. They
must not be relaxed after results are observed. A result beyond a threshold is
reported as **FAIL / khong dat** with analysis; it is never hidden or used to
retune the threshold.

The reference CPU has two physical cores and four logical CPUs. Raw Linux CPU
uses `100% = one fully utilized logical CPU`. Machine-normalized CPU divides
that value by the detected logical CPU count, so `100% = all four logical CPUs`
on the reference Dell Latitude 5480.

| Metric | Locked threshold |
| --- | ---: |
| Interactive average FPS | at least 30 FPS |
| Interactive FPS p5 | at least 24 FPS |
| Interactive 1% low | at least 20 FPS |
| Average frame time | at most 33.3 ms |
| Frame-time p95 | at most 50 ms |
| Idle machine-normalized CPU, average / peak | at most 5% / 15% |
| Pan/zoom and navigation machine-normalized CPU, average / peak | at most 40% / 75% |
| Valhalla route-computation machine-normalized CPU peak | at most 65% |
| Process-tree RSS, average / peak | at most 650 / 768 MiB |
| Process-tree PSS, average / peak | at most 550 / 640 MiB |
| Sampler machine-normalized CPU overhead | at most 1 percentage point average |
| Sampler memory overhead | at most 10 MiB |
| Sampler duration p95 at the 500 ms fast interval | at most 50 ms |

PSS is the primary RAM measure because it apportions shared mappings between
processes. RSS remains mandatory supporting evidence. Reports explain material
RSS/PSS differences rather than treating summed RSS as uniquely owned memory.
Startup time and route latency are report-only for the first baseline.

## Measurement semantics

- Fast process sampling defaults to 500 ms and records CPU, RSS, thread count,
  identity, role, and tree totals.
- `/proc/<pid>/smaps_rollup` sampling defaults to a separate 2,000 ms interval.
  Missing permission or kernel support is non-fatal and is recorded per sample.
  The interval may be increased only when measured sampler overhead exceeds a
  locked overhead threshold; the changed interval and reason must be metadata.
- A process identity is PID plus `/proc/<pid>/stat` start time, preventing a
  reused PID from inheriting another process's CPU baseline.
- Roles are `app`, `valhalla`, and `other`; every readable process also retains
  PID, parent PID, command name, executable where readable, and thread count.
- The legacy aggregate `qtmaps-perf-v3` CSV remains unchanged. Detailed samples
  use a separate versioned `qtmaps-process-v1` CSV with per-process and total
  rows. CPU is written in both normalization forms.
- “First rendered map frame” is the first completed map render signal emitted
  after the map reports ready. A generic window swap before map readiness does
  not satisfy the startup measure. Startup duration runs from process launch to
  that signal; the nearest process-tree RAM sample is reported with it.
- Warm-up samples establish CPU baselines but are excluded from statistics.
- Each scenario repeats three times by default. Reports show the median and
  min/max across runs; per-run mean and peak samples remain available.

## Fixed scenarios

1. `startup`: launch to the first rendered map frame, with RAM at that point.
2. `idle`: 60 seconds after map readiness.
3. `pan_zoom`: one fixed scripted camera path and zoom sequence.
4. `route`: fixed Hanoi endpoint pairs, recording route latency and Valhalla
   CPU/RAM peaks during each request.
5. `navigation_replay`: selected-route replay with the maneuver banner and one
   deterministic simulated deviation that triggers re-routing.

The runner supports normal-host and offline operation. Offline evidence uses a
loopback-only network namespace where the host permits it; inability to create
that namespace is explicit and never mislabeled as disconnected execution.

## Implementation increments

1. Extend and test process snapshots, tree membership, PSS, thread count, PID
   reuse protection, role classification, dual CPU normalization, and sampler
   timing. Add the sidecar CSV without changing `qtmaps-perf-v3`.
2. Add a benchmark scenario controller and CLI with bounded warm-up, duration,
   completion, failure, and automatic exit paths.
3. Add the Python standard-library runner and summarizer, one-command Makefile
   target, host/runtime/dataset metadata, Markdown and JSON output.
4. Add documentation, a short CLI smoke test, external `ps`/`htop` comparison,
   sampler-overhead evidence, full scenario runs, and the final report.

## Definition of done

- One command builds Release, runs every scenario in the requested network
  modes and repetitions, then writes raw CSV, summary JSON, and Markdown.
- App, Valhalla, other descendants, and tree totals are separately visible;
  process churn and inaccessible `smaps_rollup` are handled without crashing.
- PSS is primary, RSS is accompanying evidence, and CPU is reported with both
  documented normalizations.
- FPS, frame time, RAM, CPU, disk size, startup time, and route latency use the
  approved aggregation and report format. Every locked threshold is evaluated.
- Unit tests, the short scenario smoke test, existing tests, `make check`, and
  isolated Release review pass. At least one full real scenario suite runs on
  the reference laptop and records unverified limitations honestly.
- Process-tree totals agree with an external observation within a documented
  reasonable tolerance, and sampler overhead meets the locked thresholds or is
  reported as not meeting them with analysis.
