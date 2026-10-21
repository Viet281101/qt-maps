# Unit 02: Performance Instrumentation

Read `AGENTS.md` and the completed unit 01 evidence before starting.

## Goal

Provide low-overhead, reproducible performance instrumentation that shows current FPS in the application and writes timestamped CSV samples for FPS, total resident memory, and total CPU use across QtMaps and every child service it owns.

## Design

The instrumentation has two cooperating parts:

1. **Frame metrics:** observe rendered-frame completion through a Qt 5.13-compatible window/frame signal, aggregate over a documented sampling interval, and expose a read-only metric model to a small switchable QML overlay. The design must state what “FPS” means and avoid presenting UI timer frequency as rendered frames.
2. **Process-tree metrics:** sample the main PID and discovered descendants, aggregate resident memory and CPU consistently, and append rows to a versioned CSV schema. Child processes that start or exit during a run must be reflected without crashing or silently changing scope.

The logger records enough metadata to reproduce and interpret a run: UTC/local timestamp with timezone, scenario, Release build identifier, Qt version, renderer/driver, window size, online/offline mode, dataset identifier/coverage, sampling interval, app PID, child count, FPS, total RSS, and total CPU. CPU units and normalization must be explicit; for Linux process-style percentages, 100% represents one fully utilized logical CPU.

Instrumentation must not perform blocking filesystem or process enumeration work on the UI thread. Buffers and retained samples are bounded. Logger errors are visible but must not terminate an otherwise usable map session.

## Implementation

1. Use the build system and exact commands selected in unit 01; do not introduce a second build path.
2. Define the frame-sampling semantics, update interval, warm-up exclusion, summary statistics, CSV columns, units, escaping rules, and schema version in `docs/benchmarks/`.
3. Implement a `src/perf/` frame collector using an available Qt 5.13 render-completion signal. Cover zero-frame intervals, window hiding/minimization, reset, and clean shutdown.
4. Expose only the current and summary values needed by QML. Add a switchable overlay whose styling uses the planned theme boundary and does not obscure primary controls.
5. Implement Linux process-tree sampling for the QtMaps PID and its descendants. Aggregate RSS in bytes and CPU using a documented interval/delta method; handle short-lived processes, inaccessible entries, PID churn, and malformed reads.
6. Move sampling/parsing and CSV writes off the UI thread or otherwise prove they are non-blocking. Give the worker an explicit owner, cancellation path, and bounded shutdown.
7. Write CSV through a safe append/create path with one header per file, correct field escaping, flush/error handling, and no unbounded in-memory history.
8. Add Qt Test coverage for frame aggregation, process-sample aggregation, CPU math, CSV escaping/schema, start/stop, and representative failures. Use fixtures or injectable readers rather than depending on unstable live process values in unit tests.
9. Run fixed Release scenarios for the blank window and minimal raster proof from unit 01 with instrumentation disabled and enabled. Record logger overhead, FPS/RAM/CPU, duration, window size, and process scope in a benchmark result based on `docs/benchmarks/benchmark-template.md`.
10. Update `context/progress-tracker.md` with the exact commands, tests, measurements, limitations, and the next unit.

## Dependencies

- Unit 01 complete with Qt 5.13 provisioned, a primary build system selected, exact Release commands recorded, and blank/raster proofs runnable.
- Qt Quick render-completion mechanism confirmed available in Qt 5.13.
- Linux process accounting interfaces available on the reference machine.
- `docs/benchmarks/benchmark-template.md` and the reference hardware baseline.
- No routing or tile-server child service is required; a controlled short-lived child fixture may test aggregation if necessary.

## Verify When Done

- [ ] The Release application can enable/disable an FPS overlay whose value is derived from completed frames and whose behavior for hidden/minimized windows is documented.
- [ ] Starting a benchmark creates a schema-versioned CSV with the documented metadata and sample columns; stopping it flushes and closes the file cleanly.
- [ ] RAM and CPU totals include the main app plus all discoverable descendants, and a controlled child-process test proves inclusion and removal.
- [ ] CPU normalization, RSS units, sampling interval, warm-up, and summary calculations are documented and tested.
- [ ] File errors, disappearing processes, malformed samples, and shutdown are handled without blocking or crashing the UI.
- [ ] Automated Qt Test checks pass using the exact project test command.
- [ ] Instrumentation-on versus instrumentation-off Release results quantify overhead for both the blank window and minimal raster scenario.
- [ ] A completed benchmark table contains FPS, total RAM, total CPU, build ID, Qt/renderer details, viewport, duration, dataset, network mode, and process scope.
- [ ] `context/progress-tracker.md` contains current evidence and no architecture invariant is violated.
- [ ] **BUILD PASS** is recorded using the actual command established in unit 01.
