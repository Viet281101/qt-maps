#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGeoServiceProvider>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>

#include <algorithm>
#include <functional>
#include <memory>

#if defined(__GLIBC__)
#include <malloc.h>
#endif

#include "benchmark/BenchmarkScenarioController.h"
#include "core/ApplicationShutdownCoordinator.h"
#include "diagnostics/CameraProbeController.h"
#include "diagnostics/SimulatedNavigationController.h"
#include "map/DirectoryTileSource.h"
#include "map/MapSourceController.h"
#include "map/MbTilesTileSource.h"
#include "perf/PerformanceMonitor.h"
#include "routing/RouteNavigationController.h"
#include "routing/RoutingController.h"

#ifndef QTMAPS_BUILD_ID
#define QTMAPS_BUILD_ID "unknown"
#endif

namespace {

void probeProvider(const QString &providerName)
{
    QGeoServiceProvider provider(providerName, QVariantMap(), true);
    const auto *mappingManager = provider.mappingManager();

    qInfo().noquote() << QStringLiteral("provider=%1 constructed=true mappingManager=%2 error=%3 "
                                        "errorString=%4 mappingError=%5 mappingErrorString=%6")
                             .arg(providerName,
                                  mappingManager != nullptr ? QStringLiteral("loaded")
                                                            : QStringLiteral("null"),
                                  QString::number(static_cast<int>(provider.error())),
                                  provider.errorString().isEmpty() ? QStringLiteral("<empty>")
                                                                   : provider.errorString(),
                                  QString::number(static_cast<int>(provider.mappingError())),
                                  provider.mappingErrorString().isEmpty()
                                      ? QStringLiteral("<empty>")
                                      : provider.mappingErrorString());
}

QPair<qint64, qint64> processMemoryKiB()
{
    QFile rollup(QStringLiteral("/proc/%1/smaps_rollup").arg(QCoreApplication::applicationPid()));
    if (!rollup.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {-1, -1};
    }
    qint64 pssKiB = -1;
    qint64 rssKiB = -1;
    while (true) {
        const QByteArray line = rollup.readLine();
        if (line.isEmpty()) {
            break;
        }
        if (line.startsWith("Pss:")) {
            pssKiB = line.simplified().split(' ').value(1).toLongLong();
        } else if (line.startsWith("Rss:")) {
            rssKiB = line.simplified().split(' ').value(1).toLongLong();
        }
    }
    return {pssKiB, rssKiB};
}

} // namespace

int main(int argc, char *argv[])
{
    QElapsedTimer launchTimer;
    launchTimer.start();
    QGuiApplication application(argc, argv);
    application.setQuitOnLastWindowClosed(false);
    QCoreApplication::setApplicationName(QStringLiteral("QtMaps"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.2.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("QtMaps Qt 5.15 diagnostic application"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({QStringLiteral("mapboxgl-probe"),
                      QStringLiteral("Run the Mapbox GL local-resource probe")});
    parser.addOption({QStringLiteral("maplibre-probe"),
                      QStringLiteral("Run the MapLibre local-resource probe")});
    parser.addOption({QStringLiteral("vector-style-url"), QStringLiteral("Local vector style URL"),
                      QStringLiteral("url"), QStringLiteral("http://127.0.0.1:8766/style.json")});
    parser.addOption(
        {QStringLiteral("real-vector-data"),
         QStringLiteral("Label the vector probe as real OSM-derived data with attribution")});
    parser.addOption({QStringLiteral("unit03-local-map"),
                      QStringLiteral("Run the checksummed Unit 03 local map package")});
    parser.addOption({QStringLiteral("map-package"),
                      QStringLiteral("Unit 03 map package directory or current pointer"),
                      QStringLiteral("path"), QStringLiteral("data/packages/current")});
    parser.addOption(
        {QStringLiteral("raster-package"),
         QStringLiteral("Unit 07C offline raster package directory or current pointer"),
         QStringLiteral("path"), QStringLiteral("data/raster-packages/current")});
    parser.addOption({QStringLiteral("diagnostic-raster-root"),
                      QStringLiteral("Local diagnostic raster fallback directory"),
                      QStringLiteral("path"), QStringLiteral("generated/diagnostic-tiles")});
    parser.addOption({QStringLiteral("force-vector-failure"),
                      QStringLiteral("Force Unit 03 diagnostic raster fallback")});
    parser.addOption({QStringLiteral("review-raster-toggle-cycles"),
                      QStringLiteral("Review-only raster/vector lifecycle probe"),
                      QStringLiteral("cycles")});
    parser.addOption({QStringLiteral("review-window-size"),
                      QStringLiteral("Review-only window size WIDTHxHEIGHT"),
                      QStringLiteral("size")});
    parser.addOption({QStringLiteral("display-diagnostics"),
                      QStringLiteral("Log screen, window, and Canvas pixel geometry")});
    parser.addOption({QStringLiteral("display-capture"),
                      QStringLiteral("Save a review-only window capture after startup"),
                      QStringLiteral("path")});
    parser.addOption({QStringLiteral("navigation-benchmark"),
                      QStringLiteral("Run continuous simulated camera-follow navigation")});
    parser.addOption({QStringLiteral("routing-demo"),
                      QStringLiteral("Run the Unit 04 fixture-first routing UI")});
    parser.addOption(
        {QStringLiteral("routing-fixture-only"),
         QStringLiteral("Do not start Valhalla; keep the routing UI on its recorded fixture")});
    parser.addOption({QStringLiteral("valhalla-service"),
                      QStringLiteral("Native valhalla_service executable"), QStringLiteral("path"),
                      QStringLiteral("generated/unit-04/valhalla-venv/lib/python3.12/site-packages/"
                                     "valhalla/bin/valhalla_service")});
    parser.addOption({QStringLiteral("valhalla-config"),
                      QStringLiteral("Generated loopback-only Valhalla config"),
                      QStringLiteral("path"),
                      QStringLiteral("generated/unit-04/valhalla/valhalla.json")});
    parser.addOption({QStringLiteral("valhalla-port"),
                      QStringLiteral("Configured Valhalla loopback port"), QStringLiteral("port"),
                      QStringLiteral("18002")});
    parser.addOption(
        {QStringLiteral("blank-probe"), QStringLiteral("Run the blank-window performance probe")});
    parser.addOption({QStringLiteral("disable-instrumentation"),
                      QStringLiteral("Disable frame and process instrumentation")});
    parser.addOption({QStringLiteral("automated-pan-zoom"),
                      QStringLiteral("Run the deterministic raster pan/zoom sequence")});
    parser.addOption({QStringLiteral("benchmark-output"),
                      QStringLiteral("Record benchmark CSV to PATH"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("benchmark"),
                      QStringLiteral("Run a Unit 06 benchmark scenario and exit"),
                      QStringLiteral("scenario")});
    parser.addOption({QStringList{QStringLiteral("benchmark-duration"), QStringLiteral("duration")},
                      QStringLiteral("Recorded duration after warm-up in seconds"),
                      QStringLiteral("seconds"), QStringLiteral("15")});
    parser.addOption({QStringLiteral("benchmark-warmup"),
                      QStringLiteral("Warm-up excluded from CSV and summaries in seconds"),
                      QStringLiteral("seconds"), QStringLiteral("3")});
    parser.addOption({QStringLiteral("benchmark-interval"),
                      QStringLiteral("Sampling interval in milliseconds"),
                      QStringLiteral("milliseconds"), QStringLiteral("500")});
    parser.addOption({QStringLiteral("benchmark-pss-interval"),
                      QStringLiteral("PSS smaps_rollup interval in milliseconds"),
                      QStringLiteral("milliseconds"), QStringLiteral("2000")});
    parser.addOption({QStringLiteral("exit-after"),
                      QStringLiteral("Exit after N seconds (useful for instrumentation-off runs)"),
                      QStringLiteral("seconds")});
    parser.addOption({QStringLiteral("scenario"), QStringLiteral("Benchmark scenario name"),
                      QStringLiteral("name"), QStringLiteral("diagnostic-raster")});
    parser.addOption({QStringLiteral("benchmark-run-id"),
                      QStringLiteral("Stable identifier for this benchmark run"),
                      QStringLiteral("id")});
    parser.addOption({QStringLiteral("network-mode"), QStringLiteral("Recorded network mode"),
                      QStringLiteral("mode"), QStringLiteral("localhost-offline-fixture")});
    parser.addOption({QStringLiteral("dataset-id"), QStringLiteral("Recorded dataset identifier"),
                      QStringLiteral("id"), QStringLiteral("unit01-diagnostic-v1")});
    parser.addOption({QStringLiteral("dataset-coverage"),
                      QStringLiteral("Recorded dataset coverage"), QStringLiteral("coverage"),
                      QStringLiteral("non-geographic Hanoi XYZ grid z10-z14")});
    parser.process(application);

    const bool useMapboxGlProbe = parser.isSet(QStringLiteral("mapboxgl-probe"));
    const bool useMapLibreProbe = parser.isSet(QStringLiteral("maplibre-probe"));
    const bool useVectorProbe = useMapboxGlProbe || useMapLibreProbe;
    const QString benchmarkScenario = parser.value(QStringLiteral("benchmark"));
    const bool useBenchmarkScenario = !benchmarkScenario.isEmpty();
    const bool useUnit03LocalMap =
        parser.isSet(QStringLiteral("unit03-local-map")) || useBenchmarkScenario;
    const bool useNavigationBenchmark = parser.isSet(QStringLiteral("navigation-benchmark"));
    const bool useRoutingDemo =
        parser.isSet(QStringLiteral("routing-demo")) || useBenchmarkScenario;
    const bool useBlankProbe = parser.isSet(QStringLiteral("blank-probe"));
    const bool instrumentationEnabled = !parser.isSet(QStringLiteral("disable-instrumentation"));

    const QStringList providers = QGeoServiceProvider::availableServiceProviders();
    qInfo().noquote() << "availableServiceProviders=" << providers.join(QLatin1Char(','));
    if (useVectorProbe || useUnit03LocalMap) {
        qInfo() << "providerConstructionProbe=skipped for localhost-only vector mode";
    } else {
        probeProvider(QStringLiteral("osm"));
        probeProvider(QStringLiteral("mapboxgl"));
    }

    qtmaps::diagnostics::CameraProbeController cameraProbeController;
    qtmaps::map::MbTilesTileSource vectorTileSource;
    qtmaps::map::DirectoryTileSource diagnosticRasterSource;
    qtmaps::map::MapSourceController mapSourceController(&vectorTileSource,
                                                         &diagnosticRasterSource);
    mapSourceController.setRasterPackagePath(parser.value(QStringLiteral("raster-package")));
    qtmaps::diagnostics::SimulatedNavigationController navigationController;
    qtmaps::routing::RoutingController routingController;
    qtmaps::routing::RouteNavigationController routeNavigationController;
    qtmaps::core::ApplicationShutdownCoordinator shutdownCoordinator(&routingController);
    qtmaps::perf::PerformanceMonitor performanceMonitor;
    qtmaps::benchmark::BenchmarkScenarioController benchmarkScenarioController(
        &performanceMonitor, &routingController, &routeNavigationController,
        launchTimer.msecsSinceReference());
    QQmlApplicationEngine engine;

    if (useUnit03LocalMap &&
        !mapSourceController.start(parser.value(QStringLiteral("map-package")),
                                   parser.value(QStringLiteral("diagnostic-raster-root")),
                                   parser.isSet(QStringLiteral("force-vector-failure")))) {
        qCritical().noquote() << "Unit 03 map startup failed:"
                              << mapSourceController.fallbackReason();
        return 3;
    }
    QVariantMap metadata;
    metadata.insert(QStringLiteral("scenario"), useBenchmarkScenario
                                                    ? benchmarkScenario
                                                    : parser.value(QStringLiteral("scenario")));
    metadata.insert(QStringLiteral("run_id"), parser.value(QStringLiteral("benchmark-run-id")));
    metadata.insert(QStringLiteral("build_id"), QStringLiteral(QTMAPS_BUILD_ID));
    metadata.insert(QStringLiteral("build_type"), QStringLiteral("Release"));
    metadata.insert(QStringLiteral("qt_version"), QString::fromLatin1(qVersion()));
    const QString renderLoop = qEnvironmentVariable("QSG_RENDER_LOOP", QStringLiteral("default"));
    metadata.insert(QStringLiteral("renderer"),
                    QStringLiteral("Qt Quick %1 render loop").arg(renderLoop));
    metadata.insert(QStringLiteral("driver"), QStringLiteral("Mesa runtime on reference host"));
    metadata.insert(QStringLiteral("network_mode"), parser.value(QStringLiteral("network-mode")));
    metadata.insert(QStringLiteral("dataset_id"), parser.value(QStringLiteral("dataset-id")));
    metadata.insert(QStringLiteral("dataset_coverage"),
                    parser.value(QStringLiteral("dataset-coverage")));
    performanceMonitor.setMetadata(metadata);
    performanceMonitor.setSamplingInterval(
        parser.value(QStringLiteral("benchmark-interval")).toInt());
    performanceMonitor.setPssSamplingInterval(
        parser.value(QStringLiteral("benchmark-pss-interval")).toInt());
    performanceMonitor.setWarmupSeconds(parser.value(QStringLiteral("benchmark-warmup")).toInt());
    engine.rootContext()->setContextProperty(QStringLiteral("performanceMonitor"),
                                             &performanceMonitor);
    engine.rootContext()->setContextProperty(QStringLiteral("cameraProbeController"),
                                             &cameraProbeController);
    engine.rootContext()->setContextProperty(QStringLiteral("navigationController"),
                                             &navigationController);
    engine.rootContext()->setContextProperty(QStringLiteral("routingController"),
                                             &routingController);
    engine.rootContext()->setContextProperty(QStringLiteral("mapSourceController"),
                                             &mapSourceController);
    engine.rootContext()->setContextProperty(QStringLiteral("routeNavigationController"),
                                             &routeNavigationController);
    engine.rootContext()->setContextProperty(QStringLiteral("shutdownCoordinator"),
                                             &shutdownCoordinator);
    engine.rootContext()->setContextProperty(QStringLiteral("benchmarkScenarioController"),
                                             &benchmarkScenarioController);
    engine.rootContext()->setContextProperty(QStringLiteral("routingFixtureOnly"),
                                             parser.isSet(QStringLiteral("routing-fixture-only")));
    QObject::connect(&routingController, &qtmaps::routing::RoutingController::routeChanged,
                     &routeNavigationController,
                     [&routingController, &routeNavigationController]() {
                         if (routingController.routeAvailable()) {
                             routeNavigationController.setRoute(routingController.route(),
                                                                routingController.sourceLabel());
                         } else {
                             routeNavigationController.clearRoute();
                         }
                     });
    QObject::connect(&routeNavigationController,
                     &qtmaps::routing::RouteNavigationController::deviationDetected,
                     &routingController, &qtmaps::routing::RoutingController::requestReroute);
    QObject::connect(&routingController, &qtmaps::routing::RoutingController::rerouteSucceeded,
                     &routeNavigationController,
                     &qtmaps::routing::RouteNavigationController::start);
    engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"),
                                             instrumentationEnabled);
    engine.rootContext()->setContextProperty(QStringLiteral("benchmarkAutomation"),
                                             parser.isSet(QStringLiteral("automated-pan-zoom")));
    const bool benchmarkRouteOverlayEnabled =
        qEnvironmentVariableIntValue("QTMAPS_BENCHMARK_DISABLE_ROUTE_OVERLAY") == 0;
    const bool benchmarkRenderTrace =
        qEnvironmentVariableIntValue("QTMAPS_BENCHMARK_TRACE_RENDER") != 0;
    engine.rootContext()->setContextProperty(QStringLiteral("routeOverlayEnabled"),
                                             benchmarkRouteOverlayEnabled);
    engine.rootContext()->setContextProperty(QStringLiteral("benchmarkRenderTrace"),
                                             benchmarkRenderTrace);
    const QString vectorStyleUrl = useUnit03LocalMap
                                       ? mapSourceController.activeStyleUrl()
                                       : parser.value(QStringLiteral("vector-style-url"));
    engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"), vectorStyleUrl);
    engine.rootContext()->setContextProperty(QStringLiteral("rasterTileBaseUrl"),
                                             useUnit03LocalMap
                                                 ? mapSourceController.rasterBaseUrl()
                                                 : QStringLiteral("http://127.0.0.1:8765/"));
    engine.rootContext()->setContextProperty(QStringLiteral("fallbackReason"),
                                             mapSourceController.fallbackReason());
    engine.rootContext()->setContextProperty(QStringLiteral("vectorPluginName"),
                                             useMapLibreProbe || useUnit03LocalMap
                                                 ? QStringLiteral("maplibre")
                                                 : QStringLiteral("mapboxgl"));
    engine.rootContext()->setContextProperty(
        QStringLiteral("vectorStyleParameterName"),
        useMapLibreProbe || useUnit03LocalMap
            ? QStringLiteral("maplibre.map.styles")
            : QStringLiteral("mapboxgl.mapping.additional_style_urls"));
    engine.rootContext()->setContextProperty(QStringLiteral("datasetLabel"),
                                             parser.value(QStringLiteral("dataset-id")));
    engine.rootContext()->setContextProperty(QStringLiteral("vectorDataReal"),
                                             useUnit03LocalMap ||
                                                 parser.isSet(QStringLiteral("real-vector-data")));
    const int interactionMilliseconds =
        (std::max(0, parser.value(QStringLiteral("benchmark-warmup")).toInt()) +
         std::max(1, parser.value(QStringLiteral("benchmark-duration")).toInt())) *
        1000;
    engine.rootContext()->setContextProperty(QStringLiteral("navigationInteractionMilliseconds"),
                                             interactionMilliseconds);

    const bool unit03Vector = useUnit03LocalMap && !mapSourceController.usingRasterFallback();
    const QString initialLayerMode = mapSourceController.layerMode();
    const bool productionRaster =
        useUnit03LocalMap && (initialLayerMode == QLatin1String("raster-loading") ||
                              initialLayerMode == QLatin1String("raster"));
    const bool unit03ProductMap = unit03Vector || productionRaster;
    bool valhallaPortOk = false;
    const int valhallaPort = parser.value(QStringLiteral("valhalla-port")).toInt(&valhallaPortOk);
    if (unit03ProductMap && useRoutingDemo &&
        !parser.isSet(QStringLiteral("routing-fixture-only")) &&
        (!valhallaPortOk || valhallaPort < 1 || valhallaPort > 65535)) {
        qCritical() << "--valhalla-port must be between 1 and 65535";
        return 4;
    }
    const QString benchmarkOutput = parser.value(QStringLiteral("benchmark-output"));
    if ((!benchmarkOutput.isEmpty() || useBenchmarkScenario) && !instrumentationEnabled) {
        qCritical() << "Benchmark recording cannot be used with --disable-instrumentation";
        return 2;
    }
    QString benchmarkConfigurationError;
    if (!benchmarkScenarioController.configure(
            benchmarkScenario, parser.value(QStringLiteral("benchmark-duration")).toInt(),
            parser.value(QStringLiteral("benchmark-warmup")).toInt(),
            parser.isSet(QStringLiteral("routing-fixture-only")), &benchmarkConfigurationError)) {
        qCritical().noquote() << benchmarkConfigurationError;
        return 5;
    }
    if (useBenchmarkScenario) {
        performanceMonitor.startBenchmark(benchmarkOutput);
    }
    const QUrl qmlUrl(
        unit03ProductMap && useRoutingDemo ? QStringLiteral("qrc:/qml/Main.qml")
        : unit03Vector && useNavigationBenchmark
            ? QStringLiteral("qrc:/qml/probes/NavigationProbe.qml")
        : useVectorProbe || unit03Vector ? QStringLiteral("qrc:/qml/probes/MapboxGlProbe.qml")
        : useBlankProbe                  ? QStringLiteral("qrc:/qml/probes/BlankProbe.qml")
                        : QStringLiteral("qrc:/qml/probes/DiagnosticRasterProbe.qml"));
    engine.load(qmlUrl);
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load" << qmlUrl;
        return 1;
    }
    if (unit03ProductMap && useRoutingDemo &&
        !parser.isSet(QStringLiteral("routing-fixture-only"))) {
        const QDir workingDirectory = QDir::current();
        routingController.startBackend(
            workingDirectory.absoluteFilePath(parser.value(QStringLiteral("valhalla-service"))),
            workingDirectory.absoluteFilePath(parser.value(QStringLiteral("valhalla-config"))),
            static_cast<quint16>(valhallaPort));
    }
    QObject::connect(&application, &QGuiApplication::lastWindowClosed, &shutdownCoordinator,
                     &qtmaps::core::ApplicationShutdownCoordinator::requestQuit);
    QObject::connect(&shutdownCoordinator,
                     &qtmaps::core::ApplicationShutdownCoordinator::shutdownReady, &application,
                     &QCoreApplication::quit);
    qInfo().noquote() << "probeMode="
                      << (unit03ProductMap && useRoutingDemo
                              ? (parser.isSet(QStringLiteral("routing-fixture-only"))
                                     ? (productionRaster ? "unit07c-raster-routing-fixture"
                                                         : "unit04-routing-fixture")
                                     : (productionRaster ? "unit07c-raster-routing-valhalla"
                                                         : "unit04-routing-valhalla"))
                          : unit03Vector && useNavigationBenchmark ? "unit03-navigation"
                          : unit03Vector                           ? "unit03-vector-package"
                          : useUnit03LocalMap ? "unit03-diagnostic-raster-fallback"
                          : useMapLibreProbe  ? "maplibre-local-resources"
                          : useMapboxGlProbe  ? "mapboxgl-local-resources"
                          : useBlankProbe     ? "blank-performance"
                                              : "osm-diagnostic-raster");
    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (window && parser.isSet(QStringLiteral("review-window-size"))) {
        const QStringList dimensions =
            parser.value(QStringLiteral("review-window-size")).split(QLatin1Char('x'));
        bool widthOk = false;
        bool heightOk = false;
        const int width = dimensions.value(0).toInt(&widthOk);
        const int height = dimensions.value(1).toInt(&heightOk);
        if (dimensions.size() != 2 || !widthOk || !heightOk || width < 1 || height < 1) {
            qCritical() << "--review-window-size must use WIDTHxHEIGHT";
            return 6;
        }
        window->resize(width, height);
    }
    if (instrumentationEnabled) {
        performanceMonitor.attachWindow(window);
    }
    benchmarkScenarioController.attachWindow(window);
    const bool displayDiagnostics = parser.isSet(QStringLiteral("display-diagnostics"));
    const QString displayCapturePath = parser.value(QStringLiteral("display-capture"));
    if (window && (displayDiagnostics || !displayCapturePath.isEmpty())) {
        QTimer::singleShot(1000, window, [window, displayDiagnostics, displayCapturePath]() {
            if (!displayCapturePath.isEmpty()) {
                const QImage capture = window->grabWindow();
                if (!capture.save(displayCapturePath)) {
                    qCritical().noquote() << QStringLiteral("QTMAPS_DISPLAY capture_failed=%1")
                                                 .arg(displayCapturePath);
                } else {
                    qInfo().noquote() << QStringLiteral("QTMAPS_DISPLAY capture=%1 pixels=%2x%3")
                                             .arg(displayCapturePath)
                                             .arg(capture.width())
                                             .arg(capture.height());
                }
            }
            if (!displayDiagnostics) {
                return;
            }
            const QScreen *screen = window->screen();
            const qreal dpr = window->effectiveDevicePixelRatio();
            const QSize windowPixels(qRound(window->width() * dpr), qRound(window->height() * dpr));
            qInfo().noquote() << QStringLiteral(
                                     "QTMAPS_DISPLAY screen name=%1 geometry=%2,%3 %4x%5 "
                                     "available=%6,%7 %8x%9 dpr=%10 logical_dpi=%11x%12 "
                                     "physical_dpi=%13x%14 physical_mm=%15x%16")
                                     .arg(screen ? screen->name() : QStringLiteral("<none>"))
                                     .arg(screen ? screen->geometry().x() : 0)
                                     .arg(screen ? screen->geometry().y() : 0)
                                     .arg(screen ? screen->geometry().width() : 0)
                                     .arg(screen ? screen->geometry().height() : 0)
                                     .arg(screen ? screen->availableGeometry().x() : 0)
                                     .arg(screen ? screen->availableGeometry().y() : 0)
                                     .arg(screen ? screen->availableGeometry().width() : 0)
                                     .arg(screen ? screen->availableGeometry().height() : 0)
                                     .arg(screen ? screen->devicePixelRatio() : 0)
                                     .arg(screen ? screen->logicalDotsPerInchX() : 0)
                                     .arg(screen ? screen->logicalDotsPerInchY() : 0)
                                     .arg(screen ? screen->physicalDotsPerInchX() : 0)
                                     .arg(screen ? screen->physicalDotsPerInchY() : 0)
                                     .arg(screen ? screen->physicalSize().width() : 0)
                                     .arg(screen ? screen->physicalSize().height() : 0);
            qInfo().noquote() << QStringLiteral(
                                     "QTMAPS_DISPLAY window logical=%1x%2 dpr=%3 framebuffer=%4x%5 "
                                     "render_target=%6")
                                     .arg(window->width())
                                     .arg(window->height())
                                     .arg(dpr)
                                     .arg(windowPixels.width())
                                     .arg(windowPixels.height())
                                     .arg(window->renderTarget() ? QStringLiteral("custom")
                                                                 : QStringLiteral("default"));
            QObject *overlay = window->findChild<QObject *>(QStringLiteral("rasterRouteOverlay"));
            if (!overlay) {
                qInfo().noquote() << QStringLiteral("QTMAPS_DISPLAY canvas=unavailable");
                return;
            }
            const QSizeF canvasSize = overlay->property("canvasSize").toSizeF();
            const QSize canvasPixels(qRound(canvasSize.width() * dpr),
                                     qRound(canvasSize.height() * dpr));
            qInfo().noquote()
                << QStringLiteral("QTMAPS_DISPLAY canvas logical=%1x%2 canvas_size=%3x%4 "
                                  "requested_fbo_pixels=%5x%6 render_target=%7 render_strategy=%8")
                       .arg(overlay->property("width").toReal())
                       .arg(overlay->property("height").toReal())
                       .arg(canvasSize.width())
                       .arg(canvasSize.height())
                       .arg(canvasPixels.width())
                       .arg(canvasPixels.height())
                       .arg(overlay->property("renderTarget").toInt())
                       .arg(overlay->property("renderStrategy").toInt());
            const auto logOverlayGeometry = [window, dpr](const QString &objectName) {
                QObject *item = window->findChild<QObject *>(objectName);
                if (!item) {
                    qInfo().noquote()
                        << QStringLiteral("QTMAPS_DISPLAY item=%1 unavailable").arg(objectName);
                    return;
                }
                const qreal width = item->property("width").toReal();
                const qreal height = item->property("height").toReal();
                qInfo().noquote() << QStringLiteral(
                                         "QTMAPS_DISPLAY item=%1 logical=%2x%3 pixels=%4x%5")
                                         .arg(objectName)
                                         .arg(width)
                                         .arg(height)
                                         .arg(qRound(width * dpr))
                                         .arg(qRound(height * dpr));
            };
            logOverlayGeometry(QStringLiteral("performanceOverlay"));
            logOverlayGeometry(QStringLiteral("mapAttributionBadge"));
        });
    }
    QObject::connect(&mapSourceController, &qtmaps::map::MapSourceController::stateChanged, &engine,
                     [&engine, &mapSourceController]() {
                         if (mapSourceController.loading() ||
                             (mapSourceController.layerMode() != QLatin1String("vector") &&
                              mapSourceController.layerMode() != QLatin1String("raster"))) {
                             return;
                         }
                         QTimer::singleShot(750, &engine, [&engine]() {
                             engine.collectGarbage();
#if defined(__GLIBC__)
                             malloc_trim(0);
#endif
                         });
                     });

    bool benchmarkCompletionPending = false;
    QObject::connect(
        &benchmarkScenarioController, &qtmaps::benchmark::BenchmarkScenarioController::completed,
        &application,
        [&benchmarkCompletionPending, &performanceMonitor, &shutdownCoordinator](bool) {
            benchmarkCompletionPending = true;
            performanceMonitor.stopBenchmark();
            if (!performanceMonitor.recording()) {
                shutdownCoordinator.requestQuit();
            }
        });
    QObject::connect(&performanceMonitor, &qtmaps::perf::PerformanceMonitor::recordingChanged,
                     &application,
                     [&benchmarkCompletionPending, &performanceMonitor, &shutdownCoordinator]() {
                         if (benchmarkCompletionPending && !performanceMonitor.recording()) {
                             shutdownCoordinator.requestQuit();
                         }
                     });

    if (!useBenchmarkScenario && !benchmarkOutput.isEmpty()) {
        const int warmupSeconds =
            std::max(0, parser.value(QStringLiteral("benchmark-warmup")).toInt());
        const int durationSeconds =
            std::max(1, parser.value(QStringLiteral("benchmark-duration")).toInt());
        QTimer::singleShot(250, [&performanceMonitor, benchmarkOutput]() {
            performanceMonitor.startBenchmark(benchmarkOutput);
        });
        const int intervalMilliseconds =
            std::max(100, parser.value(QStringLiteral("benchmark-interval")).toInt());
        QTimer::singleShot((warmupSeconds + durationSeconds) * 1000 + intervalMilliseconds,
                           &performanceMonitor, &qtmaps::perf::PerformanceMonitor::stopBenchmark);
        QTimer::singleShot((warmupSeconds + durationSeconds) * 1000 + intervalMilliseconds + 500,
                           &shutdownCoordinator,
                           &qtmaps::core::ApplicationShutdownCoordinator::requestQuit);
    } else if (parser.isSet(QStringLiteral("exit-after"))) {
        const int exitSeconds = std::max(1, parser.value(QStringLiteral("exit-after")).toInt());
        QTimer::singleShot(exitSeconds * 1000, &shutdownCoordinator,
                           &qtmaps::core::ApplicationShutdownCoordinator::requestQuit);
    }

    if (parser.isSet(QStringLiteral("review-raster-toggle-cycles"))) {
        const int requestedCycles =
            std::max(1, parser.value(QStringLiteral("review-raster-toggle-cycles")).toInt());
        auto cycle = std::make_shared<int>(0);
        auto rasterPhase = std::make_shared<bool>(true);
        auto step = std::make_shared<std::function<void()>>();
        *step = [&application, &mapSourceController, &shutdownCoordinator, requestedCycles, cycle,
                 rasterPhase, step]() {
            if (*cycle >= requestedCycles) {
                shutdownCoordinator.requestQuit();
                return;
            }
            if (*rasterPhase) {
                mapSourceController.activateRaster();
                QTimer::singleShot(
                    2500, &application, [&mapSourceController, cycle, rasterPhase, step]() {
                        const auto memory = processMemoryKiB();
                        qInfo().noquote()
                            << QStringLiteral("QTMAPS_RASTER_TOGGLE cycle=%1 "
                                              "phase=raster layer=%2 pss_kib=%3 "
                                              "rss_kib=%4 resources=%5")
                                   .arg(*cycle + 1)
                                   .arg(mapSourceController.layerMode())
                                   .arg(memory.first)
                                   .arg(memory.second)
                                   .arg(mapSourceController.rasterResourcesAllocated());
                        *rasterPhase = false;
                        (*step)();
                    });
                return;
            }
            mapSourceController.activateVector();
            QTimer::singleShot(
                3000, &application, [&mapSourceController, cycle, rasterPhase, step]() {
                    const auto memory = processMemoryKiB();
                    qInfo().noquote()
                        << QStringLiteral("QTMAPS_RASTER_TOGGLE cycle=%1 phase=vector "
                                          "layer=%2 pss_kib=%3 rss_kib=%4 "
                                          "resources=%5")
                               .arg(*cycle + 1)
                               .arg(mapSourceController.layerMode())
                               .arg(memory.first)
                               .arg(memory.second)
                               .arg(mapSourceController.rasterResourcesAllocated());
                    ++(*cycle);
                    *rasterPhase = true;
                    (*step)();
                });
        };
        QTimer::singleShot(1000, &application, [&mapSourceController, step]() {
            const auto memory = processMemoryKiB();
            qInfo().noquote() << QStringLiteral(
                                     "QTMAPS_RASTER_TOGGLE cycle=0 phase=baseline-vector layer=%1 "
                                     "pss_kib=%2 rss_kib=%3 resources=%4")
                                     .arg(mapSourceController.layerMode())
                                     .arg(memory.first)
                                     .arg(memory.second)
                                     .arg(mapSourceController.rasterResourcesAllocated());
            (*step)();
        });
    }

    return application.exec();
}
