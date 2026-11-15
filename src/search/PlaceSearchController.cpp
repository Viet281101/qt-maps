#include "PlaceSearchController.h"

#include <QVariantMap>

namespace qtmaps {
namespace search {
namespace {

struct Place
{
    const char *name;
    const char *detail;
    double latitude;
    double longitude;
};

const Place kPlaces[] = {{"Hoàn Kiếm", "Hồ Hoàn Kiếm · dữ liệu demo", 21.0285, 105.8542},
                         {"Thái Hà", "Phố Thái Hà · dữ liệu demo", 21.0116, 105.8197},
                         {"Ba Đình", "Quận Ba Đình · dữ liệu demo", 21.0369, 105.8340}};

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
    if (query_ == normalized && debounce_.isActive())
        return;
    query_ = normalized;
    results_.clear();
    if (query_.isEmpty()) {
        debounce_.stop();
        state_ = QStringLiteral("idle");
        message_ = QStringLiteral("Tìm trong 3 địa điểm demo cục bộ");
    } else {
        state_ = QStringLiteral("loading");
        message_ = QStringLiteral("Đang tìm trong dữ liệu demo cục bộ…");
        debounce_.start();
    }
    emit stateChanged();
}

QVariantMap PlaceSearchController::resultAt(int index) const
{
    return index >= 0 && index < results_.size() ? results_.at(index).toMap() : QVariantMap();
}

void PlaceSearchController::applyQuery()
{
    for (const Place &place : kPlaces) {
        const QString name = QString::fromUtf8(place.name);
        if (!name.contains(query_, Qt::CaseInsensitive))
            continue;
        results_.append(QVariantMap{{QStringLiteral("name"), name},
                                    {QStringLiteral("detail"), QString::fromUtf8(place.detail)},
                                    {QStringLiteral("latitude"), place.latitude},
                                    {QStringLiteral("longitude"), place.longitude}});
    }
    state_ = results_.isEmpty() ? QStringLiteral("empty") : QStringLiteral("ready");
    message_ = results_.isEmpty() ? QStringLiteral("Không có địa điểm demo phù hợp")
                                  : QStringLiteral("Kết quả từ dữ liệu demo cục bộ");
    emit stateChanged();
}

} // namespace search
} // namespace qtmaps
