#include "DirectoryTileSource.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QMetaObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QVariantMap>

namespace qtmaps {
namespace map {

class DirectoryTileSourceWorker final : public QObject
{
    Q_OBJECT

public slots:
    QVariantMap start(const QString &directoryPath, quint16 requestedPort)
    {
        stop();
        QVariantMap result;
        const QFileInfo info(directoryPath);
        if (!info.exists() || !info.isDir()) {
            result.insert(
                QStringLiteral("error"),
                QStringLiteral("Diagnostic raster directory is missing: %1").arg(directoryPath));
            return result;
        }
        root_ = QDir(info.canonicalFilePath());
        server_ = new QTcpServer(this);
        connect(server_, &QTcpServer::newConnection, this,
                &DirectoryTileSourceWorker::acceptConnections);
        if (!server_->listen(QHostAddress::LocalHost, requestedPort)) {
            result.insert(QStringLiteral("error"),
                          QStringLiteral("Cannot bind diagnostic raster service: %1")
                              .arg(server_->errorString()));
            delete server_;
            server_ = nullptr;
            return result;
        }
        baseUrl_ = QStringLiteral("http://127.0.0.1:%1").arg(server_->serverPort());
        result.insert(QStringLiteral("running"), true);
        result.insert(QStringLiteral("base_url"), baseUrl_);
        return result;
    }

    void stop()
    {
        if (server_) {
            server_->close();
            delete server_;
            server_ = nullptr;
        }
        root_ = QDir();
        baseUrl_.clear();
    }

private slots:
    void acceptConnections()
    {
        while (server_ && server_->hasPendingConnections()) {
            QTcpSocket *socket = server_->nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                QByteArray request =
                    socket->property("requestBuffer").toByteArray() + socket->readAll();
                if (!request.contains("\r\n\r\n")) {
                    socket->setProperty("requestBuffer", request.left(8192));
                    return;
                }
                const QList<QByteArray> line = request.left(request.indexOf("\r\n")).split(' ');
                const QString path =
                    line.size() >= 2 ? QUrl::fromEncoded(line.at(1)).path() : QString();
                const QString relative = QDir::cleanPath(path.mid(1));
                QByteArray body;
                if (!relative.startsWith(QStringLiteral("../")) &&
                    !QDir::isAbsolutePath(relative)) {
                    QFile file(root_.filePath(relative));
                    if (file.open(QIODevice::ReadOnly)) {
                        body = file.readAll();
                    }
                }
                const int status = body.isEmpty() ? 404 : 200;
                QByteArray response = "HTTP/1.1 " + QByteArray::number(status) +
                                      (status == 200 ? " OK\r\n" : " Not Found\r\n") +
                                      "Content-Type: image/png\r\nContent-Length: " +
                                      QByteArray::number(body.size()) +
                                      "\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n" +
                                      body;
                socket->write(response);
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    }

private:
    QTcpServer *server_ = nullptr;
    QDir root_;
    QString baseUrl_;
};

DirectoryTileSource::DirectoryTileSource(QObject *parent) :
    TileSource(parent), worker_(new DirectoryTileSourceWorker)
{
    workerThread_.setObjectName(QStringLiteral("qtmaps-diagnostic-raster-source"));
    worker_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    workerThread_.start();
}

DirectoryTileSource::~DirectoryTileSource()
{
    stop();
    workerThread_.quit();
    workerThread_.wait(3000);
}

bool DirectoryTileSource::start(const QString &directoryPath, quint16 requestedPort)
{
    QVariantMap result;
    QMetaObject::invokeMethod(worker_, "start", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariantMap, result), Q_ARG(QString, directoryPath),
                              Q_ARG(quint16, requestedPort));
    running_ = result.value(QStringLiteral("running")).toBool();
    baseUrl_ = result.value(QStringLiteral("base_url")).toString();
    errorMessage_ = result.value(QStringLiteral("error")).toString();
    emit runningChanged();
    if (!errorMessage_.isEmpty()) {
        emit errorOccurred(errorMessage_);
    }
    return running_;
}

void DirectoryTileSource::stop()
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

bool DirectoryTileSource::isRunning() const { return running_; }
QString DirectoryTileSource::baseUrl() const { return baseUrl_; }
QString DirectoryTileSource::errorMessage() const { return errorMessage_; }

} // namespace map
} // namespace qtmaps

#include "DirectoryTileSource.moc"
