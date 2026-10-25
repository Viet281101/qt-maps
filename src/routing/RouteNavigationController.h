#pragma once

#include "Route.h"

#include <QElapsedTimer>
#include <QGeoCoordinate>
#include <QObject>
#include <QTimer>
#include <QVector>

namespace qtmaps {
namespace routing {

class RouteNavigationController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate NOTIFY positionChanged)
    Q_PROPERTY(double bearing READ bearing NOTIFY positionChanged)
    Q_PROPERTY(double travelledMeters READ travelledMeters NOTIFY positionChanged)
    Q_PROPERTY(double routeLengthMeters READ routeLengthMeters NOTIFY routeChanged)
    Q_PROPERTY(QString sourceLabel READ sourceLabel NOTIFY routeChanged)
    Q_PROPERTY(int speedMultiplier READ speedMultiplier NOTIFY speedMultiplierChanged)
    Q_PROPERTY(bool routeAvailable READ routeAvailable NOTIFY routeChanged)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool running READ running NOTIFY stateChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY stateChanged)
    Q_PROPERTY(bool arrived READ arrived NOTIFY stateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY stateChanged)

public:
    explicit RouteNavigationController(QObject *parent = nullptr);

    QGeoCoordinate coordinate() const;
    double bearing() const;
    double travelledMeters() const;
    double routeLengthMeters() const;
    QString sourceLabel() const;
    int speedMultiplier() const;
    bool routeAvailable() const;
    bool active() const;
    bool running() const;
    bool paused() const;
    bool arrived() const;
    QString statusMessage() const;

    void setRoute(const Route &route, const QString &sourceLabel);
    void clearRoute();
    QGeoCoordinate coordinateAtDistance(double distanceMeters,
                                        double *bearing = nullptr) const;
    void advanceBySeconds(double seconds);

    Q_INVOKABLE void start();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE bool setSpeedMultiplier(int multiplier);

signals:
    void routeChanged();
    void positionChanged();
    void stateChanged();
    void speedMultiplierChanged();
    void destinationReached();

private slots:
    void advance();

private:
    enum class State {
        NoRoute,
        Ready,
        Running,
        Paused,
        Arrived,
        Error,
    };

    void setState(State state, const QString &message);
    void updatePosition(double distanceMeters);

    Route route_;
    QString sourceLabel_;
    QVector<double> cumulativeMeters_;
    QTimer timer_;
    QElapsedTimer tickElapsed_;
    QGeoCoordinate coordinate_;
    double bearing_ = 0.0;
    double routeLengthMeters_ = 0.0;
    double travelledMeters_ = 0.0;
    int speedMultiplier_ = 1;
    State state_ = State::NoRoute;
    QString statusMessage_ = QStringLiteral("Chưa có tuyến để chỉ đường");
};

} // namespace routing
} // namespace qtmaps
