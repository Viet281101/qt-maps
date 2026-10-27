#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QVector>

namespace qtmaps {
namespace perf {

struct ProcessSnapshot
{
    qint64 pid = 0;
    qint64 parentPid = 0;
    quint64 cpuTicks = 0;
    quint64 rssBytes = 0;
    quint64 startTimeTicks = 0;
    quint64 pssBytes = 0;
    int threadCount = 0;
    bool pssAvailable = false;
    QString commandName;
    QString executablePath;
};

struct ProcessMetric
{
    ProcessSnapshot snapshot;
    QString role;
    double cpuPercentOneCore = 0.0;
    double cpuPercentAllCores = 0.0;
};

struct ProcessTreeSample
{
    quint64 rssBytes = 0;
    quint64 pssBytes = 0;
    double cpuPercent = 0.0;
    double cpuPercentAllCores = 0.0;
    int processCount = 0;
    int childCount = 0;
    int pssProcessCount = 0;
    int threadCount = 0;
    int logicalCpuCount = 1;
    QVector<ProcessMetric> processes;
};

class ProcessTreeSampler
{
public:
    static QHash<qint64, ProcessSnapshot> readProc(QString *errorMessage = nullptr);
    static QHash<qint64, ProcessSnapshot> readProcForTree(qint64 rootPid,
                                                          QString *errorMessage = nullptr);
    static bool readPssRollup(qint64 pid, quint64 *pssBytes, QString *errorMessage = nullptr);
    static QList<qint64> descendantPids(qint64 rootPid,
                                        const QHash<qint64, ProcessSnapshot> &snapshots);
    static QString classifyRole(qint64 pid, qint64 rootPid,
                                const QHash<qint64, ProcessSnapshot> &snapshots);
    static ProcessTreeSample aggregate(qint64 rootPid,
                                       const QHash<qint64, ProcessSnapshot> &current,
                                       const QHash<qint64, ProcessSnapshot> &previous,
                                       long clockTicksPerSecond, double elapsedSeconds,
                                       int logicalCpuCount = 1);
};

} // namespace perf
} // namespace qtmaps
