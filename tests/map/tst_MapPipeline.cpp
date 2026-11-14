#include <QtTest>

#include "diagnostics/SimulatedNavigationController.h"
#include "map/EncodedTileCache.h"
#include "map/MapPackageManifest.h"
#include "map/MapSourceController.h"
#include "map/MbTilesTileSource.h"
#include "map/RasterPackageManifest.h"
#include "map/RasterTileSource.h"
#include "map/TileSource.h"

#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QUuid>

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

QByteArray httpGet(const QString &baseUrl, const QByteArray &path)
{
    const quint16 port = baseUrl.section(QLatin1Char(':'), -1).toUShort();
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, port);
    if (!socket.waitForConnected(2000)) {
        return QByteArray();
    }
    socket.write("GET " + path + " HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n");
    socket.waitForBytesWritten(2000);
    QByteArray response;
    while (socket.waitForReadyRead(2000)) {
        response += socket.readAll();
    }
    response += socket.readAll();
    return response;
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

class FakeTileSource final : public qtmaps::map::TileSource
{
public:
    bool start(const QString &, quint16 = 0) override
    {
        ++starts;
        running = succeeds;
        return running;
    }
    void stop() override { running = false; }
    bool isRunning() const override { return running; }
    QString baseUrl() const override { return QStringLiteral("http://127.0.0.1:43210"); }
    QString errorMessage() const override { return error; }

    bool succeeds = true;
    bool running = false;
    int starts = 0;
    QString error = QStringLiteral("fixture vector failure");
};

} // namespace

class MapPipelineTest final : public QObject
{
    Q_OBJECT

private slots:
    void manifestAcceptsCompleteChecksummedPackage()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString root = createPackage(temporary);
        QVERIFY(!root.isEmpty());
        qtmaps::map::MapPackageManifest manifest;
        QString error;
        QVERIFY2(qtmaps::map::MapPackageManifest::loadAndValidate(root, &manifest, &error),
                 qPrintable(error));
        QCOMPARE(manifest.dataVersion, QStringLiteral("hanoi-20261112-v3"));
        QCOMPARE(manifest.fileSha256.size(), 11);
    }

    void manifestRejectsMissingAndCorruptFiles()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString root = createPackage(temporary);
        QVERIFY(!root.isEmpty());
        QVERIFY(QFile::remove(root + QStringLiteral("/sprites/sprite.png")));
        qtmaps::map::MapPackageManifest manifest;
        QString error;
        QVERIFY(!qtmaps::map::MapPackageManifest::loadAndValidate(root, &manifest, &error));
        QVERIFY(error.contains(QStringLiteral("Cannot read package file")));

        QTemporaryDir highDpiTemporary;
        QVERIFY(highDpiTemporary.isValid());
        const QString highDpiRoot = createPackage(highDpiTemporary);
        QVERIFY(!highDpiRoot.isEmpty());
        QVERIFY(QFile::remove(highDpiRoot + QStringLiteral("/sprites/sprite@2x.png")));
        QVERIFY(!qtmaps::map::MapPackageManifest::loadAndValidate(highDpiRoot, &manifest, &error));
        QVERIFY(error.contains(QStringLiteral("Cannot read package file")));

        QTemporaryDir glyphTemporary;
        QVERIFY(glyphTemporary.isValid());
        const QString glyphRoot = createPackage(glyphTemporary);
        QVERIFY(!glyphRoot.isEmpty());
        QVERIFY(
            QFile::remove(glyphRoot + QStringLiteral("/glyphs/Noto Sans Regular/8192-8447.pbf")));
        QVERIFY(!qtmaps::map::MapPackageManifest::loadAndValidate(glyphRoot, &manifest, &error));
        QVERIFY(error.contains(QStringLiteral("Cannot read package file")));

        QTemporaryDir corruptTemporary;
        const QString corruptRoot = createPackage(corruptTemporary);
        QVERIFY(!corruptRoot.isEmpty());
        QFile corrupt(corruptRoot + QStringLiteral("/tiles.mbtiles"));
        QVERIFY(corrupt.open(QIODevice::Append));
        corrupt.write("corrupt");
        corrupt.close();
        QVERIFY(!qtmaps::map::MapPackageManifest::loadAndValidate(corruptRoot, &manifest, &error));
        QVERIFY(error.contains(QStringLiteral("Checksum mismatch")));
    }

    void manifestRejectsContractLayerAndBoundsMismatch()
    {
        qtmaps::map::MapPackageManifest manifest;
        QString error;

        QTemporaryDir contractTemporary;
        const QString contractRoot = createPackage(contractTemporary);
        QVERIFY(!contractRoot.isEmpty());
        mutateManifest(contractRoot, [](QJsonObject &object) {
            object.insert(QStringLiteral("style_contract_version"), QStringLiteral("future-v2"));
        });
        QVERIFY(!qtmaps::map::MapPackageManifest::loadAndValidate(contractRoot, &manifest, &error));

        QTemporaryDir layerTemporary;
        const QString layerRoot = createPackage(layerTemporary);
        QVERIFY(!layerRoot.isEmpty());
        mutateManifest(layerRoot, [](QJsonObject &object) {
            QJsonObject schema = object.value(QStringLiteral("layer_schema")).toObject();
            schema.insert(QStringLiteral("source_layers"),
                          QJsonArray{QStringLiteral("building"), QStringLiteral("place")});
            object.insert(QStringLiteral("layer_schema"), schema);
        });
        QVERIFY(!qtmaps::map::MapPackageManifest::loadAndValidate(layerRoot, &manifest, &error));

        QTemporaryDir boundsTemporary;
        const QString boundsRoot = createPackage(boundsTemporary);
        QVERIFY(!boundsRoot.isEmpty());
        mutateManifest(boundsRoot, [](QJsonObject &object) {
            object.insert(QStringLiteral("bounds"), QJsonArray{100.0, 10.0, 110.0, 24.0});
        });
        QVERIFY(!qtmaps::map::MapPackageManifest::loadAndValidate(boundsRoot, &manifest, &error));

        QTemporaryDir remoteStyleTemporary;
        const QString remoteStyleRoot = createPackage(remoteStyleTemporary);
        QVERIFY(!remoteStyleRoot.isEmpty());
        const QString stylePath = remoteStyleRoot + QStringLiteral("/style/style.json");
        QFile styleInput(stylePath);
        QVERIFY(styleInput.open(QIODevice::ReadOnly));
        QJsonObject style = QJsonDocument::fromJson(styleInput.readAll()).object();
        styleInput.close();
        QJsonObject sources = style.value(QStringLiteral("sources")).toObject();
        QJsonObject qtmapsSource = sources.value(QStringLiteral("qtmaps")).toObject();
        qtmapsSource.insert(QStringLiteral("tiles"),
                            QJsonArray{QStringLiteral("https://tiles.invalid/{z}/{x}/{y}.pbf")});
        sources.insert(QStringLiteral("qtmaps"), qtmapsSource);
        style.insert(QStringLiteral("sources"), sources);
        writeFile(stylePath, QJsonDocument(style).toJson(QJsonDocument::Indented));
        mutateManifest(remoteStyleRoot, [&stylePath](QJsonObject &object) {
            QJsonObject files = object.value(QStringLiteral("files")).toObject();
            QJsonObject styleEntry = files.value(QStringLiteral("style/style.json")).toObject();
            styleEntry.insert(QStringLiteral("sha256"), digest(stylePath));
            styleEntry.insert(QStringLiteral("size_bytes"), QFileInfo(stylePath).size());
            files.insert(QStringLiteral("style/style.json"), styleEntry);
            object.insert(QStringLiteral("files"), files);
        });
        QVERIFY(
            !qtmaps::map::MapPackageManifest::loadAndValidate(remoteStyleRoot, &manifest, &error));
        QVERIFY(error.contains(QStringLiteral("Style resource URLs")));
    }

    void rasterManifestAcceptsBoundPackageAndRejectsContractMismatch()
    {
        QTemporaryDir temporary;
        const QString root = createRasterPackage(temporary);
        QVERIFY(!root.isEmpty());
        qtmaps::map::RasterPackageManifest manifest;
        QString error;
        QVERIFY2(qtmaps::map::RasterPackageManifest::loadAndValidate(root, &manifest, &error),
                 qPrintable(error));
        QCOMPARE(manifest.tileCount, 847);
        QCOMPARE(manifest.maximumZoom, 16);
        QCOMPARE(manifest.styleRevision, QStringLiteral("qtmaps-hanoi-v3"));

        QTemporaryDir mismatchTemporary;
        const QString mismatchRoot = createRasterPackage(mismatchTemporary);
        mutateManifest(mismatchRoot, [](QJsonObject &object) {
            QJsonObject vector = object.value(QStringLiteral("source_vector_package")).toObject();
            vector.insert(QStringLiteral("style_revision"), QStringLiteral("future-v3"));
            object.insert(QStringLiteral("source_vector_package"), vector);
        });
        QVERIFY(
            !qtmaps::map::RasterPackageManifest::loadAndValidate(mismatchRoot, &manifest, &error));
        QVERIFY(error.contains(QStringLiteral("style/package")));
    }

    void rasterManifestRejectsChecksumAndCoverageMismatch()
    {
        qtmaps::map::RasterPackageManifest manifest;
        QString error;
        QTemporaryDir corruptTemporary;
        const QString corruptRoot = createRasterPackage(corruptTemporary);
        QFile corrupt(corruptRoot + QStringLiteral("/raster.mbtiles"));
        QVERIFY(corrupt.open(QIODevice::Append));
        corrupt.write("corrupt");
        corrupt.close();
        QVERIFY(
            !qtmaps::map::RasterPackageManifest::loadAndValidate(corruptRoot, &manifest, &error));
        QVERIFY(error.contains(QStringLiteral("size mismatch")));

        QTemporaryDir coverageTemporary;
        const QString coverageRoot = createRasterPackage(coverageTemporary);
        mutateManifest(coverageRoot,
                       [](QJsonObject &object) { object.insert(QStringLiteral("max_zoom"), 15); });
        QVERIFY(
            !qtmaps::map::RasterPackageManifest::loadAndValidate(coverageRoot, &manifest, &error));
        QVERIFY(error.contains(QStringLiteral("coverage")));
    }

    void encodedRasterCacheNeverExceedsConfiguredLimit()
    {
        qtmaps::map::EncodedTileCache cache(10);
        cache.insert(QStringLiteral("first"), QByteArray(6, 'a'));
        cache.insert(QStringLiteral("second"), QByteArray(6, 'b'));
        QVERIFY(cache.currentBytes() <= cache.maximumBytes());
        QCOMPARE(cache.maximumBytes(), 10);
        QByteArray value;
        QVERIFY(!cache.find(QStringLiteral("first"), &value));
        QVERIFY(cache.find(QStringLiteral("second"), &value));
        QCOMPARE(value, QByteArray(6, 'b'));
    }

    void rasterTileSourceStartsAsynchronouslyServesAndStops()
    {
        QTemporaryDir temporary;
        const QString root = createRasterPackage(temporary);
        QVERIFY(!root.isEmpty());
        qtmaps::map::RasterTileSource source;
        QSignalSpy started(&source, &qtmaps::map::RasterTileSource::started);
        QElapsedTimer elapsed;
        elapsed.start();
        source.startAsync(root);
        QVERIFY(elapsed.elapsed() < 50);
        QVERIFY(started.wait(5000));
        QVERIFY(source.running());
        QCOMPARE(qtmaps::map::RasterTileSource::kEncodedCacheMaximumBytes, 8 * 1024 * 1024);
        const QByteArray style = httpGet(source.baseUrl(), QByteArrayLiteral("/style/style.json"));
        QVERIFY(style.startsWith(QByteArrayLiteral("HTTP/1.1 200")));
        QVERIFY(style.contains(QByteArrayLiteral("qtmaps-raster")));
        QVERIFY(style.contains(QByteArrayLiteral("/tiles/{z}/{x}/{y}.png")));
        const QByteArray tile =
            httpGet(source.baseUrl(), QByteArrayLiteral("/tiles/12/3251/1803.png"));
        QVERIFY(tile.startsWith(QByteArrayLiteral("HTTP/1.1 200")));
        QVERIFY(tile.endsWith(QByteArrayLiteral("png-fixture")));
        QSignalSpy stopped(&source, &qtmaps::map::RasterTileSource::stopped);
        source.stopAsync();
        QVERIFY(stopped.wait(5000));
        QVERIFY(!source.running());
    }

    void controllerSupportsManualAndAutomaticRasterActivation()
    {
        QTemporaryDir temporary;
        const QString root = createRasterPackage(temporary);
        QVERIFY(!root.isEmpty());
        FakeTileSource vector;
        FakeTileSource diagnostic;
        qtmaps::map::MapSourceController controller(&vector, &diagnostic);
        controller.setRasterPackagePath(root);
        QVERIFY(controller.start(QStringLiteral("vector"), QString()));
        QCOMPARE(controller.layerMode(), QStringLiteral("vector"));
        QVERIFY(!controller.usingRasterFallback());
        QVERIFY(!controller.rasterResourcesAllocated());

        for (int cycle = 0; cycle < 10; ++cycle) {
            controller.activateRaster();
            QCOMPARE(controller.layerMode(), QStringLiteral("raster-loading"));
            QVERIFY(controller.rasterResourcesAllocated());
            QTRY_COMPARE_WITH_TIMEOUT(controller.layerMode(), QStringLiteral("raster"), 5000);
            QVERIFY(controller.usingRasterFallback());
            QVERIFY(!controller.automaticFallback());
            controller.activateVector();
            QCOMPARE(controller.layerMode(), QStringLiteral("vector"));
            QVERIFY(!controller.usingRasterFallback());
            QTRY_VERIFY_WITH_TIMEOUT(!controller.rasterResourcesAllocated(), 5000);
        }

        controller.stop();
        vector.succeeds = false;
        controller.setRasterPackagePath(root);
        QVERIFY(controller.start(QStringLiteral("vector"), QString(), true));
        QTRY_COMPARE_WITH_TIMEOUT(controller.layerMode(), QStringLiteral("raster"), 5000);
        QVERIFY(controller.automaticFallback());
        QVERIFY(controller.mapStatusText().contains(QStringLiteral("tự động")));
    }

    void invalidRasterFailsWithoutBlockingCaller()
    {
        qtmaps::map::RasterTileSource source;
        QSignalSpy failed(&source, &qtmaps::map::RasterTileSource::failed);
        QElapsedTimer elapsed;
        elapsed.start();
        source.startAsync(QStringLiteral("/definitely/missing/raster-package"));
        QVERIFY(elapsed.elapsed() < 50);
        QVERIFY(failed.wait(5000));
        QVERIFY(source.errorMessage().contains(QStringLiteral("missing")));
    }

    void mbtilesLifecycleStartStopRestartAndServe()
    {
        QTemporaryDir temporary;
        const QString root = createPackage(temporary);
        QVERIFY(!root.isEmpty());
        qtmaps::map::MbTilesTileSource source;
        QVERIFY2(source.start(root), qPrintable(source.errorMessage()));
        QVERIFY(source.isRunning());
        QVERIFY(source.baseUrl().startsWith(QStringLiteral("http://127.0.0.1:")));
        const QByteArray health = httpGet(source.baseUrl(), QByteArrayLiteral("/healthz"));
        QVERIFY(health.startsWith(QByteArrayLiteral("HTTP/1.1 200")));
        QVERIFY(health.endsWith(QByteArrayLiteral("ok\n")));
        const QByteArray style = httpGet(source.baseUrl(), QByteArrayLiteral("/style/style.json"));
        QVERIFY(style.contains(source.baseUrl().toUtf8()));
        QVERIFY(!style.contains(QByteArrayLiteral("__QTMAPS_BASE_URL__")));
        source.stop();
        QVERIFY(!source.isRunning());
        QVERIFY2(source.start(root), qPrintable(source.errorMessage()));
        QVERIFY(source.isRunning());
    }

    void mbtilesRejectsPortConflictAndStopsCleanly()
    {
        QTemporaryDir temporary;
        const QString root = createPackage(temporary);
        QVERIFY(!root.isEmpty());
        QTcpServer occupied;
        QVERIFY(occupied.listen(QHostAddress::LocalHost, 0));
        {
            qtmaps::map::MbTilesTileSource source;
            QVERIFY(!source.start(root, occupied.serverPort()));
            QVERIFY(source.errorMessage().contains(QStringLiteral("Cannot bind")));
            QVERIFY(!source.isRunning());
        }
    }

    void mbtilesMissingAndCorruptPackageFailWithinBoundedStartupTime()
    {
        qtmaps::map::MbTilesTileSource source;
        QElapsedTimer elapsed;
        elapsed.start();
        QVERIFY(!source.start(QStringLiteral("/definitely/missing/qtmaps-package")));
        QVERIFY(elapsed.elapsed() < 5000);
        QVERIFY(source.errorMessage().contains(QStringLiteral("missing")));

        QTemporaryDir temporary;
        const QString root = createPackage(temporary);
        QVERIFY(!root.isEmpty());
        QFile corrupt(root + QStringLiteral("/tiles.mbtiles"));
        QVERIFY(corrupt.open(QIODevice::Append));
        corrupt.write("corrupt");
        corrupt.close();
        elapsed.restart();
        QVERIFY(!source.start(root));
        QVERIFY(elapsed.elapsed() < 5000);
        QVERIFY(source.errorMessage().contains(QStringLiteral("Checksum mismatch")));
    }

    void controllerReportsFallbackReason()
    {
        FakeTileSource vector;
        FakeTileSource raster;
        vector.succeeds = false;
        qtmaps::map::MapSourceController controller(&vector, &raster);
        QVERIFY(controller.start(QStringLiteral("package"), QStringLiteral("raster")));
        QVERIFY(controller.usingRasterFallback());
        QCOMPARE(controller.fallbackReason(), QStringLiteral("fixture vector failure"));
        QVERIFY(controller.rasterBaseUrl().startsWith(QStringLiteral("http://127.0.0.1:")));
        controller.stop();
        QVERIFY(!controller.usingRasterFallback());

        vector.succeeds = true;
        QVERIFY(controller.start(QStringLiteral("package"), QStringLiteral("raster"), true));
        QVERIFY(controller.fallbackReason().contains(QStringLiteral("forced")));
        QCOMPARE(vector.starts, 1);
    }

    void simulatedNavigationUsesStableHanoiRoute()
    {
        qtmaps::diagnostics::SimulatedNavigationController navigation;
        QVERIFY(navigation.routeLengthMeters() >= 4500.0);
        QVERIFY(navigation.routeLengthMeters() <= 6500.0);
        double firstBearing = 0.0;
        double secondBearing = 0.0;
        const QGeoCoordinate first = navigation.coordinateAtDistance(1333.333, &firstBearing);
        const QGeoCoordinate second = navigation.coordinateAtDistance(1333.333, &secondBearing);
        QCOMPARE(first, second);
        QCOMPARE(firstBearing, secondBearing);
        navigation.start(40.0);
        QTRY_VERIFY_WITH_TIMEOUT(navigation.travelledMeters() > 0.5, 1000);
        QCOMPARE(navigation.speedKilometersPerHour(), 40.0);
        QVERIFY(navigation.coordinate().isValid());
        QVERIFY(navigation.bearing() >= 0.0 && navigation.bearing() <= 360.0);
        navigation.stop();
        QVERIFY(!navigation.running());
    }
};

QTEST_MAIN(MapPipelineTest)

#include "tst_MapPipeline.moc"
