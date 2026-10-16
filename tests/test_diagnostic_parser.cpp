#include <QtTest>
#include "cinderde/diagnostic_parser.h"

class TestDiagnosticParser : public QObject {
    Q_OBJECT

private slots:
    void testGccErrorWithCol() {
        QString line = "main.cpp:10:5: error: 'x' was not declared in this scope";
        auto diag = CinderDE::DiagnosticParser::parseLine(line, "/home/user/project");
        QVERIFY(diag.has_value());
        QCOMPARE(diag->filePath, QString("/home/user/project/main.cpp"));
        QCOMPARE(diag->line, 10);
        QCOMPARE(diag->column, 5);
        QCOMPARE(diag->level, CinderDE::DiagnosticItem::Error);
        QCOMPARE(diag->message, QString("'x' was not declared in this scope"));
    }

    void testGccWarningNoCol() {
        QString line = "src/foo.cpp:42: warning: unused variable 'y'";
        auto diag = CinderDE::DiagnosticParser::parseLine(line, "/home/user/project");
        QVERIFY(diag.has_value());
        QCOMPARE(diag->filePath, QString("/home/user/project/src/foo.cpp"));
        QCOMPARE(diag->line, 42);
        QCOMPARE(diag->column, -1);
        QCOMPARE(diag->level, CinderDE::DiagnosticItem::Warning);
        QCOMPARE(diag->message, QString("unused variable 'y'"));
    }

    void testClangFatalError() {
        QString line = "/usr/include/math.h:100:10: fatal error: 'stddef.h' file not found";
        auto diag = CinderDE::DiagnosticParser::parseLine(line);
        QVERIFY(diag.has_value());
        QCOMPARE(diag->filePath, QString("/usr/include/math.h"));
        QCOMPARE(diag->line, 100);
        QCOMPARE(diag->column, 10);
        QCOMPARE(diag->level, CinderDE::DiagnosticItem::Error);
    }

    void testNonDiagnosticLine() {
        QString line = "Building CXX object CMakeFiles/cinderde.dir/src/main.cpp.o";
        auto diag = CinderDE::DiagnosticParser::parseLine(line);
        QVERIFY(!diag.has_value());
    }

    void testParseMultipleLines() {
        QString output = "Building target\n"
                         "main.cpp:5:1: error: expected ';' before 'return'\n"
                         "some random warning message without line\n"
                         "utils.cpp:12: warning: comparison between signed and unsigned\n";
        auto list = CinderDE::DiagnosticParser::parseOutput(output, "/app");
        QCOMPARE(list.size(), 2);
        QCOMPARE(list[0].line, 5);
        QCOMPARE(list[0].level, CinderDE::DiagnosticItem::Error);
        QCOMPARE(list[1].line, 12);
        QCOMPARE(list[1].level, CinderDE::DiagnosticItem::Warning);
    }
};

QTEST_MAIN(TestDiagnosticParser)
#include "test_diagnostic_parser.moc"
