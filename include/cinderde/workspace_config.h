#ifndef CINDERDE_WORKSPACE_CONFIG_H
#define CINDERDE_WORKSPACE_CONFIG_H

#include <QString>

namespace CinderDE {

struct WorkspaceConfig {
    bool hasWordWrap = false;
    bool wordWrap = false;

    bool hasTabSize = false;
    int tabSize = 4;

    static WorkspaceConfig loadFromWorkspace(const QString &workspacePath);
    static bool saveToWorkspace(const QString &workspacePath, const WorkspaceConfig &config);
};

} // namespace CinderDE

#endif // CINDERDE_WORKSPACE_CONFIG_H
