#include "FrameMetrics.h"

#include <QQuickWindow>

namespace qtmaps {
namespace perf {

FrameMetrics::FrameMetrics(QObject *parent)
    : QObject(parent)
{
    publishTimer_.setInterval(1000);
    publishTimer_.setTimerType(Qt::PreciseTimer);
    connect(&publishTimer_, &QTimer::timeout, this, &FrameMetrics::publishInterval);
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
qulonglong FrameMetrics::frameCount() const { return frameCount_; }
int FrameMetrics::sampleCount() const { return statistics_.sampleCount(); }

void FrameMetrics::reset()
{
    intervalFrames_ = 0;
    frameCount_ = 0;
    currentFps_ = 0.0;
    statistics_.reset();
    intervalTimer_.restart();
    emit metricsChanged();
}

void FrameMetrics::recordFrame()
{
    ++intervalFrames_;
    ++frameCount_;
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
        emit sampleReady(currentFps_, averageFps(), onePercentLowFps());
        return;
    }
    currentFps_ = intervalFrames_ * 1000.0 / elapsedMilliseconds;
    intervalFrames_ = 0;
    statistics_.addSample(currentFps_);
    emit metricsChanged();
    emit sampleReady(currentFps_, averageFps(), onePercentLowFps());
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
