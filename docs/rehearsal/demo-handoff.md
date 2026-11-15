# QtMaps Demo Rehearsal and Handoff

This runbook is the acceptance procedure for issue #9. It rehearses the
local-first Hanoi route, search, and navigation flow. It is not a benchmark,
performance soak, GPU investigation, or test of nationwide coverage.

## Preconditions

- Use the committed Release build for Qt 5.15.18. Qt 5.15.13 is comparison-only.
- The active vector package and its matching raster fallback must be provisioned
  under `data/packages/current` and `data/raster-packages/current`.
- The local Valhalla executable and `generated/unit-04/valhalla/valhalla.json`
  must be present. The application starts and terminates that loopback service;
  do not run another copy manually.
- Do not change the i915 driver/GPU configuration or supply benchmark flags.
- Network access is not required once these local packages are provisioned.

## Clean-start procedure

From the repository root, use a clean worktree and run:

```bash
make QT_EXPECTED_VERSION=5.15.18 test
./tools/rehearse-demo.sh --check
make rehearse
```

`--check` validates the selected Qt/MapLibre runtime, release executable,
vector and raster manifests/tile files, and the Valhalla payload. `make
rehearse` rebuilds the selected Release target if needed, then launches the
same `tools/run-demo.sh` runtime contract used by normal demo runs with
instrumentation disabled. It creates no benchmark record.

## Operator checklist

1. Confirm the bounded Hanoi vector map becomes ready with Vietnamese labels.
2. Open search; search for **Hoàn Kiếm**, select it as A/origin, then search
   **Thái Hà** and select it as B/destination. State that these are bounded,
   fixture-backed demo results, not general POI/address search.
3. Calculate the route. Confirm a visible route, route summary, and maneuver
   list; wait for the local Valhalla state rather than presenting a pending or
   error state as a completed route.
4. Start navigation. Confirm the simulated-position puck, camera following,
   next-maneuver banner, maneuver progression, and re-route presentation.
   State that position is deterministic simulation, not live GPS guidance.
5. Switch to Raster only if the vector layer fails to become ready, or when
   deliberately showing the recovery path. Confirm that the UI identifies the
   active/degraded state rather than silently changing layers.
6. Quit the application normally. Confirm it closes without leaving its
   application-owned Valhalla child service running.

Record the git revision, package IDs, pass/fail result for each step, and any
visible error in the successor weekly report. Do not overwrite prior reports
or benchmark evidence.

## Fallbacks and stop conditions

| Condition | Operator action | What to say |
| --- | --- | --- |
| Vector map does not become ready | Select Raster recovery if offered; otherwise stop the visual map flow. | Raster is a bounded local fallback, not an orthography layer. |
| Route service fails or route remains in an error state | Stop the route/navigation portion; capture the exact error and do not substitute a completed route. | Local routing is unavailable for this run. |
| Search returns no result | Re-enter one of the three supported fixture names exactly; if it still fails, stop and record it. | Search is limited to the labeled demo catalogue. |
| App/package preflight fails | Do not start the demo; repair/provision the named local prerequisite, then repeat from clean start. | The demo payload is not ready. |
| Orthography is requested | Do not claim it exists or attempt a download. | Orthography remains pending a licensed-source decision. |

## Management-facing limits

- Coverage is a bounded Hanoi area, not nationwide Vietnam.
- Vector map and raster fallback are local after provisioning; orthography is
  not implemented.
- Search uses a small fixture catalogue. SQLite FTS5/offline geocoding remains
  deferred.
- Routing is local Valhalla for the supplied Hanoi graph; navigation replay is
  deterministic simulation. There is no live GPS, traffic, or voice guidance.
- Existing Release benchmark reports are reference-machine evidence; this
  rehearsal neither repeats nor expands them.
