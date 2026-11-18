#include "OfflinePackageInstaller.h"

#include "map/MapPackageManifest.h"
#include "map/RasterPackageManifest.h"
#include "search/PlaceSearchController.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
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

bool OfflinePackageInstaller::resolveActiveBundle(const QString &activationRoot,
                                                  ActiveBundlePaths *paths, QString *errorMessage)
{
    if (!paths) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Missing bundle paths output");
        return false;
    }
    *paths = ActiveBundlePaths{};
    const QString current = linkPath(activationRoot, QStringLiteral("current"));
    const QString snapshot = QFileInfo(current).canonicalFilePath();
    MapSearchBundleContract contract;
    if (snapshot.isEmpty() ||
        !MapSearchBundleContract::loadAndValidate(snapshot, &contract, errorMessage)) {
        if (snapshot.isEmpty() && errorMessage)
            *errorMessage = QStringLiteral("Active bundle is missing or unreadable");
        return false;
    }
    const QDir root(snapshot);
    paths->vector = root.filePath(contract.vectorPath);
    paths->raster = root.filePath(contract.rasterPath);
    paths->search = root.filePath(contract.searchPath);
    if (!contract.orthographyPath.isEmpty())
        paths->orthography = root.filePath(contract.orthographyPath);
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
    const QString current = linkPath(activationRoot, QStringLiteral("routing-current.json"));
    const QString snapshot = QFileInfo(current).canonicalFilePath();
    RoutingGraphActivationContract contract;
    if (snapshot.isEmpty() || !validateRoutingGraph(snapshot, &contract, errorMessage)) {
        if (snapshot.isEmpty() && errorMessage)
            *errorMessage = QStringLiteral("Active routing contract is missing or unreadable");
        return QString();
    }
    return QDir(QFileInfo(snapshot).canonicalPath()).filePath(contract.graphPath);
}

namespace {
bool readRoutingConfig(const QString &path, QJsonObject *config, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Cannot read routing configuration");
        return false;
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject() ||
        !document.object().value(QStringLiteral("mjolnir")).isObject()) {
        if (error)
            *error = QStringLiteral("Invalid routing configuration");
        return false;
    }
    *config = document.object();
    return true;
}
} // namespace

bool OfflinePackageInstaller::routingConfigSelectsGraph(const QString &configPath,
                                                        const QString &graphPath, QString *error)
{
    QJsonObject config;
    if (!readRoutingConfig(configPath, &config, error))
        return false;
    const QString tileDir = config.value(QStringLiteral("mjolnir"))
                                .toObject()
                                .value(QStringLiteral("tile_dir"))
                                .toString();
    const QString extract = config.value(QStringLiteral("mjolnir"))
                                .toObject()
                                .value(QStringLiteral("tile_extract"))
                                .toString();
    const QString selected = QFileInfo(tileDir).canonicalFilePath();
    const QString expected = QFileInfo(graphPath).canonicalFilePath();
    if (!extract.isEmpty() || selected.isEmpty() || expected.isEmpty() || selected != expected) {
        if (error)
            *error = QStringLiteral("Routing configuration does not select active graph");
        return false;
    }
    return true;
}

bool OfflinePackageInstaller::writeRoutingRuntimeConfig(const QString &source,
                                                        const QString &destination,
                                                        const QString &graphPath, quint16 port,
                                                        QString *error)
{
    QJsonObject config;
    if (!readRoutingConfig(source, &config, error))
        return false;
    const QString listen = config.value(QStringLiteral("httpd"))
                               .toObject()
                               .value(QStringLiteral("service"))
                               .toObject()
                               .value(QStringLiteral("listen"))
                               .toString();
    const QString graph = QFileInfo(graphPath).canonicalFilePath();
    const QString extract = config.value(QStringLiteral("mjolnir"))
                                .toObject()
                                .value(QStringLiteral("tile_extract"))
                                .toString();
    if (!extract.isEmpty() || listen != QStringLiteral("tcp://127.0.0.1:%1").arg(port) ||
        graph.isEmpty() || !QFileInfo(graph).isDir() ||
        QFileInfo(source).absoluteFilePath() == QFileInfo(destination).absoluteFilePath()) {
        if (error)
            *error = QStringLiteral("Unsafe routing configuration or graph path");
        return false;
    }
    QJsonObject mjolnir = config.value(QStringLiteral("mjolnir")).toObject();
    mjolnir.insert(QStringLiteral("tile_dir"), graph);
    config.insert(QStringLiteral("mjolnir"), mjolnir);
    QSaveFile file(destination);
    const QByteArray bytes = QJsonDocument(config).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        if (error)
            *error = QStringLiteral("Cannot write runtime routing configuration");
        return false;
    }
    return routingConfigSelectsGraph(destination, graph, error);
}

} // namespace core
} // namespace qtmaps
