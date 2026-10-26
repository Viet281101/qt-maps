#include "SimulatedNavigationController.h"

#include <algorithm>

namespace qtmaps {
namespace map {

SimulatedNavigationController::SimulatedNavigationController(QObject *parent) :
    QObject(parent),
    route_({QGeoCoordinate(21.02790, 105.85230),  // Hoan Kiem east
            QGeoCoordinate(21.03210, 105.85120),  // Hang Bai / Trang Tien area
            QGeoCoordinate(21.03670, 105.84620),  // old quarter west
            QGeoCoordinate(21.03980, 105.83570),  // Cua Nam / Ba Dinh direction
            QGeoCoordinate(21.03440, 105.82770),  // Giang Vo direction
            QGeoCoordinate(21.02590, 105.82540),  // Lang Ha direction
            QGeoCoordinate(21.01820, 105.83240)}) // Thai Ha direction; ~5.5 km total
{
    cumulativeMeters_.reserve(route_.size());
    cumulativeMeters_.append(0.0);
    for (int index = 1; index < route_.size(); ++index) {
        routeLengthMeters_ += route_.at(index - 1).distanceTo(route_.at(index));
        cumulativeMeters_.append(routeLengthMeters_);
    }
    coordinate_ = route_.constFirst();
    timer_.setInterval(16);
    timer_.setTimerType(Qt::PreciseTimer);
    connect(&timer_, &QTimer::timeout, this, &SimulatedNavigationController::advance);
}

QGeoCoordinate SimulatedNavigationController::coordinate() const { return coordinate_; }
double SimulatedNavigationController::bearing() const { return bearing_; }
double SimulatedNavigationController::routeLengthMeters() const { return routeLengthMeters_; }
double SimulatedNavigationController::travelledMeters() const { return travelledMeters_; }
double SimulatedNavigationController::speedKilometersPerHour() const
{
    return speedKilometersPerHour_;
}
bool SimulatedNavigationController::running() const { return running_; }

QGeoCoordinate SimulatedNavigationController::coordinateAtDistance(double distanceMeters,
                                                                   double *bearing) const
{
    const double boundedDistance = std::max(0.0, std::min(distanceMeters, routeLengthMeters_));
    int segment = 0;
    while (segment + 1 < cumulativeMeters_.size() &&
           cumulativeMeters_.at(segment + 1) < boundedDistance) {
        ++segment;
    }
    if (segment + 1 >= route_.size()) {
        if (bearing) {
            *bearing = route_.at(route_.size() - 2).azimuthTo(route_.constLast());
        }
        return route_.constLast();
    }
    const QGeoCoordinate from = route_.at(segment);
    const QGeoCoordinate to = route_.at(segment + 1);
    const double segmentStart = cumulativeMeters_.at(segment);
    const double segmentLength = cumulativeMeters_.at(segment + 1) - segmentStart;
    const double ratio =
        segmentLength > 0.0
            ? std::max(0.0, std::min(1.0, (boundedDistance - segmentStart) / segmentLength))
            : 0.0;
    if (bearing) {
        *bearing = from.azimuthTo(to);
    }
    return QGeoCoordinate(from.latitude() + (to.latitude() - from.latitude()) * ratio,
                          from.longitude() + (to.longitude() - from.longitude()) * ratio);
}

void SimulatedNavigationController::start(double speedKilometersPerHour)
{
    speedKilometersPerHour_ = std::max(0.1, speedKilometersPerHour);
    travelledMeters_ = 0.0;
    elapsed_.restart();
    running_ = true;
    timer_.start();
    updatePosition(0.0);
    emit runningChanged();
}

void SimulatedNavigationController::stop()
{
    if (!running_) {
        return;
    }
    timer_.stop();
    running_ = false;
    emit runningChanged();
}

void SimulatedNavigationController::reset()
{
    stop();
    travelledMeters_ = 0.0;
    updatePosition(0.0);
}

void SimulatedNavigationController::advance()
{
    const double metersPerSecond = speedKilometersPerHour_ / 3.6;
    const double distance = metersPerSecond * elapsed_.elapsed() / 1000.0;
    updatePosition(std::min(distance, routeLengthMeters_));
    if (distance >= routeLengthMeters_) {
        stop();
    }
}

void SimulatedNavigationController::updatePosition(double distanceMeters)
{
    travelledMeters_ = std::max(0.0, std::min(distanceMeters, routeLengthMeters_));
    coordinate_ = coordinateAtDistance(travelledMeters_, &bearing_);
    emit positionChanged();
}

} // namespace map
} // namespace qtmaps
