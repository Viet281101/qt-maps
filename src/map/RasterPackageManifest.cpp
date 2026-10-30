#include "RasterPackageManifest.h"

#include "MapPackageContract.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace qtmaps {
namespace map {
namespace {

QString sha256(const QString &path, QString *errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot read raster package file %1: %2")
                                .arg(path, file.errorString());
        }
        return QString();
    }
    QCryptographicHash digest(QCryptographicHash::Sha256);
    if (!digest.addData(&file)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot hash raster package file: %1").arg(path);
        }
        return QString();
    }
    return QString::fromLatin1(digest.result().toHex());
}

bool isSafeRelativePath(const QString &path)
{
    return !path.isEmpty() && !QDir::isAbsolutePath(path) && QDir::cleanPath(path) == path &&
           path != QStringLiteral("..") && !path.startsWith(QStringLiteral("../"));
}

bool reject(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
    return false;
}

} // namespace

bool RasterPackageManifest::loadAndValidate(const QString &packagePath,
                                            RasterPackageManifest *manifest,
                                            QString *errorMessage)
{
    if (!manifest) {
        return reject(errorMessage, QStringLiteral("Raster manifest output pointer is null"));
    }
    const QFileInfo rootInfo(packagePath);
    if (!rootInfo.exists() || !rootInfo.isDir()) {
        return reject(errorMessage,
                      QStringLiteral("Raster package directory is missing: %1").arg(packagePath));
    }
    const QDir root(rootInfo.canonicalFilePath());
    QFile file(root.filePath(QStringLiteral("manifest.json")));
    if (!file.open(QIODevice::ReadOnly)) {
        return reject(errorMessage,
                      QStringLiteral("Raster package manifest is missing or unreadable: %1")
                          .arg(file.errorString()));
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (!document.isObject()) {
        return reject(errorMessage,
                      QStringLiteral("Invalid raster package manifest JSON: %1")
                          .arg(parseError.errorString()));
    }

    const QJsonObject object = document.object();
    const QJsonObject vector = object.value(QStringLiteral("source_vector_package")).toObject();
    const QJsonObject pbf = object.value(QStringLiteral("source_pbf")).toObject();
    const QJsonObject tiles = object.value(QStringLiteral("tiles")).toObject();
    const QJsonObject renderer = object.value(QStringLiteral("renderer")).toObject();
    const QJsonArray bounds = object.value(QStringLiteral("bounds")).toArray();
    const QRegularExpression digestPattern(QStringLiteral("^[0-9a-f]{64}$"));

    RasterPackageManifest parsed;
    parsed.packageId = object.value(QStringLiteral("package_id")).toString();
    parsed.vectorDataVersion = vector.value(QStringLiteral("data_version")).toString();
    parsed.styleContractVersion =
        vector.value(QStringLiteral("style_contract_version")).toString();
    parsed.styleRevision = vector.value(QStringLiteral("style_revision")).toString();
    parsed.layerSchemaVersion = vector.value(QStringLiteral("layer_schema_version")).toInt(-1);
    parsed.sourcePbfSha256 = pbf.value(QStringLiteral("sha256")).toString();
    parsed.minimumZoom = object.value(QStringLiteral("min_zoom")).toInt(-1);
    parsed.maximumZoom = object.value(QStringLiteral("max_zoom")).toInt(-1);
    parsed.tileCount = tiles.value(QStringLiteral("count")).toInt(-1);
    parsed.mbtilesRelativePath = tiles.value(QStringLiteral("path")).toString();
    parsed.mbtilesSizeBytes = tiles.value(QStringLiteral("size_bytes")).toVariant().toLongLong();

    if (object.value(QStringLiteral("schema_version")).toString() !=
        QStringLiteral("qtmaps-raster-package-v1")) {
        return reject(errorMessage, QStringLiteral("Unsupported raster package schema"));
    }
    if (vector.value(QStringLiteral("package_schema_version")).toString() !=
            contract::mapPackageSchemaVersion() ||
        parsed.styleContractVersion != contract::styleContractVersion() ||
        parsed.styleRevision != contract::styleRevision() ||
        parsed.layerSchemaVersion != contract::kLayerSchemaVersion) {
        return reject(errorMessage,
                      QStringLiteral("Raster style/package contract does not match this build"));
    }
    if (parsed.packageId.isEmpty() || parsed.vectorDataVersion.isEmpty() ||
        !digestPattern.match(vector.value(QStringLiteral("manifest_sha256")).toString())
             .hasMatch() ||
        !digestPattern.match(parsed.sourcePbfSha256).hasMatch()) {
        return reject(errorMessage, QStringLiteral("Raster source identity is incomplete"));
    }
    if (bounds.size() != 4 || bounds.at(0).toDouble() != contract::kWest ||
        bounds.at(1).toDouble() != contract::kSouth ||
        bounds.at(2).toDouble() != contract::kEast ||
        bounds.at(3).toDouble() != contract::kNorth ||
        parsed.minimumZoom != contract::kRasterMinimumZoom ||
        parsed.maximumZoom != contract::kRasterMaximumZoom ||
        parsed.tileCount != contract::kRasterTileCount) {
        return reject(errorMessage, QStringLiteral("Raster coverage, zoom, or tile count mismatch"));
    }
    if (object.value(QStringLiteral("attribution")).toString() != contract::attribution()) {
        return reject(errorMessage, QStringLiteral("Raster attribution is missing or incompatible"));
    }
    if (renderer.value(QStringLiteral("name")).toString() !=
            QStringLiteral("MapLibre Native Qt") ||
        renderer.value(QStringLiteral("version")).toString() != QStringLiteral("3.0.0") ||
        tiles.value(QStringLiteral("format")).toString() != QStringLiteral("png") ||
        tiles.value(QStringLiteral("tile_size")).toInt(-1) != 256 ||
        !isSafeRelativePath(parsed.mbtilesRelativePath) || parsed.mbtilesSizeBytes <= 0 ||
        !digestPattern.match(tiles.value(QStringLiteral("sha256")).toString()).hasMatch()) {
        return reject(errorMessage, QStringLiteral("Raster tile metadata is incompatible"));
    }

    const QString mbtilesPath = root.filePath(parsed.mbtilesRelativePath);
    const QFileInfo mbtilesInfo(mbtilesPath);
    if (!mbtilesInfo.isFile() || mbtilesInfo.size() != parsed.mbtilesSizeBytes) {
        return reject(errorMessage, QStringLiteral("Raster MBTiles size mismatch"));
    }
    QString hashError;
    const QString actualSha256 = sha256(mbtilesPath, &hashError);
    if (actualSha256.isEmpty()) {
        return reject(errorMessage, hashError);
    }
    if (actualSha256 != tiles.value(QStringLiteral("sha256")).toString()) {
        return reject(errorMessage, QStringLiteral("Raster MBTiles checksum mismatch"));
    }

    *manifest = parsed;
    return true;
}

} // namespace map
} // namespace qtmaps
