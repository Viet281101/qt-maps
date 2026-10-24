#include "ValhallaRouteParser.h"

#include "Polyline6Decoder.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <cmath>

namespace qtmaps {
namespace routing {
namespace {

bool readNonNegativeNumber(const QJsonObject &object, const QString &key, double *value)
{
    const QJsonValue field = object.value(key);
    if (!field.isDouble() || !std::isfinite(field.toDouble()) || field.toDouble() < 0.0) {
        return false;
    }
    *value = field.toDouble();
    return true;
}

bool sameCoordinate(const QGeoCoordinate &left, const QGeoCoordinate &right)
{
    return std::abs(left.latitude() - right.latitude()) < 0.0000005
        && std::abs(left.longitude() - right.longitude()) < 0.0000005;
}

} // namespace

RouteParseResult ValhallaRouteParser::parse(const QByteArray &response, RouteSource source)
{
    RouteParseResult result;
    result.route.source = source;

    QJsonParseError jsonError;
    const QJsonDocument document = QJsonDocument::fromJson(response, &jsonError);
    if (jsonError.error != QJsonParseError::NoError || !document.isObject()) {
        result.error = QStringLiteral("Invalid Valhalla JSON: %1").arg(jsonError.errorString());
        return result;
    }

    const QJsonObject trip = document.object().value(QStringLiteral("trip")).toObject();
    const QJsonObject summary = trip.value(QStringLiteral("summary")).toObject();
    double distanceKilometers = 0.0;
    if (trip.isEmpty() || summary.isEmpty()
        || !readNonNegativeNumber(summary, QStringLiteral("length"), &distanceKilometers)
        || !readNonNegativeNumber(summary, QStringLiteral("time"), &result.route.durationSeconds)) {
        result.error = QStringLiteral("Valhalla response has an invalid trip summary");
        return result;
    }
    if (trip.value(QStringLiteral("units")).toString() != QStringLiteral("kilometers")) {
        result.error = QStringLiteral("Valhalla fixture must report distance in kilometers");
        return result;
    }
    result.route.distanceMeters = distanceKilometers * 1000.0;

    const QJsonArray legs = trip.value(QStringLiteral("legs")).toArray();
    if (legs.isEmpty()) {
        result.error = QStringLiteral("Valhalla response contains no route legs");
        return result;
    }

    for (const QJsonValue &legValue : legs) {
        const QJsonObject leg = legValue.toObject();
        const PolylineDecodeResult decoded =
            Polyline6Decoder::decode(leg.value(QStringLiteral("shape")).toString().toLatin1());
        if (!decoded.ok()) {
            result.error = QStringLiteral("Invalid Valhalla leg shape: %1").arg(decoded.error);
            return result;
        }

        const int legStart = result.route.polyline.size();
        int firstCoordinate = 0;
        if (!result.route.polyline.isEmpty()
            && sameCoordinate(result.route.polyline.constLast(), decoded.coordinates.constFirst())) {
            firstCoordinate = 1;
        }
        for (int index = firstCoordinate; index < decoded.coordinates.size(); ++index) {
            result.route.polyline.append(decoded.coordinates.at(index));
        }
        const int shapeIndexOffset = legStart - firstCoordinate;

        const QJsonArray maneuvers = leg.value(QStringLiteral("maneuvers")).toArray();
        if (maneuvers.isEmpty()) {
            result.error = QStringLiteral("Valhalla route leg contains no maneuvers");
            return result;
        }
        for (const QJsonValue &maneuverValue : maneuvers) {
            const QJsonObject object = maneuverValue.toObject();
            RouteManeuver maneuver;
            const QJsonValue type = object.value(QStringLiteral("type"));
            const QJsonValue shapeIndex = object.value(QStringLiteral("begin_shape_index"));
            double lengthKilometers = 0.0;
            if (!type.isDouble() || std::floor(type.toDouble()) != type.toDouble()
                || !shapeIndex.isDouble()
                || std::floor(shapeIndex.toDouble()) != shapeIndex.toDouble()
                || !readNonNegativeNumber(object, QStringLiteral("length"), &lengthKilometers)
                || !readNonNegativeNumber(object, QStringLiteral("time"),
                                          &maneuver.durationSeconds)) {
                result.error = QStringLiteral("Valhalla maneuver has invalid numeric fields");
                return result;
            }
            maneuver.type = type.toInt();
            maneuver.shapeIndex = shapeIndexOffset + shapeIndex.toInt();
            if (maneuver.shapeIndex < 0 || maneuver.shapeIndex >= result.route.polyline.size()) {
                result.error = QStringLiteral("Valhalla maneuver shape index is outside the route");
                return result;
            }
            maneuver.instruction = object.value(QStringLiteral("instruction")).toString();
            const QJsonArray streetNames = object.value(QStringLiteral("street_names")).toArray();
            if (!streetNames.isEmpty()) {
                maneuver.streetName = streetNames.at(0).toString();
            }
            maneuver.distanceMeters = lengthKilometers * 1000.0;
            maneuver.position = result.route.polyline.at(maneuver.shapeIndex);
            result.route.maneuvers.append(maneuver);
        }
    }

    if (!result.route.isValid()) {
        result.error = QStringLiteral("Valhalla response did not produce a usable route");
    }
    return result;
}

} // namespace routing
} // namespace qtmaps
