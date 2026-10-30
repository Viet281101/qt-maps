#pragma once

#include <QByteArray>
#include <QCache>
#include <QString>

namespace qtmaps {
namespace map {

class EncodedTileCache final
{
public:
    explicit EncodedTileCache(int maximumBytes);

    bool find(const QString &key, QByteArray *value) const;
    void insert(const QString &key, const QByteArray &value);
    void clear();
    int maximumBytes() const;
    int currentBytes() const;

private:
    QCache<QString, QByteArray> entries_;
};

} // namespace map
} // namespace qtmaps
