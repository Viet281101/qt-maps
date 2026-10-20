# Unit 01: Environment and Feasibility Spike

Read `AGENTS.md` before starting. Do not install software, download a large dataset, or select a permanent build system until the owner approves the relevant choice.

## Goal

Establish a reproducible Qt 5.13 development environment on the reference laptop, prove the smallest Qt Quick and raster-map render paths, capture an honest initial performance baseline, and produce enough evidence for the owner to decide the primary build system and whether MapLibre Native is feasible with Qt 5.13.

This is a spike, not permission to begin the production map architecture. Its expected duration is one to three four-hour sessions, with the MapLibre investigation time-boxed to one day.

## Design

The spike has four evidence tracks:

1. **Environment:** compare `aqtinstall`, the Qt online installer, and a Qt 5.13 source build for reproducibility, required modules, licensing/account constraints, download/disk cost, expected build time, update behavior, and compatibility with the current Linux distribution. Recommend one approach, then wait for approval before installation.
2. **Toolchain and build:** test Qt 5.13 against GCC 13.3.0 and the system OpenSSL 3 environment. Compare qmake and CMake against the actual Qt package and candidate map integrations. Recommend a primary build system, then record the owner's decision and exact Release build command.
3. **Minimal rendering:** use only the smallest required files to show first a blank Qt Quick 2.13 window, then a minimal raster map. Prefer the installed QtLocation `osm` plugin for this proof if it is actually available. The test view must show errors explicitly and must not imply that offline raster support is complete.
4. **MapLibre feasibility:** identify the exact maintained MapLibre Native repository/version or Qt integration candidate, its license and documented Qt requirements, and test or conclusively trace its Qt 5.13 build path. Record blockers, patch burden, graphics behavior, and a go/no-go recommendation; do not mark MapLibre committed without owner approval.

The spike must inventory the installed QtLocation plugins instead of assuming they exist. At minimum, report whether `osm` and `mapboxgl` are present, loadable, and suitable for the limited proof. The `mapboxgl` plugin and MapLibre Native are separate candidates and must not be treated as interchangeable.

The initial baseline covers the blank QML window and raster-map proof in Release mode. Record FPS and resident memory for the app; record CPU as well if a reliable command is available during the spike. State the sampler, duration, viewport/window size, network state, and whether child processes exist. These measurements are exploratory and do not finalize performance thresholds.

## Implementation

1. Reconfirm the reference hardware, OS details, compiler, CMake, Make, graphics renderer, Mesa version, direct-rendering state, and absence/presence of qmake, Qt, Ninja, Clang, and ccache.
2. Document the three Qt 5.13 installation options in a decision table. Include availability of Qt Quick, QtLocation, and QtPositioning modules. Stop for owner approval before running an installer or package command.
3. After approval, install or provision exactly the selected Qt 5.13 distribution and record its source, version, location strategy, installed modules, and verification commands.
4. Inventory Qt plugins from the provisioned installation. Verify Qt Quick, QtLocation, and QtPositioning imports; list actual QtLocation geoservice plugins and explicitly test `osm` and `mapboxgl` presence/loadability.
5. Probe a minimal Qt 5.13 compilation with GCC 13.3.0. Capture complete failures and minimal reproducible evidence rather than applying undocumented compatibility patches.
6. Probe an HTTPS request appropriate to online raster loading and record Qt SSL build/runtime versions and errors to evaluate the OpenSSL 3 risk. Do not put credentials or private endpoints in logs.
7. Compare qmake and CMake using the provisioned Qt and integration constraints. Present the recommendation and wait for the owner to select the primary build system.
8. Create the smallest project layout using the approved build system and C++ language level. Build and run a blank QML window with Qt Quick 2.13 imports.
9. Add a minimal raster-map proof using an actually available QtLocation plugin. Use a bounded view and respect provider attribution/usage terms; this step does not implement the unit 03 offline cache.
10. Build both proofs in Release mode. Capture baseline FPS/RAM and, where reliable, CPU using a documented fixed observation interval and add the result to `docs/benchmarks/`.
11. Time-box the MapLibre Native Qt 5.13 feasibility investigation to one day. Prefer a minimal build/render proof; if the build cannot reasonably complete, preserve exact version, configuration, error, compatibility documentation, and estimated patch burden as the result.
12. Update `context/architecture.md`, `context/code-standards.md`, and `context/progress-tracker.md` with approved decisions only. Add exact configure, build, test, and run commands; keep rejected candidates and evidence in the spike report.

## Dependencies

- Owner approval of the Qt 5.13 installation method before any installation.
- Owner decision on qmake versus CMake after the comparison evidence is available.
- Owner approval before installing Ninja or ccache with `sudo`.
- Network access for the selected Qt installer and HTTPS check, if approved.
- A small provider-compliant raster request or fixture and an agreed initial demo region/zoom.
- Existing reference environment: GCC/G++ 13.3.0, CMake 3.28.3, GNU Make 4.3, Intel HD Graphics 520, and Mesa 25.2.8.
- No dependency on Valhalla, Planetiler, a full Vietnam PBF, an orthography source, or production-sized map data.

## Verify When Done

- [ ] A decision table compares `aqtinstall`, Qt online installer, and source build, and owner approval of the chosen provisioning method is recorded.
- [ ] `qmake --version` or the selected Qt equivalent proves Qt 5.13, and Qt Quick, QtLocation, and QtPositioning module availability is recorded.
- [ ] The QtLocation plugin inventory states whether `osm` and `mapboxgl` are present and loadable.
- [ ] Qt 5.13 plus GCC 13.3.0 compatibility has a passing minimal compilation or a reproducible blocker with a proposed next decision.
- [ ] Qt 5.13 plus system OpenSSL 3 HTTPS behavior has a passing probe or a reproducible blocker with no hidden fallback.
- [ ] The owner-selected primary build system, C++ level, and exact Release configure/build/test/run commands replace every build-command TBD in context.
- [ ] The Release build displays a blank Qt Quick 2.13 window end to end.
- [ ] The Release build displays a minimal raster map through an actually installed plugin, or a reproducible plugin/provider blocker is documented.
- [ ] A benchmark record reports blank-window and raster-proof FPS/RAM and any reliable CPU result with duration, method, viewport, network state, and process scope.
- [ ] The one-day MapLibre Native investigation records exact source/version, license, Qt requirements, build/render outcome, blockers/patch burden, and a go/no-go recommendation for Qt 5.13.
- [ ] No package or tool was installed without approval, no full production dataset was downloaded, and `context/progress-tracker.md` reflects all evidence and remaining questions.
- [ ] **BUILD PASS** is recorded with the real selected command, or the unit remains incomplete with its exact blocker.
