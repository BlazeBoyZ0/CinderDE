#include <QtTest>
#include <QTextDocument>
#include <QTextBlock>
#include "cinderde/syntax_highlighter.h"
#include "cinderde/editor_tab.h"

class TestSyntaxHighlighter : public QObject {
    Q_OBJECT

private slots:
    void testLanguageFromExtension() {
        QCOMPARE(CinderDE::SyntaxHighlighter::languageFromExtension("c"), CinderDE::Language::C);
        QCOMPARE(CinderDE::SyntaxHighlighter::languageFromExtension(".c"), CinderDE::Language::C);
        QCOMPARE(CinderDE::SyntaxHighlighter::languageFromExtension("cpp"), CinderDE::Language::Cpp);
        QCOMPARE(CinderDE::SyntaxHighlighter::languageFromExtension("cc"), CinderDE::Language::Cpp);
        QCOMPARE(CinderDE::SyntaxHighlighter::languageFromExtension("hpp"), CinderDE::Language::Cpp);
        QCOMPARE(CinderDE::SyntaxHighlighter::languageFromExtension("h"), CinderDE::Language::Cpp);
        QCOMPARE(CinderDE::SyntaxHighlighter::languageFromExtension("py"), CinderDE::Language::Python);
        QCOMPARE(CinderDE::SyntaxHighlighter::languageFromExtension("txt"), CinderDE::Language::None);
        QCOMPARE(CinderDE::SyntaxHighlighter::languageFromExtension("unknown"), CinderDE::Language::None);
    }

    void testCHighlighting() {
        QTextDocument doc;
        CinderDE::SyntaxHighlighter highlighter(&doc);
        highlighter.setLanguage(CinderDE::Language::C);

        doc.setPlainText("#include <stdio.h>\nint main() {\n    // comment\n    return 42;\n}");
        highlighter.rehighlight();

        QTextBlock b0 = doc.findBlockByNumber(0); // #include <stdio.h>
        QTextBlock b1 = doc.findBlockByNumber(1); // int main() {
        QTextBlock b2 = doc.findBlockByNumber(2); // // comment
        QTextBlock b3 = doc.findBlockByNumber(3); // return 42;

        QVERIFY(b0.layout()->formats().count() > 0);
        QVERIFY(b1.layout()->formats().count() > 0);
        QVERIFY(b2.layout()->formats().count() > 0);
        QVERIFY(b3.layout()->formats().count() > 0);
    }

    void testCppAndMultilineCommentHighlighting() {
        QTextDocument doc;
        CinderDE::SyntaxHighlighter highlighter(&doc);
        highlighter.setLanguage(CinderDE::Language::Cpp);

        doc.setPlainText("/* start multiline\n   middle line\n   end multiline */\nclass MyClass {};");
        highlighter.rehighlight();

        QTextBlock b0 = doc.findBlockByNumber(0);
        QTextBlock b1 = doc.findBlockByNumber(1);
        QTextBlock b2 = doc.findBlockByNumber(2);
        QTextBlock b3 = doc.findBlockByNumber(3);

        QCOMPARE(b0.userState(), 1); // Multiline comment state
        QCOMPARE(b1.userState(), 1);
        QCOMPARE(b2.userState(), 0); // Ended on b2

        QVERIFY(b3.layout()->formats().count() > 0); // 'class' keyword format
    }

    void testPythonAndTripleQuotesHighlighting() {
        QTextDocument doc;
        CinderDE::SyntaxHighlighter highlighter(&doc);
        highlighter.setLanguage(CinderDE::Language::Python);

        doc.setPlainText("def foo():\n    \"\"\" docstring line 1\n        docstring line 2 \"\"\"\n    return True");
        highlighter.rehighlight();

        QTextBlock b0 = doc.findBlockByNumber(0);
        QTextBlock b1 = doc.findBlockByNumber(1);
        QTextBlock b2 = doc.findBlockByNumber(2);
        QTextBlock b3 = doc.findBlockByNumber(3);

        QCOMPARE(b1.userState(), 1); // Triple quote state
        QCOMPARE(b2.userState(), 0); // Ended

        QVERIFY(b0.layout()->formats().count() > 0); // 'def' keyword
        QVERIFY(b3.layout()->formats().count() > 0); // 'return', 'True'
    }

    void testEscapedQuotesAndCommentsInStrings() {
        QTextDocument doc;
        CinderDE::SyntaxHighlighter highlighter(&doc);
        highlighter.setLanguage(CinderDE::Language::Cpp);

        doc.setPlainText("const char *str = \"Hello \\\"world\\\" // not a comment\";");
        highlighter.rehighlight();

        QTextBlock b0 = doc.findBlockByNumber(0);
        QVERIFY(b0.layout()->formats().count() > 0);
    }

    void testUnknownExtensionNoHighlighting() {
        QTextDocument doc;
        CinderDE::SyntaxHighlighter highlighter(&doc);
        highlighter.setLanguage(CinderDE::Language::None);

        doc.setPlainText("int main() { return 0; }");
        highlighter.rehighlight();

        QTextBlock b0 = doc.findBlockByNumber(0);
        QCOMPARE(b0.layout()->formats().count(), 0);
    }

    void testTabLanguageReevaluationOnSaveAs() {
        CinderDE::EditorTab tab;
        QCOMPARE(tab.highlighter()->language(), CinderDE::Language::None);

        tab.setFilePath("script.py");
        QCOMPARE(tab.highlighter()->language(), CinderDE::Language::Python);

        tab.setFilePath("main.cpp");
        QCOMPARE(tab.highlighter()->language(), CinderDE::Language::Cpp);
    }
};

QTEST_MAIN(TestSyntaxHighlighter)
#include "test_syntax_highlighter.moc"
