#include "core/OfflinePackageContract.h"
#include "core/OfflinePackageInstaller.h"
#include "map/MapPackageManifest.h"
#include "map/RasterPackageManifest.h"
#include "search/PlaceSearchController.h"
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLibraryInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

#include "../fixtures/MapPackageFixture.h"
#include "../fixtures/SearchPackageFixture.h"
#include <atomic>
#include <functional>
#include <limits>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

namespace {

bool writeJson(const QString &p, const QJsonObject &o)
{
    QFile f(p);
    return f.open(QIODevice::WriteOnly) && f.write(QJsonDocument(o).toJson()) > 0;
}
} // namespace
class OfflinePackageContractTest final : public QObject
{
    Q_OBJECT

private slots:
    void acceptsLocalBundle()
    {
        QTemporaryDir d;
        QVERIFY(d.isValid());
        QVERIFY(writeJson(
            d.path() + "/bundle.json",
            {{"schema_version", "qtmaps-local-map-search-bundle-v1"},
             {"bundle_id", "hanoi-r1"},
             {"components",
              QJsonObject{{"vector", "vector"}, {"raster", "raster"}, {"search", "search"}}},
             {"source", QJsonObject{{"name", "owner media"}, {"retrieved_on", "2026-11-17"}}},
             {"attribution", "© OpenStreetMap contributors"},
             {"installed_size_bytes", 1234}}));
        qtmaps::core::MapSearchBundleContract c;
        QString e;
        QVERIFY2(qtmaps::core::MapSearchBundleContract::loadAndValidate(d.path(), &c, &e),
                 qPrintable(e));
        QCOMPARE(c.searchPath, QString("search"));
    }
    void rejectsUnsafeGraphPath()
    {
        QTemporaryDir d;
        QVERIFY(d.isValid());
        const QString p = d.path() + "/routing.json";
        QVERIFY(writeJson(p, {{"schema_version", "qtmaps-routing-graph-activation-v1"},
                              {"graph_id", "vietnam-r1"},
                              {"graph_path", "../graphs"},
                              {"source_pbf_sha256", QString(64, QLatin1Char('a'))},
                              {"attribution", "© OpenStreetMap contributors"},
                              {"installed_size_bytes", 42}}));
        qtmaps::core::RoutingGraphActivationContract c;
        QString e;
        QVERIFY(!qtmaps::core::RoutingGraphActivationContract::loadAndValidate(p, &c, &e));
        QVERIFY(!e.isEmpty());
    }

    void atomicallyActivatesAndRollsBackValidatedBundles()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QDir directory(root.path());
        QVERIFY(directory.mkdir(QStringLiteral("first")));
        QVERIFY(directory.mkdir(QStringLiteral("second")));
        const QString activation = root.path() + QStringLiteral("/activation");
        qtmaps::core::MapSearchBundleContract contract;
        contract.bundleId = QStringLiteral("test");
        QString error;
        QVERIFY(qtmaps::core::OfflinePackageInstaller::activateValidatedBundle(
            root.path() + QStringLiteral("/first"), contract, activation, &error));
        QVERIFY(qtmaps::core::OfflinePackageInstaller::activateValidatedBundle(
            root.path() + QStringLiteral("/second"), contract, activation, &error));
        QCOMPARE(QFileInfo(activation + QStringLiteral("/current")).canonicalFilePath(),
                 root.path() + QStringLiteral("/second"));
        QVERIFY(qtmaps::core::OfflinePackageInstaller::rollback(activation, &error));
        QCOMPARE(QFileInfo(activation + QStringLiteral("/current")).canonicalFilePath(),
                 root.path() + QStringLiteral("/first"));
    }

    void resolvesAndRollsBackBundlesWithoutExternalData()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QString first = root.path() + QStringLiteral("/first");
        const QString second = root.path() + QStringLiteral("/second");
        QVERIFY(QDir().mkpath(first));
        QVERIFY(QDir().mkpath(second));
        const QJsonObject metadata{
            {"schema_version", "qtmaps-local-map-search-bundle-v1"},
            {"bundle_id", "synthetic"},
            {"components",
             QJsonObject{{"vector", "vector"}, {"raster", "raster"}, {"search", "search"}}},
            {"source", QJsonObject{{"name", "synthetic"}, {"retrieved_on", "2026-11-18"}}},
            {"attribution", "OpenStreetMap"},
            {"installed_size_bytes", 0}};
        QVERIFY(writeJson(first + QStringLiteral("/bundle.json"), metadata));
        QVERIFY(writeJson(second + QStringLiteral("/bundle.json"), metadata));
        qtmaps::core::MapSearchBundleContract contract;
        QString error;
        QVERIFY(qtmaps::core::MapSearchBundleContract::loadAndValidate(first, &contract, &error));
        const QString activation = root.path() + QStringLiteral("/activation");
        QVERIFY(qtmaps::core::OfflinePackageInstaller::activateValidatedBundle(first, contract,
                                                                               activation, &error));
        QVERIFY(qtmaps::core::OfflinePackageInstaller::activateValidatedBundle(second, contract,
                                                                               activation, &error));
        qtmaps::core::ActiveBundlePaths paths;
        QVERIFY2(
            qtmaps::core::OfflinePackageInstaller::resolveActiveBundle(activation, &paths, &error),
            qPrintable(error));
        QCOMPARE(paths.vector, second + QStringLiteral("/vector"));
        QVERIFY(paths.orthography.isEmpty());
        QVERIFY(qtmaps::core::OfflinePackageInstaller::rollback(activation, &error));
        QVERIFY2(
            qtmaps::core::OfflinePackageInstaller::resolveActiveBundle(activation, &paths, &error),
            qPrintable(error));
        QCOMPARE(paths.vector, first + QStringLiteral("/vector"));
        QCOMPARE(paths.search, first + QStringLiteral("/search"));
        // Missing optional search does not alter the vector path; each consumer validates
        // independently.
        QVERIFY(!QFileInfo::exists(paths.search));
    }

    void validatesRealSyntheticComponentsAndKeepsMapWhenSearchBreaks()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QString vector = createPackage(root);
        const QString raster = createRasterPackage(root);
        const QString search = createSearchPackage(&root);
        QVERIFY(!vector.isEmpty());
        QVERIFY(!raster.isEmpty());
        QVERIFY(!search.isEmpty());
        bindRasterToVector(raster, vector);
        const QString bundle = root.path() + QStringLiteral("/bundle");
        QVERIFY(QDir().mkpath(bundle));
        QVERIFY(QFile::link(vector, bundle + "/vector"));
        QVERIFY(QFile::link(raster, bundle + "/raster"));
        QVERIFY(QFile::link(search, bundle + "/search"));
        QVERIFY(writeJson(
            bundle + "/bundle.json",
            {{"schema_version", "qtmaps-local-map-search-bundle-v1"},
             {"bundle_id", "synthetic-full"},
             {"components",
              QJsonObject{{"vector", "vector"}, {"raster", "raster"}, {"search", "search"}}},
             {"source", QJsonObject{{"name", "fixture"}, {"retrieved_on", "2026-11-18"}}},
             {"attribution", "OpenStreetMap"},
             {"installed_size_bytes", 0}}));
        qtmaps::core::MapSearchBundleContract contract;
        QString error;
        QVERIFY2(
            qtmaps::core::OfflinePackageInstaller::validateCandidate(bundle, &contract, &error),
            qPrintable(error));
        const QString activation = root.path() + QStringLiteral("/activation");
        QVERIFY(qtmaps::core::OfflinePackageInstaller::activateValidatedBundle(bundle, contract,
                                                                               activation, &error));
        QVERIFY(QFile::remove(search + "/manifest.json"));
        QFile metadataFile(bundle + "/bundle.json");
        QVERIFY(metadataFile.open(QIODevice::ReadOnly));
        QJsonObject metadata = QJsonDocument::fromJson(metadataFile.readAll()).object();
        metadataFile.close();
        QJsonObject components = metadata.value("components").toObject();
        components.insert("orthography", "broken-orthography");
        metadata.insert("components", components);
        QVERIFY(writeJson(bundle + "/bundle.json", metadata));
        qtmaps::core::ActiveBundlePaths paths;
        QVERIFY2(
            qtmaps::core::OfflinePackageInstaller::resolveActiveBundle(activation, &paths, &error),
            qPrintable(error));
        qtmaps::map::MapPackageManifest vectorManifest;
        qtmaps::map::RasterPackageManifest rasterManifest;
        QVERIFY2(
            qtmaps::map::MapPackageManifest::loadAndValidate(paths.vector, &vectorManifest, &error),
            qPrintable(error));
        QVERIFY2(qtmaps::map::RasterPackageManifest::loadAndValidate(paths.raster, &rasterManifest,
                                                                     &error, paths.vector),
                 qPrintable(error));
        QVERIFY(!qtmaps::search::PlaceSearchController::validatePackage(paths.search, &error));
        QVERIFY(!qtmaps::map::RasterPackageManifest::loadAndValidateOrthography(
            paths.orthography, &rasterManifest, &error));
        // A blocked vector file must be cancellable before the synchronous map loader starts.
        const QString glyph = vector + QStringLiteral("/glyphs/Noto Sans Regular/0-255.pbf");
        QVERIFY(QFile::remove(glyph));
        QCOMPARE(::mkfifo(QFile::encodeName(glyph).constData(), 0600), 0);
        QProcess process;
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
        environment.insert(QStringLiteral("QT_QUICK_BACKEND"), QStringLiteral("software"));
        process.setProcessEnvironment(environment);
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(QCoreApplication::applicationDirPath() + QStringLiteral("/qtmaps"),
                      {QStringLiteral("--unit03-local-map"),
                       QStringLiteral("--routing-fixture-only"),
                       QStringLiteral("--bundle-activation-root"), activation,
                       QStringLiteral("--disable-instrumentation"), QStringLiteral("--exit-after"),
                       QStringLiteral("1")});
        QVERIFY(process.waitForStarted(3000));
        if (!process.waitForFinished(6000)) {
            process.kill();
            process.waitForFinished();
            QFAIL("Blocked vector startup did not respond to exit-after");
        }
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);
    }

    void explicitVectorSurvivesStalledBundleMetadata()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QString vector = createPackage(root);
        QVERIFY(!vector.isEmpty());
        const QString bundle = root.path() + QStringLiteral("/blocked-bundle");
        const QString activation = root.path() + QStringLiteral("/activation");
        QVERIFY(QDir().mkpath(bundle));
        QVERIFY(QDir().mkpath(activation));
        QCOMPARE(::mkfifo(QFile::encodeName(bundle + "/bundle.json").constData(), 0600), 0);
        QVERIFY(QFile::link(bundle, activation + "/current"));
        const QString tiles = root.path() + QStringLiteral("/graph-tiles");
        const QString graphActivation = root.path() + QStringLiteral("/graph-activation");
        QVERIFY(QDir().mkpath(tiles));
        QVERIFY(QDir().mkpath(graphActivation));
        QFile tile(tiles + "/0.gph");
        QVERIFY(tile.open(QIODevice::WriteOnly));
        QCOMPARE(tile.write("graph"), qint64(5));
        tile.close();
        const QString graphContract = root.path() + QStringLiteral("/routing.json");
        QVERIFY(writeJson(graphContract, {{"schema_version", "qtmaps-routing-graph-activation-v1"},
                                          {"graph_id", "fixture"},
                                          {"graph_path", "graph-tiles"},
                                          {"source_pbf_sha256", QString(64, QLatin1Char('a'))},
                                          {"attribution", "OpenStreetMap"},
                                          {"installed_size_bytes", 5}}));
        QVERIFY(QFile::link(graphContract, graphActivation + "/routing-current.json"));
        const QString config = root.path() + QStringLiteral("/valhalla.json");
        QVERIFY(writeJson(
            config, {{"mjolnir", QJsonObject{{"tile_dir", tiles}}},
                     {"httpd", QJsonObject{{"service",
                                            QJsonObject{{"listen", "tcp://127.0.0.1:18002"}}}}}}));
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert("QT_QPA_PLATFORM", "offscreen");
        environment.insert("QT_QUICK_BACKEND", "software");
        QString maplibre = qEnvironmentVariable("QTMAPS_MAPLIBRE_INSTALL");
        if (maplibre.isEmpty())
            maplibre = QDir(QCoreApplication::applicationDirPath())
                           .absoluteFilePath("../generated/unit-01b/maplibre-install-qt51518");
        environment.insert("LD_LIBRARY_PATH",
                           maplibre + "/lib:" + environment.value("LD_LIBRARY_PATH"));
        environment.insert("QT_PLUGIN_PATH", maplibre + "/plugins:" +
                                                 QLibraryInfo::location(QLibraryInfo::PluginsPath));
        environment.insert("QML2_IMPORT_PATH",
                           maplibre +
                               "/qml:" + QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath));
        auto check = [&](bool allOverrides) {
            QProcess process;
            process.setProcessEnvironment(environment);
            process.setProcessChannelMode(QProcess::MergedChannels);
            QStringList args{"--unit03-local-map",
                             "--routing-demo",
                             "--bundle-activation-root",
                             activation,
                             "--routing-activation-root",
                             graphActivation,
                             "--map-package",
                             vector,
                             "--valhalla-config",
                             config,
                             "--valhalla-service",
                             root.path() + "/missing-service",
                             "--disable-instrumentation"};
            if (allOverrides)
                args << "--raster-package" << (root.path() + "/raster") << "--search-package"
                     << (root.path() + "/search") << "--orthography-package"
                     << (root.path() + "/orthography");
            process.start(QCoreApplication::applicationDirPath() + "/qtmaps", args);
            QVERIFY(process.waitForStarted(3000));
            QElapsedTimer elapsed;
            elapsed.start();
            QByteArray output;
            while (elapsed.elapsed() < 17000 && process.state() != QProcess::NotRunning) {
                process.waitForReadyRead(200);
                output += process.readAll();
                if (output.contains("probeMode= unit04-routing-valhalla"))
                    break;
            }
            const qint64 totalMs = elapsed.elapsed();
            if (process.state() != QProcess::NotRunning) {
                process.terminate();
                if (!process.waitForFinished(3000)) {
                    process.kill();
                    process.waitForFinished();
                }
            }
            output += process.readAll();
            QVERIFY2(output.contains("probeMode= unit04-routing-valhalla"), output.constData());
            QVERIFY2(output.contains("selected: true"), output.constData());
            QVERIFY(!output.contains("Active routing graph is invalid"));
            if (allOverrides) {
                QVERIFY2(totalMs < 7000,
                         "Fully overridden components should skip the blocked bundle");
                QVERIFY(!output.contains("Startup data check timed out"));
            } else {
                QVERIFY(output.contains("Startup data check timed out"));
            }
        };
        check(true);
        check(false);
    }

    void pointerSwitchNeverMixesBundleAndMetadata()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QString first = root.path() + QStringLiteral("/first");
        const QString second = root.path() + QStringLiteral("/second");
        const QString activation = root.path() + QStringLiteral("/activation");
        QVERIFY(QDir().mkpath(first));
        QVERIFY(QDir().mkpath(second));
        QVERIFY(QDir().mkpath(activation));
        auto metadata = [](const QString &component) {
            return QJsonObject{
                {"schema_version", "qtmaps-local-map-search-bundle-v1"},
                {"bundle_id", component},
                {"components",
                 QJsonObject{{"vector", component}, {"raster", "raster"}, {"search", "search"}}},
                {"source", QJsonObject{{"name", "fixture"}, {"retrieved_on", "2026-11-18"}}},
                {"attribution", "OpenStreetMap"},
                {"installed_size_bytes", 0}};
        };
        QVERIFY(writeJson(first + "/bundle.json", metadata("vector-first")));
        QVERIFY(writeJson(second + "/bundle.json", metadata("vector-second")));
        const QByteArray current = QFile::encodeName(activation + "/current");
        const QByteArray staged = QFile::encodeName(activation + "/next");
        const QByteArray firstBytes = QFile::encodeName(first);
        const QByteArray secondBytes = QFile::encodeName(second);
        QCOMPARE(::symlink(firstBytes.constData(), current.constData()), 0);
        std::atomic<bool> stop{false};
        std::atomic<bool> switchFailed{false};
        std::thread switcher([&]() {
            bool useFirst = false;
            while (!stop.load()) {
                const QByteArray &target = useFirst ? firstBytes : secondBytes;
                if (::symlink(target.constData(), staged.constData()) != 0 ||
                    ::rename(staged.constData(), current.constData()) != 0) {
                    switchFailed = true;
                    break;
                }
                useFirst = !useFirst;
            }
        });
        bool consistent = true;
        QString error;
        for (int i = 0; i < 500; ++i) {
            qtmaps::core::ActiveBundlePaths paths;
            if (!qtmaps::core::OfflinePackageInstaller::resolveActiveBundle(activation, &paths,
                                                                            &error) ||
                (paths.vector != first + "/vector-first" &&
                 paths.vector != second + "/vector-second")) {
                consistent = false;
                break;
            }
        }
        stop = true;
        switcher.join();
        QVERIFY(!switchFailed.load());
        QVERIFY2(consistent, qPrintable(error));
    }

    void rejectsMissingAndCorruptActiveBundle()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        qtmaps::core::ActiveBundlePaths paths;
        paths.vector = QStringLiteral("stale");
        QString error;
        QVERIFY(!qtmaps::core::OfflinePackageInstaller::resolveActiveBundle(root.path(), &paths,
                                                                            &error));
        QVERIFY(paths.vector.isEmpty());
        QVERIFY(!error.isEmpty());
        QVERIFY(QDir(root.path()).mkdir(QStringLiteral("current")));
        QVERIFY(writeJson(root.path() + QStringLiteral("/current/bundle.json"),
                          {{"schema_version", "bad"}}));
        QVERIFY(!qtmaps::core::OfflinePackageInstaller::resolveActiveBundle(root.path(), &paths,
                                                                            &error));
        QVERIFY(paths.vector.isEmpty());
    }

    void rejectsBundleWhenDeclaredStorageCannotFit()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QVERIFY(QDir(root.path()).mkdir(QStringLiteral("bundle")));
        qtmaps::core::MapSearchBundleContract contract;
        contract.bundleId = QStringLiteral("oversized");
        contract.installedSizeBytes = std::numeric_limits<qint64>::max();
        QString error;
        QVERIFY(!qtmaps::core::OfflinePackageInstaller::activateValidatedBundle(
            root.path() + QStringLiteral("/bundle"), contract,
            root.path() + QStringLiteral("/activation"), &error));
        QVERIFY(error.contains(QStringLiteral("Insufficient storage")));
    }

    void runtimeConfigSelectsGraphWithoutChangingSource()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QVERIFY(QDir(root.path()).mkdir(QStringLiteral("tiles")));
        const QString source = root.path() + QStringLiteral("/source.json");
        const QString destination = root.path() + QStringLiteral("/runtime.json");
        const QJsonObject original{
            {"mjolnir", QJsonObject{{"tile_dir", "old"}, {"other", 7}}},
            {"httpd", QJsonObject{{"service", QJsonObject{{"listen", "tcp://127.0.0.1:18002"}}}}}};
        QVERIFY(writeJson(source, original));
        QString error;
        QVERIFY2(qtmaps::core::OfflinePackageInstaller::writeRoutingRuntimeConfig(
                     source, destination, root.path() + "/tiles", 18002, &error),
                 qPrintable(error));
        QVERIFY(qtmaps::core::OfflinePackageInstaller::routingConfigSelectsGraph(
            destination, root.path() + "/tiles", &error));
        QVERIFY(!qtmaps::core::OfflinePackageInstaller::routingConfigSelectsGraph(
            source, root.path() + "/tiles", &error));
        QFile file(source);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(QJsonDocument::fromJson(file.readAll()).object(), original);
        QFile runtime(destination);
        QVERIFY(runtime.open(QIODevice::ReadOnly));
        QJsonObject expected = original;
        QJsonObject selectedMjolnir = expected.value("mjolnir").toObject();
        selectedMjolnir.insert("tile_dir", QFileInfo(root.path() + "/tiles").canonicalFilePath());
        expected.insert("mjolnir", selectedMjolnir);
        QCOMPARE(QJsonDocument::fromJson(runtime.readAll()).object(), expected);
        QVERIFY(!qtmaps::core::OfflinePackageInstaller::writeRoutingRuntimeConfig(
            source, source, root.path() + "/tiles", 18002, &error));
        QVERIFY(!qtmaps::core::OfflinePackageInstaller::writeRoutingRuntimeConfig(
            source, destination, root.path() + "/tiles", 18003, &error));
        QJsonObject withExtract = original;
        QJsonObject mjolnir = withExtract.value("mjolnir").toObject();
        mjolnir.insert("tile_extract", "other.tar");
        withExtract.insert("mjolnir", mjolnir);
        QVERIFY(writeJson(source, withExtract));
        QVERIFY(!qtmaps::core::OfflinePackageInstaller::writeRoutingRuntimeConfig(
            source, destination, root.path() + "/tiles", 18002, &error));
    }

    void activatesAndReportsSeparateRoutingGraph()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QDir directory(root.path());
        QVERIFY(directory.mkdir(QStringLiteral("tiles")));
        QFile graph(root.path() + QStringLiteral("/tiles/0.gph"));
        QVERIFY(graph.open(QIODevice::WriteOnly));
        QVERIFY(graph.write("graph") > 0);
        graph.close();
        const QString contractPath = root.path() + QStringLiteral("/routing.json");
        QVERIFY(writeJson(contractPath, {{"schema_version", "qtmaps-routing-graph-activation-v1"},
                                         {"graph_id", "vietnam-r1"},
                                         {"graph_path", "tiles"},
                                         {"source_pbf_sha256", QString(64, QLatin1Char('a'))},
                                         {"attribution", "© OpenStreetMap contributors"},
                                         {"installed_size_bytes", 5}}));
        qtmaps::core::RoutingGraphActivationContract contract;
        QString error;
        QVERIFY(qtmaps::core::OfflinePackageInstaller::validateRoutingGraph(contractPath, &contract,
                                                                            &error));
        const QString activation = root.path() + QStringLiteral("/activation");
        QVERIFY(qtmaps::core::OfflinePackageInstaller::activateValidatedRoutingGraph(
            contractPath, contract, activation, &error));
        QVERIFY(qtmaps::core::OfflinePackageInstaller::activateValidatedRoutingGraph(
            contractPath, contract, activation, &error));
        QVERIFY(qtmaps::core::OfflinePackageInstaller::rollbackRoutingGraph(activation, &error));
        QCOMPARE(qtmaps::core::OfflinePackageInstaller::activeRoutingGraphPath(activation, &error),
                 root.path() + QStringLiteral("/tiles"));
    }
};
QTEST_MAIN(OfflinePackageContractTest)
#include "tst_OfflinePackageContract.moc"
