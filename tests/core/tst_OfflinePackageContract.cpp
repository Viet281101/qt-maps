#include "core/OfflinePackageContract.h"
#include "core/OfflinePackageInstaller.h"
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>

#include <limits>
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

    void resolvesAndRollsBackProvisionedBundles()
    {
        const QString fixtureRoot = qEnvironmentVariable("QTMAPS_UNIT13_FIXTURE_ROOT");
        if (fixtureRoot.isEmpty())
            QSKIP("Set QTMAPS_UNIT13_FIXTURE_ROOT for local provisioned-data verification");
        QTemporaryDir activation;
        QVERIFY(activation.isValid());
        QString error;
        qtmaps::core::MapSearchBundleContract contract;
        const QString first = fixtureRoot + QStringLiteral("/bundle-one");
        const QString second = fixtureRoot + QStringLiteral("/bundle-two");
        QVERIFY2(qtmaps::core::OfflinePackageInstaller::validateCandidate(first, &contract, &error),
                 qPrintable(error));
        QVERIFY(qtmaps::core::OfflinePackageInstaller::activateValidatedBundle(
            first, contract, activation.path(), &error));
        QVERIFY2(
            qtmaps::core::OfflinePackageInstaller::validateCandidate(second, &contract, &error),
            qPrintable(error));
        QVERIFY(qtmaps::core::OfflinePackageInstaller::activateValidatedBundle(
            second, contract, activation.path(), &error));
        qtmaps::core::ActiveBundlePaths paths;
        QVERIFY2(qtmaps::core::OfflinePackageInstaller::resolveActiveBundle(activation.path(),
                                                                            &paths, &error),
                 qPrintable(error));
        QCOMPARE(paths.vector, second + QStringLiteral("/vector"));
        QVERIFY(paths.orthography.isEmpty());
        QVERIFY(qtmaps::core::OfflinePackageInstaller::rollback(activation.path(), &error));
        QVERIFY2(qtmaps::core::OfflinePackageInstaller::resolveActiveBundle(activation.path(),
                                                                            &paths, &error),
                 qPrintable(error));
        QCOMPARE(paths.vector, first + QStringLiteral("/vector"));
        QCOMPARE(paths.search, first + QStringLiteral("/search"));
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
