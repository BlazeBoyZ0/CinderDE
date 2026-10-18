#ifndef CINDERDE_PYTHON_DEBUGGER_H
#define CINDERDE_PYTHON_DEBUGGER_H

#include <QObject>
#include <QSet>
#include <QHash>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include "cinderde/debug_adapter_client.h"
#include "cinderde/debugger_config.h"

namespace CinderDE {

class DebuggerController : public QObject {
    Q_OBJECT
public:
    explicit DebuggerController(QObject *parent = nullptr);
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
    void selectFrame(int frameId, const QString &path, int line);
    void expandVariables(int variablesReference, const QString &name);

signals:
    void outputReceived(const QString &text);
    void currentLineChanged(const QString &filePath, int line);
    void stateChanged(bool active, const QString &status);
    void sessionFinished();
    void stackFramesReceived(const QJsonArray &frames);
    void scopesReceived(const QJsonArray &scopes);
    void variablesReceived(int parentReference, const QString &scopeName, const QJsonArray &variables);

private:
    DebugAdapterClient *m_client;
    QString m_programPath;
    QString m_runtimeExecutable;
    QString m_workingDirectory;
    QString m_workspacePath;
    QString m_languageId;
    QString m_launchArgumentsJson;
    QSet<int> m_breakpointLines;
    int m_threadId = 0;
    int m_frameId = 0;
    QHash<int, QString> m_variableRequestNames;
    QHash<int, int> m_variableRequestReferences;
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
