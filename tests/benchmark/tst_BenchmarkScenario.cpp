#include "benchmark/BenchmarkScenarioController.h"
#include "perf/PerformanceMonitor.h"
#include "routing/RouteNavigationController.h"
#include "routing/RoutingController.h"

#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTest>

using qtmaps::benchmark::BenchmarkScenarioController;
using qtmaps::perf::PerformanceMonitor;
using qtmaps::routing::RouteNavigationController;
using qtmaps::routing::RoutingController;

class BenchmarkScenarioTest final : public QObject
{
    Q_OBJECT

private slots:
    void rejectsUnknownScenario()
    {
        PerformanceMonitor performance;
        RoutingController routing;
        RouteNavigationController navigation;
        QElapsedTimer launch;
        launch.start();
        BenchmarkScenarioController controller(&performance, &routing, &navigation,
                                               launch.msecsSinceReference());
        QString error;
        QVERIFY(!controller.configure(QStringLiteral("unknown"), 1, 0, true, &error));
        QVERIFY(error.contains(QStringLiteral("unknown")));
    }

    void startupCompletesAfterReadyFrameAndProcessSample()
    {
        PerformanceMonitor performance;
        RoutingController routing;
        RouteNavigationController navigation;
        QElapsedTimer launch;
        launch.start();
        BenchmarkScenarioController controller(&performance, &routing, &navigation,
                                               launch.msecsSinceReference());
        QString error;
        QVERIFY(controller.configure(QStringLiteral("startup"), 1, 0, true, &error));
        QSignalSpy completed(&controller, &BenchmarkScenarioController::completed);

        controller.notifyMapReady();
        QVERIFY(QMetaObject::invokeMethod(&controller, "handleFrameSwapped"));
        QCOMPARE(completed.count(), 0);
        QVERIFY(QMetaObject::invokeMethod(&controller, "handleProcessMetrics"));
        QCOMPARE(completed.count(), 1);
        QCOMPARE(completed.at(0).at(0).toBool(), true);
    }

    void fixtureRouteScenarioRunsEverySupportedPair()
    {
        PerformanceMonitor performance;
        RoutingController routing;
        RouteNavigationController navigation;
        connect(&routing, &RoutingController::routeChanged, &navigation, [&routing, &navigation]() {
            if (routing.routeAvailable()) {
                navigation.setRoute(routing.route(), routing.sourceLabel());
            } else {
                navigation.clearRoute();
            }
        });
        QElapsedTimer launch;
        launch.start();
        BenchmarkScenarioController controller(&performance, &routing, &navigation,
                                               launch.msecsSinceReference());
        QString error;
        QVERIFY(controller.configure(QStringLiteral("route"), 1, 0, true, &error));
        QSignalSpy completed(&controller, &BenchmarkScenarioController::completed);

        controller.notifyMapReady();
        QVERIFY(QMetaObject::invokeMethod(&controller, "handleFrameSwapped"));
        QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 2000);
        QCOMPARE(completed.at(0).at(0).toBool(), true);
        QVERIFY(routing.routeAvailable());
        QCOMPARE(routing.sourceLabel(), QStringLiteral("fixture/mô phỏng"));
    }
};

QTEST_GUILESS_MAIN(BenchmarkScenarioTest)

#include "tst_BenchmarkScenario.moc"
