#include "PackageReadinessController.h"

#include "map/MapPackageManifest.h"
#include "map/RasterPackageManifest.h"
#include "search/PlaceSearchController.h"

#include <QFileInfo>
#include <QThread>
#include <QTimer>
#include <QVariantMap>

namespace qtmaps {
namespace core {
namespace {

QString normalizedReason(const QString &path)
{
    return path.isEmpty() || !QFileInfo::exists(path) ? QStringLiteral("missing")
                                                      : QStringLiteral("invalid");
}

QVariantMap readinessItem(const QString &role, const QString &name, bool required, bool available,
                          const QString &reason)
{
    return {{QStringLiteral("role"), role},
            {QStringLiteral("name"), name},
            {QStringLiteral("required"), required},
            {QStringLiteral("available"), available},
            {QStringLiteral("reason"), available ? QStringLiteral("ready") : reason}};
}

class ReadinessWorker final : public QObject
{
    Q_OBJECT
public:
    ReadinessWorker(QString vectorPath, QString rasterPath, QString searchPath,
                    QString orthographyPath) :
        vectorPath_(std::move(vectorPath)), rasterPath_(std::move(rasterPath)),
        searchPath_(std::move(searchPath)), orthographyPath_(std::move(orthographyPath))
    {}

signals:
    void completed(const QVariantList &items);
    void finished();

public slots:
    void run()
    {
        QVariantList items;
        QString error;
        qtmaps::map::MapPackageManifest vectorManifest;
        const bool vectorOk =
            qtmaps::map::MapPackageManifest::loadAndValidate(vectorPath_, &vectorManifest, &error);
        items.append(readinessItem(QStringLiteral("vector"), QStringLiteral("Bản đồ vector"), true,
                                   vectorOk, normalizedReason(vectorPath_)));
        if (QThread::currentThread()->isInterruptionRequested()) {
            finish(items);
            return;
        }

        qtmaps::map::RasterPackageManifest rasterManifest;
        error.clear();
        const bool rasterOk =
            !rasterPath_.isEmpty() && qtmaps::map::RasterPackageManifest::loadAndValidate(
                                          rasterPath_, &rasterManifest, &error, vectorPath_);
        items.append(readinessItem(QStringLiteral("raster"), QStringLiteral("Raster ngoại tuyến"),
                                   false, rasterOk, normalizedReason(rasterPath_)));
        if (QThread::currentThread()->isInterruptionRequested()) {
            finish(items);
            return;
        }

        error.clear();
        const bool searchOk =
            !searchPath_.isEmpty() &&
            qtmaps::search::PlaceSearchController::validatePackage(searchPath_, &error);
        items.append(readinessItem(QStringLiteral("search"), QStringLiteral("Tìm kiếm ngoại tuyến"),
                                   false, searchOk, normalizedReason(searchPath_)));
        if (QThread::currentThread()->isInterruptionRequested()) {
            finish(items);
            return;
        }

        qtmaps::map::RasterPackageManifest orthographyManifest;
        error.clear();
        const bool orthographyOk = !orthographyPath_.isEmpty() &&
                                   qtmaps::map::RasterPackageManifest::loadAndValidateOrthography(
                                       orthographyPath_, &orthographyManifest, &error);
        items.append(readinessItem(QStringLiteral("orthography"), QStringLiteral("Ảnh trực giao"),
                                   false, orthographyOk, normalizedReason(orthographyPath_)));
        finish(items);
    }

private:
    void finish(const QVariantList &items)
    {
        emit completed(items);
        emit finished();
    }

    QString vectorPath_;
    QString rasterPath_;
    QString searchPath_;
    QString orthographyPath_;
};

} // namespace

PackageReadinessController::PackageReadinessController(const QString &vectorPath,
                                                       const QString &rasterPath,
                                                       const QString &searchPath,
                                                       const QString &orthographyPath,
                                                       QObject *parent) :
    QObject(parent), thread_(new QThread), timeout_(new QTimer(this))
{
    timeout_->setSingleShot(true);
    connect(timeout_, &QTimer::timeout, this, &PackageReadinessController::markTimedOut);
    auto *worker = new ReadinessWorker(vectorPath, rasterPath, searchPath, orthographyPath);
    worker->moveToThread(thread_);
    connect(thread_, &QThread::started, worker, &ReadinessWorker::run);
    connect(worker, &ReadinessWorker::completed, this, &PackageReadinessController::applyResult);
    connect(worker, &ReadinessWorker::finished, thread_, &QThread::quit, Qt::DirectConnection);
    connect(thread_, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread_, &QThread::finished, thread_, &QObject::deleteLater);
    thread_->start();
    timeout_->start(5000);
}

PackageReadinessController::~PackageReadinessController()
{
    if (thread_) {
        thread_->requestInterruption();
        thread_->quit();
        thread_->wait(500);
    }
}

QVariantList PackageReadinessController::items() const { return items_; }
QString PackageReadinessController::state() const { return state_; }

void PackageReadinessController::applyResult(const QVariantList &items)
{
    if (timedOut_)
        return;
    timeout_->stop();
    items_ = items;
    bool vectorAvailable = false;
    bool optionalUnavailable = false;
    for (const QVariant &item : items_) {
        const QVariantMap map = item.toMap();
        if (map.value(QStringLiteral("role")) == QStringLiteral("vector"))
            vectorAvailable = map.value(QStringLiteral("available")).toBool();
        else if (!map.value(QStringLiteral("available")).toBool())
            optionalUnavailable = true;
    }
    state_ = !vectorAvailable      ? QStringLiteral("error")
             : optionalUnavailable ? QStringLiteral("degraded")
                                   : QStringLiteral("ready");
    emit changed();
}

void PackageReadinessController::markTimedOut()
{
    timedOut_ = true;
    if (thread_)
        thread_->requestInterruption();
    state_ = QStringLiteral("timed-out");
    emit changed();
}

} // namespace core
} // namespace qtmaps

#include "PackageReadinessController.moc"
