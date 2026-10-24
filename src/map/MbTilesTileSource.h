#pragma once

#include "TileSource.h"

#include <QThread>

namespace qtmaps {
namespace map {

class MbTilesTileSourceWorker;

class MbTilesTileSource final : public TileSource
{
    Q_OBJECT

public:
    explicit MbTilesTileSource(QObject *parent = nullptr);
    ~MbTilesTileSource() override;

    bool start(const QString &packagePath, quint16 requestedPort = 0) override;
    void stop() override;
    bool isRunning() const override;
    QString baseUrl() const override;
    QString errorMessage() const override;

private:
    QThread workerThread_;
    MbTilesTileSourceWorker *worker_ = nullptr;
    bool running_ = false;
    QString baseUrl_;
    QString errorMessage_;
};

} // namespace map
} // namespace qtmaps
