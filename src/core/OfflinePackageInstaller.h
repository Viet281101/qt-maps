#pragma once

#include "OfflinePackageContract.h"

#include <QString>

namespace qtmaps {
namespace core {

class OfflinePackageInstaller final
{
public:
    // Validation uses the existing map, raster, orthography, and search validators.
    static bool validateCandidate(const QString &bundlePath, MapSearchBundleContract *contract,
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
};

} // namespace core
} // namespace qtmaps
