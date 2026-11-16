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

namespace {

QString sha256(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&file);
    return QString::fromLatin1(hash.result().toHex());
}

QString createSearchPackage(QTemporaryDir *temporary)
{
    const QString package = temporary->filePath(QStringLiteral("package"));
    if (!QDir().mkpath(package))
        return QString();
    const QString databasePath = package + QStringLiteral("/places.sqlite");
    const QString connection =
        QStringLiteral("search-test-%1").arg(reinterpret_cast<quintptr>(temporary));
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(databasePath);
        if (!database.open())
            return QString();
        QSqlQuery query(database);
        if (!query.exec(
                QStringLiteral("CREATE TABLE places (id INTEGER PRIMARY KEY, source_id TEXT UNIQUE "
                               "NOT NULL, name TEXT NOT NULL, aliases TEXT NOT NULL, category TEXT "
                               "NOT NULL, latitude REAL NOT NULL, longitude REAL NOT NULL);")) ||
            !query.exec(QStringLiteral(
                "CREATE VIRTUAL TABLE places_fts USING fts5(name, aliases, content='places', "
                "content_rowid='id', tokenize='unicode61 remove_diacritics 2');")) ||
            !query.exec(QStringLiteral(
                "INSERT INTO places(source_id,name,aliases,category,latitude,longitude) VALUES "
                "('n1','Hoàn Kiếm','Hồ Gươm','place',21.028,105.852);")) ||
            !query.exec(QStringLiteral("INSERT INTO places_fts(places_fts) VALUES ('rebuild');")))
            return QString();
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
    const QJsonObject manifest{
        {QStringLiteral("schema_version"), QStringLiteral("qtmaps-search-package-v1")},
        {QStringLiteral("attribution"), QStringLiteral("© OpenStreetMap contributors, ODbL 1.0")},
        {QStringLiteral("bounds"), QJsonArray{105.78, 20.97, 105.90, 21.10}},
        {QStringLiteral("source"),
         QJsonObject{{QStringLiteral("url"),
                      QStringLiteral("https://download.geofabrik.de/asia/vietnam-latest.osm.pbf")},
                     {QStringLiteral("pbf_sha256"), QStringLiteral("fixture")},
                     {QStringLiteral("retrieved_on"), QStringLiteral("2026-10-23")}}},
        {QStringLiteral("places"),
         QJsonObject{{QStringLiteral("path"), QStringLiteral("places.sqlite")},
                     {QStringLiteral("count"), 1},
                     {QStringLiteral("size_bytes"), QFileInfo(databasePath).size()},
                     {QStringLiteral("sha256"), sha256(databasePath)}}}};
    QFile manifestFile(package + QStringLiteral("/manifest.json"));
    if (!manifestFile.open(QIODevice::WriteOnly))
        return QString();
    manifestFile.write(QJsonDocument(manifest).toJson());
    return package;
}

} // namespace

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
};

QTEST_MAIN(PlaceSearchControllerTest)

#include "tst_PlaceSearchController.moc"
