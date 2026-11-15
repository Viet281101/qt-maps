#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

namespace {

bool execute(QSqlQuery &query, const QString &statement)
{
    if (query.exec(statement))
        return true;
    QTest::qFail(qPrintable(query.lastError().text()), __FILE__, __LINE__);
    return false;
}

bool insertPlace(QSqlQuery &query, const QString &name, const QString &aliases, qint64 sourceId)
{
    query.prepare(QStringLiteral("INSERT INTO places(source_id, name, aliases) VALUES (?, ?, ?)"));
    query.addBindValue(sourceId);
    query.addBindValue(name);
    query.addBindValue(aliases);
    if (query.exec())
        return true;
    QTest::qFail(qPrintable(query.lastError().text()), __FILE__, __LINE__);
    return false;
}

} // namespace

class SearchFts5SpikeTest final : public QObject
{
    Q_OBJECT

private slots:
    void qsqliteSupportsBoundVietnameseFts5Query()
    {
        QTemporaryDir temporaryDirectory;
        QVERIFY(temporaryDirectory.isValid());
        const QString connectionName =
            QStringLiteral("qtmaps-search-fts5-%1").arg(QUuid::createUuid().toString());
        {
            QSqlDatabase database =
                QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
            database.setDatabaseName(temporaryDirectory.filePath(QStringLiteral("places.sqlite")));
            QVERIFY2(database.open(), qPrintable(database.lastError().text()));

            QSqlQuery query(database);
            QVERIFY(execute(
                query,
                QStringLiteral(
                    "CREATE TABLE places (source_id INTEGER PRIMARY KEY, name TEXT NOT NULL, "
                    "aliases TEXT NOT NULL)")));
            QVERIFY(execute(
                query, QStringLiteral("CREATE VIRTUAL TABLE places_fts USING fts5("
                                      "name, aliases, content='places', content_rowid='source_id', "
                                      "tokenize='unicode61 remove_diacritics 2')")));
            QVERIFY(execute(query,
                            QStringLiteral("CREATE TRIGGER places_ai AFTER INSERT ON places BEGIN "
                                           "INSERT INTO places_fts(rowid, name, aliases) "
                                           "VALUES (new.source_id, new.name, new.aliases); END")));
            QVERIFY(execute(
                query,
                QStringLiteral("CREATE TRIGGER places_ad AFTER DELETE ON places BEGIN "
                               "INSERT INTO places_fts(places_fts, rowid, name, aliases) "
                               "VALUES ('delete', old.source_id, old.name, old.aliases); END")));
            QVERIFY(execute(
                query, QStringLiteral("CREATE TRIGGER places_au AFTER UPDATE ON places BEGIN "
                                      "INSERT INTO places_fts(places_fts, rowid, name, aliases) "
                                      "VALUES ('delete', old.source_id, old.name, old.aliases); "
                                      "INSERT INTO places_fts(rowid, name, aliases) "
                                      "VALUES (new.source_id, new.name, new.aliases); END")));
            QVERIFY(
                insertPlace(query, QStringLiteral("Hoàn Kiếm"), QStringLiteral("Hồ Hoàn Kiếm"), 1));
            QVERIFY(
                insertPlace(query, QStringLiteral("Thái Hà"), QStringLiteral("Phố Thái Hà"), 2));
            QVERIFY(insertPlace(query, QStringLiteral("Điểm A"), QStringLiteral("Hà Nội"), 3));
            QVERIFY(insertPlace(query, QStringLiteral("Điểm B"), QStringLiteral("Hà Nội"), 4));

            query.prepare(
                QStringLiteral("SELECT places.source_id, places.name FROM places_fts "
                               "JOIN places ON places.source_id = places_fts.rowid "
                               "WHERE places_fts MATCH ? "
                               "ORDER BY bm25(places_fts, 10.0, 3.0), places.source_id LIMIT ?"));
            query.addBindValue(QStringLiteral("hoan AND kiem"));
            query.addBindValue(10);
            QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
            QVERIFY(query.next());
            QCOMPARE(query.value(0).toLongLong(), 1LL);
            QCOMPARE(query.value(1).toString(), QStringLiteral("Hoàn Kiếm"));
            QVERIFY(!query.next());

            query.finish();
            query.prepare(
                QStringLiteral("SELECT places.source_id FROM places_fts "
                               "JOIN places ON places.source_id = places_fts.rowid "
                               "WHERE places_fts MATCH ? "
                               "ORDER BY bm25(places_fts, 10.0, 3.0), places.source_id LIMIT ?"));
            query.addBindValue(QStringLiteral("ha AND noi"));
            query.addBindValue(10);
            QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
            QVERIFY(query.next());
            QCOMPARE(query.value(0).toLongLong(), 3LL);
            QVERIFY(query.next());
            QCOMPARE(query.value(0).toLongLong(), 4LL);
            QVERIFY(!query.next());
            database.close();
        }
        QSqlDatabase::removeDatabase(connectionName);
        QVERIFY(QFileInfo::exists(temporaryDirectory.filePath(QStringLiteral("places.sqlite"))));
    }
};

QTEST_MAIN(SearchFts5SpikeTest)

#include "tst_SearchFts5Spike.moc"
