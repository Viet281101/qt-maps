#include <QDebug>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QGuiApplication>
#include <QGeoServiceProvider>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>

#include <algorithm>

#include "map/CameraProbeController.h"
#include "core/ApplicationShutdownCoordinator.h"
#include "map/DirectoryTileSource.h"
#include "map/MapSourceController.h"
#include "map/MbTilesTileSource.h"
#include "map/SimulatedNavigationController.h"
#include "perf/PerformanceMonitor.h"
#include "routing/RoutingController.h"

#ifndef QTMAPS_BUILD_ID
#define QTMAPS_BUILD_ID "unknown"
#endif

namespace {

void probeProvider(const QString &providerName)
{
    QGeoServiceProvider provider(providerName, QVariantMap(), true);
    const auto *mappingManager = provider.mappingManager();

    qInfo().noquote()
        << QStringLiteral("provider=%1 constructed=true mappingManager=%2 error=%3 errorString=%4 mappingError=%5 mappingErrorString=%6")
               .arg(providerName,
                    mappingManager != nullptr ? QStringLiteral("loaded") : QStringLiteral("null"),
                    QString::number(static_cast<int>(provider.error())),
                    provider.errorString().isEmpty() ? QStringLiteral("<empty>") : provider.errorString(),
                    QString::number(static_cast<int>(provider.mappingError())),
                    provider.mappingErrorString().isEmpty()
                        ? QStringLiteral("<empty>")
                        : provider.mappingErrorString());
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    application.setQuitOnLastWindowClosed(false);
    QCoreApplication::setApplicationName(QStringLiteral("QtMaps"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.2.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("QtMaps Qt 5.15 diagnostic application"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({QStringLiteral("mapboxgl-probe"), QStringLiteral("Run the Mapbox GL local-resource probe")});
    parser.addOption({QStringLiteral("maplibre-probe"), QStringLiteral("Run the MapLibre local-resource probe")});
    parser.addOption({QStringLiteral("vector-style-url"), QStringLiteral("Local vector style URL"), QStringLiteral("url"), QStringLiteral("http://127.0.0.1:8766/style.json")});
    parser.addOption({QStringLiteral("real-vector-data"), QStringLiteral("Label the vector probe as real OSM-derived data with attribution")});
    parser.addOption({QStringLiteral("unit03-local-map"), QStringLiteral("Run the checksummed Unit 03 local map package")});
    parser.addOption({QStringLiteral("map-package"), QStringLiteral("Unit 03 map package directory or current pointer"), QStringLiteral("path"), QStringLiteral("data/packages/current")});
    parser.addOption({QStringLiteral("diagnostic-raster-root"), QStringLiteral("Local diagnostic raster fallback directory"), QStringLiteral("path"), QStringLiteral("generated/diagnostic-tiles")});
    parser.addOption({QStringLiteral("force-vector-failure"), QStringLiteral("Force Unit 03 diagnostic raster fallback")});
    parser.addOption({QStringLiteral("navigation-benchmark"), QStringLiteral("Run continuous simulated camera-follow navigation")});
    parser.addOption({QStringLiteral("routing-demo"), QStringLiteral("Run the Unit 04 fixture-first routing UI")});
    parser.addOption({QStringLiteral("routing-fixture-only"), QStringLiteral("Do not start Valhalla; keep the routing UI on its recorded fixture")});
    parser.addOption({QStringLiteral("valhalla-service"), QStringLiteral("Native valhalla_service executable"), QStringLiteral("path"), QStringLiteral("generated/unit-04/valhalla-venv/lib/python3.12/site-packages/valhalla/bin/valhalla_service")});
    parser.addOption({QStringLiteral("valhalla-config"), QStringLiteral("Generated loopback-only Valhalla config"), QStringLiteral("path"), QStringLiteral("generated/unit-04/valhalla/valhalla.json")});
    parser.addOption({QStringLiteral("valhalla-port"), QStringLiteral("Configured Valhalla loopback port"), QStringLiteral("port"), QStringLiteral("18002")});
    parser.addOption({QStringLiteral("blank-probe"), QStringLiteral("Run the blank-window performance probe")});
    parser.addOption({QStringLiteral("disable-instrumentation"), QStringLiteral("Disable frame and process instrumentation")});
    parser.addOption({QStringLiteral("automated-pan-zoom"), QStringLiteral("Run the deterministic raster pan/zoom sequence")});
    parser.addOption({QStringLiteral("benchmark-output"), QStringLiteral("Record benchmark CSV to PATH"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("benchmark-duration"), QStringLiteral("Recorded duration after warm-up in seconds"), QStringLiteral("seconds"), QStringLiteral("15")});
    parser.addOption({QStringLiteral("benchmark-warmup"), QStringLiteral("Warm-up excluded from CSV and summaries in seconds"), QStringLiteral("seconds"), QStringLiteral("3")});
    parser.addOption({QStringLiteral("benchmark-interval"), QStringLiteral("Sampling interval in milliseconds"), QStringLiteral("milliseconds"), QStringLiteral("1000")});
    parser.addOption({QStringLiteral("exit-after"), QStringLiteral("Exit after N seconds (useful for instrumentation-off runs)"), QStringLiteral("seconds")});
    parser.addOption({QStringLiteral("scenario"), QStringLiteral("Benchmark scenario name"), QStringLiteral("name"), QStringLiteral("diagnostic-raster")});
    parser.addOption({QStringLiteral("network-mode"), QStringLiteral("Recorded network mode"), QStringLiteral("mode"), QStringLiteral("localhost-offline-fixture")});
    parser.addOption({QStringLiteral("dataset-id"), QStringLiteral("Recorded dataset identifier"), QStringLiteral("id"), QStringLiteral("unit01-diagnostic-v1")});
    parser.addOption({QStringLiteral("dataset-coverage"), QStringLiteral("Recorded dataset coverage"), QStringLiteral("coverage"), QStringLiteral("non-geographic Hanoi XYZ grid z10-z14")});
    parser.process(application);

    const bool useMapboxGlProbe = parser.isSet(QStringLiteral("mapboxgl-probe"));
    const bool useMapLibreProbe = parser.isSet(QStringLiteral("maplibre-probe"));
    const bool useVectorProbe = useMapboxGlProbe || useMapLibreProbe;
    const bool useUnit03LocalMap = parser.isSet(QStringLiteral("unit03-local-map"));
    const bool useNavigationBenchmark = parser.isSet(QStringLiteral("navigation-benchmark"));
    const bool useRoutingDemo = parser.isSet(QStringLiteral("routing-demo"));
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

    qtmaps::map::CameraProbeController cameraProbeController;
    qtmaps::map::MbTilesTileSource vectorTileSource;
    qtmaps::map::DirectoryTileSource diagnosticRasterSource;
    qtmaps::map::MapSourceController mapSourceController(&vectorTileSource,
                                                         &diagnosticRasterSource);
    qtmaps::map::SimulatedNavigationController navigationController;
    qtmaps::routing::RoutingController routingController;
    qtmaps::core::ApplicationShutdownCoordinator shutdownCoordinator(&routingController);
    qtmaps::perf::PerformanceMonitor performanceMonitor;
    QQmlApplicationEngine engine;

    if (useUnit03LocalMap
        && !mapSourceController.start(parser.value(QStringLiteral("map-package")),
                                      parser.value(QStringLiteral("diagnostic-raster-root")),
                                      parser.isSet(QStringLiteral("force-vector-failure")))) {
        qCritical().noquote() << "Unit 03 map startup failed:"
                              << mapSourceController.fallbackReason();
        return 3;
    }
    QVariantMap metadata;
    metadata.insert(QStringLiteral("scenario"), parser.value(QStringLiteral("scenario")));
    metadata.insert(QStringLiteral("build_id"), QStringLiteral(QTMAPS_BUILD_ID));
    metadata.insert(QStringLiteral("build_type"), QStringLiteral("Release"));
    metadata.insert(QStringLiteral("qt_version"), QString::fromLatin1(qVersion()));
    const QString renderLoop = qEnvironmentVariable("QSG_RENDER_LOOP", QStringLiteral("default"));
    metadata.insert(QStringLiteral("renderer"),
                    QStringLiteral("Qt Quick %1 render loop").arg(renderLoop));
    metadata.insert(QStringLiteral("driver"), QStringLiteral("Mesa runtime on reference host"));
    metadata.insert(QStringLiteral("network_mode"), parser.value(QStringLiteral("network-mode")));
    metadata.insert(QStringLiteral("dataset_id"), parser.value(QStringLiteral("dataset-id")));
    metadata.insert(QStringLiteral("dataset_coverage"), parser.value(QStringLiteral("dataset-coverage")));
    performanceMonitor.setMetadata(metadata);
    performanceMonitor.setSamplingInterval(parser.value(QStringLiteral("benchmark-interval")).toInt());
    performanceMonitor.setWarmupSeconds(parser.value(QStringLiteral("benchmark-warmup")).toInt());
    engine.rootContext()->setContextProperty(QStringLiteral("performanceMonitor"), &performanceMonitor);
    engine.rootContext()->setContextProperty(QStringLiteral("cameraProbeController"),
                                             &cameraProbeController);
    engine.rootContext()->setContextProperty(QStringLiteral("navigationController"),
                                             &navigationController);
    engine.rootContext()->setContextProperty(QStringLiteral("routingController"),
                                             &routingController);
    engine.rootContext()->setContextProperty(
        QStringLiteral("routingFixtureOnly"),
        parser.isSet(QStringLiteral("routing-fixture-only")));
    engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"), instrumentationEnabled);
    engine.rootContext()->setContextProperty(QStringLiteral("benchmarkAutomation"),
                                             parser.isSet(QStringLiteral("automated-pan-zoom")));
    const QString vectorStyleUrl = useUnit03LocalMap && !mapSourceController.usingRasterFallback()
        ? mapSourceController.vectorStyleUrl()
        : parser.value(QStringLiteral("vector-style-url"));
    engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"), vectorStyleUrl);
    engine.rootContext()->setContextProperty(
        QStringLiteral("rasterTileBaseUrl"),
        useUnit03LocalMap ? mapSourceController.rasterBaseUrl()
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
                                             useUnit03LocalMap
                                                 || parser.isSet(QStringLiteral("real-vector-data")));
    const int interactionMilliseconds =
        (std::max(0, parser.value(QStringLiteral("benchmark-warmup")).toInt())
         + std::max(1, parser.value(QStringLiteral("benchmark-duration")).toInt())) * 1000;
    engine.rootContext()->setContextProperty(QStringLiteral("navigationInteractionMilliseconds"),
                                             interactionMilliseconds);

    const bool unit03Vector = useUnit03LocalMap && !mapSourceController.usingRasterFallback();
    bool valhallaPortOk = false;
    const int valhallaPort = parser.value(QStringLiteral("valhalla-port")).toInt(&valhallaPortOk);
    if (unit03Vector && useRoutingDemo
        && !parser.isSet(QStringLiteral("routing-fixture-only"))
        && (!valhallaPortOk || valhallaPort < 1 || valhallaPort > 65535)) {
        qCritical() << "--valhalla-port must be between 1 and 65535";
        return 4;
    }
    const QString benchmarkOutput = parser.value(QStringLiteral("benchmark-output"));
    if (!benchmarkOutput.isEmpty() && !instrumentationEnabled) {
        qCritical() << "--benchmark-output cannot be used with --disable-instrumentation";
        return 2;
    }
    const QUrl qmlUrl(unit03Vector && useRoutingDemo
                          ? QStringLiteral("qrc:/qml/RoutingProbe.qml")
                      : unit03Vector && useNavigationBenchmark
                          ? QStringLiteral("qrc:/qml/NavigationProbe.qml")
                          : useVectorProbe || unit03Vector
                              ? QStringLiteral("qrc:/qml/MapboxGlProbe.qml")
                              : useBlankProbe ? QStringLiteral("qrc:/qml/BlankProbe.qml")
                                              : QStringLiteral("qrc:/qml/Main.qml"));
    engine.load(qmlUrl);
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load" << qmlUrl;
        return 1;
    }
    if (unit03Vector && useRoutingDemo
        && !parser.isSet(QStringLiteral("routing-fixture-only"))) {
        const QDir workingDirectory = QDir::current();
        routingController.startBackend(
            workingDirectory.absoluteFilePath(parser.value(QStringLiteral("valhalla-service"))),
            workingDirectory.absoluteFilePath(parser.value(QStringLiteral("valhalla-config"))),
            static_cast<quint16>(valhallaPort));
    }
    QObject::connect(&application, &QGuiApplication::lastWindowClosed,
                     &shutdownCoordinator,
                     &qtmaps::core::ApplicationShutdownCoordinator::requestQuit);
    QObject::connect(&shutdownCoordinator,
                     &qtmaps::core::ApplicationShutdownCoordinator::shutdownReady,
                     &application, &QCoreApplication::quit);
    qInfo().noquote() << "probeMode="
                      << (unit03Vector && useRoutingDemo
                              ? (parser.isSet(QStringLiteral("routing-fixture-only"))
                                     ? "unit04-routing-fixture"
                                     : "unit04-routing-valhalla")
                          : unit03Vector && useNavigationBenchmark ? "unit03-navigation"
                          : unit03Vector ? "unit03-vector-package"
                          : useUnit03LocalMap ? "unit03-diagnostic-raster-fallback"
                          : useMapLibreProbe ? "maplibre-local-resources"
                                          : useMapboxGlProbe ? "mapboxgl-local-resources"
                                          : useBlankProbe ? "blank-performance"
                                                          : "osm-diagnostic-raster");
    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (instrumentationEnabled) {
        performanceMonitor.attachWindow(window);
    }

    if (!benchmarkOutput.isEmpty()) {
        const int warmupSeconds = std::max(0, parser.value(QStringLiteral("benchmark-warmup")).toInt());
        const int durationSeconds = std::max(1, parser.value(QStringLiteral("benchmark-duration")).toInt());
        QTimer::singleShot(250, [&performanceMonitor, benchmarkOutput]() {
            performanceMonitor.startBenchmark(benchmarkOutput);
        });
        const int intervalMilliseconds = std::max(
            100, parser.value(QStringLiteral("benchmark-interval")).toInt());
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

    return application.exec();
}
