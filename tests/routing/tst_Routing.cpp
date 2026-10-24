#include <QtTest>

#include "routing/Polyline6Decoder.h"
#include "routing/ValhallaRouteParser.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

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
        QCOMPARE(parsed.route.polyline.size(), 320);
        QCOMPARE(parsed.route.maneuvers.size(), 14);
        QCOMPARE(parsed.route.distanceMeters, 7294.0);
        QCOMPARE(parsed.route.durationSeconds, 523.659);
        QCOMPARE(parsed.route.maneuvers.at(4).type, 10);
        QCOMPARE(parsed.route.maneuvers.at(4).streetName, QStringLiteral("Phố Tràng Thi"));
        QCOMPARE(parsed.route.maneuvers.at(4).distanceMeters, 1256.0);
        QCOMPARE(parsed.route.maneuvers.at(4).shapeIndex, 91);
        QCOMPARE(parsed.route.maneuvers.constLast().shapeIndex, 319);
    }

    void parsesSecondHanoiFixture()
    {
        const QByteArray response = fixture(QStringLiteral("hanoi_hoan_kiem_to_ba_dinh.json"));
        QVERIFY(!response.isEmpty());
        const RouteParseResult parsed = ValhallaRouteParser::parse(response, RouteSource::Fixture);

        QVERIFY2(parsed.ok(), qPrintable(parsed.error));
        QCOMPARE(parsed.route.polyline.size(), 233);
        QCOMPARE(parsed.route.maneuvers.size(), 11);
        QCOMPARE(parsed.route.distanceMeters, 4262.0);
        QCOMPARE(parsed.route.maneuvers.at(7).streetName, QStringLiteral("Phố Lê Hồng Phong"));
        QCOMPARE(parsed.route.maneuvers.constLast().shapeIndex, 232);
    }

    void fixturesAreRecordedFromValhalla()
    {
        for (const QString &name : {QStringLiteral("hanoi_hoan_kiem_to_thai_ha.json"),
                                    QStringLiteral("hanoi_hoan_kiem_to_ba_dinh.json")}) {
            const QJsonObject root = QJsonDocument::fromJson(fixture(name)).object();
            const QJsonObject metadata = root.value(QStringLiteral("fixture_metadata")).toObject();
            QCOMPARE(metadata.value(QStringLiteral("source")).toString(),
                     QStringLiteral("recorded from local Valhalla 3.9.1"));
            QCOMPARE(metadata.value(QStringLiteral("recorded_date")).toString(),
                     QStringLiteral("2026-10-24"));
            QVERIFY(metadata.value(QStringLiteral("backend_calculated")).toBool());
        }
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

    void defaultsMissingUnitsToKilometersAndKeepsSeconds()
    {
        QByteArray response = fixture(QStringLiteral("hanoi_hoan_kiem_to_ba_dinh.json"));
        QVERIFY(!response.isEmpty());
        response.replace("    \"units\": \"kilometers\",\n", "");
        const RouteParseResult parsed = ValhallaRouteParser::parse(response);

        QVERIFY2(parsed.ok(), qPrintable(parsed.error));
        QCOMPARE(parsed.route.distanceMeters, 4262.0);
        QCOMPARE(parsed.route.durationSeconds, 392.156);
        QCOMPARE(parsed.route.maneuvers.at(0).distanceMeters, 10.0);
        QCOMPARE(parsed.route.maneuvers.at(0).durationSeconds, 3.6);
    }

    void convertsExplicitMilesToMetersAndKeepsSeconds()
    {
        QByteArray response = fixture(QStringLiteral("hanoi_hoan_kiem_to_ba_dinh.json"));
        QVERIFY(!response.isEmpty());
        response.replace("\"units\": \"kilometers\"", "\"units\": \"miles\"");
        const RouteParseResult parsed = ValhallaRouteParser::parse(response);

        QVERIFY2(parsed.ok(), qPrintable(parsed.error));
        QVERIFY(std::abs(parsed.route.distanceMeters - 6859.024128) < 0.0001);
        QCOMPARE(parsed.route.durationSeconds, 392.156);
        QVERIFY(std::abs(parsed.route.maneuvers.at(0).distanceMeters - 16.09344) < 0.0001);
        QCOMPARE(parsed.route.maneuvers.at(0).durationSeconds, 3.6);
    }

    void rejectsUnsupportedDistanceUnits()
    {
        QByteArray response = fixture(QStringLiteral("hanoi_hoan_kiem_to_ba_dinh.json"));
        QVERIFY(!response.isEmpty());
        response.replace("\"units\": \"kilometers\"", "\"units\": \"nautical_miles\"");
        const RouteParseResult parsed = ValhallaRouteParser::parse(response);

        QVERIFY(!parsed.ok());
        QVERIFY(parsed.error.contains(QStringLiteral("unsupported")));
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
