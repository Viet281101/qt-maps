#include "ValhallaRouteParser.h"

#include "Polyline6Decoder.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <cmath>
#include <limits>

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

bool readNonNegativeInteger(const QJsonObject &object, const QString &key, int *value)
{
    const QJsonValue field = object.value(key);
    const double number = field.toDouble(-1.0);
    if (!field.isDouble() || !std::isfinite(number) || number < 0.0 ||
        std::floor(number) != number ||
        number > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }
    *value = static_cast<int>(number);
    return true;
}

bool sameCoordinate(const QGeoCoordinate &left, const QGeoCoordinate &right)
{
    return std::abs(left.latitude() - right.latitude()) < 0.0000005 &&
           std::abs(left.longitude() - right.longitude()) < 0.0000005;
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
    double distance = 0.0;
    if (trip.isEmpty() || summary.isEmpty() ||
        !readNonNegativeNumber(summary, QStringLiteral("length"), &distance) ||
        !readNonNegativeNumber(summary, QStringLiteral("time"), &result.route.durationSeconds)) {
        result.error = QStringLiteral("Valhalla response has an invalid trip summary");
        return result;
    }
    const QString units =
        trip.value(QStringLiteral("units")).toString(QStringLiteral("kilometers"));
    double metersPerUnit = 0.0;
    if (units == QStringLiteral("kilometers")) {
        metersPerUnit = 1000.0;
    } else if (units == QStringLiteral("miles")) {
        metersPerUnit = 1609.344;
    } else {
        result.error =
            QStringLiteral("Valhalla response uses unsupported distance units: %1").arg(units);
        return result;
    }
    result.route.distanceMeters = distance * metersPerUnit;

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
        if (!result.route.polyline.isEmpty() &&
            sameCoordinate(result.route.polyline.constLast(), decoded.coordinates.constFirst())) {
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
            int legShapeIndex = -1;
            double length = 0.0;
            if (!readNonNegativeInteger(object, QStringLiteral("type"), &maneuver.type) ||
                !readNonNegativeInteger(object, QStringLiteral("begin_shape_index"),
                                        &legShapeIndex) ||
                !readNonNegativeNumber(object, QStringLiteral("length"), &length) ||
                !readNonNegativeNumber(object, QStringLiteral("time"), &maneuver.durationSeconds)) {
                result.error = QStringLiteral("Valhalla maneuver has invalid numeric fields");
                return result;
            }
            if (legShapeIndex > std::numeric_limits<int>::max() - shapeIndexOffset) {
                result.error = QStringLiteral("Valhalla maneuver shape index is outside the route");
                return result;
            }
            maneuver.shapeIndex = shapeIndexOffset + legShapeIndex;
            if (maneuver.shapeIndex < 0 || maneuver.shapeIndex >= result.route.polyline.size()) {
                result.error = QStringLiteral("Valhalla maneuver shape index is outside the route");
                return result;
            }
            maneuver.instruction = object.value(QStringLiteral("instruction")).toString();
            const QJsonArray streetNames = object.value(QStringLiteral("street_names")).toArray();
            if (!streetNames.isEmpty()) {
                maneuver.streetName = streetNames.at(0).toString();
            }
            maneuver.distanceMeters = length * metersPerUnit;
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
