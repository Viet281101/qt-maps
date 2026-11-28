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
  24/24 on 2026-11-17. Issue #64 is closed; both issue and PR are `Done`
  in Project #6, verified on 2026-11-18.
- Unit 14A/14C/14B merged in PRs #73/#78/#80; #71/#74/#75/#77/#79 are `Done`.
  Linux v1.0 remains gated by nationwide routing, real-route replay, release
  rights and the reference benchmark after the i915 freeze.
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
- Local Valhalla routing with fixture fallback and simulated navigation provide
  endpoint entry, route geometry, maneuvers, re-routing, bounded camera follow,
  and explicit simulation labels. The navigation banner owns the active replay
  instruction; nationwide routing acceptance remains open.
- Local Hanoi OSM search uses a validated SQLite FTS5 package with debounced
  states and A/B endpoint actions. A visibly labelled fixture is used when the
  package is unavailable; wider coverage and remote geocoding remain deferred.
- The local-only `qtmaps_visual_review` harness captures production QML in
  vector and forced-raster modes at 900x580, 1366x768, and 1920x1080. Its
  latest #53 review drives the actual search input and verifies the popover,
  connection pill, route, and navigation framing do not overlap.

## Original baseline requirements: acceptance status

The first management-demo slice and clean Linux delivery are complete. The
remaining gates distinguish real nationwide routing from fixture-backed or
simulated presentation.

1. **Local map delivery and offline proof — completed in Unit 14A.** A clean
   Ubuntu 26.04 VM passed real Hanoi vector/raster/orthography/search, route,
   restart and rollback offline. Coverage remains bounded; see its VM report.
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

- PR #59 closed the QSQLITE FTS5 spike (#58); Release tests passed 19/19.
- The #61 fixture-only package builder validates Hanoi bounds and PBF provenance,
  emits a checksummed FTS5 SQLite package under the 20 MiB cap, and passed the
  isolated Release review with 20/20 tests.
- `make QT_EXPECTED_VERSION=5.15.18 review` passed for PR #53: formatting,
  isolated Release build, and 17/17 tests.
- Direct-GPU visual captures completed on the Intel HD 520 for the supported
  viewports and vector/raster states. This is functional-layout evidence, not
  a performance benchmark.
- Qt 5.15.18 requirements preflight passes; Qt 5.15.13 is comparison-only.

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

## Unit 11 — Offline readiness and application preferences (completed)
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
  `context/feature-specs/unit-11-offline-package-readiness-plan.md`; issue #64
  and PR #66 are closed/merged and both are `Done` in Project #6, verified
  on 2026-11-17.

## Next delivery plan

- **Completed baseline:** Implementation for Units 09–14 is merged. Unit 14A real-data clean VM
  evidence is in `docs/rehearsal/unit-14a-7480-vm-results.md`; Unit 14C Linux
  CI passed 24/24 non-benchmark tests and networkless synthetic fixture smoke
  in [run #45760887328](https://github.com/Viet281101/qt-maps/actions/runs/45760887328).
  Unit 14B records Windows readiness only; native Windows remains optional.
  Unit 12/13 activation evidence: [PR #67](https://github.com/Viet281101/qt-maps/pull/67)
  and [PR #70](https://github.com/Viet281101/qt-maps/pull/70).
  #15 is open solely for its short Release performance observation after the
  i915 freeze. Unit 14 issues #71/#74/#75/#77/#79 are closed and `Done`.
- **Now — V1-A:** The 2026-11-26 preflight verified the uncropped PBF SHA-256
  `4e5243c3116dc089a298f4cd7218e1257adf4b48355e42936360df5ce3200d43`,
  Valhalla native 3.9.1, about 10 GiB available RAM and 102 GiB free disk.
  The two-worker Valhalla 3.9.1 build finished in 6:01.84, peak RSS 4,242,968
  KiB, producing 734 graph tiles (1,164,439,264 bytes). Four frozen routes
  returned real geometry and maneuvers; a supplemental Thu Dau Mot–Di An route
  returned ĐT.743A and an out-of-coverage request returned an explicit error.
  App activation remains to verify. See
  `docs/routing/v1-a-national-graph-20261126.md`. Generated data remains outside
  Git. The Qt 5.15.18 Release suite passed 25/25 after provisioned map/runtime
  paths were linked into the isolated worktree. PR #82 carries V1-A
  implementation and the remaining release gates.
- **V1-B functional review completed:** Unit 12 activated the nationwide graph and Unit 13
  selected it through a private runtime config changing only `mjolnir.tile_dir`.
  The Release app's Valhalla child returned a real 7,294 m route with 14
  maneuvers. An opt-in Qt Test parsed a real Hue–Da Nang response, checked
  replay/maneuver progression, simulated deviation, replacement route and
  arrival. The regular Release suite passed 25/25. See
  `docs/routing/v1-b-real-route-replay-20261126.md`. Direct-rendering
  functional captures at supported viewports show route, camera rotation,
  simulated puck and banner from a real Valhalla route; the banner now labels
  the simulated position. Further GUI captures show Valhalla re-route and
  arrival. The long Hue–Da Nang replay passed at the controller boundary;
  Hanoi was used for GUI review within detailed map coverage. This is not
  benchmark evidence. Next: V1-C release candidate preparation and its
  remaining notice and clean-target GUI checks.
- **Now — V1-C:** The existing Unit 14C synthetic CI archive is being extended
  with an operator runbook and hashed V1-A/V1-B/14A evidence tied to its source
  commit. Its clean, networkless Ubuntu 26.04 fixture smoke passed in
  [run #45921465230](https://github.com/Viet281101/qt-maps/actions/runs/45921465230).
  The owner approved a public synthetic runtime preview retained seven days;
  real data and graph stay local for direct handoff to the owner only. That preview upload is pending CI
  notice review; CI retains only text provenance for seven days meanwhile.
  A local real-data stage (1,498,646,440 bytes) passed the
  same installer and networkless Ubuntu 26.04 container check: 9,094 FTS5
  rows, activated national graph, Hanoi–Hai Phong and Thu Dau Mot–Di An routes.
  The direct owner handoff channel is settled; transitive runtime notices remain open before release.
  The ignored internal archive tied to commit `3016987` is 543,496,479 bytes,
  SHA-256 `26cb328c7b670c888e03ab71310acf82f4843d5591e335152db47e612fec3f45`.
  Its full extraction, installer, restart, FTS5 and two real-route smoke passed
  inside networkless Ubuntu 26.04 with capabilities dropped. See
  `docs/packaging/v1-c-preparation-20261127.md`. A clean Desktop/physical-target GUI repeat
  from this exact archive and exact runtime-notice review remain open.
  `docs/packaging/v1-c-runtime-notice-inventory.md` records the current
  blocker: the pyvalhalla SBOM lists 44 components but supplies no per-component
  license fields for 50 bundled native library files. Public binary upload is
  held until this inventory is reconciled.
  The text-only CI run #45926277701 passed. On 2026-11-28, the exact local
  archive also passed offline Ubuntu 26.04 container GUI smoke with llvmpipe:
  national graph selected, Hanoi vector map and 7,294 m/14-maneuver Valhalla
  route visible. See the V1-C preparation report; clean Desktop/physical-target
  acceptance and notices remain open.
- **Blocked — V1-D:** Run locked reference-laptop Release FPS/frame-time,
  process-tree RAM/CPU, orthography observation and 30–60 minute soak only after
  the owner lifts the i915 freeze. Then V1-E reviews all gates and tags v1.0.
  Exact criteria, estimates and dependencies: [#81](https://github.com/Viet281101/qt-maps/issues/81),
  [PR #82](https://github.com/Viet281101/qt-maps/pull/82) (`In Progress`, P1,
  After Demo in Project #6), and `context/feature-specs/v1-linux-release-completion-plan.md`.

## Risks and decisions requiring the owner

1. The i915 render-benchmark freeze remains in effect. The completed matrices
   and direct-GPU visual review do not authorize a new benchmark, soak,
   MapLibre/OpenGL investigation, or GPU configuration change.
2. Orthography is now provisioned from an owner-supplied local Sentinel-2 L2A
   scene. Credentials and raw source data remain outside Git; nationwide or
   higher-resolution coverage still needs a separate decision.
3. Offline Hanoi search is integrated with bounded coverage; any broader data
   coverage or package expansion requires a separate decision.
4. The national graph build fits this host. The owner approved a public
   synthetic runtime preview with seven-day retention and internal real-data
   handoff to the owner only. Final runtime notices and release
   review date still need decisions/checks.

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
