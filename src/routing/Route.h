#pragma once

#include <QGeoCoordinate>
#include <QMetaType>
#include <QString>
#include <QVector>

namespace qtmaps {
namespace routing {

enum class RouteSource
{
    Fixture,
    Valhalla,
};

struct RouteManeuver
{
    int type = 0;
    QString instruction;
    QString streetName;
    double distanceMeters = 0.0;
    double durationSeconds = 0.0;
    int shapeIndex = -1;
    QGeoCoordinate position;
};

struct Route
{
    QVector<QGeoCoordinate> polyline;
    double distanceMeters = 0.0;
    double durationSeconds = 0.0;
    QVector<RouteManeuver> maneuvers;
    RouteSource source = RouteSource::Fixture;

    bool isValid() const
    {
        return polyline.size() >= 2 && distanceMeters >= 0.0 && durationSeconds >= 0.0 &&
               !maneuvers.isEmpty();
    }
};

} // namespace routing
} // namespace qtmaps

Q_DECLARE_METATYPE(qtmaps::routing::Route)
