#include "cinderde/debugger_config.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace CinderDE {
namespace {
QString settingsFilePath(const QString &workspacePath)
{
    return QDir(workspacePath).filePath(".cinderde/settings.json");
}
}

QList<DebuggerConfiguration> DebuggerConfigStore::load(const QString &workspacePath)
{
    QList<DebuggerConfiguration> configurations;
    if (workspacePath.isEmpty() || !QDir(workspacePath).exists())
        return configurations;

    QFile file(settingsFilePath(workspacePath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return configurations;

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return configurations;

    const QJsonArray array = document.object().value(QStringLiteral("debuggers")).toArray();
    for (const QJsonValue &value : array) {
        if (!value.isObject())
            continue;
        const QJsonObject object = value.toObject();
        DebuggerConfiguration config;
        config.id = object.value(QStringLiteral("id")).toString();
        config.name = object.value(QStringLiteral("name")).toString();
        config.languageId = object.value(QStringLiteral("languageId")).toString();
        config.executable = object.value(QStringLiteral("executable")).toString();
        config.runtimeExecutable = object.value(QStringLiteral("runtimeExecutable")).toString();
        config.workingDirectory = object.value(QStringLiteral("workingDirectory")).toString();
        config.enabled = object.value(QStringLiteral("enabled")).toBool(true);
        for (const QJsonValue &argument : object.value(QStringLiteral("arguments")).toArray()) {
            if (argument.isString())
                config.arguments.append(argument.toString());
        }
        if (!config.id.isEmpty() && !config.name.isEmpty() && !config.executable.isEmpty())
            configurations.append(config);
    }
    return configurations;
}

bool DebuggerConfigStore::save(const QString &workspacePath,
                               const QList<DebuggerConfiguration> &configurations)
{
    if (workspacePath.isEmpty() || !QDir(workspacePath).exists())
        return false;

    QDir workspace(workspacePath);
    if (!workspace.exists(QStringLiteral(".cinderde")) &&
        !workspace.mkpath(QStringLiteral(".cinderde")))
        return false;

    QJsonArray array;
    for (const DebuggerConfiguration &config : configurations) {
        QJsonArray arguments;
        for (const QString &argument : config.arguments)
            arguments.append(argument);
        array.append(QJsonObject{
            {QStringLiteral("id"), config.id},
            {QStringLiteral("name"), config.name},
            {QStringLiteral("languageId"), config.languageId},
            {QStringLiteral("executable"), config.executable},
            {QStringLiteral("runtimeExecutable"), config.runtimeExecutable},
            {QStringLiteral("arguments"), arguments},
            {QStringLiteral("workingDirectory"), config.workingDirectory},
            {QStringLiteral("enabled"), config.enabled}
        });
    }

    const QString path = settingsFilePath(workspacePath);
    QJsonObject root;
    QFile existing(path);
    if (existing.exists() && existing.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(existing.readAll(), &error);
        if (error.error == QJsonParseError::NoError && document.isObject())
            root = document.object();
        existing.close();
    }
    root.insert(QStringLiteral("debuggers"), array);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return file.commit();
}

} // namespace CinderDE
