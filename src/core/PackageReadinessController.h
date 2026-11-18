#pragma once

#include <QObject>
#include <QPointer>
#include <QVariantList>

class QThread;
class QTimer;

namespace qtmaps {
namespace core {

class PackageReadinessController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList items READ items NOTIFY changed)
    Q_PROPERTY(QString state READ state NOTIFY changed)

public:
    PackageReadinessController(const QString &vectorPath, const QString &rasterPath,
                               const QString &searchPath, const QString &orthographyPath,
                               QObject *parent = nullptr,
                               const QString &routingActivationRoot = QString());
    ~PackageReadinessController() override;

    QVariantList items() const;
    QString state() const;
    void setRoutingState(const QString &state);

signals:
    void changed();

private slots:
    void applyResult(const QVariantList &items);
    void markTimedOut();

private:
    QVariantList items_;
    QString state_ = QStringLiteral("loading");
    QPointer<QThread> thread_;
    QTimer *timeout_ = nullptr;
    bool timedOut_ = false;
    QString routingState_;
};

} // namespace core
} // namespace qtmaps
