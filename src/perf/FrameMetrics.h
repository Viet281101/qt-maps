#pragma once

#include "FrameStatistics.h"

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QTimer>

class QQuickWindow;

namespace qtmaps {
namespace perf {

class FrameMetrics final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double currentFps READ currentFps NOTIFY metricsChanged)
    Q_PROPERTY(double averageFps READ averageFps NOTIFY metricsChanged)
    Q_PROPERTY(double onePercentLowFps READ onePercentLowFps NOTIFY metricsChanged)
    Q_PROPERTY(bool interactionActive READ interactionActive NOTIFY metricsChanged)
    Q_PROPERTY(double interactionAverageFps READ interactionAverageFps NOTIFY metricsChanged)
    Q_PROPERTY(double interactionFpsP5 READ interactionFpsP5 NOTIFY metricsChanged)
    Q_PROPERTY(
        double interactionOnePercentLowFps READ interactionOnePercentLowFps NOTIFY metricsChanged)
    Q_PROPERTY(
        double frameTimeAverageMilliseconds READ frameTimeAverageMilliseconds NOTIFY metricsChanged)
    Q_PROPERTY(double frameTimeP50Milliseconds READ frameTimeP50Milliseconds NOTIFY metricsChanged)
    Q_PROPERTY(double frameTimeP95Milliseconds READ frameTimeP95Milliseconds NOTIFY metricsChanged)
    Q_PROPERTY(double frameTimeP99Milliseconds READ frameTimeP99Milliseconds NOTIFY metricsChanged)
    Q_PROPERTY(int framesOver100Milliseconds READ framesOver100Milliseconds NOTIFY metricsChanged)
    Q_PROPERTY(int interactionFrameCount READ interactionFrameCount NOTIFY metricsChanged)
    Q_PROPERTY(int settlingFrameCount READ settlingFrameCount NOTIFY metricsChanged)
    Q_PROPERTY(double settlingFrameAverageMilliseconds READ settlingFrameAverageMilliseconds NOTIFY
                   metricsChanged)
    Q_PROPERTY(double settlingFrameMaximumMilliseconds READ settlingFrameMaximumMilliseconds NOTIFY
                   metricsChanged)
    Q_PROPERTY(qulonglong frameCount READ frameCount NOTIFY metricsChanged)
    Q_PROPERTY(int sampleCount READ sampleCount NOTIFY metricsChanged)

public:
    explicit FrameMetrics(QObject *parent = nullptr);

    void attachWindow(QQuickWindow *window);
    double currentFps() const;
    double averageFps() const;
    double onePercentLowFps() const;
    bool interactionActive() const;
    double interactionAverageFps() const;
    double interactionFpsP5() const;
    double interactionOnePercentLowFps() const;
    double frameTimeAverageMilliseconds() const;
    double frameTimeP50Milliseconds() const;
    double frameTimeP95Milliseconds() const;
    double frameTimeP99Milliseconds() const;
    int framesOver100Milliseconds() const;
    int interactionFrameCount() const;
    int settlingFrameCount() const;
    double settlingFrameAverageMilliseconds() const;
    double settlingFrameMaximumMilliseconds() const;
    qulonglong frameCount() const;
    int sampleCount() const;

public slots:
    void reset();
    void beginInteractionWindow(int durationMilliseconds);

signals:
    void metricsChanged();
    void sampleReady(double currentFps, double averageFps, double onePercentLowFps,
                     bool interactionActive, int interactionFrameCount,
                     double interactionAverageFps, double frameTimeP50Milliseconds,
                     double frameTimeP95Milliseconds, double frameTimeP99Milliseconds,
                     int framesOver100Milliseconds, int settlingFrameCount,
                     double settlingFrameAverageMilliseconds,
                     double settlingFrameMaximumMilliseconds);

private slots:
    void recordFrame();
    void publishInterval();
    void endInteractionWindow();

private:
    bool isWindowMeasurable() const;

    QPointer<QQuickWindow> window_;
    QTimer publishTimer_;
    QTimer interactionTimer_;
    QElapsedTimer intervalTimer_;
    QElapsedTimer frameTimer_;
    FrameStatistics statistics_;
    qulonglong intervalFrames_ = 0;
    qulonglong frameCount_ = 0;
    double currentFps_ = 0.0;
    qint64 previousInteractionFrameNanoseconds_ = -1;
    double pendingInteractionFrameTimeMilliseconds_ = -1.0;
    bool interactionActive_ = false;
};

} // namespace perf
} // namespace qtmaps
