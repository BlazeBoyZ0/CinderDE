#include "cinderde/workspace_config.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace CinderDE {

WorkspaceConfig WorkspaceConfig::loadFromWorkspace(const QString &workspacePath) {
    WorkspaceConfig config;
    if (workspacePath.isEmpty()) return config;

    QDir dir(workspacePath);
    if (!dir.exists()) return config;

    QString filePath = dir.filePath(".cinderde/settings.json");
    if (!QFile::exists(filePath)) return config;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return config;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return config;
    }

    QJsonObject root = doc.object();

    // Check word wrap setting
    if (root.contains("editor.wordWrap") && root.value("editor.wordWrap").isBool()) {
        config.hasWordWrap = true;
        config.wordWrap = root.value("editor.wordWrap").toBool();
    } else if (root.contains("wordWrap") && root.value("wordWrap").isBool()) {
        config.hasWordWrap = true;
        config.wordWrap = root.value("wordWrap").toBool();
    } else if (root.contains("editor") && root.value("editor").isObject()) {
        QJsonObject edObj = root.value("editor").toObject();
        if (edObj.contains("wordWrap") && edObj.value("wordWrap").isBool()) {
            config.hasWordWrap = true;
            config.wordWrap = edObj.value("wordWrap").toBool();
        }
    }

    // Check tab size setting
    auto extractTabSize = [](const QJsonValue &val, int &outSize) -> bool {
        if (val.isDouble()) {
            int s = val.toInt();
            if (s >= 1 && s <= 16) {
                outSize = s;
                return true;
            }
        }
        return false;
    };

    if (root.contains("editor.tabSize") && extractTabSize(root.value("editor.tabSize"), config.tabSize)) {
        config.hasTabSize = true;
    } else if (root.contains("tabSize") && extractTabSize(root.value("tabSize"), config.tabSize)) {
        config.hasTabSize = true;
    } else if (root.contains("editor") && root.value("editor").isObject()) {
        QJsonObject edObj = root.value("editor").toObject();
        if (edObj.contains("tabSize") && extractTabSize(edObj.value("tabSize"), config.tabSize)) {
            config.hasTabSize = true;
        }
    }

    auto extractString = [](const QJsonObject &obj, const QString &key, QString &outStr) -> bool {
        if (obj.contains(key) && obj.value(key).isString()) {
            outStr = obj.value(key).toString();
            return true;
        }
        return false;
    };

    // Build command
    if (extractString(root, "buildCommand", config.buildCommand) ||
        extractString(root, "build.command", config.buildCommand)) {
        config.hasBuildCommand = true;
    }

    // Run command
    if (extractString(root, "runCommand", config.runCommand) ||
        extractString(root, "run.command", config.runCommand)) {
        config.hasRunCommand = true;
    }

    // Working directory
    if (extractString(root, "workingDirectory", config.workingDirectory) ||
        extractString(root, "build.workingDirectory", config.workingDirectory)) {
        config.hasWorkingDirectory = true;
    }

    // Execution mode
    if (extractString(root, "executionMode", config.executionMode) ||
        extractString(root, "execution.mode", config.executionMode)) {
        config.hasExecutionMode = true;
    }

    // Terminal command
    if (extractString(root, "terminalCommand", config.terminalCommand) ||
        extractString(root, "terminal.command", config.terminalCommand)) {
        config.hasTerminalCommand = true;
    }

    return config;
}

bool WorkspaceConfig::saveToWorkspace(const QString &workspacePath, const WorkspaceConfig &config) {
    if (workspacePath.isEmpty()) return false;

    QDir dir(workspacePath);
    if (!dir.exists()) return false;

    if (!dir.exists(".cinderde")) {
        if (!dir.mkdir(".cinderde")) {
            return false;
        }
    }

    QString filePath = dir.filePath(".cinderde/settings.json");
    QJsonObject root;

    // Read existing object if present to preserve other keys
    if (QFile::exists(filePath)) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QJsonDocument existingDoc = QJsonDocument::fromJson(file.readAll());
            if (existingDoc.isObject()) {
                root = existingDoc.object();
            }
            file.close();
        }
    }

    if (config.hasWordWrap) {
        root["editor.wordWrap"] = config.wordWrap;
    }
    if (config.hasTabSize) {
        root["editor.tabSize"] = config.tabSize;
    }
    if (config.hasBuildCommand) {
        root["buildCommand"] = config.buildCommand;
    }
    if (config.hasRunCommand) {
        root["runCommand"] = config.runCommand;
    }
    if (config.hasWorkingDirectory) {
        root["workingDirectory"] = config.workingDirectory;
    }
    if (config.hasExecutionMode) {
        root["executionMode"] = config.executionMode;
    }
    if (config.hasTerminalCommand) {
        root["terminalCommand"] = config.terminalCommand;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

} // namespace CinderDE
