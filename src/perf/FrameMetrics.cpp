#include "FrameMetrics.h"

#include <QQuickWindow>

namespace qtmaps {
namespace perf {

FrameMetrics::FrameMetrics(QObject *parent) : QObject(parent)
{
    publishTimer_.setInterval(1000);
    publishTimer_.setTimerType(Qt::PreciseTimer);
    connect(&publishTimer_, &QTimer::timeout, this, &FrameMetrics::publishInterval);
    interactionTimer_.setSingleShot(true);
    interactionTimer_.setTimerType(Qt::PreciseTimer);
    connect(&interactionTimer_, &QTimer::timeout, this, &FrameMetrics::endInteractionWindow);
    frameTimer_.start();
}

void FrameMetrics::attachWindow(QQuickWindow *window)
{
    if (window_ == window) {
        return;
    }
    if (window_) {
        disconnect(window_, nullptr, this, nullptr);
    }
    window_ = window;
    reset();
    if (!window_) {
        publishTimer_.stop();
        return;
    }
    connect(window_, &QQuickWindow::frameSwapped, this, &FrameMetrics::recordFrame);
    intervalTimer_.start();
    publishTimer_.start();
}

double FrameMetrics::currentFps() const { return currentFps_; }
double FrameMetrics::averageFps() const { return statistics_.averageFps(); }
double FrameMetrics::onePercentLowFps() const { return statistics_.onePercentLowFps(); }
bool FrameMetrics::interactionActive() const { return interactionActive_; }
double FrameMetrics::interactionAverageFps() const { return statistics_.interactionAverageFps(); }
double FrameMetrics::frameTimeP50Milliseconds() const
{
    return statistics_.interactionFrameTimePercentile(50.0);
}
double FrameMetrics::frameTimeP95Milliseconds() const
{
    return statistics_.interactionFrameTimePercentile(95.0);
}
double FrameMetrics::frameTimeP99Milliseconds() const
{
    return statistics_.interactionFrameTimePercentile(99.0);
}
int FrameMetrics::framesOver100Milliseconds() const
{
    return statistics_.interactionFrameTimeOverThresholdCount(100.0);
}
int FrameMetrics::interactionFrameCount() const { return statistics_.interactionFrameCount(); }
int FrameMetrics::settlingFrameCount() const { return statistics_.settlingFrameCount(); }
double FrameMetrics::settlingFrameAverageMilliseconds() const
{
    return statistics_.settlingFrameAverageMilliseconds();
}
double FrameMetrics::settlingFrameMaximumMilliseconds() const
{
    return statistics_.settlingFrameMaximumMilliseconds();
}
qulonglong FrameMetrics::frameCount() const { return frameCount_; }
int FrameMetrics::sampleCount() const { return statistics_.sampleCount(); }

void FrameMetrics::reset()
{
    intervalFrames_ = 0;
    frameCount_ = 0;
    currentFps_ = 0.0;
    statistics_.reset();
    previousInteractionFrameNanoseconds_ = -1;
    pendingInteractionFrameTimeMilliseconds_ = -1.0;
    intervalTimer_.restart();
    emit metricsChanged();
}

void FrameMetrics::beginInteractionWindow(int durationMilliseconds)
{
    if (durationMilliseconds <= 0) {
        return;
    }
    if (!interactionActive_) {
        interactionActive_ = true;
        previousInteractionFrameNanoseconds_ = -1;
    }
    interactionTimer_.start(durationMilliseconds);
    emit metricsChanged();
}

void FrameMetrics::recordFrame()
{
    ++intervalFrames_;
    ++frameCount_;
    const qint64 nowNanoseconds = frameTimer_.nsecsElapsed();
    if (interactionActive_) {
        if (previousInteractionFrameNanoseconds_ >= 0) {
            if (pendingInteractionFrameTimeMilliseconds_ > 0.0) {
                statistics_.addInteractionFrameTime(pendingInteractionFrameTimeMilliseconds_);
            }
            pendingInteractionFrameTimeMilliseconds_ =
                (nowNanoseconds - previousInteractionFrameNanoseconds_) / 1000000.0;
        }
        previousInteractionFrameNanoseconds_ = nowNanoseconds;
    } else {
        previousInteractionFrameNanoseconds_ = -1;
        pendingInteractionFrameTimeMilliseconds_ = -1.0;
    }
}

void FrameMetrics::publishInterval()
{
    const qint64 elapsedMilliseconds = intervalTimer_.restart();
    if (elapsedMilliseconds <= 0) {
        return;
    }
    if (!isWindowMeasurable()) {
        intervalFrames_ = 0;
        currentFps_ = 0.0;
        emit metricsChanged();
        emit sampleReady(currentFps_, averageFps(), onePercentLowFps(), interactionActive_,
                         interactionFrameCount(), interactionAverageFps(),
                         frameTimeP50Milliseconds(), frameTimeP95Milliseconds(),
                         frameTimeP99Milliseconds(), framesOver100Milliseconds(),
                         settlingFrameCount(), settlingFrameAverageMilliseconds(),
                         settlingFrameMaximumMilliseconds());
        return;
    }
    currentFps_ = intervalFrames_ * 1000.0 / elapsedMilliseconds;
    intervalFrames_ = 0;
    statistics_.addSample(currentFps_);
    emit metricsChanged();
    emit sampleReady(currentFps_, averageFps(), onePercentLowFps(), interactionActive_,
                     interactionFrameCount(), interactionAverageFps(), frameTimeP50Milliseconds(),
                     frameTimeP95Milliseconds(), frameTimeP99Milliseconds(),
                     framesOver100Milliseconds(), settlingFrameCount(),
                     settlingFrameAverageMilliseconds(), settlingFrameMaximumMilliseconds());
}

void FrameMetrics::endInteractionWindow()
{
    if (pendingInteractionFrameTimeMilliseconds_ > 0.0) {
        statistics_.addSettlingFrameTime(pendingInteractionFrameTimeMilliseconds_);
    }
    interactionActive_ = false;
    previousInteractionFrameNanoseconds_ = -1;
    pendingInteractionFrameTimeMilliseconds_ = -1.0;
    emit metricsChanged();
}

bool FrameMetrics::isWindowMeasurable() const
{
    if (!window_ || !window_->isVisible()) {
        return false;
    }
    return window_->visibility() != QWindow::Hidden && window_->visibility() != QWindow::Minimized;
}

} // namespace perf
} // namespace qtmaps
