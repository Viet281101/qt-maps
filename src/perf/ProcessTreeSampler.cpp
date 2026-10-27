#include "ProcessTreeSampler.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

#include <algorithm>
#include <cmath>

#include <unistd.h>

namespace qtmaps {
namespace perf {
namespace {

bool readStatSnapshot(qint64 pid, ProcessSnapshot *snapshot, QString *failure = nullptr)
{
    QFile statFile(QStringLiteral("/proc/%1/stat").arg(pid));
    if (!statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (failure)
            *failure = QStringLiteral("stat open: %1").arg(statFile.errorString());
        return false;
    }
    const QByteArray stat = statFile.readAll().trimmed();
    const int closingParenthesis = stat.lastIndexOf(')');
    if (closingParenthesis < 0) {
        if (failure)
            *failure = QStringLiteral("stat command terminator missing");
        return false;
    }
    const int openingParenthesis = stat.indexOf('(');
    const QList<QByteArray> fields = stat.mid(closingParenthesis + 2).split(' ');
    if (openingParenthesis < 0 || fields.size() < 22) {
        if (failure)
            *failure = QStringLiteral("stat field count %1").arg(fields.size());
        return false;
    }
    bool parentOk = false;
    bool userOk = false;
    bool systemOk = false;
    bool startTimeOk = false;
    bool threadsOk = false;
    bool rssOk = false;
    const qint64 parentPid = fields.at(1).toLongLong(&parentOk);
    const quint64 userTicks = fields.at(11).toULongLong(&userOk);
    const quint64 systemTicks = fields.at(12).toULongLong(&systemOk);
    const int threadCount = fields.at(17).toInt(&threadsOk);
    const quint64 startTimeTicks = fields.at(19).toULongLong(&startTimeOk);
    const qint64 rssPages = fields.at(21).toLongLong(&rssOk);
    if (!parentOk || !userOk || !systemOk || !startTimeOk || !threadsOk || !rssOk ||
        threadCount < 0 || rssPages < 0) {
        if (failure)
            *failure = QStringLiteral("stat numeric conversion");
        return false;
    }

    snapshot->pid = pid;
    snapshot->parentPid = parentPid;
    snapshot->cpuTicks = userTicks + systemTicks;
    snapshot->rssBytes = static_cast<quint64>(rssPages) * ::sysconf(_SC_PAGESIZE);
    snapshot->startTimeTicks = startTimeTicks;
    snapshot->threadCount = threadCount;
    snapshot->commandName = QString::fromLocal8Bit(
        stat.mid(openingParenthesis + 1, closingParenthesis - openingParenthesis - 1));
    return true;
}

bool readTreeDetails(qint64 pid, ProcessSnapshot *snapshot, QString *failure = nullptr)
{
    QFile statusFile(QStringLiteral("/proc/%1/status").arg(pid));
    if (!statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (failure)
            *failure = QStringLiteral("status open: %1").arg(statusFile.errorString());
        return false;
    }
    quint64 rssBytes = 0;
    bool rssFound = false;
    int threadCount = 0;
    bool threadsFound = false;
    while (true) {
        const QByteArray line = statusFile.readLine();
        if (line.isNull()) {
            break;
        }
        if (line.startsWith("Threads:")) {
            bool threadsOk = false;
            threadCount = line.mid(8).trimmed().toInt(&threadsOk);
            threadsFound = threadsOk && threadCount >= 0;
        } else if (line.startsWith("VmRSS:")) {
            QByteArray value = line.mid(6).trimmed();
            const int separator = value.indexOf(' ');
            if (separator >= 0) {
                value.truncate(separator);
            }
            bool rssOk = false;
            rssBytes = value.toULongLong(&rssOk) * 1024ULL;
            rssFound = rssOk;
        }
    }
    if (!rssFound || !threadsFound) {
        if (failure)
            *failure = QStringLiteral("VmRSS or Threads missing or malformed");
        return false;
    }

    snapshot->rssBytes = rssBytes;
    snapshot->threadCount = threadCount;
    snapshot->executablePath = QFileInfo(QStringLiteral("/proc/%1/exe").arg(pid)).symLinkTarget();
    return true;
}

bool isValhallaProcess(const ProcessSnapshot &snapshot)
{
    return snapshot.commandName == QLatin1String("valhalla_service") ||
           QFileInfo(snapshot.executablePath).fileName() == QLatin1String("valhalla_service");
}

} // namespace

QHash<qint64, ProcessSnapshot> ProcessTreeSampler::readProc(QString *errorMessage)
{
    QHash<qint64, ProcessSnapshot> snapshots;
    const QDir proc(QStringLiteral("/proc"));
    const QFileInfoList entries = proc.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    int numericEntries = 0;
    QString firstFailure;
    for (const QFileInfo &entry : entries) {
        bool pidOk = false;
        const qint64 pid = entry.fileName().toLongLong(&pidOk);
        if (!pidOk || pid <= 0) {
            continue;
        }
        ++numericEntries;
        ProcessSnapshot snapshot;
        QString failure;
        if (readStatSnapshot(pid, &snapshot, &failure)) {
            snapshots.insert(pid, snapshot);
        } else if (firstFailure.isEmpty()) {
            firstFailure = QStringLiteral("pid %1: %2").arg(pid).arg(failure);
        }
    }
    if (snapshots.isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("No readable Linux /proc process entries were found "
                                       "(entries=%1 numeric=%2 first=%3)")
                            .arg(entries.size())
                            .arg(numericEntries)
                            .arg(firstFailure);
    }
    return snapshots;
}

QHash<qint64, ProcessSnapshot> ProcessTreeSampler::readProcForTree(qint64 rootPid,
                                                                   QString *errorMessage)
{
    QString scanError;
    QHash<qint64, ProcessSnapshot> snapshots = readProc(&scanError);
    const QList<qint64> treePids = descendantPids(rootPid, snapshots);
    QString firstDetailFailure;
    for (qint64 pid : treePids) {
        ProcessSnapshot snapshot = snapshots.value(pid);
        QString detailFailure;
        if (readTreeDetails(pid, &snapshot, &detailFailure)) {
            snapshots.insert(pid, snapshot);
        } else {
            snapshots.remove(pid);
            if (firstDetailFailure.isEmpty()) {
                firstDetailFailure = QStringLiteral("pid %1: %2").arg(pid).arg(detailFailure);
            }
        }
    }
    if (!snapshots.contains(rootPid) && errorMessage) {
        *errorMessage = firstDetailFailure.isEmpty() ? scanError : firstDetailFailure;
    }
    return snapshots;
}

bool ProcessTreeSampler::readPssRollup(qint64 pid, quint64 *pssBytes, QString *errorMessage)
{
    if (!pssBytes) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("PSS output pointer is null");
        }
        return false;
    }
    QFile rollup(QStringLiteral("/proc/%1/smaps_rollup").arg(pid));
    if (!rollup.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = rollup.errorString();
        }
        return false;
    }
    while (true) {
        const QByteArray line = rollup.readLine();
        if (line.isNull()) {
            break;
        }
        if (!line.startsWith("Pss:")) {
            continue;
        }
        const QList<QByteArray> fields = line.simplified().split(' ');
        bool ok = false;
        const quint64 value = fields.size() >= 2 ? fields.at(1).toULongLong(&ok) : 0;
        if (ok) {
            *pssBytes = value * 1024ULL;
            return true;
        }
        break;
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("Pss missing or malformed");
    }
    return false;
}

QList<qint64> ProcessTreeSampler::descendantPids(qint64 rootPid,
                                                 const QHash<qint64, ProcessSnapshot> &snapshots)
{
    QList<qint64> result;
    if (!snapshots.contains(rootPid)) {
        return result;
    }
    QSet<qint64> included;
    included.insert(rootPid);
    result.append(rootPid);
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto iterator = snapshots.constBegin(); iterator != snapshots.constEnd(); ++iterator) {
            if (!included.contains(iterator.key()) && included.contains(iterator->parentPid)) {
                included.insert(iterator.key());
                result.append(iterator.key());
                changed = true;
            }
        }
    }
    return result;
}

QString ProcessTreeSampler::classifyRole(qint64 pid, qint64 rootPid,
                                         const QHash<qint64, ProcessSnapshot> &snapshots)
{
    if (pid == rootPid) {
        return QStringLiteral("app");
    }
    QSet<qint64> visited;
    qint64 currentPid = pid;
    while (snapshots.contains(currentPid) && !visited.contains(currentPid)) {
        visited.insert(currentPid);
        const ProcessSnapshot snapshot = snapshots.value(currentPid);
        if (isValhallaProcess(snapshot)) {
            return QStringLiteral("valhalla");
        }
        if (currentPid == rootPid) {
            break;
        }
        currentPid = snapshot.parentPid;
    }
    return QStringLiteral("other");
}

ProcessTreeSample ProcessTreeSampler::aggregate(qint64 rootPid,
                                                const QHash<qint64, ProcessSnapshot> &current,
                                                const QHash<qint64, ProcessSnapshot> &previous,
                                                long clockTicksPerSecond, double elapsedSeconds,
                                                int logicalCpuCount)
{
    ProcessTreeSample sample;
    sample.logicalCpuCount = std::max(1, logicalCpuCount);
    const QList<qint64> pids = descendantPids(rootPid, current);
    sample.processCount = pids.size();
    sample.childCount = std::max(0, sample.processCount - 1);
    for (qint64 pid : pids) {
        const ProcessSnapshot snapshot = current.value(pid);
        sample.rssBytes += snapshot.rssBytes;
        sample.threadCount += snapshot.threadCount;
        if (snapshot.pssAvailable) {
            sample.pssBytes += snapshot.pssBytes;
            ++sample.pssProcessCount;
        }
        quint64 tickDelta = 0;
        if (previous.contains(pid)) {
            const ProcessSnapshot previousSnapshot = previous.value(pid);
            if (snapshot.startTimeTicks == previousSnapshot.startTimeTicks &&
                snapshot.cpuTicks >= previousSnapshot.cpuTicks) {
                tickDelta = snapshot.cpuTicks - previousSnapshot.cpuTicks;
            }
        }
        ProcessMetric metric;
        metric.snapshot = snapshot;
        metric.role = classifyRole(pid, rootPid, current);
        if (clockTicksPerSecond > 0 && elapsedSeconds > 0.0) {
            metric.cpuPercentOneCore = tickDelta * 100.0 / clockTicksPerSecond / elapsedSeconds;
            metric.cpuPercentAllCores = metric.cpuPercentOneCore / sample.logicalCpuCount;
        }
        sample.cpuPercent += metric.cpuPercentOneCore;
        sample.processes.append(metric);
    }
    sample.cpuPercentAllCores = sample.cpuPercent / sample.logicalCpuCount;
    std::sort(sample.processes.begin(), sample.processes.end(),
              [](const ProcessMetric &left, const ProcessMetric &right) {
                  return left.snapshot.pid < right.snapshot.pid;
              });
    return sample;
}

} // namespace perf
} // namespace qtmaps
