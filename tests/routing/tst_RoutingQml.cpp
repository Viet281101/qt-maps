#include <QtTest>

#include <QColor>
#include <QFile>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>

#include "core/ApplicationShutdownCoordinator.h"
#include "perf/PerformanceMonitor.h"
#include "routing/RouteNavigationController.h"
#include "routing/RoutingController.h"

using qtmaps::core::ApplicationShutdownCoordinator;
using qtmaps::perf::PerformanceMonitor;
using qtmaps::routing::RouteNavigationController;
using qtmaps::routing::RoutingController;

class RoutingQmlTest final : public QObject
{
    Q_OBJECT

private slots:
    void editedEndpointsAreAppliedTogether()
    {
        RoutingController routing;
        RouteNavigationController navigation;
        ApplicationShutdownCoordinator shutdown(&routing);
        PerformanceMonitor performance;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("routingController"), &routing);
        engine.rootContext()->setContextProperty(QStringLiteral("routeNavigationController"),
                                                 &navigation);
        engine.rootContext()->setContextProperty(QStringLiteral("shutdownCoordinator"), &shutdown);
        engine.rootContext()->setContextProperty(QStringLiteral("performanceMonitor"),
                                                 &performance);
        engine.rootContext()->setContextProperty(QStringLiteral("routingFixtureOnly"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"), QString());

        QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/RoutingProbe.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QObject *origin = root->findChild<QObject *>(QStringLiteral("originField"));
        QObject *destination = root->findChild<QObject *>(QStringLiteral("destinationField"));
        QVERIFY(origin);
        QVERIFY(destination);
        origin->setProperty("text", QStringLiteral("21.021352,105.832698"));
        destination->setProperty("text", QStringLiteral("21.021352,105.834744"));

        QVERIFY(QMetaObject::invokeMethod(root.data(), "applyEndpoints"));
        QCOMPARE(routing.origin(), QGeoCoordinate(21.021352, 105.832698));
        QCOMPARE(routing.destination(), QGeoCoordinate(21.021352, 105.834744));
    }

    void maneuverListKeepsMinimumHeight_data()
    {
        QTest::addColumn<QSize>("windowSize");
        QTest::newRow("minimum-window") << QSize(900, 580);
        QTest::newRow("demo-window") << QSize(1366, 768);
    }

    void maneuverListKeepsMinimumHeight()
    {
        QFETCH(QSize, windowSize);

        RoutingController routing;
        RouteNavigationController navigation;
        connect(&routing, &RoutingController::routeChanged, &navigation, [&]() {
            if (routing.routeAvailable())
                navigation.setRoute(routing.route(), routing.sourceLabel());
            else
                navigation.clearRoute();
        });
        routing.calculateFixtureRoute();
        navigation.start();

        ApplicationShutdownCoordinator shutdown(&routing);
        PerformanceMonitor performance;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("routingController"), &routing);
        engine.rootContext()->setContextProperty(QStringLiteral("routeNavigationController"),
                                                 &navigation);
        engine.rootContext()->setContextProperty(QStringLiteral("shutdownCoordinator"), &shutdown);
        engine.rootContext()->setContextProperty(QStringLiteral("performanceMonitor"),
                                                 &performance);
        engine.rootContext()->setContextProperty(QStringLiteral("routingFixtureOnly"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"), QString());

        QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/RoutingProbe.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QQuickWindow *window = qobject_cast<QQuickWindow *>(root.data());
        QVERIFY(window);
        window->resize(windowSize);
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window));

        QQuickItem *maneuvers = root->findChild<QQuickItem *>(QStringLiteral("maneuverList"));
        QQuickItem *controls = root->findChild<QQuickItem *>(QStringLiteral("routeControlsScroll"));
        QQuickItem *banner = root->findChild<QQuickItem *>(QStringLiteral("navigationBanner"));
        QQuickItem *status = root->findChild<QQuickItem *>(QStringLiteral("connectionStatusPill"));
        QVERIFY(maneuvers);
        QVERIFY(controls);
        QVERIFY(banner);
        QVERIFY(status);
        QTRY_VERIFY(maneuvers->height() >= 120.0);
        QVERIFY(maneuvers->property("contentHeight").toReal() > maneuvers->height());
        if (windowSize.height() == 580)
            QVERIFY(controls->property("contentHeight").toReal() > controls->height());

        const QRectF bannerRect(banner->mapToScene(QPointF(0, 0)), banner->size());
        const QRectF statusRect(status->mapToScene(QPointF(0, 0)), status->size());
        QVERIFY2(!bannerRect.intersects(statusRect),
                 qPrintable(QStringLiteral("banner %1,%2 %3x%4 intersects status %5,%6 %7x%8")
                                .arg(bannerRect.x())
                                .arg(bannerRect.y())
                                .arg(bannerRect.width())
                                .arg(bannerRect.height())
                                .arg(statusRect.x())
                                .arg(statusRect.y())
                                .arg(statusRect.width())
                                .arg(statusRect.height())));
    }

    void mapClickDoesNotChangeRouteDuringReplay()
    {
        RoutingController routing;
        RouteNavigationController navigation;
        connect(&routing, &RoutingController::routeChanged, &navigation, [&]() {
            if (routing.routeAvailable())
                navigation.setRoute(routing.route(), routing.sourceLabel());
            else
                navigation.clearRoute();
        });
        routing.calculateFixtureRoute();
        QVERIFY(routing.routeAvailable());
        navigation.start();
        QVERIFY(navigation.running());

        ApplicationShutdownCoordinator shutdown(&routing);
        PerformanceMonitor performance;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("routingController"), &routing);
        engine.rootContext()->setContextProperty(QStringLiteral("routeNavigationController"),
                                                 &navigation);
        engine.rootContext()->setContextProperty(QStringLiteral("shutdownCoordinator"), &shutdown);
        engine.rootContext()->setContextProperty(QStringLiteral("performanceMonitor"),
                                                 &performance);
        engine.rootContext()->setContextProperty(QStringLiteral("routingFixtureOnly"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"), QString());

        QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/RoutingProbe.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QQuickWindow *window = qobject_cast<QQuickWindow *>(root.data());
        QVERIFY(window);
        window->setWidth(1120);
        window->setHeight(700);
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window));

        QObject *tapHandler = root->findChild<QObject *>(QStringLiteral("routeMapTapHandler"));
        QVERIFY(tapHandler);
        QCOMPARE(tapHandler->property("enabled").toBool(), false);

        const QGeoCoordinate origin = routing.origin();
        const QGeoCoordinate destination = routing.destination();
        const int routePointCount = routing.routePath().size();
        QSignalSpy endpointsSpy(&routing, &RoutingController::endpointsChanged);
        QSignalSpy routeSpy(&routing, &RoutingController::routeChanged);

        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(800, 400));
        QTest::qWait(50);
        QCOMPARE(endpointsSpy.size(), 0);
        QCOMPARE(routeSpy.size(), 0);
        QCOMPARE(routing.origin(), origin);
        QCOMPARE(routing.destination(), destination);
        QCOMPARE(routing.routePath().size(), routePointCount);
        QVERIFY(routing.routeAvailable());
        QVERIFY(navigation.running());

        navigation.stop();
        QTRY_COMPARE(tapHandler->property("enabled").toBool(), true);
    }

    void arrivedActionRestartsSameRoute()
    {
        RoutingController routing;
        RouteNavigationController navigation;
        connect(&routing, &RoutingController::routeChanged, &navigation, [&]() {
            if (routing.routeAvailable())
                navigation.setRoute(routing.route(), routing.sourceLabel());
            else
                navigation.clearRoute();
        });
        routing.calculateFixtureRoute();
        navigation.start();
        navigation.advanceBySeconds(routing.durationSeconds() * 2.0);
        QVERIFY(navigation.arrived());
        const double routeLength = navigation.routeLengthMeters();
        QVERIFY(routeLength > 0.0);

        ApplicationShutdownCoordinator shutdown(&routing);
        PerformanceMonitor performance;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("routingController"), &routing);
        engine.rootContext()->setContextProperty(QStringLiteral("routeNavigationController"),
                                                 &navigation);
        engine.rootContext()->setContextProperty(QStringLiteral("shutdownCoordinator"), &shutdown);
        engine.rootContext()->setContextProperty(QStringLiteral("performanceMonitor"),
                                                 &performance);
        engine.rootContext()->setContextProperty(QStringLiteral("routingFixtureOnly"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"), QString());

        QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/RoutingProbe.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QObject *action = root->findChild<QObject *>(QStringLiteral("navigationPrimaryAction"));
        QVERIFY(action);
        QCOMPARE(action->property("text").toString(), QStringLiteral("Đi lại từ đầu"));

        QVERIFY(QMetaObject::invokeMethod(action, "triggered"));
        QTRY_VERIFY(navigation.running());
        QVERIFY(!navigation.arrived());
        QVERIFY(navigation.travelledMeters() < routeLength);
        QVERIFY(navigation.travelledMeters() < 1.0);
    }

    void maneuverRowHasPersistentSelectedStyle()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine,
                                QUrl(QStringLiteral("qrc:/qml/components/ManeuverRow.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> row(component.create());
        QVERIFY2(row, qPrintable(component.errorString()));
        row->setProperty("maneuverIndex", 3);
        row->setProperty("maneuver",
                         QVariantMap{{QStringLiteral("instruction"), QStringLiteral("Rẽ phải")},
                                     {QStringLiteral("distanceMeters"), 250.0}});

        row->setProperty("selected", false);
        const QColor normal = row->property("color").value<QColor>();
        row->setProperty("selected", true);
        const QColor selected = row->property("color").value<QColor>();

        QVERIFY(normal.isValid());
        QVERIFY(selected.isValid());
        QVERIFY(normal != selected);
        QCOMPARE(row->property("selected").toBool(), true);
    }

    void keyboardActionButtonSupportsTabReturnAndSpace()
    {
        QQmlEngine engine;
        QQmlComponent component(
            &engine, QUrl(QStringLiteral("qrc:/qml/components/KeyboardActionButton.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> button(component.create());
        QVERIFY2(button, qPrintable(component.errorString()));
        QQuickItem *item = qobject_cast<QQuickItem *>(button.data());
        QVERIFY(item);
        QVERIFY(item->activeFocusOnTab());

        QQuickWindow window;
        item->setParentItem(window.contentItem());
        window.show();
        item->forceActiveFocus();
        QVERIFY(item->hasActiveFocus());
        QSignalSpy triggered(button.data(), SIGNAL(triggered()));
        QTest::keyClick(&window, Qt::Key_Return);
        QTest::keyClick(&window, Qt::Key_Space);
        QCOMPARE(triggered.size(), 2);
    }

    void navigationBannerFormatsGuidanceAndArrival()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine,
                                QUrl(QStringLiteral("qrc:/qml/components/NavigationBanner.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> banner(component.create());
        QVERIFY2(banner, qPrintable(component.errorString()));

        banner->setProperty("instruction", QStringLiteral("Rẽ trái"));
        banner->setProperty("streetName", QStringLiteral("Phố Huế"));
        banner->setProperty("sourceLabel", QStringLiteral("fixture/mô phỏng"));
        banner->setProperty("distanceToManeuverMeters", 850.0);
        banner->setProperty("remainingDistanceMeters", 1540.0);
        banner->setProperty("remainingDurationSeconds", 121.0);

        QCOMPARE(banner->property("primaryText").toString(), QStringLiteral("Rẽ trái"));
        QCOMPARE(banner->property("maneuverDistanceText").toString(), QStringLiteral("850 m"));
        QCOMPARE(banner->property("remainingText").toString(),
                 QStringLiteral("Còn 1.5 km • 3 phút"));
        QCOMPARE(banner->property("sourceText").toString(),
                 QStringLiteral("Nguồn: fixture/mô phỏng"));

        banner->setProperty("arrived", true);
        QCOMPARE(banner->property("primaryText").toString(), QStringLiteral("Đã đến đích"));
        QCOMPARE(banner->property("maneuverDistanceText").toString(), QStringLiteral("0 m"));
    }

    void routingProbeDeclaresExpectedTabOrder()
    {
        QFile file(QStringLiteral(":/qml/RoutingProbe.qml"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray qml = file.readAll();
        QVERIFY(qml.contains("KeyNavigation.tab: destinationField"));
        QVERIFY(qml.contains("KeyNavigation.tab: applyButton"));
        QVERIFY(qml.contains("KeyNavigation.tab: routeButton"));
        QVERIFY(qml.contains("KeyNavigation.tab: startNavigationButton"));
        QVERIFY(qml.contains("KeyNavigation.backtab: routeButton"));
    }

    void routingProbeKeepsRouteClearOfPanelAndExposesUiState()
    {
        QFile file(QStringLiteral(":/qml/RoutingProbe.qml"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray qml = file.readAll();
        QVERIFY(qml.contains("routePanel.width + Theme.spaceLg * 2"));
        QVERIFY(qml.contains("property bool panelCollapsed"));
        QVERIFY(qml.contains("routingController.streetLabels"));
        QVERIFY(qml.contains("map.zoomLevel >= 15"));
        QVERIFY(qml.contains("routingController.currentManeuverIndex"));
        QVERIFY(qml.contains("routeNavigationController.currentManeuverIndex"));
        QVERIFY(qml.contains("NavigationBanner"));
        QVERIFY(qml.contains("text: qsTr(\"Theo vị trí hiện tại\")"));
        QVERIFY(qml.contains("root.recenterNavigation()"));
        QVERIFY(qml.contains("routeNavigationController.simulateDeviation()"));
        QVERIFY(
            qml.contains("routingController.loading && routeNavigationController.routeAvailable"));
        QVERIFY(qml.contains("routeSummaryColor"));
        QVERIFY(qml.contains("shutdownCoordinator.requestQuit()"));
    }
};

QTEST_MAIN(RoutingQmlTest)

#include "tst_RoutingQml.moc"
