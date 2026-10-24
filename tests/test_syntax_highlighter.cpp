#include <QtTest>
#include <QTextDocument>
#include <QTextBlock>
#include <QJsonDocument>
#include "cinderde/syntax_highlighter.h"
#include "cinderde/editor_tab.h"
#include "cinderde/language_pack.h"

class TestSyntaxHighlighter : public QObject {
    Q_OBJECT

private slots:
    void testNoHighlightingWhenPackNotInstalled() {
        // Clear any registered definitions to simulate uninstalled packs
        CinderDE::LanguageRegistry::instance().clear();

        QTextDocument doc;
        CinderDE::SyntaxHighlighter highlighter(&doc);
        highlighter.setLanguageDefinition(CinderDE::LanguageDefinition());

        doc.setPlainText("#include <stdio.h>\nint main() {\n    // comment\n    return 42;\n}");
        highlighter.rehighlight();

        QTextBlock b0 = doc.findBlockByNumber(0);
        QTextBlock b1 = doc.findBlockByNumber(1);

        // When no language pack is installed, CDE must not invent language-specific support
        QCOMPARE(b0.layout()->formats().count(), 0);
        QCOMPARE(b1.layout()->formats().count(), 0);

        CinderDE::EditorTab tab;
        tab.setFilePath("script.py");
        QVERIFY(!tab.highlighter()->languageDefinition().isValid());
    }

    void testHighlightingWithInstalledLanguagePack() {
        CinderDE::LanguageRegistry::instance().clear();

        // Register a C++ language pack
        QString cppPackJson = R"({
            "id": "cpp",
            "name": "C++",
            "extensions": ["cpp", "hpp", "h"],
            "keywords": ["class", "int", "return"],
            "types": ["MyClass"],
            "comments": {
                "line": "//",
                "block_start": "/*",
                "block_end": "*/"
            }
        })";

        QJsonDocument docJson = QJsonDocument::fromJson(cppPackJson.toUtf8());
        QVERIFY(docJson.isObject());
        QVERIFY(CinderDE::LanguageRegistry::instance().loadFromJson(docJson.object()));

        QTextDocument doc;
        CinderDE::SyntaxHighlighter highlighter(&doc);
        CinderDE::LanguageDefinition cppDef = CinderDE::LanguageRegistry::instance().definitionForId("cpp");
        QVERIFY(cppDef.isValid());
        highlighter.setLanguageDefinition(cppDef);

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

    void testPythonHighlightingWithInstalledPack() {
        CinderDE::LanguageRegistry::instance().clear();

        QString pythonPackJson = R"({
            "id": "python",
            "name": "Python",
            "extensions": ["py"],
            "keywords": ["def", "return"],
            "builtins": ["True"],
            "comments": {
                "line": "#",
                "block_start": "\"\"\"",
                "block_end": "\"\"\""
            }
        })";

        QJsonDocument docJson = QJsonDocument::fromJson(pythonPackJson.toUtf8());
        QVERIFY(docJson.isObject());
        QVERIFY(CinderDE::LanguageRegistry::instance().loadFromJson(docJson.object()));

        QTextDocument doc;
        CinderDE::SyntaxHighlighter highlighter(&doc);
        CinderDE::LanguageDefinition pyDef = CinderDE::LanguageRegistry::instance().definitionForId("python");
        QVERIFY(pyDef.isValid());
        highlighter.setLanguageDefinition(pyDef);

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
        CinderDE::LanguageRegistry::instance().clear();

        QString cppPackJson = R"({
            "id": "cpp",
            "name": "C++",
            "extensions": ["cpp"],
            "keywords": ["const", "char"]
        })";

        QJsonDocument docJson = QJsonDocument::fromJson(cppPackJson.toUtf8());
        QVERIFY(CinderDE::LanguageRegistry::instance().loadFromJson(docJson.object()));

        QTextDocument doc;
        CinderDE::SyntaxHighlighter highlighter(&doc);
        highlighter.setLanguageDefinition(CinderDE::LanguageRegistry::instance().definitionForId("cpp"));

        doc.setPlainText("const char *str = \"Hello \\\"world\\\" // not a comment\";");
        highlighter.rehighlight();

        QTextBlock b0 = doc.findBlockByNumber(0);
        QVERIFY(b0.layout()->formats().count() > 0);
    }

    void testUnknownExtensionNoHighlighting() {
        CinderDE::LanguageRegistry::instance().clear();

        QTextDocument doc;
        CinderDE::SyntaxHighlighter highlighter(&doc);
        highlighter.setLanguageDefinition(CinderDE::LanguageDefinition());

        doc.setPlainText("int main() { return 0; }");
        highlighter.rehighlight();

        QTextBlock b0 = doc.findBlockByNumber(0);
        QCOMPARE(b0.layout()->formats().count(), 0);
    }

    void testTabLanguageReevaluationOnSaveAs() {
        CinderDE::LanguageRegistry::instance().clear();

        QString pythonPackJson = R"({
            "id": "python",
            "name": "Python",
            "extensions": ["py"]
        })";
        QJsonDocument docJson = QJsonDocument::fromJson(pythonPackJson.toUtf8());
        QVERIFY(CinderDE::LanguageRegistry::instance().loadFromJson(docJson.object()));

        CinderDE::EditorTab tab;
        QVERIFY(!tab.highlighter()->languageDefinition().isValid());

        tab.setFilePath("script.py");
        QVERIFY(tab.highlighter()->languageDefinition().isValid());
        QCOMPARE(tab.highlighter()->languageDefinition().id, QString("python"));

        tab.setFilePath("main.unknown");
        QVERIFY(!tab.highlighter()->languageDefinition().isValid());
    }
};

QTEST_MAIN(TestSyntaxHighlighter)
#include "test_syntax_highlighter.moc"
