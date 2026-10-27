#include "BenchmarkScenarioController.h"

#include "perf/FrameMetrics.h"
#include "perf/PerformanceMonitor.h"
#include "routing/RouteNavigationController.h"
#include "routing/RoutingController.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickWindow>
#include <QVariantMap>

#include <algorithm>

namespace qtmaps {
namespace benchmark {
namespace {

const char kEventPrefix[] = "QTMAPS_BENCHMARK_EVENT ";

bool isKnownScenario(const QString &scenario)
{
    return scenario == QLatin1String("startup") || scenario == QLatin1String("idle") ||
           scenario == QLatin1String("pan_zoom") || scenario == QLatin1String("route") ||
           scenario == QLatin1String("navigation_replay");
}

} // namespace

BenchmarkScenarioController::BenchmarkScenarioController(
    perf::PerformanceMonitor *performanceMonitor, routing::RoutingController *routingController,
    routing::RouteNavigationController *navigationController, qint64 launchReferenceMilliseconds,
    QObject *parent) :
    QObject(parent), performanceMonitor_(performanceMonitor), routingController_(routingController),
    navigationController_(navigationController),
    launchReferenceMilliseconds_(launchReferenceMilliseconds)
{
    warmupTimer_.setSingleShot(true);
    completionTimer_.setSingleShot(true);
    deviationTimer_.setSingleShot(true);
    cameraTimer_.setInterval(250);
    cameraTimer_.setTimerType(Qt::PreciseTimer);

    connect(&warmupTimer_, &QTimer::timeout, this, &BenchmarkScenarioController::startMeasurement);
    connect(&completionTimer_, &QTimer::timeout, this,
            [this]() { finish(!routeFailureSeen_, QStringLiteral("duration complete")); });
    connect(&cameraTimer_, &QTimer::timeout, this, &BenchmarkScenarioController::advanceCamera);
    connect(&deviationTimer_, &QTimer::timeout, this, [this]() {
        if (!navigationController_->active()) {
            routeFailureSeen_ = true;
            emitEvent(QStringLiteral("reroute_skipped"),
                      {{QStringLiteral("reason"), QStringLiteral("navigation inactive")}});
            return;
        }
        reroutePending_ = true;
        rerouteTimer_.restart();
        emitEvent(QStringLiteral("reroute_started"));
        navigationController_->simulateDeviation();
    });
    connect(performanceMonitor_, &perf::PerformanceMonitor::processMetricsChanged, this,
            &BenchmarkScenarioController::handleProcessMetrics);
    connect(routingController_, &routing::RoutingController::backendReady, this,
            &BenchmarkScenarioController::handleBackendReady);
    connect(routingController_, &routing::RoutingController::routeChanged, this,
            &BenchmarkScenarioController::handleRoutingChanged);
    connect(routingController_, &routing::RoutingController::stateChanged, this,
            &BenchmarkScenarioController::handleRoutingChanged);
    connect(routingController_, &routing::RoutingController::rerouteSucceeded, this,
            &BenchmarkScenarioController::handleRerouteSucceeded);
}

bool BenchmarkScenarioController::configure(const QString &scenario, int durationSeconds,
                                            int warmupSeconds, bool fixtureOnly,
                                            QString *errorMessage)
{
    if (scenario.isEmpty()) {
        return true;
    }
    if (!isKnownScenario(scenario)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unknown benchmark scenario: %1").arg(scenario);
        }
        return false;
    }
    scenario_ = scenario;
    durationSeconds_ = std::max(1, durationSeconds);
    warmupSeconds_ = std::max(0, warmupSeconds);
    fixtureOnly_ = fixtureOnly;
    routePairs_ = {
        {QStringLiteral("Hoàn Kiếm"), QStringLiteral("Thái Hà")},
        {QStringLiteral("Hoàn Kiếm"), QStringLiteral("Ba Đình")},
        {QStringLiteral("21.036900, 105.834000"), QStringLiteral("21.011600, 105.819700")}};
    if (fixtureOnly_) {
        routePairs_.removeLast();
        backendReady_ = true;
    }
    return true;
}

void BenchmarkScenarioController::attachWindow(QQuickWindow *window)
{
    if (window_ == window) {
        return;
    }
    if (window_) {
        disconnect(window_, nullptr, this, nullptr);
    }
    window_ = window;
    if (window_) {
        connect(window_, &QQuickWindow::frameSwapped, this,
                &BenchmarkScenarioController::handleFrameSwapped);
        if (mapReady_) {
            window_->update();
        }
    }
}

bool BenchmarkScenarioController::enabled() const { return !scenario_.isEmpty(); }
QString BenchmarkScenarioController::scenario() const { return scenario_; }

void BenchmarkScenarioController::notifyMapReady()
{
    if (!enabled() || mapReady_) {
        return;
    }
    mapReady_ = true;
    emitEvent(QStringLiteral("map_ready"),
              {{QStringLiteral("elapsed_ms"), elapsedFromLaunchMilliseconds()}});
    if (window_) {
        window_->update();
    }
}

void BenchmarkScenarioController::handleFrameSwapped()
{
    if (!enabled() || !mapReady_ || firstMapFrameSeen_) {
        return;
    }
    firstMapFrameSeen_ = true;
    firstMapFrameMilliseconds_ = elapsedFromLaunchMilliseconds();
    emitEvent(QStringLiteral("first_rendered_map_frame"),
              {{QStringLiteral("startup_ms"), firstMapFrameMilliseconds_}});
    if (scenario_ == QLatin1String("startup")) {
        startupSamplePending_ = true;
        performanceMonitor_->sampleNow();
        return;
    }
    maybeScheduleWarmup();
}

void BenchmarkScenarioController::handleProcessMetrics()
{
    if (!startupSamplePending_ || finished_) {
        return;
    }
    startupSamplePending_ = false;
    emitEvent(QStringLiteral("startup_memory"),
              {{QStringLiteral("startup_ms"), firstMapFrameMilliseconds_},
               {QStringLiteral("pss_mib"), performanceMonitor_->totalPssMiB()},
               {QStringLiteral("rss_mib"), performanceMonitor_->totalRssMiB()}});
    finish(true, QStringLiteral("first rendered map frame sampled"));
}

void BenchmarkScenarioController::handleBackendReady()
{
    backendReady_ = true;
    emitEvent(QStringLiteral("backend_ready"),
              {{QStringLiteral("elapsed_ms"), elapsedFromLaunchMilliseconds()}});
    maybeScheduleWarmup();
}

void BenchmarkScenarioController::handleRoutingChanged()
{
    if (finished_) {
        return;
    }
    if (routeRequestActive_) {
        QTimer::singleShot(0, this, &BenchmarkScenarioController::evaluateRouteRequest);
    }
    if (scenario_ == QLatin1String("navigation_replay") && measurementStarted_ &&
        !navigationStarted_ && !navigationController_->active() &&
        routingController_->routeAvailable() && !routingController_->loading()) {
        startNavigationMeasurement();
    }
    if (reroutePending_) {
        QTimer::singleShot(0, this, [this]() {
            if (!reroutePending_ || routingController_->loading()) {
                return;
            }
            reroutePending_ = false;
            routeFailureSeen_ = true;
            emitEvent(QStringLiteral("reroute_finished"),
                      {{QStringLiteral("success"), false},
                       {QStringLiteral("latency_ms"), rerouteTimer_.elapsed()},
                       {QStringLiteral("status"), routingController_->statusMessage()}});
        });
    }
}

void BenchmarkScenarioController::handleRerouteSucceeded()
{
    if (!reroutePending_) {
        return;
    }
    reroutePending_ = false;
    rerouteSucceeded_ = true;
    emitEvent(QStringLiteral("reroute_finished"),
              {{QStringLiteral("success"), true},
               {QStringLiteral("latency_ms"), rerouteTimer_.elapsed()},
               {QStringLiteral("distance_m"), routingController_->distanceMeters()}});
}

void BenchmarkScenarioController::advanceCamera()
{
    static const QGeoCoordinate path[] = {
        QGeoCoordinate(21.0285, 105.8542), QGeoCoordinate(21.0227, 105.8468),
        QGeoCoordinate(21.0177, 105.8335), QGeoCoordinate(21.0116, 105.8197),
        QGeoCoordinate(21.0208, 105.8258), QGeoCoordinate(21.0369, 105.8340)};
    const int pathCount = static_cast<int>(sizeof(path) / sizeof(path[0]));
    const int index = cameraStep_ % pathCount;
    const int cycle = (cameraStep_ / pathCount) % 2;
    const double zoom = cycle == 0 ? 14.0 + index * 0.35 : 16.0 - index * 0.3;
    const double bearing = (cameraStep_ * 23) % 360;
    emit cameraCommand(path[index], zoom, bearing);
    ++cameraStep_;
}

bool BenchmarkScenarioController::requiresBackend() const
{
    return !fixtureOnly_ && scenario_ != QLatin1String("startup");
}

void BenchmarkScenarioController::maybeScheduleWarmup()
{
    if (!enabled() || finished_ || warmupScheduled_ || !firstMapFrameSeen_ ||
        (requiresBackend() && !backendReady_)) {
        return;
    }
    warmupScheduled_ = true;
    emitEvent(QStringLiteral("warmup_started"),
              {{QStringLiteral("warmup_seconds"), warmupSeconds_}});
    warmupTimer_.start(warmupSeconds_ * 1000);
}

void BenchmarkScenarioController::startMeasurement()
{
    if (measurementStarted_ || finished_) {
        return;
    }
    measurementStarted_ = true;
    performanceMonitor_->resetFrameSummary();
    emitEvent(QStringLiteral("measurement_started"),
              {{QStringLiteral("duration_seconds"), durationSeconds_}});
    if (scenario_ == QLatin1String("idle")) {
        completionTimer_.start(durationSeconds_ * 1000);
    } else if (scenario_ == QLatin1String("pan_zoom")) {
        performanceMonitor_->beginInteractionWindow(durationSeconds_ * 1000);
        cameraStep_ = 0;
        advanceCamera();
        cameraTimer_.start();
        completionTimer_.start(durationSeconds_ * 1000);
    } else if (scenario_ == QLatin1String("route")) {
        startRouteMeasurement();
    } else if (scenario_ == QLatin1String("navigation_replay")) {
        if (routingController_->routeAvailable()) {
            startNavigationMeasurement();
        } else if (fixtureOnly_) {
            routingController_->calculateFixtureRoute();
        } else {
            routingController_->calculateRoute();
        }
    }
}

void BenchmarkScenarioController::startRouteMeasurement()
{
    routeIndex_ = 0;
    routeFailureSeen_ = false;
    requestNextRoute();
}

void BenchmarkScenarioController::requestNextRoute()
{
    if (routeIndex_ >= routePairs_.size()) {
        finish(!routeFailureSeen_, QStringLiteral("route pairs complete"));
        return;
    }
    const QPair<QString, QString> pair = routePairs_.at(routeIndex_);
    const bool endpointsValid = routingController_->setOriginInput(pair.first) &&
                                routingController_->setDestinationInput(pair.second);
    if (!endpointsValid) {
        routeFailureSeen_ = true;
        emitEvent(QStringLiteral("route_finished"),
                  {{QStringLiteral("index"), routeIndex_},
                   {QStringLiteral("success"), false},
                   {QStringLiteral("status"), routingController_->statusMessage()}});
        ++routeIndex_;
        QTimer::singleShot(0, this, &BenchmarkScenarioController::requestNextRoute);
        return;
    }
    routeRequestActive_ = true;
    routeRequestTimer_.restart();
    emitEvent(QStringLiteral("route_started"), {{QStringLiteral("index"), routeIndex_},
                                                {QStringLiteral("origin"), pair.first},
                                                {QStringLiteral("destination"), pair.second}});
    if (fixtureOnly_) {
        routingController_->calculateFixtureRoute();
    } else {
        routingController_->calculateRoute();
    }
}

void BenchmarkScenarioController::evaluateRouteRequest()
{
    if (!routeRequestActive_ || routingController_->loading()) {
        return;
    }
    routeRequestActive_ = false;
    const bool success =
        routingController_->routeAvailable() &&
        (fixtureOnly_ || routingController_->sourceLabel() == QLatin1String("Valhalla local"));
    routeFailureSeen_ = routeFailureSeen_ || !success;
    emitEvent(QStringLiteral("route_finished"),
              {{QStringLiteral("index"), routeIndex_},
               {QStringLiteral("success"), success},
               {QStringLiteral("latency_ms"), routeRequestTimer_.elapsed()},
               {QStringLiteral("source"), routingController_->sourceLabel()},
               {QStringLiteral("distance_m"), routingController_->distanceMeters()},
               {QStringLiteral("duration_s"), routingController_->durationSeconds()},
               {QStringLiteral("status"), routingController_->statusMessage()}});
    ++routeIndex_;
    QTimer::singleShot(100, this, &BenchmarkScenarioController::requestNextRoute);
}

void BenchmarkScenarioController::startNavigationMeasurement()
{
    if (navigationController_->active() || finished_) {
        return;
    }
    navigationStarted_ = true;
    navigationController_->setSpeedMultiplier(4);
    performanceMonitor_->beginInteractionWindow(durationSeconds_ * 1000);
    navigationController_->start();
    emitEvent(QStringLiteral("navigation_started"),
              {{QStringLiteral("source"), navigationController_->sourceLabel()},
               {QStringLiteral("speed_multiplier"), navigationController_->speedMultiplier()}});
    const int deviationDelayMilliseconds = std::max(2000, std::min(durationSeconds_ * 500, 10000));
    deviationTimer_.start(deviationDelayMilliseconds);
    completionTimer_.start(durationSeconds_ * 1000);
}

void BenchmarkScenarioController::finish(bool success, const QString &reason)
{
    if (finished_) {
        return;
    }
    finished_ = true;
    warmupTimer_.stop();
    completionTimer_.stop();
    cameraTimer_.stop();
    deviationTimer_.stop();
    if (scenario_ == QLatin1String("navigation_replay")) {
        navigationController_->stop();
        if (!rerouteSucceeded_) {
            success = false;
            routeFailureSeen_ = true;
        }
    }
    auto *frames = qobject_cast<perf::FrameMetrics *>(performanceMonitor_->frameMetrics());
    QVariantMap fields;
    fields.insert(QStringLiteral("success"), success);
    fields.insert(QStringLiteral("reason"), reason);
    fields.insert(QStringLiteral("startup_ms"), firstMapFrameMilliseconds_);
    fields.insert(QStringLiteral("pss_mib"), performanceMonitor_->totalPssMiB());
    fields.insert(QStringLiteral("rss_mib"), performanceMonitor_->totalRssMiB());
    if (frames) {
        fields.insert(QStringLiteral("interaction_frame_count"), frames->interactionFrameCount());
        fields.insert(QStringLiteral("fps_average"), frames->interactionAverageFps());
        fields.insert(QStringLiteral("fps_p5"), frames->interactionFpsP5());
        fields.insert(QStringLiteral("fps_one_percent_low"), frames->interactionOnePercentLowFps());
        fields.insert(QStringLiteral("frame_time_average_ms"),
                      frames->frameTimeAverageMilliseconds());
        fields.insert(QStringLiteral("frame_time_p95_ms"), frames->frameTimeP95Milliseconds());
        fields.insert(QStringLiteral("frame_time_p99_ms"), frames->frameTimeP99Milliseconds());
    }
    emitEvent(QStringLiteral("result"), fields);
    emit completed(success);
}

void BenchmarkScenarioController::emitEvent(const QString &eventName,
                                            const QVariantMap &fields) const
{
    QVariantMap event = fields;
    event.insert(QStringLiteral("event"), eventName);
    event.insert(QStringLiteral("scenario"), scenario_);
    event.insert(QStringLiteral("timestamp_utc"),
                 QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    const QByteArray json =
        QJsonDocument(QJsonObject::fromVariantMap(event)).toJson(QJsonDocument::Compact);
    qInfo().noquote() << kEventPrefix << QString::fromUtf8(json);
}

qint64 BenchmarkScenarioController::elapsedFromLaunchMilliseconds() const
{
    QElapsedTimer now;
    now.start();
    return now.msecsSinceReference() - launchReferenceMilliseconds_;
}

} // namespace benchmark
} // namespace qtmaps
