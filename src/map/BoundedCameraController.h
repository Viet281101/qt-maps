#pragma once

#include "map/MapPackageContract.h"

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

struct CameraViewport
{
    qreal width = 0.0;
    qreal height = 0.0;
    qreal leftPadding = 0.0;
    qreal rightPadding = 0.0;
    qreal topPadding = 0.0;
    qreal bottomPadding = 0.0;
};

class BoundedCameraController final : public QObject
{
    Q_OBJECT
public:
    static constexpr qreal kMinimumZoom = contract::kDisplayMinimumZoom;
    static constexpr qreal kMaximumZoom = contract::kDisplayMaximumZoom;
    static constexpr qreal kNavigationZoom = 16.0;

    static CameraState overview();
    static CameraState normalize(const CameraState &requested);
    static CameraState normalizeForViewport(const CameraState &requested,
                                            const CameraViewport &viewport);
    static bool routeFit(const QGeoCoordinate &southwest, const QGeoCoordinate &northeast,
                         const CameraViewport &viewport, const CameraState &current,
                         CameraState *result);
    static CameraState navigationFollow(const QGeoCoordinate &coordinate, qreal bearing,
                                        const CameraState &current);
    static bool isWithinBounds(const QGeoCoordinate &coordinate);
    explicit BoundedCameraController(QObject *parent = nullptr);
    Q_INVOKABLE QVariantMap overviewCamera(qreal width, qreal height) const;
    Q_INVOKABLE QVariantMap normalizedCamera(qreal latitude, qreal longitude, qreal zoom,
                                             qreal bearing, qreal tilt, qreal width,
                                             qreal height) const;
    Q_INVOKABLE QVariantMap routeFitCamera(qreal south, qreal north, qreal west, qreal east,
                                           qreal width, qreal height, qreal leftPadding,
                                           qreal rightPadding, qreal topPadding,
                                           qreal bottomPadding, qreal bearing, qreal tilt) const;
    Q_INVOKABLE QVariantMap navigationCamera(qreal latitude, qreal longitude, qreal bearing,
                                             qreal currentZoom, qreal width, qreal height) const;
};

} // namespace map
} // namespace qtmaps
