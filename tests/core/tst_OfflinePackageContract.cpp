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
