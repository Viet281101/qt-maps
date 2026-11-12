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

constexpr qreal kPi = 3.14159265358979323846;

qreal bounded(qreal value, qreal minimum, qreal maximum)
{
    return std::max(minimum, std::min(maximum, value));
}

qreal normalizedBearing(qreal bearing)
{
    if (!std::isfinite(bearing))
        return 0.0;
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
    const qreal radians = latitude * kPi / 180.0;
    return (1.0 - std::log(std::tan(radians) + 1.0 / std::cos(radians)) / kPi) / 2.0;
}

qreal latitudeFromMercator(qreal y)
{
    return std::atan(std::sinh(kPi * (1.0 - 2.0 * y))) * 180.0 / kPi;
}

bool hasValidViewport(const CameraViewport &viewport)
{
    return std::isfinite(viewport.width) && std::isfinite(viewport.height) &&
           viewport.width > 0.0 && viewport.height > 0.0;
}

struct ViewportOffsets
{
    qreal minimumX = 0.0;
    qreal maximumX = 0.0;
    qreal minimumY = 0.0;
    qreal maximumY = 0.0;
};

ViewportOffsets viewportOffsets(const CameraViewport &viewport, qreal bearing)
{
    const qreal radians = bearing * kPi / 180.0;
    const qreal cosine = std::cos(radians);
    const qreal sine = std::sin(radians);
    const qreal left = std::isfinite(viewport.leftPadding) ? viewport.leftPadding : 0.0;
    const qreal right = std::isfinite(viewport.rightPadding) ? viewport.rightPadding : 0.0;
    const qreal top = std::isfinite(viewport.topPadding) ? viewport.topPadding : 0.0;
    const qreal bottom = std::isfinite(viewport.bottomPadding) ? viewport.bottomPadding : 0.0;
    const qreal xValues[] = {left - viewport.width / 2.0,
                             viewport.width - right - viewport.width / 2.0};
    const qreal yValues[] = {top - viewport.height / 2.0,
                             viewport.height - bottom - viewport.height / 2.0};
    ViewportOffsets result;
    result.minimumX = result.maximumX = cosine * xValues[0] - sine * yValues[0];
    result.minimumY = result.maximumY = sine * xValues[0] + cosine * yValues[0];
    for (qreal x : xValues) {
        for (qreal y : yValues) {
            const qreal rotatedX = cosine * x - sine * y;
            const qreal rotatedY = sine * x + cosine * y;
            result.minimumX = std::min(result.minimumX, rotatedX);
            result.maximumX = std::max(result.maximumX, rotatedX);
            result.minimumY = std::min(result.minimumY, rotatedY);
            result.maximumY = std::max(result.maximumY, rotatedY);
        }
    }
    return result;
}

qreal minimumViewportZoom(const CameraViewport &viewport, qreal bearing)
{
    if (!hasValidViewport(viewport))
        return BoundedCameraController::kMinimumZoom;

    const ViewportOffsets offsets = viewportOffsets(viewport, bearing);
    const qreal horizontalFootprint = offsets.maximumX - offsets.minimumX;
    const qreal verticalFootprint = offsets.maximumY - offsets.minimumY;
    const qreal longitudeSpan = contract::kEast - contract::kWest;
    const qreal latitudeSpan = mercatorY(contract::kSouth) - mercatorY(contract::kNorth);
    const qreal zoomX = std::log2(horizontalFootprint * 360.0 / (256.0 * longitudeSpan));
    const qreal zoomY = std::log2(verticalFootprint / (256.0 * latitudeSpan));
    return bounded(std::max({BoundedCameraController::kMinimumZoom, zoomX, zoomY}),
                   BoundedCameraController::kMinimumZoom, BoundedCameraController::kMaximumZoom);
}

} // namespace

BoundedCameraController::BoundedCameraController(QObject *parent) : QObject(parent) {}

QVariantMap BoundedCameraController::overviewCamera(qreal width, qreal height) const
{
    return cameraMap(normalizeForViewport(overview(), {width, height}));
}

QVariantMap BoundedCameraController::normalizedCamera(qreal latitude, qreal longitude, qreal zoom,
                                                      qreal bearing, qreal tilt, qreal width,
                                                      qreal height) const
{
    const CameraState state = normalizeForViewport(
        {QGeoCoordinate(latitude, longitude), zoom, bearing, tilt}, {width, height});
    return cameraMap(state);
}

QVariantMap BoundedCameraController::routeFitCamera(qreal south, qreal north, qreal west,
                                                    qreal east, qreal width, qreal height,
                                                    qreal leftPadding, qreal rightPadding,
                                                    qreal topPadding, qreal bottomPadding,
                                                    qreal bearing, qreal tilt) const
{
    CameraState result = overview();
    const CameraState current = normalize({overview().center, overview().zoom, bearing, tilt});
    const CameraViewport viewport = {width,        height,     leftPadding,
                                     rightPadding, topPadding, bottomPadding};
    if (!routeFit(QGeoCoordinate(south, west), QGeoCoordinate(north, east), viewport, current,
                  &result)) {
        return cameraMap(current, false);
    }
    return cameraMap(result);
}

QVariantMap BoundedCameraController::navigationCamera(qreal latitude, qreal longitude,
                                                      qreal bearing, qreal currentZoom, qreal width,
                                                      qreal height) const
{
    CameraState current = overview();
    current.zoom = currentZoom;
    const CameraState state = normalizeForViewport(
        navigationFollow(QGeoCoordinate(latitude, longitude), bearing, current), {width, height});
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
    if (requested.center.isValid() && std::isfinite(requested.center.latitude()) &&
        std::isfinite(requested.center.longitude())) {
        result.center =
            QGeoCoordinate(bounded(requested.center.latitude(), contract::kSouth, contract::kNorth),
                           bounded(requested.center.longitude(), contract::kWest, contract::kEast));
    }
    if (std::isfinite(requested.zoom))
        result.zoom = bounded(requested.zoom, kMinimumZoom, kMaximumZoom);
    result.bearing = normalizedBearing(requested.bearing);
    if (std::isfinite(requested.tilt))
        result.tilt = bounded(requested.tilt, 0.0, 60.0);
    return result;
}

CameraState BoundedCameraController::normalizeForViewport(const CameraState &requested,
                                                          const CameraViewport &viewport)
{
    CameraState result = normalize(requested);
    if (!hasValidViewport(viewport))
        return result;

    result.zoom = std::max(result.zoom, minimumViewportZoom(viewport, result.bearing));
    const qreal worldPixels = 256.0 * std::pow(2.0, result.zoom);
    const ViewportOffsets offsets = viewportOffsets(viewport, result.bearing);
    const qreal northY = mercatorY(contract::kNorth);
    const qreal southY = mercatorY(contract::kSouth);
    const qreal centerY = mercatorY(result.center.latitude());

    const qreal minimumLongitude = contract::kWest - offsets.minimumX * 360.0 / worldPixels;
    const qreal maximumLongitude = contract::kEast - offsets.maximumX * 360.0 / worldPixels;
    const qreal minimumY = northY - offsets.minimumY / worldPixels;
    const qreal maximumY = southY - offsets.maximumY / worldPixels;
    const qreal midpointLongitude = (contract::kWest + contract::kEast) / 2.0;
    const qreal midpointY = (northY + southY) / 2.0;
    const qreal longitude =
        minimumLongitude <= maximumLongitude
            ? bounded(result.center.longitude(), minimumLongitude, maximumLongitude)
            : midpointLongitude;
    const qreal y = minimumY <= maximumY ? bounded(centerY, minimumY, maximumY) : midpointY;
    result.center = QGeoCoordinate(latitudeFromMercator(y), longitude);
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
        !std::isfinite(viewport.bottomPadding) || viewport.leftPadding < 0.0 ||
        viewport.rightPadding < 0.0 || viewport.topPadding < 0.0 || viewport.bottomPadding < 0.0 ||
        !isWithinBounds(southwest) || !isWithinBounds(northeast)) {
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

    CameraState requested = normalize(current);
    requested.zoom =
        std::max(std::min(zoomX, zoomY), minimumViewportZoom(viewport, requested.bearing));
    requested.zoom = bounded(requested.zoom, kMinimumZoom, kMaximumZoom);
    const qreal worldPixels = 256.0 * std::pow(2.0, requested.zoom);
    const qreal routeCenterLongitude = (southwest.longitude() + northeast.longitude()) / 2.0;
    const qreal routeCenterY = (northY + southY) / 2.0;
    const qreal targetX = viewport.leftPadding + usableWidth / 2.0;
    const qreal targetY = viewport.topPadding + usableHeight / 2.0;
    requested.center = QGeoCoordinate(
        latitudeFromMercator(routeCenterY - (targetY - viewport.height / 2.0) / worldPixels),
        routeCenterLongitude - (targetX - viewport.width / 2.0) * 360.0 / worldPixels);
    *result = normalizeForViewport(requested, viewport);
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
