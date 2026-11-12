#include "diagnostics/CameraProbeController.h"
#include "map/BoundedCameraController.h"

#include <QSignalSpy>
#include <QTest>

using qtmaps::diagnostics::CameraProbeController;
using qtmaps::map::BoundedCameraController;
using qtmaps::map::CameraState;

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

    void followsNavigationWithoutLeavingThePackage()
    {
        CameraState current = BoundedCameraController::overview();
        current.zoom = 13.0;
        const CameraState followed = BoundedCameraController::navigationFollow(
            QGeoCoordinate(20.0, 110.0), 405.0, current);
        QVERIFY(BoundedCameraController::isWithinBounds(followed.center));
        QCOMPARE(followed.zoom, BoundedCameraController::kNavigationZoom);
        QCOMPARE(followed.bearing, 45.0);
        QCOMPARE(followed.tilt, 45.0);
    }
};

QTEST_GUILESS_MAIN(CameraProbeControllerTest)

#include "tst_CameraProbeController.moc"
