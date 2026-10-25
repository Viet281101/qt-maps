#include "ApplicationShutdownCoordinator.h"

#include "routing/RoutingController.h"

#include <QTimer>

namespace qtmaps {
namespace core {

ApplicationShutdownCoordinator::ApplicationShutdownCoordinator(
    routing::RoutingController *routingController, QObject *parent)
    : QObject(parent), routingController_(routingController), forceKillTimer_(new QTimer(this))
{
    Q_ASSERT(routingController_);
    forceKillTimer_->setSingleShot(true);
    connect(forceKillTimer_, &QTimer::timeout, routingController_,
            &routing::RoutingController::forceStopBackend);
    connect(routingController_, &routing::RoutingController::backendStopped,
            this, &ApplicationShutdownCoordinator::finishShutdown);
}

void ApplicationShutdownCoordinator::requestQuit()
{
    if (shuttingDown_) {
        return;
    }
    shuttingDown_ = true;
    if (!routingController_->backendRunning()) {
        QTimer::singleShot(0, this, &ApplicationShutdownCoordinator::finishShutdown);
        return;
    }
    routingController_->stopBackend();
    forceKillTimer_->start(2000);
}

bool ApplicationShutdownCoordinator::isShuttingDown() const { return shuttingDown_; }

void ApplicationShutdownCoordinator::finishShutdown()
{
    if (!shuttingDown_) {
        return;
    }
    forceKillTimer_->stop();
    emit shutdownReady();
}

} // namespace core
} // namespace qtmaps
