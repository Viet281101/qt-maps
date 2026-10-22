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
    Q_PROPERTY(qulonglong frameCount READ frameCount NOTIFY metricsChanged)
    Q_PROPERTY(int sampleCount READ sampleCount NOTIFY metricsChanged)

public:
    explicit FrameMetrics(QObject *parent = nullptr);

    void attachWindow(QQuickWindow *window);
    double currentFps() const;
    double averageFps() const;
    double onePercentLowFps() const;
    qulonglong frameCount() const;
    int sampleCount() const;

public slots:
    void reset();

signals:
    void metricsChanged();
    void sampleReady(double currentFps, double averageFps, double onePercentLowFps);

private slots:
    void recordFrame();
    void publishInterval();

private:
    bool isWindowMeasurable() const;

    QPointer<QQuickWindow> window_;
    QTimer publishTimer_;
    QElapsedTimer intervalTimer_;
    FrameStatistics statistics_;
    qulonglong intervalFrames_ = 0;
    qulonglong frameCount_ = 0;
    double currentFps_ = 0.0;
};

} // namespace perf
} // namespace qtmaps
