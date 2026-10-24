#pragma once

#include "TileSource.h"

#include <QThread>

namespace qtmaps {
namespace map {

class DirectoryTileSourceWorker;

class DirectoryTileSource final : public TileSource
{
    Q_OBJECT

public:
    explicit DirectoryTileSource(QObject *parent = nullptr);
    ~DirectoryTileSource() override;

    bool start(const QString &directoryPath, quint16 requestedPort = 0) override;
    void stop() override;
    bool isRunning() const override;
    QString baseUrl() const override;
    QString errorMessage() const override;

private:
    QThread workerThread_;
    DirectoryTileSourceWorker *worker_ = nullptr;
    bool running_ = false;
    QString baseUrl_;
    QString errorMessage_;
};

} // namespace map
} // namespace qtmaps
