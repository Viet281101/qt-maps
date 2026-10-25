# Unit 05: Route Navigation and Measurement

## Goal and deadline

Complete the demo-critical selected-route replay and guidance flow before the
2026-11-01 feature freeze. On 2026-11-02 and 2026-11-03 only bug fixes,
rehearsal, and reporting are allowed. Stop for owner review after D3 and after
E. Stop earlier if an increment exceeds its estimate by about 50%.

## D. Route navigation (2-3 days)

### D1. Selected-route replay

- Replay the current backend or recorded-fixture route, retaining its visible
  source label. Do not replace the Unit 03 benchmark fixture.
- 1x uses the route's reported duration; 2x and 4x multiply replay progress.
- Provide start, pause/resume, stop, current-position feedback, camera follow,
  heading rotation, and 45-degree tilt without blocking the UI thread.

### D2. Guidance banner

- Show the active next maneuver, street, distance to it, remaining route
  distance/time, and arrival state.
- Advance maneuvers from cumulative polyline distance and shape indices.

### D3. Review gate

- Qt Test covers lifecycle, interpolation, maneuver selection, duplicate
  maneuver positions, route start/end, invalid routes/speeds, and arrival.
- A QML test covers the guidance banner contract. Release `make test`, manual
  fixture and Valhalla smoke, `git diff --check`, short evidence, and the
  progress tracker must pass before owner review.

### D4. Optional re-routing

Only after D1-D3 review and only if schedule remains. Simulate a deviation over
40 m, request a route from the current position, retain generation cancellation,
and show backend failure. Otherwise record the deferred design honestly.

## E. Measurement (1 day, after D review)

Run 120-second cold/warm offline Release scenarios three times over the full
process tree and report medians. CPU/RSS are primary; FPS and p95/p99 confirm
responsiveness and note vsync. Run a 30-minute replay soak only if time remains.

## F. Small follow-ups

Keep free-port selection, known Valhalla extract warnings, clean-shell MapLibre
reproduction, and the verified demo checklist in separate commits. Do not let
them delay the D1-D3 gate.
