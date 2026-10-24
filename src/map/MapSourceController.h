#pragma once

#include <QObject>
#include <QString>

namespace qtmaps {
namespace map {

class TileSource;

class MapSourceController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString fallbackReason READ fallbackReason NOTIFY stateChanged)
    Q_PROPERTY(bool usingRasterFallback READ usingRasterFallback NOTIFY stateChanged)

public:
    explicit MapSourceController(TileSource *vectorSource, TileSource *rasterSource,
                                 QObject *parent = nullptr);

    bool start(const QString &packagePath, const QString &diagnosticRasterPath,
               bool forceVectorFailure = false);
    void stop();
    bool usingRasterFallback() const;
    QString fallbackReason() const;
    QString vectorStyleUrl() const;
    QString rasterBaseUrl() const;

signals:
    void stateChanged();

private:
    TileSource *vectorSource_ = nullptr;
    TileSource *rasterSource_ = nullptr;
    bool usingRasterFallback_ = false;
    QString fallbackReason_;
};

} // namespace map
} // namespace qtmaps
