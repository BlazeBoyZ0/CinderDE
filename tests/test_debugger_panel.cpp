#include <QtTest>
#include <QJsonObject>
#include <QTreeWidget>
#include "cinderde/debugger_panel.h"

using namespace CinderDE;

class DebuggerPanelTest : public QObject {
    Q_OBJECT
private slots:
    void loadsLocalsAndExpandsNestedValues();
};

void DebuggerPanelTest::loadsLocalsAndExpandsNestedValues()
{
    DebuggerPanel panel;
    QSignalSpy expandSpy(&panel, &DebuggerPanel::expandRequested);
    QVERIFY(expandSpy.isValid());

    panel.setScopes(QJsonArray{QJsonObject{
        {QStringLiteral("name"), QStringLiteral("Locals")},
        {QStringLiteral("variablesReference"), 10},
        {QStringLiteral("expensive"), false}
    }});
    QCOMPARE(expandSpy.count(), 1);
    QCOMPARE(expandSpy.takeFirst().at(0).toInt(), 10);

    QTreeWidget *tree = panel.findChild<QTreeWidget *>();
    QVERIFY(tree);
    QTreeWidgetItem *locals = tree->topLevelItem(0);
    QVERIFY(locals);
    panel.populateVariables(10, QStringLiteral("Locals"), QJsonArray{QJsonObject{
        {QStringLiteral("name"), QStringLiteral("player")},
        {QStringLiteral("value"), QStringLiteral("<Player object>")},
        {QStringLiteral("variablesReference"), 20}
    }});

    QTreeWidgetItem *player = locals->child(0);
    QVERIFY(player);
    tree->expandItem(player);
    QCOMPARE(expandSpy.count(), 1);
    QCOMPARE(expandSpy.takeFirst().at(0).toInt(), 20);

    panel.populateVariables(20, QStringLiteral("player"), QJsonArray{QJsonObject{
        {QStringLiteral("name"), QStringLiteral("health")},
        {QStringLiteral("value"), QStringLiteral("100")},
        {QStringLiteral("variablesReference"), 0}
    }});
    QCOMPARE(player->childCount(), 1);
    QCOMPARE(player->child(0)->text(0), QStringLiteral("health"));
    QCOMPARE(player->child(0)->text(1), QStringLiteral("100"));
}

QTEST_MAIN(DebuggerPanelTest)
#include "test_debugger_panel.moc"
