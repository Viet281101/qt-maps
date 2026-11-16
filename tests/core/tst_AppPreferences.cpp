#include "core/AppPreferences.h"
#include "core/PackageReadinessController.h"

#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class AppPreferencesTest final : public QObject
{
    Q_OBJECT

private slots:
    void languageChoicePersistsAndNormalizes()
    {
        QTemporaryDir settingsDirectory;
        QVERIFY(settingsDirectory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("QtMapsTest"));
        QCoreApplication::setApplicationName(QStringLiteral("AppPreferencesTest"));
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settingsDirectory.path());
        QSettings settings;
        settings.clear();

        {
            qtmaps::core::AppPreferences first(nullptr);
            QCOMPARE(first.language(), QStringLiteral("vi"));
            QCOMPARE(QCoreApplication::translate("SettingsOverlay", "Ngôn ngữ"),
                     QStringLiteral("Ngôn ngữ"));
            QSignalSpy changed(&first, &qtmaps::core::AppPreferences::languageChanged);
            first.setLanguage(QStringLiteral("en"));
            QCOMPARE(first.language(), QStringLiteral("en"));
            QCOMPARE(QCoreApplication::translate("SettingsOverlay", "Ngôn ngữ"),
                     QStringLiteral("Language"));
            QCOMPARE(QCoreApplication::translate("SettingsOverlay", "Dữ liệu cục bộ"),
                     QStringLiteral("Local data"));
            QCOMPARE(changed.count(), 1);
        }

        qtmaps::core::AppPreferences restored(nullptr);
        QCOMPARE(restored.language(), QStringLiteral("en"));
        restored.setLanguage(QStringLiteral("unsupported"));
        QCOMPARE(restored.language(), QStringLiteral("vi"));
        QCOMPARE(QCoreApplication::translate("SettingsOverlay", "Ngôn ngữ"),
                 QStringLiteral("Ngôn ngữ"));
        QCOMPARE(QSettings().value(QStringLiteral("preferences/language")).toString(),
                 QStringLiteral("vi"));
    }

    void qmlRetranslatesImmediately()
    {
        QQmlApplicationEngine engine;
        qtmaps::core::AppPreferences preferences(&engine);
        preferences.setLanguage(QStringLiteral("vi"));
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.13\nItem { property string label: "
                          "qsTranslate('SettingsOverlay', 'Ngôn ngữ') }",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> item(component.create());
        QVERIFY(item);
        QCOMPARE(item->property("label").toString(), QStringLiteral("Ngôn ngữ"));
        preferences.setLanguage(QStringLiteral("en"));
        QTRY_COMPARE(item->property("label").toString(), QStringLiteral("Language"));
        preferences.setLanguage(QStringLiteral("vi"));
        QTRY_COMPARE(item->property("label").toString(), QStringLiteral("Ngôn ngữ"));
    }

    void settingsButtonOpensAndEscapeDismissesPanel()
    {
        QQmlApplicationEngine engine;
        qtmaps::core::AppPreferences preferences(&engine);
        qtmaps::core::PackageReadinessController readiness{QString(), QString(), QString(),
                                                           QString()};
        engine.rootContext()->setContextProperty(QStringLiteral("appPreferences"), &preferences);
        engine.rootContext()->setContextProperty(QStringLiteral("packageReadiness"), &readiness);
        QQmlComponent component(&engine,
                                QUrl(QStringLiteral("qrc:/qml/components/SettingsOverlay.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> panel(component.create());
        QVERIFY2(panel, qPrintable(component.errorString()));
        QQuickItem *item = qobject_cast<QQuickItem *>(panel.data());
        QVERIFY(item);
        item->setWidth(900);
        item->setHeight(580);
        QQuickWindow window;
        item->setParentItem(window.contentItem());
        window.show();
        QObject *button = panel->findChild<QObject *>(QStringLiteral("settingsButton"));
        QVERIFY(button);
        QCOMPARE(button->property("width").toInt(), 40);
        QVERIFY(!panel->property("open").toBool());
        QVERIFY(QMetaObject::invokeMethod(button, "triggered"));
        QTRY_VERIFY(panel->property("open").toBool());
        QObject *settingsPanel = panel->findChild<QObject *>(QStringLiteral("settingsPanel"));
        QVERIFY(settingsPanel);
        QVERIFY(settingsPanel->property("y").toReal() +
                    settingsPanel->property("height").toReal() <=
                item->height());
        QTest::keyClick(&window, Qt::Key_Escape);
        QTRY_VERIFY(!panel->property("open").toBool());
    }
};

QTEST_MAIN(AppPreferencesTest)

#include "tst_AppPreferences.moc"
