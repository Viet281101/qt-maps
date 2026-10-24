#include "ValhallaRouteClient.h"

#include "ValhallaRouteParser.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrlQuery>

namespace qtmaps {
namespace routing {

ValhallaRouteClient::ValhallaRouteClient(QObject *parent)
    : QObject(parent), network_(new QNetworkAccessManager(this)), timeout_(new QTimer(this))
{
    timeout_->setSingleShot(true);
    connect(timeout_, &QTimer::timeout, this, [this]() {
        if (!reply_) {
            return;
        }
        QNetworkReply *reply = reply_;
        reply_ = nullptr;
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
        emit requestPendingChanged();
        emit routeFailed(QStringLiteral("Valhalla route request timed out"));
    });
}

bool ValhallaRouteClient::setEndpoint(const QUrl &endpoint, QString *error)
{
    QHostAddress address;
    const bool literalAddress = address.setAddress(endpoint.host());
    if (endpoint.scheme() != QStringLiteral("http") || endpoint.port() <= 0
        || !literalAddress || !address.isLoopback()) {
        if (error) {
            *error = QStringLiteral("Valhalla endpoint must be an HTTP loopback address with a port");
        }
        return false;
    }
    endpoint_ = endpoint;
    endpoint_.setPath(QString());
    endpoint_.setQuery(QString());
    endpoint_.setFragment(QString());
    return true;
}

QUrl ValhallaRouteClient::endpoint() const { return endpoint_; }
bool ValhallaRouteClient::isRequestPending() const { return reply_ != nullptr; }

void ValhallaRouteClient::requestRoute(const QGeoCoordinate &origin,
                                       const QGeoCoordinate &destination, int timeoutMs)
{
    if (reply_) {
        finishError(QStringLiteral("A Valhalla route request is already pending"));
        return;
    }
    if (endpoint_.isEmpty()) {
        emit routeFailed(QStringLiteral("Valhalla endpoint is not configured"));
        return;
    }
    if (!origin.isValid() || !destination.isValid()) {
        emit routeFailed(QStringLiteral("Route endpoints must be valid coordinates"));
        return;
    }

    QJsonArray locations;
    for (const QGeoCoordinate &coordinate : {origin, destination}) {
        locations.append(QJsonObject{{QStringLiteral("lat"), coordinate.latitude()},
                                     {QStringLiteral("lon"), coordinate.longitude()}});
    }
    QJsonObject directions;
    directions.insert(QStringLiteral("units"), QStringLiteral("kilometers"));
    directions.insert(QStringLiteral("language"), QStringLiteral("vi-VN"));
    QJsonObject requestObject;
    requestObject.insert(QStringLiteral("locations"), locations);
    requestObject.insert(QStringLiteral("costing"), QStringLiteral("auto"));
    requestObject.insert(QStringLiteral("units"), QStringLiteral("kilometers"));
    requestObject.insert(QStringLiteral("language"), QStringLiteral("vi-VN"));
    requestObject.insert(QStringLiteral("directions_options"), directions);

    QUrl url = endpoint_.resolved(QUrl(QStringLiteral("/route")));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("json"),
                       QString::fromUtf8(QJsonDocument(requestObject).toJson(QJsonDocument::Compact)));
    url.setQuery(query);
    reply_ = network_->get(QNetworkRequest(url));
    emit requestPendingChanged();
    timeout_->start(qMax(100, timeoutMs));
    connect(reply_, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = reply_;
        if (!reply) {
            return;
        }
        reply_ = nullptr;
        timeout_->stop();
        emit requestPendingChanged();
        const QByteArray response = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            const QString error = QStringLiteral("Valhalla request failed: %1")
                                      .arg(reply->errorString());
            reply->deleteLater();
            emit routeFailed(error);
            return;
        }
        reply->deleteLater();
        const RouteParseResult parsed = ValhallaRouteParser::parse(response, RouteSource::Valhalla);
        if (!parsed.ok()) {
            emit routeFailed(parsed.error);
            return;
        }
        emit routeReady(parsed.route);
    });
}

void ValhallaRouteClient::cancel()
{
    if (!reply_) {
        return;
    }
    QNetworkReply *reply = reply_;
    reply_ = nullptr;
    timeout_->stop();
    disconnect(reply, nullptr, this, nullptr);
    reply->abort();
    reply->deleteLater();
    emit requestPendingChanged();
}

void ValhallaRouteClient::finishError(const QString &error)
{
    cancel();
    emit routeFailed(error);
}

} // namespace routing
} // namespace qtmaps
