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

QVariantMap cameraMap(const CameraState &state, bool valid = true)
{
    return {{QStringLiteral("valid"), valid},
            {QStringLiteral("latitude"), state.center.latitude()},
            {QStringLiteral("longitude"), state.center.longitude()},
            {QStringLiteral("zoom"), state.zoom},
            {QStringLiteral("bearing"), state.bearing},
            {QStringLiteral("tilt"), state.tilt}};
}

qreal mercatorY(qreal latitude)
{
    const qreal radians = latitude * M_PI / 180.0;
    return (1.0 - std::log(std::tan(radians) + 1.0 / std::cos(radians)) / M_PI) / 2.0;
}

qreal latitudeFromMercator(qreal y)
{
    return std::atan(std::sinh(M_PI * (1.0 - 2.0 * y))) * 180.0 / M_PI;
}

} // namespace

BoundedCameraController::BoundedCameraController(QObject *parent) : QObject(parent) {}

QVariantMap BoundedCameraController::overviewCamera() const { return cameraMap(overview()); }

QVariantMap BoundedCameraController::normalizedCamera(qreal latitude, qreal longitude, qreal zoom,
                                                      qreal bearing, qreal tilt) const
{
    const CameraState state = normalize({QGeoCoordinate(latitude, longitude), zoom, bearing, tilt});
    return cameraMap(state);
}

QVariantMap BoundedCameraController::routeFitCamera(qreal south, qreal north, qreal west,
                                                    qreal east, qreal width, qreal height,
                                                    qreal leftPadding, qreal rightPadding,
                                                    qreal topPadding, qreal bottomPadding,
                                                    qreal bearing, qreal tilt) const
{
    CameraState result = overview();
    const CameraState current = {overview().center, overview().zoom, bearing, tilt};
    const CameraViewport viewport = {width,        height,     leftPadding,
                                     rightPadding, topPadding, bottomPadding};
    if (!routeFit(QGeoCoordinate(south, west), QGeoCoordinate(north, east), viewport, current,
                  &result)) {
        return cameraMap(current, false);
    }
    return cameraMap(result);
}

QVariantMap BoundedCameraController::navigationCamera(qreal latitude, qreal longitude,
                                                      qreal bearing, qreal currentZoom) const
{
    CameraState current = overview();
    current.zoom = currentZoom;
    const CameraState state =
        navigationFollow(QGeoCoordinate(latitude, longitude), bearing, current);
    return cameraMap(state);
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
    return coordinate.isValid() && coordinate.latitude() >= contract::kSouth &&
           coordinate.latitude() <= contract::kNorth && coordinate.longitude() >= contract::kWest &&
           coordinate.longitude() <= contract::kEast;
}

CameraState BoundedCameraController::normalize(const CameraState &requested)
{
    CameraState result = overview();
    if (requested.center.isValid()) {
        result.center =
            QGeoCoordinate(bounded(requested.center.latitude(), contract::kSouth, contract::kNorth),
                           bounded(requested.center.longitude(), contract::kWest, contract::kEast));
    }
    result.zoom = bounded(requested.zoom, kMinimumZoom, kMaximumZoom);
    result.bearing = normalizedBearing(requested.bearing);
    result.tilt = bounded(requested.tilt, 0.0, 60.0);
    return result;
}

bool BoundedCameraController::routeFit(const QGeoCoordinate &southwest,
                                       const QGeoCoordinate &northeast,
                                       const CameraViewport &viewport, const CameraState &current,
                                       CameraState *result)
{
    if (result == nullptr || !southwest.isValid() || !northeast.isValid() ||
        southwest.latitude() >= northeast.latitude() ||
        southwest.longitude() >= northeast.longitude() || !std::isfinite(viewport.width) ||
        !std::isfinite(viewport.height) || !std::isfinite(viewport.leftPadding) ||
        !std::isfinite(viewport.rightPadding) || !std::isfinite(viewport.topPadding) ||
        !std::isfinite(viewport.bottomPadding)) {
        return false;
    }

    const qreal usableWidth = viewport.width - viewport.leftPadding - viewport.rightPadding;
    const qreal usableHeight = viewport.height - viewport.topPadding - viewport.bottomPadding;
    if (usableWidth <= 0.0 || usableHeight <= 0.0) {
        return false;
    }

    const qreal northY = mercatorY(northeast.latitude());
    const qreal southY = mercatorY(southwest.latitude());
    const qreal longitudeSpan = northeast.longitude() - southwest.longitude();
    const qreal latitudeSpan = southY - northY;
    if (!std::isfinite(northY) || !std::isfinite(southY) || longitudeSpan <= 0.0 ||
        latitudeSpan <= 0.0) {
        return false;
    }

    const qreal zoomX = std::log2(usableWidth * 360.0 / (256.0 * longitudeSpan));
    const qreal zoomY = std::log2(usableHeight / (256.0 * latitudeSpan));
    if (!std::isfinite(zoomX) || !std::isfinite(zoomY)) {
        return false;
    }

    CameraState requested = current;
    requested.zoom = std::min(zoomX, zoomY);
    const qreal worldPixels = 256.0 * std::pow(2.0, requested.zoom);
    const qreal routeCenterLongitude = (southwest.longitude() + northeast.longitude()) / 2.0;
    const qreal routeCenterY = (northY + southY) / 2.0;
    const qreal targetX = viewport.leftPadding + usableWidth / 2.0;
    const qreal targetY = viewport.topPadding + usableHeight / 2.0;
    requested.center = QGeoCoordinate(
        latitudeFromMercator(routeCenterY - (targetY - viewport.height / 2.0) / worldPixels),
        routeCenterLongitude - (targetX - viewport.width / 2.0) * 360.0 / worldPixels);
    *result = normalize(requested);
    return true;
}

CameraState BoundedCameraController::navigationFollow(const QGeoCoordinate &coordinate,
                                                      qreal bearing, const CameraState &current)
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
