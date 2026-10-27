#pragma once

#include "ProcessTreeSampler.h"

#include <QFile>
#include <QVariantMap>

namespace qtmaps {
namespace perf {

class ProcessCsvWriter
{
public:
    static QString schemaVersion();
    static QStringList columns();
    static QString sidecarPath(const QString &aggregatePath);

    bool open(const QString &path, QString *errorMessage);
    bool writeSample(const QVariantMap &metadata, const ProcessTreeSample &sample,
                     qint64 timestampMonotonicMilliseconds, qint64 pssSampleAgeMilliseconds,
                     double samplerDurationMilliseconds, const QString &pssError,
                     QString *errorMessage);
    void close();
    bool isOpen() const;

private:
    bool writeRow(const QVariantMap &values, QString *errorMessage);

    QFile file_;
};

} // namespace perf
} // namespace qtmaps
