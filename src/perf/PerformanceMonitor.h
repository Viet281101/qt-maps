#pragma once

#include "FrameMetrics.h"

#include <QObject>
#include <QThread>
#include <QVariantMap>

class QQuickWindow;

namespace qtmaps {
namespace perf {

class ProcessSamplerWorker;

class PerformanceMonitor final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *frameMetrics READ frameMetrics CONSTANT)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(double totalRssMiB READ totalRssMiB NOTIFY processMetricsChanged)
    Q_PROPERTY(double totalPssMiB READ totalPssMiB NOTIFY processMetricsChanged)
    Q_PROPERTY(double totalCpuPercent READ totalCpuPercent NOTIFY processMetricsChanged)
    Q_PROPERTY(
        double totalCpuMachinePercent READ totalCpuMachinePercent NOTIFY processMetricsChanged)
    Q_PROPERTY(int childCount READ childCount NOTIFY processMetricsChanged)
    Q_PROPERTY(QString outputPath READ outputPath NOTIFY outputPathChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit PerformanceMonitor(QObject *parent = nullptr);
    ~PerformanceMonitor() override;

    QObject *frameMetrics();
    bool recording() const;
    double totalRssMiB() const;
    double totalPssMiB() const;
    double totalCpuPercent() const;
    double totalCpuMachinePercent() const;
    int childCount() const;
    QString outputPath() const;
    QString errorMessage() const;

    void attachWindow(QQuickWindow *window);
    void setMetadata(const QVariantMap &metadata);
    void setSamplingInterval(int milliseconds);
    void setPssSamplingInterval(int milliseconds);
    void setWarmupSeconds(int seconds);

    Q_INVOKABLE bool startBenchmark(const QString &requestedPath = QString());
    Q_INVOKABLE void stopBenchmark();
    // Semantic checkpoints must not report a stale PSS cache entry.
    Q_INVOKABLE void sampleNow(bool forcePss = true);
    Q_INVOKABLE void resetFrameSummary();
    Q_INVOKABLE void beginInteractionWindow(int durationMilliseconds = 450);

signals:
    void recordingChanged();
    void processMetricsChanged();
    void outputPathChanged();
    void errorMessageChanged();
    void requestStart(qint64 rootPid, const QString &path, int intervalMilliseconds,
                      int pssIntervalMilliseconds, int warmupSeconds, const QVariantMap &metadata);
    void requestStop();
    void requestSample();
    void requestForcedSample();
    void frameSampleChanged(double currentFps, double averageFps, double onePercentLowFps,
                            bool interactionActive, int interactionFrameCount,
                            double interactionAverageFps, double frameTimeP50Milliseconds,
                            double frameTimeP95Milliseconds, double frameTimeP99Milliseconds,
                            int framesOver100Milliseconds, int settlingFrameCount,
                            double settlingFrameAverageMilliseconds,
                            double settlingFrameMaximumMilliseconds);

private slots:
    void handleStarted(const QString &path);
    void handleStopped();
    void handleSample(quint64 rssBytes, quint64 pssBytes, double cpuPercent,
                      double cpuMachinePercent, int childCount);
    void handleError(const QString &message);

private:
    QString defaultOutputPath() const;

    FrameMetrics frameMetrics_;
    QThread workerThread_;
    ProcessSamplerWorker *worker_ = nullptr;
    QVariantMap metadata_;
    int samplingIntervalMilliseconds_ = 500;
    int pssSamplingIntervalMilliseconds_ = 2000;
    int warmupSeconds_ = 3;
    bool recording_ = false;
    double totalRssMiB_ = 0.0;
    double totalPssMiB_ = 0.0;
    double totalCpuPercent_ = 0.0;
    double totalCpuMachinePercent_ = 0.0;
    int childCount_ = 0;
    QString outputPath_;
    QString errorMessage_;
};

} // namespace perf
} // namespace qtmaps
