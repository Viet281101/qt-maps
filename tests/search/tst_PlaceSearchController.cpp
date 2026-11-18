#include "search/PlaceSearchController.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../fixtures/SearchPackageFixture.h"

namespace {} // namespace

class PlaceSearchControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void emptyQueryIsExplicitlyIdle()
    {
        qtmaps::search::PlaceSearchController controller;
        controller.setQuery(QStringLiteral(""));
        QCOMPARE(controller.state(), QStringLiteral("idle"));
        QVERIFY(controller.results().isEmpty());
        QVERIFY(controller.message().contains(QStringLiteral("demo")));
    }

    void matchingQueryReturnsBoundedFixture()
    {
        qtmaps::search::PlaceSearchController controller;
        QSignalSpy changed(&controller, &qtmaps::search::PlaceSearchController::stateChanged);
        controller.setQuery(QStringLiteral("thái"));
        QCOMPARE(controller.state(), QStringLiteral("loading"));
        QTRY_COMPARE(controller.state(), QStringLiteral("ready"));
        QCOMPARE(controller.results().size(), 1);
        QCOMPARE(controller.results().constFirst().toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("Thái Hà"));
        QVERIFY(changed.count() >= 2);

        const int completedChangeCount = changed.count();
        controller.setQuery(QStringLiteral("  thái  "));
        QCOMPARE(changed.count(), completedChangeCount);
    }

    void laterQuerySupersedesPendingQuery()
    {
        qtmaps::search::PlaceSearchController controller;
        controller.setQuery(QStringLiteral("Hoàn"));
        controller.setQuery(QStringLiteral("Ba"));
        QTRY_COMPARE(controller.state(), QStringLiteral("ready"));
        QCOMPARE(controller.results().size(), 1);
        QCOMPARE(controller.results().constFirst().toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("Ba Đình"));
    }

    void noMatchIsExplicit()
    {
        qtmaps::search::PlaceSearchController controller;
        controller.setQuery(QStringLiteral("không tồn tại"));
        QTRY_COMPARE(controller.state(), QStringLiteral("empty"));
        QVERIFY(controller.results().isEmpty());
        QVERIFY(controller.message().contains(QStringLiteral("Không có")));
    }

    void validPackageIsLoadedAndQueriedOffThread()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString package = createSearchPackage(&temporary);
        QVERIFY(!package.isEmpty());
        qtmaps::search::PlaceSearchController controller(package);
        QTRY_VERIFY(controller.sourceLabel().contains(QStringLiteral("OpenStreetMap")));
        controller.setQuery(QStringLiteral("hoan kiem"));
        QTRY_COMPARE(controller.state(), QStringLiteral("ready"));
        QCOMPARE(controller.results().size(), 1);
        const QVariantMap result = controller.results().constFirst().toMap();
        QCOMPARE(result.value(QStringLiteral("name")).toString(), QStringLiteral("Hoàn Kiếm"));
        QVERIFY(result.value(QStringLiteral("coordinate"))
                    .toString()
                    .contains(QStringLiteral("21.028000")));
    }

    void invalidPackageKeepsFixtureSearchVisible()
    {
        QTemporaryDir temporary;
        qtmaps::search::PlaceSearchController controller(
            temporary.filePath(QStringLiteral("missing")));
        QTRY_VERIFY(controller.sourceLabel().contains(QStringLiteral("demo")));
        controller.setQuery(QStringLiteral("Hoàn"));
        QTRY_COMPARE(controller.state(), QStringLiteral("ready"));
        QCOMPARE(controller.results().constFirst().toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("Hoàn Kiếm"));
    }

    void packageValidationReportsValidMissingAndCorruptPackages()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString package = createSearchPackage(&temporary);
        QVERIFY(!package.isEmpty());
        QString error;
        QVERIFY(qtmaps::search::PlaceSearchController::validatePackage(package, &error));
        QVERIFY(error.isEmpty());

        QVERIFY(!qtmaps::search::PlaceSearchController::validatePackage(
            temporary.filePath(QStringLiteral("missing")), &error));
        QVERIFY(!error.isEmpty());

        QFile database(package + QStringLiteral("/places.sqlite"));
        QVERIFY(database.open(QIODevice::Append));
        database.write("corrupt");
        database.close();
        QVERIFY(!qtmaps::search::PlaceSearchController::validatePackage(package, &error));
        QVERIFY(!error.isEmpty());
    }

    void packageValidationRejectsValidChecksumWithoutFtsSchema()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString package = createSearchPackage(&temporary);
        QVERIFY(!package.isEmpty());
        const QString databasePath = package + QStringLiteral("/places.sqlite");
        const QString connection = QStringLiteral("search-test-missing-fts");
        {
            QSqlDatabase database =
                QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
            database.setDatabaseName(databasePath);
            QVERIFY(database.open());
            QSqlQuery query(database);
            QVERIFY(query.exec(QStringLiteral("DROP TABLE places_fts")));
            database.close();
        }
        QSqlDatabase::removeDatabase(connection);
        QFile manifestFile(package + QStringLiteral("/manifest.json"));
        QVERIFY(manifestFile.open(QIODevice::ReadOnly));
        QJsonObject manifest = QJsonDocument::fromJson(manifestFile.readAll()).object();
        manifestFile.close();
        QJsonObject places = manifest.value(QStringLiteral("places")).toObject();
        places.insert(QStringLiteral("size_bytes"), QFileInfo(databasePath).size());
        places.insert(QStringLiteral("sha256"), sha256(databasePath));
        manifest.insert(QStringLiteral("places"), places);
        QVERIFY(manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
        manifestFile.write(QJsonDocument(manifest).toJson());
        manifestFile.close();
        QString error;
        QVERIFY(!qtmaps::search::PlaceSearchController::validatePackage(package, &error));
        QVERIFY(!error.isEmpty());
    }
};

QTEST_MAIN(PlaceSearchControllerTest)

#include "tst_PlaceSearchController.moc"
