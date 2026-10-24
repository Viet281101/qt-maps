#pragma once

#include <QObject>
#include <QString>

namespace qtmaps {
namespace map {

class CameraProbeController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal requestedBearing READ requestedBearing NOTIFY cameraChanged)
    Q_PROPERTY(qreal requestedTilt READ requestedTilt NOTIFY cameraChanged)
    Q_PROPERTY(QString stepName READ stepName NOTIFY cameraChanged)

public:
    explicit CameraProbeController(QObject *parent = nullptr);

    qreal requestedBearing() const;
    qreal requestedTilt() const;
    QString stepName() const;

    Q_INVOKABLE void advance(qreal minimumTilt, qreal maximumTilt);
    Q_INVOKABLE void reset();

    static qreal normalizedBearing(qreal bearing);
    static qreal boundedTilt(qreal tilt, qreal minimumTilt, qreal maximumTilt);

signals:
    void cameraChanged();

private:
    void selectStep(int index, qreal minimumTilt, qreal maximumTilt);

    int stepIndex_ = 0;
    qreal requestedBearing_ = 0.0;
    qreal requestedTilt_ = 0.0;
    QString stepName_ = QStringLiteral("north-reset");
};

} // namespace map
} // namespace qtmaps
