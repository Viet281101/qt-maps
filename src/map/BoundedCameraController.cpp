#include "map/BoundedCameraController.h"

#include "map/MapPackageContract.h"

#include <algorithm>
#include <cmath>

namespace qtmaps {
namespace map {

constexpr qreal BoundedCameraController::kMinimumZoom;
constexpr qreal BoundedCameraController::kMaximumZoom;
constexpr qreal BoundedCameraController::kNavigationZoom;

namespace {

qreal bounded(qreal value, qreal minimum, qreal maximum)
{
    return std::max(minimum, std::min(maximum, value));
}

qreal normalizedBearing(qreal bearing)
{
    qreal result = std::fmod(bearing, 360.0);
    return result < 0.0 ? result + 360.0 : result;
}

} // namespace

BoundedCameraController::BoundedCameraController(QObject *parent) : QObject(parent) {}

QVariantMap BoundedCameraController::normalizedCamera(qreal latitude, qreal longitude,
                                                       qreal zoom, qreal bearing, qreal tilt) const
{
    const CameraState state = normalize({QGeoCoordinate(latitude, longitude), zoom, bearing, tilt});
    return {{QStringLiteral("latitude"), state.center.latitude()},
            {QStringLiteral("longitude"), state.center.longitude()},
            {QStringLiteral("zoom"), state.zoom},
            {QStringLiteral("bearing"), state.bearing},
            {QStringLiteral("tilt"), state.tilt}};
}

QVariantMap BoundedCameraController::navigationCamera(qreal latitude, qreal longitude,
                                                       qreal bearing, qreal currentZoom) const
{
    CameraState current = overview();
    current.zoom = currentZoom;
    const CameraState state = navigationFollow(QGeoCoordinate(latitude, longitude), bearing, current);
    return {{QStringLiteral("latitude"), state.center.latitude()},
            {QStringLiteral("longitude"), state.center.longitude()},
            {QStringLiteral("zoom"), state.zoom},
            {QStringLiteral("bearing"), state.bearing},
            {QStringLiteral("tilt"), state.tilt}};
}

CameraState BoundedCameraController::overview()
{
    CameraState state;
    state.center = QGeoCoordinate((contract::kSouth + contract::kNorth) / 2.0,
                                  (contract::kWest + contract::kEast) / 2.0);
    return state;
}

bool BoundedCameraController::isWithinBounds(const QGeoCoordinate &coordinate)
{
    return coordinate.isValid()
        && coordinate.latitude() >= contract::kSouth
        && coordinate.latitude() <= contract::kNorth
        && coordinate.longitude() >= contract::kWest
        && coordinate.longitude() <= contract::kEast;
}

CameraState BoundedCameraController::normalize(const CameraState &requested)
{
    CameraState result = overview();
    if (requested.center.isValid()) {
        result.center = QGeoCoordinate(bounded(requested.center.latitude(), contract::kSouth, contract::kNorth),
                                       bounded(requested.center.longitude(), contract::kWest, contract::kEast));
    }
    result.zoom = bounded(requested.zoom, kMinimumZoom, kMaximumZoom);
    result.bearing = normalizedBearing(requested.bearing);
    result.tilt = bounded(requested.tilt, 0.0, 60.0);
    return result;
}

CameraState BoundedCameraController::navigationFollow(const QGeoCoordinate &coordinate,
                                                       qreal bearing,
                                                       const CameraState &current)
{
    CameraState requested = current;
    requested.center = coordinate;
    requested.bearing = bearing;
    requested.tilt = 45.0;
    requested.zoom = std::max(current.zoom, kNavigationZoom);
    return normalize(requested);
}

} // namespace map
} // namespace qtmaps
