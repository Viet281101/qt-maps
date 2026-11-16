#include "PlaceSearchController.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QVariantMap>

namespace qtmaps {
namespace search {
namespace {

struct Place
{
    const char *name;
    const char *detail;
};

const Place kPlaces[] = {{"Hoàn Kiếm", "Hồ Hoàn Kiếm · dữ liệu demo"},
                         {"Thái Hà", "Phố Thái Hà · dữ liệu demo"},
                         {"Ba Đình", "Quận Ba Đình · dữ liệu demo"}};

QString fileDigest(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd())
        hash.addData(file.read(1024 * 1024));
    return QString::fromLatin1(hash.result().toHex());
}

QString ftsPrefixQuery(const QString &input)
{
    const QStringList terms = input.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QStringList quoted;
    for (QString term : terms) {
        term.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        quoted.append(QStringLiteral("\"%1\"*").arg(term));
    }
    return quoted.join(QStringLiteral(" AND "));
}

bool validateSearchPackage(const QString &packagePath, QString *errorMessage)
{
    QString error;
    QFileInfo packageInfo(packagePath);
    const QString packageDirectory = packageInfo.canonicalFilePath();
    const QString manifestPath = QDir(packageDirectory).filePath(QStringLiteral("manifest.json"));
    const QString databasePath = QDir(packageDirectory).filePath(QStringLiteral("places.sqlite"));
    QFile manifestFile(manifestPath);
    if (!manifestFile.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Không tìm thấy gói tìm kiếm ngoại tuyến");
    } else {
        const QJsonObject manifest = QJsonDocument::fromJson(manifestFile.readAll()).object();
        const QJsonObject places = manifest.value(QStringLiteral("places")).toObject();
        const QJsonObject source = manifest.value(QStringLiteral("source")).toObject();
        const QJsonArray bounds = manifest.value(QStringLiteral("bounds")).toArray();
        const bool contractMatches =
            manifest.value(QStringLiteral("schema_version")).toString() ==
                QStringLiteral("qtmaps-search-package-v1") &&
            manifest.value(QStringLiteral("attribution")).toString() ==
                QStringLiteral("© OpenStreetMap contributors, ODbL 1.0") &&
            places.value(QStringLiteral("path")).toString() == QStringLiteral("places.sqlite") &&
            places.value(QStringLiteral("count")).toInt() > 0 &&
            source.value(QStringLiteral("url")).toString() ==
                QStringLiteral("https://download.geofabrik.de/asia/vietnam-latest.osm.pbf") &&
            !source.value(QStringLiteral("pbf_sha256")).toString().isEmpty() &&
            !source.value(QStringLiteral("retrieved_on")).toString().isEmpty() &&
            bounds.size() == 4 && bounds.at(0).toDouble() == 105.78 &&
            bounds.at(1).toDouble() == 20.97 && bounds.at(2).toDouble() == 105.90 &&
            bounds.at(3).toDouble() == 21.10;
        if (!contractMatches || !QFileInfo::exists(databasePath) ||
            QFileInfo(databasePath).size() > 20 * 1024 * 1024 ||
            QFileInfo(databasePath).size() !=
                places.value(QStringLiteral("size_bytes")).toVariant().toLongLong() ||
            fileDigest(databasePath) != places.value(QStringLiteral("sha256")).toString()) {
            error = QStringLiteral("Gói tìm kiếm ngoại tuyến không hợp lệ");
        }
    }
    if (errorMessage)
        *errorMessage = error;
    return error.isEmpty();
}

class SearchPackageWorker final : public QObject
{
public:
    SearchPackageWorker(QString packagePath, PlaceSearchController *controller) :
        packagePath_(std::move(packagePath)), controller_(controller)
    {}

    ~SearchPackageWorker() override
    {
        if (!database_.isValid())
            return;
        database_.close();
        database_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName_);
    }

    void initialize()
    {
        QString error;
        const QString databasePath = QDir(QFileInfo(packagePath_).canonicalFilePath())
                                         .filePath(QStringLiteral("places.sqlite"));
        validateSearchPackage(packagePath_, &error);
        if (error.isEmpty()) {
            connectionName_ = QStringLiteral("qtmaps-search-%1")
                                  .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
            database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
            database_.setDatabaseName(databasePath);
            if (!database_.open()) {
                error = database_.lastError().text();
            } else {
                QSqlQuery schema(database_);
                if (!schema.exec(QStringLiteral("SELECT 1 FROM places_fts LIMIT 1")))
                    error = schema.lastError().text();
            }
        }
        const bool available = error.isEmpty();
        if (!available)
            qWarning().noquote() << "Offline search package unavailable:" << error;
        notifyStatus(available, available
                                    ? QStringLiteral("Tìm trong gói địa điểm Hà Nội ngoại tuyến")
                                    : error);
    }

    void query(const QString &input, quint64 requestId)
    {
        QVariantList results;
        QString error;
        if (!database_.isOpen()) {
            error = QStringLiteral("Gói tìm kiếm ngoại tuyến không sẵn sàng");
        } else {
            QSqlQuery query(database_);
            query.prepare(
                QStringLiteral("SELECT p.name, p.category, p.latitude, p.longitude "
                               "FROM places_fts f JOIN places p ON p.id = f.rowid "
                               "WHERE f.places_fts MATCH ? "
                               "ORDER BY bm25(places_fts), lower(p.name), p.source_id LIMIT 8"));
            query.addBindValue(ftsPrefixQuery(input));
            if (!query.exec()) {
                error = query.lastError().text();
            } else {
                while (query.next()) {
                    const QString name = query.value(0).toString();
                    const double latitude = query.value(2).toDouble();
                    const double longitude = query.value(3).toDouble();
                    results.append(QVariantMap{
                        {QStringLiteral("name"), name},
                        {QStringLiteral("detail"), QStringLiteral("%1 · OpenStreetMap (ODbL)")
                                                       .arg(query.value(1).toString())},
                        {QStringLiteral("coordinate"), QStringLiteral("%1, %2")
                                                           .arg(latitude, 0, 'f', 6)
                                                           .arg(longitude, 0, 'f', 6)}});
                }
            }
        }
        if (controller_)
            QMetaObject::invokeMethod(
                controller_,
                [controller = controller_, requestId, results, error] {
                    controller->handlePackageResults(requestId, results, error);
                },
                Qt::QueuedConnection);
    }

private:
    void notifyStatus(bool available, const QString &message)
    {
        if (controller_)
            QMetaObject::invokeMethod(
                controller_,
                [controller = controller_, available, message] {
                    controller->handlePackageStatus(available, message);
                },
                Qt::QueuedConnection);
    }

    QString packagePath_;
    QPointer<PlaceSearchController> controller_;
    QString connectionName_;
    QSqlDatabase database_;
};

} // namespace

bool PlaceSearchController::validatePackage(const QString &packagePath, QString *errorMessage)
{
    return validateSearchPackage(packagePath, errorMessage);
}

PlaceSearchController::PlaceSearchController(const QString &packagePath, QObject *parent) :
    QObject(parent)
{
    debounce_.setSingleShot(true);
    debounce_.setInterval(120);
    connect(&debounce_, &QTimer::timeout, this, &PlaceSearchController::applyQuery);
    worker_ = new SearchPackageWorker(packagePath, this);
    worker_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    workerThread_.start();
    QMetaObject::invokeMethod(
        worker_, [worker = worker_] { static_cast<SearchPackageWorker *>(worker)->initialize(); },
        Qt::QueuedConnection);
}

PlaceSearchController::~PlaceSearchController()
{
    workerThread_.quit();
    workerThread_.wait();
}

QString PlaceSearchController::query() const { return query_; }
QVariantList PlaceSearchController::results() const { return results_; }
QString PlaceSearchController::state() const { return state_; }
QString PlaceSearchController::message() const { return message_; }
QString PlaceSearchController::sourceLabel() const { return sourceLabel_; }

void PlaceSearchController::setQuery(const QString &query)
{
    const QString normalized = query.simplified();
    if (query_ == normalized)
        return;
    query_ = normalized;
    results_.clear();
    if (query_.isEmpty()) {
        debounce_.stop();
        state_ = QStringLiteral("idle");
        message_ = QStringLiteral("Tìm trong dữ liệu demo cục bộ");
    } else {
        state_ = QStringLiteral("loading");
        message_ = QStringLiteral("Đang tìm trong dữ liệu demo cục bộ…");
        debounce_.start();
    }
    emit stateChanged();
}

void PlaceSearchController::applyQuery()
{
    if (packageAvailable_) {
        const quint64 requestId = ++requestId_;
        QMetaObject::invokeMethod(
            worker_,
            [worker = worker_, query = query_, requestId] {
                static_cast<SearchPackageWorker *>(worker)->query(query, requestId);
            },
            Qt::QueuedConnection);
        return;
    }
    applyFixtureQuery();
}

void PlaceSearchController::applyFixtureQuery()
{
    for (const Place &place : kPlaces) {
        const QString name = QString::fromUtf8(place.name);
        if (!name.contains(query_, Qt::CaseInsensitive))
            continue;
        results_.append(QVariantMap{{QStringLiteral("name"), name},
                                    {QStringLiteral("detail"), QString::fromUtf8(place.detail)}});
    }
    state_ = results_.isEmpty() ? QStringLiteral("empty") : QStringLiteral("ready");
    message_ = results_.isEmpty() ? QStringLiteral("Không có địa điểm demo phù hợp")
                                  : QStringLiteral("Kết quả từ dữ liệu demo cục bộ");
    emit stateChanged();
}

void PlaceSearchController::handlePackageStatus(bool available, const QString &message)
{
    packageAvailable_ = available;
    if (available) {
        sourceLabel_ = QStringLiteral("Gói địa điểm Hà Nội · OpenStreetMap (ODbL)");
    } else {
        sourceLabel_ = QStringLiteral("Dữ liệu demo cục bộ · gói ngoại tuyến chưa sẵn sàng");
    }
    if (query_.isEmpty()) {
        message_ =
            available ? message : QStringLiteral("Tìm trong dữ liệu demo cục bộ; %1").arg(message);
        emit stateChanged();
    } else {
        applyQuery();
    }
}

void PlaceSearchController::handlePackageResults(quint64 requestId, const QVariantList &results,
                                                 const QString &error)
{
    if (requestId != requestId_)
        return;
    if (!error.isEmpty()) {
        packageAvailable_ = false;
        sourceLabel_ = QStringLiteral("Dữ liệu demo cục bộ · gói ngoại tuyến lỗi");
        applyFixtureQuery();
        return;
    }
    results_ = results;
    state_ = results_.isEmpty() ? QStringLiteral("empty") : QStringLiteral("ready");
    message_ = results_.isEmpty() ? QStringLiteral("Không có địa điểm ngoại tuyến phù hợp")
                                  : QStringLiteral("Kết quả từ gói địa điểm Hà Nội ngoại tuyến");
    emit stateChanged();
}

} // namespace search
} // namespace qtmaps
