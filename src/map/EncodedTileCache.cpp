#include "EncodedTileCache.h"

#include <algorithm>

namespace qtmaps {
namespace map {

EncodedTileCache::EncodedTileCache(int maximumBytes) :
    entries_(std::max(0, maximumBytes))
{}

bool EncodedTileCache::find(const QString &key, QByteArray *value) const
{
    if (!value) {
        return false;
    }
    const QByteArray *cached = entries_.object(key);
    if (!cached) {
        return false;
    }
    *value = *cached;
    return true;
}

void EncodedTileCache::insert(const QString &key, const QByteArray &value)
{
    if (value.isEmpty() || value.size() > entries_.maxCost()) {
        return;
    }
    entries_.insert(key, new QByteArray(value), value.size());
}

void EncodedTileCache::clear() { entries_.clear(); }
int EncodedTileCache::maximumBytes() const { return entries_.maxCost(); }
int EncodedTileCache::currentBytes() const { return entries_.totalCost(); }

} // namespace map
} // namespace qtmaps
