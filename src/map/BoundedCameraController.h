#pragma once

#include <QGeoCoordinate>
#include <QObject>
#include <QVariantMap>

namespace qtmaps {
namespace map {

struct CameraState
{
    QGeoCoordinate center;
    qreal zoom = 14.0;
    qreal bearing = 0.0;
    qreal tilt = 0.0;
};

class BoundedCameraController final : public QObject
{
    Q_OBJECT
public:
    static constexpr qreal kMinimumZoom = 12.0;
    static constexpr qreal kMaximumZoom = 18.0;
    static constexpr qreal kNavigationZoom = 16.0;

    static CameraState overview();
    static CameraState normalize(const CameraState &requested);
    static CameraState navigationFollow(const QGeoCoordinate &coordinate,
                                        qreal bearing,
                                        const CameraState &current);
    static bool isWithinBounds(const QGeoCoordinate &coordinate);
    explicit BoundedCameraController(QObject *parent = nullptr);
    Q_INVOKABLE QVariantMap normalizedCamera(qreal latitude, qreal longitude,
                                             qreal zoom, qreal bearing, qreal tilt) const;
    Q_INVOKABLE QVariantMap navigationCamera(qreal latitude, qreal longitude,
                                             qreal bearing, qreal currentZoom) const;
};

} // namespace map
} // namespace qtmaps
