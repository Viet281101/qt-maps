#include "map/CameraProbeController.h"

#include <QSignalSpy>
#include <QTest>

using qtmaps::map::CameraProbeController;

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
};

QTEST_GUILESS_MAIN(CameraProbeControllerTest)

#include "tst_CameraProbeController.moc"
