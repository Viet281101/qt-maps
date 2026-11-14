#include "diagnostics/CameraProbeController.h"
#include "map/BoundedCameraController.h"

#include <QSignalSpy>
#include <QTest>

#include <cmath>
#include <limits>

using qtmaps::diagnostics::CameraProbeController;
using qtmaps::map::BoundedCameraController;
using qtmaps::map::CameraState;
using qtmaps::map::CameraViewport;

class CameraProbeControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void normalizesBearing()
    {
        QCOMPARE(CameraProbeController::normalizedBearing(-45.0), 315.0);
        QCOMPARE(CameraProbeController::normalizedBearing(405.0), 45.0);
        QCOMPARE(CameraProbeController::normalizedBearing(720.0), 0.0);
    }

    void boundsTiltAndHandlesReversedLimits()
    {
        QCOMPARE(CameraProbeController::boundedTilt(45.0, 0.0, 30.0), 30.0);
        QCOMPARE(CameraProbeController::boundedTilt(-5.0, 0.0, 30.0), 0.0);
        QCOMPARE(CameraProbeController::boundedTilt(20.0, 30.0, 0.0), 20.0);
    }

    void advancesThroughRequestedCameraSequence()
    {
        CameraProbeController controller;
        QSignalSpy changed(&controller, &CameraProbeController::cameraChanged);

        controller.advance(0.0, 40.0);
        QCOMPARE(controller.requestedBearing(), 45.0);
        QCOMPARE(controller.requestedTilt(), 30.0);
        QCOMPARE(controller.stepName(), QStringLiteral("northeast-30"));

        controller.advance(0.0, 40.0);
        QCOMPARE(controller.requestedBearing(), 315.0);
        QCOMPARE(controller.requestedTilt(), 40.0);
        QCOMPARE(controller.stepName(), QStringLiteral("northwest-45"));

        controller.reset();
        QCOMPARE(controller.requestedBearing(), 0.0);
        QCOMPARE(controller.requestedTilt(), 0.0);
        QCOMPARE(changed.count(), 3);
    }

    void boundsCameraStateAndFallsBackForInvalidCoordinates()
    {
        CameraState requested;
        requested.center = QGeoCoordinate(22.0, 106.0);
        requested.zoom = 99.0;
        requested.bearing = -45.0;
        requested.tilt = 90.0;
        const CameraState bounded = BoundedCameraController::normalize(requested);
        QCOMPARE(bounded.center.latitude(), 21.10);
        QCOMPARE(bounded.center.longitude(), 105.90);
        QCOMPARE(bounded.zoom, BoundedCameraController::kMaximumZoom);
        QCOMPARE(bounded.bearing, 315.0);
        QCOMPARE(bounded.tilt, 60.0);

        requested.center = QGeoCoordinate();
        QVERIFY(BoundedCameraController::isWithinBounds(
            BoundedCameraController::normalize(requested).center));
    }

    void replacesNonFiniteCameraFieldsWithSafeDefaults()
    {
        CameraState requested = BoundedCameraController::overview();
        requested.zoom = std::numeric_limits<qreal>::infinity();
        requested.bearing = std::numeric_limits<qreal>::quiet_NaN();
        requested.tilt = -std::numeric_limits<qreal>::infinity();

        const CameraState bounded = BoundedCameraController::normalize(requested);
        QVERIFY(std::isfinite(bounded.zoom));
        QVERIFY(std::isfinite(bounded.bearing));
        QVERIFY(std::isfinite(bounded.tilt));
        QCOMPARE(bounded.zoom, BoundedCameraController::overview().zoom);
        QCOMPARE(bounded.bearing, 0.0);
        QCOMPARE(bounded.tilt, 0.0);
    }

    void followsNavigationWithoutLeavingThePackage()
    {
        CameraState current = BoundedCameraController::overview();
        current.zoom = 13.0;
        const CameraState followed =
            BoundedCameraController::navigationFollow(QGeoCoordinate(20.0, 110.0), 405.0, current);
        QVERIFY(BoundedCameraController::isWithinBounds(followed.center));
        QCOMPARE(followed.zoom, BoundedCameraController::kNavigationZoom);
        QCOMPARE(followed.bearing, 45.0);
        QCOMPARE(followed.tilt, 45.0);
    }

    void fitsRouteWithPanelSafePadding()
    {
        const CameraViewport viewport = {1366.0, 768.0, 362.0, 48.0, 32.0, 32.0};
        CameraState fitted;
        QVERIFY(BoundedCameraController::routeFit(QGeoCoordinate(21.0000, 105.8000),
                                                  QGeoCoordinate(21.0500, 105.8500), viewport,
                                                  BoundedCameraController::overview(), &fitted));
        QVERIFY(BoundedCameraController::isWithinBounds(fitted.center));
        QVERIFY(fitted.zoom >= BoundedCameraController::kMinimumZoom);
        QVERIFY(fitted.zoom <= BoundedCameraController::kMaximumZoom);
        QVERIFY(fitted.center.longitude() < 105.8250);

        CameraState repeated;
        QVERIFY(BoundedCameraController::routeFit(QGeoCoordinate(21.0000, 105.8000),
                                                  QGeoCoordinate(21.0500, 105.8500), viewport,
                                                  BoundedCameraController::overview(), &repeated));
        QCOMPARE(repeated.center.latitude(), fitted.center.latitude());
        QCOMPARE(repeated.center.longitude(), fitted.center.longitude());
        QCOMPARE(repeated.zoom, fitted.zoom);
    }

    void fitsRouteUsingTheClampedZoomForItsPaddingOffset()
    {
        const CameraViewport viewport = {900.0, 580.0, 362.0, 48.0, 32.0, 32.0};
        const QGeoCoordinate southwest(21.02000, 105.82000);
        const QGeoCoordinate northeast(21.02001, 105.82001);
        CameraState fitted;
        QVERIFY(BoundedCameraController::routeFit(southwest, northeast, viewport,
                                                  BoundedCameraController::overview(), &fitted));
        QCOMPARE(fitted.zoom, BoundedCameraController::kMaximumZoom);

        const qreal targetX = viewport.leftPadding +
                              (viewport.width - viewport.leftPadding - viewport.rightPadding) / 2.0;
        const qreal expectedLongitude =
            (southwest.longitude() + northeast.longitude()) / 2.0 -
            (targetX - viewport.width / 2.0) * 360.0 / (256.0 * std::pow(2.0, fitted.zoom));
        QVERIFY(std::abs(fitted.center.longitude() - expectedLongitude) < 0.000001);
    }

    void fitsRotatedRouteInsideThePaddedViewport()
    {
        const CameraViewport viewport = {1366.0, 768.0, 362.0, 48.0, 32.0, 32.0};
        const QGeoCoordinate southwest(21.0200, 105.7800);
        const QGeoCoordinate northeast(21.0250, 105.8800);
        CameraState current = BoundedCameraController::overview();
        current.bearing = 90.0;
        CameraState fitted;
        QVERIFY(
            BoundedCameraController::routeFit(southwest, northeast, viewport, current, &fitted));

        const qreal worldPixels = 256.0 * std::pow(2.0, fitted.zoom);
        const auto mercatorY = [](qreal latitude) {
            constexpr qreal pi = 3.14159265358979323846;
            const qreal radians = latitude * pi / 180.0;
            return (1.0 - std::log(std::tan(radians) + 1.0 / std::cos(radians)) / pi) / 2.0;
        };
        const auto screenPosition = [&](const QGeoCoordinate &coordinate) {
            const qreal x =
                (coordinate.longitude() - fitted.center.longitude()) / 360.0 * worldPixels;
            const qreal y =
                (mercatorY(coordinate.latitude()) - mercatorY(fitted.center.latitude())) *
                worldPixels;
            return QPointF(viewport.width / 2.0 + y, viewport.height / 2.0 - x);
        };

        const QGeoCoordinate corners[] = {
            southwest,
            QGeoCoordinate(southwest.latitude(), northeast.longitude()),
            QGeoCoordinate(northeast.latitude(), southwest.longitude()),
            northeast,
        };
        for (const QGeoCoordinate &corner : corners) {
            const QPointF screen = screenPosition(corner);
            QVERIFY(screen.x() >= viewport.leftPadding - 0.001);
            QVERIFY(screen.x() <= viewport.width - viewport.rightPadding + 0.001);
            QVERIFY(screen.y() >= viewport.topPadding - 0.001);
            QVERIFY(screen.y() <= viewport.height - viewport.bottomPadding + 0.001);
        }
    }

    void keepsTheViewportFootprintInsideThePackage()
    {
        const CameraViewport viewport = {1920.0, 1080.0};
        CameraState requested = BoundedCameraController::overview();
        requested.center = QGeoCoordinate(21.10, 105.90);
        requested.zoom = BoundedCameraController::kMinimumZoom;
        const CameraState bounded =
            BoundedCameraController::normalizeForViewport(requested, viewport);

        QVERIFY(bounded.zoom > BoundedCameraController::overview().zoom);
        const qreal longitudeEdge = viewport.width * 180.0 / (256.0 * std::pow(2.0, bounded.zoom));
        QVERIFY(bounded.center.longitude() - longitudeEdge >= 105.78 - 0.000001);
        QVERIFY(bounded.center.longitude() + longitudeEdge <= 105.90 + 0.000001);
    }

    void rejectsDegenerateRouteFitInputs()
    {
        const CameraViewport viewport = {900.0, 580.0, 48.0, 48.0, 32.0, 32.0};
        CameraState result = BoundedCameraController::overview();
        QVERIFY(!BoundedCameraController::routeFit(QGeoCoordinate(21.02, 105.82),
                                                   QGeoCoordinate(21.02, 105.82), viewport, result,
                                                   &result));
        QCOMPARE(result.center.latitude(), BoundedCameraController::overview().center.latitude());
        QCOMPARE(result.center.longitude(), BoundedCameraController::overview().center.longitude());
    }

    void rejectsRouteFitOutsideThePackageOrWithNegativePadding()
    {
        CameraState result = BoundedCameraController::overview();
        const CameraViewport viewport = {900.0, 580.0, 48.0, 48.0, 32.0, 32.0};
        QVERIFY(!BoundedCameraController::routeFit(QGeoCoordinate(20.96, 105.80),
                                                   QGeoCoordinate(21.02, 105.85), viewport, result,
                                                   &result));

        CameraViewport invalidPadding = viewport;
        invalidPadding.leftPadding = -1.0;
        QVERIFY(!BoundedCameraController::routeFit(QGeoCoordinate(21.00, 105.80),
                                                   QGeoCoordinate(21.02, 105.85), invalidPadding,
                                                   result, &result));
        QCOMPARE(result.center.latitude(), BoundedCameraController::overview().center.latitude());
        QCOMPARE(result.center.longitude(), BoundedCameraController::overview().center.longitude());
    }
};

QTEST_GUILESS_MAIN(CameraProbeControllerTest)

#include "tst_CameraProbeController.moc"
