#ifndef CINDERDE_MAIN_WINDOW_H
#define CINDERDE_MAIN_WINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QSplitter>
#include <QLabel>
#include <QToolBar>
#include <QAction>
#include "cinderde/editor_tab.h"
#include "cinderde/file_explorer.h"

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
    void increaseFontSize();
    void decreaseFontSize();
    void resetFontSize();

    // Help actions
    void about();

    // Tab state slots
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void updateTabTitle(EditorTab *tab);

private:
    QSplitter *m_splitter;
    FileExplorer *m_fileExplorer;
    QTabWidget *m_tabWidget;

    // Status bar labels
    QLabel *m_statusCursorLabel;
    QLabel *m_statusFileTypeLabel;
    QLabel *m_statusEncodingLabel;

    // View action references for checking state
    QAction *m_toggleExplorerAction;
    QAction *m_wordWrapAction;

    void setupUi();
    void setupMenus();
    void setupToolbar();
    void setupStatusBar();
    void applyTheme();
    void loadSettings();
    void saveSettings();

    EditorTab* currentTab() const;
    EditorTab* tabAt(int index) const;
    bool promptSaveIfModified(EditorTab *tab);
    EditorTab* createNewTab(const QString &filePath = QString());
    EditorTab* findTabByPath(const QString &filePath) const;
};

} // namespace CinderDE

#endif // CINDERDE_MAIN_WINDOW_H
