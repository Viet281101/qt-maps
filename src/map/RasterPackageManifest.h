#pragma once

#include <QString>

namespace qtmaps {
namespace map {

struct RasterPackageManifest
{
    QString packageId;
    QString vectorDataVersion;
    QString styleContractVersion;
    QString styleRevision;
    QString sourcePbfSha256;
    QString mbtilesRelativePath;
    qint64 mbtilesSizeBytes = 0;
    int layerSchemaVersion = 0;
    int minimumZoom = 0;
    int maximumZoom = 0;
    int tileCount = 0;

    static bool loadAndValidate(const QString &packagePath, RasterPackageManifest *manifest,
                                QString *errorMessage,
                                const QString &expectedVectorPackagePath = QString());
};

} // namespace map
} // namespace qtmaps
