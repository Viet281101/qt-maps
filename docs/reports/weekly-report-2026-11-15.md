# QtMaps Weekly Report — 2026-11-15

**Period:** 2026-11-09 to 2026-11-15  
**Milestone/Project:** [Post-demo stabilization](https://github.com/Viet281101/qt-maps/milestone/2) · [Project #6](https://github.com/users/Viet281101/projects/6)  
**Git revision demonstrated:** `14a44a8` (`docs: add deterministic demo rehearsal handoff`)

## Completed This Week

- Merged bounded, explicitly fixture-backed Hanoi place search in PR #53.
- Added the repeatable `make rehearse` entry point, clean-start preflight, and
  operator handoff runbook for issue #9.
- Completed a clean-start operator rehearsal: local vector map became ready;
  Hoàn Kiếm and Thái Hà were selected as A/B; the 7.294 km route with 14
  maneuvers was calculated; and the simulated navigation puck, banner, and
  maneuver progression were observed before normal application shutdown.

## Planned Next

- Start issue #15 only with an owner-approved orthography source decision:
  license, attribution, bounded coverage, zoom range, storage budget, offline
  behavior, and fallback UX.
- Specify and spike SQLite FTS5 only after the orthography decision gate.

## Risks and Blockers

| Risk or blocker | Impact | Evidence/status | Owner action needed |
| --- | --- | --- | --- |
| i915 benchmark freeze | No new GPU benchmark, soak, or configuration work | This rehearsal disabled instrumentation and created no benchmark record | Explicit approval before any expansion |
| Orthography source undecided | No orthography implementation may start | Issue #15 remains a decision gate | Approve a source and limits before data download/generation |
| Search catalogue is bounded | Not general offline POI/address search | Three labeled fixture places; SQLite FTS5 is deferred | None for the completed demo flow |

## Latest Performance Results

No benchmark was run during rehearsal. The retained Release reference-machine
matrix remains the latest performance evidence: 30 direct-GPU runs across five
scenarios and both network modes passed the locked thresholds. See the
[Release benchmark record](../benchmark/report-template.md).

## Decisions and Changes

- **Committed:** Qt 5.15.18 Release runtime; bounded local Hanoi vector map,
  matching raster fallback, local Valhalla route service, and deterministic
  navigation replay.
- **Proposed or awaiting approval:** Licensed bounded orthography; SQLite FTS5
  offline search expansion.
- **Rejected/deferred:** Live GPS, live traffic, voice guidance, nationwide
  coverage, and a benchmark expansion during the i915 freeze.

## Demo and Verification

- Build/test: `make QT_EXPECTED_VERSION=5.15.18 test` — **17/17 passed**.
- Clean-start preflight: `./tools/rehearse-demo.sh --check` — **passed** with
  `hanoi-20261114-v3-r3`, matching `hanoi-raster-20261115-v3-r4`, and local
  Valhalla configuration.
- Rehearsal: `make rehearse` — **passed**. Runtime logged `mapReady=true`,
  local backend readiness, and route application; operator verified the visible
  search, A/B selection, navigation presentation, and clean exit.
- The Valhalla messages about empty tile/traffic extracts are known warnings:
  routing graph tiles are loaded from `tile_dir`; traffic is outside scope.

## Notes for Management

The Hanoi demonstration is now repeatable from a clean local start. It uses a
bounded local map and local routing, with deterministic simulated navigation.
Search is intentionally a small labeled demo catalogue; it is not full
offline geocoding. Raster is a local recovery layer. Orthography is not yet
implemented because its license and data limits have not been approved.
