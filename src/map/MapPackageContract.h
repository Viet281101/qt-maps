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
constexpr int kRasterTileCount = 847;

inline QString mapPackageSchemaVersion() { return QStringLiteral("qtmaps-map-package-v2"); }

inline QString styleContractVersion() { return QStringLiteral("qtmaps-style-v2"); }

inline QString styleRevision() { return QStringLiteral("qtmaps-hanoi-v2"); }

inline QString attribution() { return QStringLiteral("© OpenStreetMap contributors, ODbL 1.0"); }

} // namespace contract
} // namespace map
} // namespace qtmaps
