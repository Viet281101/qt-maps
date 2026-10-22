#include "PerformanceMonitor.h"

#include "BenchmarkCsvWriter.h"
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
               int warmupSeconds, const QVariantMap &metadata)
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
        rootPid_ = rootPid;
        intervalMilliseconds_ = std::max(100, intervalMilliseconds);
        warmupMilliseconds_ = std::max(0, warmupSeconds) * 1000;
        metadata_ = metadata;
        QString procError;
        previous_ = ProcessTreeSampler::readProc(&procError);
        if (!previous_.contains(rootPid_)) {
            writer_.close();
            emit error(procError.isEmpty()
                           ? QStringLiteral("QtMaps PID is not readable in /proc")
                           : procError);
            return;
        }
        elapsed_.start();
        previousSampleMilliseconds_ = 0;
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
        previous_.clear();
        if (wasOpen) {
            emit stopped();
        }
    }

    void updateFrameMetrics(double currentFps, double averageFps, double onePercentLowFps)
    {
        currentFps_ = currentFps;
        averageFps_ = averageFps;
        onePercentLowFps_ = onePercentLowFps;
    }

signals:
    void started(const QString &path);
    void stopped();
    void sampled(quint64 rssBytes, double cpuPercent, int childCount);
    void warmupComplete();
    void error(const QString &message);

private slots:
    void sample()
    {
        QString procError;
        const QHash<qint64, ProcessSnapshot> current = ProcessTreeSampler::readProc(&procError);
        const qint64 nowMilliseconds = elapsed_.elapsed();
        const double intervalSeconds = (nowMilliseconds - previousSampleMilliseconds_) / 1000.0;
        const ProcessTreeSample processSample = ProcessTreeSampler::aggregate(
            rootPid_, current, previous_, ::sysconf(_SC_CLK_TCK), intervalSeconds);
        previous_ = current;
        previousSampleMilliseconds_ = nowMilliseconds;

        if (!current.contains(rootPid_)) {
            emit error(QStringLiteral("QtMaps PID disappeared while recording"));
            stop();
            return;
        }
        emit sampled(processSample.rssBytes, processSample.cpuPercent, processSample.childCount);
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
        row.insert(QStringLiteral("total_rss_bytes"), processSample.rssBytes);
        row.insert(QStringLiteral("total_cpu_percent_one_core_100"),
                   QString::number(processSample.cpuPercent, 'f', 3));
        row.insert(QStringLiteral("error"), procError);
        QString writerError;
        if (!writer_.writeRow(row, &writerError)) {
            emit error(QStringLiteral("Benchmark CSV write failed: %1").arg(writerError));
            stop();
        }
    }

private:
    BenchmarkCsvWriter writer_;
    QTimer *timer_ = nullptr;
    QElapsedTimer elapsed_;
    QHash<qint64, ProcessSnapshot> previous_;
    QVariantMap metadata_;
    qint64 rootPid_ = 0;
    qint64 previousSampleMilliseconds_ = 0;
    int intervalMilliseconds_ = 1000;
    int warmupMilliseconds_ = 0;
    bool warmupSignaled_ = false;
    double currentFps_ = 0.0;
    double averageFps_ = 0.0;
    double onePercentLowFps_ = 0.0;
};

PerformanceMonitor::PerformanceMonitor(QObject *parent)
    : QObject(parent)
{
    connect(&frameMetrics_, &FrameMetrics::sampleReady,
            this, &PerformanceMonitor::frameSampleChanged);
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
double PerformanceMonitor::totalCpuPercent() const { return totalCpuPercent_; }
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
    metadata_.insert(QStringLiteral("warmup_seconds"), warmupSeconds_);
    frameMetrics_.reset();
    if (!worker_) {
        worker_ = new ProcessSamplerWorker;
        worker_->moveToThread(&workerThread_);
        connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
        connect(this, &PerformanceMonitor::requestStart, worker_, &ProcessSamplerWorker::start);
        connect(this, &PerformanceMonitor::requestStop, worker_, &ProcessSamplerWorker::stop);
        connect(this, &PerformanceMonitor::frameSampleChanged,
                worker_, &ProcessSamplerWorker::updateFrameMetrics);
        connect(worker_, &ProcessSamplerWorker::started,
                this, &PerformanceMonitor::handleStarted);
        connect(worker_, &ProcessSamplerWorker::stopped,
                this, &PerformanceMonitor::handleStopped);
        connect(worker_, &ProcessSamplerWorker::sampled,
                this, &PerformanceMonitor::handleSample);
        connect(worker_, &ProcessSamplerWorker::error,
                this, &PerformanceMonitor::handleError);
        connect(worker_, &ProcessSamplerWorker::warmupComplete,
                &frameMetrics_, &FrameMetrics::reset);
    }
    if (!workerThread_.isRunning()) {
        workerThread_.start();
    }
    recording_ = true;
    emit recordingChanged();
    emit requestStart(QCoreApplication::applicationPid(), outputPath_,
                      samplingIntervalMilliseconds_, warmupSeconds_, metadata_);
    return true;
}

void PerformanceMonitor::stopBenchmark() { emit requestStop(); }
void PerformanceMonitor::resetFrameSummary() { frameMetrics_.reset(); }

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

void PerformanceMonitor::handleSample(quint64 rssBytes, double cpuPercent, int childCount)
{
    totalRssMiB_ = rssBytes / 1024.0 / 1024.0;
    totalCpuPercent_ = cpuPercent;
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
