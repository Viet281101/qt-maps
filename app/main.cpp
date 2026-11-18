#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QGeoServiceProvider>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickView>
#include <QQuickWindow>
#include <QStringList>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QVariantMap>

#include <algorithm>
#include <cstdio>
#include <functional>
#include <memory>

#if defined(__GLIBC__)
#include <malloc.h>
#endif

#include "benchmark/BenchmarkScenarioController.h"
#include "core/AppPreferences.h"
#include "core/ApplicationShutdownCoordinator.h"
#include "core/OfflinePackageInstaller.h"
#include "core/PackageReadinessController.h"
#include "diagnostics/CameraProbeController.h"
#include "diagnostics/SimulatedNavigationController.h"
#include "map/BoundedCameraController.h"
#include "map/DirectoryTileSource.h"
#include "map/MapPackageManifest.h"
#include "map/MapSourceController.h"
#include "map/MbTilesTileSource.h"
#include "perf/PerformanceMonitor.h"
#include "routing/RouteNavigationController.h"
#include "routing/RoutingController.h"
#include "search/PlaceSearchController.h"

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

std::unique_ptr<QQuickView> makeStartupView()
{
    auto view = std::unique_ptr<QQuickView>(new QQuickView);
    view->setTitle(QStringLiteral("QtMaps — đang kiểm tra dữ liệu cục bộ"));
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    auto *component = new QQmlComponent(view->engine(), view.get());
    component->setData(
        "import QtQuick 2.12\nRectangle { width: 460; height: 120; color: '#1c2936'; "
        "Text { anchors.centerIn: parent; color: 'white'; font.pixelSize: 16; "
        "text: 'Đang kiểm tra dữ liệu cục bộ…' } }",
        QUrl());
    if (QObject *item = component->create())
        view->setContent(QUrl(), component, item);
    return view;
}

struct StartupResolution
{
    qtmaps::core::ActiveBundlePaths bundle;
    QString bundleError, graph, graphError, vectorError;
    bool hasBundle = false, hasGraph = false, bundleValid = false, cancelled = false;
    bool startupFailed = false, vectorChecked = false, vectorOk = false;
    bool graphUnverified = false;
};

StartupResolution resolveStartup(const QString &bundleRoot, const QString &graphRoot,
                                 const QString &explicitVector, bool vectorProvided,
                                 bool skipBundle, bool checkGraph, int exitAfterSeconds)
{
    StartupResolution result;
    QProcess bundleResolver;
    bundleResolver.setProgram(QCoreApplication::applicationFilePath());
    bundleResolver.setArguments({QStringLiteral("--internal-resolve-startup"),
                                 QStringLiteral("--bundle-activation-root"), bundleRoot});
    if (vectorProvided)
        bundleResolver.setArguments(bundleResolver.arguments() +
                                    QStringList{QStringLiteral("--map-package"), explicitVector});
    if (skipBundle)
        bundleResolver.setArguments(bundleResolver.arguments() +
                                    QStringList{QStringLiteral("--internal-skip-bundle")});
    QProcess graphResolver;
    if (checkGraph) {
        graphResolver.setProgram(QCoreApplication::applicationFilePath());
        graphResolver.setArguments({QStringLiteral("--internal-resolve-startup"),
                                    QStringLiteral("--internal-routing-only"),
                                    QStringLiteral("--routing-activation-root"), graphRoot});
    }

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    bool timedOut = false, cancelled = false, displayed = false;
    auto startupView = makeStartupView();
    auto finished = [&]() {
        if (bundleResolver.state() == QProcess::NotRunning &&
            (!checkGraph || graphResolver.state() == QProcess::NotRunning))
            loop.quit();
    };
    QObject::connect(startupView.get(), &QWindow::visibleChanged, &loop, [&](bool visible) {
        if (!visible && displayed && loop.isRunning()) {
            cancelled = true;
            loop.quit();
        }
    });
    QObject::connect(&bundleResolver, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                     &loop, finished);
    QObject::connect(&bundleResolver, &QProcess::errorOccurred, &loop, finished);
    if (checkGraph) {
        QObject::connect(&graphResolver, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                         &loop, finished);
        QObject::connect(&graphResolver, &QProcess::errorOccurred, &loop, finished);
    }
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&]() {
        timedOut = true;
        loop.quit();
    });
    if (exitAfterSeconds > 0) {
        QTimer::singleShot(exitAfterSeconds * 1000, &loop, [&]() {
            cancelled = true;
            loop.quit();
        });
    }
    QTimer::singleShot(150, &loop, [&]() {
        if ((bundleResolver.state() != QProcess::NotRunning ||
             (checkGraph && graphResolver.state() != QProcess::NotRunning)) &&
            startupView->rootObject()) {
            displayed = true;
            startupView->show();
        }
    });
    bundleResolver.start();
    if (checkGraph)
        graphResolver.start();
    QTimer::singleShot(0, &loop, finished);
    timeout.start(10000);
    loop.exec();
    timeout.stop();
    startupView->hide();
    const bool bundleDone = bundleResolver.state() == QProcess::NotRunning &&
                            bundleResolver.exitStatus() == QProcess::NormalExit &&
                            bundleResolver.exitCode() == 0;
    const bool graphDone = checkGraph && graphResolver.state() == QProcess::NotRunning &&
                           graphResolver.exitStatus() == QProcess::NormalExit &&
                           graphResolver.exitCode() == 0;
    for (QProcess *process : {&bundleResolver, &graphResolver}) {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            process->waitForFinished(1000);
        }
    }
    if (cancelled) {
        result.cancelled = true;
        return result;
    }

    QJsonObject bundleObject, vectorProgress;
    for (const QByteArray &line : bundleResolver.readAllStandardOutput().split('\n')) {
        const QJsonDocument document = QJsonDocument::fromJson(line);
        if (!document.isObject())
            continue;
        const QJsonObject object = document.object();
        if (object.value(QStringLiteral("phase")) == QStringLiteral("explicit-vector"))
            vectorProgress = object;
        else
            bundleObject = object;
    }
    if (bundleDone && !bundleObject.isEmpty()) {
        result.hasBundle = bundleObject.value("hasBundle").toBool();
        result.bundleValid = bundleObject.value("bundleValid").toBool();
        result.bundleError = bundleObject.value("bundleError").toString();
        result.bundle.vector = bundleObject.value("vector").toString();
        result.bundle.raster = bundleObject.value("raster").toString();
        result.bundle.search = bundleObject.value("search").toString();
        result.bundle.orthography = bundleObject.value("orthography").toString();
        result.vectorChecked = bundleObject.value("vectorChecked").toBool();
        result.vectorOk = bundleObject.value("vectorOk").toBool();
        result.vectorError = bundleObject.value("vectorError").toString();
    } else {
        result.startupFailed = true;
        result.hasBundle = true;
        result.bundleError = timedOut ? QStringLiteral("Startup data check timed out")
                                      : QStringLiteral("Startup data check failed");
        if (vectorProvided && !vectorProgress.isEmpty()) {
            result.vectorChecked = vectorProgress.value("vectorChecked").toBool();
            result.vectorOk = vectorProgress.value("vectorOk").toBool();
            result.vectorError = vectorProgress.value("vectorError").toString();
        }
    }
    if (checkGraph) {
        const QJsonDocument graphDocument =
            QJsonDocument::fromJson(graphResolver.readAllStandardOutput());
        if (graphDone && graphDocument.isObject()) {
            const QJsonObject graph = graphDocument.object();
            result.hasGraph = graph.value("hasGraph").toBool();
            result.graph = graph.value("graph").toString();
            result.graphError = graph.value("graphError").toString();
        } else {
            result.hasGraph = true;
            result.graphUnverified = true;
            result.graphError = timedOut ? QStringLiteral("Routing data check timed out")
                                         : QStringLiteral("Routing data check failed");
        }
    }
    return result;
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
    QCommandLineOption resolveStartupOption(QStringLiteral("internal-resolve-startup"));
    resolveStartupOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(resolveStartupOption);
    QCommandLineOption skipBundleOption(QStringLiteral("internal-skip-bundle"));
    skipBundleOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(skipBundleOption);
    QCommandLineOption routingOnlyOption(QStringLiteral("internal-routing-only"));
    routingOnlyOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(routingOnlyOption);
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
    parser.addOption(
        {QStringLiteral("orthography-package"),
         QStringLiteral("Unit 10 local orthography package directory or current pointer"),
         QStringLiteral("path"), QStringLiteral("data/orthography-packages/current")});
    parser.addOption({QStringLiteral("search-package"),
                      QStringLiteral("Unit 09 offline search package directory or current pointer"),
                      QStringLiteral("path"), QStringLiteral("data/search-packages/current")});
    parser.addOption({QStringLiteral("routing-activation-root"),
                      QStringLiteral("Unit 12 local routing graph activation directory"),
                      QStringLiteral("path"), QStringLiteral("data/routing-activation")});
    parser.addOption({QStringLiteral("bundle-activation-root"),
                      QStringLiteral("Unit 12 local map/search bundle activation directory"),
                      QStringLiteral("path"), QStringLiteral("data/package-activation")});
    parser.addOption(
        {QStringLiteral("install-local-bundle"),
         QStringLiteral("Validate and atomically activate an owner-provided local bundle"),
         QStringLiteral("path")});
    parser.addOption(
        {QStringLiteral("activate-routing-graph"),
         QStringLiteral("Validate and atomically activate a local routing graph contract"),
         QStringLiteral("path")});
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

    // Internal, short-lived helper: the parent can cancel it even if storage I/O stalls.
    if (parser.isSet(QStringLiteral("internal-resolve-startup"))) {
        const QString bundleRoot = parser.value(QStringLiteral("bundle-activation-root"));
        const QString graphRoot = parser.value(QStringLiteral("routing-activation-root"));
        if (parser.isSet(QStringLiteral("internal-routing-only"))) {
            const QFileInfo graphPointer(
                QDir(graphRoot).filePath(QStringLiteral("routing-current.json")));
            const bool hasGraph = graphPointer.exists() || graphPointer.isSymLink();
            QString error, graph;
            if (hasGraph)
                graph = qtmaps::core::OfflinePackageInstaller::activeRoutingGraphPath(graphRoot,
                                                                                      &error);
            const QJsonObject result{
                {"hasGraph", hasGraph}, {"graph", graph}, {"graphError", error}};
            const QByteArray bytes = QJsonDocument(result).toJson(QJsonDocument::Compact);
            return std::fwrite(bytes.constData(), 1, static_cast<size_t>(bytes.size()), stdout) ==
                           static_cast<size_t>(bytes.size())
                       ? 0
                       : 4;
        }
        const bool vectorProvided = parser.isSet(QStringLiteral("map-package"));
        QString vectorPath = parser.value(QStringLiteral("map-package"));
        QString vectorError;
        bool vectorChecked = false;
        bool vectorOk = false;
        if (vectorProvided) {
            vectorChecked = !vectorPath.isEmpty();
            qtmaps::map::MapPackageManifest vectorManifest;
            vectorOk = vectorChecked && qtmaps::map::MapPackageManifest::loadAndValidate(
                                            vectorPath, &vectorManifest, &vectorError);
            const QJsonObject progress{{"phase", "explicit-vector"},
                                       {"vectorChecked", vectorChecked},
                                       {"vectorOk", vectorOk},
                                       {"vectorError", vectorError}};
            const QByteArray progressBytes =
                QJsonDocument(progress).toJson(QJsonDocument::Compact) + '\n';
            if (std::fwrite(progressBytes.constData(), 1, static_cast<size_t>(progressBytes.size()),
                            stdout) != static_cast<size_t>(progressBytes.size()) ||
                std::fflush(stdout) != 0)
                return 4;
        }
        const QFileInfo bundlePointer(QDir(bundleRoot).filePath(QStringLiteral("current")));
        const bool hasBundle = !parser.isSet(QStringLiteral("internal-skip-bundle")) &&
                               (bundlePointer.exists() || bundlePointer.isSymLink());
        qtmaps::core::ActiveBundlePaths paths;
        QString bundleError;
        const bool valid = hasBundle && qtmaps::core::OfflinePackageInstaller::resolveActiveBundle(
                                            bundleRoot, &paths, &bundleError);
        if (!vectorProvided) {
            if (hasBundle)
                vectorPath = valid ? paths.vector : QString();
            vectorChecked = !vectorPath.isEmpty();
            qtmaps::map::MapPackageManifest vectorManifest;
            vectorOk = vectorChecked && qtmaps::map::MapPackageManifest::loadAndValidate(
                                            vectorPath, &vectorManifest, &vectorError);
        }
        const QJsonObject result{{"hasBundle", hasBundle},
                                 {"bundleValid", valid},
                                 {"bundleError", bundleError},
                                 {"vector", paths.vector},
                                 {"raster", paths.raster},
                                 {"search", paths.search},
                                 {"orthography", paths.orthography},
                                 {"vectorChecked", vectorChecked},
                                 {"vectorOk", vectorOk},
                                 {"vectorError", vectorError}};
        const QByteArray bytes = QJsonDocument(result).toJson(QJsonDocument::Compact);
        return std::fwrite(bytes.constData(), 1, static_cast<size_t>(bytes.size()), stdout) ==
                       static_cast<size_t>(bytes.size())
                   ? 0
                   : 4;
    }

    if (parser.isSet(QStringLiteral("install-local-bundle"))) {
        qtmaps::core::MapSearchBundleContract contract;
        QString error;
        const QString path = parser.value(QStringLiteral("install-local-bundle"));
        if (!qtmaps::core::OfflinePackageInstaller::validateCandidate(path, &contract, &error) ||
            !qtmaps::core::OfflinePackageInstaller::activateValidatedBundle(
                path, contract, parser.value(QStringLiteral("bundle-activation-root")), &error)) {
            qCritical().noquote() << "Unit 12 bundle activation failed:" << error;
            return 4;
        }
        qInfo().noquote() << "Unit 12 bundle activated:" << contract.bundleId;
        return 0;
    }
    if (parser.isSet(QStringLiteral("activate-routing-graph"))) {
        qtmaps::core::RoutingGraphActivationContract contract;
        QString error;
        const QString path = parser.value(QStringLiteral("activate-routing-graph"));
        if (!qtmaps::core::OfflinePackageInstaller::validateRoutingGraph(path, &contract, &error) ||
            !qtmaps::core::OfflinePackageInstaller::activateValidatedRoutingGraph(
                path, contract, parser.value(QStringLiteral("routing-activation-root")), &error)) {
            qCritical().noquote() << "Unit 12 routing graph activation failed:" << error;
            return 4;
        }
        qInfo().noquote() << "Unit 12 routing graph activated:" << contract.graphId;
        return 0;
    }

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
    if (qEnvironmentVariableIsSet("QTMAPS_ASSERT_SOFTWARE_BACKEND")) {
        const QString quickBackend = qEnvironmentVariable("QT_QUICK_BACKEND");
        qInfo().noquote()
            << QStringLiteral("QTMAPS_CTEST_BACKEND QT_QUICK_BACKEND=%1").arg(quickBackend);
        if (quickBackend != QStringLiteral("software")) {
            qCritical() << "QTMAPS_ASSERT_SOFTWARE_BACKEND requires QT_QUICK_BACKEND=software";
            return 2;
        }
    }

    const QStringList providers = QGeoServiceProvider::availableServiceProviders();
    qInfo().noquote() << "availableServiceProviders=" << providers.join(QLatin1Char(','));
    if (useVectorProbe || useUnit03LocalMap) {
        qInfo() << "providerConstructionProbe=skipped for localhost-only vector mode";
    } else {
        probeProvider(QStringLiteral("osm"));
        probeProvider(QStringLiteral("mapboxgl"));
    }

    qtmaps::core::ActiveBundlePaths activeBundle;
    QString bundleError, activeGraph, graphError;
    const QString bundleRoot = parser.value(QStringLiteral("bundle-activation-root"));
    const QString graphRoot = parser.value(QStringLiteral("routing-activation-root"));
    bool hasBundle = false, hasGraph = false, bundleValid = false;
    bool startupFailed = false, vectorChecked = false, vectorOk = false;
    bool graphUnverified = false;
    QString vectorError;
    if (useUnit03LocalMap) {
        const int exitAfter = parser.isSet(QStringLiteral("exit-after"))
                                  ? std::max(1, parser.value(QStringLiteral("exit-after")).toInt())
                                  : 0;
        const bool allComponentsProvided = parser.isSet(QStringLiteral("map-package")) &&
                                           parser.isSet(QStringLiteral("raster-package")) &&
                                           parser.isSet(QStringLiteral("search-package")) &&
                                           parser.isSet(QStringLiteral("orthography-package"));
        const StartupResolution resolved = resolveStartup(
            bundleRoot, graphRoot, parser.value(QStringLiteral("map-package")),
            parser.isSet(QStringLiteral("map-package")), allComponentsProvided,
            useRoutingDemo && !parser.isSet(QStringLiteral("routing-fixture-only")), exitAfter);
        if (resolved.cancelled)
            return 0;
        activeBundle = resolved.bundle;
        bundleError = resolved.bundleError;
        activeGraph = resolved.graph;
        graphError = resolved.graphError;
        graphUnverified = resolved.graphUnverified;
        hasBundle = resolved.hasBundle;
        hasGraph = resolved.hasGraph;
        bundleValid = resolved.bundleValid;
        startupFailed = resolved.startupFailed;
        vectorChecked = resolved.vectorChecked;
        vectorOk = resolved.vectorOk;
        vectorError = resolved.vectorError;
    }
    if (hasBundle && !bundleValid)
        qWarning().noquote() << "Active bundle is invalid:" << bundleError;
    if (hasGraph && activeGraph.isEmpty())
        qWarning().noquote() << (graphUnverified ? "Routing graph check unavailable:"
                                                 : "Active routing graph is invalid:")
                             << graphError;
    std::unique_ptr<QTemporaryDir> unavailableBundle;
    if ((hasBundle && !bundleValid) || startupFailed || (vectorChecked && !vectorOk))
        unavailableBundle.reset(new QTemporaryDir);
    const auto packagePath = [&](const QString &option, const QString &resolved) {
        if (parser.isSet(option) || !hasBundle)
            return parser.value(option);
        // A corrupt active bundle must not silently select an unrelated legacy package.
        return bundleValid
                   ? resolved
                   : (unavailableBundle && unavailableBundle->isValid() ? unavailableBundle->path()
                                                                        : QString());
    };
    const QString vectorPackage = packagePath(QStringLiteral("map-package"), activeBundle.vector);
    const QString rasterPackage =
        packagePath(QStringLiteral("raster-package"), activeBundle.raster);
    const QString searchPackage =
        packagePath(QStringLiteral("search-package"), activeBundle.search);
    const QString orthographyPackage =
        packagePath(QStringLiteral("orthography-package"), activeBundle.orthography);
    if (hasBundle)
        qInfo().noquote() << "Runtime packages: vector=" << vectorPackage
                          << "raster=" << rasterPackage << "search=" << searchPackage
                          << "orthography="
                          << (orthographyPackage.isEmpty() ? "<none>" : orthographyPackage);
    qtmaps::diagnostics::CameraProbeController cameraProbeController;
    qtmaps::map::BoundedCameraController boundedCameraController;
    qtmaps::map::MbTilesTileSource vectorTileSource;
    qtmaps::map::DirectoryTileSource diagnosticRasterSource;
    qtmaps::map::MapSourceController mapSourceController(&vectorTileSource,
                                                         &diagnosticRasterSource);
    mapSourceController.setRasterPackagePath(rasterPackage);
    mapSourceController.setOrthographyPackagePath(orthographyPackage);
    qtmaps::diagnostics::SimulatedNavigationController navigationController;
    qtmaps::routing::RoutingController routingController;
    qtmaps::search::PlaceSearchController placeSearchController(searchPackage);
    qtmaps::routing::RouteNavigationController routeNavigationController;
    qtmaps::core::ApplicationShutdownCoordinator shutdownCoordinator(&routingController);
    qtmaps::perf::PerformanceMonitor performanceMonitor;
    qtmaps::benchmark::BenchmarkScenarioController benchmarkScenarioController(
        &performanceMonitor, &routingController, &routeNavigationController,
        launchTimer.msecsSinceReference());
    QQmlApplicationEngine engine;
    qtmaps::core::AppPreferences appPreferences(&engine);
    qtmaps::core::PackageReadinessController packageReadiness(
        vectorPackage, rasterPackage, searchPackage, orthographyPackage, nullptr,
        QString()); // Routing state comes from the isolated startup resolver.

    const bool verifiedVectorOverride =
        parser.isSet(QStringLiteral("map-package")) && vectorChecked && vectorOk;
    const QString vectorStartupPackage =
        ((startupFailed && !verifiedVectorOverride) || (vectorChecked && !vectorOk))
            ? (unavailableBundle && unavailableBundle->isValid() ? unavailableBundle->path()
                                                                 : QString())
            : vectorPackage;
    if (vectorChecked && !vectorOk)
        qWarning().noquote() << "Vector package is invalid:" << vectorError;
    if (useUnit03LocalMap) {
        auto startupView = makeStartupView();
        QTimer::singleShot(150, startupView.get(), [&]() {
            if (startupView->rootObject())
                startupView->show();
        });
        const bool started = mapSourceController.start(
            vectorStartupPackage, parser.value(QStringLiteral("diagnostic-raster-root")),
            parser.isSet(QStringLiteral("force-vector-failure")));
        startupView->hide();
        if (!started) {
            qCritical().noquote() << "Unit 03 map startup failed:"
                                  << mapSourceController.fallbackReason();
            return 3;
        }
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
    engine.rootContext()->setContextProperty(QStringLiteral("appPreferences"), &appPreferences);
    engine.rootContext()->setContextProperty(QStringLiteral("packageReadiness"), &packageReadiness);
    engine.rootContext()->setContextProperty(QStringLiteral("cameraProbeController"),
                                             &cameraProbeController);
    engine.rootContext()->setContextProperty(QStringLiteral("boundedCameraController"),
                                             &boundedCameraController);
    engine.rootContext()->setContextProperty(QStringLiteral("navigationController"),
                                             &navigationController);
    engine.rootContext()->setContextProperty(QStringLiteral("routingController"),
                                             &routingController);
    engine.rootContext()->setContextProperty(QStringLiteral("placeSearchController"),
                                             &placeSearchController);
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
    QTemporaryDir runtimeRoutingDirectory;
    const QString originalConfig =
        QDir::current().absoluteFilePath(parser.value(QStringLiteral("valhalla-config")));
    QString selectedConfig = originalConfig;
    bool graphSelected = false;
    QString routingState = graphUnverified         ? QStringLiteral("unverified")
                           : !hasGraph             ? QStringLiteral("missing")
                           : activeGraph.isEmpty() ? QStringLiteral("invalid")
                                                   : QStringLiteral("installed");
    const bool launchValhalla =
        unit03ProductMap && useRoutingDemo && !parser.isSet(QStringLiteral("routing-fixture-only"));
    bool canLaunch = launchValhalla;
    if (launchValhalla && !activeGraph.isEmpty()) {
        if (parser.isSet(QStringLiteral("valhalla-config"))) {
            graphSelected = qtmaps::core::OfflinePackageInstaller::routingConfigSelectsGraph(
                originalConfig, activeGraph, &graphError);
        } else if (runtimeRoutingDirectory.isValid()) {
            selectedConfig = runtimeRoutingDirectory.filePath(QStringLiteral("valhalla.json"));
            graphSelected = qtmaps::core::OfflinePackageInstaller::writeRoutingRuntimeConfig(
                originalConfig, selectedConfig, activeGraph, static_cast<quint16>(valhallaPort),
                &graphError);
            canLaunch = graphSelected;
        } else {
            graphError = QStringLiteral("Cannot create private runtime configuration directory");
            canLaunch = false;
        }
        if (!graphSelected)
            qWarning().noquote() << "Routing graph installed but not selected:" << graphError;
        else
            routingState = QStringLiteral("unavailable");
    } else if (launchValhalla && hasGraph && activeGraph.isEmpty() &&
               !parser.isSet(QStringLiteral("valhalla-config"))) {
        canLaunch = false;
    }
    if (launchValhalla)
        packageReadiness.setRoutingState(routingState);
    QObject::connect(&routingController, &qtmaps::routing::RoutingController::stateChanged,
                     &packageReadiness, [&]() {
                         if (!graphSelected)
                             return;
                         const bool ready = routingController.backendState() ==
                                            qtmaps::routing::ValhallaServiceManager::State::Ready;
                         packageReadiness.setRoutingState(ready ? QStringLiteral("selected")
                                                                : QStringLiteral("unavailable"));
                     });
    if (canLaunch) {
        const QDir workingDirectory = QDir::current();
        qInfo().noquote() << "Routing runtime config:" << selectedConfig
                          << "active graph:" << (activeGraph.isEmpty() ? "<none>" : activeGraph)
                          << "selected:" << graphSelected;
        routingController.startBackend(
            workingDirectory.absoluteFilePath(parser.value(QStringLiteral("valhalla-service"))),
            selectedConfig, static_cast<quint16>(valhallaPort));
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
