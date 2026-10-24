#pragma once

#include <QElapsedTimer>
#include <QGeoCoordinate>
#include <QObject>
#include <QTimer>
#include <QVector>

namespace qtmaps {
namespace map {

class SimulatedNavigationController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate NOTIFY positionChanged)
    Q_PROPERTY(double bearing READ bearing NOTIFY positionChanged)
    Q_PROPERTY(double routeLengthMeters READ routeLengthMeters CONSTANT)
    Q_PROPERTY(double travelledMeters READ travelledMeters NOTIFY positionChanged)
    Q_PROPERTY(double speedKilometersPerHour READ speedKilometersPerHour NOTIFY runningChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit SimulatedNavigationController(QObject *parent = nullptr);

    QGeoCoordinate coordinate() const;
    double bearing() const;
    double routeLengthMeters() const;
    double travelledMeters() const;
    double speedKilometersPerHour() const;
    bool running() const;
    QGeoCoordinate coordinateAtDistance(double distanceMeters, double *bearing = nullptr) const;

    Q_INVOKABLE void start(double speedKilometersPerHour = 40.0);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void reset();

signals:
    void positionChanged();
    void runningChanged();

private slots:
    void advance();

private:
    void updatePosition(double distanceMeters);

    QVector<QGeoCoordinate> route_;
    QVector<double> cumulativeMeters_;
    QTimer timer_;
    QElapsedTimer elapsed_;
    QGeoCoordinate coordinate_;
    double bearing_ = 0.0;
    double routeLengthMeters_ = 0.0;
    double travelledMeters_ = 0.0;
    double speedKilometersPerHour_ = 40.0;
    bool running_ = false;
};

} // namespace map
} // namespace qtmaps
