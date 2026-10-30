#pragma once

#include <QObject>
#include <QString>

#include <memory>

namespace qtmaps {
namespace map {

class TileSource;
class RasterTileSource;

class MapSourceController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString fallbackReason READ fallbackReason NOTIFY stateChanged)
    Q_PROPERTY(bool usingRasterFallback READ usingRasterFallback NOTIFY stateChanged)
    Q_PROPERTY(QString activeStyleUrl READ activeStyleUrl NOTIFY stateChanged)
    Q_PROPERTY(QString layerMode READ layerMode NOTIFY stateChanged)
    Q_PROPERTY(QString mapStatusText READ mapStatusText NOTIFY stateChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool rasterConfigured READ rasterConfigured NOTIFY stateChanged)
    Q_PROPERTY(bool rasterResourcesAllocated READ rasterResourcesAllocated NOTIFY stateChanged)
    Q_PROPERTY(bool automaticFallback READ automaticFallback NOTIFY stateChanged)

public:
    explicit MapSourceController(TileSource *vectorSource, TileSource *rasterSource,
                                 QObject *parent = nullptr);
    ~MapSourceController() override;

    void setRasterPackagePath(const QString &packagePath);
    bool start(const QString &packagePath, const QString &diagnosticRasterPath,
               bool forceVectorFailure = false);
    void stop();
    bool usingRasterFallback() const;
    QString fallbackReason() const;
    QString vectorStyleUrl() const;
    QString rasterBaseUrl() const;
    QString activeStyleUrl() const;
    QString layerMode() const;
    QString mapStatusText() const;
    bool loading() const;
    bool rasterConfigured() const;
    bool rasterResourcesAllocated() const;
    bool automaticFallback() const;

    Q_INVOKABLE void activateRaster();
    Q_INVOKABLE void activateVector();

signals:
    void stateChanged();

private:
    void activateRaster(bool automatic);

    TileSource *vectorSource_ = nullptr;
    TileSource *rasterSource_ = nullptr;
    std::unique_ptr<RasterTileSource> productionRasterSource_;
    bool usingRasterFallback_ = false;
    bool loading_ = false;
    bool automaticFallback_ = false;
    QString fallbackReason_;
    QString rasterPackagePath_;
    QString activeStyleUrl_;
    QString layerMode_ = QStringLiteral("unavailable");
    QString mapStatusText_ = QStringLiteral("Bản đồ chưa sẵn sàng");
};

} // namespace map
} // namespace qtmaps
