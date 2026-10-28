#pragma once

#include <QElapsedTimer>
#include <QGeoCoordinate>
#include <QObject>
#include <QPair>
#include <QTimer>
#include <QVariantMap>
#include <QVector>

class QQuickWindow;

namespace qtmaps {
namespace perf {
class PerformanceMonitor;
}
namespace routing {
class RouteNavigationController;
class RoutingController;
} // namespace routing
namespace benchmark {

class BenchmarkScenarioController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled CONSTANT)
    Q_PROPERTY(QString scenario READ scenario CONSTANT)

public:
    BenchmarkScenarioController(perf::PerformanceMonitor *performanceMonitor,
                                routing::RoutingController *routingController,
                                routing::RouteNavigationController *navigationController,
                                qint64 launchReferenceMilliseconds, QObject *parent = nullptr);

    bool configure(const QString &scenario, int durationSeconds, int warmupSeconds,
                   bool fixtureOnly, QString *errorMessage);
    void attachWindow(QQuickWindow *window);
    bool enabled() const;
    QString scenario() const;

    Q_INVOKABLE void notifyMapReady();

signals:
    void cameraCommand(const QGeoCoordinate &coordinate, double zoomLevel, double bearing);
    void completed(bool success);

private slots:
    void handleFrameSwapped();
    void handleProcessMetrics();
    void handleBackendReady();
    void handleRoutingChanged();
    void handleRerouteSucceeded();
    void advanceCamera();

private:
    bool requiresBackend() const;
    void maybeScheduleWarmup();
    void startMeasurement();
    void startRouteMeasurement();
    void requestNextRoute();
    void evaluateRouteRequest();
    void startNavigationMeasurement();
    void finish(bool success, const QString &reason = QString(), bool forcePss = true);
    void finalizeFinish();
    void emitEvent(const QString &eventName, const QVariantMap &fields = QVariantMap()) const;
    qint64 elapsedFromLaunchMilliseconds() const;

    perf::PerformanceMonitor *performanceMonitor_ = nullptr;
    routing::RoutingController *routingController_ = nullptr;
    routing::RouteNavigationController *navigationController_ = nullptr;
    QQuickWindow *window_ = nullptr;
    QString scenario_;
    int durationSeconds_ = 0;
    int warmupSeconds_ = 0;
    bool fixtureOnly_ = false;
    bool mapReady_ = false;
    bool firstMapFrameSeen_ = false;
    bool startupSamplePending_ = false;
    bool backendReady_ = false;
    bool warmupScheduled_ = false;
    bool measurementStarted_ = false;
    bool finished_ = false;
    bool finishSamplePending_ = false;
    bool finishSuccess_ = false;
    bool routeRequestActive_ = false;
    bool routeFailureSeen_ = false;
    bool navigationStarted_ = false;
    bool reroutePending_ = false;
    bool rerouteSucceeded_ = false;
    qint64 launchReferenceMilliseconds_ = 0;
    qint64 firstMapFrameMilliseconds_ = -1;
    QString finishReason_;
    int routeIndex_ = 0;
    int cameraStep_ = 0;
    QVector<QPair<QString, QString>> routePairs_;
    QElapsedTimer routeRequestTimer_;
    QElapsedTimer rerouteTimer_;
    QTimer warmupTimer_;
    QTimer completionTimer_;
    QTimer cameraTimer_;
    QTimer deviationTimer_;
};

} // namespace benchmark
} // namespace qtmaps
