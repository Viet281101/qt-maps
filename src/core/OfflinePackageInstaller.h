#pragma once

#include "OfflinePackageContract.h"

#include <QString>

namespace qtmaps {
namespace core {

struct ActiveBundlePaths
{
    QString vector, raster, search, orthography;
};

class OfflinePackageInstaller final
{
public:
    // Validation uses the existing map, raster, orthography, and search validators.
    static bool validateCandidate(const QString &bundlePath, MapSearchBundleContract *contract,
                                  QString *errorMessage);
    // Snapshot and validate bundle metadata; consumers validate each component.
    static bool resolveActiveBundle(const QString &activationRoot, ActiveBundlePaths *paths,
                                    QString *errorMessage);
    // Activation only switches local pointers after a validated candidate is supplied.
    static bool activateValidatedBundle(const QString &bundlePath,
                                        const MapSearchBundleContract &contract,
                                        const QString &activationRoot, QString *errorMessage);
    static bool rollback(const QString &activationRoot, QString *errorMessage);

    static bool validateRoutingGraph(const QString &contractPath,
                                     RoutingGraphActivationContract *contract,
                                     QString *errorMessage);
    static bool activateValidatedRoutingGraph(const QString &contractPath,
                                              const RoutingGraphActivationContract &contract,
                                              const QString &activationRoot, QString *errorMessage);
    static bool rollbackRoutingGraph(const QString &activationRoot, QString *errorMessage);
    static QString activeRoutingGraphPath(const QString &activationRoot, QString *errorMessage);
    // The source config is never changed; only a private runtime copy is written.
    static bool writeRoutingRuntimeConfig(const QString &source, const QString &destination,
                                          const QString &graphPath, quint16 port,
                                          QString *errorMessage);
    static bool routingConfigSelectsGraph(const QString &configPath, const QString &graphPath,
                                          QString *errorMessage);
};

} // namespace core
} // namespace qtmaps
