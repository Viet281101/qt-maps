#include "perf/BenchmarkCsvWriter.h"
#include "perf/FrameMetrics.h"
#include "perf/FrameStatistics.h"
#include "perf/PerformanceMonitor.h"
#include "perf/ProcessCsvWriter.h"
#include "perf/ProcessTreeSampler.h"

#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace qtmaps::perf;

class PerformanceTest final : public QObject
{
    Q_OBJECT

private slots:
    void frameStatisticsIncludeZeroFrameIntervals()
    {
        FrameStatistics statistics(4);
        statistics.addSample(60.0);
        statistics.addSample(0.0);
        statistics.addSample(30.0);
        QCOMPARE(statistics.sampleCount(), 3);
        QCOMPARE(statistics.averageFps(), 30.0);
        QCOMPARE(statistics.onePercentLowFps(), 0.0);
        statistics.reset();
        QCOMPARE(statistics.sampleCount(), 0);
    }

    void frameStatisticsRemainBounded()
    {
        FrameStatistics statistics(2);
        statistics.addSample(10.0);
        statistics.addSample(20.0);
        statistics.addSample(30.0);
        QCOMPARE(statistics.sampleCount(), 2);
        QCOMPARE(statistics.averageFps(), 25.0);
    }

    void interactionFrameStatisticsExcludeIdleAndReportPercentiles()
    {
        FrameStatistics statistics(8);
        statistics.addSample(0.0); // Legacy interval sample remains available.
        statistics.addInteractionFrameTime(10.0);
        statistics.addInteractionFrameTime(20.0);
        statistics.addInteractionFrameTime(30.0);
        statistics.addInteractionFrameTime(40.0);
        statistics.addInteractionFrameTime(100.0);
        statistics.addSettlingFrameTime(350.0);
        statistics.addInteractionFrameTime(0.0); // Invalid frame interval is ignored.

        QCOMPARE(statistics.averageFps(), 0.0);
        QCOMPARE(statistics.interactionFrameCount(), 5);
        QCOMPARE(statistics.interactionAverageFps(), 25.0);
        QCOMPARE(statistics.interactionFpsPercentile(5.0), 10.0);
        QCOMPARE(statistics.interactionOnePercentLowFps(), 10.0);
        QCOMPARE(statistics.interactionFrameTimeAverageMilliseconds(), 40.0);
        QCOMPARE(statistics.interactionFrameTimePercentile(50.0), 30.0);
        QCOMPARE(statistics.interactionFrameTimePercentile(95.0), 100.0);
        QCOMPARE(statistics.interactionFrameTimePercentile(99.0), 100.0);
        QCOMPARE(statistics.interactionFrameTimeOverThresholdCount(99.0), 1);
        QCOMPARE(statistics.interactionFrameTimeOverThresholdCount(100.0), 0);
        QCOMPARE(statistics.settlingFrameCount(), 1);
        QCOMPARE(statistics.settlingFrameAverageMilliseconds(), 350.0);
        QCOMPARE(statistics.settlingFrameMaximumMilliseconds(), 350.0);

        statistics.reset();
        QCOMPARE(statistics.interactionFrameCount(), 0);
        QCOMPARE(statistics.settlingFrameCount(), 0);
        QCOMPARE(statistics.interactionFrameTimePercentile(95.0), 0.0);
    }

    void interactionFrameStatisticsRemainBounded()
    {
        FrameStatistics statistics(2, 2);
        statistics.addInteractionFrameTime(10.0);
        statistics.addInteractionFrameTime(20.0);
        statistics.addInteractionFrameTime(40.0);
        QCOMPARE(statistics.interactionFrameCount(), 2);
        QCOMPARE(statistics.interactionFrameTimePercentile(50.0), 20.0);
    }

    void interactionFrameStatisticsCoverFullTwoMinuteSixtyFpsWindow()
    {
        FrameStatistics statistics;
        for (int frame = 0; frame < 7200; ++frame) {
            statistics.addInteractionFrameTime(16.666);
        }
        QCOMPARE(statistics.interactionFrameCount(), 7200);
        QCOMPARE(statistics.interactionFrameTimePercentile(99.0), 16.666);
    }

    void finalInteractionIntervalBecomesSettlingSample()
    {
        FrameMetrics metrics;
        metrics.beginInteractionWindow(80);
        QVERIFY(QMetaObject::invokeMethod(&metrics, "recordFrame"));
        QTest::qWait(5);
        QVERIFY(QMetaObject::invokeMethod(&metrics, "recordFrame"));
        QTest::qWait(5);
        QVERIFY(QMetaObject::invokeMethod(&metrics, "recordFrame"));
        QCOMPARE(metrics.interactionFrameCount(), 1);
        QCOMPARE(metrics.settlingFrameCount(), 0);

        QTRY_COMPARE_WITH_TIMEOUT(metrics.settlingFrameCount(), 1, 200);
        QCOMPARE(metrics.interactionFrameCount(), 1);
        QVERIFY(metrics.settlingFrameMaximumMilliseconds() > 0.0);
    }

    void processAggregationIncludesDescendantsAndCpuDelta()
    {
        QHash<qint64, ProcessSnapshot> previous;
        previous.insert(10, {10, 1, 100, 1000});
        previous.insert(11, {11, 10, 50, 2000});
        QHash<qint64, ProcessSnapshot> current;
        current.insert(10, {10, 1, 120, 1500});
        current.insert(11, {11, 10, 60, 2500});
        current.insert(12, {12, 11, 5, 3000});
        current.insert(99, {99, 1, 900, 9000});

        const ProcessTreeSample sample =
            ProcessTreeSampler::aggregate(10, current, previous, 100, 1.0);
        QCOMPARE(sample.processCount, 3);
        QCOMPARE(sample.childCount, 2);
        QCOMPARE(sample.rssBytes, quint64(7000));
        QCOMPARE(sample.cpuPercent, 30.0);

        current.remove(11);
        current.remove(12);
        const ProcessTreeSample afterExit =
            ProcessTreeSampler::aggregate(10, current, previous, 100, 1.0);
        QCOMPARE(afterExit.processCount, 1);
        QCOMPARE(afterExit.childCount, 0);
    }

    void processAggregationReportsPssThreadsRolesAndDualCpu()
    {
        QHash<qint64, ProcessSnapshot> previous;
        previous.insert(10, {10, 1, 100, 1000, 1000, 800, 3, true, QStringLiteral("qtmaps"),
                             QStringLiteral("/tmp/qtmaps")});
        previous.insert(11,
                        {11, 10, 50, 2000, 2000, 1500, 2, true, QStringLiteral("valhalla_service"),
                         QStringLiteral("/tmp/valhalla_service")});
        QHash<qint64, ProcessSnapshot> current;
        current.insert(10, {10, 1, 120, 1200, 1000, 900, 4, true, QStringLiteral("qtmaps"),
                            QStringLiteral("/tmp/qtmaps")});
        current.insert(11,
                       {11, 10, 70, 2200, 2000, 1600, 3, true, QStringLiteral("valhalla_service"),
                        QStringLiteral("/tmp/valhalla_service")});
        current.insert(12, {12, 11, 5, 500, 3000, 0, 1, false, QStringLiteral("worker"),
                            QStringLiteral("/tmp/worker")});

        const ProcessTreeSample sample =
            ProcessTreeSampler::aggregate(10, current, previous, 100, 1.0, 4);
        QCOMPARE(sample.rssBytes, quint64(3900));
        QCOMPARE(sample.pssBytes, quint64(2500));
        QCOMPARE(sample.pssProcessCount, 2);
        QCOMPARE(sample.threadCount, 8);
        QCOMPARE(sample.cpuPercent, 40.0);
        QCOMPARE(sample.cpuPercentAllCores, 10.0);
        QCOMPARE(sample.logicalCpuCount, 4);
        QCOMPARE(sample.processes.size(), 3);
        QCOMPARE(sample.processes.at(0).role, QStringLiteral("app"));
        QCOMPARE(sample.processes.at(1).role, QStringLiteral("valhalla"));
        QCOMPARE(sample.processes.at(2).role, QStringLiteral("valhalla"));
    }

    void processAggregationRejectsCpuDeltaAfterPidReuse()
    {
        QHash<qint64, ProcessSnapshot> previous;
        previous.insert(10, {10, 1, 900, 1000, 100});
        QHash<qint64, ProcessSnapshot> current;
        current.insert(10, {10, 1, 950, 1000, 200});
        const ProcessTreeSample sample =
            ProcessTreeSampler::aggregate(10, current, previous, 100, 1.0, 4);
        QCOMPARE(sample.cpuPercent, 0.0);
        QCOMPARE(sample.cpuPercentAllCores, 0.0);
    }

    void liveProcessTreeFindsAndRemovesControlledChild()
    {
        QProcess child;
        child.start(QStringLiteral("/bin/sleep"), {QStringLiteral("5")});
        QVERIFY(child.waitForStarted());
        const qint64 childPid = child.processId();
        const qint64 rootPid = QCoreApplication::applicationPid();
        const auto during = ProcessTreeSampler::readProc();
        const QList<qint64> descendants = ProcessTreeSampler::descendantPids(rootPid, during);
        const bool childFound = descendants.contains(childPid);
        child.terminate();
        QVERIFY(child.waitForFinished());
        QVERIFY2(
            childFound,
            qPrintable(
                QStringLiteral(
                    "root=%1 rootReadable=%2 child=%3 childReadable=%4 childPpid=%5 snapshots=%6")
                    .arg(rootPid)
                    .arg(during.contains(rootPid))
                    .arg(childPid)
                    .arg(during.contains(childPid))
                    .arg(during.value(childPid).parentPid)
                    .arg(during.size())));
        QTRY_VERIFY_WITH_TIMEOUT(
            !ProcessTreeSampler::descendantPids(rootPid, ProcessTreeSampler::readProc())
                 .contains(childPid),
            1000);
    }

    void livePssRollupIsReadableForCurrentProcess()
    {
        quint64 pssBytes = 0;
        QString error;
        QVERIFY2(ProcessTreeSampler::readPssRollup(QCoreApplication::applicationPid(), &pssBytes,
                                                   &error),
                 qPrintable(error));
        QVERIFY(pssBytes > 0);
    }

    void csvEscapesAndWritesHeaderOnce()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("metrics.csv"));
        QVariantMap row;
        row.insert(QStringLiteral("schema_version"), BenchmarkCsvWriter::schemaVersion());
        row.insert(QStringLiteral("scenario"), QStringLiteral("pan, \"zoom\""));
        QString error;
        {
            BenchmarkCsvWriter writer;
            QVERIFY2(writer.open(path, &error), qPrintable(error));
            QVERIFY2(writer.writeRow(row, &error), qPrintable(error));
        }
        {
            BenchmarkCsvWriter writer;
            QVERIFY2(writer.open(path, &error), qPrintable(error));
            QVERIFY2(writer.writeRow(row, &error), qPrintable(error));
        }
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QList<QByteArray> lines = file.readAll().split('\n');
        QCOMPARE(lines.at(0), BenchmarkCsvWriter::columns().join(QLatin1Char(',')).toUtf8());
        QCOMPARE(BenchmarkCsvWriter::schemaVersion(), QStringLiteral("qtmaps-perf-v3"));
        QVERIFY(BenchmarkCsvWriter::columns().contains(
            QStringLiteral("interaction_frame_time_p99_ms")));
        QVERIFY(BenchmarkCsvWriter::columns().contains(
            QStringLiteral("interaction_frames_over_100ms")));
        QVERIFY(
            BenchmarkCsvWriter::columns().contains(QStringLiteral("settling_frame_maximum_ms")));
        QCOMPARE(lines.size(), 4); // header, two rows, trailing empty entry
        QVERIFY(lines.at(1).contains("\"pan, \"\"zoom\"\"\""));
    }

    void processCsvWritesPerProcessAndTreeRows()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString aggregatePath = directory.filePath(QStringLiteral("run.csv"));
        const QString path = ProcessCsvWriter::sidecarPath(aggregatePath);
        QCOMPARE(path, directory.filePath(QStringLiteral("run-processes.csv")));

        ProcessTreeSample sample;
        sample.rssBytes = 1000;
        sample.pssBytes = 700;
        sample.cpuPercent = 40.0;
        sample.cpuPercentAllCores = 10.0;
        sample.processCount = 1;
        sample.pssProcessCount = 1;
        sample.threadCount = 3;
        sample.logicalCpuCount = 4;
        ProcessMetric metric;
        metric.snapshot = {10,
                           1,
                           120,
                           1000,
                           1000,
                           700,
                           3,
                           true,
                           QStringLiteral("qtmaps"),
                           QStringLiteral("/tmp/qtmaps")};
        metric.role = QStringLiteral("app");
        metric.cpuPercentOneCore = 40.0;
        metric.cpuPercentAllCores = 10.0;
        sample.processes.append(metric);

        ProcessCsvWriter writer;
        QString error;
        QVERIFY2(writer.open(path, &error), qPrintable(error));
        QVariantMap metadata;
        metadata.insert(QStringLiteral("scenario"), QStringLiteral("idle"));
        metadata.insert(QStringLiteral("run_id"), QStringLiteral("idle-1"));
        metadata.insert(QStringLiteral("fast_sampling_interval_ms"), 500);
        metadata.insert(QStringLiteral("pss_sampling_interval_ms"), 2000);
        QVERIFY2(writer.writeSample(metadata, sample, 500, 0, 2.5, QString(), &error),
                 qPrintable(error));
        writer.close();

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QByteArray contents = file.readAll();
        QVERIFY(contents.startsWith(ProcessCsvWriter::columns().join(QLatin1Char(',')).toUtf8()));
        QCOMPARE(contents.count('\n'), 3);
        QVERIFY(contents.contains("qtmaps-process-v1"));
        QVERIFY(contents.contains("tree_total,total"));
    }

    void monitorStartsStopsAndRejectsUnwritablePath()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        PerformanceMonitor monitor;
        monitor.setWarmupSeconds(0);
        monitor.setSamplingInterval(100);
        QSignalSpy recordingSpy(&monitor, &PerformanceMonitor::recordingChanged);
        QVERIFY(monitor.startBenchmark(directory.filePath(QStringLiteral("run.csv"))));
        QTRY_VERIFY_WITH_TIMEOUT(monitor.recording() || !monitor.errorMessage().isEmpty(), 1000);
        QVERIFY2(monitor.recording(), qPrintable(monitor.errorMessage()));
        QTest::qWait(250);
        monitor.stopBenchmark();
        QTRY_VERIFY_WITH_TIMEOUT(!monitor.recording(), 1000);
        QVERIFY(recordingSpy.count() >= 2);
        QVERIFY(QFile::exists(directory.filePath(QStringLiteral("run-processes.csv"))));

        QVERIFY(monitor.startBenchmark(QStringLiteral("/proc/qtmaps-forbidden/run.csv")));
        QTRY_VERIFY_WITH_TIMEOUT(!monitor.errorMessage().isEmpty(), 1000);
        QVERIFY(!monitor.recording());
    }
};

QTEST_GUILESS_MAIN(PerformanceTest)

#include "tst_Performance.moc"
