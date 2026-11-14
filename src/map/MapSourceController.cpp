#include "MapSourceController.h"

#include "RasterTileSource.h"
#include "TileSource.h"

namespace qtmaps {
namespace map {

MapSourceController::MapSourceController(TileSource *vectorSource, TileSource *rasterSource,
                                         QObject *parent) :
    QObject(parent), vectorSource_(vectorSource), rasterSource_(rasterSource)
{}

MapSourceController::~MapSourceController() { stop(); }

void MapSourceController::setRasterPackagePath(const QString &packagePath)
{
    rasterPackagePath_ = packagePath;
    emit stateChanged();
}

bool MapSourceController::start(const QString &packagePath, const QString &diagnosticRasterPath,
                                bool forceVectorFailure)
{
    stop();
    vectorPackagePath_ = packagePath;
    bool vectorStarted = false;
    if (!forceVectorFailure && vectorSource_) {
        vectorStarted = vectorSource_->start(packagePath);
    }
    if (vectorStarted) {
        activeStyleUrl_ = vectorStyleUrl();
        layerMode_ = QStringLiteral("vector");
        mapStatusText_ = QStringLiteral("Bản đồ vector · dữ liệu cục bộ");
        emit stateChanged();
        return true;
    }
    fallbackReason_ = forceVectorFailure
                          ? QStringLiteral("Vector failure was forced for fallback verification")
                      : vectorSource_ ? vectorSource_->errorMessage()
                                      : QStringLiteral("Vector tile source is unavailable");
    if (fallbackReason_.isEmpty()) {
        fallbackReason_ = QStringLiteral("Vector package validation or startup failed");
    }
    if (!rasterPackagePath_.isEmpty()) {
        activateRaster(true);
        return true;
    }
    if (!rasterSource_ || !rasterSource_->start(diagnosticRasterPath)) {
        const QString rasterError = rasterSource_ ? rasterSource_->errorMessage()
                                                  : QStringLiteral("raster source unavailable");
        fallbackReason_ +=
            QStringLiteral("; diagnostic raster fallback failed: %1").arg(rasterError);
        emit stateChanged();
        return false;
    }
    usingRasterFallback_ = true;
    layerMode_ = QStringLiteral("diagnostic-raster");
    mapStatusText_ = QStringLiteral("Raster chẩn đoán · không phải dữ liệu sản phẩm");
    emit stateChanged();
    return true;
}

void MapSourceController::stop()
{
    if (vectorSource_) {
        vectorSource_->stop();
    }
    if (rasterSource_) {
        rasterSource_->stop();
    }
    productionRasterSource_.reset();
    usingRasterFallback_ = false;
    loading_ = false;
    automaticFallback_ = false;
    fallbackReason_.clear();
    vectorPackagePath_.clear();
    activeStyleUrl_.clear();
    layerMode_ = QStringLiteral("unavailable");
    mapStatusText_ = QStringLiteral("Bản đồ chưa sẵn sàng");
}

bool MapSourceController::usingRasterFallback() const { return usingRasterFallback_; }
QString MapSourceController::fallbackReason() const { return fallbackReason_; }
QString MapSourceController::vectorStyleUrl() const
{
    return vectorSource_ && vectorSource_->isRunning()
               ? vectorSource_->baseUrl() + QStringLiteral("/style/style.json")
               : QString();
}
QString MapSourceController::rasterBaseUrl() const
{
    if (productionRasterSource_ && productionRasterSource_->running()) {
        return productionRasterSource_->baseUrl() + QLatin1Char('/');
    }
    return rasterSource_ && rasterSource_->isRunning() ? rasterSource_->baseUrl() + QLatin1Char('/')
                                                       : QString();
}

QString MapSourceController::activeStyleUrl() const { return activeStyleUrl_; }
QString MapSourceController::layerMode() const { return layerMode_; }
QString MapSourceController::mapStatusText() const { return mapStatusText_; }
bool MapSourceController::loading() const { return loading_; }
bool MapSourceController::rasterConfigured() const { return !rasterPackagePath_.isEmpty(); }
bool MapSourceController::rasterResourcesAllocated() const
{
    return productionRasterSource_ &&
           (productionRasterSource_->loading() || productionRasterSource_->running());
}
bool MapSourceController::automaticFallback() const { return automaticFallback_; }

void MapSourceController::activateRaster() { activateRaster(false); }

void MapSourceController::activateRaster(bool automatic)
{
    if (loading_ || layerMode_ == QStringLiteral("raster")) {
        return;
    }
    automaticFallback_ = automatic;
    loading_ = true;
    layerMode_ = QStringLiteral("raster-loading");
    mapStatusText_ = automatic ? QStringLiteral("Đang mở raster dự phòng…")
                               : QStringLiteral("Đang mở raster ngoại tuyến…");
    emit stateChanged();
    if (rasterPackagePath_.isEmpty()) {
        loading_ = false;
        layerMode_ = QStringLiteral("error");
        mapStatusText_ = QStringLiteral("Chưa cấu hình gói raster ngoại tuyến");
        emit stateChanged();
        return;
    }
    if (!productionRasterSource_) {
        productionRasterSource_.reset(new RasterTileSource);
        connect(productionRasterSource_.get(), &RasterTileSource::started, this, [this]() {
            loading_ = false;
            usingRasterFallback_ = true;
            activeStyleUrl_ = productionRasterSource_->styleUrl();
            layerMode_ = QStringLiteral("raster");
            mapStatusText_ = automaticFallback_
                                 ? QStringLiteral("Raster ngoại tuyến · tự động dự phòng")
                                 : QStringLiteral("Raster ngoại tuyến · chọn thủ công");
            emit stateChanged();
        });
        connect(productionRasterSource_.get(), &RasterTileSource::failed, this,
                [this](const QString &message) {
                    loading_ = false;
                    usingRasterFallback_ = false;
                    layerMode_ = QStringLiteral("error");
                    fallbackReason_ = fallbackReason_.isEmpty()
                                          ? message
                                          : fallbackReason_ + QStringLiteral("; ") + message;
                    mapStatusText_ = QStringLiteral("Không mở được raster ngoại tuyến");
                    if (vectorSource_ && vectorSource_->isRunning()) {
                        activeStyleUrl_ = vectorStyleUrl();
                    }
                    emit stateChanged();
                });
        connect(productionRasterSource_.get(), &RasterTileSource::stopped, this,
                [this]() { emit stateChanged(); });
    }
    productionRasterSource_->startAsync(rasterPackagePath_, vectorPackagePath_);
}

void MapSourceController::activateVector()
{
    if (!vectorSource_ || !vectorSource_->isRunning()) {
        layerMode_ = QStringLiteral("error");
        mapStatusText_ = QStringLiteral("Bản đồ vector không sẵn sàng");
        emit stateChanged();
        return;
    }
    loading_ = false;
    usingRasterFallback_ = false;
    automaticFallback_ = false;
    activeStyleUrl_ = vectorStyleUrl();
    layerMode_ = QStringLiteral("vector");
    mapStatusText_ = QStringLiteral("Bản đồ vector · dữ liệu cục bộ");
    if (productionRasterSource_) {
        productionRasterSource_->stopAsync();
    }
    emit stateChanged();
}

} // namespace map
} // namespace qtmaps
