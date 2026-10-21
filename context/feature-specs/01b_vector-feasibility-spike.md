# Unit 01b: Local Vector and MapLibre Native Feasibility Spike

Read `AGENTS.md`, the accepted environment evidence in unit 01, and the completed unit 02 instrumentation evidence before starting. This is a bounded investigation, not permission to install software, download data, or commit generated datasets. Present every proposed download/install command, expected size, license, destination, and cleanup plan to the owner and wait for approval.

## Goal

Produce evidence-based answers to four questions on the reference Intel HD Graphics 520 laptop:

1. Can Qt 5.13.2's installed `mapboxgl` plugin render project-generated local vector tiles with a local style, glyphs, and sprites over HTTP loopback?
2. Can QML `QtLocation.Map` set and retain bearing and tilt when the active backend is the Qt 5.13.2 `mapboxgl` plugin?
3. What rendered-frame FPS, total RSS, and total CPU are observed during a repeatable pan/zoom scenario over a dense Hanoi urban area?
4. Under what measured conditions should the project stop investing in the installed plugin and begin a separately estimated MapLibre Native Qt integration/build?

The result is a go/no-go recommendation with explicit unknowns. Loading a background-only style, constructing a provider, or finding a plugin file is not proof of local vector rendering.

## Design

Use the unit 02 rendered-frame and process-tree instrumentation so this spike does not invent a second benchmark method. Run the same automated pan/zoom sequence three times in Release mode and report the median, with viewport, renderer, power, CPU governor, temperatures, dataset identity, zoom range, render loop, and process scope.

The verified starting evidence is deliberately narrow:

- Unit 01 proved that `mapboxgl` loads one HTTP-loopback background-only style and reports no map error. No vector source, glyph, or sprite was involved.
- [QtLocation source at tag `v5.13.2`](https://github.com/qt/qtlocation/blob/v5.13.2/src/plugins/geoservices/mapboxgl/qgeomappingmanagerenginemapboxgl.cpp) verifies the plugin parameter `mapboxgl.mapping.additional_style_urls` and also contains parameters for access token, cache memory/directory/size, FBO use, and item insertion. It does not by itself prove that the proposed local style resources render correctly.
- The installed Qt 5.13.2 `QtLocation/plugins.qmltypes` declares writable `Map.bearing` and `Map.tilt` properties plus minimum/maximum tilt. Actual behavior and limits with this plugin remain open and require a render test.
- The approved real source is the [Geofabrik Vietnam `.osm.pbf`](https://download.geofabrik.de/asia/vietnam.html), fetched directly as a source dataset with `curl` after owner approval. This is distinct from downloading rendered/vector tiles from a public tile server, which remains prohibited.
- [Planetiler](https://github.com/onthegomap/planetiler) is the proposed MVT/MBTiles generator, but no version or distribution method is selected. Reconfirm its license, exact release, Java/runtime requirement, disk/RAM estimates, and output/style schema immediately before requesting approval.

Keep every endpoint loopback-only. Generated PBF derivatives, MBTiles, unpacked tiles, caches, glyphs, sprites, logs, and benchmark CSV files stay outside Git. Commit only reproducible configuration/scripts that contain no third-party binary or generated data.

The following remain open until the spike research or experiment resolves them:

- the exact Planetiler release/distribution and whether that choice requires a separately installed Java runtime;
- how to constrain generation to a small dense Hanoi region without accidentally processing or emitting the full national high-zoom dataset;
- the vector schema/profile and matching style version;
- the exact local URL layout and Qt 5.13.2 compatibility rules for vector sources, glyph templates, sprites, and fonts;
- whether a static HTTP server is sufficient or an approved MBTiles-aware loopback service is required;
- the effective bearing/tilt ranges and gesture behavior of the installed plugin;
- the MapLibre Native repository/tag, license, Qt integration boundary, Qt 5.13.2 support, compiler/dependency requirements, patch burden, build duration, and binary/runtime footprint.

Recommend a separate MapLibre Native build investigation only if evidence shows one or more material blockers: local vector/style resources cannot render correctly; bearing/tilt required by navigation is absent or unreliable; repeatable dense-urban performance misses an owner-approved threshold; the installed backend crashes or has unacceptable correctness defects; or its maintenance/security/deployment constraints are worse than the estimated native integration. A switch is not justified merely because MapLibre Native is newer.

## Implementation

1. Reconfirm the installed `mapboxgl` plugin and preserve the unit 01 background-style control result.
2. Inspect QtLocation `v5.13.2` source and the installed QML type metadata. Record only parameter/property names that are directly verified; keep style-source/glyph/sprite details open until confirmed.
3. Select and record exact candidate versions/licenses for Planetiler, a tile-serving approach, the vector schema/style, font/glyph inputs, sprite inputs, and MapLibre Native. Do not install or download them yet.
4. Prepare an approval table containing every command that would download the Geofabrik Vietnam PBF, tooling, styles, fonts, or sprites; include source URL, expected size, license/attribution, destination, checksum method, and cleanup command. Never download from a public tile endpoint.
5. Define the smallest reproducible dense-Hanoi generation boundary and zoom range. Prove from the selected tool's documentation or dry-run metadata that the boundary affects emitted work as intended before a large generation run.
6. After approval, download the Geofabrik PBF with `curl`, verify its checksum/metadata, and generate a bounded MVT dataset plus compatible local style resources. Serve only on `127.0.0.1` and record request logs.
7. Render the local vector map with the installed Qt 5.13.2 `mapboxgl` plugin. Verify visible roads/buildings/labels, successful vector/glyph/sprite requests, attribution, pan/zoom, missing-resource behavior, and absence of unintended external requests.
8. Programmatically set bearing and tilt through `QtLocation.Map`, then verify requested versus observed values, visual camera change, supported limits, reset-to-north behavior, and interaction with pan/zoom.
9. Run the unit 02 automated dense-urban pan/zoom scenario three times in Release mode. Report medians for active-motion FPS, 1% low, total RSS, and total CPU; retain run-level evidence and environmental metadata.
10. Time-box a source/documentation investigation of MapLibre Native. Produce a separate build/integration estimate covering repository/tag, license, toolchain/dependencies, Qt bridge work, patches, build time, binary size, runtime assets, and test burden. Do not build it without separate owner approval.
11. Compare the installed plugin evidence with the explicit switch criteria, write a cautious recommendation, and update architecture/progress only after owner review.

## Dependencies

- Unit 01 environment portion accepted and merged to `main`.
- Unit 02 completed sufficiently to provide real rendered-frame FPS, 1% low, process-tree RSS/CPU CSV logging, and the repeatable three-run pan/zoom scenario.
- Owner approval before any dataset/tool/style/font/sprite download or software installation.
- A selected Geofabrik Vietnam `.osm.pbf` snapshot with recorded URL, date, size, checksum, and ODbL attribution obligations; it has not yet been downloaded.
- An owner-approved Planetiler distribution/version and runtime plan; Java requirements are unresolved until that exact choice is made.
- An owner-approved bounded-generation method, vector schema/style, font/glyph source, sprite source, and loopback tile-serving method.
- Existing Qt 5.13.2 `gcc_64`, CMake/Ninja Release path, Intel HD Graphics 520/Mesa environment, and the no-public-tile invariant.

Items requiring approval must be listed first; this spec authorizes no `curl`, package-manager, Java/runtime, Planetiler, tile-server, style, font, sprite, MapLibre source, or large data action.

## Verify When Done

- [ ] Exact versions, licenses, checksums, sizes, destinations, and approval records exist for every downloaded or installed input/tool.
- [ ] Network logs prove the test uses only approved source-data downloads and HTTP loopback at runtime, with no request to a public tile server.
- [ ] A reproducible bounded Hanoi dataset-generation command and measured generation time, peak RSS, disk use, coverage, schema, and zoom range are recorded.
- [ ] The installed Qt 5.13.2 `mapboxgl` plugin visibly renders local vector geometry and labels from the matching style, with successful local vector/glyph/sprite requests; otherwise the exact blocker is preserved.
- [ ] Bearing and tilt tests record requested/observed values, supported limits, reset behavior, screenshots or equivalent render evidence, and any backend-specific defect.
- [ ] The automated dense-urban pan/zoom scenario runs three times in Release mode and reports median active-motion FPS, 1% low, total RSS, and total CPU with complete environment metadata.
- [ ] The result distinguishes Qt's installed `mapboxgl` plugin from MapLibre Native and makes no equivalence claim without evidence.
- [ ] The MapLibre Native estimate identifies repository/tag, license, toolchain, Qt 5.13.2 integration route, patch burden, build time, footprint, and verification work without performing an unapproved build.
- [ ] The recommendation evaluates every switch criterion and labels all unresolved facts as open questions.
- [ ] Generated/downloaded datasets and third-party artifacts remain outside Git; attribution and cleanup are documented.
- [ ] `context/progress-tracker.md` and architecture decision statuses reflect the owner-reviewed outcome.
- [ ] **BUILD PASS** is recorded with the validated Qt 5.13 CMake/Ninja Release command.
