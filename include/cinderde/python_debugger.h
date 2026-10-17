#ifndef CINDERDE_PYTHON_DEBUGGER_H
#define CINDERDE_PYTHON_DEBUGGER_H

#include <QObject>
#include <QSet>
#include <QHash>
#include <QString>
#include <QJsonObject>
#include "cinderde/debug_adapter_client.h"
#include "cinderde/debugger_config.h"

namespace CinderDE {

class PythonDebugger : public QObject {
    Q_OBJECT
public:
    explicit PythonDebugger(QObject *parent = nullptr);
    bool start(const DebuggerConfiguration &config, const QString &programPath,
               const QString &workspacePath, const QSet<int> &breakpointLines);
    bool isActive() const;
    QString programPath() const { return m_programPath; }
    void setBreakpoints(const QSet<int> &lines);
    void continueExecution();
    void pauseExecution();
    void stepOver();
    void stepInto();
    void stepOut();
    void stop();

signals:
    void outputReceived(const QString &text);
    void currentLineChanged(const QString &filePath, int line);
    void stateChanged(bool active, const QString &status);
    void sessionFinished();

private:
    DebugAdapterClient *m_client;
    QString m_programPath;
    QString m_workspacePath;
    QString m_workingDirectory;
    QSet<int> m_breakpointLines;
    int m_threadId = 0;
    int m_frameId = 0;
    QHash<int, QString> m_variableRequestNames;
    bool m_initialized = false;
    bool m_launching = false;
    bool m_stopping = false;

    void sendBreakpoints();
    void requestStackTrace(int threadId);
    void requestScopes(int frameId);
    void requestVariables(int variablesReference, const QString &scopeName);
    void reportResponseFailure(const QString &command, const QString &message);
};

} // namespace CinderDE

#endif // CINDERDE_PYTHON_DEBUGGER_H
