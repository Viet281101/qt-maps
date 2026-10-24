#include <QtTest>

#include "routing/Polyline6Decoder.h"
#include "routing/ValhallaRouteParser.h"

#include <QFile>

#include <cmath>

using qtmaps::routing::Polyline6Decoder;
using qtmaps::routing::RouteParseResult;
using qtmaps::routing::RouteSource;
using qtmaps::routing::ValhallaRouteParser;

namespace {

QByteArray fixture(const QString &name)
{
    QFile file(QStringLiteral(QTMAPS_SOURCE_DIR "/tests/fixtures/routing/") + name);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return file.readAll();
}

void compareCoordinate(const QGeoCoordinate &actual, double latitude, double longitude)
{
    QVERIFY(std::abs(actual.latitude() - latitude) < 0.0000005);
    QVERIFY(std::abs(actual.longitude() - longitude) < 0.0000005);
}

} // namespace

class RoutingTest final : public QObject
{
    Q_OBJECT

private slots:
    void decodesPolyline6WithPositiveAndNegativeDeltas()
    {
        const auto decoded = Polyline6Decoder::decode(
            QByteArrayLiteral("wbmbg@wwu{hEoeGvcAo~GnwHw`EfoSnpI~rNfrOvnCf`NotL"));

        QVERIFY2(decoded.ok(), qPrintable(decoded.error));
        QCOMPARE(decoded.coordinates.size(), 7);
        compareCoordinate(decoded.coordinates.at(0), 21.02790, 105.85230);
        compareCoordinate(decoded.coordinates.at(3), 21.03980, 105.83570);
        compareCoordinate(decoded.coordinates.at(6), 21.01820, 105.83240);
    }

    void rejectsMalformedPolyline6()
    {
        QVERIFY(!Polyline6Decoder::decode(QByteArray()).ok());
        const auto truncated = Polyline6Decoder::decode(QByteArrayLiteral("~"));
        QVERIFY(!truncated.ok());
        QVERIFY(truncated.error.contains(QStringLiteral("middle")));

        QByteArray invalid;
        invalid.append(char(10));
        invalid.append(char(10));
        QVERIFY(!Polyline6Decoder::decode(invalid).ok());

        QVERIFY(!Polyline6Decoder::decode(QByteArrayLiteral("~~~~~~~~~~~~?~")).ok());
    }

    void parsesHoanKiemToThaiHaFixture()
    {
        const QByteArray response = fixture(QStringLiteral("hanoi_hoan_kiem_to_thai_ha.json"));
        QVERIFY(!response.isEmpty());
        const RouteParseResult parsed = ValhallaRouteParser::parse(response, RouteSource::Fixture);

        QVERIFY2(parsed.ok(), qPrintable(parsed.error));
        QCOMPARE(parsed.route.source, RouteSource::Fixture);
        QCOMPARE(parsed.route.polyline.size(), 7);
        QCOMPARE(parsed.route.maneuvers.size(), 7);
        QCOMPARE(parsed.route.distanceMeters, 5475.0);
        QCOMPARE(parsed.route.durationSeconds, 720.0);
        QCOMPARE(parsed.route.maneuvers.at(1).type, 15);
        QCOMPARE(parsed.route.maneuvers.at(1).streetName, QStringLiteral("Phố Tràng Thi"));
        QCOMPARE(parsed.route.maneuvers.at(1).distanceMeters, 729.0);
        compareCoordinate(parsed.route.maneuvers.at(1).position, 21.03210, 105.85120);
        QCOMPARE(parsed.route.maneuvers.constLast().shapeIndex, 6);
    }

    void parsesSecondHanoiFixture()
    {
        const QByteArray response = fixture(QStringLiteral("hanoi_hoan_kiem_to_ba_dinh.json"));
        QVERIFY(!response.isEmpty());
        const RouteParseResult parsed = ValhallaRouteParser::parse(response, RouteSource::Fixture);

        QVERIFY2(parsed.ok(), qPrintable(parsed.error));
        QCOMPARE(parsed.route.polyline.size(), 5);
        QCOMPARE(parsed.route.maneuvers.size(), 5);
        QCOMPARE(parsed.route.distanceMeters, 2600.0);
        QCOMPARE(parsed.route.maneuvers.at(3).streetName, QStringLiteral("Đường Hùng Vương"));
        compareCoordinate(parsed.route.polyline.constLast(), 21.03670, 105.82980);
    }

    void rejectsManeuverOutsideShape()
    {
        QByteArray response = fixture(QStringLiteral("hanoi_hoan_kiem_to_ba_dinh.json"));
        QVERIFY(!response.isEmpty());
        response.replace("\"begin_shape_index\": 4", "\"begin_shape_index\": 99");
        const RouteParseResult parsed = ValhallaRouteParser::parse(response);

        QVERIFY(!parsed.ok());
        QVERIFY(parsed.error.contains(QStringLiteral("outside")));
    }

    void rejectsUnsupportedDistanceUnits()
    {
        QByteArray response = fixture(QStringLiteral("hanoi_hoan_kiem_to_ba_dinh.json"));
        QVERIFY(!response.isEmpty());
        response.replace("\"units\": \"kilometers\"", "\"units\": \"miles\"");
        const RouteParseResult parsed = ValhallaRouteParser::parse(response);

        QVERIFY(!parsed.ok());
        QVERIFY(parsed.error.contains(QStringLiteral("kilometers")));
    }

    void rejectsNegativeManeuverIndex()
    {
        QByteArray response = fixture(QStringLiteral("hanoi_hoan_kiem_to_ba_dinh.json"));
        QVERIFY(!response.isEmpty());
        response.replace("\"begin_shape_index\": 0", "\"begin_shape_index\": -1");
        const RouteParseResult parsed = ValhallaRouteParser::parse(response);

        QVERIFY(!parsed.ok());
        QVERIFY(parsed.error.contains(QStringLiteral("numeric")));
    }
};

QTEST_GUILESS_MAIN(RoutingTest)

#include "tst_Routing.moc"
