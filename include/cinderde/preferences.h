#ifndef CINDERDE_PREFERENCES_H
#define CINDERDE_PREFERENCES_H

#include <QSettings>
#include <QString>
#include <QByteArray>

namespace CinderDE {

class Preferences {
public:
    static constexpr int DEFAULT_FONT_SIZE = 11;

    static Preferences& instance();

    int fontSize() const;
    void setFontSize(int size);

    bool wordWrap() const;
    void setWordWrap(bool enable);

    bool explorerVisible() const;
    void setExplorerVisible(bool visible);

    QString lastWorkspace() const;
    void setLastWorkspace(const QString &path);

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
