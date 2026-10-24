#include <QtTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include "cinderde/editor_tab.h"
#include "cinderde/preferences.h"
#include "cinderde/language_pack.h"

namespace {

CinderDE::LanguageDefinition testCppDefinition() {
    CinderDE::LanguageDefinition def;
    def.id = "test-cpp";
    def.name = "Test C++";
    def.extensions = {"cpp"};
    def.comments.linePrefix = "//";
    def.indentation.tabSize = 4;
    def.indentation.useSpaces = true;
    def.indentation.autoIndentOnBrace = true;
    return def;
}

} // namespace

class TestEditorTab : public QObject {
    Q_OBJECT

private slots:
    void testFileOpenSave() {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.write("Initial text");
        QString path = tempFile.fileName();
        tempFile.close();

        CinderDE::EditorTab tab;
        QVERIFY(tab.openFile(path));
        QCOMPARE(tab.editor()->toPlainText(), QString("Initial text"));
        QVERIFY(!tab.isModified());

        QTextCursor cursor = tab.editor()->textCursor();
        cursor.movePosition(QTextCursor::End);
        tab.editor()->setTextCursor(cursor);
        tab.editor()->insertPlainText(" additional");
        QVERIFY(tab.isModified());

        QVERIFY(tab.save());
        QVERIFY(!tab.isModified());

        QFile readBack(path);
        QVERIFY(readBack.open(QIODevice::ReadOnly | QIODevice::Text));
        QCOMPARE(readBack.readAll(), QByteArray("Initial text additional"));
    }

    void testSaveAsFailurePreservesPathAndModified() {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.write("Original content");
        QString path = tempFile.fileName();
        tempFile.close();

        CinderDE::EditorTab tab;
        QVERIFY(tab.openFile(path));

        tab.editor()->insertPlainText(" modified");
        QVERIFY(tab.isModified());

        // Attempt saveAs to an invalid path (e.g., directory or non-existent path)
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString invalidPath = tempDir.path(); // directory path cannot be opened for writing as a file

        bool result = tab.saveAs(invalidPath, false); // pass showDialog = false to avoid blocking modal dialog in unit tests
        QVERIFY(!result);
        QCOMPARE(tab.filePath(), path);
        QVERIFY(tab.isModified());
    }

    void testResetFontSizeRestoresDefault() {
        CinderDE::EditorTab tab;
        CinderDE::Preferences::instance().setFontSize(20);
        tab.setFontSize(20);
        QCOMPARE(tab.fontSize(), 20);

        tab.resetFontSize();
        QCOMPARE(tab.fontSize(), CinderDE::Preferences::DEFAULT_FONT_SIZE);
        QCOMPARE(CinderDE::Preferences::instance().fontSize(), CinderDE::Preferences::DEFAULT_FONT_SIZE);
    }

    void testSmartIndentationAndBrackets() {
        CinderDE::EditorTab tab;
        tab.setFilePath("test.cpp");

        auto *editor = tab.editor();
        editor->setLanguageDefinition(testCppDefinition());
        editor->setPlainText("int main() {");
        QTextCursor cursor = editor->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor->setTextCursor(cursor);

        QTest::keyClick(editor, Qt::Key_Return);
        QCOMPARE(editor->toPlainText(), QString("int main() {\n    "));

        // Test Bracket Pairing
        QTest::keyClicks(editor, "(");
        QCOMPARE(editor->toPlainText(), QString("int main() {\n    ()"));

        // Test Backspace Pair Delete
        QTest::keyClick(editor, Qt::Key_Backspace);
        QCOMPARE(editor->toPlainText(), QString("int main() {\n    "));
    }

    void testCommentToggling() {
        CinderDE::EditorTab tab;
        tab.setFilePath("test.cpp");

        auto *editor = tab.editor();
        editor->setPlainText("int a = 1;\nint b = 2;");

        QTextCursor cursor = editor->textCursor();
        cursor.select(QTextCursor::Document);
        editor->setTextCursor(cursor);

        editor->toggleComment();
        QCOMPARE(editor->toPlainText(), QString("// int a = 1;\n// int b = 2;"));

        cursor.select(QTextCursor::Document);
        editor->setTextCursor(cursor);

        editor->toggleComment();
        QCOMPARE(editor->toPlainText(), QString("int a = 1;\nint b = 2;"));
    }
};

QTEST_MAIN(TestEditorTab)
#include "test_editor_tab.moc"
