#ifndef CINDERDE_WORKSPACE_CONFIG_H
#define CINDERDE_WORKSPACE_CONFIG_H

#include <QString>

namespace CinderDE {

struct WorkspaceConfig {
    bool hasWordWrap = false;
    bool wordWrap = false;

    bool hasTabSize = false;
    int tabSize = 4;

    bool hasBuildCommand = false;
    QString buildCommand;

    bool hasRunCommand = false;
    QString runCommand;

    bool hasWorkingDirectory = false;
    QString workingDirectory;

    bool hasExecutionMode = false;
    QString executionMode; // "output_panel" or "external_terminal"

    bool hasTerminalCommand = false;
    QString terminalCommand;

    static WorkspaceConfig loadFromWorkspace(const QString &workspacePath);
    static bool saveToWorkspace(const QString &workspacePath, const WorkspaceConfig &config);
};

} // namespace CinderDE

#endif // CINDERDE_WORKSPACE_CONFIG_H
