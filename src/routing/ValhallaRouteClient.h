#pragma once

#include "Route.h"

#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace qtmaps {
namespace routing {

class ValhallaRouteClient final : public QObject
{
    Q_OBJECT

public:
    explicit ValhallaRouteClient(QObject *parent = nullptr);

    bool setEndpoint(const QUrl &endpoint, QString *error = nullptr);
    QUrl endpoint() const;
    bool isRequestPending() const;
    void requestRoute(const QGeoCoordinate &origin, const QGeoCoordinate &destination,
                      int timeoutMs = 5000);
    void cancel();

signals:
    void routeReady(const qtmaps::routing::Route &route);
    void routeFailed(const QString &error);
    void requestPendingChanged();

private:
    void finishError(const QString &error);

    QNetworkAccessManager *network_ = nullptr;
    QNetworkReply *reply_ = nullptr;
    QTimer *timeout_ = nullptr;
    QUrl endpoint_;
    quint64 requestGeneration_ = 0;
};

} // namespace routing
} // namespace qtmaps
