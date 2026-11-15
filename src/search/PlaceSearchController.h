#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>

namespace qtmaps {
namespace search {

class PlaceSearchController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString query READ query NOTIFY stateChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY stateChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString message READ message NOTIFY stateChanged)

public:
    explicit PlaceSearchController(QObject *parent = nullptr);

    QString query() const;
    QVariantList results() const;
    QString state() const;
    QString message() const;

    Q_INVOKABLE void setQuery(const QString &query);

signals:
    void stateChanged();

private:
    void applyQuery();

    QString query_;
    QVariantList results_;
    QString state_ = QStringLiteral("idle");
    QString message_ = QStringLiteral("Tìm trong dữ liệu demo cục bộ");
    QTimer debounce_;
};

} // namespace search
} // namespace qtmaps
