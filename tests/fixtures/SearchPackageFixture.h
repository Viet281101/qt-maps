#pragma once

// Shared synthetic FTS5 search package builder.
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
