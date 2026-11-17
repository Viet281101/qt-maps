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
- Unit 11 (#64) is implemented and in [PR #66](https://github.com/Viet281101/qt-maps/pull/66): its readiness controller validates vector,
  raster, search, and orthography packages off the UI thread; shows a compact
  settings/readiness panel; and persists the Vietnamese/English preference.
  The PR is review-ready and unmerged; Issue #64 and the PR remain `In Progress`
  on Project #6 until the owner accepts the squash merge.
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

- [Issue #64](https://github.com/Viet281101/qt-maps/issues/64) and
  [PR #66](https://github.com/Viet281101/qt-maps/pull/66) are P1, `In Progress`,
  `After Demo` on Project #6. The PR is open and review-ready, with no owner
  review or merge yet. The follow-up review fixes are verified locally on the branch.
- Its plan in `context/feature-specs/unit-11-offline-package-readiness-plan.md`
  reuses current validators to report required/optional package readiness, adds
  a Vietnamese/English translation base and persisted language setting, and
  consolidates idle map status controls without data downloads, new
  dependencies, benchmarks, or packaging work.
- The current branch adds a bounded asynchronous readiness controller using the
  existing package validators, including the search-package contract, with
  normalized ready/missing/invalid results. Focused tests cover missing,
  valid, and corrupt search packages; the Qt 5.15.18 Release suite passed
  23/23 on 2026-11-15. Production-QML visual review also captured all vector,
  raster, and orthography flows at 900x580, 1366x768, and 1920x1080.
- The settings action remains at the top-right with a hint only on pointer
  hover. The layer selector now sits at the bottom-right directly above OSM
  attribution and opens upward; degraded/error status stacks above the closed
  selector or open menu. Search results yield while the menu is open. Normal
  loopback routing still shows no map-level pill.
- `qtmaps_app_preferences_tests` now verifies the default Vietnamese choice,
  persisted English choice, signal emission, and invalid-language fallback in
  an isolated settings scope.
- The English catalog now includes controller-origin status text at the QML
  boundary and currently has 178/178 finished entries. The Release build
  compiles its ignored `.qm` from the source-controlled `.ts` using the
  available Qt `lrelease` tool. A translation regression test caught and fixed
  the English-to-Vietnamese switch retaining the old translator.
  `QQmlApplicationEngine::retranslate()` is verified in both directions by a
  live QML binding test.
- The Settings UI test opens with the keyboard action, dismisses on Escape,
  and verifies that the panel remains within the 900x580 window. Missing and
  invalid packages receive concise recovery guidance. The readiness timeout
  test verifies that a late result cannot replace the timeout state.
- The worker has an independent lifetime and no UI-thread shutdown wait when
  validation stalls. Search readiness now opens the SQLite database read-only
  and probes its FTS schema after validating the manifest/checksum, matching
  the runtime open path. Recovery text distinguishes search fixture fallback,
  unavailable orthography, and missing raster. A production-QML capture of the open Settings panel at
  900x580 exposed a translucent layer label behind it; the panel now uses an
  opaque theme surface.
- **BUILD PASS** on Qt 5.15.18 Release (2026-11-16): `make BUILD_DIR=build-qt515
  check` passed 24/24 tests and `git diff --check`. The production QML visual
  review captured 17 frames including Settings at 900x580, vector/raster, and
  orthography at supported viewports. The open Settings panel is opaque, fully
  inside the window, and leaves attribution readable. This is functional
  evidence; no new FPS/RAM/CPU measurement was run under the active i915 freeze.
- Follow-up review fix verification (2026-11-16): Qt 5.15.18 Release
  `make check` passed 24/24 tests including schema-corruption and live
  runtime-status retranslation coverage. The 900x580 production QML capture
  confirms the icon-only Settings control and the layer selector above
  attribution. The new open-layer capture confirms its upward menu and visible
  status pill have no overlap at the minimum viewport. The 18-frame visual
  review also passed at the larger supported viewports.
- Route-contrast follow-up (2026-11-17): a production Valhalla route exposed
  that MapLibre painted the white outline over the blue route despite their
  QML `z` values. Hiding the outline revealed the intact blue route; reversing
  the two polyline declarations restored the blue stroke with its white edge
  on the real 7.3 km route. The visual-review flow now also captures a route
  after recalculation. Qt 5.15.18 Release tests passed 24/24, and 19 functional
  captures passed across vector, raster, and orthography on 2026-11-17. The
  isolated Release review gate also passed 24/24 tests with a clean worktree.
- Second review follow-up (2026-11-17): repeated production starts showed the
  Valhalla route could still appear white, so declaration order alone was not
  reliable. A two-blue-stroke mitigation stayed readable but varied in width
  and shade; the final route now uses one blue stroke to remove the ordering
  dependency. The English prompt also translates origin/destination values.
  The final Qt 5.15.18 Release suite passed 24/24; five consecutive real
  Valhalla starts produced the same blue route footprint. The 19-frame visual
  review passed across vector, raster, and orthography; the blue route remains
  clear on both light maps and the licensed imagery. The English production UI
  shows "choose origin". Isolated review verification remains pending.
- The isolated `make REVIEW_BUILD_DIR=build-review-qt515 review` gate passed
  24/24 tests with a clean worktree on 2026-11-16.

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
3. **#64 — readiness and preferences (P1, review-ready).** PR #66 is open for
   owner review. Keep Issue #64 and the Project items `In Progress` until
   squash merge. The Unit 11 plan retains its no-download, no-new-dependency,
   no-benchmark boundary.
4. **Unit 12 — local package delivery (proposed, [#65](https://github.com/Viet281101/qt-maps/issues/65)).** The decision-first plan in
   `context/feature-specs/unit-12-local-package-delivery-plan.md` awaits the
   owner's choice of distribution, licensing, storage, and update policy.
   The issue is `Todo`, P2, `After Demo` in Project #6. Implementation is not
   authorized by this planning document.

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
