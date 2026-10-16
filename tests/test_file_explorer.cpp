#include <QtTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include "cinderde/file_explorer.h"

class TestFileExplorer : public QObject {
    Q_OBJECT

private slots:
    void testInitialEmptyWorkspace() {
        CinderDE::FileExplorer explorer;
        QCOMPARE(explorer.workspacePath(), QString(""));
    }

    void testSetWorkspacePathValidAndClear() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        CinderDE::FileExplorer explorer;
        bool workspaceChangedFired = false;
        QString lastChangedPath;

        connect(&explorer, &CinderDE::FileExplorer::workspaceChanged, [&workspaceChangedFired, &lastChangedPath](const QString &path) {
            workspaceChangedFired = true;
            lastChangedPath = path;
        });

        explorer.setWorkspacePath(tempDir.path());
        QVERIFY(workspaceChangedFired);
        QCOMPARE(QDir(explorer.workspacePath()).canonicalPath(), QDir(tempDir.path()).canonicalPath());

        workspaceChangedFired = false;
        explorer.clearWorkspace();
        QVERIFY(workspaceChangedFired);
        QCOMPARE(explorer.workspacePath(), QString(""));
    }

    void testInvalidWorkspacePathDefaultsToEmpty() {
        CinderDE::FileExplorer explorer;
        explorer.setWorkspacePath("/path/that/does/not/exist/12345");
        QCOMPARE(explorer.workspacePath(), QString(""));
    }

    void testRefresh() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        CinderDE::FileExplorer explorer;
        explorer.setWorkspacePath(tempDir.path());

        // Create a new file in directory
        QFile file(QDir(tempDir.path()).filePath("new_file.txt"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("test");
        file.close();

        explorer.refresh();
        QCOMPARE(QDir(explorer.workspacePath()).canonicalPath(), QDir(tempDir.path()).canonicalPath());
    }
};

QTEST_MAIN(TestFileExplorer)
#include "test_file_explorer.moc"
