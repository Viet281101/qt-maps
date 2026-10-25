#include <QtTest>

#include "routing/RouteNavigationController.h"

#include <QSignalSpy>

using qtmaps::routing::Route;
using qtmaps::routing::RouteManeuver;
using qtmaps::routing::RouteNavigationController;

namespace {

Route simpleRoute()
{
    Route route;
    route.polyline = {QGeoCoordinate(21.0, 105.0),
                      QGeoCoordinate(21.0, 105.001),
                      QGeoCoordinate(21.001, 105.001)};
    route.distanceMeters = route.polyline.at(0).distanceTo(route.polyline.at(1))
        + route.polyline.at(1).distanceTo(route.polyline.at(2));
    route.durationSeconds = 100.0;
    RouteManeuver depart;
    depart.instruction = QStringLiteral("Đi thẳng");
    depart.shapeIndex = 0;
    depart.position = route.polyline.at(0);
    RouteManeuver arrive;
    arrive.instruction = QStringLiteral("Đã đến nơi");
    arrive.shapeIndex = 2;
    arrive.position = route.polyline.at(2);
    route.maneuvers = {depart, arrive};
    return route;
}

} // namespace

class RouteNavigationTest final : public QObject
{
    Q_OBJECT

private slots:
    void rejectsStartWithoutRoute()
    {
        RouteNavigationController controller;
        controller.start();
        QVERIFY(!controller.running());
        QVERIFY(controller.statusMessage().contains(QStringLiteral("chưa có tuyến")));
    }

    void replaysSelectedRouteAndSupportsPauseStopAndSpeed()
    {
        RouteNavigationController controller;
        const Route route = simpleRoute();
        controller.setRoute(route, QStringLiteral("fixture/mô phỏng"));
        QVERIFY(controller.routeAvailable());
        QCOMPARE(controller.sourceLabel(), QStringLiteral("fixture/mô phỏng"));
        QVERIFY(controller.coordinate().distanceTo(route.polyline.constFirst()) < 0.1);

        QVERIFY(controller.setSpeedMultiplier(2));
        controller.start();
        QVERIFY(controller.running());
        controller.advanceBySeconds(25.0);
        QVERIFY(qAbs(controller.travelledMeters() - controller.routeLengthMeters() / 2.0)
                < 0.5);
        const double pausedDistance = controller.travelledMeters();
        controller.pause();
        QVERIFY(controller.paused());
        controller.advanceBySeconds(10.0);
        QCOMPARE(controller.travelledMeters(), pausedDistance);

        controller.start();
        controller.advanceBySeconds(30.0);
        QVERIFY(controller.arrived());
        QVERIFY(controller.coordinate().distanceTo(route.polyline.constLast()) < 0.1);

        controller.stop();
        QVERIFY(!controller.active());
        QCOMPARE(controller.travelledMeters(), 0.0);
        QVERIFY(controller.coordinate().distanceTo(route.polyline.constFirst()) < 0.1);
    }

    void rejectsUnsupportedReplaySpeed()
    {
        RouteNavigationController controller;
        controller.setRoute(simpleRoute(), QStringLiteral("Valhalla local"));
        QVERIFY(!controller.setSpeedMultiplier(3));
        QCOMPARE(controller.speedMultiplier(), 1);
        QVERIFY(controller.statusMessage().contains(QStringLiteral("1x, 2x hoặc 4x")));
    }

    void advancesManeuverAndRemainingGuidance()
    {
        RouteNavigationController controller;
        Route route = simpleRoute();
        route.maneuvers[0].streetName = QStringLiteral("Phố đầu");
        RouteManeuver turn;
        turn.instruction = QStringLiteral("Rẽ phải");
        turn.streetName = QStringLiteral("Phố tiếp theo");
        turn.shapeIndex = 1;
        turn.position = route.polyline.at(1);
        route.maneuvers.insert(1, turn);

        controller.setRoute(route, QStringLiteral("Valhalla local"));
        QCOMPARE(controller.currentManeuverIndex(), 0);
        QCOMPARE(controller.maneuverInstruction(), QStringLiteral("Đi thẳng"));
        QCOMPARE(controller.remainingDistanceMeters(), route.distanceMeters);
        QCOMPARE(controller.remainingDurationSeconds(), route.durationSeconds);

        controller.start();
        controller.advanceBySeconds(25.0);
        QCOMPARE(controller.currentManeuverIndex(), 1);
        QCOMPARE(controller.maneuverStreetName(), QStringLiteral("Phố tiếp theo"));
        QVERIFY(controller.distanceToManeuverMeters() > 0.0);
        QVERIFY(controller.remainingDistanceMeters() < route.distanceMeters);
        QVERIFY(controller.remainingDurationSeconds() < route.durationSeconds);
    }
};

QTEST_GUILESS_MAIN(RouteNavigationTest)

#include "tst_RouteNavigation.moc"
