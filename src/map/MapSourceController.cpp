#include "MapSourceController.h"

#include "TileSource.h"

namespace qtmaps {
namespace map {

MapSourceController::MapSourceController(TileSource *vectorSource, TileSource *rasterSource,
                                         QObject *parent) :
    QObject(parent), vectorSource_(vectorSource), rasterSource_(rasterSource)
{}

bool MapSourceController::start(const QString &packagePath, const QString &diagnosticRasterPath,
                                bool forceVectorFailure)
{
    stop();
    bool vectorStarted = false;
    if (!forceVectorFailure && vectorSource_) {
        vectorStarted = vectorSource_->start(packagePath);
    }
    if (vectorStarted) {
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
    if (!rasterSource_ || !rasterSource_->start(diagnosticRasterPath)) {
        const QString rasterError = rasterSource_ ? rasterSource_->errorMessage()
                                                  : QStringLiteral("raster source unavailable");
        fallbackReason_ +=
            QStringLiteral("; diagnostic raster fallback failed: %1").arg(rasterError);
        emit stateChanged();
        return false;
    }
    usingRasterFallback_ = true;
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
    usingRasterFallback_ = false;
    fallbackReason_.clear();
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
    return rasterSource_ && rasterSource_->isRunning() ? rasterSource_->baseUrl() + QLatin1Char('/')
                                                       : QString();
}

} // namespace map
} // namespace qtmaps
