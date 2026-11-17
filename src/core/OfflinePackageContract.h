#pragma once

#include <QString>

namespace qtmaps {
namespace core {
struct MapSearchBundleContract
{
    QString bundleId, vectorPath, rasterPath, searchPath, orthographyPath;
    QString source, retrievedOn, attribution;
    qint64 installedSizeBytes = 0;
    static bool loadAndValidate(const QString &, MapSearchBundleContract *, QString *);
};
struct RoutingGraphActivationContract
{
    QString graphId, graphPath, sourcePbfSha256, attribution;
    qint64 installedSizeBytes = 0;
    static bool loadAndValidate(const QString &, RoutingGraphActivationContract *, QString *);
};
} // namespace core
} // namespace qtmaps
