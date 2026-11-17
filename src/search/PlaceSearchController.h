#pragma once

#include <QObject>
#include <QPointer>
#include <QThread>
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
    Q_PROPERTY(QString sourceLabel READ sourceLabel NOTIFY stateChanged)

public:
    explicit PlaceSearchController(
        const QString &packagePath = QStringLiteral("data/search-packages/current"),
        QObject *parent = nullptr);
    ~PlaceSearchController() override;

    // Performs the same local contract checks used before the worker opens a
    // search database.  The readiness surface uses this narrow entry point;
    // it never opens a network connection or issues a query.
    static bool validatePackage(const QString &packagePath, QString *errorMessage);

    QString query() const;
    QVariantList results() const;
    QString state() const;
    QString message() const;
    QString sourceLabel() const;

    Q_INVOKABLE void setQuery(const QString &query);

    // Called on the controller's thread by the private package worker.
    void handlePackageStatus(bool available, const QString &message);
    void handlePackageResults(quint64 requestId, const QVariantList &results, const QString &error);

signals:
    void stateChanged();

private:
    void applyQuery();
    void applyFixtureQuery();

    QString query_;
    QVariantList results_;
    QString state_ = QStringLiteral("idle");
    QString message_ = QStringLiteral("Tìm trong dữ liệu demo cục bộ");
    QString sourceLabel_ = QStringLiteral("Dữ liệu demo cục bộ");
    bool packageAvailable_ = false;
    quint64 requestId_ = 0;
    QTimer debounce_;
    QThread workerThread_;
    QObject *worker_ = nullptr;
};

} // namespace search
} // namespace qtmaps
