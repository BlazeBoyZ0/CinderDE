#include "cinderde/main_window.h"
#include "cinderde/preferences.h"
#include "cinderde/workspace_config.h"
#include "cinderde/build_run_dialog.h"
#include "cinderde/preferences_dialog.h"
#include "cinderde/lsp_dialog.h"
#include "cinderde/lsp_manager.h"
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QKeySequence>
#include <QDir>
#include <QFileInfo>
#include <QStyle>
#include <QToolButton>
#include <QTextBlock>
#include <QTextCursor>

namespace CinderDE {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_recentWorkspacesMenu(nullptr),
      m_sessionRestoreAction(nullptr)
{
    setObjectName("main_window");
    setupUi();
    applyTheme();
    loadSettings();
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUi() {
    m_mainSplitter = new QSplitter(Qt::Vertical, this);
    m_mainSplitter->setObjectName("main_splitter");

    m_topSplitter = new QSplitter(Qt::Horizontal, this);
    m_topSplitter->setObjectName("top_splitter");

    m_fileExplorer = new FileExplorer(this);
    m_fileExplorer->setObjectName("file_explorer");

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setObjectName("main_tab_widget");
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);

    m_outputPanel = new OutputPanel(this);
    m_outputPanel->setObjectName("output_panel");
    m_outputPanel->setVisible(false);

    m_topSplitter->addWidget(m_fileExplorer);
    m_topSplitter->addWidget(m_tabWidget);
    m_topSplitter->setStretchFactor(0, 0);
    m_topSplitter->setStretchFactor(1, 1);
    m_topSplitter->setSizes(QList<int>() << 220 << 780);

    m_mainSplitter->addWidget(m_topSplitter);
    m_mainSplitter->addWidget(m_outputPanel);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setSizes(QList<int>() << 550 << 170);

    setCentralWidget(m_mainSplitter);

    m_buildRunManager = new BuildRunManager(this);

    setupMenus();
    setupToolbar();
    setupStatusBar();

    connect(m_buildRunManager, &BuildRunManager::outputReceived, this, [this](const QString &text, bool isBuild) {
        QString workingDir = BuildRunManager::resolveWorkingDirectory(
            currentTab() ? currentTab()->filePath() : "",
            m_fileExplorer->workspacePath(),
            m_activeWorkspaceConfig);
        m_outputPanel->setWorkingDir(workingDir);
        if (isBuild) {
            m_outputPanel->appendBuildOutput(text, workingDir);
        } else {
            m_outputPanel->appendRunOutput(text);
        }
    });

    connect(m_buildRunManager, &BuildRunManager::stateChanged, this, [this](bool isRunning, const QString &statusText) {
        m_outputPanel->setProcessRunning(isRunning, statusText);
        if (m_stopProcessAction) {
            m_stopProcessAction->setEnabled(isRunning);
        }
    });

    connect(m_buildRunManager, &BuildRunManager::processError, this, [this](const QString &errorMsg, bool) {
        m_outputPanel->appendSystemMessage(QString("Error: %1\n").arg(errorMsg));
        statusBar()->showMessage(errorMsg, 4000);
    });

    connect(m_outputPanel, &OutputPanel::stopRequested, this, &MainWindow::stopProcess);
    connect(m_outputPanel, &OutputPanel::stdinSubmitted, m_buildRunManager, &BuildRunManager::writeStdin);
    connect(m_outputPanel, &OutputPanel::diagnosticClicked, this, &MainWindow::navigateToDiagnostic);
    connect(m_outputPanel, &OutputPanel::closeRequested, this, [this]() {
        m_outputPanel->setVisible(false);
        if (m_toggleOutputPanelAction) {
            m_toggleOutputPanelAction->setChecked(false);
        }
    });

    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        onTabChanged(index);
        saveSessionState();
    });
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    connect(m_fileExplorer, &FileExplorer::openFolderRequested, this, &MainWindow::openFolder);
    connect(m_fileExplorer, &FileExplorer::openWorkspaceRequested, this, &MainWindow::openWorkspaceFolder);
    connect(m_fileExplorer, &FileExplorer::fileSelected, this, [this](const QString &path) {
        EditorTab *existing = findTabByPath(path);
        if (existing) {
            m_tabWidget->setCurrentWidget(existing);
        } else {
            createNewTab(path);
        }
    });

    connect(m_fileExplorer, &FileExplorer::fileRenamed, this, [this](const QString &oldPath, const QString &newPath) {
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            EditorTab *tab = tabAt(i);
            if (!tab) continue;

            if (tab->filePath() == oldPath) {
                tab->setFilePath(newPath);
                updateTabTitle(tab);
            } else if (tab->filePath().startsWith(oldPath + "/")) {
                QString rel = tab->filePath().mid(oldPath.length());
                tab->setFilePath(newPath + rel);
                updateTabTitle(tab);
            }
        }
        if (currentTab()) {
            m_statusFileTypeLabel->setText(currentTab()->fileTypeInfo().name);
        }
        saveSessionState();
    });

    connect(m_fileExplorer, &FileExplorer::fileDeleted, this, [this](const QString &deletedPath) {
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            EditorTab *tab = tabAt(i);
            if (!tab) continue;

            if (tab->filePath() == deletedPath || tab->filePath().startsWith(deletedPath + "/")) {
                tab->clearFilePathAndMarkModified();
                updateTabTitle(tab);
            }
        }
        if (currentTab()) {
            m_statusFileTypeLabel->setText(currentTab()->fileTypeInfo().name);
        }
        saveSessionState();
    });
}

void MainWindow::setupMenus() {
    QMenuBar *menuBar = this->menuBar();

    // --- File Menu ---
    QMenu *fileMenu = menuBar->addMenu("&File");

    QAction *newAct = fileMenu->addAction("&New File", QKeySequence::New, this, &MainWindow::newFile);
    newAct->setStatusTip("Create a new document");

    QAction *openFileAct = fileMenu->addAction("&Open File...", QKeySequence::Open, this, &MainWindow::openFile);
    openFileAct->setStatusTip("Open an existing file");

    QAction *openFolderAct = fileMenu->addAction("Open &Folder...", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), this, &MainWindow::openFolder);
    openFolderAct->setStatusTip("Open a workspace folder");

    m_recentWorkspacesMenu = fileMenu->addMenu("Recent &Workspaces");
    connect(m_recentWorkspacesMenu, &QMenu::aboutToShow, this, &MainWindow::updateRecentWorkspacesMenu);

    fileMenu->addSeparator();

    QAction *saveAct = fileMenu->addAction("&Save", QKeySequence::Save, this, &MainWindow::saveFile);
    saveAct->setStatusTip("Save current document");

    QAction *saveAsAct = fileMenu->addAction("Save &As...", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), this, &MainWindow::saveFileAs);
    saveAsAct->setStatusTip("Save current document with a new name");

    fileMenu->addSeparator();

    QAction *closeTabAct = fileMenu->addAction("&Close Tab", QKeySequence::Close, this, &MainWindow::closeCurrentTab);
    closeTabAct->setStatusTip("Close current document tab");

    fileMenu->addSeparator();

    QAction *prefAct = fileMenu->addAction("Prefere&nces...", QKeySequence(Qt::CTRL | Qt::Key_Comma), this, &MainWindow::showPreferencesDialog);
    prefAct->setStatusTip("Configure editor and language packs");

    fileMenu->addSeparator();

    QAction *exitAct = fileMenu->addAction("E&xit", QKeySequence::Quit, this, &QWidget::close);
    exitAct->setStatusTip("Exit CinderDE");

    // --- Edit Menu ---
    QMenu *editMenu = menuBar->addMenu("&Edit");

    editMenu->addAction("&Undo", QKeySequence::Undo, this, &MainWindow::undo);
    editMenu->addAction("&Redo", QKeySequence::Redo, this, &MainWindow::redo);
    editMenu->addSeparator();
    editMenu->addAction("Cu&t", QKeySequence::Cut, this, &MainWindow::cut);
    editMenu->addAction("&Copy", QKeySequence::Copy, this, &MainWindow::copy);
    editMenu->addAction("&Paste", QKeySequence::Paste, this, &MainWindow::paste);
    editMenu->addAction("Select &All", QKeySequence::SelectAll, this, &MainWindow::selectAll);
    editMenu->addSeparator();
    editMenu->addAction("Toggle Line &Comment", QKeySequence(Qt::CTRL | Qt::Key_Slash), this, &MainWindow::toggleComment);
    editMenu->addSeparator();
    editMenu->addAction("&Find...", QKeySequence::Find, this, &MainWindow::showFind);
    editMenu->addAction("&Replace...", QKeySequence(Qt::CTRL | Qt::Key_H), this, &MainWindow::showReplace);

    // --- Build Menu ---
    QMenu *buildMenu = menuBar->addMenu("&Build");

    QAction *buildRunAct = buildMenu->addAction("Build && &Run", QKeySequence(Qt::Key_F5), this, &MainWindow::buildAndRun);
    buildRunAct->setStatusTip("Build and run current project or file");

    QAction *buildAct = buildMenu->addAction("&Build", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B), this, &MainWindow::buildOnly);
    buildAct->setStatusTip("Build current project or file");

    QAction *runAct = buildMenu->addAction("&Run", this, &MainWindow::runOnly);
    runAct->setStatusTip("Run current project or binary");

    m_stopProcessAction = buildMenu->addAction("&Stop Process", QKeySequence(Qt::SHIFT | Qt::Key_F5), this, &MainWindow::stopProcess);
    m_stopProcessAction->setStatusTip("Stop active process");
    m_stopProcessAction->setEnabled(false);

    buildMenu->addSeparator();

    buildMenu->addAction("Build && Run &Settings...", this, &MainWindow::showBuildRunSettings);

    // --- Tools Menu ---
    QMenu *toolsMenu = menuBar->addMenu("&Tools");

    QAction *lspAct = toolsMenu->addAction("Language &Servers...", this, &MainWindow::showLspDialog);
    lspAct->setStatusTip("Configure and manage Language Server Protocol (LSP) servers");

    // --- View Menu ---
    QMenu *viewMenu = menuBar->addMenu("&View");

    m_toggleExplorerAction = viewMenu->addAction("Toggle &Explorer", QKeySequence(Qt::CTRL | Qt::Key_B), this, &MainWindow::toggleExplorer);
    m_toggleExplorerAction->setCheckable(true);
    m_toggleExplorerAction->setChecked(Preferences::instance().explorerVisible());

    m_toggleOutputPanelAction = viewMenu->addAction("Toggle &Output Panel", QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_O), this, &MainWindow::toggleOutputPanel);
    m_toggleOutputPanelAction->setCheckable(true);
    m_toggleOutputPanelAction->setChecked(false);

    m_wordWrapAction = viewMenu->addAction("Toggle &Word Wrap", QKeySequence(Qt::ALT | Qt::Key_Z), this, &MainWindow::toggleWordWrap);
    m_wordWrapAction->setCheckable(true);
    m_wordWrapAction->setChecked(Preferences::instance().wordWrap());

    m_sessionRestoreAction = viewMenu->addAction("Restore Session on Startup", this, &MainWindow::toggleSessionRestore);
    m_sessionRestoreAction->setCheckable(true);
    m_sessionRestoreAction->setChecked(Preferences::instance().sessionRestoreEnabled());

    viewMenu->addSeparator();

    viewMenu->addAction("Increase Font Size", QKeySequence::ZoomIn, this, &MainWindow::increaseFontSize);
    viewMenu->addAction("Decrease Font Size", QKeySequence::ZoomOut, this, &MainWindow::decreaseFontSize);
    viewMenu->addAction("Reset Font Size", QKeySequence(Qt::CTRL | Qt::Key_0), this, &MainWindow::resetFontSize);

    // --- Tab Navigation Shortcuts ---
    QAction *nextTabAct = new QAction("Next Tab", this);
    nextTabAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab));
    connect(nextTabAct, &QAction::triggered, this, &MainWindow::nextTab);
    addAction(nextTabAct);

    QAction *prevTabAct = new QAction("Previous Tab", this);
    prevTabAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));
    connect(prevTabAct, &QAction::triggered, this, &MainWindow::previousTab);
    addAction(prevTabAct);

    // --- Help Menu ---
    QMenu *helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction("&About CinderDE", this, &MainWindow::about);
}

void MainWindow::setupToolbar() {
    QToolBar *toolbar = new QToolBar("Main Toolbar", this);
    toolbar->setObjectName("main_toolbar");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    addToolBar(toolbar);

    toolbar->addAction("New", this, &MainWindow::newFile);
    toolbar->addAction("Open", this, &MainWindow::openFile);
    toolbar->addAction("Save", this, &MainWindow::saveFile);
    toolbar->addSeparator();
    toolbar->addAction("Find", this, &MainWindow::showFind);
    toolbar->addSeparator();
    toolbar->addAction("Explorer", this, &MainWindow::toggleExplorer);
    toolbar->addAction("Output", this, &MainWindow::toggleOutputPanel);
    toolbar->addSeparator();
    toolbar->addAction("Build", this, &MainWindow::buildOnly);
    toolbar->addAction("Run", this, &MainWindow::runOnly);
    toolbar->addAction("Build & Run", this, &MainWindow::buildAndRun);
    toolbar->addAction("Stop", this, &MainWindow::stopProcess);
}

void MainWindow::setupStatusBar() {
    m_statusCursorLabel = new QLabel("Ln 1, Col 1", this);
    m_statusFileTypeLabel = new QLabel("Plain Text", this);
    m_statusEncodingLabel = new QLabel("UTF-8", this);

    m_statusCursorLabel->setMinimumWidth(120);
    m_statusFileTypeLabel->setMinimumWidth(140);
    m_statusEncodingLabel->setMinimumWidth(80);

    statusBar()->addPermanentWidget(m_statusCursorLabel);
    statusBar()->addPermanentWidget(m_statusFileTypeLabel);
    statusBar()->addPermanentWidget(m_statusEncodingLabel);

    statusBar()->showMessage("Ready", 3000);
}

void MainWindow::applyTheme() {
    // Dark Charcoal theme with Ember-Orange accents
    QString qss = R"(
        QMainWindow, QDialog {
            background-color: #1e1e1e;
            color: #cccccc;
        }
        QMenuBar {
            background-color: #252526;
            color: #cccccc;
            border-bottom: 1px solid #333333;
        }
        QMenuBar::item:selected {
            background-color: #3c3c3c;
            color: #ffffff;
        }
        QMenu {
            background-color: #252526;
            color: #cccccc;
            border: 1px solid #3c3c3c;
        }
        QMenu::item:selected {
            background-color: #ff6b35;
            color: #ffffff;
        }
        QToolBar {
            background-color: #2d2d2d;
            border-bottom: 1px solid #333333;
            spacing: 3px;
            padding: 2px 4px;
        }
        QToolButton {
            background-color: #3c3c3c;
            color: #cccccc;
            border: 1px solid #454545;
            border-radius: 3px;
            padding: 3px 8px;
            margin: 1px 2px;
            font-size: 12px;
        }
        QToolButton:hover {
            background-color: #4f4f4f;
            color: #ffffff;
        }
        QToolButton:pressed {
            background-color: #ff6b35;
            color: #ffffff;
        }
        QTabWidget::pane {
            border: none;
            background-color: #1e1e1e;
        }
        QTabBar::tab {
            background-color: #2d2d2d;
            color: #969696;
            padding: 5px 12px;
            border-right: 1px solid #252526;
            border-top: 2px solid transparent;
        }
        QTabBar::tab:selected {
            background-color: #1e1e1e;
            color: #ffffff;
            border-top: 2px solid #ff6b35;
        }
        QTabBar::tab:hover:!selected {
            background-color: #383838;
            color: #e0e0e0;
        }
        QStatusBar {
            background-color: #252526;
            color: #ffffff;
            font-size: 11px;
            border-top: 1px solid #333333;
        }
        QStatusBar QLabel {
            color: #ffffff;
        }
        QSplitter::handle {
            background-color: #2d2d2d;
        }
        QPlainTextEdit {
            background-color: #1e1e1e;
            color: #d4d4d4;
            selection-background-color: #264f78;
            selection-color: #ffffff;
            border: none;
        }
    )";

    qApp->setStyleSheet(qss);
}

void MainWindow::updateRecentWorkspacesMenu() {
    if (!m_recentWorkspacesMenu) return;

    m_recentWorkspacesMenu->clear();
    QStringList recentList = Preferences::instance().recentWorkspaces();

    if (recentList.isEmpty()) {
        QAction *emptyAct = m_recentWorkspacesMenu->addAction("No Recent Workspaces");
        emptyAct->setEnabled(false);
    } else {
        for (const QString &path : recentList) {
            bool exists = QDir(path).exists();
            QFileInfo info(path);
            QString folderName = info.fileName();
            if (folderName.isEmpty()) folderName = path;

            QString itemText = QString("%1  [%2]").arg(folderName, path);
            if (!exists) {
                itemText += " (Missing)";
            }

            QAction *act = m_recentWorkspacesMenu->addAction(itemText);
            connect(act, &QAction::triggered, this, [this, path, exists]() {
                if (exists && QDir(path).exists()) {
                    openWorkspaceFolder(path);
                } else {
                    QMessageBox::warning(
                        this, "Workspace Missing",
                        QString("The workspace folder no longer exists:\n%1").arg(path));
                    Preferences::instance().removeRecentWorkspace(path);
                    updateRecentWorkspacesMenu();
                    m_fileExplorer->updateRecentWorkspacesUI();
                }
            });
        }
    }

    m_recentWorkspacesMenu->addSeparator();
    QAction *clearAct = m_recentWorkspacesMenu->addAction("Clear Recent List");
    connect(clearAct, &QAction::triggered, this, [this]() {
        Preferences::instance().clearRecentWorkspaces();
        updateRecentWorkspacesMenu();
        m_fileExplorer->updateRecentWorkspacesUI();
    });
}

void MainWindow::applyWorkspaceSettings() {
    QString wsPath = m_fileExplorer->workspacePath();
    if (!wsPath.isEmpty()) {
        m_activeWorkspaceConfig = WorkspaceConfig::loadFromWorkspace(wsPath);
    } else {
        m_activeWorkspaceConfig = WorkspaceConfig();
    }

    bool wrap = m_activeWorkspaceConfig.hasWordWrap ? m_activeWorkspaceConfig.wordWrap : Preferences::instance().wordWrap();
    int tabSz = m_activeWorkspaceConfig.hasTabSize ? m_activeWorkspaceConfig.tabSize : Preferences::instance().tabSize();

    m_wordWrapAction->setChecked(wrap);

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        EditorTab *tab = tabAt(i);
        if (tab) {
            tab->setWordWrap(wrap);
            tab->setTabSize(tabSz);
        }
    }
}

void MainWindow::loadSettings() {
    Preferences &prefs = Preferences::instance();

    if (!prefs.windowGeometry().isEmpty()) {
        restoreGeometry(prefs.windowGeometry());
    } else {
        resize(1024, 720);
    }

    if (!prefs.windowState().isEmpty()) {
        restoreState(prefs.windowState());
    }

    m_fileExplorer->setVisible(prefs.explorerVisible());
    m_toggleExplorerAction->setChecked(prefs.explorerVisible());

    restoreSession();
}

void MainWindow::restoreSession() {
    Preferences &prefs = Preferences::instance();

    if (!prefs.sessionRestoreEnabled()) {
        m_fileExplorer->clearWorkspace();
        applyWorkspaceSettings();
        if (m_tabWidget->count() == 0) {
            createNewTab();
        }
        return;
    }

    QString wsPath = prefs.sessionWorkspace();
    if (wsPath.isEmpty()) {
        wsPath = prefs.lastWorkspace();
    }

    if (!wsPath.isEmpty() && QDir(wsPath).exists()) {
        m_fileExplorer->setWorkspacePath(wsPath);
    } else {
        m_fileExplorer->clearWorkspace();
    }

    applyWorkspaceSettings();

    QStringList tabs = prefs.sessionOpenTabs();
    QList<QPair<int, int>> cursorPositions = prefs.sessionCursorPositions();
    int activeIdx = prefs.sessionActiveTab();

    int openedCount = 0;
    for (int i = 0; i < tabs.size(); ++i) {
        const QString &filePath = tabs[i];
        if (!filePath.isEmpty() && QFileInfo::exists(filePath) && QFileInfo(filePath).isFile()) {
            EditorTab *tab = createNewTab(filePath);
            if (tab) {
                openedCount++;
                if (i < cursorPositions.size()) {
                    int line = cursorPositions[i].first;
                    int col = cursorPositions[i].second;
                    QTextBlock block = tab->editor()->document()->findBlockByLineNumber(qMax(0, line - 1));
                    if (block.isValid()) {
                        QTextCursor cursor(block);
                        int posInBlock = qMin(qMax(0, col - 1), block.length() - 1);
                        cursor.setPosition(block.position() + posInBlock);
                        tab->editor()->setTextCursor(cursor);
                    }
                }
            }
        }
    }

    if (m_tabWidget->count() == 0) {
        createNewTab();
    } else {
        int targetIdx = qBound(0, activeIdx, m_tabWidget->count() - 1);
        m_tabWidget->setCurrentIndex(targetIdx);
    }
}

void MainWindow::saveSettings() {
    Preferences &prefs = Preferences::instance();
    prefs.setWindowGeometry(saveGeometry());
    prefs.setWindowState(saveState());
    prefs.setExplorerVisible(m_fileExplorer->isVisible());
    saveSessionState();
}

void MainWindow::saveSessionState() {
    Preferences &prefs = Preferences::instance();

    prefs.setSessionWorkspace(m_fileExplorer->workspacePath());

    QStringList openTabs;
    QList<QPair<int, int>> cursorPositions;
    int activeIdx = m_tabWidget->currentIndex();
    int validCount = 0;
    int savedActiveIdx = 0;

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        EditorTab *tab = tabAt(i);
        if (tab && !tab->isUntitled()) {
            openTabs.append(tab->filePath());
            QTextCursor cursor = tab->editor()->textCursor();
            int line = cursor.blockNumber() + 1;
            int col = cursor.columnNumber() + 1;
            cursorPositions.append({line, col});
            if (i == activeIdx) {
                savedActiveIdx = validCount;
            }
            validCount++;
        }
    }

    prefs.setSessionOpenTabs(openTabs);
    prefs.setSessionActiveTab(savedActiveIdx);
    prefs.setSessionCursorPositions(cursorPositions);
}

EditorTab* MainWindow::currentTab() const {
    return qobject_cast<EditorTab*>(m_tabWidget->currentWidget());
}

EditorTab* MainWindow::tabAt(int index) const {
    return qobject_cast<EditorTab*>(m_tabWidget->widget(index));
}

EditorTab* MainWindow::findTabByPath(const QString &filePath) const {
    if (filePath.isEmpty()) return nullptr;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        EditorTab *tab = tabAt(i);
        if (tab && tab->filePath() == filePath) {
            return tab;
        }
    }
    return nullptr;
}

EditorTab* MainWindow::createNewTab(const QString &filePath) {
    // If opening an empty file and current single tab is untitled and unmodified, reuse it
    if (!filePath.isEmpty() && m_tabWidget->count() == 1) {
        EditorTab *cur = currentTab();
        if (cur && cur->isUntitled() && !cur->isModified()) {
            if (cur->openFile(filePath)) {
                updateTabTitle(cur);
                saveSessionState();
                return cur;
            } else {
                return nullptr;
            }
        }
    }

    EditorTab *tab = new EditorTab(this);
    tab->setFontSize(Preferences::instance().fontSize());

    bool wrap = m_activeWorkspaceConfig.hasWordWrap ? m_activeWorkspaceConfig.wordWrap : Preferences::instance().wordWrap();
    int tabSz = m_activeWorkspaceConfig.hasTabSize ? m_activeWorkspaceConfig.tabSize : Preferences::instance().tabSize();

    tab->setWordWrap(wrap);
    tab->setTabSize(tabSz);

    if (!filePath.isEmpty()) {
        if (!tab->openFile(filePath)) {
            delete tab;
            return nullptr;
        }
    }

    int idx = m_tabWidget->addTab(tab, tab->fileName());

    QToolButton *closeBtn = new QToolButton(m_tabWidget->tabBar());
    closeBtn->setText("×");
    closeBtn->setToolTip("Close Tab");
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(R"(
        QToolButton {
            border: none;
            background: transparent;
            color: #aaaaaa;
            font-weight: bold;
            font-size: 13px;
            padding: 0px;
            margin: 1px 2px 1px 2px;
            width: 16px;
            height: 16px;
            border-radius: 8px;
        }
        QToolButton:hover {
            background-color: #ff6b35;
            color: #ffffff;
        }
    )");

    connect(closeBtn, &QToolButton::clicked, this, [this, tab]() {
        int index = m_tabWidget->indexOf(tab);
        if (index != -1) {
            closeTab(index);
        }
    });

    m_tabWidget->tabBar()->setTabButton(idx, QTabBar::RightSide, closeBtn);

    m_tabWidget->setCurrentIndex(idx);

    connect(tab, &EditorTab::modificationChanged, this, [this, tab](bool) {
        updateTabTitle(tab);
        saveSessionState();
    });

    connect(tab, &EditorTab::cursorPositionChanged, this, [this](int line, int col) {
        m_statusCursorLabel->setText(QString("Ln %1, Col %2").arg(line).arg(col));
    });

    connect(tab, &EditorTab::fileTypeChanged, this, [this](const FileTypeInfo &info) {
        m_statusFileTypeLabel->setText(info.name);
    });

    updateTabTitle(tab);
    saveSessionState();
    return tab;
}

void MainWindow::updateTabTitle(EditorTab *tab) {
    if (!tab) return;
    int idx = m_tabWidget->indexOf(tab);
    if (idx != -1) {
        QString title = tab->fileName();
        if (tab->isModified()) {
            title += " *";
        }
        m_tabWidget->setTabText(idx, title);
    }
}

void MainWindow::newFile() {
    createNewTab();
}

void MainWindow::openFile() {
    QString filePath = QFileDialog::getOpenFileName(this, "Open File", QString(), "All Files (*)");
    if (!filePath.isEmpty()) {
        EditorTab *existing = findTabByPath(filePath);
        if (existing) {
            m_tabWidget->setCurrentWidget(existing);
        } else {
            createNewTab(filePath);
        }
    }
}

void MainWindow::openWorkspaceFolder(const QString &dirPath) {
    if (dirPath.isEmpty()) return;

    if (!promptSaveAllIfModified()) {
        return; // User cancelled
    }

    QDir dir(dirPath);
    if (!dir.exists()) {
        QMessageBox::warning(
            this, "Directory Not Found",
            QString("The workspace directory does not exist:\n%1").arg(dirPath));
        Preferences::instance().removeRecentWorkspace(dirPath);
        updateRecentWorkspacesMenu();
        m_fileExplorer->updateRecentWorkspacesUI();
        return;
    }

    m_fileExplorer->setWorkspacePath(dirPath);
    QString canonPath = m_fileExplorer->workspacePath();
    if (!canonPath.isEmpty()) {
        Preferences::instance().addRecentWorkspace(canonPath);
        Preferences::instance().setLastWorkspace(canonPath);
        LspManager::instance().setWorkspacePath(canonPath);
        applyWorkspaceSettings();
        updateRecentWorkspacesMenu();
        m_fileExplorer->updateRecentWorkspacesUI();
        saveSessionState();
    }
}

void MainWindow::openFolder() {
    QString dirPath = QFileDialog::getExistingDirectory(this, "Open Workspace Folder");
    if (!dirPath.isEmpty()) {
        openWorkspaceFolder(dirPath);
    }
}

bool MainWindow::saveFile() {
    EditorTab *tab = currentTab();
    // A workspace build/run can be valid even when no editor tab is open.
    // No active document means there is nothing to save, not a save failure.
    if (!tab) return true;

    if (tab->isUntitled()) {
        return saveFileAs();
    }

    if (!tab->save()) return false;

    updateTabTitle(tab);
    statusBar()->showMessage("File saved", 2000);
    saveSessionState();
    return true;
}

bool MainWindow::saveFileAs() {
    EditorTab *tab = currentTab();
    if (!tab) return false;

    QString defaultName = tab->filePath().isEmpty() ? tab->fileName() : tab->filePath();
    QString filePath = QFileDialog::getSaveFileName(this, "Save File As", defaultName, "All Files (*)");
    if (filePath.isEmpty()) return false;

    if (!tab->saveAs(filePath)) return false;

    updateTabTitle(tab);
    statusBar()->showMessage("File saved", 2000);
    saveSessionState();
    return true;
}

bool MainWindow::promptSaveIfModified(EditorTab *tab) {
    if (!tab || !tab->isModified()) return true;

    QMessageBox::StandardButton reply = QMessageBox::warning(
        this, "Save Changes",
        QString("Do you want to save changes to '%1'?").arg(tab->fileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (reply == QMessageBox::Save) {
        if (tab->isUntitled()) {
            QString filePath = QFileDialog::getSaveFileName(this, "Save File As", tab->fileName(), "All Files (*)");
            if (filePath.isEmpty()) return false; // Canceled save as
            return tab->saveAs(filePath);
        } else {
            return tab->save();
        }
    } else if (reply == QMessageBox::Discard) {
        return true;
    } else {
        return false; // Cancel
    }
}

bool MainWindow::promptSaveAllIfModified() {
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        EditorTab *tab = tabAt(i);
        if (tab && tab->isModified()) {
            m_tabWidget->setCurrentIndex(i);
            if (!promptSaveIfModified(tab)) {
                return false;
            }
        }
    }
    return true;
}

void MainWindow::closeTab(int index) {
    EditorTab *tab = tabAt(index);
    if (!tab) return;

    if (promptSaveIfModified(tab)) {
        m_tabWidget->removeTab(index);
        delete tab;

        if (m_tabWidget->count() == 0) {
            createNewTab();
        }
        saveSessionState();
    }
}

void MainWindow::closeCurrentTab() {
    closeTab(m_tabWidget->currentIndex());
}

void MainWindow::onTabChanged(int index) {
    EditorTab *tab = tabAt(index);
    if (tab) {
        m_statusFileTypeLabel->setText(tab->fileTypeInfo().name);
        emit tab->editor()->cursorPositionChanged();
    }
}

void MainWindow::onTabCloseRequested(int index) {
    closeTab(index);
}

void MainWindow::undo() {
    if (currentTab()) currentTab()->editor()->undo();
}

void MainWindow::redo() {
    if (currentTab()) currentTab()->editor()->redo();
}

void MainWindow::cut() {
    if (currentTab()) currentTab()->editor()->cut();
}

void MainWindow::copy() {
    if (currentTab()) currentTab()->editor()->copy();
}

void MainWindow::paste() {
    if (currentTab()) currentTab()->editor()->paste();
}

void MainWindow::selectAll() {
    if (currentTab()) currentTab()->editor()->selectAll();
}

void MainWindow::toggleComment() {
    if (currentTab()) currentTab()->editor()->toggleComment();
}

void MainWindow::showFind() {
    if (currentTab()) currentTab()->showFindReplaceBar(false);
}

void MainWindow::showReplace() {
    if (currentTab()) currentTab()->showFindReplaceBar(true);
}

void MainWindow::showLspDialog() {
    LspDialog dlg(this);
    dlg.exec();
}

void MainWindow::showPreferencesDialog() {
    PreferencesDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        applyWorkspaceSettings();
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            EditorTab *tab = tabAt(i);
            if (tab) {
                tab->setFontSize(Preferences::instance().fontSize());
                tab->updateHighlighterLanguage();
            }
        }
    }
}

void MainWindow::buildAndRun() {
    if (!saveFile()) return;
    m_outputPanel->setVisible(true);
    if (m_toggleOutputPanelAction) m_toggleOutputPanelAction->setChecked(true);

    QString curPath = currentTab() ? currentTab()->filePath() : QString();
    QString wsPath = m_fileExplorer->workspacePath();
    m_buildRunManager->buildAndRun(curPath, wsPath, m_activeWorkspaceConfig);
}

void MainWindow::buildOnly() {
    if (!saveFile()) return;
    m_outputPanel->setVisible(true);
    if (m_toggleOutputPanelAction) m_toggleOutputPanelAction->setChecked(true);

    QString curPath = currentTab() ? currentTab()->filePath() : QString();
    QString wsPath = m_fileExplorer->workspacePath();
    m_buildRunManager->build(curPath, wsPath, m_activeWorkspaceConfig);
}

void MainWindow::runOnly() {
    if (!saveFile()) return;
    m_outputPanel->setVisible(true);
    if (m_toggleOutputPanelAction) m_toggleOutputPanelAction->setChecked(true);

    QString curPath = currentTab() ? currentTab()->filePath() : QString();
    QString wsPath = m_fileExplorer->workspacePath();
    m_buildRunManager->run(curPath, wsPath, m_activeWorkspaceConfig);
}

void MainWindow::stopProcess() {
    m_buildRunManager->stop();
}

void MainWindow::toggleOutputPanel() {
    bool vis = !m_outputPanel->isVisible();
    m_outputPanel->setVisible(vis);
    if (m_toggleOutputPanelAction) {
        m_toggleOutputPanelAction->setChecked(vis);
    }
}

void MainWindow::showBuildRunSettings() {
    QString wsPath = m_fileExplorer->workspacePath();
    BuildRunDialog dlg(wsPath, m_activeWorkspaceConfig, this);
    if (dlg.exec() == QDialog::Accepted) {
        WorkspaceConfig newCfg = dlg.config();
        if (!wsPath.isEmpty()) {
            WorkspaceConfig::saveToWorkspace(wsPath, newCfg);
        } else {
            if (newCfg.hasExecutionMode) {
                Preferences::instance().setExecutionMode(newCfg.executionMode);
            }
            if (newCfg.hasTerminalCommand) {
                Preferences::instance().setTerminalCommand(newCfg.terminalCommand);
            }
        }
        applyWorkspaceSettings();
    }
}

void MainWindow::nextTab() {
    if (m_tabWidget->count() > 1) {
        int next = (m_tabWidget->currentIndex() + 1) % m_tabWidget->count();
        m_tabWidget->setCurrentIndex(next);
    }
}

void MainWindow::previousTab() {
    if (m_tabWidget->count() > 1) {
        int prev = (m_tabWidget->currentIndex() - 1 + m_tabWidget->count()) % m_tabWidget->count();
        m_tabWidget->setCurrentIndex(prev);
    }
}

void MainWindow::navigateToDiagnostic(const QString &filePath, int line, int column) {
    if (filePath.isEmpty()) return;

    EditorTab *tab = findTabByPath(filePath);
    if (!tab) {
        if (QFileInfo::exists(filePath)) {
            tab = createNewTab(filePath);
        } else {
            statusBar()->showMessage(QString("File not found: %1").arg(filePath), 3000);
            return;
        }
    }

    if (tab) {
        m_tabWidget->setCurrentWidget(tab);
        if (line > 0) {
            QTextBlock block = tab->editor()->document()->findBlockByLineNumber(line - 1);
            if (block.isValid()) {
                QTextCursor cursor(block);
                int colInBlock = (column > 0) ? qMin(column - 1, block.length() - 1) : 0;
                cursor.setPosition(block.position() + colInBlock);
                tab->editor()->setTextCursor(cursor);
                tab->editor()->centerCursor();
                tab->editor()->setFocus();
            }
        }
    }
}

void MainWindow::toggleExplorer() {
    bool vis = !m_fileExplorer->isVisible();
    m_fileExplorer->setVisible(vis);
    m_toggleExplorerAction->setChecked(vis);
}

void MainWindow::toggleWordWrap() {
    bool wrap = !m_wordWrapAction->isChecked();
    m_wordWrapAction->setChecked(wrap);

    QString wsPath = m_fileExplorer->workspacePath();
    if (!wsPath.isEmpty()) {
        m_activeWorkspaceConfig.hasWordWrap = true;
        m_activeWorkspaceConfig.wordWrap = wrap;
        WorkspaceConfig::saveToWorkspace(wsPath, m_activeWorkspaceConfig);
    } else {
        Preferences::instance().setWordWrap(wrap);
    }

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        EditorTab *tab = tabAt(i);
        if (tab) tab->setWordWrap(wrap);
    }
}

void MainWindow::toggleSessionRestore(bool checked) {
    Preferences::instance().setSessionRestoreEnabled(checked);
}

void MainWindow::increaseFontSize() {
    EditorTab *tab = currentTab();
    if (tab) {
        tab->zoomIn();
        Preferences::instance().setFontSize(tab->fontSize());
    }
}

void MainWindow::decreaseFontSize() {
    EditorTab *tab = currentTab();
    if (tab) {
        tab->zoomOut();
        Preferences::instance().setFontSize(tab->fontSize());
    }
}

void MainWindow::resetFontSize() {
    EditorTab *tab = currentTab();
    if (tab) {
        tab->resetFontSize();
    }
}

void MainWindow::about() {
    QMessageBox::about(
        this, "About CinderDE",
        "<h3>CinderDE v0.6</h3>"
        "<p>A lightweight desktop code editor built with Qt 6 for Linux.</p>"
        "<p>Featuring Smart Editing, Syntax Highlighting, Smart Indentation, Bracket Pairing, Code Completion, and Build/Run Tools.</p>");
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!promptSaveAllIfModified()) {
        event->ignore();
        return;
    }
    saveSettings();
    event->accept();
}

} // namespace CinderDE
