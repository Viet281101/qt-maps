#pragma once

#include <QObject>
#include <QString>

class QThread;

namespace qtmaps {
namespace map {

class RasterTileSourceWorker;

class RasterTileSource final : public QObject
{
    Q_OBJECT

public:
    static constexpr int kCacheMaximumBytes = 64 * 1024 * 1024;

    explicit RasterTileSource(QObject *parent = nullptr);
    ~RasterTileSource() override;

    void startAsync(const QString &packagePath, quint16 requestedPort = 0);
    void stopAsync();
    bool running() const;
    bool loading() const;
    QString baseUrl() const;
    QString styleUrl() const;
    QString errorMessage() const;

signals:
    void started();
    void failed(const QString &message);
    void stopped();

private:
    void finishThread();

    QThread *workerThread_ = nullptr;
    RasterTileSourceWorker *worker_ = nullptr;
    bool running_ = false;
    bool loading_ = false;
    QString baseUrl_;
    QString errorMessage_;
};

} // namespace map
} // namespace qtmaps
