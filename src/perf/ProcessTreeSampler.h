#pragma once

#include <QHash>
#include <QList>
#include <QString>

namespace qtmaps {
namespace perf {

struct ProcessSnapshot
{
    qint64 pid = 0;
    qint64 parentPid = 0;
    quint64 cpuTicks = 0;
    quint64 rssBytes = 0;
};

struct ProcessTreeSample
{
    quint64 rssBytes = 0;
    double cpuPercent = 0.0;
    int processCount = 0;
    int childCount = 0;
};

class ProcessTreeSampler
{
public:
    static QHash<qint64, ProcessSnapshot> readProc(QString *errorMessage = nullptr);
    static QList<qint64> descendantPids(qint64 rootPid,
                                        const QHash<qint64, ProcessSnapshot> &snapshots);
    static ProcessTreeSample aggregate(qint64 rootPid,
                                       const QHash<qint64, ProcessSnapshot> &current,
                                       const QHash<qint64, ProcessSnapshot> &previous,
                                       long clockTicksPerSecond, double elapsedSeconds);
};

} // namespace perf
} // namespace qtmaps
