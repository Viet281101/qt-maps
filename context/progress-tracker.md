# Progress Tracker

This file is the concise operational snapshot. Detailed implementation evidence
remains in the cited pull requests and Git history; generated data and raw
benchmark artifacts remain ignored.

## Current phase

- Units 03--08 are merged. Qt 5.15.18 is the production baseline; Qt 5.15.13
  is comparison-only.
- Unit 09 merged in PR #62. Unit 10 (#15) has an owner-supplied authorized
  local Sentinel-2 L2A scene; deterministic processing, app integration, the
  Qt 5.15.18 test suite, and functional visual review are complete locally.
  [PR #63](https://github.com/Viet281101/qt-maps/pull/63) merged as `71ee64e`
  on 2026-11-15.
  #15 remains open only for its separately approved performance observation.
- Unit 11 (#64) is merged on the current `main` as `5436b2d` (PR #66): its
  readiness controller validates vector, raster, search, and orthography
  packages off the UI thread; shows a compact settings/readiness panel; and
  persists the Vietnamese/English preference. Local Release tests passed
  24/24 on 2026-11-17. The live Issue/Project fields still require a separate
  GitHub-state check before this file treats them as closed or `Done`.
- The active GitHub milestone is `Post-demo stabilization` with no due date.
- [PR #53](https://github.com/Viet281101/qt-maps/pull/53) merged as `15d198a`
  on 2026-11-15 and closed #16. Project #6 records both items as `Done`.
- The management-demo vertical slice is accepted: #9 and #1 are closed and
  `Done` on Project #6. #15 (licensed orthography, `In Progress`, P1) has its
  integration merged in PR #63. #58 (offline SQLite FTS5 search
  spike, P1) is closed and `Done` on Project #6.

## Completed product baseline

- Local Hanoi vector and validated raster packages render through the MapLibre
  and loopback boundaries. The app keeps one readable app-owned OSM attribution
  pill, retains route/puck/map items across layer switches, and rejects
  incompatible raster/vector packages.
- Fixture-backed routing and navigation provide endpoint entry, route geometry,
  maneuvers, simulated movement, re-routing presentation, bounded camera
  follow, and explicit simulation labels. The navigation banner is the sole
  owner of the active instruction while replay is running.
- Bounded local place search supplies Hoàn Kiếm, Thái Hà, and Ba Đình with
  debounced idle/loading/ready/empty states and explicit A/B endpoint actions.
  It is visibly labelled as fixture data. The SQLite FTS5 capability is proven,
  but wider coverage and all remote geocoding remain deferred.
- The local-only `qtmaps_visual_review` harness captures production QML in
  vector and forced-raster modes at 900x580, 1366x768, and 1920x1080. Its
  latest #53 review drives the actual search input and verifies the popover,
  connection pill, route, and navigation framing do not overlap.

## Original baseline requirements: remaining acceptance work

The first management-demo slice is complete, but the following work remains
before the original product requirements can be described as complete. These
items deliberately distinguish real local data and routing from the existing
fixture-backed or simulated presentation.

1. **Local map delivery and offline proof.** Make the approved raster, vector,
   orthography, and search packages reproducibly available on a clean target
   machine without a network connection. Record package versions, manifests,
   checksums, attribution, setup steps, and the supported coverage/zoom bounds.
   Demonstrate selecting each installed layer with networking disabled. The
   current app validates developer-provisioned local packages. Unit 12 now
   uses owner-provisioned explicit local imports, manual updates, one retained
   rollback bundle, and no runtime downloader; routing graphs remain separate
   artifacts.
2. **Real nationwide car routing.** Build and retain evidence for a Valhalla
   graph from the approved uncropped Geofabrik Vietnam PBF, including input
   hash, tool/version configuration, graph-build duration, and peak RAM. Wire
   arbitrary two-point requests in the application to that graph and verify
   routes that exercise national and provincial roads. The acceptance evidence
   must show route geometry and maneuvers from the routing engine, explicit
   failure/degraded states, and must not rely on a route fixture.
3. **Navigation from a real route.** Feed a successful real routing response
   into the existing selected-route replay so the guidance view, camera follow,
   rotation, maneuver progression, and re-route presentation are validated
   end to end. Keep the position source explicitly labelled as simulated until
   real GNSS/GPS support is separately approved; it must not be represented as
   live turn-by-turn navigation.
4. **Reference-laptop performance evidence.** After the i915 benchmark freeze
   is lifted, run the locked Release scenarios against the real local packages
   and nationwide routing graph. Publish FPS/frame-time, process-tree PSS/RSS,
   CPU, duration, viewport, data package, and network mode; compare them with
   the approved thresholds. Functional visual review is not a substitute for
   this benchmark.

## Current verified evidence

- PR #59 merged as `0c7964d` and closed #58: the QSQLITE FTS5 spike creates and
  queries an external-content Vietnamese fixture schema locally; Release tests
  passed 19/19.
- The #61 fixture-only package builder validates Hanoi bounds and PBF provenance,
  emits a checksummed FTS5 SQLite package under the 20 MiB cap, and passed the
  isolated Release review with 20/20 tests.
- `make QT_EXPECTED_VERSION=5.15.18 review` passed for PR #53: formatting,
  isolated Release build, and 17/17 tests.
- Direct-GPU visual captures completed on the Intel HD 520 for the supported
  viewports and vector/raster states. This is functional-layout evidence, not
  a performance benchmark.
- Current ignored compatible packages are `hanoi-20261114-v3-r3` and
  `hanoi-raster-20261115-v3-r4`; each has the v2 glyph revision and matching
  v3 manifests.
- Environment documentation was synchronized on 2026-11-15: system Qt 5.15.18
  is the production baseline, while Qt 5.15.13 remains comparison-only. The
  checked `make QT_EXPECTED_VERSION=5.15.18 requirements` preflight passes.

## Unit 09 — Offline search integration (completed locally 2026-11-15)

- The approved local Geofabrik PBF was extracted offline into 9,094 named
  Hanoi records and packaged as ignored `hanoi-search-20261115-v1-r1`.
  `places.sqlite` is 1,060,864 bytes, under the 20 MiB cap; its manifest
  records the 2026-10-23 source retrieval date, PBF SHA-256, ODbL attribution,
  Hanoi bounds, and database checksum.
- `PlaceSearchController` validates and opens the package on a worker thread,
  queries bounded normalized FTS5 results there, labels the real OSM source in
  the UI, and preserves visibly labelled fixture search if the package is
  absent, corrupt, or incompatible. No remote geocoding path exists.
- Automated Release evidence: search controller, FTS5 spike, package builder,
  extractor, and QML routing suites passed. Direct-GPU visual review captured
  vector and raster at 900x580, 1366x768, and 1920x1080 under
  `generated/unit-09/visual-review-20261115-final-capped/`; it is visual evidence,
  not a benchmark. Run the isolated `make ... review` gate before merging #62.

## Unit 10 — Licensed orthography integration (active 2026-11-15)
- The owner supplied the ignored local Sentinel-2C L2A ZIP
  `S2C_MSIL2A_20261014T032711_N0513_R018_T48QWJ_20261014T082606.SAFE.zip`.
  Its SHA-256 is
  `e2de4c393374dd82b059dd69be14cb16e696cc1ef7d59418bf19d5d896f75232`; metadata
  confirms the 2026-10-14 acquisition, processing baseline 05.13, EPSG:32648,
  B02/B03/B04 10 m inputs, and 12.59245% declared cloud cover.
- The local-only preparation pipeline generated the complete Hanoi z12--z15
  RGB tree (226 tiles). The offline package builder produced the ignored
  `hanoi-orthography-20261115-s2-l2a-v2` package: 14,700,544 bytes, below the
  100 MiB cap, with MBTiles SHA-256
  `6286ce7d3956d83d324619f879d1b11d7a05f59cfc833c22319700da63370e70`.
- The application validates the orthography manifest and MBTiles checksum on a
  lazy worker, offers an explicit local 10 m layer, preserves Sentinel and OSM
  attribution, limits the layer to z15, and reports missing/corrupt packages
  without a remote fallback. The visual-review harness captured the active
  layer at 900x580, 1366x768, and 1920x1080; this is functional evidence only.
- [PR #63](https://github.com/Viet281101/qt-maps/pull/63) merged the Unit 10
  integration as `71ee64e`; raw imagery and generated artifacts are
  intentionally absent from its diff. It satisfies four #15 acceptance items;
  the short Release performance observation remains deferred by the i915 freeze.

## Unit 11 — Offline readiness and application preferences (completed locally)

- PR #66 is represented by `5436b2d` on the local `main`; the controller
  validates required and optional packages asynchronously, reports actionable
  readiness, and persists Vietnamese/English preferences.
- The controller maintains timeout-safe worker ownership, probes SQLite FTS5
  read-only after manifest validation, and distinguishes fixture fallback,
  unavailable orthography, and missing raster.
- Release tests passed 24/24 on 2026-11-17. The functional QML review captured
  vector, raster, orthography, and route/navigation states at all supported
  viewports; the final single-blue route remained readable over both light maps
  and imagery. This is functional evidence, not a performance benchmark.
- Detailed implementation and review evidence remains in PR #66 and
  `context/feature-specs/unit-11-offline-package-readiness-plan.md`; confirm
  the live Issue/Project state separately before marking external tracking done.

## Next delivery plan

1. **#9 rehearsal and #1 handoff — completed.** PR #54 merged as `6e11910` on
   2026-11-15; #9 and #1 are closed and their Project #6 items are `Done`.
   The clean-start preflight, route/search/navigation operator checklist, and
   clean exit passed. Evidence and management-facing limits are in
   `docs/rehearsal/demo-handoff.md` and
   `docs/reports/weekly-report-2026-11-15.md`; no benchmark was added.
2. **#15 — licensed orthography integration (P1).** The owner approved the
   Sentinel-2 Level-2A 10 m regional RGB source, Hanoi-only bounds, z12–z15,
   100 MiB package cap, required attribution, offline-only runtime, and explicit
   unavailable/failure UX on 2026-11-15. PR #55 merged as `37534a7`. The exact
   contract and provisioning gate are in
   `context/feature-specs/15_orthography-decision-gate.md`. The first offline
   package/manifest contract now has a compact synthetic fixture and focused
   tests; it records complete coverage and provenance but does not process real
   imagery. PR #57 merged as `631d713`. Unit 10's complete integration merged
   in PR #63, including review-gate and functional visual evidence. Keep #15 open for the separately
   approved performance observation while the i915 freeze remains active.
3. **#64 — readiness and preferences (P1, merged locally).** PR #66 is
   represented by `5436b2d` on the current local `main`; the 2026-11-17
   Release test gate passed 24/24. Check the live GitHub Issue and Project
   state before closing or moving its external tracking items to `Done`.
4. **Unit 12 — local package delivery ([#65](https://github.com/Viet281101/qt-maps/issues/65)).** The owner approved local-file provisioning:
   map/search bundles are explicitly imported and manually updated; the
   compatible vector/raster pair and search package activate together; permitted
   orthography remains optional; routing graphs are separate local artifacts;
   and QtMaps has no runtime downloader. Slice 1 is implemented locally as
   `OfflinePackageContract`: it validates safe, complete map/search bundle and
   routing-graph activation metadata without replacing existing package
   validators; its focused Qt 5.15.18 Release test passed on 2026-11-17.
   Slice 2 is also implemented locally: `OfflinePackageInstaller` validates
   the candidate with those validators, atomically switches local `current`
   and `previous` pointers, and supports explicit rollback without copying or
   downloading owner-provided data. Focused activation/rollback tests passed
   on 2026-11-17. Slice 3 is also implemented locally: a separate routing
   graph contract validates non-empty graph storage, activates
   `routing-current.json`, and reports the graph path without modifying
   Valhalla runtime configuration. Focused tests passed on 2026-11-17. Slice 4
   is implemented locally: the existing off-UI-thread readiness worker reports
   routing activation as an optional Offline routing item in Settings with
   recovery guidance, and `--routing-activation-root` supplies the local
   pointer directory. The Unit 12 spec now includes the clean offline operator
   checklist. Its explicit activation CLI validated and activated the local
   owner-provisioned map/search bundle and routing graph on 2026-11-17; the
   headless local-only walkthrough returned a Valhalla route of 7,294 m with
   14 maneuvers. Evidence is in
   `docs/rehearsal/unit-12-offline-walkthrough-20261117.md`; it is functional
   evidence, not a benchmark or clean-target transfer test. Full Qt 5.15.18
   Release tests passed 25/25 on 2026-11-17. Unit 12 is complete locally and
   ready for its canonical implementation PR; a clean-target repeat remains
   deployment evidence after merge.

## Risks and decisions requiring the owner

1. The i915 render-benchmark freeze remains in effect. The completed matrices
   and direct-GPU visual review do not authorize a new benchmark, soak,
   MapLibre/OpenGL investigation, or GPU configuration change.
2. Orthography is now provisioned from an owner-supplied local Sentinel-2 L2A
   scene. Credentials and raw source data remain outside Git; nationwide or
   higher-resolution coverage still needs a separate decision.
3. SQLite FTS5 target-runtime capability is proven, but its source dataset,
   coverage, storage budget, generator, and runtime package lifecycle remain
   Proposed except for #61's local package-generator contract; extractor and
   runtime work still require separate acceptance.

## Working rules

- Before each code commit, run `make QT_EXPECTED_VERSION=5.15.18 test`; run
  `make QT_EXPECTED_VERSION=5.15.18 review` before review-ready handoff.
- Use Qt 5 CMake APIs with CMake/Ninja. Do not use Qt 6 APIs, force-push,
  rewrite pushed history, push directly to `main`, or commit generated data,
  build trees, raw benchmarks, credentials, or user-owned files.
- Each PR uses its canonical Issue, active milestone, Project #6, matching
  Priority/Demo Phase, assignment to `Viet281101`, and live-state verification
  after GitHub mutations.
- `tools/run-demo.sh --print-env` selects the approved Qt 5.15.18 payload. The
  explicit `maplibre-qt51518` rebuild remains owner-authorized work only.
