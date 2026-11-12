#pragma once

#include <QGeoCoordinate>

namespace qtmaps {
namespace map {

struct CameraState
{
    QGeoCoordinate center;
    qreal zoom = 14.0;
    qreal bearing = 0.0;
    qreal tilt = 0.0;
};

class BoundedCameraController final
{
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
};

} // namespace map
} // namespace qtmaps
