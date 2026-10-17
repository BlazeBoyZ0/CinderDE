#ifndef CINDERDE_DEBUGGER_CONFIG_H
#define CINDERDE_DEBUGGER_CONFIG_H

#include <QString>
#include <QStringList>
#include <QList>

namespace CinderDE {

struct DebuggerConfiguration {
    QString id;
    QString name;
    QString languageId;
    QString executable;
    QStringList arguments;
    QString workingDirectory;
    bool enabled = true;
};

class DebuggerConfigStore {
public:
    static QList<DebuggerConfiguration> load(const QString &workspacePath);
    static bool save(const QString &workspacePath, const QList<DebuggerConfiguration> &configurations);
};

} // namespace CinderDE

#endif // CINDERDE_DEBUGGER_CONFIG_H
