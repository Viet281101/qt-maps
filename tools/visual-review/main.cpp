#include <QCommandLineParser>
#include <QDir>
#include <QGeoCoordinate>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>

#include <functional>

#include "benchmark/BenchmarkScenarioController.h"
#include "core/ApplicationShutdownCoordinator.h"
#include "map/BoundedCameraController.h"
#include "map/DirectoryTileSource.h"
#include "map/MapSourceController.h"
#include "map/MbTilesTileSource.h"
#include "perf/PerformanceMonitor.h"
#include "routing/RouteNavigationController.h"
#include "routing/RoutingController.h"

namespace {

class VisualReviewController final : public QObject
{
public:
    VisualReviewController(QQuickWindow *window, QObject *root,
                           qtmaps::map::MapSourceController *mapSource,
                           qtmaps::routing::RouteNavigationController *navigation,
                           const QString &outputDirectory, int settleMilliseconds,
                           QObject *parent = nullptr) :
        QObject(parent), window_(window), root_(root), mapSource_(mapSource),
        navigation_(navigation), outputDirectory_(outputDirectory),
        settleMilliseconds_(settleMilliseconds)
    {}

    void start()
    {
        const QDir output(outputDirectory_);
        if (output.exists() &&
            !output.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty()) {
            fail(QStringLiteral("Visual-review output must be empty: %1").arg(outputDirectory_));
            return;
        }
        if (!QDir().mkpath(outputDirectory_)) {
            fail(QStringLiteral("Cannot create visual-review output: %1").arg(outputDirectory_));
            return;
        }
        waitForMap([this](QObject *map) { captureVector(map); });
    }

private:
    void waitForMap(const std::function<void(QObject *)> &next)
    {
        QObject *map = root_->findChild<QObject *>(QStringLiteral("routingMap"));
        if (map && map->property("mapReady").toBool()) {
            QTimer::singleShot(settleMilliseconds_, this, [next, map]() { next(map); });
            return;
        }
        if (++mapReadyAttempts_ > 100) {
            fail(QStringLiteral("Timed out waiting for the production map surface"));
            return;
        }
        QTimer::singleShot(50, this, [this, next]() { waitForMap(next); });
    }

    void capture(const QString &name, const std::function<void()> &next)
    {
        QTimer::singleShot(settleMilliseconds_, this, [this, name, next]() {
            const QImage image = window_->grabWindow();
            const QString output =
                outputDirectory_ + QLatin1Char('/') + name + QStringLiteral(".png");
            if (image.isNull() || !image.save(output, "PNG")) {
                fail(QStringLiteral("Cannot capture visual-review frame: %1").arg(output));
                return;
            }
            qInfo().noquote() << QStringLiteral("visual_review_capture=%1 size=%2x%3")
                                     .arg(output)
                                     .arg(image.width())
                                     .arg(image.height());
            next();
        });
    }

    void resize(const QSize &size)
    {
        window_->resize(size);
        window_->show();
    }

    void setCamera(QObject *map, const QGeoCoordinate &center, double zoom, double bearing)
    {
        map->setProperty("center", QVariant::fromValue(center));
        map->setProperty("zoomLevel", zoom);
        map->setProperty("bearing", bearing);
    }

    void captureVector(QObject *map)
    {
        resize(QSize(900, 580));
        capture(QStringLiteral("vector-route-900x580"), [this, map]() {
            resize(QSize(1366, 768));
            setCamera(map, QGeoCoordinate(21.0245, 105.8382), 15.0, 0.0);
            capture(QStringLiteral("vector-pan-zoom-1366x768"), [this, map]() {
                setCamera(map, QGeoCoordinate(21.0245, 105.8382), 15.0, 42.0);
                capture(QStringLiteral("vector-rotation-1366x768"), [this, map]() {
                    QMetaObject::invokeMethod(root_, "fitRouteWithPanelPadding");
                    capture(QStringLiteral("vector-route-fit-1366x768"), [this]() {
                        navigation_->start();
                        navigation_->advanceBySeconds(75.0);
                        QMetaObject::invokeMethod(root_, "recenterNavigation");
                        capture(QStringLiteral("vector-navigation-recenter-1366x768"), [this]() {
                            resize(QSize(1920, 1080));
                            capture(QStringLiteral("vector-route-1920x1080"), [this]() {
                                mapReadyAttempts_ = 0;
                                mapSource_->activateRaster();
                                waitForRaster();
                            });
                        });
                    });
                });
            });
        });
    }

    void waitForRaster()
    {
        if (mapSource_->layerMode() == QLatin1String("raster")) {
            mapReadyAttempts_ = 0;
            waitForMap([this](QObject *map) { captureRaster(map); });
            return;
        }
        if (mapSource_->layerMode() == QLatin1String("error") || ++mapReadyAttempts_ > 100) {
            fail(QStringLiteral("Timed out switching to the validated raster package"));
            return;
        }
        QTimer::singleShot(50, this, [this]() { waitForRaster(); });
    }

    void captureRaster(QObject *map)
    {
        navigation_->stop();
        resize(QSize(900, 580));
        QMetaObject::invokeMethod(root_, "fitRouteWithPanelPadding");
        capture(QStringLiteral("raster-route-900x580"), [this, map]() {
            resize(QSize(1366, 768));
            setCamera(map, QGeoCoordinate(21.0245, 105.8382), 15.0, 0.0);
            capture(QStringLiteral("raster-pan-zoom-1366x768"), [this, map]() {
                setCamera(map, QGeoCoordinate(21.0245, 105.8382), 15.0, 42.0);
                capture(QStringLiteral("raster-rotation-1366x768"), [this]() {
                    QMetaObject::invokeMethod(root_, "fitRouteWithPanelPadding");
                    capture(QStringLiteral("raster-route-fit-1366x768"), [this]() {
                        navigation_->start();
                        navigation_->advanceBySeconds(75.0);
                        QMetaObject::invokeMethod(root_, "recenterNavigation");
                        capture(QStringLiteral("raster-navigation-recenter-1366x768"), [this]() {
                            resize(QSize(1920, 1080));
                            capture(QStringLiteral("raster-route-1920x1080"), [this]() {
                                qInfo() << "visual_review_result=captured";
                                QCoreApplication::exit(0);
                            });
                        });
                    });
                });
            });
        });
    }

    void fail(const QString &message)
    {
        qCritical().noquote() << message;
        QCoreApplication::exit(1);
    }

    QQuickWindow *window_ = nullptr;
    QObject *root_ = nullptr;
    qtmaps::map::MapSourceController *mapSource_ = nullptr;
    qtmaps::routing::RouteNavigationController *navigation_ = nullptr;
    QString outputDirectory_;
    int settleMilliseconds_ = 500;
    int mapReadyAttempts_ = 0;
};

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    application.setQuitOnLastWindowClosed(false);
    QCoreApplication::setApplicationName(QStringLiteral("QtMaps visual review"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Local-only QtMaps GPU visual-review capture"));
    parser.addHelpOption();
    parser.addOption({QStringLiteral("map-package"), QStringLiteral("Validated vector package"),
                      QStringLiteral("path"), QStringLiteral("data/packages/current")});
    parser.addOption({QStringLiteral("raster-package"), QStringLiteral("Validated raster package"),
                      QStringLiteral("path"), QStringLiteral("data/raster-packages/current")});
    parser.addOption({QStringLiteral("output"), QStringLiteral("Empty PNG output directory"),
                      QStringLiteral("path")});
    parser.addOption({QStringLiteral("settle-ms"),
                      QStringLiteral("Render settle interval before each capture"),
                      QStringLiteral("milliseconds"), QStringLiteral("2500")});
    parser.process(application);

    const QString outputDirectory = parser.value(QStringLiteral("output"));
    bool settleOk = false;
    const int settleMilliseconds = parser.value(QStringLiteral("settle-ms")).toInt(&settleOk);
    if (outputDirectory.isEmpty() || !settleOk || settleMilliseconds < 250 ||
        settleMilliseconds > 5000) {
        qCritical() << "--output is required and --settle-ms must be between 250 and 5000";
        return 2;
    }

    qtmaps::map::MbTilesTileSource vectorTileSource;
    qtmaps::map::DirectoryTileSource diagnosticRasterSource;
    qtmaps::map::MapSourceController mapSource(&vectorTileSource, &diagnosticRasterSource);
    mapSource.setRasterPackagePath(parser.value(QStringLiteral("raster-package")));
    if (!mapSource.start(parser.value(QStringLiteral("map-package")), QString())) {
        qCritical().noquote() << mapSource.fallbackReason();
        return 3;
    }

    qtmaps::routing::RoutingController routing;
    qtmaps::routing::RouteNavigationController navigation;
    qtmaps::core::ApplicationShutdownCoordinator shutdown(&routing);
    qtmaps::perf::PerformanceMonitor performance;
    qtmaps::map::BoundedCameraController boundedCamera;
    qtmaps::benchmark::BenchmarkScenarioController benchmark(&performance, &routing, &navigation,
                                                             0);
    QString benchmarkError;
    if (!benchmark.configure(QString(), 1, 0, true, &benchmarkError)) {
        qCritical().noquote() << benchmarkError;
        return 4;
    }
    QObject::connect(&routing, &qtmaps::routing::RoutingController::routeChanged, &navigation,
                     [&routing, &navigation]() {
                         if (routing.routeAvailable()) {
                             navigation.setRoute(routing.route(), routing.sourceLabel());
                         } else {
                             navigation.clearRoute();
                         }
                     });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("performanceMonitor"), &performance);
    engine.rootContext()->setContextProperty(QStringLiteral("boundedCameraController"),
                                             &boundedCamera);
    engine.rootContext()->setContextProperty(QStringLiteral("routingController"), &routing);
    engine.rootContext()->setContextProperty(QStringLiteral("mapSourceController"), &mapSource);
    engine.rootContext()->setContextProperty(QStringLiteral("routeNavigationController"),
                                             &navigation);
    engine.rootContext()->setContextProperty(QStringLiteral("shutdownCoordinator"), &shutdown);
    engine.rootContext()->setContextProperty(QStringLiteral("benchmarkScenarioController"),
                                             &benchmark);
    engine.rootContext()->setContextProperty(QStringLiteral("routingFixtureOnly"), true);
    engine.rootContext()->setContextProperty(QStringLiteral("instrumentationEnabled"), false);
    engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"),
                                             mapSource.activeStyleUrl());
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return 5;
    }

    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (!window) {
        return 5;
    }
    benchmark.attachWindow(window);
    if (!routing.routeAvailable()) {
        routing.calculateFixtureRoute();
    }

    VisualReviewController review(window, engine.rootObjects().constFirst(), &mapSource,
                                  &navigation, QDir::cleanPath(outputDirectory), settleMilliseconds,
                                  &application);
    QTimer::singleShot(0, &review, [&review]() { review.start(); });
    return application.exec();
}
