#include "BenchmarkCsvWriter.h"

#include <QFileInfo>
#include <QTextStream>

namespace qtmaps {
namespace perf {

QString BenchmarkCsvWriter::schemaVersion() { return QStringLiteral("qtmaps-perf-v1"); }

QStringList BenchmarkCsvWriter::columns()
{
    return {QStringLiteral("schema_version"), QStringLiteral("timestamp_utc"),
            QStringLiteral("timestamp_local"), QStringLiteral("timezone_offset"),
            QStringLiteral("scenario"), QStringLiteral("build_id"), QStringLiteral("build_type"),
            QStringLiteral("qt_version"), QStringLiteral("renderer"), QStringLiteral("driver"),
            QStringLiteral("window_width_px"), QStringLiteral("window_height_px"),
            QStringLiteral("network_mode"), QStringLiteral("dataset_id"),
            QStringLiteral("dataset_coverage"), QStringLiteral("sampling_interval_ms"),
            QStringLiteral("warmup_seconds"), QStringLiteral("app_pid"),
            QStringLiteral("process_count"), QStringLiteral("child_count"),
            QStringLiteral("current_fps"), QStringLiteral("average_fps"),
            QStringLiteral("one_percent_low_fps"), QStringLiteral("total_rss_bytes"),
            QStringLiteral("total_cpu_percent_one_core_100"), QStringLiteral("error")};
}

QString BenchmarkCsvWriter::escape(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    if (escaped.contains(QLatin1Char(',')) || escaped.contains(QLatin1Char('"'))
        || escaped.contains(QLatin1Char('\n')) || escaped.contains(QLatin1Char('\r'))) {
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return escaped;
}

bool BenchmarkCsvWriter::open(const QString &path, QString *errorMessage)
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
    if (alreadyExists && file_.size() > 0) {
        file_.seek(0);
        const QByteArray firstLine = file_.readLine().trimmed();
        const QByteArray expected = columns().join(QLatin1Char(',')).toUtf8();
        if (firstLine != expected) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Existing CSV header does not match schema %1")
                                    .arg(schemaVersion());
            }
            close();
            return false;
        }
        file_.seek(file_.size());
        return true;
    }
    QTextStream stream(&file_);
    stream << columns().join(QLatin1Char(',')) << '\n';
    stream.flush();
    if (stream.status() != QTextStream::Ok) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to write CSV header");
        }
        close();
        return false;
    }
    return true;
}

bool BenchmarkCsvWriter::writeRow(const QVariantMap &values, QString *errorMessage)
{
    if (!file_.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("CSV file is not open");
        }
        return false;
    }
    QStringList fields;
    for (const QString &column : columns()) {
        fields.append(escape(values.value(column).toString()));
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

void BenchmarkCsvWriter::close()
{
    if (file_.isOpen()) {
        file_.flush();
        file_.close();
    }
}

bool BenchmarkCsvWriter::isOpen() const { return file_.isOpen(); }

} // namespace perf
} // namespace qtmaps
