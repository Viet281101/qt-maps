#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QGeoCoordinate>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <QtMath>

#include <cmath>

#include "map/MapPackageContract.h"
#include "map/MbTilesTileSource.h"

namespace {

struct TileCoordinate
{
    int zoom = 0;
    int x = 0;
    int y = 0;
};

int longitudeToX(double longitude, int zoom)
{
    return static_cast<int>(
        std::floor((longitude + 180.0) / 360.0 * static_cast<double>(qint64(1) << zoom)));
}

int latitudeToY(double latitude, int zoom)
{
    const double radians = qDegreesToRadians(latitude);
    return static_cast<int>(std::floor((1.0 - std::asinh(std::tan(radians)) / M_PI) / 2.0 *
                                       static_cast<double>(qint64(1) << zoom)));
}

QGeoCoordinate tileCenter(const TileCoordinate &tile)
{
    const double count = static_cast<double>(qint64(1) << tile.zoom);
    const double longitude = (static_cast<double>(tile.x) + 0.5) / count * 360.0 - 180.0;
    const double mercator = M_PI * (1.0 - 2.0 * (static_cast<double>(tile.y) + 0.5) / count);
    const double latitude = qRadiansToDegrees(std::atan(std::sinh(mercator)));
    return QGeoCoordinate(latitude, longitude);
}

QVector<TileCoordinate> contractedTiles()
{
    QVector<TileCoordinate> tiles;
    for (int zoom = qtmaps::map::contract::kRasterMinimumZoom;
         zoom <= qtmaps::map::contract::kRasterMaximumZoom; ++zoom) {
        const int minimumX = longitudeToX(qtmaps::map::contract::kWest, zoom);
        const int maximumX = longitudeToX(qtmaps::map::contract::kEast, zoom);
        const int minimumY = latitudeToY(qtmaps::map::contract::kNorth, zoom);
        const int maximumY = latitudeToY(qtmaps::map::contract::kSouth, zoom);
        for (int x = minimumX; x <= maximumX; ++x) {
            for (int y = minimumY; y <= maximumY; ++y) {
                tiles.append(TileCoordinate{zoom, x, y});
            }
        }
    }
    return tiles;
}

} // namespace

class RasterRenderController final : public QObject
{
    Q_OBJECT

public:
    RasterRenderController(const QString &outputPath, int settleMilliseconds,
                           QObject *parent = nullptr) :
        QObject(parent), outputRoot_(outputPath), settleMilliseconds_(settleMilliseconds),
        tiles_(contractedTiles())
    {
        settleTimer_.setSingleShot(true);
        settleTimer_.setInterval(settleMilliseconds_);
        connect(&settleTimer_, &QTimer::timeout, this, &RasterRenderController::captureCurrent);
        elapsed_.start();
    }

    void attachWindow(QQuickWindow *window)
    {
        window_ = window;
        connect(window_, &QQuickWindow::frameSwapped, this, [this]() {
            if (rendering_) {
                settleTimer_.start();
            }
        });
    }

public slots:
    void mapReady(QObject *map)
    {
        if (map_ || !map) {
            return;
        }
        map_ = map;
        QTimer::singleShot(0, this, &RasterRenderController::renderNext);
    }

signals:
    void completed(bool success);

private slots:
    void renderNext()
    {
        if (index_ >= tiles_.size()) {
            qInfo().noquote() << QStringLiteral("rendered_tiles=%1 duration_seconds=%2")
                                     .arg(index_)
                                     .arg(elapsed_.elapsed() / 1000.0, 0, 'f', 3);
            emit completed(true);
            return;
        }
        current_ = tiles_.at(index_);
        map_->setProperty("zoomLevel", current_.zoom);
        map_->setProperty("center", QVariant::fromValue(tileCenter(current_)));
        rendering_ = true;
        settleTimer_.start();
    }

    void captureCurrent()
    {
        if (!window_ || !rendering_) {
            return;
        }
        rendering_ = false;
        const QImage image = window_->grabWindow();
        if (image.size() != QSize(256, 256)) {
            qCritical() << "Unexpected raster tile image size" << image.size();
            emit completed(false);
            return;
        }
        const QString directory =
            outputRoot_ + QStringLiteral("/%1/%2").arg(current_.zoom).arg(current_.x);
        if (!QDir().mkpath(directory)) {
            qCritical().noquote()
                << QStringLiteral("Cannot create raster directory: %1").arg(directory);
            emit completed(false);
            return;
        }
        const QString output = directory + QStringLiteral("/%1.png").arg(current_.y);
        if (QFileInfo::exists(output) || !image.save(output, "PNG")) {
            qCritical().noquote() << QStringLiteral("Cannot write raster tile: %1").arg(output);
            emit completed(false);
            return;
        }
        ++index_;
        if (index_ % 100 == 0 || index_ == tiles_.size()) {
            qInfo() << "raster render progress" << index_ << "/" << tiles_.size();
        }
        QTimer::singleShot(0, this, &RasterRenderController::renderNext);
    }

private:
    QString outputRoot_;
    int settleMilliseconds_ = 150;
    QVector<TileCoordinate> tiles_;
    int index_ = 0;
    TileCoordinate current_;
    QQuickWindow *window_ = nullptr;
    QObject *map_ = nullptr;
    QTimer settleTimer_;
    QElapsedTimer elapsed_;
    bool rendering_ = false;
};

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("QtMaps raster renderer"));
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({QStringLiteral("vector-package"), QStringLiteral("Validated vector package"),
                      QStringLiteral("path"), QStringLiteral("data/packages/current")});
    parser.addOption({QStringLiteral("output"), QStringLiteral("Empty z/x/y.png output root"),
                      QStringLiteral("path"), QStringLiteral("generated/unit-07c/rendered-tiles")});
    parser.addOption({QStringLiteral("settle-ms"),
                      QStringLiteral("Quiet frame interval before each capture"),
                      QStringLiteral("milliseconds"), QStringLiteral("150")});
    parser.process(application);

    bool settleOk = false;
    const int settleMilliseconds = parser.value(QStringLiteral("settle-ms")).toInt(&settleOk);
    if (!settleOk || settleMilliseconds < 50 || settleMilliseconds > 2000) {
        qCritical() << "--settle-ms must be between 50 and 2000";
        return 2;
    }
    const QString output = QDir::cleanPath(parser.value(QStringLiteral("output")));
    const QDir outputDirectory(output);
    if (outputDirectory.exists() &&
        !outputDirectory.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty()) {
        qCritical().noquote() << QStringLiteral("Raster output must be empty: %1").arg(output);
        return 2;
    }
    if (!QDir().mkpath(output)) {
        qCritical().noquote() << QStringLiteral("Cannot create raster output: %1").arg(output);
        return 2;
    }

    qtmaps::map::MbTilesTileSource vectorSource;
    if (!vectorSource.start(parser.value(QStringLiteral("vector-package")))) {
        qCritical().noquote() << vectorSource.errorMessage();
        return 3;
    }
    RasterRenderController controller(output, settleMilliseconds);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("vectorStyleUrl"),
                                             vectorSource.baseUrl() +
                                                 QStringLiteral("/style/style.json"));
    engine.rootContext()->setContextProperty(QStringLiteral("rasterRenderController"), &controller);
    engine.load(QUrl(QStringLiteral("qrc:/raster-renderer/RasterRenderer.qml")));
    if (engine.rootObjects().isEmpty()) {
        return 4;
    }
    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (!window) {
        return 4;
    }
    controller.attachWindow(window);
    int result = 5;
    QObject::connect(&controller, &RasterRenderController::completed, &application,
                     [&application, &result](bool success) {
                         result = success ? 0 : 5;
                         application.quit();
                     });
    application.exec();
    vectorSource.stop();
    return result;
}

#include "main.moc"
