#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

namespace qtmaps {
namespace map {

struct MapPackageManifest
{
    QString schemaVersion;
    QString dataVersion;
    QString styleContractVersion;
    double west = 0.0;
    double south = 0.0;
    double east = 0.0;
    double north = 0.0;
    int minimumZoom = 0;
    int maximumZoom = 0;
    QString planetilerVersion;
    QString planetilerSha256;
    QString sourcePbfSha256;
    QStringList sourceLayers;
    QString attribution;
    QHash<QString, QString> fileSha256;

    static bool loadAndValidate(const QString &packagePath,
                                MapPackageManifest *manifest,
                                QString *errorMessage);
};

} // namespace map
} // namespace qtmaps
