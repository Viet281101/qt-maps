#include "RoutingController.h"

#include "ValhallaRouteParser.h"
#include "ValhallaRouteClient.h"

#include <QFile>
#include <QDebug>
#include <QLocale>
#include <QSet>
#include <QVariantMap>

namespace qtmaps {
namespace routing {
namespace {

const QGeoCoordinate kHoanKiem(21.0285, 105.8542);
const QGeoCoordinate kThaiHa(21.0116, 105.8197);
const QGeoCoordinate kBaDinh(21.0369, 105.8340);

QString coordinateText(const QGeoCoordinate &coordinate)
{
    return QStringLiteral("%1, %2")
        .arg(coordinate.latitude(), 0, 'f', 6)
        .arg(coordinate.longitude(), 0, 'f', 6);
}

} // namespace

RoutingController::RoutingController(QObject *parent)
    : QObject(parent), originInput_(QStringLiteral("Hoàn Kiếm")),
      destinationInput_(QStringLiteral("Thái Hà")), origin_(kHoanKiem), destination_(kThaiHa),
      service_(this), client_(new ValhallaRouteClient(this))
{
    connect(&service_, &ValhallaServiceManager::stateChanged,
            this, &RoutingController::updateServiceState);
    connect(&service_, &ValhallaServiceManager::ready, this, [this]() {
        QString error;
        if (!client_->setEndpoint(service_.endpoint(), &error)) {
            setStatus(error);
            return;
        }
        emit backendReady();
        calculateRoute();
    });
    connect(&service_, &ValhallaServiceManager::stopped,
            this, &RoutingController::backendStopped);
    connect(client_, &ValhallaRouteClient::routeReady, this, [this](const Route &route) {
        const bool completedReroute = rerouting_;
        rerouting_ = false;
        setLoading(false);
        applyRoute(route, QStringLiteral("Valhalla local"));
        qInfo().noquote() << "Valhalla route applied:" << route.distanceMeters
                          << "m," << route.durationSeconds << "s,"
                          << route.maneuvers.size() << "maneuvers";
        setStatus(completedReroute ? QStringLiteral("Đã tính lại tuyến bằng Valhalla local")
                                   : QStringLiteral("Đã tính tuyến bằng Valhalla local"));
        if (completedReroute) {
            emit rerouteSucceeded();
        }
    });
    connect(client_, &ValhallaRouteClient::routeFailed, this, [this](const QString &error) {
        setLoading(false);
        if (rerouting_) {
            rerouting_ = false;
            setStatus(QStringLiteral("Không thể tính lại tuyến: %1").arg(error));
            return;
        }
        loadFixtureRoute(QStringLiteral("%1; đã chuyển sang fixture").arg(error));
    });
}

QString RoutingController::originInput() const { return originInput_; }
QString RoutingController::destinationInput() const { return destinationInput_; }
QGeoCoordinate RoutingController::origin() const { return origin_; }
QGeoCoordinate RoutingController::destination() const { return destination_; }
QVariantList RoutingController::routePath() const { return routePath_; }
QVariantList RoutingController::maneuvers() const { return maneuvers_; }
QVariantList RoutingController::streetLabels() const { return streetLabels_; }
double RoutingController::distanceMeters() const { return route_.distanceMeters; }
double RoutingController::durationSeconds() const { return route_.durationSeconds; }
QString RoutingController::sourceLabel() const { return sourceLabel_; }
QString RoutingController::connectionLabel() const { return connectionLabel_; }
QString RoutingController::statusMessage() const { return statusMessage_; }
bool RoutingController::routeAvailable() const { return route_.isValid(); }
bool RoutingController::loading() const { return loading_; }
QString RoutingController::nextMapSelection() const
{
    return selectOriginNext_ ? QStringLiteral("điểm đi") : QStringLiteral("điểm đến");
}
int RoutingController::currentManeuverIndex() const { return currentManeuverIndex_; }
const Route &RoutingController::route() const { return route_; }

bool RoutingController::setOriginInput(const QString &input)
{
    QGeoCoordinate coordinate;
    QString canonical;
    if (!parseLocation(input, &coordinate, &canonical)) {
        setStatus(QStringLiteral("Điểm đi không hợp lệ; dùng tên mẫu hoặc vĩ độ, kinh độ"));
        return false;
    }
    origin_ = coordinate;
    originInput_ = canonical;
    selectOriginNext_ = false;
    clearRoute();
    emit endpointsChanged();
    setStatus(QStringLiteral("Đã cập nhật điểm đi"));
    return true;
}

bool RoutingController::setDestinationInput(const QString &input)
{
    QGeoCoordinate coordinate;
    QString canonical;
    if (!parseLocation(input, &coordinate, &canonical)) {
        setStatus(QStringLiteral("Điểm đến không hợp lệ; dùng tên mẫu hoặc vĩ độ, kinh độ"));
        return false;
    }
    destination_ = coordinate;
    destinationInput_ = canonical;
    selectOriginNext_ = true;
    clearRoute();
    emit endpointsChanged();
    setStatus(QStringLiteral("Đã cập nhật điểm đến"));
    return true;
}

void RoutingController::selectMapPoint(const QGeoCoordinate &coordinate)
{
    if (!coordinate.isValid()) {
        setStatus(QStringLiteral("Không thể chọn tọa độ bản đồ này"));
        return;
    }
    if (selectOriginNext_) {
        origin_ = coordinate;
        originInput_ = coordinateText(coordinate);
    } else {
        destination_ = coordinate;
        destinationInput_ = coordinateText(coordinate);
    }
    selectOriginNext_ = !selectOriginNext_;
    clearRoute();
    emit endpointsChanged();
    setStatus(QStringLiteral("Đã chọn trên bản đồ; tiếp theo chọn %1").arg(nextMapSelection()));
}

void RoutingController::calculateFixtureRoute()
{
    clearRoute();
    loadFixtureRoute();
}

void RoutingController::calculateRoute()
{
    clearRoute();
    if (service_.state() == ValhallaServiceManager::State::Ready) {
        setLoading(true);
        setStatus(QStringLiteral("Đang tính tuyến bằng Valhalla local…"));
        client_->requestRoute(origin_, destination_);
        return;
    }
    const QString reason = service_.state() == ValhallaServiceManager::State::Error
        ? QStringLiteral("Valhalla chưa chạy: %1").arg(service_.errorMessage())
        : QStringLiteral("Valhalla chưa chạy: dịch vụ chưa sẵn sàng");
    setStatus(reason);
}

void RoutingController::requestReroute(const QGeoCoordinate &position)
{
    if (!position.isValid()) {
        setStatus(QStringLiteral("Không thể tính lại tuyến: vị trí lệch không hợp lệ"));
        return;
    }
    if (service_.state() != ValhallaServiceManager::State::Ready) {
        setStatus(QStringLiteral("Không thể tính lại tuyến: Valhalla chưa sẵn sàng"));
        return;
    }
    client_->cancel();
    rerouting_ = true;
    setLoading(true);
    setStatus(QStringLiteral("Đang tính lại tuyến bằng Valhalla local…"));
    client_->requestRoute(position, destination_);
}

void RoutingController::startBackend(const QString &program, const QString &configPath,
                                     quint16 port)
{
    connectionLabel_ = QStringLiteral("đang khởi động Valhalla local");
    emit stateChanged();
    service_.start(program, configPath, port, 2, 10000);
    if (service_.state() == ValhallaServiceManager::State::Error) {
        qCritical().noquote() << statusMessage_;
    }
}

void RoutingController::stopBackend()
{
    client_->cancel();
    service_.stop();
}

void RoutingController::forceStopBackend()
{
    client_->cancel();
    service_.forceStop();
}

bool RoutingController::backendRunning() const { return service_.isRunning(); }
qint64 RoutingController::backendProcessId() const { return service_.processId(); }

bool RoutingController::loadFixtureRoute(const QString &reason)
{
    if (!origin_.isValid() || !destination_.isValid()) {
        setStatus(QStringLiteral("Cần đủ điểm đi và điểm đến hợp lệ"));
        return false;
    }
    if (origin_.distanceTo(kHoanKiem) > 250.0) {
        setStatus(QStringLiteral("%1%2Fixture chỉ hỗ trợ điểm đi Hoàn Kiếm")
                      .arg(reason, reason.isEmpty() ? QString() : QStringLiteral("; ")));
        return false;
    }
    QString resource;
    if (destination_.distanceTo(kThaiHa) <= 500.0) {
        resource = QStringLiteral(":/fixtures/routing/hanoi_hoan_kiem_to_thai_ha.json");
    } else if (destination_.distanceTo(kBaDinh) <= 500.0) {
        resource = QStringLiteral(":/fixtures/routing/hanoi_hoan_kiem_to_ba_dinh.json");
    } else {
        setStatus(QStringLiteral("%1%2Fixture chỉ có tuyến Thái Hà hoặc Ba Đình")
                      .arg(reason, reason.isEmpty() ? QString() : QStringLiteral("; ")));
        return false;
    }

    QFile file(resource);
    if (!file.open(QIODevice::ReadOnly)) {
        setStatus(QStringLiteral("Không thể đọc route fixture đã đóng gói"));
        return false;
    }
    const RouteParseResult parsed = ValhallaRouteParser::parse(file.readAll(), RouteSource::Fixture);
    if (!parsed.ok()) {
        setStatus(QStringLiteral("Route fixture lỗi: %1").arg(parsed.error));
        return false;
    }
    applyRoute(parsed.route, QStringLiteral("fixture/mô phỏng"));
    connectionLabel_ = reason.isEmpty() ? QStringLiteral("offline — fixture")
                                        : QStringLiteral("offline — fixture fallback");
    emit stateChanged();
    setStatus(reason.isEmpty() ? QStringLiteral("Đã nạp tuyến fixture offline") : reason);
    return true;
}

void RoutingController::focusManeuver(int index)
{
    if (index < 0 || index >= route_.maneuvers.size()) {
        setStatus(QStringLiteral("Bước chỉ đường không hợp lệ"));
        return;
    }
    if (currentManeuverIndex_ != index) {
        currentManeuverIndex_ = index;
        emit currentManeuverChanged();
    }
    emit focusCoordinateRequested(route_.maneuvers.at(index).position, 16.5);
}

bool RoutingController::parseLocation(const QString &input, QGeoCoordinate *coordinate,
                                      QString *canonicalInput)
{
    const QString trimmed = input.trimmed();
    if (trimmed.compare(QStringLiteral("Hoàn Kiếm"), Qt::CaseInsensitive) == 0) {
        *coordinate = kHoanKiem;
        *canonicalInput = QStringLiteral("Hoàn Kiếm");
        return true;
    }
    if (trimmed.compare(QStringLiteral("Thái Hà"), Qt::CaseInsensitive) == 0) {
        *coordinate = kThaiHa;
        *canonicalInput = QStringLiteral("Thái Hà");
        return true;
    }
    if (trimmed.compare(QStringLiteral("Ba Đình"), Qt::CaseInsensitive) == 0) {
        *coordinate = kBaDinh;
        *canonicalInput = QStringLiteral("Ba Đình");
        return true;
    }
    const QStringList parts = trimmed.split(QLatin1Char(','));
    if (parts.size() != 2) {
        return false;
    }
    bool latitudeOk = false;
    bool longitudeOk = false;
    const double latitude = QLocale::c().toDouble(parts.at(0).trimmed(), &latitudeOk);
    const double longitude = QLocale::c().toDouble(parts.at(1).trimmed(), &longitudeOk);
    const QGeoCoordinate parsed(latitude, longitude);
    if (!latitudeOk || !longitudeOk || !parsed.isValid()) {
        return false;
    }
    *coordinate = parsed;
    *canonicalInput = coordinateText(parsed);
    return true;
}

void RoutingController::clearRoute()
{
    client_->cancel();
    rerouting_ = false;
    setLoading(false);
    if (currentManeuverIndex_ != -1) {
        currentManeuverIndex_ = -1;
        emit currentManeuverChanged();
    }
    if (!route_.isValid() && routePath_.isEmpty() && maneuvers_.isEmpty()) {
        return;
    }
    route_ = Route();
    routePath_.clear();
    maneuvers_.clear();
    streetLabels_.clear();
    sourceLabel_ = QStringLiteral("fixture/mô phỏng");
    emit routeChanged();
}

void RoutingController::applyRoute(const Route &route, const QString &sourceLabel)
{
    if (currentManeuverIndex_ != -1) {
        currentManeuverIndex_ = -1;
        emit currentManeuverChanged();
    }
    if (route_.isValid() && route_.polyline == route.polyline
        && route_.maneuvers.size() == route.maneuvers.size()
        && route_.distanceMeters == route.distanceMeters
        && route_.durationSeconds == route.durationSeconds) {
        route_ = route;
        sourceLabel_ = sourceLabel;
        emit routeChanged();
        emit stateChanged();
        return;
    }
    route_ = route;
    sourceLabel_ = sourceLabel;
    routePath_.clear();
    for (const QGeoCoordinate &coordinate : route.polyline) {
        routePath_.append(QVariant::fromValue(coordinate));
    }
    maneuvers_.clear();
    streetLabels_.clear();
    QSet<QString> labeledStreets;
    for (const RouteManeuver &maneuver : route.maneuvers) {
        QVariantMap item;
        item.insert(QStringLiteral("type"), maneuver.type);
        item.insert(QStringLiteral("instruction"), maneuver.instruction);
        item.insert(QStringLiteral("streetName"), maneuver.streetName);
        item.insert(QStringLiteral("distanceMeters"), maneuver.distanceMeters);
        item.insert(QStringLiteral("durationSeconds"), maneuver.durationSeconds);
        item.insert(QStringLiteral("coordinate"), QVariant::fromValue(maneuver.position));
        maneuvers_.append(item);
        const QString streetName = maneuver.streetName.trimmed();
        if (!streetName.isEmpty() && !labeledStreets.contains(streetName)) {
            QVariantMap label;
            label.insert(QStringLiteral("streetName"), streetName);
            label.insert(QStringLiteral("coordinate"),
                         QVariant::fromValue(maneuver.position));
            streetLabels_.append(label);
            labeledStreets.insert(streetName);
        }
    }
    emit routeChanged();
    emit stateChanged();
}

void RoutingController::setStatus(const QString &status)
{
    if (statusMessage_ == status) {
        return;
    }
    statusMessage_ = status;
    emit stateChanged();
}

void RoutingController::setLoading(bool loading)
{
    if (loading_ == loading) {
        return;
    }
    loading_ = loading;
    emit stateChanged();
}

void RoutingController::updateServiceState()
{
    switch (service_.state()) {
    case ValhallaServiceManager::State::Starting:
        connectionLabel_ = QStringLiteral("đang khởi động Valhalla local");
        break;
    case ValhallaServiceManager::State::Ready:
        connectionLabel_ = QStringLiteral("Valhalla local — loopback");
        break;
    case ValhallaServiceManager::State::Stopping:
        connectionLabel_ = QStringLiteral("đang dừng Valhalla local");
        break;
    case ValhallaServiceManager::State::Error:
        connectionLabel_ = QStringLiteral("Valhalla chưa chạy: %1")
                               .arg(service_.errorMessage());
        setStatus(connectionLabel_);
        break;
    case ValhallaServiceManager::State::Stopped:
        connectionLabel_ = QStringLiteral("offline — fixture");
        break;
    }
    emit stateChanged();
}

} // namespace routing
} // namespace qtmaps
