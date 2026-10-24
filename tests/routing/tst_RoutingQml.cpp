#include <QtTest>

#include <QColor>
#include <QQmlComponent>
#include <QQmlEngine>

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
};

QTEST_MAIN(RoutingQmlTest)

#include "tst_RoutingQml.moc"
