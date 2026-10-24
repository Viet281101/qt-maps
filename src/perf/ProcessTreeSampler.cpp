#include "ProcessTreeSampler.h"

#include <QDir>
#include <QFile>
#include <QSet>
#include <QTextStream>

#include <algorithm>

namespace qtmaps {
namespace perf {
namespace {

bool readSnapshot(qint64 pid, ProcessSnapshot *snapshot, QString *failure = nullptr)
{
    QFile statFile(QStringLiteral("/proc/%1/stat").arg(pid));
    if (!statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (failure) *failure = QStringLiteral("stat open: %1").arg(statFile.errorString());
        return false;
    }
    const QByteArray stat = statFile.readAll().trimmed();
    const int closingParenthesis = stat.lastIndexOf(')');
    if (closingParenthesis < 0) {
        if (failure) *failure = QStringLiteral("stat command terminator missing");
        return false;
    }
    const QList<QByteArray> fields = stat.mid(closingParenthesis + 2).split(' ');
    if (fields.size() < 22) {
        if (failure) *failure = QStringLiteral("stat field count %1").arg(fields.size());
        return false;
    }
    bool parentOk = false;
    bool userOk = false;
    bool systemOk = false;
    const qint64 parentPid = fields.at(1).toLongLong(&parentOk);
    const quint64 userTicks = fields.at(11).toULongLong(&userOk);
    const quint64 systemTicks = fields.at(12).toULongLong(&systemOk);
    if (!parentOk || !userOk || !systemOk) {
        if (failure) *failure = QStringLiteral("stat numeric conversion");
        return false;
    }

    QFile statusFile(QStringLiteral("/proc/%1/status").arg(pid));
    if (!statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (failure) *failure = QStringLiteral("status open: %1").arg(statusFile.errorString());
        return false;
    }
    quint64 rssBytes = 0;
    bool rssFound = false;
    while (true) {
        const QByteArray line = statusFile.readLine();
        if (line.isNull()) {
            break;
        }
        if (!line.startsWith("VmRSS:")) {
            continue;
        }
        QByteArray value = line.mid(6).trimmed();
        const int separator = value.indexOf(' ');
        if (separator >= 0) {
            value.truncate(separator);
        }
        bool rssOk = false;
        rssBytes = value.toULongLong(&rssOk) * 1024ULL;
        rssFound = rssOk;
        break;
    }
    if (!rssFound) {
        if (failure) *failure = QStringLiteral("VmRSS missing or malformed");
        return false;
    }

    snapshot->pid = pid;
    snapshot->parentPid = parentPid;
    snapshot->cpuTicks = userTicks + systemTicks;
    snapshot->rssBytes = rssBytes;
    return true;
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
        if (readSnapshot(pid, &snapshot, &failure)) {
            snapshots.insert(pid, snapshot);
        } else if (firstFailure.isEmpty()) {
            firstFailure = QStringLiteral("pid %1: %2").arg(pid).arg(failure);
        }
    }
    if (snapshots.isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("No readable Linux /proc process entries were found (entries=%1 numeric=%2 first=%3)")
                            .arg(entries.size())
                            .arg(numericEntries)
                            .arg(firstFailure);
    }
    return snapshots;
}

QList<qint64> ProcessTreeSampler::descendantPids(
    qint64 rootPid, const QHash<qint64, ProcessSnapshot> &snapshots)
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

ProcessTreeSample ProcessTreeSampler::aggregate(
    qint64 rootPid,
    const QHash<qint64, ProcessSnapshot> &current,
    const QHash<qint64, ProcessSnapshot> &previous,
    long clockTicksPerSecond,
    double elapsedSeconds)
{
    ProcessTreeSample sample;
    const QList<qint64> pids = descendantPids(rootPid, current);
    sample.processCount = pids.size();
    sample.childCount = std::max(0, sample.processCount - 1);
    quint64 tickDelta = 0;
    for (qint64 pid : pids) {
        const ProcessSnapshot snapshot = current.value(pid);
        sample.rssBytes += snapshot.rssBytes;
        if (previous.contains(pid) && snapshot.cpuTicks >= previous.value(pid).cpuTicks) {
            tickDelta += snapshot.cpuTicks - previous.value(pid).cpuTicks;
        }
    }
    if (clockTicksPerSecond > 0 && elapsedSeconds > 0.0) {
        sample.cpuPercent = tickDelta * 100.0 / clockTicksPerSecond / elapsedSeconds;
    }
    return sample;
}

} // namespace perf
} // namespace qtmaps
