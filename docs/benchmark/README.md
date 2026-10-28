# QtMaps Process-Tree Benchmark

This directory documents the Unit 06 benchmark used for the 2026-11-03
management demo. Only Qt 5.15.13 Release results from the reference Dell
Latitude 5480 are accepted as project benchmark evidence. The locked thresholds
are defined in `context/feature-specs/06_process-tree-benchmark.md` and must not
be changed after measurements are observed.

## One-command run

The full default matrix builds Release and runs all five scenarios three times
with the host network available and again inside a loopback-only network
namespace. It also performs a matched instrumentation-off/on overhead check.

```bash
make benchmark
```

Generated evidence is ignored under `generated/benchmarks/<timestamp>/`:

- `session-metadata.json`: machine, software, Git, dataset, disk-size, interval,
  and repetition identity;
- `runs/<scenario>-<mode>-rN/metrics.csv`: unchanged aggregate
  `qtmaps-perf-v3` frame/RSS/CPU samples;
- `runs/.../metrics-processes.csv`: `qtmaps-process-v1` per-process and tree
  rows with PSS, RSS, threads, roles, and both CPU normalizations;
- `runs/.../run.json` and `run.log`: command identity and structured scenario
  events, including report-only startup and route latency;
- `sampler-overhead.json`: matched external measurements with instrumentation
  disabled and enabled; and
- `summary.json` plus `report.md`: median [min, max] aggregation and locked
  pass/fail evaluation.

Useful bounded runs:

```bash
# One scenario and one repetition, still using the real backend.
make benchmark BENCHMARK_ARGS="--scenarios pan_zoom --mode offline --repeat 1"

# Direct application run. The runner normally supplies all metadata fields.
./tools/run-demo.sh --benchmark idle --duration 60 \
  --benchmark-warmup 5 --benchmark-interval 500 \
  --benchmark-pss-interval 2000 \
  --benchmark-output /tmp/qtmaps-idle.csv

# Rebuild a report from already recorded raw files.
python3 tools/benchmark/summarize_benchmarks.py \
  generated/benchmarks/<timestamp>
```

The runner exits `0` when all runs and locked thresholds pass, `2` when raw
evidence was produced but one or more locked thresholds are `FAIL / không đạt`,
and nonzero with an explicit error when execution or evidence generation fails.
Do not discard a code-2 report; it is the required failure evidence.

## Scenarios

| Scenario | Definition |
| --- | --- |
| `startup` | Process launch to the first completed map render; RAM is sampled immediately after that frame. |
| `idle` | 60 measured seconds after map/backend readiness and warm-up. |
| `pan_zoom` | Fixed 250 ms camera commands over the same six Hanoi coordinates, zooms, and bearings. |
| `route` | Three fixed Hanoi route requests (two in fixture-only test mode), reporting each backend latency and Valhalla peaks. |
| `navigation_replay` | 4x selected-route replay with banner/camera updates and exactly one simulated deviation/re-routing request. |

“First rendered map frame” is deliberately not the first generic window swap.
The QML `Map.mapReadyChanged` signal arms the measurement, then the first
`QQuickWindow::frameSwapped` completion after map readiness ends startup. This
is the first completed render containing the ready map according to the Qt 5
signals available to the application.

Warm-up establishes CPU/PSS baselines but is excluded from both CSV schemas and
summary statistics. Startup and route latency are report-only in the first
baseline; they have no pass/fail threshold.

## Reading CPU

The reference i5-6300U has **two physical cores and four logical CPUs**.

- `cpu_percent_one_core_100`: Linux process-style CPU; 100% means one logical
  CPU is fully busy. A two-worker service can exceed 100%.
- `cpu_percent_all_cores_100`: the preceding value divided by the detected
  logical CPU count; 100% means the whole four-logical-CPU machine is busy.

Both values come from monotonic deltas of `/proc/<pid>/stat` user plus system
ticks. PID plus process start time prevents a reused PID from inheriting an old
baseline. New processes need two observations before contributing CPU delta.

## Reading PSS and RSS

PSS is the primary RAM measure. `/proc/<pid>/smaps_rollup` apportions a shared
mapping between the processes using it, so process-tree PSS is a closer estimate
of physical memory attributable to QtMaps and Valhalla. RSS counts the whole
shared mapping in each process and therefore normally produces a larger summed
tree value. Every report states the observed RSS-minus-PSS difference.

Fast CPU/RSS/thread samples default to 500 ms. PSS is read separately every
2,000 ms and cached with `pss_sample_age_ms` between reads. If PSS is denied or
unsupported, rows record `pss_available=0` and the error; the report marks the
RAM result unavailable rather than silently substituting RSS. If matched
overhead exceeds a locked threshold, the result remains `FAIL / không đạt`.
Any later slower PSS interval must be explicit in session metadata and its
reason recorded; it cannot rewrite the failed evidence or locked threshold.
The first rendered-frame and scenario-completion checkpoints force one fresh
PSS read, so their reported RAM is never a cached route-phase value.

## Repetitions and thresholds

Each run first calculates mean/peak values from its time samples. Across N
repetitions the report shows the **median [minimum, maximum]** for every metric.
Locked thresholds are evaluated against the median per-run metric. A single-run
breach remains visible in the min/max range even when the median passes. Raw
per-run files are retained for investigation.

Interactive FPS is calculated from completed interaction-frame intervals.
FPS p5 is the nearest-rank fifth percentile of per-frame FPS; 1% low is the
arithmetic mean of the slowest one percent of those frame FPS values. Average
frame time and nearest-rank p95 are calculated from the same bounded interaction
window. Idle render-on-demand FPS is not a responsiveness threshold.

## Online and offline modes

`online` means the normal host network remains available; map and route inputs
still use only the approved local package and loopback Valhalla service.
`offline` creates a user/network namespace, enables only loopback, and runs the
same command and local services inside it. If `unshare -Urn` or `ip` is not
available, the runner stops with an explicit error and never labels the run
offline.

The runner disables optional accessibility bridging and removes `SESSION_MANAGER`
for benchmark child processes only. A loopback-only namespace isolates abstract
Unix sockets; otherwise Qt's X11 session-manager handshake retries for about one
second before falling back to a filesystem socket. This is a benchmark harness
artifact, not a map, routing, or normal offline-launch request.

## External cross-check

During a real run, compare the sidecar's latest `process` and `tree_total` rows
with a simultaneous snapshot such as:

```bash
pstree -ap <qtmaps-pid>
ps -o pid,ppid,rss,pcpu,nlwp,comm -p <comma-separated-tree-pids>
```

`ps` RSS is KiB and its `%CPU` averaging window differs from the benchmark's
500 ms delta, so compare scope and order of magnitude rather than expecting an
identical instant. PSS can be checked by summing each readable
`/proc/<pid>/smaps_rollup` `Pss:` value. Record the timestamp, PIDs, sidecar
rows, tolerance, and explanation in the final evidence.

## Limitations

- A process that starts and exits entirely between two 500 ms samples cannot be
  observed; owned long-lived QtMaps and Valhalla processes are covered.
- PSS access depends on Linux `/proc` permissions and kernel support.
- PSS values between 2,000 ms reads are cached and carry their age.
- The sampler itself consumes resources. The external instrumentation-off/on
  comparison is authoritative for overhead; self-reported duration only
  diagnoses sampling work.
- `frameSwapped` confirms a completed Qt Quick window swap after map readiness;
  it does not prove that every map tile for the viewport has reached a final
  cache state.
- Online mode does not intentionally contact public map services, so it tests
  host-network availability versus hard disconnection, not remote tile speed.
