#include "search/PlaceSearchController.h"

#include <QSignalSpy>
#include <QtTest>

class PlaceSearchControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void emptyQueryIsExplicitlyIdle()
    {
        qtmaps::search::PlaceSearchController controller;
        controller.setQuery(QStringLiteral(""));
        QCOMPARE(controller.state(), QStringLiteral("idle"));
        QVERIFY(controller.results().isEmpty());
        QVERIFY(controller.message().contains(QStringLiteral("demo")));
    }

    void matchingQueryReturnsBoundedFixture()
    {
        qtmaps::search::PlaceSearchController controller;
        QSignalSpy changed(&controller, &qtmaps::search::PlaceSearchController::stateChanged);
        controller.setQuery(QStringLiteral("thái"));
        QCOMPARE(controller.state(), QStringLiteral("loading"));
        QTRY_COMPARE(controller.state(), QStringLiteral("ready"));
        QCOMPARE(controller.results().size(), 1);
        QCOMPARE(controller.results().constFirst().toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("Thái Hà"));
        QVERIFY(changed.count() >= 2);

        const int completedChangeCount = changed.count();
        controller.setQuery(QStringLiteral("  thái  "));
        QCOMPARE(changed.count(), completedChangeCount);
    }

    void laterQuerySupersedesPendingQuery()
    {
        qtmaps::search::PlaceSearchController controller;
        controller.setQuery(QStringLiteral("Hoàn"));
        controller.setQuery(QStringLiteral("Ba"));
        QTRY_COMPARE(controller.state(), QStringLiteral("ready"));
        QCOMPARE(controller.results().size(), 1);
        QCOMPARE(controller.results().constFirst().toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("Ba Đình"));
    }

    void noMatchIsExplicit()
    {
        qtmaps::search::PlaceSearchController controller;
        controller.setQuery(QStringLiteral("không tồn tại"));
        QTRY_COMPARE(controller.state(), QStringLiteral("empty"));
        QVERIFY(controller.results().isEmpty());
        QVERIFY(controller.message().contains(QStringLiteral("Không có")));
    }
};

QTEST_MAIN(PlaceSearchControllerTest)

#include "tst_PlaceSearchController.moc"
