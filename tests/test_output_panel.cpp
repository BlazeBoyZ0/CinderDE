#include <QtTest>
#include <QSignalSpy>
#include "cinderde/output_panel.h"

class TestOutputPanel : public QObject {
    Q_OBJECT

private slots:
    void testOutputLoggingAndClear() {
        CinderDE::OutputPanel panel;
        panel.appendBuildOutput("Building target...\n");
        panel.appendRunOutput("Hello World\n");
        panel.appendSystemMessage("Process finished\n");

        // Test clear
        panel.clearOutput();
    }

    void testProcessRunningState() {
        CinderDE::OutputPanel panel;
        panel.setProcessRunning(true, "Building...");
        panel.setProcessRunning(false);
    }

    void testStdinSignal() {
        CinderDE::OutputPanel panel;
        QSignalSpy spy(&panel, &CinderDE::OutputPanel::stdinSubmitted);

        // Find stdin line edit and send
        QLineEdit *stdinEdit = panel.findChild<QLineEdit*>();
        QVERIFY(stdinEdit != nullptr);

        stdinEdit->setText("user input");
        QMetaObject::invokeMethod(stdinEdit, "returnPressed");

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), QString("user input\n"));
    }
};

QTEST_MAIN(TestOutputPanel)
#include "test_output_panel.moc"
