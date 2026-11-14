#pragma once

#include <QString>

namespace qtmaps {
namespace map {
namespace contract {

constexpr int kLayerSchemaVersion = 2;
constexpr double kWest = 105.78;
constexpr double kSouth = 20.97;
constexpr double kEast = 105.90;
constexpr double kNorth = 21.10;
constexpr int kVectorMinimumZoom = 12;
constexpr int kVectorMaximumZoom = 15;
constexpr int kRasterMinimumZoom = 12;
constexpr int kRasterMaximumZoom = 16;
// Source tiles stop at the values above. The validated MapLibre renderer overzooms them
// locally through z18 so interaction and navigation keep one display-camera contract.
constexpr int kDisplayMinimumZoom = 12;
constexpr int kDisplayMaximumZoom = 18;
constexpr int kRasterTileCount = 847;

inline QString mapPackageSchemaVersion() { return QStringLiteral("qtmaps-map-package-v3"); }

inline QString styleContractVersion() { return QStringLiteral("qtmaps-style-v3"); }

inline QString styleRevision() { return QStringLiteral("qtmaps-hanoi-v3"); }

inline QString glyphRevision() { return QStringLiteral("noto-sans-regular-v1"); }

inline QString attribution() { return QStringLiteral("© OpenStreetMap contributors, ODbL 1.0"); }

} // namespace contract
} // namespace map
} // namespace qtmaps
