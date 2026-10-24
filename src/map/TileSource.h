#pragma once

#include <QObject>
#include <QString>

namespace qtmaps {
namespace map {

class TileSource : public QObject
{
    Q_OBJECT

public:
    explicit TileSource(QObject *parent = nullptr) : QObject(parent) {}
    ~TileSource() override = default;

    virtual bool start(const QString &packagePath, quint16 requestedPort = 0) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual QString baseUrl() const = 0;
    virtual QString errorMessage() const = 0;

signals:
    void runningChanged();
    void errorOccurred(const QString &message);
};

} // namespace map
} // namespace qtmaps
