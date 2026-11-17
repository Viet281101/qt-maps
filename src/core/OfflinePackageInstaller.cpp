#include "OfflinePackageInstaller.h"

#include "map/MapPackageManifest.h"
#include "map/RasterPackageManifest.h"
#include "search/PlaceSearchController.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>
#include <QUuid>

namespace qtmaps {
namespace core {
namespace {
QString linkPath(const QString &root, const QString &name) { return QDir(root).filePath(name); }
bool replaceLink(const QString &target, const QString &path, QString *error)
{
    QFile::remove(path);
    if (!QFile::link(target, path)) {
        if (error)
            *error = QStringLiteral("Cannot create local activation pointer: %1").arg(path);
        return false;
    }
    return true;
}
} // namespace

bool OfflinePackageInstaller::validateCandidate(const QString &bundlePath,
                                                MapSearchBundleContract *contract,
                                                QString *errorMessage)
{
    MapSearchBundleContract parsed;
    if (!MapSearchBundleContract::loadAndValidate(bundlePath, &parsed, errorMessage))
        return false;
    const QDir root(QFileInfo(bundlePath).canonicalFilePath());
    QString error;
    map::MapPackageManifest vector;
    map::RasterPackageManifest raster;
    if (!map::MapPackageManifest::loadAndValidate(root.filePath(parsed.vectorPath), &vector,
                                                  &error) ||
        !map::RasterPackageManifest::loadAndValidate(root.filePath(parsed.rasterPath), &raster,
                                                     &error, root.filePath(parsed.vectorPath)) ||
        !search::PlaceSearchController::validatePackage(root.filePath(parsed.searchPath), &error)) {
        if (errorMessage)
            *errorMessage = error;
        return false;
    }
    map::RasterPackageManifest orthography;
    if (!parsed.orthographyPath.isEmpty() &&
        !map::RasterPackageManifest::loadAndValidateOrthography(
            root.filePath(parsed.orthographyPath), &orthography, &error)) {
        if (errorMessage)
            *errorMessage = error;
        return false;
    }
    *contract = parsed;
    return true;
}

bool OfflinePackageInstaller::activateValidatedBundle(const QString &bundlePath,
                                                      const MapSearchBundleContract &contract,
                                                      const QString &activationRoot,
                                                      QString *errorMessage)
{
    const QString target = QFileInfo(bundlePath).canonicalFilePath();
    if (target.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Bundle directory is missing: %1").arg(bundlePath);
        return false;
    }
    QDir root;
    if (!root.mkpath(activationRoot)) {
        if (errorMessage)
            *errorMessage =
                QStringLiteral("Cannot create activation directory: %1").arg(activationRoot);
        return false;
    }
    const QStorageInfo storage(activationRoot);
    if (storage.isValid() && storage.bytesAvailable() < contract.installedSizeBytes) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Insufficient storage for bundle activation");
        return false;
    }
    const QString current = linkPath(activationRoot, QStringLiteral("current"));
    const QString previous = linkPath(activationRoot, QStringLiteral("previous"));
    const QString staged =
        linkPath(activationRoot,
                 QStringLiteral(".staged-") + QUuid::createUuid().toString(QUuid::WithoutBraces));
    if (!replaceLink(target, staged, errorMessage))
        return false;
    QFile::remove(previous);
    if (QFileInfo::exists(current) && !QFile::rename(current, previous)) {
        QFile::remove(staged);
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot retain previous active bundle");
        return false;
    }
    if (!QFile::rename(staged, current)) {
        if (QFileInfo::exists(previous))
            QFile::rename(previous, current);
        QFile::remove(staged);
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot activate staged bundle");
        return false;
    }
    return true;
}

bool OfflinePackageInstaller::rollback(const QString &activationRoot, QString *errorMessage)
{
    const QString current = linkPath(activationRoot, QStringLiteral("current"));
    const QString previous = linkPath(activationRoot, QStringLiteral("previous"));
    if (!QFileInfo::exists(previous)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("No previous bundle is available for rollback");
        return false;
    }
    const QString staged = linkPath(activationRoot, QStringLiteral(".rollback-"));
    QFile::remove(staged);
    if (QFileInfo::exists(current) && !QFile::rename(current, staged)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot stage active bundle for rollback");
        return false;
    }
    if (!QFile::rename(previous, current)) {
        if (QFileInfo::exists(staged))
            QFile::rename(staged, current);
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot restore previous bundle");
        return false;
    }
    QFile::remove(previous);
    if (QFileInfo::exists(staged))
        QFile::rename(staged, previous);
    return true;
}

bool OfflinePackageInstaller::validateRoutingGraph(const QString &contractPath,
                                                   RoutingGraphActivationContract *contract,
                                                   QString *errorMessage)
{
    RoutingGraphActivationContract parsed;
    if (!RoutingGraphActivationContract::loadAndValidate(contractPath, &parsed, errorMessage))
        return false;
    const QDir contractDirectory(QFileInfo(contractPath).canonicalPath());
    const QFileInfo graph(contractDirectory.filePath(parsed.graphPath));
    if (!graph.isDir() || QDir(graph.canonicalFilePath())
                              .entryList(QDir::NoDotAndDotDot | QDir::AllEntries)
                              .isEmpty()) {
        if (errorMessage)
            *errorMessage =
                QStringLiteral("Routing graph is missing or empty: %1").arg(graph.filePath());
        return false;
    }
    *contract = parsed;
    return true;
}

bool OfflinePackageInstaller::activateValidatedRoutingGraph(
    const QString &contractPath, const RoutingGraphActivationContract &contract,
    const QString &activationRoot, QString *errorMessage)
{
    Q_UNUSED(contract)
    const QString target = QFileInfo(contractPath).canonicalFilePath();
    if (target.isEmpty()) {
        if (errorMessage)
            *errorMessage =
                QStringLiteral("Routing graph contract is missing: %1").arg(contractPath);
        return false;
    }
    QDir root;
    if (!root.mkpath(activationRoot)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot create routing activation directory: %1")
                                .arg(activationRoot);
        return false;
    }
    const QString current = linkPath(activationRoot, QStringLiteral("routing-current.json"));
    const QString previous = linkPath(activationRoot, QStringLiteral("routing-previous.json"));
    const QString staged = linkPath(activationRoot, QStringLiteral(".routing-staged-")) +
                           QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!replaceLink(target, staged, errorMessage))
        return false;
    QFile::remove(previous);
    if (QFileInfo::exists(current) && !QFile::rename(current, previous)) {
        QFile::remove(staged);
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot retain previous routing graph");
        return false;
    }
    if (!QFile::rename(staged, current)) {
        if (QFileInfo::exists(previous))
            QFile::rename(previous, current);
        QFile::remove(staged);
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot activate staged routing graph");
        return false;
    }
    return true;
}

bool OfflinePackageInstaller::rollbackRoutingGraph(const QString &activationRoot,
                                                   QString *errorMessage)
{
    const QString current = linkPath(activationRoot, QStringLiteral("routing-current.json"));
    const QString previous = linkPath(activationRoot, QStringLiteral("routing-previous.json"));
    if (!QFileInfo::exists(previous)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("No previous routing graph is available for rollback");
        return false;
    }
    const QString staged = linkPath(activationRoot, QStringLiteral(".routing-rollback"));
    QFile::remove(staged);
    if (QFileInfo::exists(current) && !QFile::rename(current, staged)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot stage active routing graph for rollback");
        return false;
    }
    if (!QFile::rename(previous, current)) {
        if (QFileInfo::exists(staged))
            QFile::rename(staged, current);
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot restore previous routing graph");
        return false;
    }
    QFile::remove(previous);
    if (QFileInfo::exists(staged))
        QFile::rename(staged, previous);
    return true;
}

QString OfflinePackageInstaller::activeRoutingGraphPath(const QString &activationRoot,
                                                        QString *errorMessage)
{
    const QString contractPath = linkPath(activationRoot, QStringLiteral("routing-current.json"));
    RoutingGraphActivationContract contract;
    if (!validateRoutingGraph(contractPath, &contract, errorMessage))
        return QString();
    return QDir(QFileInfo(contractPath).canonicalPath()).filePath(contract.graphPath);
}

} // namespace core
} // namespace qtmaps
