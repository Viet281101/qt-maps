#include "AppPreferences.h"
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QTranslator>
namespace qtmaps {
namespace core {
AppPreferences::AppPreferences(QQmlApplicationEngine *engine, QObject *parent) :
    QObject(parent), engine_(engine), translator_(new QTranslator(this))
{
    language_ =
        QSettings().value(QStringLiteral("preferences/language"), QStringLiteral("vi")).toString();
    if (language_ != QStringLiteral("en"))
        language_ = QStringLiteral("vi");
    applyLanguage();
}
QString AppPreferences::language() const { return language_; }
void AppPreferences::setLanguage(const QString &language)
{
    const QString normalized = language == QStringLiteral("en") ? language : QStringLiteral("vi");
    if (language_ == normalized)
        return;
    language_ = normalized;
    QSettings().setValue(QStringLiteral("preferences/language"), language_);
    applyLanguage();
    emit languageChanged();
}
void AppPreferences::applyLanguage()
{
    QCoreApplication::removeTranslator(translator_);
    delete translator_;
    translator_ = new QTranslator(this);
    if (language_ == QStringLiteral("en"))
        translator_->load(QStringLiteral(":/i18n/qtmaps_en.qm"));
    QCoreApplication::installTranslator(translator_);
    if (engine_)
        engine_->retranslate();
}
} // namespace core
} // namespace qtmaps
