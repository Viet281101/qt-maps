#include "MapPackageManifest.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <algorithm>

namespace qtmaps {
namespace map {
namespace {

QString sha256(const QString &path, QString *errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot read package file %1: %2")
                                .arg(path, file.errorString());
        }
        return QString();
    }
    QCryptographicHash digest(QCryptographicHash::Sha256);
    if (!digest.addData(&file)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot hash package file: %1").arg(path);
        }
        return QString();
    }
    return QString::fromLatin1(digest.result().toHex());
}

bool isSafeRelativePath(const QString &path)
{
    return !path.isEmpty() && !QDir::isAbsolutePath(path)
        && QDir::cleanPath(path) == path && path != QStringLiteral("..")
        && !path.startsWith(QStringLiteral("../"));
}

} // namespace

bool MapPackageManifest::loadAndValidate(const QString &packagePath,
                                         MapPackageManifest *manifest,
                                         QString *errorMessage)
{
    if (!manifest) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Manifest output pointer is null");
        }
        return false;
    }
    const QFileInfo rootInfo(packagePath);
    if (!rootInfo.exists() || !rootInfo.isDir()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Map package directory is missing: %1")
                                .arg(packagePath);
        }
        return false;
    }
    const QDir root(rootInfo.canonicalFilePath());
    QFile file(root.filePath(QStringLiteral("manifest.json")));
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Map package manifest is missing or unreadable: %1")
                                .arg(file.errorString());
        }
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (document.isNull() || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid map package manifest JSON: %1")
                                .arg(parseError.errorString());
        }
        return false;
    }
    const QJsonObject object = document.object();
    MapPackageManifest parsed;
    parsed.schemaVersion = object.value(QStringLiteral("schema_version")).toString();
    parsed.dataVersion = object.value(QStringLiteral("data_version")).toString();
    parsed.styleContractVersion = object.value(QStringLiteral("style_contract_version")).toString();
    parsed.minimumZoom = object.value(QStringLiteral("min_zoom")).toInt(-1);
    parsed.maximumZoom = object.value(QStringLiteral("max_zoom")).toInt(-1);
    parsed.planetilerVersion = object.value(QStringLiteral("planetiler")).toObject()
                                   .value(QStringLiteral("version")).toString();
    parsed.planetilerSha256 = object.value(QStringLiteral("planetiler")).toObject()
                                  .value(QStringLiteral("sha256")).toString();
    parsed.sourcePbfSha256 = object.value(QStringLiteral("source_pbf")).toObject()
                                 .value(QStringLiteral("sha256")).toString();
    parsed.attribution = object.value(QStringLiteral("attribution")).toString();
    const QJsonObject layerSchema = object.value(QStringLiteral("layer_schema")).toObject();
    for (const QJsonValue &value : layerSchema.value(QStringLiteral("source_layers")).toArray()) {
        parsed.sourceLayers.append(value.toString());
    }
    parsed.sourceLayers.sort();

    const QJsonArray bounds = object.value(QStringLiteral("bounds")).toArray();
    if (bounds.size() == 4) {
        parsed.west = bounds.at(0).toDouble();
        parsed.south = bounds.at(1).toDouble();
        parsed.east = bounds.at(2).toDouble();
        parsed.north = bounds.at(3).toDouble();
    }
    const QRegularExpression digestPattern(QStringLiteral("^[0-9a-f]{64}$"));
    const QStringList expectedLayers = {QStringLiteral("building"), QStringLiteral("place"),
                                        QStringLiteral("transportation")};
    if (parsed.schemaVersion != QStringLiteral("qtmaps-map-package-v1")
        || parsed.styleContractVersion != QStringLiteral("qtmaps-style-v1")
        || parsed.dataVersion.isEmpty() || parsed.planetilerVersion != QStringLiteral("0.10.2")
        || parsed.planetilerSha256
               != QStringLiteral("f310bd0413e2e4512b27f4046d418664e8e1d3bf31603c2a70e23de06c167e4d")
        || !digestPattern.match(parsed.sourcePbfSha256).hasMatch()
        || parsed.attribution != QStringLiteral("© OpenStreetMap contributors, ODbL 1.0")
        || layerSchema.value(QStringLiteral("version")).toInt(-1) != 1
        || parsed.sourceLayers != expectedLayers
        || bounds.size() != 4 || parsed.west != 105.78 || parsed.south != 20.97
        || parsed.east != 105.90 || parsed.north != 21.10
        || parsed.minimumZoom != 12 || parsed.maximumZoom != 15) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Map package manifest metadata is incompatible or incomplete");
        }
        return false;
    }

    const QJsonObject files = object.value(QStringLiteral("files")).toObject();
    const QStringList required = {QStringLiteral("tiles.mbtiles"),
                                  QStringLiteral("style/style.json"),
                                  QStringLiteral("sprites/sprite.json"),
                                  QStringLiteral("sprites/sprite.png")};
    for (auto iterator = files.constBegin(); iterator != files.constEnd(); ++iterator) {
        const QString relativePath = iterator.key();
        const QString expected = iterator.value().toObject()
                                     .value(QStringLiteral("sha256")).toString();
        if (!isSafeRelativePath(relativePath)
            || !digestPattern.match(expected).hasMatch()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Unsafe path or invalid checksum in manifest: %1")
                                    .arg(relativePath);
            }
            return false;
        }
        QString hashError;
        const QString actual = sha256(root.filePath(relativePath), &hashError);
        if (actual.isEmpty() || actual != expected) {
            if (errorMessage) {
                *errorMessage = actual.isEmpty()
                    ? hashError
                    : QStringLiteral("Checksum mismatch for %1").arg(relativePath);
            }
            return false;
        }
        parsed.fileSha256.insert(relativePath, expected);
    }
    for (const QString &relativePath : required) {
        if (!parsed.fileSha256.contains(relativePath)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Required package file is not checksummed: %1")
                                    .arg(relativePath);
            }
            return false;
        }
    }
    const bool hasGlyph = std::any_of(parsed.fileSha256.keyBegin(), parsed.fileSha256.keyEnd(),
                                      [](const QString &path) {
                                          return path.startsWith(QStringLiteral("glyphs/"))
                                              && path.endsWith(QStringLiteral(".pbf"));
                                      });
    if (!hasGlyph) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Map package contains no checksummed glyph resource");
        }
        return false;
    }
    QFile styleFile(root.filePath(QStringLiteral("style/style.json")));
    if (!styleFile.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot read style contract");
        }
        return false;
    }
    const QJsonDocument styleDocument = QJsonDocument::fromJson(styleFile.readAll());
    const QJsonObject style = styleDocument.object();
    const QString token = QStringLiteral("__QTMAPS_BASE_URL__");
    if (style.value(QStringLiteral("sprite")).toString() != token + QStringLiteral("/sprites/sprite")
        || style.value(QStringLiteral("glyphs")).toString()
               != token + QStringLiteral("/glyphs/{fontstack}/{range}.pbf")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Style resource URLs violate qtmaps-style-v1");
        }
        return false;
    }
    QStringList styleLayers;
    for (const QJsonValue &value : style.value(QStringLiteral("layers")).toArray()) {
        const QString sourceLayer = value.toObject().value(QStringLiteral("source-layer")).toString();
        if (!sourceLayer.isEmpty() && !styleLayers.contains(sourceLayer)) {
            styleLayers.append(sourceLayer);
        }
    }
    styleLayers.sort();
    if (styleLayers != expectedLayers) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Style source layers do not match the package layer schema");
        }
        return false;
    }
    *manifest = parsed;
    return true;
}

} // namespace map
} // namespace qtmaps
