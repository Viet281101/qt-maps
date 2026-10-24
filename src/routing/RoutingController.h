#pragma once

#include "Route.h"

#include <QGeoCoordinate>
#include <QObject>
#include <QVariantList>

namespace qtmaps {
namespace routing {

class RoutingController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString originInput READ originInput NOTIFY endpointsChanged)
    Q_PROPERTY(QString destinationInput READ destinationInput NOTIFY endpointsChanged)
    Q_PROPERTY(QGeoCoordinate origin READ origin NOTIFY endpointsChanged)
    Q_PROPERTY(QGeoCoordinate destination READ destination NOTIFY endpointsChanged)
    Q_PROPERTY(QVariantList routePath READ routePath NOTIFY routeChanged)
    Q_PROPERTY(QVariantList maneuvers READ maneuvers NOTIFY routeChanged)
    Q_PROPERTY(double distanceMeters READ distanceMeters NOTIFY routeChanged)
    Q_PROPERTY(double durationSeconds READ durationSeconds NOTIFY routeChanged)
    Q_PROPERTY(QString sourceLabel READ sourceLabel NOTIFY routeChanged)
    Q_PROPERTY(QString connectionLabel READ connectionLabel NOTIFY stateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY stateChanged)
    Q_PROPERTY(bool routeAvailable READ routeAvailable NOTIFY routeChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(QString nextMapSelection READ nextMapSelection NOTIFY endpointsChanged)

public:
    explicit RoutingController(QObject *parent = nullptr);

    QString originInput() const;
    QString destinationInput() const;
    QGeoCoordinate origin() const;
    QGeoCoordinate destination() const;
    QVariantList routePath() const;
    QVariantList maneuvers() const;
    double distanceMeters() const;
    double durationSeconds() const;
    QString sourceLabel() const;
    QString connectionLabel() const;
    QString statusMessage() const;
    bool routeAvailable() const;
    bool loading() const;
    QString nextMapSelection() const;

    Q_INVOKABLE bool setOriginInput(const QString &input);
    Q_INVOKABLE bool setDestinationInput(const QString &input);
    Q_INVOKABLE void selectMapPoint(const QGeoCoordinate &coordinate);
    Q_INVOKABLE void calculateFixtureRoute();
    Q_INVOKABLE void focusManeuver(int index);

signals:
    void endpointsChanged();
    void routeChanged();
    void stateChanged();
    void focusCoordinateRequested(const QGeoCoordinate &coordinate, double zoomLevel);

private:
    static bool parseLocation(const QString &input, QGeoCoordinate *coordinate,
                              QString *canonicalInput);
    void clearRoute();
    void applyRoute(const Route &route, const QString &sourceLabel);
    void setStatus(const QString &status);

    QString originInput_;
    QString destinationInput_;
    QGeoCoordinate origin_;
    QGeoCoordinate destination_;
    Route route_;
    QVariantList routePath_;
    QVariantList maneuvers_;
    QString sourceLabel_ = QStringLiteral("fixture/mô phỏng");
    QString connectionLabel_ = QStringLiteral("offline — fixture");
    QString statusMessage_ = QStringLiteral("Chọn hai điểm rồi bấm Tính đường");
    bool loading_ = false;
    bool selectOriginNext_ = true;
};

} // namespace routing
} // namespace qtmaps

