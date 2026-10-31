#include <QtTest>

#include <QColor>
#include <QFile>
#include <QFont>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>

#include <cmath>

#include "core/ApplicationShutdownCoordinator.h"
#include "map/MapSourceController.h"
#include "map/TileSource.h"
#include "perf/PerformanceMonitor.h"
#include "routing/RouteNavigationController.h"
#include "routing/RoutingController.h"

using qtmaps::core::ApplicationShutdownCoordinator;
using qtmaps::perf::PerformanceMonitor;
using qtmaps::routing::RouteNavigationController;
using qtmaps::routing::RoutingController;

namespace {

double relativeLuminance(const QColor &color)
{
    const auto channel = [](double value) {
        return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(color.redF()) + 0.7152 * channel(color.greenF()) +
           0.0722 * channel(color.blueF());
}

double contrastRatio(const QColor &first, const QColor &second)
{
    const double lighter = std::max(relativeLuminance(first), relativeLuminance(second));
    const double darker = std::min(relativeLuminance(first), relativeLuminance(second));
    return (lighter + 0.05) / (darker + 0.05);
}

class StaticTileSource final : public qtmaps::map::TileSource
{
public:
    bool start(const QString &, quint16 = 0) override
    {
        running_ = true;
        return true;
    }
    void stop() override { running_ = false; }
    bool isRunning() const override { return running_; }
    QString baseUrl() const override { return QStringLiteral("http://127.0.0.1:9"); }
    QString errorMessage() const override { return QString(); }

private:
    bool running_ = false;
};

} // namespace

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
        StaticTileSource mapTiles;
        qtmaps::map::MapSourceController mapSource(&mapTiles, nullptr);
        QVERIFY(mapSource.start(QString(), QString()));
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("routingController"), &routing);
        engine.rootContext()->setContextProperty(QStringLiteral("routeNavigationController"),
                                                 &navigation);
        engine.rootContext()->setContextProperty(QStringLiteral("shutdownCoordinator"), &shutdown);
        engine.rootContext()->setContextProperty(QStringLiteral("performanceMonitor"),
                                                 &performance);
        engine.rootContext()->setContextProperty(QStringLiteral("mapSourceController"), &mapSource);
        engine.rootContext()->setContextProperty(QStringLiteral("routingFixtureOnly"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"), QString());

        QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/Main.qml")));
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
        StaticTileSource mapTiles;
        qtmaps::map::MapSourceController mapSource(&mapTiles, nullptr);
        QVERIFY(mapSource.start(QString(), QString()));
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("routingController"), &routing);
        engine.rootContext()->setContextProperty(QStringLiteral("routeNavigationController"),
                                                 &navigation);
        engine.rootContext()->setContextProperty(QStringLiteral("shutdownCoordinator"), &shutdown);
        engine.rootContext()->setContextProperty(QStringLiteral("performanceMonitor"),
                                                 &performance);
        engine.rootContext()->setContextProperty(QStringLiteral("mapSourceController"), &mapSource);
        engine.rootContext()->setContextProperty(QStringLiteral("routingFixtureOnly"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"), QString());

        QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/Main.qml")));
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
        StaticTileSource mapTiles;
        qtmaps::map::MapSourceController mapSource(&mapTiles, nullptr);
        QVERIFY(mapSource.start(QString(), QString()));
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("routingController"), &routing);
        engine.rootContext()->setContextProperty(QStringLiteral("routeNavigationController"),
                                                 &navigation);
        engine.rootContext()->setContextProperty(QStringLiteral("shutdownCoordinator"), &shutdown);
        engine.rootContext()->setContextProperty(QStringLiteral("performanceMonitor"),
                                                 &performance);
        engine.rootContext()->setContextProperty(QStringLiteral("mapSourceController"), &mapSource);
        engine.rootContext()->setContextProperty(QStringLiteral("routingFixtureOnly"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"), QString());

        QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/Main.qml")));
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
        StaticTileSource mapTiles;
        qtmaps::map::MapSourceController mapSource(&mapTiles, nullptr);
        QVERIFY(mapSource.start(QString(), QString()));
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("routingController"), &routing);
        engine.rootContext()->setContextProperty(QStringLiteral("routeNavigationController"),
                                                 &navigation);
        engine.rootContext()->setContextProperty(QStringLiteral("shutdownCoordinator"), &shutdown);
        engine.rootContext()->setContextProperty(QStringLiteral("performanceMonitor"),
                                                 &performance);
        engine.rootContext()->setContextProperty(QStringLiteral("mapSourceController"), &mapSource);
        engine.rootContext()->setContextProperty(QStringLiteral("routingFixtureOnly"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"), QString());

        QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/Main.qml")));
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

    void maneuverDirectionSymbolMeetsGraphicsContrast()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine,
                                QUrl(QStringLiteral("qrc:/qml/components/ManeuverRow.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> row(component.create());
        QVERIFY2(row, qPrintable(component.errorString()));
        row->setProperty("maneuver",
                         QVariantMap{{QStringLiteral("instruction"), QStringLiteral("Rẽ phải")},
                                     {QStringLiteral("distanceMeters"), 250.0}});
        QObject *icon = row->findChild<QObject *>(QStringLiteral("maneuverDirectionIcon"));
        QObject *symbol = row->findChild<QObject *>(QStringLiteral("maneuverDirectionSymbol"));
        QVERIFY(icon);
        QVERIFY(symbol);

        row->setProperty("selected", false);
        const QColor normalBackground = icon->property("color").value<QColor>();
        const QColor normalSymbol = symbol->property("color").value<QColor>();
        QVERIFY(contrastRatio(normalBackground, normalSymbol) >= 3.0);

        row->setProperty("selected", true);
        const QColor selectedBackground = icon->property("color").value<QColor>();
        const QColor selectedSymbol = symbol->property("color").value<QColor>();
        QVERIFY(contrastRatio(selectedBackground, selectedSymbol) >= 3.0);
    }

    void performanceBadgeDoesNotReportFakeZeroRss()
    {
        PerformanceMonitor performance;
        QQmlEngine engine;
        QQmlComponent component(&engine,
                                QUrl(QStringLiteral("qrc:/qml/components/PerformanceOverlay.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> overlay(component.create());
        QVERIFY2(overlay, qPrintable(component.errorString()));
        overlay->setProperty("monitor", QVariant::fromValue<QObject *>(&performance));

        QCOMPARE(overlay->property("processSampleAvailable").toBool(), false);
        QCOMPARE(overlay->property("compactRssText").toString(), QStringLiteral("RSS —"));
    }

    void connectionStatusUsesEverydayVietnameseLabels()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine,
                                QUrl(QStringLiteral("qrc:/qml/components/ConnectionStatus.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> status(component.create());
        QVERIFY2(status, qPrintable(component.errorString()));

        status->setProperty("connectionLabel", QStringLiteral("Valhalla local — loopback"));
        QCOMPARE(status->property("statusText").toString(), QStringLiteral("Valhalla (cục bộ)"));
        status->setProperty("connectionLabel", QStringLiteral("offline — fixture"));
        QCOMPARE(status->property("statusText").toString(),
                 QStringLiteral("Ngoại tuyến · tuyến mô phỏng"));
    }

    void navigationStartCollapsesPanelAndHidesStartAction()
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

        ApplicationShutdownCoordinator shutdown(&routing);
        PerformanceMonitor performance;
        StaticTileSource mapTiles;
        qtmaps::map::MapSourceController mapSource(&mapTiles, nullptr);
        QVERIFY(mapSource.start(QString(), QString()));
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("routingController"), &routing);
        engine.rootContext()->setContextProperty(QStringLiteral("routeNavigationController"),
                                                 &navigation);
        engine.rootContext()->setContextProperty(QStringLiteral("shutdownCoordinator"), &shutdown);
        engine.rootContext()->setContextProperty(QStringLiteral("performanceMonitor"),
                                                 &performance);
        engine.rootContext()->setContextProperty(QStringLiteral("mapSourceController"), &mapSource);
        engine.rootContext()->setContextProperty(QStringLiteral("routingFixtureOnly"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"), false);
        engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"), QString());

        QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/Main.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QObject *startAction = root->findChild<QObject *>(QStringLiteral("startNavigationButton"));
        QVERIFY(startAction);
        QCOMPARE(root->property("panelCollapsed").toBool(), false);
        QTRY_VERIFY(startAction->property("visible").toBool());

        navigation.start();
        QTRY_VERIFY(root->property("panelCollapsed").toBool());
        QTRY_VERIFY(!startAction->property("visible").toBool());
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
        QObject *nextInstruction =
            banner->findChild<QObject *>(QStringLiteral("nextInstructionText"));
        QVERIFY(nextInstruction);
        QVERIFY(nextInstruction->property("font").value<QFont>().pixelSize() >= 13);

        QCOMPARE(banner->property("primaryText").toString(), QStringLiteral("Rẽ trái"));
        QCOMPARE(banner->property("maneuverDistanceText").toString(), QStringLiteral("850 m"));
        QCOMPARE(banner->property("remainingText").toString(), QStringLiteral("1,5 km · 3 phút"));
        QCOMPARE(banner->property("sourceText").toString(),
                 QStringLiteral("Nguồn: fixture/mô phỏng"));

        banner->setProperty("rerouting", true);
        QCOMPARE(banner->property("primaryText").toString(), QStringLiteral("Đang tìm đường mới…"));
        banner->setProperty("rerouting", false);
        banner->setProperty("arrived", true);
        QCOMPARE(banner->property("primaryText").toString(), QStringLiteral("Đã đến nơi"));
        QCOMPARE(banner->property("maneuverDistanceText").toString(), QStringLiteral("0 m"));
    }

    void productRoutePanelDeclaresExpectedTabOrder()
    {
        QFile file(QStringLiteral(":/qml/components/RoutePanel.qml"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray qml = file.readAll();
        QVERIFY(qml.contains("KeyNavigation.tab: destinationField"));
        QVERIFY(qml.contains("KeyNavigation.tab: swapButton"));
        QVERIFY(qml.contains("KeyNavigation.tab: clearButton"));
        QVERIFY(qml.contains("KeyNavigation.tab: routeButton"));
        QVERIFY(qml.contains("KeyNavigation.tab: startNavigationButton"));
        QVERIFY(qml.contains("KeyNavigation.backtab: clearButton"));
    }

    void productShellKeepsRouteClearOfPanelAndExposesUiState()
    {
        QFile mainFile(QStringLiteral(":/qml/Main.qml"));
        QFile mapFile(QStringLiteral(":/qml/components/MapCanvas.qml"));
        QFile navigationFile(QStringLiteral(":/qml/components/NavigationOverlay.qml"));
        QFile bannerFile(QStringLiteral(":/qml/components/NavigationBanner.qml"));
        QFile panelFile(QStringLiteral(":/qml/components/RoutePanel.qml"));
        QFile toolsFile(QStringLiteral(":/qml/components/DemoToolsOverlay.qml"));
        QVERIFY(mainFile.open(QIODevice::ReadOnly));
        QVERIFY(mapFile.open(QIODevice::ReadOnly));
        QVERIFY(navigationFile.open(QIODevice::ReadOnly));
        QVERIFY(bannerFile.open(QIODevice::ReadOnly));
        QVERIFY(panelFile.open(QIODevice::ReadOnly));
        QVERIFY(toolsFile.open(QIODevice::ReadOnly));
        const QByteArray mainQml = mainFile.readAll();
        const QByteArray mapQml = mapFile.readAll();
        const QByteArray navigationQml = navigationFile.readAll();
        const QByteArray bannerQml = bannerFile.readAll();
        const QByteArray panelQml = panelFile.readAll();
        const QByteArray toolsQml = toolsFile.readAll();
        QVERIFY(mapQml.contains("panelWidth + Theme.spaceLg * 2"));
        QVERIFY(mainQml.contains("property bool panelCollapsed"));
        const QByteArray qml = mainQml + mapQml + navigationQml + bannerQml + panelQml + toolsQml;
        QVERIFY(qml.contains("routingController.streetLabels"));
        QVERIFY(qml.contains("map.zoomLevel >= 15"));
        QVERIFY(qml.contains("routingController.currentManeuverIndex"));
        QVERIFY(qml.contains("routeNavigationController.currentManeuverIndex"));
        QVERIFY(qml.contains("NavigationBanner"));
        QVERIFY(qml.contains("text: qsTr(\"Theo vị trí hiện tại\")"));
        QVERIFY(qml.contains("root.recenterNavigation()"));
        QVERIFY(qml.contains("routeNavigationController.simulateDeviation()"));
        QVERIFY(qml.contains("travelledRoutePath"));
        QVERIFY(qml.contains("Theme.routeTravelled"));
        QVERIFY(qml.contains("canvas.travelledRoutePath.length > 1"));
        QVERIFY(qml.contains("objectName: \"rasterRouteOverlay\""));
        QVERIFY(qml.contains("map.fromCoordinate"));
        QVERIFY(qml.contains("mapSourceController.activateRaster()"));
        QVERIFY(qml.contains("mapSourceController.activateVector()"));
        QVERIFY(qml.contains("độ phân giải thấp hơn"));
        QVERIFY(qml.contains("Qt.callLater(canvas.settleNavigationState)"));
        QVERIFY(qml.contains("canvas.guidanceActive"));
        QVERIFY(qml.contains("Đang tìm đường mới…"));
        QVERIFY(qml.contains("text: qsTr(\"Kết thúc\")"));
        QVERIFY(
            qml.contains("routingController.loading && routeNavigationController.routeAvailable"));
        QVERIFY(qml.contains("routeSummaryColor"));
        QVERIFY(qml.contains("&& !routingController.loading"));
        QVERIFY(qml.contains("navigationActive: navigationOverlay.guidanceVisible"));
        QVERIFY(qml.contains("shutdownCoordinator.requestQuit()"));
    }
};

QTEST_MAIN(RoutingQmlTest)

#include "tst_RoutingQml.moc"
