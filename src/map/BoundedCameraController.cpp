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
