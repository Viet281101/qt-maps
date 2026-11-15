#pragma once

#include <QObject>

class QTranslator;
class QQmlApplicationEngine;

namespace qtmaps { namespace core {
class AppPreferences final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
public:
    explicit AppPreferences(QQmlApplicationEngine *engine, QObject *parent = nullptr);
    QString language() const;
    Q_INVOKABLE void setLanguage(const QString &language);
signals:
    void languageChanged();
private:
    void applyLanguage();
    QQmlApplicationEngine *engine_ = nullptr;
    QTranslator *translator_ = nullptr;
    QString language_;
};
} }
