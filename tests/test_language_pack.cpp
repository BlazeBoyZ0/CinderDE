#include <QtTest>
#include <QJsonObject>
#include <QJsonDocument>
#include "cinderde/language_pack.h"
#include "cinderde/file_type_detector.h"
#include "cinderde/syntax_highlighter.h"
#include "cinderde/editor_tab.h"
#include "cinderde/code_completion.h"
#include "cinderde/build_run_manager.h"

class TestLanguagePack : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Register custom test fixture language pack: Kekno
        QString keknoJson = R"({
            "id": "kekno",
            "name": "Kekno Source",
            "extensions": ["kek", "kekno"],
            "keywords": ["fn", "let", "mut", "if", "else", "return"],
            "types": ["int", "bool", "str"],
            "builtins": ["print", "len"],
            "comments": {
                "line": ";",
                "block_start": "/*",
                "block_end": "*/"
            },
            "indentation": {
                "tab_size": 2,
                "use_spaces": true,
                "auto_indent_on_brace": true
            },
            "build_run": {
                "build_command": "keknoc \"${file}\" -o \"${fileDir}/${fileBase}\"",
                "run_command": "\"${fileDir}/${fileBase}\""
            }
        })";

        QJsonDocument doc = QJsonDocument::fromJson(keknoJson.toUtf8());
        QVERIFY(doc.isObject());
        QVERIFY(CinderDE::LanguageRegistry::instance().loadFromJson(doc.object()));
    }

    void testValidJsonParsing() {
        CinderDE::LanguageDefinition def = CinderDE::LanguageRegistry::instance().definitionForId("kekno");
        QVERIFY(def.isValid());
        QCOMPARE(def.id, QString("kekno"));
        QCOMPARE(def.name, QString("Kekno Source"));
        QCOMPARE(def.extensions.size(), 2);
        QVERIFY(def.extensions.contains("kek"));
        QVERIFY(def.extensions.contains("kekno"));

        QVERIFY(def.keywords.contains("fn"));
        QVERIFY(def.keywords.contains("let"));
        QVERIFY(def.types.contains("int"));
        QVERIFY(def.builtins.contains("print"));

        QCOMPARE(def.comments.linePrefix, QString(";"));
        QCOMPARE(def.comments.blockStart, QString("/*"));
        QCOMPARE(def.comments.blockEnd, QString("*/"));

        QCOMPARE(def.indentation.tabSize, 2);
        QVERIFY(def.indentation.useSpaces);
        QVERIFY(def.indentation.autoIndentOnBrace);

        QCOMPARE(def.buildRun.buildCommand, QString("keknoc \"${file}\" -o \"${fileDir}/${fileBase}\""));
        QCOMPARE(def.buildRun.runCommand, QString("\"${fileDir}/${fileBase}\""));
    }

    void testMalformedAndMissingFieldsHandling() {
        // Missing required 'id'
        QJsonObject obj1;
        obj1["name"] = "No ID Language";
        bool ok1 = true;
        CinderDE::LanguageDefinition def1 = CinderDE::LanguageDefinition::fromJson(obj1, &ok1);
        QVERIFY(!ok1);
        QVERIFY(!def1.isValid());

        // Invalid JSON loading
        QVERIFY(!CinderDE::LanguageRegistry::instance().loadFromFile("/path/to/nonexistent/file.json"));

        // Omitted optional fields defaulted safely
        QJsonObject obj2;
        obj2["id"] = "minimal";
        obj2["name"] = "Minimal Lang";
        bool ok2 = false;
        CinderDE::LanguageDefinition def2 = CinderDE::LanguageDefinition::fromJson(obj2, &ok2);
        QVERIFY(ok2);
        QVERIFY(def2.isValid());
        QCOMPARE(def2.indentation.tabSize, 4); // Default
    }

    void testExtensionMatching() {
        CinderDE::LanguageDefinition def1 = CinderDE::LanguageRegistry::instance().definitionForExtension("kek");
        QVERIFY(def1.isValid());
        QCOMPARE(def1.id, QString("kekno"));

        CinderDE::LanguageDefinition def2 = CinderDE::LanguageRegistry::instance().definitionForExtension(".KEKNO");
        QVERIFY(def2.isValid());
        QCOMPARE(def2.id, QString("kekno"));

        CinderDE::FileTypeInfo info = CinderDE::FileTypeDetector::detectType("program.kek");
        QCOMPARE(info.name, QString("Kekno Source"));
    }

    void testHighlightingWithLanguagePack() {
        QTextDocument doc;
        CinderDE::SyntaxHighlighter highlighter(&doc);

        CinderDE::LanguageDefinition kekDef = CinderDE::LanguageRegistry::instance().definitionForId("kekno");
        highlighter.setLanguageDefinition(kekDef);

        doc.setPlainText("fn main() {\n    ; single line comment\n    let x: int = 42;\n    /* multiline comment */\n}");
        highlighter.rehighlight();

        QTextBlock b0 = doc.findBlockByNumber(0); // fn main() {
        QTextBlock b1 = doc.findBlockByNumber(1); // ; comment
        QTextBlock b2 = doc.findBlockByNumber(2); // let x: int = 42;

        QVERIFY(b0.layout()->formats().count() > 0);
        QVERIFY(b1.layout()->formats().count() > 0);
        QVERIFY(b2.layout()->formats().count() > 0);
    }

    void testCommentTogglingWithLanguagePack() {
        CinderDE::CodeEditor editor;
        CinderDE::LanguageDefinition kekDef = CinderDE::LanguageRegistry::instance().definitionForId("kekno");
        editor.setLanguageDefinition(kekDef);

        editor.setPlainText("let x = 10;");
        QTextCursor cursor = editor.textCursor();
        cursor.setPosition(0);
        editor.setTextCursor(cursor);

        editor.toggleComment();
        QCOMPARE(editor.toPlainText(), QString("; let x = 10;"));

        editor.toggleComment();
        QCOMPARE(editor.toPlainText(), QString("let x = 10;"));
    }

    void testCompletionWithLanguagePack() {
        CinderDE::CodeEditor editor;
        CinderDE::CodeCompletionPopup popup(&editor);
        CinderDE::LanguageDefinition kekDef = CinderDE::LanguageRegistry::instance().definitionForId("kekno");

        popup.updateSuggestions("le", kekDef);
        QVERIFY(popup.count() > 0);
        QCOMPARE(popup.item(0)->text(), QString("len"));
    }

    void testBuildRunProfileResolution() {
        CinderDE::WorkspaceConfig emptyConfig;
        auto cmds = CinderDE::BuildRunManager::resolveCommands("/path/to/script.kek", "", emptyConfig);

        QCOMPARE(cmds.first, QString("keknoc \"${file}\" -o \"${fileDir}/${fileBase}\""));
        QCOMPARE(cmds.second, QString("\"${fileDir}/${fileBase}\""));
    }
};

QTEST_MAIN(TestLanguagePack)
#include "test_language_pack.moc"
