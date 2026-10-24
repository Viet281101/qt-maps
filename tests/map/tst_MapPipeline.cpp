#include <QtTest>

#include "map/MapPackageManifest.h"
#include "map/MapSourceController.h"
#include "map/MbTilesTileSource.h"
#include "map/SimulatedNavigationController.h"
#include "map/TileSource.h"

#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
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
    const QString root = temporary.path() + QStringLiteral("/hanoi-20261024-v1");
    if (!QDir().mkpath(root + QStringLiteral("/style"))
        || !QDir().mkpath(root + QStringLiteral("/glyphs/Noto Sans Regular"))
        || !QDir().mkpath(root + QStringLiteral("/sprites"))) {
        return QString();
    }
    writeFile(root + QStringLiteral("/style/style.json"),
              QByteArrayLiteral(
                  "{\"version\":8,\"sprite\":\"__QTMAPS_BASE_URL__/sprites/sprite\","
                  "\"glyphs\":\"__QTMAPS_BASE_URL__/glyphs/{fontstack}/{range}.pbf\","
                  "\"sources\":{},\"layers\":["
                  "{\"id\":\"building\",\"source-layer\":\"building\"},"
                  "{\"id\":\"place\",\"source-layer\":\"place\"},"
                  "{\"id\":\"road\",\"source-layer\":\"transportation\"}]}\n"));
    writeFile(root + QStringLiteral("/glyphs/Noto Sans Regular/0-255.pbf"),
              QByteArrayLiteral("glyph-fixture"));
    writeFile(root + QStringLiteral("/sprites/sprite.json"), QByteArrayLiteral("{}\n"));
    writeFile(root + QStringLiteral("/sprites/sprite.png"), QByteArrayLiteral("png-fixture"));

    const QString connection = QStringLiteral("package-fixture-%1")
                                   .arg(QUuid::createUuid().toString());
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(root + QStringLiteral("/tiles.mbtiles"));
        if (!database.open()) {
            return QString();
        }
        QSqlQuery query(database);
        if (!query.exec(QStringLiteral(
                "CREATE TABLE tiles (zoom_level INTEGER, tile_column INTEGER, tile_row INTEGER, tile_data BLOB)"))) {
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
        QStringLiteral("sprites/sprite.json"),
        QStringLiteral("sprites/sprite.png"),
    };
    QJsonObject files;
    for (const QString &relative : paths) {
        QJsonObject entry;
        entry.insert(QStringLiteral("sha256"), digest(root + QLatin1Char('/') + relative));
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
    manifest.insert(QStringLiteral("schema_version"), QStringLiteral("qtmaps-map-package-v1"));
    manifest.insert(QStringLiteral("data_version"), QStringLiteral("hanoi-20261024-v1"));
    manifest.insert(QStringLiteral("style_contract_version"), QStringLiteral("qtmaps-style-v1"));
    QJsonObject layerSchema;
    layerSchema.insert(QStringLiteral("version"), 1);
    layerSchema.insert(QStringLiteral("source_layers"),
                       QJsonArray{QStringLiteral("building"), QStringLiteral("place"),
                                  QStringLiteral("transportation")});
    manifest.insert(QStringLiteral("layer_schema"), layerSchema);
    manifest.insert(QStringLiteral("bounds"), QJsonArray{105.78, 20.97, 105.90, 21.10});
    manifest.insert(QStringLiteral("min_zoom"), 12);
    manifest.insert(QStringLiteral("max_zoom"), 15);
    manifest.insert(QStringLiteral("planetiler"), planetiler);
    manifest.insert(QStringLiteral("source_pbf"), sourcePbf);
    manifest.insert(QStringLiteral("attribution"),
                    QStringLiteral("© OpenStreetMap contributors, ODbL 1.0"));
    manifest.insert(QStringLiteral("files"), files);
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
        QCOMPARE(manifest.dataVersion, QStringLiteral("hanoi-20261024-v1"));
        QCOMPARE(manifest.fileSha256.size(), 5);
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

        QTemporaryDir corruptTemporary;
        const QString corruptRoot = createPackage(corruptTemporary);
        QVERIFY(!corruptRoot.isEmpty());
        QFile corrupt(corruptRoot + QStringLiteral("/tiles.mbtiles"));
        QVERIFY(corrupt.open(QIODevice::Append));
        corrupt.write("corrupt");
        corrupt.close();
        QVERIFY(!qtmaps::map::MapPackageManifest::loadAndValidate(
            corruptRoot, &manifest, &error));
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
        QVERIFY(!qtmaps::map::MapPackageManifest::loadAndValidate(
            contractRoot, &manifest, &error));

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
        qtmaps::map::SimulatedNavigationController navigation;
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
