#include <QtTest>

#include "core/ApplicationShutdownCoordinator.h"
#include "perf/ProcessTreeSampler.h"
#include "routing/ValhallaRouteClient.h"
#include "routing/ValhallaServiceManager.h"
#include "routing/RoutingController.h"
#include "routing/RouteNavigationController.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrlQuery>

#if defined(Q_OS_LINUX)
#include <cerrno>
#include <csignal>
#endif

using qtmaps::routing::Route;
using qtmaps::routing::RouteSource;
using qtmaps::routing::RoutingController;
using qtmaps::routing::RouteNavigationController;
using qtmaps::routing::ValhallaRouteClient;
using qtmaps::routing::ValhallaServiceManager;
using qtmaps::core::ApplicationShutdownCoordinator;
using qtmaps::perf::ProcessTreeSampler;

namespace {

void writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
    QCOMPARE(file.write(contents), contents.size());
}

quint16 unusedLoopbackPort()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        return 0;
    }
    return server.serverPort();
}

class HttpFixtureServer final : public QObject
{
    Q_OBJECT

public:
    explicit HttpFixtureServer(QByteArray response, bool replyToRequest = true,
                               QObject *parent = nullptr)
        : QObject(parent), response_(std::move(response)), replyToRequest_(replyToRequest)
    {
        connect(&server_, &QTcpServer::newConnection, this, [this]() {
            while (QTcpSocket *socket = server_.nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                    request_ += socket->readAll();
                    if (!request_.contains("\r\n\r\n") || !replyToRequest_) {
                        return;
                    }
                    const QByteArray header = QByteArrayLiteral(
                        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: ")
                        + QByteArray::number(response_.size()) + QByteArrayLiteral("\r\n\r\n");
                    socket->write(header + response_);
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }

    bool start() { return server_.listen(QHostAddress::LocalHost, 0); }
    quint16 port() const { return server_.serverPort(); }
    QByteArray request() const { return request_; }

private:
    QTcpServer server_;
    QByteArray response_;
    QByteArray request_;
    bool replyToRequest_ = true;
};

int runFakeValhallaService(int argc, char **argv, const QJsonObject &config)
{
#if defined(Q_OS_LINUX)
    if (config.value(QStringLiteral("fake_ignore_interrupt")).toBool()) {
        std::signal(SIGINT, SIG_IGN);
    }
#endif
    QCoreApplication application(argc, argv);
    QFile fixture(QStringLiteral(":/fixtures/routing/hanoi_hoan_kiem_to_thai_ha.json"));
    if (!fixture.open(QIODevice::ReadOnly)) {
        return 3;
    }
    const QByteArray routeBody = fixture.readAll();
    const int routeDelayMs = config.value(QStringLiteral("fake_route_delay_ms")).toInt();
    const bool routeError = config.value(QStringLiteral("fake_route_error")).toBool();
    const QString requestLogPath = config.value(QStringLiteral("fake_request_log")).toString();
    QTcpServer server;
    const quint16 port = static_cast<quint16>(config.value(QStringLiteral("fake_service_port")).toInt());
    if (!server.listen(QHostAddress::LocalHost, port)) {
        return 2;
    }
    QObject::connect(&server, &QTcpServer::newConnection, &application,
                     [&server, routeBody, routeDelayMs, routeError, requestLogPath]() {
        while (QTcpSocket *socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::readyRead, socket,
                             [socket, routeBody, routeDelayMs, routeError, requestLogPath]() {
                const QByteArray request = socket->property("request").toByteArray()
                    + socket->readAll();
                socket->setProperty("request", request);
                if (!request.contains("\r\n\r\n")
                    || socket->property("responseScheduled").toBool()) {
                    return;
                }
                socket->setProperty("responseScheduled", true);
                if (!requestLogPath.isEmpty() && request.startsWith("GET /route?")) {
                    QFile log(requestLogPath);
                    if (log.open(QIODevice::WriteOnly | QIODevice::Append)) {
                        log.write(request.split('\n').constFirst());
                        log.write("\n");
                    }
                }
                const QByteArray body = request.startsWith("GET /route?")
                    ? routeBody : QByteArrayLiteral("{\"version\":\"test\"}");
                const int delayMs = request.startsWith("GET /route?") ? routeDelayMs : 0;
                const bool fail = routeError && request.startsWith("GET /route?");
                QTimer::singleShot(delayMs, socket, [socket, body, fail]() {
                    socket->write((fail ? QByteArrayLiteral("HTTP/1.1 503 Service Unavailable\r\n")
                                        : QByteArrayLiteral("HTTP/1.1 200 OK\r\n"))
                                  + QByteArrayLiteral("Content-Type: application/json\r\nConnection: close\r\nContent-Length: ")
                                  + QByteArray::number(body.size())
                                  + QByteArrayLiteral("\r\n\r\n") + body);
                    socket->disconnectFromHost();
                });
            });
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });
    return application.exec();
}

int runShutdownCoordinatorProbe(int argc, char **argv, bool waitForParentDeath)
{
    QCoreApplication application(argc, argv);
    bool portOk = false;
    const quint16 port = QString::fromLocal8Bit(argv[3]).toUShort(&portOk);
    if (!portOk) {
        return 4;
    }
    RoutingController controller;
    ApplicationShutdownCoordinator coordinator(&controller);
    QObject::connect(&controller, &RoutingController::backendReady, &application, [&]() {
        if (waitForParentDeath) {
            QFile pidFile(QString::fromLocal8Bit(argv[4]));
            if (!pidFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                application.exit(5);
                return;
            }
            pidFile.write(QByteArray::number(controller.backendProcessId()));
            pidFile.close();
        } else {
            QTimer::singleShot(0, &coordinator,
                               &ApplicationShutdownCoordinator::requestQuit);
        }
    });
    QObject::connect(&coordinator, &ApplicationShutdownCoordinator::shutdownReady,
                     &application, &QCoreApplication::quit);
    QTimer::singleShot(8000, &application, [&application]() { application.exit(6); });
    controller.startBackend(QCoreApplication::applicationFilePath(),
                            QString::fromLocal8Bit(argv[2]), port);
    return application.exec();
}

} // namespace

class ValhallaRuntimeTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { qRegisterMetaType<Route>(); }

    void managedServiceStartsBecomesReadyAndStops()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const quint16 port = unusedLoopbackPort();
        QVERIFY(port != 0);
        const QString configPath = temporary.path() + QStringLiteral("/fake-valhalla.json");
        QJsonObject serviceConfig;
        serviceConfig.insert(QStringLiteral("listen"),
                             QStringLiteral("tcp://127.0.0.1:%1").arg(port));
        QJsonObject httpd;
        httpd.insert(QStringLiteral("service"), serviceConfig);
        QJsonObject config;
        config.insert(QStringLiteral("fake_service_port"), static_cast<int>(port));
        config.insert(QStringLiteral("httpd"), httpd);
        writeFile(configPath, QJsonDocument(config).toJson(QJsonDocument::Compact));

        ValhallaServiceManager service;
        QSignalSpy readySpy(&service, &ValhallaServiceManager::ready);
        service.start(QCoreApplication::applicationFilePath(), configPath, port, 2, 3000);
        QVERIFY2(readySpy.wait(3000), qPrintable(service.errorMessage()));
        QCOMPARE(service.state(), ValhallaServiceManager::State::Ready);
        QCOMPARE(service.endpoint(), QUrl(QStringLiteral("http://127.0.0.1:%1").arg(port)));
        const QList<qint64> descendants = ProcessTreeSampler::descendantPids(
            QCoreApplication::applicationPid(), ProcessTreeSampler::readProc());
        QVERIFY(descendants.contains(service.processId()));

        service.stop();
        QTRY_COMPARE_WITH_TIMEOUT(service.state(), ValhallaServiceManager::State::Stopped, 3000);
    }

    void managedServiceRejectsUnsafeConfiguration()
    {
        ValhallaServiceManager service;
        service.start(QStringLiteral("/missing/valhalla_service"),
                      QStringLiteral("/missing/valhalla.json"), 8002);
        QCOMPARE(service.state(), ValhallaServiceManager::State::Error);
        QCOMPARE(service.errorMessage(), QStringLiteral("thiếu /missing/valhalla_service"));

        service.start(QCoreApplication::applicationFilePath(),
                      QStringLiteral("/missing/valhalla.json"), 8002);
        QCOMPARE(service.state(), ValhallaServiceManager::State::Error);
        QCOMPARE(service.errorMessage(), QStringLiteral("thiếu /missing/valhalla.json"));

        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const quint16 port = unusedLoopbackPort();
        QJsonObject serviceConfig;
        serviceConfig.insert(QStringLiteral("listen"),
                             QStringLiteral("tcp://0.0.0.0:%1").arg(port));
        QJsonObject httpd;
        httpd.insert(QStringLiteral("service"), serviceConfig);
        const QString configPath = temporary.path() + QStringLiteral("/unsafe.json");
        writeFile(configPath,
                  QJsonDocument(QJsonObject{{QStringLiteral("httpd"), httpd}})
                      .toJson(QJsonDocument::Compact));
        ValhallaServiceManager unsafeService;
        unsafeService.start(QCoreApplication::applicationFilePath(), configPath, port);
        QCOMPARE(unsafeService.state(), ValhallaServiceManager::State::Error);
        QVERIFY(unsafeService.errorMessage().contains(QStringLiteral("listen only")));
    }

    void applicationShutdownCoordinatorStopsChildBeforeExit()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const quint16 port = unusedLoopbackPort();
        QVERIFY(port != 0);
        QJsonObject serviceConfig;
        serviceConfig.insert(QStringLiteral("listen"),
                             QStringLiteral("tcp://127.0.0.1:%1").arg(port));
        QJsonObject httpd;
        httpd.insert(QStringLiteral("service"), serviceConfig);
        QJsonObject config;
        config.insert(QStringLiteral("fake_service_port"), static_cast<int>(port));
        config.insert(QStringLiteral("httpd"), httpd);
        const QString configPath = temporary.path() + QStringLiteral("/shutdown.json");
        writeFile(configPath, QJsonDocument(config).toJson(QJsonDocument::Compact));

        QProcess probe;
        probe.start(QCoreApplication::applicationFilePath(),
                    {QStringLiteral("--shutdown-coordinator-probe"), configPath,
                     QString::number(port)});
        QVERIFY(probe.waitForStarted(2000));
        QVERIFY2(probe.waitForFinished(5000), qPrintable(probe.errorString()));
        QCOMPARE(probe.exitStatus(), QProcess::NormalExit);
        QCOMPARE(probe.exitCode(), 0);

        QTcpServer releasedPort;
        QVERIFY2(releasedPort.listen(QHostAddress::LocalHost, port),
                 qPrintable(releasedPort.errorString()));
    }

    void applicationShutdownCoordinatorForceKillsUnresponsiveChild()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const quint16 port = unusedLoopbackPort();
        QVERIFY(port != 0);
        QJsonObject serviceConfig;
        serviceConfig.insert(QStringLiteral("listen"),
                             QStringLiteral("tcp://127.0.0.1:%1").arg(port));
        QJsonObject httpd;
        httpd.insert(QStringLiteral("service"), serviceConfig);
        QJsonObject config;
        config.insert(QStringLiteral("fake_service_port"), static_cast<int>(port));
        config.insert(QStringLiteral("fake_ignore_interrupt"), true);
        config.insert(QStringLiteral("httpd"), httpd);
        const QString configPath = temporary.path() + QStringLiteral("/forced-shutdown.json");
        writeFile(configPath, QJsonDocument(config).toJson(QJsonDocument::Compact));

        QElapsedTimer elapsed;
        elapsed.start();
        QProcess probe;
        probe.start(QCoreApplication::applicationFilePath(),
                    {QStringLiteral("--shutdown-coordinator-probe"), configPath,
                     QString::number(port)});
        QVERIFY(probe.waitForStarted(2000));
        QVERIFY2(probe.waitForFinished(6000), qPrintable(probe.errorString()));
        QCOMPARE(probe.exitStatus(), QProcess::NormalExit);
        QCOMPARE(probe.exitCode(), 0);
        QVERIFY(elapsed.elapsed() >= 1900);
        QVERIFY(elapsed.elapsed() < 5000);

        QTcpServer releasedPort;
        QVERIFY2(releasedPort.listen(QHostAddress::LocalHost, port),
                 qPrintable(releasedPort.errorString()));
    }

#if defined(Q_OS_LINUX)
    void parentDeathSignalStopsManagedChild()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const quint16 port = unusedLoopbackPort();
        QVERIFY(port != 0);
        QJsonObject serviceConfig;
        serviceConfig.insert(QStringLiteral("listen"),
                             QStringLiteral("tcp://127.0.0.1:%1").arg(port));
        QJsonObject httpd;
        httpd.insert(QStringLiteral("service"), serviceConfig);
        QJsonObject config;
        config.insert(QStringLiteral("fake_service_port"), static_cast<int>(port));
        config.insert(QStringLiteral("httpd"), httpd);
        const QString configPath = temporary.path() + QStringLiteral("/pdeath.json");
        const QString pidPath = temporary.path() + QStringLiteral("/child.pid");
        writeFile(configPath, QJsonDocument(config).toJson(QJsonDocument::Compact));

        QProcess probe;
        probe.start(QCoreApplication::applicationFilePath(),
                    {QStringLiteral("--pdeathsig-probe"), configPath,
                     QString::number(port), pidPath});
        QVERIFY(probe.waitForStarted(2000));
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(pidPath), 3000);
        QFile pidFile(pidPath);
        QVERIFY(pidFile.open(QIODevice::ReadOnly));
        bool pidOk = false;
        const qint64 childPid = pidFile.readAll().trimmed().toLongLong(&pidOk);
        QVERIFY(pidOk);
        QVERIFY(childPid > 1);

        probe.kill();
        QVERIFY(probe.waitForFinished(2000));
        QTRY_VERIFY_WITH_TIMEOUT(
            (::kill(static_cast<pid_t>(childPid), 0) == -1 && errno == ESRCH), 3000);
        QTcpServer releasedPort;
        QVERIFY2(releasedPort.listen(QHostAddress::LocalHost, port),
                 qPrintable(releasedPort.errorString()));
    }
#endif

    void clientRequestsVietnameseKilometerRouteAsynchronously()
    {
        QFile fixture(QStringLiteral(QTMAPS_SOURCE_DIR
                                     "/tests/fixtures/routing/hanoi_hoan_kiem_to_ba_dinh.json"));
        QVERIFY(fixture.open(QIODevice::ReadOnly));
        HttpFixtureServer server(fixture.readAll());
        QVERIFY(server.start());

        ValhallaRouteClient client;
        QString error;
        QVERIFY2(client.setEndpoint(
                     QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.port())), &error),
                 qPrintable(error));
        QSignalSpy readySpy(&client, &ValhallaRouteClient::routeReady);
        client.requestRoute(QGeoCoordinate(21.0285, 105.8542),
                            QGeoCoordinate(21.0369, 105.8340));
        QVERIFY(readySpy.wait(2000));
        QCOMPARE(readySpy.size(), 1);
        const Route route = qvariant_cast<Route>(readySpy.takeFirst().at(0));
        QCOMPARE(route.source, RouteSource::Valhalla);
        QCOMPARE(route.distanceMeters, 4262.0);

        const QByteArray requestLine = server.request().split('\n').constFirst().trimmed();
        const QList<QByteArray> parts = requestLine.split(' ');
        QCOMPARE(parts.size(), 3);
        const QUrl requestUrl(QString::fromLatin1(parts.at(1)));
        const QJsonObject payload = QJsonDocument::fromJson(
            QUrlQuery(requestUrl).queryItemValue(QStringLiteral("json")).toUtf8()).object();
        QCOMPARE(payload.value(QStringLiteral("costing")).toString(), QStringLiteral("auto"));
        QCOMPARE(payload.value(QStringLiteral("units")).toString(), QStringLiteral("kilometers"));
        QCOMPARE(payload.value(QStringLiteral("language")).toString(), QStringLiteral("vi-VN"));
    }

    void clientTimesOutAndRejectsRemoteEndpoints()
    {
        HttpFixtureServer server(QByteArray(), false);
        QVERIFY(server.start());
        ValhallaRouteClient client;
        QString error;
        QVERIFY(!client.setEndpoint(QUrl(QStringLiteral("http://192.0.2.1:8002")), &error));
        QVERIFY(error.contains(QStringLiteral("loopback")));
        QVERIFY(client.setEndpoint(
            QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.port()))));
        QSignalSpy failureSpy(&client, &ValhallaRouteClient::routeFailed);
        client.requestRoute(QGeoCoordinate(21.0285, 105.8542),
                            QGeoCoordinate(21.0369, 105.8340), 100);
        QVERIFY(failureSpy.wait(1000));
        QVERIFY(failureSpy.takeFirst().at(0).toString().contains(QStringLiteral("timed out")));
        QVERIFY(!client.isRequestPending());
    }

    void fixtureControllerSelectsPointsCalculatesAndFocusesManeuver()
    {
        RoutingController controller;
        QVERIFY(controller.setOriginInput(QStringLiteral("Hoàn Kiếm")));
        QVERIFY(controller.setDestinationInput(QStringLiteral("Ba Đình")));
        controller.calculateFixtureRoute();
        QVERIFY(controller.routeAvailable());
        QCOMPARE(controller.sourceLabel(), QStringLiteral("fixture/mô phỏng"));
        QCOMPARE(controller.connectionLabel(), QStringLiteral("offline — fixture"));
        QCOMPARE(controller.distanceMeters(), 4262.0);
        QCOMPARE(controller.maneuvers().size(), 11);
        QVERIFY(!controller.streetLabels().isEmpty());
        QVERIFY(controller.routePath().size() > 200);

        QSignalSpy focusSpy(&controller, &RoutingController::focusCoordinateRequested);
        controller.focusManeuver(4);
        QCOMPARE(focusSpy.size(), 1);
        QCOMPARE(controller.currentManeuverIndex(), 4);
        QVERIFY(focusSpy.takeFirst().at(0).value<QGeoCoordinate>().isValid());

        controller.selectMapPoint(QGeoCoordinate(21.0300, 105.8500));
        QVERIFY(!controller.routeAvailable());
        QCOMPARE(controller.currentManeuverIndex(), -1);
        QVERIFY(controller.originInput().contains(QLatin1Char(',')));
        controller.calculateFixtureRoute();
        QVERIFY(controller.statusMessage().contains(QStringLiteral("Fixture chỉ")));
    }

    void controllerSwitchesFromFixtureToManagedBackend()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const quint16 port = unusedLoopbackPort();
        QVERIFY(port != 0);
        QJsonObject serviceConfig;
        serviceConfig.insert(QStringLiteral("listen"),
                             QStringLiteral("tcp://127.0.0.1:%1").arg(port));
        QJsonObject httpd;
        httpd.insert(QStringLiteral("service"), serviceConfig);
        QJsonObject config;
        config.insert(QStringLiteral("fake_service_port"), static_cast<int>(port));
        config.insert(QStringLiteral("httpd"), httpd);
        const QString configPath = temporary.path() + QStringLiteral("/fake-valhalla.json");
        writeFile(configPath, QJsonDocument(config).toJson(QJsonDocument::Compact));

        RoutingController controller;
        QSignalSpy routeChangedSpy(&controller, &RoutingController::routeChanged);
        controller.calculateRoute();
        QVERIFY(!controller.routeAvailable());
        QVERIFY(controller.statusMessage().contains(QStringLiteral("chưa sẵn sàng")));
        QCOMPARE(routeChangedSpy.size(), 0);
        controller.startBackend(QCoreApplication::applicationFilePath(), configPath, port);
        QTRY_COMPARE_WITH_TIMEOUT(controller.sourceLabel(), QStringLiteral("Valhalla local"), 4000);
        QCOMPARE(routeChangedSpy.size(), 1);
        QCOMPARE(controller.connectionLabel(), QStringLiteral("Valhalla local — loopback"));
        QVERIFY(controller.routeAvailable());
        QVERIFY(!controller.loading());
        controller.stopBackend();
    }

    void controllerReportsMissingBackendPathWithoutFixtureFallback()
    {
        RoutingController controller;
        const QString missingProgram = QStringLiteral("/missing/unit04/valhalla_service");
        controller.startBackend(missingProgram,
                                QStringLiteral("/missing/unit04/valhalla.json"), 18002);

        QVERIFY(!controller.routeAvailable());
        QCOMPARE(controller.statusMessage(),
                 QStringLiteral("Valhalla chưa chạy: thiếu %1").arg(missingProgram));
        QCOMPARE(controller.connectionLabel(), controller.statusMessage());
        QCOMPARE(controller.sourceLabel(), QStringLiteral("fixture/mô phỏng"));
    }

    void controllerIgnoresDelayedRouteAfterEndpointsChange()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const quint16 port = unusedLoopbackPort();
        QVERIFY(port != 0);
        QJsonObject serviceConfig;
        serviceConfig.insert(QStringLiteral("listen"),
                             QStringLiteral("tcp://127.0.0.1:%1").arg(port));
        QJsonObject httpd;
        httpd.insert(QStringLiteral("service"), serviceConfig);
        QJsonObject config;
        config.insert(QStringLiteral("fake_service_port"), static_cast<int>(port));
        config.insert(QStringLiteral("fake_route_delay_ms"), 400);
        config.insert(QStringLiteral("httpd"), httpd);
        const QString configPath = temporary.path() + QStringLiteral("/delayed-valhalla.json");
        writeFile(configPath, QJsonDocument(config).toJson(QJsonDocument::Compact));

        RoutingController controller;
        RouteNavigationController navigation;
        connect(&controller, &RoutingController::routeChanged, &navigation, [&]() {
            if (controller.routeAvailable())
                navigation.setRoute(controller.route(), controller.sourceLabel());
            else
                navigation.clearRoute();
        });
        controller.calculateFixtureRoute();
        navigation.start();
        QVERIFY(navigation.running());
        controller.startBackend(QCoreApplication::applicationFilePath(), configPath, port);
        QTRY_VERIFY_WITH_TIMEOUT(controller.loading(), 3000);
        QVERIFY(!navigation.active());
        QVERIFY(!navigation.routeAvailable());

        QVERIFY(controller.setDestinationInput(QStringLiteral("Ba Đình")));
        QVERIFY(!controller.loading());
        QVERIFY(!controller.routeAvailable());
        QVERIFY(!navigation.active());
        QTest::qWait(700);

        QVERIFY(!controller.routeAvailable());
        QCOMPARE(controller.destinationInput(), QStringLiteral("Ba Đình"));
        QVERIFY(controller.sourceLabel() != QStringLiteral("Valhalla local"));
        controller.stopBackend();
    }

    void changingEitherEndpointStopsReplay()
    {
        RoutingController controller;
        RouteNavigationController navigation;
        connect(&controller, &RoutingController::routeChanged, &navigation, [&]() {
            if (controller.routeAvailable())
                navigation.setRoute(controller.route(), controller.sourceLabel());
            else
                navigation.clearRoute();
        });
        controller.calculateFixtureRoute();
        navigation.start();
        QVERIFY(navigation.running());
        QVERIFY(controller.setOriginInput(QStringLiteral("Hoàn Kiếm")));
        QVERIFY(!navigation.active());
        QVERIFY(!navigation.routeAvailable());

        controller.calculateFixtureRoute();
        navigation.start();
        QVERIFY(navigation.running());
        QVERIFY(controller.setDestinationInput(QStringLiteral("Ba Đình")));
        QVERIFY(!navigation.active());
        QVERIFY(!navigation.routeAvailable());
    }

    void rerouteReplacesDelayedRequestAndResumesNavigation()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const quint16 port = unusedLoopbackPort();
        QVERIFY(port != 0);
        const QJsonObject service{{QStringLiteral("listen"),
                                   QStringLiteral("tcp://127.0.0.1:%1").arg(port)}};
        const QString logPath = temporary.path() + QStringLiteral("/requests.log");
        const QJsonObject config{{QStringLiteral("fake_service_port"), static_cast<int>(port)},
                                 {QStringLiteral("fake_route_delay_ms"), 500},
                                 {QStringLiteral("fake_request_log"), logPath},
                                 {QStringLiteral("httpd"),
                                  QJsonObject{{QStringLiteral("service"), service}}}};
        const QString configPath = temporary.path() + QStringLiteral("/reroute.json");
        writeFile(configPath, QJsonDocument(config).toJson(QJsonDocument::Compact));

        RoutingController controller;
        RouteNavigationController navigation;
        connect(&controller, &RoutingController::routeChanged, &navigation, [&]() {
            if (controller.routeAvailable())
                navigation.setRoute(controller.route(), controller.sourceLabel());
            else
                navigation.clearRoute();
        });
        connect(&navigation, &RouteNavigationController::deviationDetected,
                &controller, &RoutingController::requestReroute);
        connect(&controller, &RoutingController::rerouteSucceeded,
                &navigation, &RouteNavigationController::start);
        controller.startBackend(QCoreApplication::applicationFilePath(), configPath, port);
        QTRY_VERIFY_WITH_TIMEOUT(controller.routeAvailable(), 4000);
        navigation.start();
        const QGeoCoordinate destination = controller.destination();
        QSignalSpy successSpy(&controller, &RoutingController::rerouteSucceeded);
        navigation.simulateDeviation();
        QVERIFY(controller.loading());
        QVERIFY(!navigation.active());
        QTest::qWait(250);
        controller.requestReroute(QGeoCoordinate(21.029, 105.855));
        QTest::qWait(320);
        QCOMPARE(successSpy.size(), 0);
        QVERIFY(controller.loading());
        QVERIFY(!controller.routeAvailable());
        QTRY_COMPARE_WITH_TIMEOUT(successSpy.size(), 1, 2000);
        QVERIFY(navigation.running());
        QCOMPARE(controller.sourceLabel(), QStringLiteral("Valhalla local"));
        QCOMPARE(controller.destination(), destination);
        QFile log(logPath);
        QVERIFY(log.open(QIODevice::ReadOnly));
        const QList<QByteArray> lines = log.readAll().trimmed().split('\n');
        QVERIFY(lines.size() >= 3);
        const QList<QByteArray> requestParts = lines.constLast().trimmed().split(' ');
        QCOMPARE(requestParts.size(), 3);
        const QJsonObject payload = QJsonDocument::fromJson(
            QUrlQuery(QUrl(QString::fromLatin1(requestParts.at(1))))
                .queryItemValue(QStringLiteral("json")).toUtf8()).object();
        const QJsonArray locations = payload.value(QStringLiteral("locations")).toArray();
        QCOMPARE(locations.size(), 2);
        QCOMPARE(locations.at(0).toObject().value(QStringLiteral("lat")).toDouble(), 21.029);
        QCOMPARE(locations.at(0).toObject().value(QStringLiteral("lon")).toDouble(), 105.855);
        QCOMPARE(locations.at(1).toObject().value(QStringLiteral("lat")).toDouble(),
                 destination.latitude());
        QCOMPARE(locations.at(1).toObject().value(QStringLiteral("lon")).toDouble(),
                 destination.longitude());
        controller.stopBackend();
    }

    void rerouteBackendFailureIsVisibleWithoutFixtureFallback()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const quint16 port = unusedLoopbackPort();
        QVERIFY(port != 0);
        const QJsonObject service{{QStringLiteral("listen"),
                                   QStringLiteral("tcp://127.0.0.1:%1").arg(port)}};
        const QJsonObject config{{QStringLiteral("fake_service_port"), static_cast<int>(port)},
                                 {QStringLiteral("fake_route_error"), true},
                                 {QStringLiteral("httpd"),
                                  QJsonObject{{QStringLiteral("service"), service}}}};
        const QString configPath = temporary.path() + QStringLiteral("/error.json");
        writeFile(configPath, QJsonDocument(config).toJson(QJsonDocument::Compact));
        RoutingController controller;
        QSignalSpy readySpy(&controller, &RoutingController::backendReady);
        controller.startBackend(QCoreApplication::applicationFilePath(), configPath, port);
        QVERIFY(readySpy.wait(3000));
        QTRY_VERIFY_WITH_TIMEOUT(!controller.loading(), 2000);
        controller.requestReroute(QGeoCoordinate(21.029, 105.855));
        QTRY_VERIFY_WITH_TIMEOUT(!controller.loading(), 2000);
        QVERIFY(!controller.routeAvailable());
        QVERIFY(controller.statusMessage().contains(QStringLiteral("Không thể tính lại tuyến")));
        QVERIFY(!controller.statusMessage().contains(QStringLiteral("fixture")));
        controller.stopBackend();
    }
};

int main(int argc, char **argv)
{
    if (argc == 4 && QByteArray(argv[1]) == QByteArrayLiteral("--shutdown-coordinator-probe")) {
        return runShutdownCoordinatorProbe(argc, argv, false);
    }
    if (argc == 5 && QByteArray(argv[1]) == QByteArrayLiteral("--pdeathsig-probe")) {
        return runShutdownCoordinatorProbe(argc, argv, true);
    }
    if (argc == 3) {
        QFile configFile(QString::fromLocal8Bit(argv[1]));
        if (configFile.open(QIODevice::ReadOnly)) {
            const QJsonObject config = QJsonDocument::fromJson(configFile.readAll()).object();
            if (config.contains(QStringLiteral("fake_service_port"))) {
                return runFakeValhallaService(argc, argv, config);
            }
        }
    }
    QCoreApplication application(argc, argv);
    ValhallaRuntimeTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_ValhallaRuntime.moc"
