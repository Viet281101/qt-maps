#include "RasterTileSource.h"

#include "EncodedTileCache.h"
#include "MapPackageContract.h"
#include "RasterPackageManifest.h"

#include <QDir>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QUrl>
#include <QVariantMap>

#include <memory>

namespace qtmaps {
namespace map {

class RasterTileSourceWorker final : public QObject
{
    Q_OBJECT

public slots:
    void start(const QString &packagePath, quint16 requestedPort)
    {
        stopResources();
        QVariantMap result;
        RasterPackageManifest manifest;
        QString validationError;
        if (!RasterPackageManifest::loadAndValidate(packagePath, &manifest, &validationError)) {
            result.insert(QStringLiteral("error"), validationError);
            emit startFinished(result);
            return;
        }
        root_ = QDir(packagePath);
        connectionName_ =
            QStringLiteral("qtmaps-raster-%1").arg(reinterpret_cast<quintptr>(this));
        database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
        database_.setDatabaseName(root_.filePath(manifest.mbtilesRelativePath));
        database_.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        if (!database_.open()) {
            result.insert(QStringLiteral("error"),
                          QStringLiteral("Cannot open validated raster MBTiles: %1")
                              .arg(database_.lastError().text()));
            stopResources();
            emit startFinished(result);
            return;
        }
        QSqlQuery probe(database_);
        if (!probe.exec(QStringLiteral("SELECT COUNT(*) FROM tiles")) || !probe.next() ||
            probe.value(0).toInt() != contract::kRasterTileCount) {
            result.insert(QStringLiteral("error"),
                          QStringLiteral("Raster MBTiles does not contain the contracted 847 tiles"));
            stopResources();
            emit startFinished(result);
            return;
        }
        cache_.reset(new EncodedTileCache(RasterTileSource::kCacheMaximumBytes));
        server_ = new QTcpServer(this);
        connect(server_, &QTcpServer::newConnection, this,
                &RasterTileSourceWorker::acceptConnections);
        if (!server_->listen(QHostAddress::LocalHost, requestedPort)) {
            result.insert(QStringLiteral("error"),
                          QStringLiteral("Cannot bind raster service to 127.0.0.1:%1: %2")
                              .arg(requestedPort)
                              .arg(server_->errorString()));
            stopResources();
            emit startFinished(result);
            return;
        }
        baseUrl_ = QStringLiteral("http://127.0.0.1:%1").arg(server_->serverPort());
        result.insert(QStringLiteral("running"), true);
        result.insert(QStringLiteral("base_url"), baseUrl_);
        emit startFinished(result);
    }

    void stop()
    {
        stopResources();
        emit stopFinished();
    }

signals:
    void startFinished(const QVariantMap &result);
    void stopFinished();

private slots:
    void acceptConnections()
    {
        while (server_ && server_->hasPendingConnections()) {
            QTcpSocket *socket = server_->nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this,
                    [this, socket]() { handleRequest(socket); });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    }

private:
    void closeDatabase()
    {
        if (connectionName_.isEmpty()) {
            return;
        }
        database_.close();
        database_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName_);
        connectionName_.clear();
    }

    void stopResources()
    {
        if (server_) {
            server_->close();
            const auto sockets = server_->findChildren<QTcpSocket *>();
            for (QTcpSocket *socket : sockets) {
                socket->abort();
            }
            delete server_;
            server_ = nullptr;
        }
        closeDatabase();
        cache_.reset();
        root_ = QDir();
        baseUrl_.clear();
    }

    void writeResponse(QTcpSocket *socket, int status, const QByteArray &contentType,
                       const QByteArray &body)
    {
        const QByteArray reason = status == 200   ? QByteArrayLiteral("OK")
                                  : status == 404 ? QByteArrayLiteral("Not Found")
                                                  : QByteArrayLiteral("Bad Request");
        QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + " " + reason + "\r\n";
        response += "Content-Type: " + contentType + "\r\n";
        response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
        response += "Cache-Control: no-store\r\nConnection: close\r\n\r\n";
        response += body;
        socket->write(response);
        socket->disconnectFromHost();
    }

    QByteArray styleJson() const
    {
        QJsonObject source;
        source.insert(QStringLiteral("type"), QStringLiteral("raster"));
        source.insert(QStringLiteral("tiles"),
                      QJsonArray{baseUrl_ + QStringLiteral("/tiles/{z}/{x}/{y}.png")});
        source.insert(QStringLiteral("tileSize"), 256);
        source.insert(QStringLiteral("minzoom"), contract::kRasterMinimumZoom);
        source.insert(QStringLiteral("maxzoom"), contract::kRasterMaximumZoom);
        source.insert(QStringLiteral("attribution"), contract::attribution());
        QJsonObject sources;
        sources.insert(QStringLiteral("qtmaps-raster"), source);
        QJsonObject layer;
        layer.insert(QStringLiteral("id"), QStringLiteral("qtmaps-raster"));
        layer.insert(QStringLiteral("type"), QStringLiteral("raster"));
        layer.insert(QStringLiteral("source"), QStringLiteral("qtmaps-raster"));
        QJsonObject style;
        style.insert(QStringLiteral("version"), 8);
        style.insert(QStringLiteral("name"), QStringLiteral("QtMaps raster ngoại tuyến"));
        style.insert(QStringLiteral("sources"), sources);
        style.insert(QStringLiteral("layers"), QJsonArray{layer});
        return QJsonDocument(style).toJson(QJsonDocument::Compact);
    }

    void handleRequest(QTcpSocket *socket)
    {
        const QByteArray request =
            socket->property("requestBuffer").toByteArray() + socket->readAll();
        if (!request.contains("\r\n\r\n")) {
            if (request.size() > 8192) {
                writeResponse(socket, 400, QByteArrayLiteral("text/plain"),
                              QByteArrayLiteral("request headers too large\n"));
            } else {
                socket->setProperty("requestBuffer", request);
            }
            return;
        }
        const QList<QByteArray> requestLine = request.left(request.indexOf("\r\n")).split(' ');
        if (requestLine.size() < 2 || requestLine.at(0) != QByteArrayLiteral("GET")) {
            writeResponse(socket, 400, QByteArrayLiteral("text/plain"),
                          QByteArrayLiteral("Only GET is supported\n"));
            return;
        }
        const QString path = QUrl::fromEncoded(requestLine.at(1)).path();
        if (path == QStringLiteral("/healthz")) {
            writeResponse(socket, 200, QByteArrayLiteral("text/plain"), QByteArrayLiteral("ok\n"));
            return;
        }
        if (path == QStringLiteral("/style/style.json")) {
            writeResponse(socket, 200, QByteArrayLiteral("application/json"), styleJson());
            return;
        }
        const QRegularExpression tilePattern(QStringLiteral("^/tiles/(\\d+)/(\\d+)/(\\d+)\\.png$"));
        const QRegularExpressionMatch match = tilePattern.match(path);
        if (match.hasMatch()) {
            const int zoom = match.captured(1).toInt();
            const int x = match.captured(2).toInt();
            const int xyzY = match.captured(3).toInt();
            const QString cacheKey = QStringLiteral("%1/%2/%3").arg(zoom).arg(x).arg(xyzY);
            QByteArray tile;
            if (cache_ && cache_->find(cacheKey, &tile)) {
                writeResponse(socket, 200, QByteArrayLiteral("image/png"), tile);
                return;
            }
            if (zoom >= contract::kRasterMinimumZoom && zoom <= contract::kRasterMaximumZoom) {
                const qint64 tmsY = (qint64(1) << zoom) - 1 - xyzY;
                QSqlQuery query(database_);
                query.prepare(QStringLiteral("SELECT tile_data FROM tiles WHERE zoom_level=? AND "
                                             "tile_column=? AND tile_row=?"));
                query.addBindValue(zoom);
                query.addBindValue(x);
                query.addBindValue(tmsY);
                if (query.exec() && query.next()) {
                    tile = query.value(0).toByteArray();
                    if (cache_) {
                        cache_->insert(cacheKey, tile);
                    }
                    writeResponse(socket, 200, QByteArrayLiteral("image/png"), tile);
                    return;
                }
            }
        }
        writeResponse(socket, 404, QByteArrayLiteral("text/plain"),
                      QByteArrayLiteral("not found\n"));
    }

    QTcpServer *server_ = nullptr;
    QSqlDatabase database_;
    QString connectionName_;
    QDir root_;
    QString baseUrl_;
    std::unique_ptr<EncodedTileCache> cache_;
};

RasterTileSource::RasterTileSource(QObject *parent) : QObject(parent) {}

RasterTileSource::~RasterTileSource()
{
    if (workerThread_ && workerThread_->isRunning()) {
        if (running_ || loading_) {
            QMetaObject::invokeMethod(worker_, "stop", Qt::BlockingQueuedConnection);
        }
        workerThread_->quit();
        workerThread_->wait(3000);
    }
}

void RasterTileSource::startAsync(const QString &packagePath, quint16 requestedPort)
{
    if (loading_ || running_) {
        return;
    }
    if (!workerThread_) {
        workerThread_ = new QThread(this);
        workerThread_->setObjectName(QStringLiteral("qtmaps-raster-tile-source"));
        worker_ = new RasterTileSourceWorker;
        worker_->moveToThread(workerThread_);
        connect(workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
        connect(worker_, &RasterTileSourceWorker::startFinished, this,
                [this](const QVariantMap &result) {
                    loading_ = false;
                    running_ = result.value(QStringLiteral("running")).toBool();
                    baseUrl_ = result.value(QStringLiteral("base_url")).toString();
                    errorMessage_ = result.value(QStringLiteral("error")).toString();
                    if (running_) {
                        emit started();
                    } else {
                        emit failed(errorMessage_);
                    }
                });
        connect(worker_, &RasterTileSourceWorker::stopFinished, this, [this]() {
            running_ = false;
            loading_ = false;
            baseUrl_.clear();
            finishThread();
        });
        workerThread_->start();
    }
    loading_ = true;
    errorMessage_.clear();
    QMetaObject::invokeMethod(worker_, "start", Qt::QueuedConnection, Q_ARG(QString, packagePath),
                              Q_ARG(quint16, requestedPort));
}

void RasterTileSource::stopAsync()
{
    if (!workerThread_ || !workerThread_->isRunning()) {
        running_ = false;
        loading_ = false;
        baseUrl_.clear();
        emit stopped();
        return;
    }
    QMetaObject::invokeMethod(worker_, "stop", Qt::QueuedConnection);
}

void RasterTileSource::finishThread()
{
    if (!workerThread_) {
        return;
    }
    QThread *thread = workerThread_;
    connect(thread, &QThread::finished, this, [this, thread]() {
        if (workerThread_ == thread) {
            workerThread_ = nullptr;
            worker_ = nullptr;
        }
        thread->deleteLater();
        emit stopped();
    });
    thread->quit();
}

bool RasterTileSource::running() const { return running_; }
bool RasterTileSource::loading() const { return loading_; }
QString RasterTileSource::baseUrl() const { return baseUrl_; }
QString RasterTileSource::styleUrl() const
{
    return running_ ? baseUrl_ + QStringLiteral("/style/style.json") : QString();
}
QString RasterTileSource::errorMessage() const { return errorMessage_; }

} // namespace map
} // namespace qtmaps

#include "RasterTileSource.moc"
