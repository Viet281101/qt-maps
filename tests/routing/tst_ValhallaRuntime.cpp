#include <QtTest>

#include "routing/ValhallaRouteClient.h"
#include "routing/ValhallaServiceManager.h"
#include "routing/RoutingController.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrlQuery>

using qtmaps::routing::Route;
using qtmaps::routing::RouteSource;
using qtmaps::routing::RoutingController;
using qtmaps::routing::ValhallaRouteClient;
using qtmaps::routing::ValhallaServiceManager;

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
    QCoreApplication application(argc, argv);
    QFile fixture(QStringLiteral(":/fixtures/routing/hanoi_hoan_kiem_to_thai_ha.json"));
    if (!fixture.open(QIODevice::ReadOnly)) {
        return 3;
    }
    const QByteArray routeBody = fixture.readAll();
    const int routeDelayMs = config.value(QStringLiteral("fake_route_delay_ms")).toInt();
    QTcpServer server;
    const quint16 port = static_cast<quint16>(config.value(QStringLiteral("fake_service_port")).toInt());
    if (!server.listen(QHostAddress::LocalHost, port)) {
        return 2;
    }
    QObject::connect(&server, &QTcpServer::newConnection, &application,
                     [&server, routeBody, routeDelayMs]() {
        while (QTcpSocket *socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::readyRead, socket,
                             [socket, routeBody, routeDelayMs]() {
                const QByteArray request = socket->property("request").toByteArray()
                    + socket->readAll();
                socket->setProperty("request", request);
                if (!request.contains("\r\n\r\n")
                    || socket->property("responseScheduled").toBool()) {
                    return;
                }
                socket->setProperty("responseScheduled", true);
                const QByteArray body = request.startsWith("GET /route?")
                    ? routeBody : QByteArrayLiteral("{\"version\":\"test\"}");
                const int delayMs = request.startsWith("GET /route?") ? routeDelayMs : 0;
                QTimer::singleShot(delayMs, socket, [socket, body]() {
                    socket->write(QByteArrayLiteral(
                                      "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: ")
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

        service.stop();
        QTRY_COMPARE_WITH_TIMEOUT(service.state(), ValhallaServiceManager::State::Stopped, 3000);
    }

    void managedServiceRejectsUnsafeConfiguration()
    {
        ValhallaServiceManager service;
        service.start(QStringLiteral("/missing/valhalla_service"),
                      QStringLiteral("/missing/valhalla.json"), 8002);
        QCOMPARE(service.state(), ValhallaServiceManager::State::Error);
        QVERIFY(service.errorMessage().contains(QStringLiteral("executable")));

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
        QVERIFY(controller.routePath().size() > 200);

        QSignalSpy focusSpy(&controller, &RoutingController::focusCoordinateRequested);
        controller.focusManeuver(4);
        QCOMPARE(focusSpy.size(), 1);
        QVERIFY(focusSpy.takeFirst().at(0).value<QGeoCoordinate>().isValid());

        controller.selectMapPoint(QGeoCoordinate(21.0300, 105.8500));
        QVERIFY(!controller.routeAvailable());
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
        QCOMPARE(controller.sourceLabel(), QStringLiteral("fixture/mô phỏng"));
        QCOMPARE(routeChangedSpy.size(), 1);
        controller.startBackend(QCoreApplication::applicationFilePath(), configPath, port);
        QTRY_COMPARE_WITH_TIMEOUT(controller.sourceLabel(), QStringLiteral("Valhalla local"), 4000);
        QCOMPARE(routeChangedSpy.size(), 1);
        QCOMPARE(controller.connectionLabel(), QStringLiteral("Valhalla local — loopback"));
        QVERIFY(controller.routeAvailable());
        QVERIFY(!controller.loading());
        controller.stopBackend();
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
        controller.startBackend(QCoreApplication::applicationFilePath(), configPath, port);
        QTRY_VERIFY_WITH_TIMEOUT(controller.loading(), 3000);

        QVERIFY(controller.setDestinationInput(QStringLiteral("Ba Đình")));
        QVERIFY(!controller.loading());
        QVERIFY(!controller.routeAvailable());
        QTest::qWait(700);

        QVERIFY(!controller.routeAvailable());
        QCOMPARE(controller.destinationInput(), QStringLiteral("Ba Đình"));
        QVERIFY(controller.sourceLabel() != QStringLiteral("Valhalla local"));
        controller.stopBackend();
    }
};

int main(int argc, char **argv)
{
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
