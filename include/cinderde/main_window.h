#ifndef CINDERDE_MAIN_WINDOW_H
#define CINDERDE_MAIN_WINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QSplitter>
#include <QLabel>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include "cinderde/editor_tab.h"
#include "cinderde/file_explorer.h"
#include "cinderde/workspace_config.h"
#include "cinderde/output_panel.h"
#include "cinderde/build_run_manager.h"

namespace CinderDE {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // File actions
    void newFile();
    void openFile();
    void openFolder();
    void saveFile();
    void saveFileAs();
    void closeTab(int index);
    void closeCurrentTab();

    // Edit actions
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void showFind();
    void showReplace();

    // View actions
    void toggleExplorer();
    void toggleWordWrap();
    void toggleSessionRestore(bool checked);
    void increaseFontSize();
    void decreaseFontSize();
    void resetFontSize();

    // Help actions
    void about();

    // Build and Run actions
    void buildAndRun();
    void buildOnly();
    void runOnly();
    void stopProcess();
    void toggleOutputPanel();
    void showBuildRunSettings();

    // Navigation actions
    void nextTab();
    void previousTab();
    void navigateToDiagnostic(const QString &filePath, int line, int column);

    // Tab state slots
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void updateTabTitle(EditorTab *tab);

private:
    QSplitter *m_mainSplitter; // Vertical
    QSplitter *m_topSplitter;  // Horizontal
    FileExplorer *m_fileExplorer;
    QTabWidget *m_tabWidget;
    OutputPanel *m_outputPanel;
    BuildRunManager *m_buildRunManager;

    // Status bar labels
    QLabel *m_statusCursorLabel;
    QLabel *m_statusFileTypeLabel;
    QLabel *m_statusEncodingLabel;

    // View action references for checking state
    QAction *m_toggleExplorerAction;
    QAction *m_toggleOutputPanelAction;
    QAction *m_wordWrapAction;
    QAction *m_sessionRestoreAction;
    QAction *m_stopProcessAction;
    QMenu *m_recentWorkspacesMenu;

    WorkspaceConfig m_activeWorkspaceConfig;

    void setupUi();
    void setupMenus();
    void setupToolbar();
    void setupStatusBar();
    void applyTheme();
    void loadSettings();
    void saveSettings();
    void restoreSession();
    void saveSessionState();

    void openWorkspaceFolder(const QString &dirPath);
    void updateRecentWorkspacesMenu();
    void applyWorkspaceSettings();

    EditorTab* currentTab() const;
    EditorTab* tabAt(int index) const;
    bool promptSaveIfModified(EditorTab *tab);
    bool promptSaveAllIfModified();
    EditorTab* createNewTab(const QString &filePath = QString());
    EditorTab* findTabByPath(const QString &filePath) const;
};

} // namespace CinderDE

#endif // CINDERDE_MAIN_WINDOW_H
