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

    void handlesRouteStartTurnBoundaryAndArrival()
    {
        RouteNavigationController controller;
        Route route = simpleRoute();
        RouteManeuver turn;
        turn.instruction = QStringLiteral("Rẽ trái");
        turn.shapeIndex = 1;
        turn.position = route.polyline.at(1);
        route.maneuvers.insert(1, turn);
        controller.setRoute(route, QStringLiteral("fixture/mô phỏng"));

        QCOMPARE(controller.currentManeuverIndex(), 0);
        QCOMPARE(controller.distanceToManeuverMeters(), 0.0);

        controller.start();
        controller.advanceBySeconds(0.1);
        QCOMPARE(controller.currentManeuverIndex(), 1);

        const double firstSegment = route.polyline.at(0).distanceTo(route.polyline.at(1));
        const double secondsToTurn = firstSegment / controller.routeLengthMeters()
            * route.durationSeconds;
        controller.advanceBySeconds(secondsToTurn);
        QCOMPARE(controller.currentManeuverIndex(), 2);

        controller.advanceBySeconds(route.durationSeconds);
        QVERIFY(controller.arrived());
        QCOMPARE(controller.currentManeuverIndex(), 2);
        QCOMPARE(controller.distanceToManeuverMeters(), 0.0);
        QVERIFY(qAbs(controller.remainingDistanceMeters()) < 0.001);
        QVERIFY(qAbs(controller.remainingDurationSeconds()) < 0.001);
    }

    void skipsDuplicateManeuverPositionsAfterDeparture()
    {
        Route route = simpleRoute();
        RouteManeuver duplicateDeparture;
        duplicateDeparture.instruction = QStringLiteral("Tiếp tục");
        duplicateDeparture.shapeIndex = 0;
        duplicateDeparture.position = route.polyline.at(0);
        RouteManeuver turn;
        turn.instruction = QStringLiteral("Rẽ phải");
        turn.shapeIndex = 1;
        turn.position = route.polyline.at(1);
        route.maneuvers.insert(1, duplicateDeparture);
        route.maneuvers.insert(2, turn);

        RouteNavigationController controller;
        controller.setRoute(route, QStringLiteral("Valhalla local"));
        QCOMPARE(controller.currentManeuverIndex(), 0);
        controller.start();
        controller.advanceBySeconds(0.1);
        QCOMPARE(controller.currentManeuverIndex(), 2);
        QCOMPARE(controller.maneuverInstruction(), QStringLiteral("Rẽ phải"));
    }

    void rejectsManeuverOutsideRouteGeometry()
    {
        Route route = simpleRoute();
        route.maneuvers[0].shapeIndex = route.polyline.size();

        RouteNavigationController controller;
        controller.setRoute(route, QStringLiteral("fixture/mô phỏng"));
        QVERIFY(!controller.routeAvailable());
        QCOMPARE(controller.currentManeuverIndex(), -1);
        QVERIFY(controller.statusMessage().contains(QStringLiteral("maneuver ngoài hình học")));
    }

    void rejectsRouteWithoutReplayDuration()
    {
        Route route = simpleRoute();
        route.durationSeconds = 0.0;

        RouteNavigationController controller;
        controller.setRoute(route, QStringLiteral("fixture/mô phỏng"));
        QVERIFY(!controller.routeAvailable());
        QVERIFY(controller.statusMessage().contains(QStringLiteral("thiếu quãng đường")));
    }
};

QTEST_GUILESS_MAIN(RouteNavigationTest)

#include "tst_RouteNavigation.moc"
