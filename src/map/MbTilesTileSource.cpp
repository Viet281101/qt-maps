#include "MbTilesTileSource.h"

#include "MapPackageManifest.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QMetaObject>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QVariantMap>

namespace qtmaps {
namespace map {

class MbTilesTileSourceWorker final : public QObject
{
    Q_OBJECT

public slots:
    QVariantMap start(const QString &packagePath, quint16 requestedPort)
    {
        stop();
        QVariantMap result;
        MapPackageManifest manifest;
        QString validationError;
        if (!MapPackageManifest::loadAndValidate(packagePath, &manifest, &validationError)) {
            result.insert(QStringLiteral("error"), validationError);
            return result;
        }
        root_ = QDir(QFileInfo(packagePath).canonicalFilePath());
        connectionName_ = QStringLiteral("qtmaps-mbtiles-%1")
                              .arg(reinterpret_cast<quintptr>(this));
        database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
        database_.setDatabaseName(root_.filePath(QStringLiteral("tiles.mbtiles")));
        database_.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        if (!database_.open()) {
            result.insert(QStringLiteral("error"),
                          QStringLiteral("Cannot open validated MBTiles database: %1")
                              .arg(database_.lastError().text()));
            closeDatabase();
            return result;
        }
        QSqlQuery probe(database_);
        if (!probe.exec(QStringLiteral("SELECT 1 FROM tiles LIMIT 1")) || !probe.next()) {
            result.insert(QStringLiteral("error"),
                          QStringLiteral("MBTiles database has no readable tiles: %1")
                              .arg(probe.lastError().text()));
            closeDatabase();
            return result;
        }
        server_ = new QTcpServer(this);
        connect(server_, &QTcpServer::newConnection,
                this, &MbTilesTileSourceWorker::acceptConnections);
        if (!server_->listen(QHostAddress::LocalHost, requestedPort)) {
            result.insert(QStringLiteral("error"),
                          QStringLiteral("Cannot bind tile service to 127.0.0.1:%1: %2")
                              .arg(requestedPort).arg(server_->errorString()));
            server_->deleteLater();
            server_ = nullptr;
            closeDatabase();
            return result;
        }
        baseUrl_ = QStringLiteral("http://127.0.0.1:%1").arg(server_->serverPort());
        result.insert(QStringLiteral("running"), true);
        result.insert(QStringLiteral("base_url"), baseUrl_);
        result.insert(QStringLiteral("data_version"), manifest.dataVersion);
        return result;
    }

    void stop()
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
        root_ = QDir();
        baseUrl_.clear();
    }

private slots:
    void acceptConnections()
    {
        while (server_ && server_->hasPendingConnections()) {
            QTcpSocket *socket = server_->nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                handleRequest(socket);
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    }

private:
    void closeDatabase()
    {
        if (!connectionName_.isEmpty()) {
            database_.close();
            database_ = QSqlDatabase();
            QSqlDatabase::removeDatabase(connectionName_);
            connectionName_.clear();
        }
    }

    void writeResponse(QTcpSocket *socket, int status, const QByteArray &contentType,
                       const QByteArray &body)
    {
        const QByteArray reason = status == 200 ? QByteArrayLiteral("OK")
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

    QByteArray readStatic(const QString &relativePath)
    {
        const QString clean = QDir::cleanPath(relativePath);
        if (clean.startsWith(QStringLiteral("../")) || QDir::isAbsolutePath(clean)) {
            return QByteArray();
        }
        QFile file(root_.filePath(clean));
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

    void handleRequest(QTcpSocket *socket)
    {
        const QByteArray request = socket->property("requestBuffer").toByteArray()
            + socket->readAll();
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
            writeResponse(socket, 200, QByteArrayLiteral("text/plain"),
                          QByteArrayLiteral("ok\n"));
            return;
        }
        if (path == QStringLiteral("/style/style.json")) {
            QByteArray style = readStatic(QStringLiteral("style/style.json"));
            if (!style.isEmpty()) {
                style.replace("__QTMAPS_BASE_URL__", baseUrl_.toUtf8());
                writeResponse(socket, 200, QByteArrayLiteral("application/json"), style);
                return;
            }
        }
        const QRegularExpression tilePattern(
            QStringLiteral("^/tiles/(\\d+)/(\\d+)/(\\d+)\\.pbf$"));
        const QRegularExpressionMatch tileMatch = tilePattern.match(path);
        if (tileMatch.hasMatch()) {
            const int zoom = tileMatch.captured(1).toInt();
            const int x = tileMatch.captured(2).toInt();
            const int xyzY = tileMatch.captured(3).toInt();
            if (zoom >= 0 && zoom < 31) {
                const qint64 tmsY = (qint64(1) << zoom) - 1 - xyzY;
                QSqlQuery query(database_);
                query.prepare(QStringLiteral(
                    "SELECT tile_data FROM tiles WHERE zoom_level=? AND tile_column=? AND tile_row=?"));
                query.addBindValue(zoom);
                query.addBindValue(x);
                query.addBindValue(tmsY);
                if (query.exec() && query.next()) {
                    writeResponse(socket, 200,
                                  QByteArrayLiteral("application/vnd.mapbox-vector-tile"),
                                  query.value(0).toByteArray());
                    return;
                }
            }
        }
        QString relative;
        QByteArray contentType;
        if (path.startsWith(QStringLiteral("/glyphs/"))) {
            relative = path.mid(1);
            contentType = QByteArrayLiteral("application/x-protobuf");
        } else if (path.startsWith(QStringLiteral("/sprites/"))) {
            relative = path.mid(1);
            contentType = path.endsWith(QStringLiteral(".png"))
                ? QByteArrayLiteral("image/png") : QByteArrayLiteral("application/json");
        }
        if (!relative.isEmpty()) {
            const QByteArray body = readStatic(relative);
            if (!body.isEmpty()) {
                writeResponse(socket, 200, contentType, body);
                return;
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
};

MbTilesTileSource::MbTilesTileSource(QObject *parent)
    : TileSource(parent), worker_(new MbTilesTileSourceWorker)
{
    workerThread_.setObjectName(QStringLiteral("qtmaps-mbtiles-tile-source"));
    worker_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    workerThread_.start();
}

MbTilesTileSource::~MbTilesTileSource()
{
    stop();
    workerThread_.quit();
    workerThread_.wait(3000);
}

bool MbTilesTileSource::start(const QString &packagePath, quint16 requestedPort)
{
    QVariantMap result;
    QMetaObject::invokeMethod(worker_, "start", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariantMap, result),
                              Q_ARG(QString, packagePath), Q_ARG(quint16, requestedPort));
    running_ = result.value(QStringLiteral("running")).toBool();
    baseUrl_ = result.value(QStringLiteral("base_url")).toString();
    errorMessage_ = result.value(QStringLiteral("error")).toString();
    emit runningChanged();
    if (!errorMessage_.isEmpty()) {
        emit errorOccurred(errorMessage_);
    }
    return running_;
}

void MbTilesTileSource::stop()
{
    if (!workerThread_.isRunning()) {
        return;
    }
    QMetaObject::invokeMethod(worker_, "stop", Qt::BlockingQueuedConnection);
    const bool changed = running_;
    running_ = false;
    baseUrl_.clear();
    if (changed) {
        emit runningChanged();
    }
}

bool MbTilesTileSource::isRunning() const { return running_; }
QString MbTilesTileSource::baseUrl() const { return baseUrl_; }
QString MbTilesTileSource::errorMessage() const { return errorMessage_; }

} // namespace map
} // namespace qtmaps

#include "MbTilesTileSource.moc"
