#include <QtTest>
#include <QTemporaryFile>
#include "cinderde/editor_tab.h"

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
};

QTEST_MAIN(TestEditorTab)
#include "test_editor_tab.moc"
