#pragma once

#include <QFile>
#include <QVariantMap>

namespace qtmaps {
namespace perf {

class BenchmarkCsvWriter
{
public:
    static QString schemaVersion();
    static QStringList columns();
    static QString escape(const QString &value);

    bool open(const QString &path, QString *errorMessage);
    bool writeRow(const QVariantMap &values, QString *errorMessage);
    void close();
    bool isOpen() const;

private:
    QFile file_;
};

} // namespace perf
} // namespace qtmaps
