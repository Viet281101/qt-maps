#include "ValhallaServiceManager.h"

#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include <csignal>
#if defined(Q_OS_LINUX)
#include <sys/prctl.h>
#include <unistd.h>
#endif

namespace qtmaps {
namespace routing {

namespace {

class ValhallaProcess final : public QProcess
{
public:
    using QProcess::QProcess;

protected:
    void setupChildProcess() override
    {
#if defined(Q_OS_LINUX)
        if (::prctl(PR_SET_PDEATHSIG, SIGTERM) == -1 || ::getppid() == 1) {
            ::_exit(127);
        }
#endif
    }
};

} // namespace

ValhallaServiceManager::ValhallaServiceManager(QObject *parent)
    : QObject(parent), process_(new ValhallaProcess(this)),
      network_(new QNetworkAccessManager(this)),
      healthTimer_(new QTimer(this)), startupTimer_(new QTimer(this)),
      killTimer_(new QTimer(this))
{
    healthTimer_->setSingleShot(true);
    healthTimer_->setInterval(100);
    startupTimer_->setSingleShot(true);
    killTimer_->setSingleShot(true);

    connect(healthTimer_, &QTimer::timeout, this, &ValhallaServiceManager::probeHealth);
    connect(startupTimer_, &QTimer::timeout, this, [this]() {
        fail(QStringLiteral("Valhalla readiness check timed out"));
    });
    connect(killTimer_, &QTimer::timeout, this, [this]() {
        if (process_->state() != QProcess::NotRunning) {
            process_->kill();
        }
    });
    connect(process_, &QProcess::started, this, &ValhallaServiceManager::scheduleHealthCheck);
    connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::Crashed || state_ != State::Stopping) {
            fail(QStringLiteral("Valhalla process error: %1").arg(process_->errorString()));
        }
    });
    connect(process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        healthTimer_->stop();
        startupTimer_->stop();
        killTimer_->stop();
        if (healthReply_) {
            healthReply_->abort();
            healthReply_->deleteLater();
            healthReply_ = nullptr;
        }
        if (state_ == State::Stopping) {
            setState(State::Stopped);
            emit stopped();
        } else if (state_ != State::Error) {
            fail(QStringLiteral("Valhalla exited before shutdown (code %1, status %2)")
                     .arg(exitCode)
                     .arg(static_cast<int>(exitStatus)));
        }
    });
    process_->setProcessChannelMode(QProcess::ForwardedChannels);
}

void ValhallaServiceManager::start(const QString &program, const QString &configPath,
                                   quint16 port, int concurrency, int readinessTimeoutMs)
{
    if (process_->state() != QProcess::NotRunning) {
        fail(QStringLiteral("Valhalla process has not stopped yet"));
        return;
    }
    if (state_ != State::Stopped && state_ != State::Error) {
        fail(QStringLiteral("Valhalla is already running or changing state"));
        return;
    }
    const QFileInfo programInfo(program);
    const QFileInfo configInfo(configPath);
    if (!programInfo.isFile()) {
        fail(QStringLiteral("thiếu %1").arg(programInfo.absoluteFilePath()));
        return;
    }
    if (!programInfo.isExecutable()) {
        fail(QStringLiteral("không thể chạy %1").arg(programInfo.absoluteFilePath()));
        return;
    }
    if (!configInfo.isFile()) {
        fail(QStringLiteral("thiếu %1").arg(configInfo.absoluteFilePath()));
        return;
    }
    if (!configInfo.isReadable()) {
        fail(QStringLiteral("không thể đọc %1").arg(configInfo.absoluteFilePath()));
        return;
    }
    if (port == 0) {
        fail(QStringLiteral("Valhalla requires a configured loopback port"));
        return;
    }
    if (concurrency < 1 || concurrency > 2) {
        fail(QStringLiteral("Valhalla concurrency must be between one and two"));
        return;
    }

    QFile configFile(configInfo.absoluteFilePath());
    if (!configFile.open(QIODevice::ReadOnly)) {
        fail(QStringLiteral("Valhalla config cannot be opened"));
        return;
    }
    const QJsonObject config = QJsonDocument::fromJson(configFile.readAll()).object();
    const QString listen = config.value(QStringLiteral("httpd")).toObject()
                               .value(QStringLiteral("service")).toObject()
                               .value(QStringLiteral("listen")).toString();
    const QString requiredListen = QStringLiteral("tcp://127.0.0.1:%1").arg(port);
    if (listen != requiredListen) {
        fail(QStringLiteral("Valhalla config must listen only at %1").arg(requiredListen));
        return;
    }

    endpoint_ = QUrl(QStringLiteral("http://127.0.0.1:%1").arg(port));
    errorMessage_.clear();
    setState(State::Starting);
    startupTimer_->start(qMax(100, readinessTimeoutMs));
    process_->setProgram(programInfo.absoluteFilePath());
    process_->setArguments({configInfo.absoluteFilePath(), QString::number(concurrency)});
    process_->start();
}

void ValhallaServiceManager::stop()
{
    healthTimer_->stop();
    startupTimer_->stop();
    if (healthReply_) {
        healthReply_->abort();
        healthReply_->deleteLater();
        healthReply_ = nullptr;
    }
    if (process_->state() == QProcess::NotRunning) {
        setState(State::Stopped);
        emit stopped();
        return;
    }
    setState(State::Stopping);
#if defined(Q_OS_UNIX)
    ::kill(static_cast<pid_t>(process_->processId()), SIGINT);
#else
    process_->terminate();
#endif
}

void ValhallaServiceManager::forceStop()
{
    healthTimer_->stop();
    startupTimer_->stop();
    killTimer_->stop();
    if (process_->state() == QProcess::NotRunning) {
        setState(State::Stopped);
        emit stopped();
        return;
    }
    setState(State::Stopping);
    process_->kill();
}

ValhallaServiceManager::State ValhallaServiceManager::state() const { return state_; }
bool ValhallaServiceManager::isRunning() const
{
    return process_->state() != QProcess::NotRunning;
}
qint64 ValhallaServiceManager::processId() const { return process_->processId(); }
QString ValhallaServiceManager::errorMessage() const { return errorMessage_; }
QUrl ValhallaServiceManager::endpoint() const { return endpoint_; }

void ValhallaServiceManager::setState(State state, const QString &error)
{
    const bool changed = state_ != state || errorMessage_ != error;
    state_ = state;
    errorMessage_ = error;
    if (changed) {
        emit stateChanged();
    }
}

void ValhallaServiceManager::scheduleHealthCheck()
{
    if (state_ == State::Starting) {
        healthTimer_->start();
    }
}

void ValhallaServiceManager::probeHealth()
{
    if (state_ != State::Starting || healthReply_) {
        return;
    }
    healthReply_ = network_->get(QNetworkRequest(endpoint_.resolved(QUrl(QStringLiteral("/status")))));
    connect(healthReply_, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = healthReply_;
        healthReply_ = nullptr;
        if (!reply) {
            return;
        }
        const QByteArray body = reply->readAll();
        const bool healthy = reply->error() == QNetworkReply::NoError
            && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200
            && !QJsonDocument::fromJson(body).object().value(QStringLiteral("version")).toString().isEmpty();
        reply->deleteLater();
        if (state_ != State::Starting) {
            return;
        }
        if (healthy) {
            startupTimer_->stop();
            setState(State::Ready);
            emit ready();
        } else {
            healthTimer_->start();
        }
    });
}

void ValhallaServiceManager::fail(const QString &error)
{
    healthTimer_->stop();
    startupTimer_->stop();
    setState(State::Error, error);
    if (process_->state() != QProcess::NotRunning) {
        process_->terminate();
        killTimer_->start(1500);
    }
}

} // namespace routing
} // namespace qtmaps
