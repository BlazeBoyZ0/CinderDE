#ifndef CINDERDE_PREFERENCES_H
#define CINDERDE_PREFERENCES_H

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QList>
#include <QPair>

namespace CinderDE {

class Preferences {
public:
    static constexpr int DEFAULT_FONT_SIZE = 11;
    static constexpr int DEFAULT_TAB_SIZE = 4;
    static constexpr int MAX_RECENT_WORKSPACES = 10;

    static Preferences& instance();

    int fontSize() const;
    void setFontSize(int size);

    int tabSize() const;
    void setTabSize(int size);

    bool wordWrap() const;
    void setWordWrap(bool enable);

    bool explorerVisible() const;
    void setExplorerVisible(bool visible);

    QString lastWorkspace() const;
    void setLastWorkspace(const QString &path);

    // Recent Workspaces
    QStringList recentWorkspaces() const;
    void addRecentWorkspace(const QString &path);
    void removeRecentWorkspace(const QString &path);
    void clearRecentWorkspaces();

    // Session Restoration Settings
    bool sessionRestoreEnabled() const;
    void setSessionRestoreEnabled(bool enable);

    // Session State
    QString sessionWorkspace() const;
    void setSessionWorkspace(const QString &path);

    QStringList sessionOpenTabs() const;
    void setSessionOpenTabs(const QStringList &tabs);

    int sessionActiveTab() const;
    void setSessionActiveTab(int index);

    QList<QPair<int, int>> sessionCursorPositions() const;
    void setSessionCursorPositions(const QList<QPair<int, int>> &positions);

    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray &geometry);

    QByteArray windowState() const;
    void setWindowState(const QByteArray &state);

private:
    Preferences();
    QSettings m_settings;
};

} // namespace CinderDE

#endif // CINDERDE_PREFERENCES_H
