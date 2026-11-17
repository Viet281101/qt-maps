#include "core/PackageReadinessController.h"

#include <QtTest>

class PackageReadinessControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void missingPackagesAreReportedWithoutBlockingStartup()
    {
        qtmaps::core::PackageReadinessController controller(
            QStringLiteral("/definitely-missing/vector"),
            QStringLiteral("/definitely-missing/raster"),
            QStringLiteral("/definitely-missing/search"),
            QStringLiteral("/definitely-missing/orthography"));

        QTRY_COMPARE(controller.state(), QStringLiteral("error"));
        QCOMPARE(controller.items().size(), 4);
        const QVariantMap vectorItem = controller.items().at(0).toMap();
        QCOMPARE(vectorItem.value(QStringLiteral("role")).toString(), QStringLiteral("vector"));
        QVERIFY(vectorItem.value(QStringLiteral("required")).toBool());
        QVERIFY(!vectorItem.value(QStringLiteral("available")).toBool());
        QCOMPARE(vectorItem.value(QStringLiteral("reason")).toString(), QStringLiteral("missing"));
    }

    void timeoutIgnoresLateValidationResult()
    {
        qtmaps::core::PackageReadinessController controller(
            QStringLiteral("/definitely-missing/vector"), QString(), QString(), QString());
        QVERIFY(QMetaObject::invokeMethod(&controller, "markTimedOut"));
        QCOMPARE(controller.state(), QStringLiteral("timed-out"));
        QTest::qWait(100);
        QCOMPARE(controller.state(), QStringLiteral("timed-out"));
        QVERIFY(controller.items().isEmpty());
    }
};

QTEST_MAIN(PackageReadinessControllerTest)

#include "tst_PackageReadinessController.moc"
