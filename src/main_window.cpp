#include "cinderde/main_window.h"
#include "cinderde/preferences.h"
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QKeySequence>
#include <QDir>
#include <QStyle>

namespace CinderDE {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    applyTheme();
    loadSettings();

    if (m_tabWidget->count() == 0) {
        createNewTab();
    }
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUi() {
    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_fileExplorer = new FileExplorer(this);
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);

    m_splitter->addWidget(m_fileExplorer);
    m_splitter->addWidget(m_tabWidget);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes(QList<int>() << 220 << 780);

    setCentralWidget(m_splitter);

    setupMenus();
    setupToolbar();
    setupStatusBar();

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    connect(m_fileExplorer, &FileExplorer::fileSelected, this, [this](const QString &path) {
        EditorTab *existing = findTabByPath(path);
        if (existing) {
            m_tabWidget->setCurrentWidget(existing);
        } else {
            createNewTab(path);
        }
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

    fileMenu->addSeparator();

    QAction *saveAct = fileMenu->addAction("&Save", QKeySequence::Save, this, &MainWindow::saveFile);
    saveAct->setStatusTip("Save current document");

    QAction *saveAsAct = fileMenu->addAction("Save &As...", QKeySequence::SaveAs, this, &MainWindow::saveFileAs);
    saveAsAct->setStatusTip("Save current document with a new name");

    fileMenu->addSeparator();

    QAction *closeTabAct = fileMenu->addAction("&Close Tab", QKeySequence::Close, this, &MainWindow::closeCurrentTab);
    closeTabAct->setStatusTip("Close current document tab");

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
    editMenu->addAction("&Find...", QKeySequence::Find, this, &MainWindow::showFind);
    editMenu->addAction("&Replace...", QKeySequence(Qt::CTRL | Qt::Key_H), this, &MainWindow::showReplace);

    // --- View Menu ---
    QMenu *viewMenu = menuBar->addMenu("&View");

    m_toggleExplorerAction = viewMenu->addAction("Toggle &Explorer", QKeySequence(Qt::CTRL | Qt::Key_B), this, &MainWindow::toggleExplorer);
    m_toggleExplorerAction->setCheckable(true);
    m_toggleExplorerAction->setChecked(Preferences::instance().explorerVisible());

    m_wordWrapAction = viewMenu->addAction("Toggle &Word Wrap", QKeySequence(Qt::ALT | Qt::Key_Z), this, &MainWindow::toggleWordWrap);
    m_wordWrapAction->setCheckable(true);
    m_wordWrapAction->setChecked(Preferences::instance().wordWrap());

    viewMenu->addSeparator();

    viewMenu->addAction("Increase Font Size", QKeySequence::ZoomIn, this, &MainWindow::increaseFontSize);
    viewMenu->addAction("Decrease Font Size", QKeySequence::ZoomOut, this, &MainWindow::decreaseFontSize);
    viewMenu->addAction("Reset Font Size", QKeySequence(Qt::CTRL | Qt::Key_0), this, &MainWindow::resetFontSize);

    // --- Help Menu ---
    QMenu *helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction("&About CinderDE", this, &MainWindow::about);
}

void MainWindow::setupToolbar() {
    QToolBar *toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(18, 18));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    toolbar->addAction("New", this, &MainWindow::newFile);
    toolbar->addAction("Open", this, &MainWindow::openFile);
    toolbar->addAction("Save", this, &MainWindow::saveFile);
    toolbar->addSeparator();
    toolbar->addAction("Find", this, &MainWindow::showFind);
    toolbar->addSeparator();
    toolbar->addAction("Explorer", this, &MainWindow::toggleExplorer);
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
            spacing: 4px;
            padding: 2px;
        }
        QToolButton {
            background-color: #3c3c3c;
            color: #cccccc;
            border: 1px solid #454545;
            border-radius: 3px;
            padding: 3px 8px;
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
            padding: 6px 14px;
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
        QTabBar::close-button {
            image: none;
            subcontrol-origin: padding;
            subcontrol-position: right;
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

    QString lastWorkspace = prefs.lastWorkspace();
    if (!lastWorkspace.isEmpty() && QDir(lastWorkspace).exists()) {
        m_fileExplorer->setWorkspacePath(lastWorkspace);
    }
}

void MainWindow::saveSettings() {
    Preferences &prefs = Preferences::instance();
    prefs.setWindowGeometry(saveGeometry());
    prefs.setWindowState(saveState());
    prefs.setExplorerVisible(m_fileExplorer->isVisible());
    if (!m_fileExplorer->workspacePath().isEmpty()) {
        prefs.setLastWorkspace(m_fileExplorer->workspacePath());
    }
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
                return cur;
            } else {
                return nullptr;
            }
        }
    }

    EditorTab *tab = new EditorTab(this);
    tab->setFontSize(Preferences::instance().fontSize());
    tab->setWordWrap(m_wordWrapAction->isChecked());

    if (!filePath.isEmpty()) {
        if (!tab->openFile(filePath)) {
            delete tab;
            return nullptr;
        }
    }

    int idx = m_tabWidget->addTab(tab, tab->fileName());
    m_tabWidget->setCurrentIndex(idx);

    connect(tab, &EditorTab::modificationChanged, this, [this, tab](bool) {
        updateTabTitle(tab);
    });

    connect(tab, &EditorTab::cursorPositionChanged, this, [this](int line, int col) {
        m_statusCursorLabel->setText(QString("Ln %1, Col %2").arg(line).arg(col));
    });

    connect(tab, &EditorTab::fileTypeChanged, this, [this](const FileTypeInfo &info) {
        m_statusFileTypeLabel->setText(info.name);
    });

    updateTabTitle(tab);
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

void MainWindow::openFolder() {
    QString dirPath = QFileDialog::getExistingDirectory(this, "Open Workspace Folder");
    if (!dirPath.isEmpty()) {
        m_fileExplorer->setWorkspacePath(dirPath);
        Preferences::instance().setLastWorkspace(dirPath);
    }
}

void MainWindow::saveFile() {
    EditorTab *tab = currentTab();
    if (!tab) return;

    if (tab->isUntitled()) {
        saveFileAs();
    } else {
        if (tab->save()) {
            updateTabTitle(tab);
            statusBar()->showMessage("File saved", 2000);
        }
    }
}

void MainWindow::saveFileAs() {
    EditorTab *tab = currentTab();
    if (!tab) return;

    QString defaultName = tab->filePath().isEmpty() ? tab->fileName() : tab->filePath();
    QString filePath = QFileDialog::getSaveFileName(this, "Save File As", defaultName, "All Files (*)");
    if (!filePath.isEmpty()) {
        if (tab->saveAs(filePath)) {
            updateTabTitle(tab);
            statusBar()->showMessage("File saved", 2000);
        }
    }
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

void MainWindow::closeTab(int index) {
    EditorTab *tab = tabAt(index);
    if (!tab) return;

    if (promptSaveIfModified(tab)) {
        m_tabWidget->removeTab(index);
        delete tab;

        if (m_tabWidget->count() == 0) {
            createNewTab();
        }
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

void MainWindow::showFind() {
    if (currentTab()) currentTab()->showFindReplaceBar(false);
}

void MainWindow::showReplace() {
    if (currentTab()) currentTab()->showFindReplaceBar(true);
}

void MainWindow::toggleExplorer() {
    bool vis = !m_fileExplorer->isVisible();
    m_fileExplorer->setVisible(vis);
    m_toggleExplorerAction->setChecked(vis);
}

void MainWindow::toggleWordWrap() {
    bool wrap = !m_wordWrapAction->isChecked();
    m_wordWrapAction->setChecked(wrap);
    Preferences::instance().setWordWrap(wrap);

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        EditorTab *tab = tabAt(i);
        if (tab) tab->setWordWrap(wrap);
    }
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
        "<h3>CinderDE v0.1</h3>"
        "<p>A lightweight desktop code editor built with Qt 6 for Linux.</p>"
        "<p>Designed for responsive editing on low-end hardware with dark charcoal theme & ember-orange accents.</p>");
}

void MainWindow::closeEvent(QCloseEvent *event) {
    for (int i = m_tabWidget->count() - 1; i >= 0; --i) {
        EditorTab *tab = tabAt(i);
        if (tab && !promptSaveIfModified(tab)) {
            event->ignore();
            return;
        }
    }
    saveSettings();
    event->accept();
}

} // namespace CinderDE
