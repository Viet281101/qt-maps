#include "OfflinePackageContract.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace qtmaps {
namespace core {
namespace {
bool safePath(const QString &p)
{
    return !p.isEmpty() && !QDir::isAbsolutePath(p) && QDir::cleanPath(p) == p && p != ".." &&
           !p.startsWith("../");
}
bool size(const QJsonValue &v, qint64 *out)
{
    if (!v.isDouble())
        return false;
    const qint64 n = v.toVariant().toLongLong();
    if (n < 0 || n != v.toDouble())
        return false;
    *out = n;
    return true;
}
bool object(const QString &path, QJsonObject *out, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Package contract is missing or unreadable: %1").arg(path);
        return false;
    }
    QJsonParseError e;
    const QJsonDocument d = QJsonDocument::fromJson(f.readAll(), &e);
    if (d.isNull() || !d.isObject()) {
        if (error)
            *error = QStringLiteral("Package contract JSON is invalid: %1").arg(e.errorString());
        return false;
    }
    *out = d.object();
    return true;
}
} // namespace

bool MapSearchBundleContract::loadAndValidate(const QString &path, MapSearchBundleContract *out,
                                              QString *error)
{
    if (!out) {
        if (error)
            *error = QStringLiteral("Bundle contract output pointer is null");
        return false;
    }
    const QFileInfo root(path);
    if (!root.isDir()) {
        if (error)
            *error = QStringLiteral("Bundle directory is missing: %1").arg(path);
        return false;
    }
    QJsonObject o;
    if (!object(QDir(root.canonicalFilePath()).filePath("bundle.json"), &o, error))
        return false;
    const QJsonObject c = o.value("components").toObject(), s = o.value("source").toObject();
    MapSearchBundleContract p;
    p.bundleId = o.value("bundle_id").toString();
    p.vectorPath = c.value("vector").toString();
    p.rasterPath = c.value("raster").toString();
    p.searchPath = c.value("search").toString();
    p.orthographyPath = c.value("orthography").toString();
    p.source = s.value("name").toString();
    p.retrievedOn = s.value("retrieved_on").toString();
    p.attribution = o.value("attribution").toString();
    if (o.value("schema_version").toString() != "qtmaps-local-map-search-bundle-v1" ||
        p.bundleId.isEmpty() || !safePath(p.vectorPath) || !safePath(p.rasterPath) ||
        !safePath(p.searchPath) || (!p.orthographyPath.isEmpty() && !safePath(p.orthographyPath)) ||
        p.source.isEmpty() || p.retrievedOn.isEmpty() || p.attribution.isEmpty() ||
        !size(o.value("installed_size_bytes"), &p.installedSizeBytes)) {
        if (error)
            *error = QStringLiteral("Map/search bundle contract is incompatible or incomplete");
        return false;
    }
    *out = p;
    return true;
}
bool RoutingGraphActivationContract::loadAndValidate(const QString &path,
                                                     RoutingGraphActivationContract *out,
                                                     QString *error)
{
    if (!out) {
        if (error)
            *error = QStringLiteral("Routing contract output pointer is null");
        return false;
    }
    QJsonObject o;
    if (!object(path, &o, error))
        return false;
    RoutingGraphActivationContract p;
    p.graphId = o.value("graph_id").toString();
    p.graphPath = o.value("graph_path").toString();
    p.sourcePbfSha256 = o.value("source_pbf_sha256").toString();
    p.attribution = o.value("attribution").toString();
    if (o.value("schema_version").toString() != "qtmaps-routing-graph-activation-v1" ||
        p.graphId.isEmpty() || !safePath(p.graphPath) ||
        !QRegularExpression("^[0-9a-f]{64}$").match(p.sourcePbfSha256).hasMatch() ||
        p.attribution.isEmpty() || !size(o.value("installed_size_bytes"), &p.installedSizeBytes)) {
        if (error)
            *error = QStringLiteral("Routing graph contract is incompatible or incomplete");
        return false;
    }
    *out = p;
    return true;
}
} // namespace core
} // namespace qtmaps
