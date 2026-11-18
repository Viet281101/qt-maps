#pragma once

// Shared synthetic package builders for offline runtime tests.
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>
#include <functional>

namespace {

void writeFile(const QString &path, const QByteArray &data)
{
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
    QCOMPARE(file.write(data), data.size());
}

QString digest(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        return QString();
    }
    return QString::fromLatin1(hash.result().toHex());
}

QString createPackage(QTemporaryDir &temporary)
{
    const QString root = temporary.path() + QStringLiteral("/hanoi-20261112-v3");
    if (!QDir().mkpath(root + QStringLiteral("/style")) ||
        !QDir().mkpath(root + QStringLiteral("/glyphs/Noto Sans Regular")) ||
        !QDir().mkpath(root + QStringLiteral("/sprites"))) {
        return QString();
    }
    writeFile(root + QStringLiteral("/style/style.json"),
              QByteArrayLiteral("{\"version\":8,\"sprite\":\"__QTMAPS_BASE_URL__/sprites/sprite\","
                                "\"glyphs\":\"__QTMAPS_BASE_URL__/glyphs/{fontstack}/{range}.pbf\","
                                "\"sources\":{\"qtmaps\":{\"type\":\"vector\",\"tiles\":["
                                "\"__QTMAPS_BASE_URL__/tiles/{z}/{x}/{y}.pbf\"]}},\"layers\":["
                                "{\"id\":\"water\",\"source-layer\":\"water\"},"
                                "{\"id\":\"green\",\"source-layer\":\"landuse\"},"
                                "{\"id\":\"building\",\"source-layer\":\"building\"},"
                                "{\"id\":\"place\",\"source-layer\":\"place\"},"
                                "{\"id\":\"road\",\"source-layer\":\"transportation\"}]}\n"));
    writeFile(root + QStringLiteral("/glyphs/Noto Sans Regular/0-255.pbf"),
              QByteArrayLiteral("glyph-fixture"));
    writeFile(root + QStringLiteral("/glyphs/Noto Sans Regular/256-511.pbf"),
              QByteArrayLiteral("glyph-fixture"));
    writeFile(root + QStringLiteral("/glyphs/Noto Sans Regular/768-1023.pbf"),
              QByteArrayLiteral("glyph-fixture"));
    writeFile(root + QStringLiteral("/glyphs/Noto Sans Regular/7680-7935.pbf"),
              QByteArrayLiteral("glyph-fixture"));
    writeFile(root + QStringLiteral("/glyphs/Noto Sans Regular/8192-8447.pbf"),
              QByteArrayLiteral("glyph-fixture"));
    writeFile(root + QStringLiteral("/sprites/sprite.json"), QByteArrayLiteral("{}\n"));
    writeFile(root + QStringLiteral("/sprites/sprite.png"), QByteArrayLiteral("png-fixture"));
    writeFile(root + QStringLiteral("/sprites/sprite@2x.json"), QByteArrayLiteral("{}\n"));
    writeFile(root + QStringLiteral("/sprites/sprite@2x.png"), QByteArrayLiteral("png-fixture"));

    const QString connection =
        QStringLiteral("package-fixture-%1").arg(QUuid::createUuid().toString());
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(root + QStringLiteral("/tiles.mbtiles"));
        if (!database.open()) {
            return QString();
        }
        QSqlQuery query(database);
        if (!query.exec(QStringLiteral("CREATE TABLE tiles (zoom_level INTEGER, tile_column "
                                       "INTEGER, tile_row INTEGER, tile_data BLOB)"))) {
            return QString();
        }
        query.prepare(QStringLiteral("INSERT INTO tiles VALUES (12, 3251, 2048, ?)"));
        query.addBindValue(QByteArrayLiteral("vector-tile-fixture"));
        if (!query.exec()) {
            return QString();
        }
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);

    const QStringList paths = {
        QStringLiteral("tiles.mbtiles"),
        QStringLiteral("style/style.json"),
        QStringLiteral("glyphs/Noto Sans Regular/0-255.pbf"),
        QStringLiteral("glyphs/Noto Sans Regular/256-511.pbf"),
        QStringLiteral("glyphs/Noto Sans Regular/768-1023.pbf"),
        QStringLiteral("glyphs/Noto Sans Regular/7680-7935.pbf"),
        QStringLiteral("glyphs/Noto Sans Regular/8192-8447.pbf"),
        QStringLiteral("sprites/sprite.json"),
        QStringLiteral("sprites/sprite.png"),
        QStringLiteral("sprites/sprite@2x.json"),
        QStringLiteral("sprites/sprite@2x.png"),
    };
    QJsonObject files;
    for (const QString &relative : paths) {
        QJsonObject entry;
        entry.insert(QStringLiteral("sha256"), digest(root + QLatin1Char('/') + relative));
        entry.insert(QStringLiteral("size_bytes"),
                     QFileInfo(root + QLatin1Char('/') + relative).size());
        files.insert(relative, entry);
    }
    QJsonObject planetiler;
    planetiler.insert(QStringLiteral("version"), QStringLiteral("0.10.2"));
    planetiler.insert(
        QStringLiteral("sha256"),
        QStringLiteral("f310bd0413e2e4512b27f4046d418664e8e1d3bf31603c2a70e23de06c167e4d"));
    QJsonObject sourcePbf;
    sourcePbf.insert(QStringLiteral("sha256"), QString(64, QLatin1Char('a')));
    QJsonObject manifest;
    manifest.insert(QStringLiteral("schema_version"), QStringLiteral("qtmaps-map-package-v3"));
    manifest.insert(QStringLiteral("data_version"), QStringLiteral("hanoi-20261112-v3"));
    manifest.insert(QStringLiteral("style_contract_version"), QStringLiteral("qtmaps-style-v3"));
    QJsonObject layerSchema;
    layerSchema.insert(QStringLiteral("version"), 2);
    layerSchema.insert(QStringLiteral("source_layers"),
                       QJsonArray{QStringLiteral("building"), QStringLiteral("landuse"),
                                  QStringLiteral("place"), QStringLiteral("transportation"),
                                  QStringLiteral("water")});
    manifest.insert(QStringLiteral("layer_schema"), layerSchema);
    manifest.insert(QStringLiteral("bounds"), QJsonArray{105.78, 20.97, 105.90, 21.10});
    manifest.insert(QStringLiteral("min_zoom"), 12);
    manifest.insert(QStringLiteral("max_zoom"), 15);
    manifest.insert(QStringLiteral("planetiler"), planetiler);
    manifest.insert(QStringLiteral("source_pbf"), sourcePbf);
    QJsonObject pipelineConfig;
    pipelineConfig.insert(QStringLiteral("revision"), QStringLiteral("unit03-test-v1"));
    pipelineConfig.insert(QStringLiteral("sha256"), QString(64, QLatin1Char('b')));
    manifest.insert(QStringLiteral("pipeline_config"), pipelineConfig);
    QJsonObject resources;
    resources.insert(QStringLiteral("style_revision"), QStringLiteral("qtmaps-hanoi-v3"));
    resources.insert(QStringLiteral("glyph_revision"), QStringLiteral("noto-sans-regular-v2"));
    resources.insert(QStringLiteral("sprite_revision"), QStringLiteral("sprite-test-v1"));
    manifest.insert(QStringLiteral("resources"), resources);
    manifest.insert(QStringLiteral("attribution"),
                    QStringLiteral("© OpenStreetMap contributors, ODbL 1.0"));
    manifest.insert(QStringLiteral("files"), files);
    writeFile(root + QStringLiteral("/manifest.json"),
              QJsonDocument(manifest).toJson(QJsonDocument::Indented));
    return root;
}

QString createRasterPackage(QTemporaryDir &temporary)
{
    const QString root = temporary.path() + QStringLiteral("/hanoi-raster-20261111-v2");
    if (!QDir().mkpath(root)) {
        return QString();
    }
    const QString databasePath = root + QStringLiteral("/raster.mbtiles");
    const QString connection =
        QStringLiteral("raster-package-fixture-%1").arg(QUuid::createUuid().toString());
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(databasePath);
        if (!database.open()) {
            return QString();
        }
        QSqlQuery query(database);
        if (!query.exec(QStringLiteral("CREATE TABLE tiles (zoom_level INTEGER, tile_column "
                                       "INTEGER, tile_row INTEGER, tile_data BLOB)"))) {
            return QString();
        }
        query.prepare(QStringLiteral("INSERT INTO tiles VALUES (?, ?, ?, ?)"));
        for (int index = 0; index < 847; ++index) {
            query.addBindValue(index == 0 ? 12 : 16);
            query.addBindValue(index == 0 ? 3251 : index);
            query.addBindValue(index == 0 ? 2292 : 0);
            query.addBindValue(QByteArrayLiteral("png-fixture"));
            if (!query.exec()) {
                return QString();
            }
        }
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);

    QJsonObject vector;
    vector.insert(QStringLiteral("package_schema_version"),
                  QStringLiteral("qtmaps-map-package-v3"));
    vector.insert(QStringLiteral("data_version"), QStringLiteral("hanoi-20261112-v3"));
    vector.insert(QStringLiteral("manifest_sha256"), QString(64, QLatin1Char('b')));
    vector.insert(QStringLiteral("style_contract_version"), QStringLiteral("qtmaps-style-v3"));
    vector.insert(QStringLiteral("style_revision"), QStringLiteral("qtmaps-hanoi-v3"));
    vector.insert(QStringLiteral("layer_schema_version"), 2);
    QJsonObject sourcePbf;
    sourcePbf.insert(QStringLiteral("name"), QStringLiteral("vietnam-latest.osm.pbf"));
    sourcePbf.insert(QStringLiteral("sha256"), QString(64, QLatin1Char('a')));
    QJsonObject renderer;
    renderer.insert(QStringLiteral("name"), QStringLiteral("MapLibre Native Qt"));
    renderer.insert(QStringLiteral("version"), QStringLiteral("3.0.0"));
    QJsonObject tiles;
    tiles.insert(QStringLiteral("path"), QStringLiteral("raster.mbtiles"));
    tiles.insert(QStringLiteral("format"), QStringLiteral("png"));
    tiles.insert(QStringLiteral("tile_size"), 256);
    tiles.insert(QStringLiteral("count"), 847);
    tiles.insert(QStringLiteral("size_bytes"), QFileInfo(databasePath).size());
    tiles.insert(QStringLiteral("sha256"), digest(databasePath));
    QJsonObject manifest;
    manifest.insert(QStringLiteral("schema_version"), QStringLiteral("qtmaps-raster-package-v1"));
    manifest.insert(QStringLiteral("package_id"), QStringLiteral("hanoi-raster-20261111-v2"));
    manifest.insert(QStringLiteral("source_vector_package"), vector);
    manifest.insert(QStringLiteral("source_pbf"), sourcePbf);
    manifest.insert(QStringLiteral("bounds"), QJsonArray{105.78, 20.97, 105.90, 21.10});
    manifest.insert(QStringLiteral("min_zoom"), 12);
    manifest.insert(QStringLiteral("max_zoom"), 16);
    manifest.insert(QStringLiteral("renderer"), renderer);
    manifest.insert(QStringLiteral("tiles"), tiles);
    manifest.insert(QStringLiteral("attribution"),
                    QStringLiteral("© OpenStreetMap contributors, ODbL 1.0"));
    writeFile(root + QStringLiteral("/manifest.json"),
              QJsonDocument(manifest).toJson(QJsonDocument::Indented));
    return root;
}

void mutateManifest(const QString &root, const std::function<void(QJsonObject &)> &mutation)
{
    QFile input(root + QStringLiteral("/manifest.json"));
    QVERIFY(input.open(QIODevice::ReadOnly));
    QJsonObject object = QJsonDocument::fromJson(input.readAll()).object();
    input.close();
    mutation(object);
    writeFile(root + QStringLiteral("/manifest.json"),
              QJsonDocument(object).toJson(QJsonDocument::Indented));
}

void bindRasterToVector(const QString &rasterRoot, const QString &vectorRoot)
{
    QFile vectorInput(vectorRoot + QStringLiteral("/manifest.json"));
    QVERIFY(vectorInput.open(QIODevice::ReadOnly));
    const QJsonObject vectorManifest = QJsonDocument::fromJson(vectorInput.readAll()).object();
    vectorInput.close();
    mutateManifest(rasterRoot, [&vectorManifest, &vectorRoot](QJsonObject &object) {
        QJsonObject vector = object.value(QStringLiteral("source_vector_package")).toObject();
        vector.insert(QStringLiteral("package_schema_version"),
                      vectorManifest.value(QStringLiteral("schema_version")).toString());
        vector.insert(QStringLiteral("data_version"),
                      vectorManifest.value(QStringLiteral("data_version")).toString());
        vector.insert(QStringLiteral("manifest_sha256"),
                      digest(vectorRoot + QStringLiteral("/manifest.json")));
        vector.insert(QStringLiteral("style_contract_version"),
                      vectorManifest.value(QStringLiteral("style_contract_version")).toString());
        const QJsonObject resources = vectorManifest.value(QStringLiteral("resources")).toObject();
        vector.insert(QStringLiteral("style_revision"),
                      resources.value(QStringLiteral("style_revision")).toString());
        vector.insert(QStringLiteral("layer_schema_version"),
                      vectorManifest.value(QStringLiteral("layer_schema"))
                          .toObject()
                          .value(QStringLiteral("version"))
                          .toInt());
        object.insert(QStringLiteral("source_vector_package"), vector);
    });
}

} // namespace
