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
#include <QStackedWidget>

namespace CinderDE {

class FileExplorer : public QWidget {
    Q_OBJECT

public:
    explicit FileExplorer(QWidget *parent = nullptr);

    void setWorkspacePath(const QString &path);
    void clearWorkspace();
    void refresh();
    QString workspacePath() const { return m_workspacePath; }

signals:
    void fileSelected(const QString &filePath);
    void workspaceChanged(const QString &path);
    void fileRenamed(const QString &oldPath, const QString &newPath);
    void fileDeleted(const QString &path);
    void openFolderRequested();
    void openWorkspaceRequested(const QString &path);

public slots:
    void updateRecentWorkspacesUI();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onItemClicked(const QModelIndex &index);
    void showContextMenu(const QPoint &pos);
    void createNewFile();
    void createNewFolder();
    void renameSelected();
    void deleteSelected();

private:
    QStackedWidget *m_stackedWidget;
    QWidget *m_welcomeWidget;
    QWidget *m_treeContainer;
    QVBoxLayout *m_recentContainerLayout;

    QFileSystemModel *m_model;
    QTreeView *m_treeView;
    QLabel *m_titleLabel;
    QString m_workspacePath;

    void setupWelcomeWidget();
    void setupTreeWidget();
    QModelIndex selectedIndex() const;
    QString selectedFilePath() const;
};

} // namespace CinderDE

#endif // CINDERDE_FILE_EXPLORER_H
