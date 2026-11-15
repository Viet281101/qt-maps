#include "PlaceSearchController.h"

#include <QVariantMap>

namespace qtmaps {
namespace search {
namespace {

struct Place
{
    const char *name;
    const char *detail;
};

const Place kPlaces[] = {{"Hoàn Kiếm", "Hồ Hoàn Kiếm · dữ liệu demo"},
                         {"Thái Hà", "Phố Thái Hà · dữ liệu demo"},
                         {"Ba Đình", "Quận Ba Đình · dữ liệu demo"}};

} // namespace

PlaceSearchController::PlaceSearchController(QObject *parent) : QObject(parent)
{
    debounce_.setSingleShot(true);
    debounce_.setInterval(120);
    connect(&debounce_, &QTimer::timeout, this, &PlaceSearchController::applyQuery);
}

QString PlaceSearchController::query() const { return query_; }
QVariantList PlaceSearchController::results() const { return results_; }
QString PlaceSearchController::state() const { return state_; }
QString PlaceSearchController::message() const { return message_; }

void PlaceSearchController::setQuery(const QString &query)
{
    const QString normalized = query.simplified();
    if (query_ == normalized)
        return;
    query_ = normalized;
    results_.clear();
    if (query_.isEmpty()) {
        debounce_.stop();
        state_ = QStringLiteral("idle");
        message_ = QStringLiteral("Tìm trong dữ liệu demo cục bộ");
    } else {
        state_ = QStringLiteral("loading");
        message_ = QStringLiteral("Đang tìm trong dữ liệu demo cục bộ…");
        debounce_.start();
    }
    emit stateChanged();
}

void PlaceSearchController::applyQuery()
{
    for (const Place &place : kPlaces) {
        const QString name = QString::fromUtf8(place.name);
        if (!name.contains(query_, Qt::CaseInsensitive))
            continue;
        results_.append(QVariantMap{{QStringLiteral("name"), name},
                                    {QStringLiteral("detail"), QString::fromUtf8(place.detail)}});
    }
    state_ = results_.isEmpty() ? QStringLiteral("empty") : QStringLiteral("ready");
    message_ = results_.isEmpty() ? QStringLiteral("Không có địa điểm demo phù hợp")
                                  : QStringLiteral("Kết quả từ dữ liệu demo cục bộ");
    emit stateChanged();
}

} // namespace search
} // namespace qtmaps
