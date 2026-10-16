#include <QtTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>
#include "cinderde/preferences.h"
#include "cinderde/workspace_config.h"
#include "cinderde/editor_tab.h"

class TestWorkspaceSession : public QObject {
    Q_OBJECT

private slots:
    void init() {
        CinderDE::Preferences::instance().clearRecentWorkspaces();
    }

    void testRecentWorkspacesBoundAndCanonical() {
        CinderDE::Preferences &prefs = CinderDE::Preferences::instance();
        prefs.clearRecentWorkspaces();
        QCOMPARE(prefs.recentWorkspaces().size(), 0);

        QList<QTemporaryDir*> tempDirs;
        for (int i = 0; i < 12; ++i) {
            auto *dir = new QTemporaryDir();
            QVERIFY(dir->isValid());
            tempDirs.append(dir);
            prefs.addRecentWorkspace(dir->path());
        }

        // Must be bounded to 10
        QStringList recents = prefs.recentWorkspaces();
        QCOMPARE(recents.size(), CinderDE::Preferences::MAX_RECENT_WORKSPACES);

        // Most recent should be top
        QString lastAddedCanon = QDir(tempDirs.last()->path()).canonicalPath();
        QCOMPARE(QDir(recents.first()).canonicalPath(), lastAddedCanon);

        // Re-adding an earlier dir should move it to top without duplicate
        QString firstAddedCanon = QDir(tempDirs[2]->path()).canonicalPath(); // tempDirs[2] is still in recents
        prefs.addRecentWorkspace(tempDirs[2]->path());
        recents = prefs.recentWorkspaces();
        QCOMPARE(recents.size(), CinderDE::Preferences::MAX_RECENT_WORKSPACES);
        QCOMPARE(QDir(recents.first()).canonicalPath(), firstAddedCanon);

        // Removing recent workspace
        prefs.removeRecentWorkspace(tempDirs[2]->path());
        recents = prefs.recentWorkspaces();
        QCOMPARE(recents.size(), CinderDE::Preferences::MAX_RECENT_WORKSPACES - 1);

        for (auto *dir : tempDirs) {
            delete dir;
        }
    }

    void testOpeningFileDoesNotAddRecentWorkspace() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString filePath = QDir(tempDir.path()).filePath("sample.txt");
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("hello world");
        file.close();

        CinderDE::Preferences::instance().clearRecentWorkspaces();

        CinderDE::EditorTab tab;
        QVERIFY(tab.openFile(filePath));

        // Opening file must NOT add parent directory to recent workspaces
        QCOMPARE(CinderDE::Preferences::instance().recentWorkspaces().size(), 0);
    }

    void testWorkspaceConfigLoadAndSave() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Default when missing
        CinderDE::WorkspaceConfig cfg = CinderDE::WorkspaceConfig::loadFromWorkspace(tempDir.path());
        QVERIFY(!cfg.hasWordWrap);
        QVERIFY(!cfg.hasTabSize);

        // Save overrides
        cfg.hasWordWrap = true;
        cfg.wordWrap = true;
        cfg.hasTabSize = true;
        cfg.tabSize = 2;
        QVERIFY(CinderDE::WorkspaceConfig::saveToWorkspace(tempDir.path(), cfg));

        // Load again
        CinderDE::WorkspaceConfig reloaded = CinderDE::WorkspaceConfig::loadFromWorkspace(tempDir.path());
        QVERIFY(reloaded.hasWordWrap);
        QCOMPARE(reloaded.wordWrap, true);
        QVERIFY(reloaded.hasTabSize);
        QCOMPARE(reloaded.tabSize, 2);
    }

    void testWorkspaceConfigInvalidJsonHandling() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Create .cinderde directory and malformed settings.json
        QDir(tempDir.path()).mkdir(".cinderde");
        QString cfgPath = QDir(tempDir.path()).filePath(".cinderde/settings.json");
        QFile f(cfgPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("{ invalid json formatting ... ");
        f.close();

        // Must fall back safely without crashing
        CinderDE::WorkspaceConfig cfg = CinderDE::WorkspaceConfig::loadFromWorkspace(tempDir.path());
        QVERIFY(!cfg.hasWordWrap);
        QVERIFY(!cfg.hasTabSize);
    }

    void testSessionRestorationPreferences() {
        CinderDE::Preferences &prefs = CinderDE::Preferences::instance();

        prefs.setSessionRestoreEnabled(true);
        QVERIFY(prefs.sessionRestoreEnabled());

        prefs.setSessionRestoreEnabled(false);
        QVERIFY(!prefs.sessionRestoreEnabled());

        prefs.setSessionRestoreEnabled(true);

        QString ws = "/tmp/test_workspace_path";
        prefs.setSessionWorkspace(ws);
        QCOMPARE(prefs.sessionWorkspace(), ws);

        QStringList tabs = { "/tmp/a.txt", "/tmp/b.txt" };
        prefs.setSessionOpenTabs(tabs);
        QCOMPARE(prefs.sessionOpenTabs(), tabs);

        prefs.setSessionActiveTab(1);
        QCOMPARE(prefs.sessionActiveTab(), 1);

        QList<QPair<int, int>> positions = { {10, 5}, {2, 1} };
        prefs.setSessionCursorPositions(positions);
        QCOMPARE(prefs.sessionCursorPositions(), positions);
    }
};

QTEST_MAIN(TestWorkspaceSession)
#include "test_workspace_session.moc"
