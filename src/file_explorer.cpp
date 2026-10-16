#include "cinderde/file_explorer.h"
#include "cinderde/preferences.h"
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QKeyEvent>

namespace CinderDE {

FileExplorer::FileExplorer(QWidget *parent)
    : QWidget(parent),
      m_workspacePath("")
{
    m_stackedWidget = new QStackedWidget(this);

    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    setupWelcomeWidget();
    setupTreeWidget();

    m_treeView->installEventFilter(this);

    m_stackedWidget->addWidget(m_welcomeWidget);
    m_stackedWidget->addWidget(m_treeContainer);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_stackedWidget);

    setStyleSheet(
        "QTreeView { background-color: #252526; color: #cccccc; border: none; }"
        "QTreeView::item:hover { background-color: #2a2d2e; }"
        "QTreeView::item:selected { background-color: #37373d; color: #ffffff; }"
    );

    clearWorkspace();
}

void FileExplorer::setupWelcomeWidget() {
    m_welcomeWidget = new QWidget(this);
    m_welcomeWidget->setStyleSheet("background-color: #252526; color: #cccccc;");

    QVBoxLayout *layout = new QVBoxLayout(m_welcomeWidget);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    QLabel *header = new QLabel("EXPLORER", m_welcomeWidget);
    header->setStyleSheet("font-weight: bold; color: #aaaaaa; text-transform: uppercase; font-size: 11px;");

    QLabel *msgTitle = new QLabel("NO FOLDER OPEN", m_welcomeWidget);
    msgTitle->setStyleSheet("font-weight: bold; font-size: 13px; color: #e0e0e0; margin-top: 20px;");
    msgTitle->setAlignment(Qt::AlignCenter);

    QLabel *msgSub = new QLabel("You have not yet opened a folder as a workspace.", m_welcomeWidget);
    msgSub->setWordWrap(true);
    msgSub->setAlignment(Qt::AlignCenter);
    msgSub->setStyleSheet("color: #888888; font-size: 11px;");

    QPushButton *openBtn = new QPushButton("Open Folder", m_welcomeWidget);
    openBtn->setCursor(Qt::PointingHandCursor);
    openBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3c3c3c;"
        "  color: #ffffff;"
        "  border: 1px solid #ff6b35;"
        "  border-radius: 3px;"
        "  padding: 6px 12px;"
        "  font-weight: bold;"
        "  margin-top: 10px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #ff6b35;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #e05a2b;"
        "}"
    );

    connect(openBtn, &QPushButton::clicked, this, &FileExplorer::openFolderRequested);

    QWidget *recentContainer = new QWidget(m_welcomeWidget);
    m_recentContainerLayout = new QVBoxLayout(recentContainer);
    m_recentContainerLayout->setContentsMargins(0, 10, 0, 0);
    m_recentContainerLayout->setSpacing(4);

    QScrollArea *scrollArea = new QScrollArea(m_welcomeWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent;");
    scrollArea->setWidget(recentContainer);

    layout->addWidget(header);
    layout->addWidget(msgTitle);
    layout->addWidget(msgSub);
    layout->addWidget(openBtn, 0, Qt::AlignCenter);
    layout->addWidget(scrollArea, 1);

    updateRecentWorkspacesUI();
}

void FileExplorer::updateRecentWorkspacesUI() {
    if (!m_recentContainerLayout) return;

    // Clear existing layout items
    QLayoutItem *child;
    while ((child = m_recentContainerLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }

    QLabel *recentHeader = new QLabel("RECENT WORKSPACES", m_welcomeWidget);
    recentHeader->setStyleSheet("font-weight: bold; color: #aaaaaa; font-size: 11px; margin-top: 10px; text-transform: uppercase;");
    m_recentContainerLayout->addWidget(recentHeader);

    QStringList recentList = Preferences::instance().recentWorkspaces();
    if (recentList.isEmpty()) {
        QLabel *noRecentLabel = new QLabel("No recent workspaces", m_welcomeWidget);
        noRecentLabel->setStyleSheet("color: #777777; font-size: 11px; font-style: italic;");
        m_recentContainerLayout->addWidget(noRecentLabel);
    } else {
        for (const QString &path : recentList) {
            bool exists = QDir(path).exists();
            QFileInfo info(path);
            QString folderName = info.fileName();
            if (folderName.isEmpty()) folderName = path;

            QString btnText = folderName;
            if (!exists) {
                btnText += " (Missing)";
            }

            QPushButton *btn = new QPushButton(btnText, m_welcomeWidget);
            btn->setToolTip(path);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setFlat(true);

            if (exists) {
                btn->setStyleSheet(
                    "QPushButton {"
                    "  text-align: left;"
                    "  color: #ff6b35;"
                    "  background: transparent;"
                    "  border: none;"
                    "  padding: 4px 2px;"
                    "  font-size: 12px;"
                    "}"
                    "QPushButton:hover {"
                    "  color: #ffffff;"
                    "  background-color: #383838;"
                    "  border-radius: 3px;"
                    "}"
                );
            } else {
                btn->setStyleSheet(
                    "QPushButton {"
                    "  text-align: left;"
                    "  color: #888888;"
                    "  background: transparent;"
                    "  border: none;"
                    "  padding: 4px 2px;"
                    "  font-size: 12px;"
                    "  font-style: italic;"
                    "}"
                    "QPushButton:hover {"
                    "  color: #aaaaaa;"
                    "  background-color: #383838;"
                    "  border-radius: 3px;"
                    "}"
                );
            }

            connect(btn, &QPushButton::clicked, this, [this, path, exists]() {
                if (exists && QDir(path).exists()) {
                    emit openWorkspaceRequested(path);
                } else {
                    QMessageBox::warning(
                        this, "Workspace Missing",
                        QString("The workspace folder no longer exists:\n%1").arg(path));
                    Preferences::instance().removeRecentWorkspace(path);
                    updateRecentWorkspacesUI();
                }
            });

            m_recentContainerLayout->addWidget(btn);
        }
    }

    m_recentContainerLayout->addStretch();
}

void FileExplorer::setupTreeWidget() {
    m_treeContainer = new QWidget(this);

    m_treeView = new QTreeView(m_treeContainer);
    m_treeView->setModel(m_model);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(16);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);

    m_treeView->hideColumn(1);
    m_treeView->hideColumn(2);
    m_treeView->hideColumn(3);
    m_treeView->header()->hide();

    m_titleLabel = new QLabel("WORKSPACE", m_treeContainer);
    m_titleLabel->setStyleSheet("font-weight: bold; color: #aaaaaa; padding: 4px; text-transform: uppercase; font-size: 11px;");

    QVBoxLayout *treeLayout = new QVBoxLayout(m_treeContainer);
    treeLayout->setContentsMargins(2, 4, 2, 2);
    treeLayout->setSpacing(4);
    treeLayout->addWidget(m_titleLabel);
    treeLayout->addWidget(m_treeView);

    connect(m_treeView, &QTreeView::clicked, this, &FileExplorer::onItemClicked);
    connect(m_treeView, &QTreeView::doubleClicked, this, &FileExplorer::onItemClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &FileExplorer::showContextMenu);
}

void FileExplorer::setWorkspacePath(const QString &path) {
    if (path.isEmpty()) {
        clearWorkspace();
        return;
    }

    QDir dir(path);
    if (!dir.exists()) {
        clearWorkspace();
        return;
    }

    m_workspacePath = dir.canonicalPath();
    if (m_workspacePath.isEmpty()) {
        m_workspacePath = dir.absolutePath();
    }

    QModelIndex rootIndex = m_model->setRootPath(m_workspacePath);
    m_treeView->setRootIndex(rootIndex);

    QFileInfo info(m_workspacePath);
    QString folderName = info.fileName();
    if (folderName.isEmpty()) {
        folderName = m_workspacePath;
    }
    m_titleLabel->setText(QString("WORKSPACE: %1").arg(folderName.toUpper()));

    m_stackedWidget->setCurrentWidget(m_treeContainer);
    emit workspaceChanged(m_workspacePath);
}

void FileExplorer::clearWorkspace() {
    m_workspacePath.clear();
    m_model->setRootPath("");
    m_treeView->setRootIndex(QModelIndex());
    updateRecentWorkspacesUI();
    m_stackedWidget->setCurrentWidget(m_welcomeWidget);
    emit workspaceChanged("");
}

void FileExplorer::refresh() {
    if (!m_workspacePath.isEmpty() && QDir(m_workspacePath).exists()) {
        QModelIndex rootIndex = m_model->setRootPath(m_workspacePath);
        m_treeView->setRootIndex(rootIndex);
    }
}

void FileExplorer::onItemClicked(const QModelIndex &index) {
    if (!index.isValid()) return;

    QString filePath = m_model->filePath(index);
    QFileInfo info(filePath);
    if (info.isFile()) {
        emit fileSelected(filePath);
    }
}

QModelIndex FileExplorer::selectedIndex() const {
    QModelIndexList selected = m_treeView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) return QModelIndex();
    return selected.first();
}

QString FileExplorer::selectedFilePath() const {
    QModelIndex index = selectedIndex();
    if (!index.isValid()) return m_workspacePath;
    return m_model->filePath(index);
}

void FileExplorer::showContextMenu(const QPoint &pos) {
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #252526; color: #cccccc; border: 1px solid #3c3c3c; }"
        "QMenu::item:selected { background-color: #094771; color: #ffffff; }"
    );

    menu.addAction("New File", this, &FileExplorer::createNewFile);
    menu.addAction("New Folder", this, &FileExplorer::createNewFolder);

    QModelIndex index = m_treeView->indexAt(pos);
    if (index.isValid()) {
        menu.addSeparator();
        menu.addAction("Rename", this, &FileExplorer::renameSelected);
        menu.addAction("Delete", this, &FileExplorer::deleteSelected);
    }

    menu.addSeparator();
    menu.addAction("Refresh Explorer", this, &FileExplorer::refresh);

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void FileExplorer::createNewFile() {
    if (m_workspacePath.isEmpty()) return;

    QString dirPath = selectedFilePath();
    QFileInfo info(dirPath);
    if (info.isFile()) {
        dirPath = info.absolutePath();
    }

    if (dirPath.isEmpty()) dirPath = m_workspacePath;

    bool ok;
    QString fileName = QInputDialog::getText(this, "New File", "File name:", QLineEdit::Normal, "", &ok);
    if (ok && !fileName.trimmed().isEmpty()) {
        QString fullPath = QDir(dirPath).filePath(fileName.trimmed());
        if (QFile::exists(fullPath)) {
            QMessageBox::warning(this, "File Exists", QString("A file or directory named '%1' already exists.").arg(fileName.trimmed()));
            return;
        }
        QFile file(fullPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            refresh();
            emit fileSelected(fullPath);
        } else {
            QMessageBox::critical(this, "Error", QString("Failed to create file:\n%1").arg(file.errorString()));
        }
    }
}

void FileExplorer::createNewFolder() {
    if (m_workspacePath.isEmpty()) return;

    QString dirPath = selectedFilePath();
    QFileInfo info(dirPath);
    if (info.isFile()) {
        dirPath = info.absolutePath();
    }

    if (dirPath.isEmpty()) dirPath = m_workspacePath;

    bool ok;
    QString folderName = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, "", &ok);
    if (ok && !folderName.trimmed().isEmpty()) {
        QDir dir(dirPath);
        if (dir.mkdir(folderName.trimmed())) {
            refresh();
        } else {
            QMessageBox::critical(this, "Error", "Failed to create directory.");
        }
    }
}

void FileExplorer::renameSelected() {
    QModelIndex index = selectedIndex();
    if (!index.isValid()) return;

    QString currentPath = m_model->filePath(index);
    QFileInfo info(currentPath);

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal, info.fileName(), &ok);
    if (ok && !newName.trimmed().isEmpty() && newName != info.fileName()) {
        QString newPath = QDir(info.absolutePath()).filePath(newName.trimmed());
        if (QFile::rename(currentPath, newPath)) {
            refresh();
            emit fileRenamed(currentPath, newPath);
        } else {
            QMessageBox::critical(this, "Error", "Failed to rename file/directory.");
        }
    }
}

bool FileExplorer::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_treeView && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_F2) {
            renameSelected();
            return true;
        } else if (keyEvent->key() == Qt::Key_Delete) {
            deleteSelected();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void FileExplorer::deleteSelected() {
    QModelIndex index = selectedIndex();
    if (!index.isValid()) return;

    QString path = m_model->filePath(index);
    QFileInfo info(path);

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Delete",
        QString("Are you sure you want to delete '%1'?").arg(info.fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        bool success = false;
        if (info.isDir()) {
            success = QDir(path).removeRecursively();
        } else {
            success = QFile::remove(path);
        }

        if (success) {
            refresh();
            emit fileDeleted(path);
        } else {
            QMessageBox::critical(this, "Error", "Failed to delete file/directory.");
        }
    }
}

} // namespace CinderDE
