#include "RouteNavigationController.h"

#include <algorithm>
#include <cmath>

namespace qtmaps {
namespace routing {

RouteNavigationController::RouteNavigationController(QObject *parent) : QObject(parent)
{
    timer_.setInterval(33);
    timer_.setTimerType(Qt::PreciseTimer);
    connect(&timer_, &QTimer::timeout, this, &RouteNavigationController::advance);
}

QGeoCoordinate RouteNavigationController::coordinate() const { return coordinate_; }
double RouteNavigationController::bearing() const { return bearing_; }
double RouteNavigationController::travelledMeters() const { return travelledMeters_; }
double RouteNavigationController::routeLengthMeters() const { return routeLengthMeters_; }
QString RouteNavigationController::sourceLabel() const { return sourceLabel_; }
int RouteNavigationController::speedMultiplier() const { return speedMultiplier_; }
bool RouteNavigationController::routeAvailable() const { return route_.isValid(); }
bool RouteNavigationController::active() const
{
    return state_ == State::Running || state_ == State::Paused || state_ == State::Arrived;
}
bool RouteNavigationController::running() const { return state_ == State::Running; }
bool RouteNavigationController::paused() const { return state_ == State::Paused; }
bool RouteNavigationController::arrived() const { return state_ == State::Arrived; }
QString RouteNavigationController::statusMessage() const { return statusMessage_; }

void RouteNavigationController::setRoute(const Route &route, const QString &sourceLabel)
{
    timer_.stop();
    route_ = route;
    sourceLabel_ = sourceLabel;
    cumulativeMeters_.clear();
    routeLengthMeters_ = 0.0;
    travelledMeters_ = 0.0;
    bearing_ = 0.0;

    if (!route_.isValid()) {
        route_ = Route();
        coordinate_ = QGeoCoordinate();
        setState(State::NoRoute, QStringLiteral("Chưa có tuyến để chỉ đường"));
        emit routeChanged();
        emit positionChanged();
        return;
    }

    cumulativeMeters_.reserve(route_.polyline.size());
    cumulativeMeters_.append(0.0);
    for (int index = 1; index < route_.polyline.size(); ++index) {
        const double segment = route_.polyline.at(index - 1)
                                   .distanceTo(route_.polyline.at(index));
        if (!std::isfinite(segment) || segment < 0.0) {
            clearRoute();
            setState(State::Error, QStringLiteral("Không thể phát lại hình học tuyến"));
            return;
        }
        routeLengthMeters_ += segment;
        cumulativeMeters_.append(routeLengthMeters_);
    }
    if (routeLengthMeters_ <= 0.0 || route_.durationSeconds <= 0.0) {
        clearRoute();
        setState(State::Error, QStringLiteral("Tuyến thiếu quãng đường hoặc thời gian phát lại"));
        return;
    }

    updatePosition(0.0);
    setState(State::Ready, QStringLiteral("Sẵn sàng chỉ đường • %1").arg(sourceLabel_));
    emit routeChanged();
}

void RouteNavigationController::clearRoute()
{
    timer_.stop();
    route_ = Route();
    sourceLabel_.clear();
    cumulativeMeters_.clear();
    coordinate_ = QGeoCoordinate();
    bearing_ = 0.0;
    routeLengthMeters_ = 0.0;
    travelledMeters_ = 0.0;
    setState(State::NoRoute, QStringLiteral("Chưa có tuyến để chỉ đường"));
    emit routeChanged();
    emit positionChanged();
}

QGeoCoordinate RouteNavigationController::coordinateAtDistance(double distanceMeters,
                                                                double *bearing) const
{
    if (!route_.isValid() || cumulativeMeters_.size() != route_.polyline.size()) {
        if (bearing) {
            *bearing = 0.0;
        }
        return QGeoCoordinate();
    }
    const double bounded = std::max(0.0, std::min(distanceMeters, routeLengthMeters_));
    int segment = 0;
    while (segment + 1 < cumulativeMeters_.size()
           && cumulativeMeters_.at(segment + 1) < bounded) {
        ++segment;
    }
    if (segment + 1 >= route_.polyline.size()) {
        if (bearing) {
            *bearing = route_.polyline.at(route_.polyline.size() - 2)
                           .azimuthTo(route_.polyline.constLast());
        }
        return route_.polyline.constLast();
    }
    const QGeoCoordinate from = route_.polyline.at(segment);
    const QGeoCoordinate to = route_.polyline.at(segment + 1);
    const double segmentStart = cumulativeMeters_.at(segment);
    const double segmentLength = cumulativeMeters_.at(segment + 1) - segmentStart;
    const double ratio = segmentLength > 0.0
        ? std::max(0.0, std::min(1.0, (bounded - segmentStart) / segmentLength))
        : 0.0;
    if (bearing) {
        *bearing = from.azimuthTo(to);
    }
    return QGeoCoordinate(from.latitude() + (to.latitude() - from.latitude()) * ratio,
                          from.longitude() + (to.longitude() - from.longitude()) * ratio);
}

void RouteNavigationController::start()
{
    if (!route_.isValid()) {
        setState(State::Error, QStringLiteral("Không thể bắt đầu: chưa có tuyến hợp lệ"));
        return;
    }
    if (state_ == State::Running) {
        return;
    }
    if (state_ == State::Arrived) {
        updatePosition(0.0);
    }
    tickElapsed_.restart();
    timer_.start();
    setState(State::Running,
             QStringLiteral("Đang chỉ đường %1x • %2").arg(speedMultiplier_).arg(sourceLabel_));
}

void RouteNavigationController::pause()
{
    if (state_ != State::Running) {
        return;
    }
    timer_.stop();
    setState(State::Paused, QStringLiteral("Đã tạm dừng chỉ đường"));
}

void RouteNavigationController::stop()
{
    timer_.stop();
    if (!route_.isValid()) {
        setState(State::NoRoute, QStringLiteral("Chưa có tuyến để chỉ đường"));
        return;
    }
    updatePosition(0.0);
    setState(State::Ready, QStringLiteral("Đã dừng chỉ đường"));
}

bool RouteNavigationController::setSpeedMultiplier(int multiplier)
{
    if (multiplier != 1 && multiplier != 2 && multiplier != 4) {
        setState(State::Error, QStringLiteral("Tốc độ phát lại chỉ hỗ trợ 1x, 2x hoặc 4x"));
        return false;
    }
    if (speedMultiplier_ == multiplier) {
        return true;
    }
    speedMultiplier_ = multiplier;
    emit speedMultiplierChanged();
    if (state_ == State::Running) {
        setState(State::Running,
                 QStringLiteral("Đang chỉ đường %1x • %2")
                     .arg(speedMultiplier_).arg(sourceLabel_));
    }
    return true;
}

void RouteNavigationController::advanceBySeconds(double seconds)
{
    if (state_ != State::Running || !std::isfinite(seconds) || seconds <= 0.0) {
        return;
    }
    const double replayMetersPerSecond = routeLengthMeters_ / route_.durationSeconds;
    const double nextDistance = travelledMeters_
        + replayMetersPerSecond * speedMultiplier_ * seconds;
    updatePosition(std::min(nextDistance, routeLengthMeters_));
    if (nextDistance >= routeLengthMeters_) {
        timer_.stop();
        setState(State::Arrived, QStringLiteral("Đã đến đích"));
        emit destinationReached();
    }
}

void RouteNavigationController::advance()
{
    const qint64 elapsedMilliseconds = tickElapsed_.restart();
    if (elapsedMilliseconds > 0) {
        advanceBySeconds(elapsedMilliseconds / 1000.0);
    }
}

void RouteNavigationController::setState(State state, const QString &message)
{
    if (state_ == state && statusMessage_ == message) {
        return;
    }
    state_ = state;
    statusMessage_ = message;
    emit stateChanged();
}

void RouteNavigationController::updatePosition(double distanceMeters)
{
    travelledMeters_ = std::max(0.0, std::min(distanceMeters, routeLengthMeters_));
    coordinate_ = coordinateAtDistance(travelledMeters_, &bearing_);
    emit positionChanged();
}

} // namespace routing
} // namespace qtmaps
