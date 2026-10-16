#ifndef CINDERDE_FILE_EXPLORER_H
#define CINDERDE_FILE_EXPLORER_H

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMenu>

namespace CinderDE {

class FileExplorer : public QWidget {
    Q_OBJECT

public:
    explicit FileExplorer(QWidget *parent = nullptr);

    void setWorkspacePath(const QString &path);
    QString workspacePath() const { return m_workspacePath; }

signals:
    void fileSelected(const QString &filePath);
    void workspaceChanged(const QString &path);
    void fileRenamed(const QString &oldPath, const QString &newPath);
    void fileDeleted(const QString &path);

private slots:
    void onItemDoubleClicked(const QModelIndex &index);
    void showContextMenu(const QPoint &pos);
    void createNewFile();
    void createNewFolder();
    void renameSelected();
    void deleteSelected();

private:
    QFileSystemModel *m_model;
    QTreeView *m_treeView;
    QLabel *m_titleLabel;
    QString m_workspacePath;

    QModelIndex selectedIndex() const;
    QString selectedFilePath() const;
};

} // namespace CinderDE

#endif // CINDERDE_FILE_EXPLORER_H
