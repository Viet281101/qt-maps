#pragma once

#include <QObject>

class QTimer;

namespace qtmaps {
namespace routing {
class RoutingController;
}
namespace core {

class ApplicationShutdownCoordinator final : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationShutdownCoordinator(routing::RoutingController *routingController,
                                            QObject *parent = nullptr);

    void requestQuit();
    bool isShuttingDown() const;

signals:
    void shutdownReady();

private:
    void finishShutdown();

    routing::RoutingController *routingController_ = nullptr;
    QTimer *forceKillTimer_ = nullptr;
    bool shuttingDown_ = false;
};

} // namespace core
} // namespace qtmaps
