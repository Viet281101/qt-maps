#include <QDebug>
#include <QCommandLineOption>
#include <QCommandLineParser>
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
#include "perf/PerformanceMonitor.h"

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
    QCoreApplication::setApplicationName(QStringLiteral("QtMaps"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.2.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("QtMaps Qt 5.13 diagnostic application"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({QStringLiteral("mapboxgl-probe"), QStringLiteral("Run the Mapbox GL local-resource probe")});
    parser.addOption({QStringLiteral("vector-style-url"), QStringLiteral("Mapbox GL local style URL"), QStringLiteral("url"), QStringLiteral("http://127.0.0.1:8766/style.json")});
    parser.addOption({QStringLiteral("real-vector-data"), QStringLiteral("Label the vector probe as real OSM-derived data with attribution")});
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
    const bool useBlankProbe = parser.isSet(QStringLiteral("blank-probe"));
    const bool instrumentationEnabled = !parser.isSet(QStringLiteral("disable-instrumentation"));

    const QStringList providers = QGeoServiceProvider::availableServiceProviders();
    qInfo().noquote() << "availableServiceProviders=" << providers.join(QLatin1Char(','));
    if (useMapboxGlProbe) {
        qInfo() << "providerConstructionProbe=skipped for localhost-only vector mode";
    } else {
        probeProvider(QStringLiteral("osm"));
        probeProvider(QStringLiteral("mapboxgl"));
    }

    QQmlApplicationEngine engine;
    qtmaps::map::CameraProbeController cameraProbeController;
    qtmaps::perf::PerformanceMonitor performanceMonitor;
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
    engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"), instrumentationEnabled);
    engine.rootContext()->setContextProperty(QStringLiteral("benchmarkAutomation"),
                                             parser.isSet(QStringLiteral("automated-pan-zoom")));
    engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"),
                                             parser.value(QStringLiteral("vector-style-url")));
    engine.rootContext()->setContextProperty(QStringLiteral("datasetLabel"),
                                             parser.value(QStringLiteral("dataset-id")));
    engine.rootContext()->setContextProperty(QStringLiteral("vectorDataReal"),
                                             parser.isSet(QStringLiteral("real-vector-data")));

    const QUrl qmlUrl(useMapboxGlProbe
                          ? QStringLiteral("qrc:/qml/MapboxGlProbe.qml")
                          : useBlankProbe ? QStringLiteral("qrc:/qml/BlankProbe.qml")
                                          : QStringLiteral("qrc:/qml/Main.qml"));
    qInfo().noquote() << "probeMode="
                      << (useMapboxGlProbe ? "mapboxgl-local-resources"
                                          : useBlankProbe ? "blank-performance"
                                                          : "osm-diagnostic-raster");
    engine.load(qmlUrl);
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load" << qmlUrl;
        return 1;
    }

    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (instrumentationEnabled) {
        performanceMonitor.attachWindow(window);
    }

    const QString benchmarkOutput = parser.value(QStringLiteral("benchmark-output"));
    if (!benchmarkOutput.isEmpty()) {
        if (!instrumentationEnabled) {
            qCritical() << "--benchmark-output cannot be used with --disable-instrumentation";
            return 2;
        }
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
                           &application, &QCoreApplication::quit);
    } else if (parser.isSet(QStringLiteral("exit-after"))) {
        const int exitSeconds = std::max(1, parser.value(QStringLiteral("exit-after")).toInt());
        QTimer::singleShot(exitSeconds * 1000, &application, &QCoreApplication::quit);
    }

    return application.exec();
}
