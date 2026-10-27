#include "ProcessCsvWriter.h"

#include "BenchmarkCsvWriter.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

namespace qtmaps {
namespace perf {

QString ProcessCsvWriter::schemaVersion() { return QStringLiteral("qtmaps-process-v1"); }

QStringList ProcessCsvWriter::columns()
{
    return {QStringLiteral("schema_version"),
            QStringLiteral("timestamp_utc"),
            QStringLiteral("elapsed_ms"),
            QStringLiteral("scenario"),
            QStringLiteral("run_id"),
            QStringLiteral("row_type"),
            QStringLiteral("role"),
            QStringLiteral("pid"),
            QStringLiteral("ppid"),
            QStringLiteral("start_time_ticks"),
            QStringLiteral("command"),
            QStringLiteral("executable"),
            QStringLiteral("threads"),
            QStringLiteral("rss_bytes"),
            QStringLiteral("pss_bytes"),
            QStringLiteral("pss_available"),
            QStringLiteral("pss_process_count"),
            QStringLiteral("pss_sample_age_ms"),
            QStringLiteral("cpu_percent_one_core_100"),
            QStringLiteral("cpu_percent_all_cores_100"),
            QStringLiteral("logical_cpu_count"),
            QStringLiteral("process_count"),
            QStringLiteral("child_count"),
            QStringLiteral("fast_sampling_interval_ms"),
            QStringLiteral("pss_sampling_interval_ms"),
            QStringLiteral("sampler_duration_ms"),
            QStringLiteral("pss_error")};
}

QString ProcessCsvWriter::sidecarPath(const QString &aggregatePath)
{
    const QFileInfo info(aggregatePath);
    const QString fileName = info.completeBaseName() + QStringLiteral("-processes.csv");
    return info.dir().filePath(fileName);
}

bool ProcessCsvWriter::open(const QString &path, QString *errorMessage)
{
    close();
    file_.setFileName(path);
    const bool alreadyExists = QFileInfo::exists(path);
    if (!file_.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append)) {
        if (errorMessage) {
            *errorMessage = file_.errorString();
        }
        return false;
    }
    const QByteArray expected = columns().join(QLatin1Char(',')).toUtf8();
    if (alreadyExists && file_.size() > 0) {
        file_.seek(0);
        if (file_.readLine().trimmed() != expected) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Existing process CSV header does not match %1")
                                    .arg(schemaVersion());
            }
            close();
            return false;
        }
        file_.seek(file_.size());
        return true;
    }
    QTextStream stream(&file_);
    stream << expected << '\n';
    stream.flush();
    if (stream.status() != QTextStream::Ok) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to write process CSV header");
        }
        close();
        return false;
    }
    return true;
}

bool ProcessCsvWriter::writeSample(const QVariantMap &metadata, const ProcessTreeSample &sample,
                                   qint64 timestampMonotonicMilliseconds,
                                   qint64 pssSampleAgeMilliseconds,
                                   double samplerDurationMilliseconds, const QString &pssError,
                                   QString *errorMessage)
{
    const QString timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    for (const ProcessMetric &metric : sample.processes) {
        QVariantMap row = metadata;
        row.insert(QStringLiteral("schema_version"), schemaVersion());
        row.insert(QStringLiteral("timestamp_utc"), timestamp);
        row.insert(QStringLiteral("elapsed_ms"), timestampMonotonicMilliseconds);
        row.insert(QStringLiteral("row_type"), QStringLiteral("process"));
        row.insert(QStringLiteral("role"), metric.role);
        row.insert(QStringLiteral("pid"), metric.snapshot.pid);
        row.insert(QStringLiteral("ppid"), metric.snapshot.parentPid);
        row.insert(QStringLiteral("start_time_ticks"), metric.snapshot.startTimeTicks);
        row.insert(QStringLiteral("command"), metric.snapshot.commandName);
        row.insert(QStringLiteral("executable"), metric.snapshot.executablePath);
        row.insert(QStringLiteral("threads"), metric.snapshot.threadCount);
        row.insert(QStringLiteral("rss_bytes"), metric.snapshot.rssBytes);
        row.insert(QStringLiteral("pss_bytes"), metric.snapshot.pssBytes);
        row.insert(QStringLiteral("pss_available"), metric.snapshot.pssAvailable ? 1 : 0);
        row.insert(QStringLiteral("pss_sample_age_ms"), pssSampleAgeMilliseconds);
        row.insert(QStringLiteral("cpu_percent_one_core_100"),
                   QString::number(metric.cpuPercentOneCore, 'f', 3));
        row.insert(QStringLiteral("cpu_percent_all_cores_100"),
                   QString::number(metric.cpuPercentAllCores, 'f', 3));
        row.insert(QStringLiteral("logical_cpu_count"), sample.logicalCpuCount);
        if (!writeRow(row, errorMessage)) {
            return false;
        }
    }

    QVariantMap total = metadata;
    total.insert(QStringLiteral("schema_version"), schemaVersion());
    total.insert(QStringLiteral("timestamp_utc"), timestamp);
    total.insert(QStringLiteral("elapsed_ms"), timestampMonotonicMilliseconds);
    total.insert(QStringLiteral("row_type"), QStringLiteral("tree_total"));
    total.insert(QStringLiteral("role"), QStringLiteral("total"));
    total.insert(QStringLiteral("threads"), sample.threadCount);
    total.insert(QStringLiteral("rss_bytes"), sample.rssBytes);
    total.insert(QStringLiteral("pss_bytes"), sample.pssBytes);
    total.insert(QStringLiteral("pss_available"),
                 sample.pssProcessCount == sample.processCount ? 1 : 0);
    total.insert(QStringLiteral("pss_process_count"), sample.pssProcessCount);
    total.insert(QStringLiteral("pss_sample_age_ms"), pssSampleAgeMilliseconds);
    total.insert(QStringLiteral("cpu_percent_one_core_100"),
                 QString::number(sample.cpuPercent, 'f', 3));
    total.insert(QStringLiteral("cpu_percent_all_cores_100"),
                 QString::number(sample.cpuPercentAllCores, 'f', 3));
    total.insert(QStringLiteral("logical_cpu_count"), sample.logicalCpuCount);
    total.insert(QStringLiteral("process_count"), sample.processCount);
    total.insert(QStringLiteral("child_count"), sample.childCount);
    total.insert(QStringLiteral("sampler_duration_ms"),
                 QString::number(samplerDurationMilliseconds, 'f', 3));
    total.insert(QStringLiteral("pss_error"), pssError);
    return writeRow(total, errorMessage);
}

bool ProcessCsvWriter::writeRow(const QVariantMap &values, QString *errorMessage)
{
    if (!file_.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Process CSV file is not open");
        }
        return false;
    }
    QStringList fields;
    for (const QString &column : columns()) {
        fields.append(BenchmarkCsvWriter::escape(values.value(column).toString()));
    }
    QTextStream stream(&file_);
    stream << fields.join(QLatin1Char(',')) << '\n';
    stream.flush();
    file_.flush();
    if (stream.status() != QTextStream::Ok || file_.error() != QFile::NoError) {
        if (errorMessage) {
            *errorMessage = file_.errorString();
        }
        return false;
    }
    return true;
}

void ProcessCsvWriter::close()
{
    if (file_.isOpen()) {
        file_.flush();
        file_.close();
    }
}

bool ProcessCsvWriter::isOpen() const { return file_.isOpen(); }

} // namespace perf
} // namespace qtmaps
