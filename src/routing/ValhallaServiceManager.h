#pragma once

#include <QObject>
#include <QProcess>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace qtmaps {
namespace routing {

class ValhallaServiceManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(QUrl endpoint READ endpoint NOTIFY stateChanged)

public:
    enum class State {
        Stopped,
        Starting,
        Ready,
        Stopping,
        Error,
    };
    Q_ENUM(State)

    explicit ValhallaServiceManager(QObject *parent = nullptr);
    ~ValhallaServiceManager() override;

    void start(const QString &program, const QString &configPath, quint16 port,
               int concurrency = 2, int readinessTimeoutMs = 10000);
    void stop();

    State state() const;
    QString errorMessage() const;
    QUrl endpoint() const;

signals:
    void stateChanged();
    void ready();

private:
    void setState(State state, const QString &error = QString());
    void scheduleHealthCheck();
    void probeHealth();
    void fail(const QString &error);

    QProcess process_;
    QNetworkAccessManager *network_ = nullptr;
    QTimer *healthTimer_ = nullptr;
    QTimer *startupTimer_ = nullptr;
    QTimer *killTimer_ = nullptr;
    QNetworkReply *healthReply_ = nullptr;
    State state_ = State::Stopped;
    QString errorMessage_;
    QUrl endpoint_;
};

} // namespace routing
} // namespace qtmaps

