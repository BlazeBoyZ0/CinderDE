#include <QtTest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDir>
#include <QTemporaryDir>
#include <QFile>
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

    void testBuiltinDiscoveryAndRegressions() {
        CinderDE::LanguageRegistry &registry = CinderDE::LanguageRegistry::instance();

        // Built-in C, C++, Python must always be discoverable
        QVERIFY(registry.hasDefinition("c"));
        QVERIFY(registry.hasDefinition("cpp"));
        QVERIFY(registry.hasDefinition("python"));

        CinderDE::LanguageDefinition cDef = registry.definitionForId("c");
        QVERIFY(cDef.isValid());
        QCOMPARE(cDef.name, QString("C Source"));

        CinderDE::LanguageDefinition cppDef = registry.definitionForId("cpp");
        QVERIFY(cppDef.isValid());
        QCOMPARE(cppDef.name, QString("C++ Source"));

        CinderDE::LanguageDefinition pyDef = registry.definitionForId("python");
        QVERIFY(pyDef.isValid());
        QCOMPARE(pyDef.name, QString("Python Source"));
    }

    void testUserPackDirectoryCreationAndDiscovery() {
        QString userDir = CinderDE::LanguageRegistry::userLanguagesDirectory();
        QVERIFY(!userDir.isEmpty());
        QVERIFY(QDir(userDir).exists());

        QTemporaryDir tempUserDir;
        QVERIFY(tempUserDir.isValid());

        QString customPackPath = tempUserDir.filePath("sample.json");
        QFile file(customPackPath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QString json = R"({
            "id": "samplelang",
            "name": "Sample Language",
            "extensions": ["smp"],
            "keywords": ["sample_kw"]
        })";
        file.write(json.toUtf8());
        file.close();

        CinderDE::LanguageRegistry &registry = CinderDE::LanguageRegistry::instance();
        int loaded = registry.loadFromDirectory(tempUserDir.path(), CinderDE::LanguagePackSource::User);
        QCOMPARE(loaded, 1);
        QVERIFY(registry.hasDefinition("samplelang"));

        CinderDE::LanguageDefinition sampleDef = registry.definitionForExtension("smp");
        QVERIFY(sampleDef.isValid());
        QCOMPARE(sampleDef.name, QString("Sample Language"));
    }

    void testDuplicateIdentifierRejection() {
        CinderDE::LanguageRegistry &registry = CinderDE::LanguageRegistry::instance();

        // Try to register user pack with duplicate ID "cpp"
        QString duplicateCppJson = R"({
            "id": "cpp",
            "name": "User Override CPP",
            "extensions": ["cpp_override"]
        })";

        QJsonDocument doc = QJsonDocument::fromJson(duplicateCppJson.toUtf8());
        QVERIFY(doc.isObject());
        bool result = registry.loadFromJson(doc.object(), "user_cpp.json", CinderDE::LanguagePackSource::User);

        QVERIFY(!result);
        // Built-in definition remains untouched
        QCOMPARE(registry.definitionForId("cpp").name, QString("C++ Source"));

        // Check error reporting in packInfos
        bool duplicateFound = false;
        for (const auto &info : registry.packInfos()) {
            if (info.id == "cpp" && !info.loaded) {
                duplicateFound = true;
                QVERIFY(info.statusMessage.contains("Duplicate identifier"));
            }
        }
        QVERIFY(duplicateFound);
    }

    void testMalformedAndMissingFieldsHandling() {
        // Missing required 'id'
        QJsonObject obj1;
        obj1["name"] = "No ID Language";
        bool ok1 = true;
        CinderDE::LanguageDefinition def1 = CinderDE::LanguageDefinition::fromJson(obj1, &ok1);
        QVERIFY(!ok1);
        QVERIFY(!def1.isValid());

        // Invalid JSON file loading
        QVERIFY(!CinderDE::LanguageRegistry::instance().loadFromFile("/path/to/nonexistent/file.json"));

        // Malformed JSON string
        QTemporaryDir tempDir;
        QString badJsonPath = tempDir.filePath("bad.json");
        QFile badFile(badJsonPath);
        QVERIFY(badFile.open(QIODevice::WriteOnly | QIODevice::Text));
        badFile.write("{ invalid json content ");
        badFile.close();

        QVERIFY(!CinderDE::LanguageRegistry::instance().loadFromFile(badJsonPath));

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
