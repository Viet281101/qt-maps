#include "Polyline6Decoder.h"

#include <QtGlobal>

namespace qtmaps {
namespace routing {
namespace {

bool decodeComponent(const QByteArray &encoded, int *index, qint64 *value, QString *error)
{
    quint64 result = 0;
    int shift = 0;
    while (*index < encoded.size()) {
        const int character = static_cast<unsigned char>(encoded.at((*index)++));
        if (character < 63 || character > 126) {
            *error = QStringLiteral("Polyline contains a character outside the encoded range");
            return false;
        }
        const quint64 chunk = static_cast<quint64>(character - 63);
        if (shift > 60) {
            *error = QStringLiteral("Polyline component is too large");
            return false;
        }
        result |= (chunk & 0x1fU) << shift;
        if ((chunk & 0x20U) == 0) {
            const quint64 magnitude = result >> 1U;
            if (magnitude > 360000000U) {
                *error = QStringLiteral("Polyline coordinate delta is out of range");
                return false;
            }
            *value = (result & 1U) != 0
                ? -static_cast<qint64>(magnitude + 1U)
                : static_cast<qint64>(magnitude);
            return true;
        }
        shift += 5;
    }
    *error = QStringLiteral("Polyline ends in the middle of a coordinate");
    return false;
}

} // namespace

PolylineDecodeResult Polyline6Decoder::decode(const QByteArray &encoded)
{
    PolylineDecodeResult decoded;
    if (encoded.isEmpty()) {
        decoded.error = QStringLiteral("Polyline is empty");
        return decoded;
    }

    qint64 latitude = 0;
    qint64 longitude = 0;
    int index = 0;
    while (index < encoded.size()) {
        qint64 latitudeDelta = 0;
        qint64 longitudeDelta = 0;
        if (!decodeComponent(encoded, &index, &latitudeDelta, &decoded.error)
            || !decodeComponent(encoded, &index, &longitudeDelta, &decoded.error)) {
            decoded.coordinates.clear();
            return decoded;
        }
        latitude += latitudeDelta;
        longitude += longitudeDelta;
        const QGeoCoordinate coordinate(latitude / 1000000.0, longitude / 1000000.0);
        if (!coordinate.isValid()) {
            decoded.coordinates.clear();
            decoded.error = QStringLiteral("Polyline decodes to an invalid coordinate");
            return decoded;
        }
        decoded.coordinates.append(coordinate);
    }

    if (decoded.coordinates.size() < 2) {
        decoded.coordinates.clear();
        decoded.error = QStringLiteral("Route polyline must contain at least two coordinates");
    }
    return decoded;
}

} // namespace routing
} // namespace qtmaps
