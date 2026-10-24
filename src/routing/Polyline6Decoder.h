#pragma once

#include <QByteArray>
#include <QGeoCoordinate>
#include <QString>
#include <QVector>

namespace qtmaps {
namespace routing {

struct PolylineDecodeResult
{
    QVector<QGeoCoordinate> coordinates;
    QString error;

    bool ok() const { return error.isEmpty(); }
};

class Polyline6Decoder final
{
public:
    static PolylineDecodeResult decode(const QByteArray &encoded);
};

} // namespace routing
} // namespace qtmaps
