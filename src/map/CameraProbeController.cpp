#include "map/CameraProbeController.h"

#include <algorithm>
#include <cmath>

namespace qtmaps {
namespace map {
namespace {

struct CameraStep
{
    qreal bearing;
    qreal tilt;
    const char *name;
};

const CameraStep kSteps[] = {
    {0.0, 0.0, "north-reset"},
    {45.0, 30.0, "northeast-30"},
    {315.0, 45.0, "northwest-45"},
    {0.0, 0.0, "north-reset"},
};

constexpr int kStepCount = sizeof(kSteps) / sizeof(kSteps[0]);

} // namespace

CameraProbeController::CameraProbeController(QObject *parent)
    : QObject(parent)
{
}

qreal CameraProbeController::requestedBearing() const
{
    return requestedBearing_;
}

qreal CameraProbeController::requestedTilt() const
{
    return requestedTilt_;
}

QString CameraProbeController::stepName() const
{
    return stepName_;
}

void CameraProbeController::advance(qreal minimumTilt, qreal maximumTilt)
{
    selectStep((stepIndex_ + 1) % kStepCount, minimumTilt, maximumTilt);
}

void CameraProbeController::reset()
{
    selectStep(0, 0.0, 90.0);
}

qreal CameraProbeController::normalizedBearing(qreal bearing)
{
    qreal normalized = std::fmod(bearing, 360.0);
    if (normalized < 0.0) {
        normalized += 360.0;
    }
    return normalized;
}

qreal CameraProbeController::boundedTilt(qreal tilt, qreal minimumTilt, qreal maximumTilt)
{
    if (minimumTilt > maximumTilt) {
        std::swap(minimumTilt, maximumTilt);
    }
    return std::max(minimumTilt, std::min(maximumTilt, tilt));
}

void CameraProbeController::selectStep(int index, qreal minimumTilt, qreal maximumTilt)
{
    stepIndex_ = index;
    const CameraStep &step = kSteps[stepIndex_];
    requestedBearing_ = normalizedBearing(step.bearing);
    requestedTilt_ = boundedTilt(step.tilt, minimumTilt, maximumTilt);
    stepName_ = QString::fromLatin1(step.name);
    emit cameraChanged();
}

} // namespace map
} // namespace qtmaps
