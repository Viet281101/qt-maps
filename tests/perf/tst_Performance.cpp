#include "perf/BenchmarkCsvWriter.h"
#include "perf/FrameStatistics.h"
#include "perf/PerformanceMonitor.h"
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
        statistics.addInteractionFrameTime(0.0); // Invalid frame interval is ignored.

        QCOMPARE(statistics.averageFps(), 0.0);
        QCOMPARE(statistics.interactionFrameCount(), 5);
        QCOMPARE(statistics.interactionAverageFps(), 25.0);
        QCOMPARE(statistics.interactionFrameTimePercentile(50.0), 30.0);
        QCOMPARE(statistics.interactionFrameTimePercentile(95.0), 100.0);
        QCOMPARE(statistics.interactionFrameTimePercentile(99.0), 100.0);

        statistics.reset();
        QCOMPARE(statistics.interactionFrameCount(), 0);
        QCOMPARE(statistics.interactionFrameTimePercentile(95.0), 0.0);
    }

    void interactionFrameStatisticsRemainBounded()
    {
        FrameStatistics statistics(2);
        statistics.addInteractionFrameTime(10.0);
        statistics.addInteractionFrameTime(20.0);
        statistics.addInteractionFrameTime(40.0);
        QCOMPARE(statistics.interactionFrameCount(), 2);
        QCOMPARE(statistics.interactionFrameTimePercentile(50.0), 20.0);
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

        const ProcessTreeSample sample = ProcessTreeSampler::aggregate(
            10, current, previous, 100, 1.0);
        QCOMPARE(sample.processCount, 3);
        QCOMPARE(sample.childCount, 2);
        QCOMPARE(sample.rssBytes, quint64(7000));
        QCOMPARE(sample.cpuPercent, 30.0);

        current.remove(11);
        current.remove(12);
        const ProcessTreeSample afterExit = ProcessTreeSampler::aggregate(
            10, current, previous, 100, 1.0);
        QCOMPARE(afterExit.processCount, 1);
        QCOMPARE(afterExit.childCount, 0);
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
        QVERIFY2(childFound,
                 qPrintable(QStringLiteral("root=%1 rootReadable=%2 child=%3 childReadable=%4 childPpid=%5 snapshots=%6")
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
        QCOMPARE(BenchmarkCsvWriter::schemaVersion(), QStringLiteral("qtmaps-perf-v2"));
        QVERIFY(BenchmarkCsvWriter::columns().contains(
            QStringLiteral("interaction_frame_time_p99_ms")));
        QCOMPARE(lines.size(), 4); // header, two rows, trailing empty entry
        QVERIFY(lines.at(1).contains("\"pan, \"\"zoom\"\"\""));
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
        monitor.stopBenchmark();
        QTRY_VERIFY_WITH_TIMEOUT(!monitor.recording(), 1000);
        QVERIFY(recordingSpy.count() >= 2);

        QVERIFY(monitor.startBenchmark(QStringLiteral("/proc/qtmaps-forbidden/run.csv")));
        QTRY_VERIFY_WITH_TIMEOUT(!monitor.errorMessage().isEmpty(), 1000);
        QVERIFY(!monitor.recording());
    }
};

QTEST_GUILESS_MAIN(PerformanceTest)

#include "tst_Performance.moc"
