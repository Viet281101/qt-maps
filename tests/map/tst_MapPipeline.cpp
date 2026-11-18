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
#include <QTimer>
#include <QUuid>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <functional>

#include "../fixtures/MapPackageFixture.h"

namespace {

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
    bool startupTimedOut() const override { return timeout; }

    bool succeeds = true;
    bool running = false;
    bool timeout = false;
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

        QTemporaryDir glyphRevisionTemporary;
        const QString glyphRevisionRoot = createPackage(glyphRevisionTemporary);
        QVERIFY(!glyphRevisionRoot.isEmpty());
        mutateManifest(glyphRevisionRoot, [](QJsonObject &object) {
            QJsonObject resources = object.value(QStringLiteral("resources")).toObject();
            resources.insert(QStringLiteral("glyph_revision"),
                             QStringLiteral("noto-sans-regular-v1"));
            object.insert(QStringLiteral("resources"), resources);
        });
        QVERIFY(!qtmaps::map::MapPackageManifest::loadAndValidate(glyphRevisionRoot, &manifest,
                                                                  &error));
        QVERIFY(error.contains(QStringLiteral("metadata is incompatible")));

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
        const QString vectorRoot = createPackage(temporary);
        QVERIFY(!vectorRoot.isEmpty());
        const QString root = createRasterPackage(temporary);
        QVERIFY(!root.isEmpty());
        bindRasterToVector(root, vectorRoot);
        qtmaps::map::RasterPackageManifest manifest;
        QString error;
        QVERIFY2(qtmaps::map::RasterPackageManifest::loadAndValidate(root, &manifest, &error,
                                                                     vectorRoot),
                 qPrintable(error));
        QCOMPARE(manifest.tileCount, 847);
        QCOMPARE(manifest.maximumZoom, 16);
        QCOMPARE(manifest.styleRevision, QStringLiteral("qtmaps-hanoi-v3"));

        QTemporaryDir mismatchTemporary;
        const QString mismatchVectorRoot = createPackage(mismatchTemporary);
        QVERIFY(!mismatchVectorRoot.isEmpty());
        const QString mismatchRoot = createRasterPackage(mismatchTemporary);
        bindRasterToVector(mismatchRoot, mismatchVectorRoot);
        mutateManifest(mismatchRoot, [](QJsonObject &object) {
            QJsonObject vector = object.value(QStringLiteral("source_vector_package")).toObject();
            vector.insert(QStringLiteral("style_revision"), QStringLiteral("future-v3"));
            object.insert(QStringLiteral("source_vector_package"), vector);
        });
        QVERIFY(!qtmaps::map::RasterPackageManifest::loadAndValidate(mismatchRoot, &manifest,
                                                                     &error, mismatchVectorRoot));
        QVERIFY(error.contains(QStringLiteral("style/package")));
    }

    void rasterManifestRejectsDifferentActiveVectorPackage()
    {
        QTemporaryDir packageTemporary;
        const QString vectorRoot = createPackage(packageTemporary);
        QVERIFY(!vectorRoot.isEmpty());
        const QString rasterRoot = createRasterPackage(packageTemporary);
        QVERIFY(!rasterRoot.isEmpty());
        bindRasterToVector(rasterRoot, vectorRoot);

        QTemporaryDir replacementTemporary;
        const QString replacementVectorRoot = createPackage(replacementTemporary);
        QVERIFY(!replacementVectorRoot.isEmpty());
        mutateManifest(replacementVectorRoot, [](QJsonObject &object) {
            object.insert(QStringLiteral("data_version"), QStringLiteral("hanoi-replacement-v3"));
        });

        qtmaps::map::RasterPackageManifest manifest;
        QString error;
        QVERIFY(!qtmaps::map::RasterPackageManifest::loadAndValidate(rasterRoot, &manifest, &error,
                                                                     replacementVectorRoot));
        QVERIFY(error.contains(QStringLiteral("not bound to the active vector package")));
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
        const QString vectorRoot = createPackage(temporary);
        QVERIFY(!vectorRoot.isEmpty());
        const QString root = createRasterPackage(temporary);
        QVERIFY(!root.isEmpty());
        bindRasterToVector(root, vectorRoot);
        qtmaps::map::RasterTileSource source;
        QSignalSpy started(&source, &qtmaps::map::RasterTileSource::started);
        QElapsedTimer elapsed;
        elapsed.start();
        source.startAsync(root, vectorRoot);
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
        const QString vectorRoot = createPackage(temporary);
        QVERIFY(!vectorRoot.isEmpty());
        const QString root = createRasterPackage(temporary);
        QVERIFY(!root.isEmpty());
        bindRasterToVector(root, vectorRoot);
        FakeTileSource vector;
        FakeTileSource diagnostic;
        qtmaps::map::MapSourceController controller(&vector, &diagnostic);
        controller.setRasterPackagePath(root);
        QVERIFY(controller.start(vectorRoot, QString()));
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
        QVERIFY(controller.start(vectorRoot, QString(), true));
        QTRY_COMPARE_WITH_TIMEOUT(controller.layerMode(), QStringLiteral("raster"), 5000);
        QVERIFY(controller.automaticFallback());
        QVERIFY(controller.mapStatusText().contains(QStringLiteral("tự động")));
    }

    void invalidRasterFailsWithoutBlockingCaller()
    {
        QTemporaryDir temporary;
        const QString vectorRoot = createPackage(temporary);
        QVERIFY(!vectorRoot.isEmpty());
        qtmaps::map::RasterTileSource source;
        QSignalSpy failed(&source, &qtmaps::map::RasterTileSource::failed);
        QElapsedTimer elapsed;
        elapsed.start();
        source.startAsync(QStringLiteral("/definitely/missing/raster-package"), vectorRoot);
        QVERIFY(elapsed.elapsed() < 50);
        QVERIFY(failed.wait(5000));
        QVERIFY(source.errorMessage().contains(QStringLiteral("missing")));
    }

    void rasterSourceRejectsDifferentActiveVectorAsynchronously()
    {
        QTemporaryDir packageTemporary;
        const QString vectorRoot = createPackage(packageTemporary);
        QVERIFY(!vectorRoot.isEmpty());
        const QString rasterRoot = createRasterPackage(packageTemporary);
        QVERIFY(!rasterRoot.isEmpty());
        bindRasterToVector(rasterRoot, vectorRoot);

        QTemporaryDir replacementTemporary;
        const QString replacementVectorRoot = createPackage(replacementTemporary);
        QVERIFY(!replacementVectorRoot.isEmpty());
        mutateManifest(replacementVectorRoot, [](QJsonObject &object) {
            object.insert(QStringLiteral("data_version"), QStringLiteral("hanoi-replacement-v3"));
        });

        qtmaps::map::RasterTileSource source;
        QSignalSpy failed(&source, &qtmaps::map::RasterTileSource::failed);
        source.startAsync(rasterRoot, replacementVectorRoot);
        QVERIFY(failed.wait(5000));
        QVERIFY(source.errorMessage().contains(
            QStringLiteral("not bound to the active vector package")));
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

    void vectorTimeoutDoesNotStartRasterAgainstBlockedVector()
    {
        FakeTileSource vector;
        FakeTileSource diagnostic;
        vector.succeeds = false;
        vector.timeout = true;
        qtmaps::map::MapSourceController controller(&vector, &diagnostic);
        controller.setRasterPackagePath(QStringLiteral("/unused/raster"));
        QVERIFY(controller.start(QStringLiteral("/blocked/vector"), QStringLiteral("/diagnostic")));
        QCOMPARE(controller.layerMode(), QStringLiteral("error"));
        QCOMPARE(diagnostic.starts, 0);
    }

    void blockedVectorStartupTimesOutWithoutBlockingEvents()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString root = createPackage(temporary);
        QVERIFY(!root.isEmpty());
        qtmaps::map::MbTilesTileSource source;
        QVERIFY2(source.start(root), qPrintable(source.errorMessage()));
        QVERIFY(source.isRunning());
        QVERIFY(!source.baseUrl().isEmpty());
        const QString glyph = root + QStringLiteral("/glyphs/Noto Sans Regular/0-255.pbf");
        QVERIFY(QFile::remove(glyph));
        QCOMPARE(::mkfifo(QFile::encodeName(glyph).constData(), 0600), 0);
        bool timerFired = false;
        QTimer::singleShot(100, &source, [&]() { timerFired = true; });
        QElapsedTimer elapsed;
        elapsed.start();
        const bool started = source.start(root);
        const qint64 elapsedMs = elapsed.elapsed();
        // Unblock the detached worker so it can finish and clean up.
        const int writer = ::open(QFile::encodeName(glyph).constData(), O_WRONLY | O_NONBLOCK);
        if (writer >= 0) {
            const ssize_t written = ::write(writer, "x", 1);
            Q_UNUSED(written)
            ::close(writer);
        }
        QVERIFY(!started);
        QVERIFY(!source.isRunning());
        QVERIFY(source.baseUrl().isEmpty());
        QVERIFY(timerFired);
        QVERIFY2(elapsedMs < 13000, "Vector startup did not respect its timeout");
        QVERIFY(source.errorMessage().contains(QStringLiteral("timed out")));
        QVERIFY(source.startupTimedOut());
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
