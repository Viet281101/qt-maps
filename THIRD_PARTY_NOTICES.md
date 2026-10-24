# Third-Party Notices

This file records the third-party components and data currently selected for QtMaps. It is a development notice, not legal advice and not yet a complete release bundle. Before distribution, follow `docs/packaging/linux-dynamic-linking.md` and verify the exact binaries, modules, transitive dependencies, notices, and corresponding-source offer in the packaged artifact.

## OpenStreetMap data

- **Use:** source data for the bounded Hanoi vector package and planned offline map packages.
- **Source:** Geofabrik extracts derived from OpenStreetMap.
- **License:** Open Data Commons Open Database License 1.0 (ODbL 1.0).
- **Required credit:** `© OpenStreetMap contributors` must remain visible on browsable maps, and the application/package must make clear that the data is available under ODbL 1.0.
- **Links:** <https://www.openstreetmap.org/copyright>, <https://opendatacommons.org/licenses/odbl/1-0/>.

Any distributed derived database must retain its source identity, retrieval date, checksum, attribution, and applicable share-alike information in the dataset manifest. Map styling and application code do not replace the data-license obligations.

## MapLibre Native and MapLibre Native Qt

- **Version:** MapLibre Native Qt v3.0.0, signed tag commit `d929c783737120787b43417d9ef05da88da75dfd`.
- **Use:** vector rendering through the Qt Location/QML integration.
- **Core license:** BSD 2-Clause.
- **Qt/QML binding choice for QtMaps:** GNU Lesser General Public License v3.0 (LGPL-3.0-only), selected from the binding's GPL-2.0/GPL-3.0/LGPL-3.0 alternatives.
- **Examples:** MIT; QtMaps does not currently ship MapLibre examples.
- **Links:** <https://github.com/maplibre/maplibre-native-qt/tree/v3.0.0>, <https://github.com/maplibre/maplibre-native-qt/tree/v3.0.0/LICENSES>.

QtMaps must dynamically link the MapLibre/Qt shared objects and preserve the BSD notice plus the selected LGPL-3.0 license text and user rights described in the packaging guide. The exact MapLibre Native transitive notices must be collected from the built v3.0.0 source tree for every release.

## Qt

- **Version:** Qt 5.15.13 baseline; Qt 5.13.2 is comparison-only and is not the planned release runtime.
- **Use:** Core, Gui, Location, Positioning, Qml, Quick, and their required runtime dependencies. Qt Test is build/test-only.
- **License choice:** open-source Qt modules are used under GNU LGPL v3 where that option is available. Each packaged module and its third-party code must be checked against Qt's module-specific license metadata before distribution.
- **Links:** <https://doc.qt.io/archives/qt-5.15/licensing.html>, <https://www.qt.io/development/open-source-lgpl-obligations>.

The release must include prominent Qt/LGPL notice and license text, corresponding source or a valid written offer/instructions, installation information, and a practical way for recipients to replace/relink the LGPL libraries and run the modified combination. Distribution terms must not prohibit the reverse engineering needed for that relinking right.

## Planetiler

- **Version:** v0.10.2.
- **Use:** build-time data-pipeline tool; the jar is stored outside Git and is not part of the planned QtMaps runtime package.
- **License:** Apache License 2.0.
- **Links:** <https://github.com/onthegomap/planetiler/releases/tag/v0.10.2>, <https://github.com/onthegomap/planetiler/blob/v0.10.2/LICENSE>, <https://github.com/onthegomap/planetiler/blob/v0.10.2/NOTICE.md>.

If Planetiler or a bundled launcher is ever redistributed, include its Apache-2.0 license, NOTICE, and notices for bundled dependencies/JRE. Planetiler does not replace the attribution and share-alike requirements of its OSM input data.

## Project-owned resources

The current marker sprite and generated glyph resources used by the Unit 01b probe are project-owned test resources. Their manifest and checksums must travel with any dataset package that references them. No public map tile, proprietary map style, or third-party font asset may be added without a separate source/license review.
