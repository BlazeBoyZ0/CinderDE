#ifndef CINDERDE_BUILD_RUN_MANAGER_H
#define CINDERDE_BUILD_RUN_MANAGER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include "cinderde/workspace_config.h"

namespace CinderDE {

class BuildRunManager : public QObject {
    Q_OBJECT

public:
    enum State {
        Idle,
        Building,
        Running
    };

    explicit BuildRunManager(QObject *parent = nullptr);
    ~BuildRunManager() override;

    void build(const QString &filePath, const QString &workspacePath, const WorkspaceConfig &config);
    void run(const QString &filePath, const QString &workspacePath, const WorkspaceConfig &config);
    void buildAndRun(const QString &filePath, const QString &workspacePath, const WorkspaceConfig &config);
    void stop();
    void writeStdin(const QString &data);

    bool isRunning() const { return m_state != Idle; }
    State state() const { return m_state; }
    bool isExternalTerminalRunning() const { return m_isExternalTerminal; }

    static QString detectTerminalEmulator();
    static QString findCMakeExecutable(const QString &workspacePath, const QString &workingDir);
    static QString substituteMacros(const QString &cmd, const QString &filePath, const QString &workspacePath, const QString &workingDir);
    static QString resolveWorkingDirectory(const QString &filePath, const QString &workspacePath, const WorkspaceConfig &config);
    static std::pair<QString, QString> resolveCommands(const QString &filePath, const QString &workspacePath, const WorkspaceConfig &config);

signals:
    void outputReceived(const QString &text, bool isBuild);
    void processStarted(const QString &commandName, bool isBuild);
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus, bool isBuild);
    void processError(const QString &errorMsg, bool isBuild);
    void stateChanged(bool isRunning, const QString &statusText);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessErrorOccurred(QProcess::ProcessError error);

private:
    QProcess *m_process;
    State m_state;
    bool m_runPendingAfterBuild;
    bool m_isExternalTerminal{false};

    QString m_currentFilePath;
    QString m_currentWorkspacePath;
    WorkspaceConfig m_currentConfig;

    void startProcess(const QString &command, const QString &workingDir, bool isBuild, bool externalTerminal = false, const QString &termCmd = QString());
    bool launchExternalTerminal(const QString &command, const QString &workingDir, const QString &termCmd);
};

} // namespace CinderDE

#endif // CINDERDE_BUILD_RUN_MANAGER_H
