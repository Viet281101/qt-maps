# Unit 01: Environment and Feasibility Spike

Read `AGENTS.md` before starting. The owner selected Qt 5.13.2 `gcc_64` through `aqtinstall` under `~/Qt` and CMake with Ninja. The approved Qt base installation is complete. Do not install further software or download a large dataset until the owner approves the exact command; print every `sudo apt` command for the owner to run manually.

## Goal

Establish a reproducible Qt 5.13.2 development environment on the reference laptop, validate the committed CMake/Ninja path, prove the smallest Qt Quick and raster-map render paths, capture an honest initial performance baseline, and produce enough evidence to decide whether MapLibre Native is feasible with Qt 5.13.2.

This is a spike, not permission to begin the production map architecture. Its expected duration is one to three four-hour sessions, with the MapLibre investigation time-boxed to one day.

## Design

The spike has four evidence tracks:

1. **Environment:** use `aqtinstall` for Qt 5.13.2 `gcc_64` under `~/Qt`. Before installation, run `aqt list-qt linux desktop --modules 5.13.2 gcc_64`, present the module list, and wait for owner approval of the exact install command.
2. **Toolchain and build:** validate CMake with Ninja against Qt 5.13.2, GCC 13.3.0, and candidate map integrations. Determine the exact Release build command only after it succeeds. Build files use Qt 5 CMake APIs and must not use Qt 6 convenience APIs.
3. **Minimal rendering:** use only the smallest required files to show first a blank Qt Quick 2.13 window, then a minimal raster map. Prefer the installed QtLocation `osm` plugin for this proof if it is actually available. The test view must show errors explicitly and must not imply that offline raster support is complete.
4. **MapLibre feasibility:** identify the exact maintained MapLibre Native repository/version or Qt integration candidate, its license and documented Qt requirements, and test or conclusively trace its Qt 5.13.2 build path. Record blockers, patch burden, graphics behavior, and a go/no-go recommendation; do not mark MapLibre committed without owner approval.

The spike must inventory the installed QtLocation plugins instead of assuming they exist. At minimum, report whether `osm` and `mapboxgl` are present, loadable, and suitable for the limited proof. The `mapboxgl` plugin and MapLibre Native are separate candidates and must not be treated as interchangeable. Use `ldd` after provisioning to identify missing system libraries.

OpenSSL 1.1 versus OpenSSL 3 compatibility must be tested for online HTTPS tile access. Do not describe an incompatibility or compatibility as confirmed before the probe. The offline path uses localhost HTTP, so this TLS question alone does not block offline operation.

The initial baseline covers the blank QML window and raster-map proof in Release mode. Record FPS and resident memory for the app; record CPU as well if a reliable command is available during the spike. State the sampler, duration, viewport/window size, network state, and whether child processes exist. These measurements are exploratory and do not finalize performance thresholds.

## Implementation

1. Reconfirm the reference hardware, OS details, compiler, CMake, Make, graphics renderer, Mesa version, direct-rendering state, and absence/presence of qmake, Qt, Ninja, Clang, and ccache.
2. Confirm `aqtinstall` availability, run `aqt list-qt linux desktop --modules 5.13.2 gcc_64`, and present the complete output as a module table. Do not run the Qt install command yet.
3. Print any required `sudo apt` commands for Ninja, ccache, and later confirmed runtime libraries. The owner executes them manually. After explicit approval, provision Qt 5.13.2 `gcc_64` under `~/Qt` and record the exact command, source, installed modules, and verification commands.
4. Inventory Qt plugins from the provisioned installation. Verify Qt Quick, QtLocation, and QtPositioning imports; list actual QtLocation geoservice plugins and explicitly test `osm` and `mapboxgl` presence/loadability.
5. Probe a minimal Qt 5.13.2 compilation with GCC 13.3.0. Capture complete failures and minimal reproducible evidence rather than applying undocumented compatibility patches.
6. Probe an HTTPS request appropriate to online raster loading and record Qt SSL build/runtime versions and errors to evaluate OpenSSL 1.1 versus OpenSSL 3 behavior. Separately verify that localhost HTTP works for the offline service path. Do not put credentials or private endpoints in logs.
7. Validate the committed CMake/Ninja build path using `find_package(Qt5 ...)`, `CMAKE_AUTOMOC`/`CMAKE_AUTORCC`, `qt5_add_resources`, and `add_executable`; do not use Qt 6 CMake APIs.
8. Create the smallest project layout using CMake/Ninja and the approved C++ language level. Build and run a blank QML window with Qt Quick 2.13 imports.
9. Add a minimal raster-map proof using an actually available QtLocation plugin. Use a bounded view and respect provider attribution/usage terms; this step does not implement the unit 03 offline cache.
10. Build both proofs in Release mode. Capture baseline FPS/RAM and, where reliable, CPU using a documented fixed observation interval and add the result to `docs/benchmarks/`.
11. Time-box the MapLibre Native Qt 5.13.2 feasibility investigation to one day. Prefer a minimal build/render proof; if the build cannot reasonably complete, preserve exact version, configuration, error, compatibility documentation, and estimated patch burden as the result.
12. Update `context/architecture.md`, `context/code-standards.md`, and `context/progress-tracker.md` with approved decisions only. Add exact configure, build, test, and run commands; keep rejected candidates and evidence in the spike report.

## Dependencies

- `aqtinstall` available to perform the required module query; installing the tool itself requires approval if it is absent.
- Owner review of the Qt 5.13.2 module list and explicit approval of the exact Qt install command.
- Owner manually installs the already-approved Ninja and ccache packages from the printed `sudo apt` command.
- Network access for the selected Qt installer and HTTPS check, if approved.
- A small provider-compliant raster request or fixture for Hanoi and its surrounding Red River Delta area, initially bounded to zoom 11-16; zoom 16 remains provisional.
- Existing reference environment: GCC/G++ 13.3.0, CMake 3.28.3, GNU Make 4.3, Intel HD Graphics 520, and Mesa 25.2.8.
- No dependency on Valhalla, Planetiler, a full Vietnam PBF, an orthography source, or production-sized map data.

## Verify When Done

- [x] The complete output of `aqt list-qt linux desktop --modules 5.13.2 gcc_64` was presented before installation, and owner approval of the exact install command was recorded.
- [x] `/home/viet/Qt/5.13.2/gcc_64/bin/qmake --version` proves Qt 5.13.2; Qt Quick, QtLocation, and QtPositioning libraries/QML directories are present.
- [ ] The QtLocation plugin inventory confirms `osm` and `mapboxgl` files are present, but actual plugin loading remains unverified until the minimal QML probe.
- [ ] Qt 5.13.2 plus GCC 13.3.0 compatibility has a passing minimal compilation or a reproducible blocker with a proposed next decision.
- [ ] Qt 5.13.2 OpenSSL 1.1/system OpenSSL 3 HTTPS behavior remains unresolved: a temporary probe returned `supportsSsl: true` with build OpenSSL 1.1.1b and runtime OpenSSL 3.0.13, while warning that `EVP_PKEY_base_id` and `SSL_get_peer_certificate` could not be resolved. A real HTTPS request and localhost HTTP remain to be tested.
- [ ] CMake/Ninja is validated, the selected C++ level is recorded, and exact Release configure/build/test/run commands replace every build-command TBD in context.
- [ ] The Release build displays a blank Qt Quick 2.13 window end to end.
- [ ] The Release build displays a minimal raster map through an actually installed plugin, or a reproducible plugin/provider blocker is documented.
- [ ] A benchmark record reports blank-window and raster-proof FPS/RAM and any reliable CPU result with duration, method, viewport, network state, and process scope.
- [ ] The one-day MapLibre Native investigation records exact source/version, license, Qt requirements, build/render outcome, blockers/patch burden, and a go/no-go recommendation for Qt 5.13.2.
- [ ] No package or tool was installed without approval, no full production dataset was downloaded, and `context/progress-tracker.md` reflects all evidence and remaining questions.
- [ ] **BUILD PASS** is recorded with the real selected command, or the unit remains incomplete with its exact blocker.

### Verified Environment Evidence

- Qt was installed under `/home/viet/Qt/5.13.2/gcc_64` without changing `PATH` or shell startup files; `/home/viet/Qt` occupies 563 MiB according to `du -sh`.
- Geoservices present: `esri`, `itemsoverlay`, `mapbox`, `mapboxgl`, `nokia`, and `osm`.
- Position plugins present: `geoclue`, `geoclue2`, `positionpoll`, and `serialnmea`.
- `qml/QtLocation` and `qml/QtPositioning` each contain a declarative plugin, `plugins.qmltypes`, and `qmldir`.
- `ldd` reported no missing dependency for `libQt5Core`, `libQt5Quick`, `libQt5Location`, `libQt5Positioning`, or `plugins/platforms/libqxcb.so`; therefore no runtime-library `sudo apt install` command is currently justified by `ldd`.
- A temporary SSL probe outside the repository compiled with GCC 13.3.0 and exited normally. This confirms a small Qt Core/Network compilation only, not Qt Quick/QtLocation compatibility or HTTPS correctness.
