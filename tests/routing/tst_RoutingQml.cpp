#include <QtTest>

#include <QColor>
#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>

class RoutingQmlTest final : public QObject
{
    Q_OBJECT

private slots:
    void maneuverRowHasPersistentSelectedStyle()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine,
                                QUrl(QStringLiteral("qrc:/qml/components/ManeuverRow.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> row(component.create());
        QVERIFY2(row, qPrintable(component.errorString()));
        row->setProperty("maneuverIndex", 3);
        row->setProperty("maneuver",
                         QVariantMap{{QStringLiteral("instruction"), QStringLiteral("Rẽ phải")},
                                     {QStringLiteral("distanceMeters"), 250.0}});

        row->setProperty("selected", false);
        const QColor normal = row->property("color").value<QColor>();
        row->setProperty("selected", true);
        const QColor selected = row->property("color").value<QColor>();

        QVERIFY(normal.isValid());
        QVERIFY(selected.isValid());
        QVERIFY(normal != selected);
        QCOMPARE(row->property("selected").toBool(), true);
    }

    void keyboardActionButtonSupportsTabReturnAndSpace()
    {
        QQmlEngine engine;
        QQmlComponent component(
            &engine, QUrl(QStringLiteral("qrc:/qml/components/KeyboardActionButton.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> button(component.create());
        QVERIFY2(button, qPrintable(component.errorString()));
        QQuickItem *item = qobject_cast<QQuickItem *>(button.data());
        QVERIFY(item);
        QVERIFY(item->activeFocusOnTab());

        QQuickWindow window;
        item->setParentItem(window.contentItem());
        window.show();
        item->forceActiveFocus();
        QVERIFY(item->hasActiveFocus());
        QSignalSpy triggered(button.data(), SIGNAL(triggered()));
        QTest::keyClick(&window, Qt::Key_Return);
        QTest::keyClick(&window, Qt::Key_Space);
        QCOMPARE(triggered.size(), 2);
    }

    void routingProbeDeclaresExpectedTabOrder()
    {
        QFile file(QStringLiteral(":/qml/RoutingProbe.qml"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray qml = file.readAll();
        QVERIFY(qml.contains("KeyNavigation.tab: destinationField"));
        QVERIFY(qml.contains("KeyNavigation.tab: applyButton"));
        QVERIFY(qml.contains("KeyNavigation.tab: routeButton"));
        QVERIFY(qml.contains("KeyNavigation.tab: originField"));
    }

    void routingProbeKeepsRouteClearOfPanelAndExposesUiState()
    {
        QFile file(QStringLiteral(":/qml/RoutingProbe.qml"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray qml = file.readAll();
        QVERIFY(qml.contains("routePanel.width + Theme.spaceLg * 2"));
        QVERIFY(qml.contains("property bool panelCollapsed"));
        QVERIFY(qml.contains("routingController.streetLabels"));
        QVERIFY(qml.contains("map.zoomLevel >= 15"));
        QVERIFY(qml.contains("routingController.currentManeuverIndex"));
        QVERIFY(qml.contains("routeSummaryColor"));
        QVERIFY(qml.contains("shutdownCoordinator.requestQuit()"));
    }
};

QTEST_MAIN(RoutingQmlTest)

#include "tst_RoutingQml.moc"
