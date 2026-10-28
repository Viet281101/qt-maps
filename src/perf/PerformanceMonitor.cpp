#include "PerformanceMonitor.h"

#include "BenchmarkCsvWriter.h"
#include "ProcessCsvWriter.h"
#include "ProcessTreeSampler.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>

#include <unistd.h>

#include <algorithm>
#include <cstdlib>

namespace qtmaps {
namespace perf {

class ProcessSamplerWorker final : public QObject
{
    Q_OBJECT

public slots:
    void start(qint64 rootPid, const QString &path, int intervalMilliseconds,
               int pssIntervalMilliseconds, int warmupSeconds, const QVariantMap &metadata)
    {
        stop();
        const QFileInfo outputInfo(path);
        if (!QDir().mkpath(outputInfo.absolutePath())) {
            emit error(QStringLiteral("Cannot create benchmark directory: %1")
                           .arg(outputInfo.absolutePath()));
            return;
        }
        QString writerError;
        if (!writer_.open(path, &writerError)) {
            emit error(QStringLiteral("Cannot open benchmark CSV: %1").arg(writerError));
            return;
        }
        processOutputPath_ = ProcessCsvWriter::sidecarPath(path);
        if (!processWriter_.open(processOutputPath_, &writerError)) {
            writer_.close();
            emit error(QStringLiteral("Cannot open process CSV: %1").arg(writerError));
            return;
        }
        rootPid_ = rootPid;
        intervalMilliseconds_ = std::max(100, intervalMilliseconds);
        pssIntervalMilliseconds_ = std::max(intervalMilliseconds_, pssIntervalMilliseconds);
        warmupMilliseconds_ = std::max(0, warmupSeconds) * 1000;
        metadata_ = metadata;
        QString procError;
        previous_ = ProcessTreeSampler::readProcForTree(rootPid_, &procError);
        if (!previous_.contains(rootPid_)) {
            writer_.close();
            processWriter_.close();
            emit error(procError.isEmpty() ? QStringLiteral("QtMaps PID is not readable in /proc")
                                           : procError);
            return;
        }
        elapsed_.start();
        previousSampleMilliseconds_ = 0;
        lastPssSampleMilliseconds_ = -pssIntervalMilliseconds_;
        warmupSignaled_ = warmupMilliseconds_ == 0;
        timer_ = new QTimer(this);
        timer_->setTimerType(Qt::PreciseTimer);
        timer_->setInterval(intervalMilliseconds_);
        connect(timer_, &QTimer::timeout, this, &ProcessSamplerWorker::sample);
        timer_->start();
        emit started(path);
        if (warmupSignaled_) {
            emit warmupComplete();
        }
    }

    void stop()
    {
        if (timer_) {
            timer_->stop();
            timer_->deleteLater();
            timer_ = nullptr;
        }
        const bool wasOpen = writer_.isOpen();
        writer_.close();
        processWriter_.close();
        previous_.clear();
        pssCache_.clear();
        if (wasOpen) {
            emit stopped();
        }
    }

    void updateFrameMetrics(double currentFps, double averageFps, double onePercentLowFps,
                            bool interactionActive, int interactionFrameCount,
                            double interactionAverageFps, double frameTimeP50Milliseconds,
                            double frameTimeP95Milliseconds, double frameTimeP99Milliseconds,
                            int framesOver100Milliseconds, int settlingFrameCount,
                            double settlingFrameAverageMilliseconds,
                            double settlingFrameMaximumMilliseconds)
    {
        currentFps_ = currentFps;
        averageFps_ = averageFps;
        onePercentLowFps_ = onePercentLowFps;
        interactionActive_ = interactionActive;
        interactionFrameCount_ = interactionFrameCount;
        interactionAverageFps_ = interactionAverageFps;
        frameTimeP50Milliseconds_ = frameTimeP50Milliseconds;
        frameTimeP95Milliseconds_ = frameTimeP95Milliseconds;
        frameTimeP99Milliseconds_ = frameTimeP99Milliseconds;
        framesOver100Milliseconds_ = framesOver100Milliseconds;
        settlingFrameCount_ = settlingFrameCount;
        settlingFrameAverageMilliseconds_ = settlingFrameAverageMilliseconds;
        settlingFrameMaximumMilliseconds_ = settlingFrameMaximumMilliseconds;
    }

signals:
    void started(const QString &path);
    void stopped();
    void sampled(quint64 rssBytes, quint64 pssBytes, double cpuPercent, double cpuMachinePercent,
                 int childCount);
    void warmupComplete();
    void error(const QString &message);

public slots:
    void sample() { takeSample(false); }

    void sampleForced() { takeSample(true); }

private:
    void takeSample(bool forcePss)
    {
        QElapsedTimer samplerTimer;
        samplerTimer.start();
        QString procError;
        QHash<qint64, ProcessSnapshot> current =
            ProcessTreeSampler::readProcForTree(rootPid_, &procError);
        const qint64 nowMilliseconds = elapsed_.elapsed();
        const double intervalSeconds = (nowMilliseconds - previousSampleMilliseconds_) / 1000.0;
        const QList<qint64> treePids = ProcessTreeSampler::descendantPids(rootPid_, current);
        QString pssError;
        if (forcePss || nowMilliseconds - lastPssSampleMilliseconds_ >= pssIntervalMilliseconds_) {
            int pssFailureCount = 0;
            QString firstPssFailure;
            for (qint64 pid : treePids) {
                quint64 pssBytes = 0;
                QString readError;
                if (ProcessTreeSampler::readPssRollup(pid, &pssBytes, &readError)) {
                    ProcessSnapshot snapshot = current.value(pid);
                    snapshot.pssBytes = pssBytes;
                    snapshot.pssAvailable = true;
                    current.insert(pid, snapshot);
                    pssCache_.insert(pid, snapshot);
                } else {
                    ++pssFailureCount;
                    if (firstPssFailure.isEmpty()) {
                        firstPssFailure = QStringLiteral("pid %1: %2").arg(pid).arg(readError);
                    }
                    pssCache_.remove(pid);
                }
            }
            lastPssSampleMilliseconds_ = nowMilliseconds;
            if (pssFailureCount > 0) {
                pssError = QStringLiteral("%1/%2 smaps_rollup reads failed; first=%3")
                               .arg(pssFailureCount)
                               .arg(treePids.size())
                               .arg(firstPssFailure);
            }
        } else {
            for (qint64 pid : treePids) {
                if (!pssCache_.contains(pid)) {
                    continue;
                }
                const ProcessSnapshot cached = pssCache_.value(pid);
                ProcessSnapshot snapshot = current.value(pid);
                if (snapshot.startTimeTicks == cached.startTimeTicks) {
                    snapshot.pssBytes = cached.pssBytes;
                    snapshot.pssAvailable = cached.pssAvailable;
                    current.insert(pid, snapshot);
                } else {
                    pssCache_.remove(pid);
                }
            }
        }
        const int logicalCpuCount = std::max(1, QThread::idealThreadCount());
        const ProcessTreeSample processSample = ProcessTreeSampler::aggregate(
            rootPid_, current, previous_, ::sysconf(_SC_CLK_TCK), intervalSeconds, logicalCpuCount);
        previous_ = current;
        previousSampleMilliseconds_ = nowMilliseconds;

        if (!current.contains(rootPid_)) {
            emit error(QStringLiteral("QtMaps PID disappeared while recording"));
            stop();
            return;
        }
        emit sampled(processSample.rssBytes, processSample.pssBytes, processSample.cpuPercent,
                     processSample.cpuPercentAllCores, processSample.childCount);
        if (nowMilliseconds < warmupMilliseconds_) {
            return;
        }
        if (!warmupSignaled_) {
            warmupSignaled_ = true;
            emit warmupComplete();
            return;
        }

        const QDateTime utc = QDateTime::currentDateTimeUtc();
        const QDateTime local = utc.toLocalTime();
        QVariantMap row = metadata_;
        row.insert(QStringLiteral("schema_version"), BenchmarkCsvWriter::schemaVersion());
        row.insert(QStringLiteral("timestamp_utc"), utc.toString(Qt::ISODateWithMs));
        row.insert(QStringLiteral("timestamp_local"), local.toString(Qt::ISODateWithMs));
        const int offset = local.offsetFromUtc();
        row.insert(QStringLiteral("timezone_offset"),
                   QStringLiteral("%1%2:%3")
                       .arg(offset < 0 ? QLatin1String("-") : QLatin1String("+"))
                       .arg(std::abs(offset) / 3600, 2, 10, QLatin1Char('0'))
                       .arg((std::abs(offset) % 3600) / 60, 2, 10, QLatin1Char('0')));
        row.insert(QStringLiteral("app_pid"), rootPid_);
        row.insert(QStringLiteral("process_count"), processSample.processCount);
        row.insert(QStringLiteral("child_count"), processSample.childCount);
        row.insert(QStringLiteral("current_fps"), QString::number(currentFps_, 'f', 3));
        row.insert(QStringLiteral("average_fps"), QString::number(averageFps_, 'f', 3));
        row.insert(QStringLiteral("one_percent_low_fps"),
                   QString::number(onePercentLowFps_, 'f', 3));
        row.insert(QStringLiteral("interaction_active"), interactionActive_ ? 1 : 0);
        row.insert(QStringLiteral("interaction_frame_count"), interactionFrameCount_);
        row.insert(QStringLiteral("interaction_average_fps"),
                   QString::number(interactionAverageFps_, 'f', 3));
        row.insert(QStringLiteral("interaction_frame_time_p50_ms"),
                   QString::number(frameTimeP50Milliseconds_, 'f', 3));
        row.insert(QStringLiteral("interaction_frame_time_p95_ms"),
                   QString::number(frameTimeP95Milliseconds_, 'f', 3));
        row.insert(QStringLiteral("interaction_frame_time_p99_ms"),
                   QString::number(frameTimeP99Milliseconds_, 'f', 3));
        row.insert(QStringLiteral("interaction_frames_over_100ms"), framesOver100Milliseconds_);
        row.insert(QStringLiteral("settling_frame_count"), settlingFrameCount_);
        row.insert(QStringLiteral("settling_frame_average_ms"),
                   QString::number(settlingFrameAverageMilliseconds_, 'f', 3));
        row.insert(QStringLiteral("settling_frame_maximum_ms"),
                   QString::number(settlingFrameMaximumMilliseconds_, 'f', 3));
        row.insert(QStringLiteral("total_rss_bytes"), processSample.rssBytes);
        row.insert(QStringLiteral("total_cpu_percent_one_core_100"),
                   QString::number(processSample.cpuPercent, 'f', 3));
        row.insert(QStringLiteral("error"), procError);
        QString writerError;
        if (!writer_.writeRow(row, &writerError)) {
            emit error(QStringLiteral("Benchmark CSV write failed: %1").arg(writerError));
            stop();
            return;
        }
        const qint64 pssSampleAgeMilliseconds =
            std::max<qint64>(0, nowMilliseconds - lastPssSampleMilliseconds_);
        const double samplerDurationMilliseconds = samplerTimer.nsecsElapsed() / 1000000.0;
        if (!processWriter_.writeSample(metadata_, processSample, nowMilliseconds,
                                        pssSampleAgeMilliseconds, samplerDurationMilliseconds,
                                        pssError, &writerError)) {
            emit error(QStringLiteral("Process CSV write failed: %1").arg(writerError));
            stop();
        }
    }

public:
private:
    BenchmarkCsvWriter writer_;
    ProcessCsvWriter processWriter_;
    QTimer *timer_ = nullptr;
    QElapsedTimer elapsed_;
    QHash<qint64, ProcessSnapshot> previous_;
    QHash<qint64, ProcessSnapshot> pssCache_;
    QVariantMap metadata_;
    qint64 rootPid_ = 0;
    qint64 previousSampleMilliseconds_ = 0;
    qint64 lastPssSampleMilliseconds_ = 0;
    int intervalMilliseconds_ = 500;
    int pssIntervalMilliseconds_ = 2000;
    int warmupMilliseconds_ = 0;
    bool warmupSignaled_ = false;
    QString processOutputPath_;
    double currentFps_ = 0.0;
    double averageFps_ = 0.0;
    double onePercentLowFps_ = 0.0;
    bool interactionActive_ = false;
    int interactionFrameCount_ = 0;
    double interactionAverageFps_ = 0.0;
    double frameTimeP50Milliseconds_ = 0.0;
    double frameTimeP95Milliseconds_ = 0.0;
    double frameTimeP99Milliseconds_ = 0.0;
    int framesOver100Milliseconds_ = 0;
    int settlingFrameCount_ = 0;
    double settlingFrameAverageMilliseconds_ = 0.0;
    double settlingFrameMaximumMilliseconds_ = 0.0;
};

PerformanceMonitor::PerformanceMonitor(QObject *parent) : QObject(parent)
{
    connect(&frameMetrics_, &FrameMetrics::sampleReady, this,
            &PerformanceMonitor::frameSampleChanged);
    workerThread_.setObjectName(QStringLiteral("performance-process-sampler"));
}

PerformanceMonitor::~PerformanceMonitor()
{
    if (workerThread_.isRunning()) {
        QMetaObject::invokeMethod(worker_, "stop", Qt::BlockingQueuedConnection);
        workerThread_.quit();
        workerThread_.wait(3000);
    }
}

QObject *PerformanceMonitor::frameMetrics() { return &frameMetrics_; }
bool PerformanceMonitor::recording() const { return recording_; }
double PerformanceMonitor::totalRssMiB() const { return totalRssMiB_; }
double PerformanceMonitor::totalPssMiB() const { return totalPssMiB_; }
double PerformanceMonitor::totalCpuPercent() const { return totalCpuPercent_; }
double PerformanceMonitor::totalCpuMachinePercent() const { return totalCpuMachinePercent_; }
int PerformanceMonitor::childCount() const { return childCount_; }
QString PerformanceMonitor::outputPath() const { return outputPath_; }
QString PerformanceMonitor::errorMessage() const { return errorMessage_; }

void PerformanceMonitor::attachWindow(QQuickWindow *window)
{
    frameMetrics_.attachWindow(window);
    if (window) {
        metadata_.insert(QStringLiteral("window_width_px"), window->width());
        metadata_.insert(QStringLiteral("window_height_px"), window->height());
    }
}

void PerformanceMonitor::setMetadata(const QVariantMap &metadata) { metadata_ = metadata; }
void PerformanceMonitor::setSamplingInterval(int milliseconds)
{
    samplingIntervalMilliseconds_ = std::max(100, milliseconds);
}
void PerformanceMonitor::setPssSamplingInterval(int milliseconds)
{
    pssSamplingIntervalMilliseconds_ = std::max(samplingIntervalMilliseconds_, milliseconds);
}
void PerformanceMonitor::setWarmupSeconds(int seconds) { warmupSeconds_ = std::max(0, seconds); }

bool PerformanceMonitor::startBenchmark(const QString &requestedPath)
{
    if (recording_) {
        return false;
    }
    errorMessage_.clear();
    emit errorMessageChanged();
    outputPath_ = requestedPath.isEmpty() ? defaultOutputPath() : requestedPath;
    emit outputPathChanged();
    metadata_.insert(QStringLiteral("sampling_interval_ms"), samplingIntervalMilliseconds_);
    metadata_.insert(QStringLiteral("fast_sampling_interval_ms"), samplingIntervalMilliseconds_);
    metadata_.insert(QStringLiteral("pss_sampling_interval_ms"), pssSamplingIntervalMilliseconds_);
    metadata_.insert(QStringLiteral("logical_cpu_count"), std::max(1, QThread::idealThreadCount()));
    metadata_.insert(QStringLiteral("warmup_seconds"), warmupSeconds_);
    frameMetrics_.reset();
    if (!worker_) {
        worker_ = new ProcessSamplerWorker;
        worker_->moveToThread(&workerThread_);
        connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
        connect(this, &PerformanceMonitor::requestStart, worker_, &ProcessSamplerWorker::start);
        connect(this, &PerformanceMonitor::requestStop, worker_, &ProcessSamplerWorker::stop);
        connect(this, &PerformanceMonitor::requestSample, worker_, &ProcessSamplerWorker::sample);
        connect(this, &PerformanceMonitor::requestForcedSample, worker_,
                &ProcessSamplerWorker::sampleForced);
        connect(this, &PerformanceMonitor::frameSampleChanged, worker_,
                &ProcessSamplerWorker::updateFrameMetrics);
        connect(worker_, &ProcessSamplerWorker::started, this, &PerformanceMonitor::handleStarted);
        connect(worker_, &ProcessSamplerWorker::stopped, this, &PerformanceMonitor::handleStopped);
        connect(worker_, &ProcessSamplerWorker::sampled, this, &PerformanceMonitor::handleSample);
        connect(worker_, &ProcessSamplerWorker::error, this, &PerformanceMonitor::handleError);
        connect(worker_, &ProcessSamplerWorker::warmupComplete, &frameMetrics_,
                &FrameMetrics::reset);
    }
    if (!workerThread_.isRunning()) {
        workerThread_.start();
    }
    recording_ = true;
    emit recordingChanged();
    emit requestStart(QCoreApplication::applicationPid(), outputPath_,
                      samplingIntervalMilliseconds_, pssSamplingIntervalMilliseconds_,
                      warmupSeconds_, metadata_);
    return true;
}

void PerformanceMonitor::stopBenchmark() { emit requestStop(); }
void PerformanceMonitor::sampleNow(bool forcePss)
{
    if (forcePss) {
        emit requestForcedSample();
    } else {
        emit requestSample();
    }
}
void PerformanceMonitor::resetFrameSummary() { frameMetrics_.reset(); }
void PerformanceMonitor::beginInteractionWindow(int durationMilliseconds)
{
    frameMetrics_.beginInteractionWindow(durationMilliseconds);
}

void PerformanceMonitor::handleStarted(const QString &path)
{
    outputPath_ = path;
    emit outputPathChanged();
}

void PerformanceMonitor::handleStopped()
{
    if (!recording_) {
        return;
    }
    recording_ = false;
    emit recordingChanged();
}

void PerformanceMonitor::handleSample(quint64 rssBytes, quint64 pssBytes, double cpuPercent,
                                      double cpuMachinePercent, int childCount)
{
    totalRssMiB_ = rssBytes / 1024.0 / 1024.0;
    totalPssMiB_ = pssBytes / 1024.0 / 1024.0;
    totalCpuPercent_ = cpuPercent;
    totalCpuMachinePercent_ = cpuMachinePercent;
    childCount_ = childCount;
    emit processMetricsChanged();
}

void PerformanceMonitor::handleError(const QString &message)
{
    errorMessage_ = message;
    recording_ = false;
    emit errorMessageChanged();
    emit recordingChanged();
}

QString PerformanceMonitor::defaultOutputPath() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return QDir(base).filePath(
        QStringLiteral("benchmarks/qtmaps-%1.csv")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"))));
}

} // namespace perf
} // namespace qtmaps

#include "PerformanceMonitor.moc"
