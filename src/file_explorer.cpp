#include "cinderde/file_explorer.h"
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>

namespace CinderDE {

FileExplorer::FileExplorer(QWidget *parent)
    : QWidget(parent)
{
    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(16);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);

    // Hide extra columns like size, type, date modified by default for clean look
    m_treeView->hideColumn(1);
    m_treeView->hideColumn(2);
    m_treeView->hideColumn(3);
    m_treeView->header()->hide();

    m_titleLabel = new QLabel("EXPLORER", this);
    m_titleLabel->setStyleSheet("font-weight: bold; color: #aaaaaa; padding: 4px; text-transform: uppercase; font-size: 11px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 4, 2, 2);
    mainLayout->setSpacing(4);
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addWidget(m_treeView);

    connect(m_treeView, &QTreeView::doubleClicked, this, &FileExplorer::onItemDoubleClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &FileExplorer::showContextMenu);

    setStyleSheet(
        "QTreeView { background-color: #252526; color: #cccccc; border: none; }"
        "QTreeView::item:hover { background-color: #2a2d2e; }"
        "QTreeView::item:selected { background-color: #37373d; color: #ffffff; }"
    );
}

void FileExplorer::setWorkspacePath(const QString &path) {
    if (path.isEmpty() || !QDir(path).exists()) return;

    m_workspacePath = path;
    QModelIndex rootIndex = m_model->setRootPath(path);
    m_treeView->setRootIndex(rootIndex);

    QFileInfo info(path);
    m_titleLabel->setText(QString("EXPLORER: %1").arg(info.fileName().toUpper()));
    emit workspaceChanged(path);
}

void FileExplorer::onItemDoubleClicked(const QModelIndex &index) {
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

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void FileExplorer::createNewFile() {
    QString dirPath = selectedFilePath();
    QFileInfo info(dirPath);
    if (info.isFile()) {
        dirPath = info.absolutePath();
    }

    if (dirPath.isEmpty()) dirPath = m_workspacePath;
    if (dirPath.isEmpty()) return;

    bool ok;
    QString fileName = QInputDialog::getText(this, "New File", "File name:", QLineEdit::Normal, "", &ok);
    if (ok && !fileName.trimmed().isEmpty()) {
        QString fullPath = QDir(dirPath).filePath(fileName.trimmed());
        QFile file(fullPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            emit fileSelected(fullPath);
        } else {
            QMessageBox::critical(this, "Error", QString("Failed to create file:\n%1").arg(file.errorString()));
        }
    }
}

void FileExplorer::createNewFolder() {
    QString dirPath = selectedFilePath();
    QFileInfo info(dirPath);
    if (info.isFile()) {
        dirPath = info.absolutePath();
    }

    if (dirPath.isEmpty()) dirPath = m_workspacePath;
    if (dirPath.isEmpty()) return;

    bool ok;
    QString folderName = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, "", &ok);
    if (ok && !folderName.trimmed().isEmpty()) {
        QDir dir(dirPath);
        if (!dir.mkdir(folderName.trimmed())) {
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
        if (!QFile::rename(currentPath, newPath)) {
            QMessageBox::critical(this, "Error", "Failed to rename file/directory.");
        }
    }
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

        if (!success) {
            QMessageBox::critical(this, "Error", "Failed to delete file/directory.");
        }
    }
}

} // namespace CinderDE
