#include <QtTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include "cinderde/editor_tab.h"
#include "cinderde/preferences.h"

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

        bool result = tab.saveAs(invalidPath);
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
};

QTEST_MAIN(TestEditorTab)
#include "test_editor_tab.moc"
