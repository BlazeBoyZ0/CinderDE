#include "cinderde/build_run_manager.h"
#include "cinderde/preferences.h"
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

namespace CinderDE {

BuildRunManager::BuildRunManager(QObject *parent)
    : QObject(parent),
      m_process(nullptr),
      m_state(Idle),
      m_runPendingAfterBuild(false)
{
}

BuildRunManager::~BuildRunManager() {
    stop();
}

QString BuildRunManager::detectTerminalEmulator() {
    static const QStringList candidates = {
        "x-terminal-emulator",
        "gnome-terminal",
        "konsole",
        "xfce4-terminal",
        "alacritty",
        "kitty",
        "xterm"
    };

    for (const QString &candidate : candidates) {
        if (!QStandardPaths::findExecutable(candidate).isEmpty()) {
            return candidate;
        }
    }
    return QString();
}

QString BuildRunManager::substituteMacros(const QString &cmd, const QString &filePath, const QString &workspacePath, const QString &workingDir) {
    if (cmd.isEmpty()) return QString();

    QFileInfo fileInfo(filePath);
    QString fileDir = fileInfo.absolutePath();
    QString fileName = fileInfo.fileName();
    QString fileBase = fileInfo.completeBaseName();
    QString fileExt = fileInfo.suffix();

    QString resolvedWorkingDir = workingDir;
    if (resolvedWorkingDir.isEmpty()) {
        resolvedWorkingDir = !workspacePath.isEmpty() ? workspacePath : fileDir;
    }

    QString execPath = QDir(fileDir).filePath(fileBase);

    QString result = cmd;
    result.replace("${file}", filePath);
    result.replace("${fileDir}", fileDir);
    result.replace("${fileName}", fileName);
    result.replace("${fileBase}", fileBase);
    result.replace("${fileExt}", fileExt);
    result.replace("${workspace}", workspacePath);
    result.replace("${workingDir}", resolvedWorkingDir);
    result.replace("${execPath}", execPath);

    return result;
}

QString BuildRunManager::resolveWorkingDirectory(const QString &filePath, const QString &workspacePath, const WorkspaceConfig &config) {
    if (config.hasWorkingDirectory && !config.workingDirectory.trimmed().isEmpty()) {
        return substituteMacros(config.workingDirectory.trimmed(), filePath, workspacePath, "");
    }
    if (!workspacePath.isEmpty() && QDir(workspacePath).exists()) {
        return workspacePath;
    }
    if (!filePath.isEmpty()) {
        return QFileInfo(filePath).absolutePath();
    }
    return QDir::currentPath();
}

std::pair<QString, QString> BuildRunManager::resolveCommands(const QString &filePath, const QString &workspacePath, const WorkspaceConfig &config) {
    QString buildCmd;
    QString runCmd;

    if (config.hasBuildCommand) {
        buildCmd = config.buildCommand;
    }
    if (config.hasRunCommand) {
        runCmd = config.runCommand;
    }

    if (!buildCmd.isEmpty() || !runCmd.isEmpty()) {
        return {buildCmd, runCmd};
    }

    // Check if CMake workspace
    if (!workspacePath.isEmpty() && QFile::exists(QDir(workspacePath).filePath("CMakeLists.txt"))) {
        buildCmd = "cmake -B build -S . && cmake --build build";
        // Default run command if build target exists or configured
        runCmd = config.hasRunCommand ? config.runCommand : "";
        return {buildCmd, runCmd};
    }

    // Single-file fallback defaults based on extension
    QFileInfo info(filePath);
    QString ext = info.suffix().toLower();

    if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "c++") {
        buildCmd = "g++ -g \"${file}\" -o \"${fileDir}/${fileBase}\"";
        runCmd = "\"${fileDir}/${fileBase}\"";
    } else if (ext == "c") {
        buildCmd = "gcc -g \"${file}\" -o \"${fileDir}/${fileBase}\"";
        runCmd = "\"${fileDir}/${fileBase}\"";
    } else if (ext == "py") {
        buildCmd = "";
        runCmd = "python3 \"${file}\"";
    } else if (ext == "sh") {
        buildCmd = "";
        runCmd = "bash \"${file}\"";
    }

    return {buildCmd, runCmd};
}

void BuildRunManager::build(const QString &filePath, const QString &workspacePath, const WorkspaceConfig &config) {
    if (isRunning()) {
        emit processError("A process is already running.", true);
        return;
    }

    m_currentFilePath = filePath;
    m_currentWorkspacePath = workspacePath;
    m_currentConfig = config;
    m_runPendingAfterBuild = false;

    auto commands = resolveCommands(filePath, workspacePath, config);
    QString workingDir = resolveWorkingDirectory(filePath, workspacePath, config);
    QString buildCmd = substituteMacros(commands.first, filePath, workspacePath, workingDir);

    if (buildCmd.trimmed().isEmpty()) {
        emit outputReceived("=== No build command specified or needed. ===\n", true);
        emit processFinished(0, QProcess::NormalExit, true);
        return;
    }

    startProcess(buildCmd, workingDir, true);
}

void BuildRunManager::run(const QString &filePath, const QString &workspacePath, const WorkspaceConfig &config) {
    if (isRunning()) {
        emit processError("A process is already running.", false);
        return;
    }

    m_currentFilePath = filePath;
    m_currentWorkspacePath = workspacePath;
    m_currentConfig = config;
    m_runPendingAfterBuild = false;

    auto commands = resolveCommands(filePath, workspacePath, config);
    QString workingDir = resolveWorkingDirectory(filePath, workspacePath, config);
    QString runCmd = substituteMacros(commands.second, filePath, workspacePath, workingDir);

    if (runCmd.trimmed().isEmpty()) {
        emit processError("No run command configured or determined for this file/project.", false);
        return;
    }

    QString mode = config.hasExecutionMode ? config.executionMode : Preferences::instance().executionMode();
    bool externalTerm = (mode == "external_terminal");
    QString termCmd = config.hasTerminalCommand ? config.terminalCommand : Preferences::instance().terminalCommand();

    if (externalTerm) {
        launchExternalTerminal(runCmd, workingDir, termCmd);
    } else {
        startProcess(runCmd, workingDir, false);
    }
}

void BuildRunManager::buildAndRun(const QString &filePath, const QString &workspacePath, const WorkspaceConfig &config) {
    if (isRunning()) {
        emit processError("A process is already running.", true);
        return;
    }

    m_currentFilePath = filePath;
    m_currentWorkspacePath = workspacePath;
    m_currentConfig = config;

    auto commands = resolveCommands(filePath, workspacePath, config);
    QString workingDir = resolveWorkingDirectory(filePath, workspacePath, config);
    QString buildCmd = substituteMacros(commands.first, filePath, workspacePath, workingDir);
    QString runCmd = substituteMacros(commands.second, filePath, workspacePath, workingDir);

    if (buildCmd.trimmed().isEmpty()) {
        // No build step, run directly
        run(filePath, workspacePath, config);
        return;
    }

    if (runCmd.trimmed().isEmpty()) {
        emit processError("No run command configured. Performing build only.", true);
        build(filePath, workspacePath, config);
        return;
    }

    m_runPendingAfterBuild = true;
    startProcess(buildCmd, workingDir, true);
}

void BuildRunManager::startProcess(const QString &command, const QString &workingDir, bool isBuild, bool externalTerminal, const QString &termCmd) {
    Q_UNUSED(externalTerminal);
    Q_UNUSED(termCmd);

    bool pendingRun = m_runPendingAfterBuild;
    stop();
    m_runPendingAfterBuild = pendingRun;

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(workingDir);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildRunManager::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildRunManager::onReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &BuildRunManager::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &BuildRunManager::onProcessErrorOccurred);

    m_state = isBuild ? Building : Running;
    emit stateChanged(true, isBuild ? "Building..." : "Running...");
    emit processStarted(command, isBuild);

    emit outputReceived(QString("=== %1: %2 ===\nWorking Directory: %3\n")
                            .arg(isBuild ? "BUILD STARTED" : "RUN STARTED")
                            .arg(command)
                            .arg(workingDir), isBuild);

    // Run via system shell to support arguments and pipelines safely
    m_process->start("sh", QStringList() << "-c" << command);
}

bool BuildRunManager::launchExternalTerminal(const QString &command, const QString &workingDir, const QString &termCmd) {
    QString termExe = termCmd.trimmed();
    if (termExe.isEmpty()) {
        termExe = detectTerminalEmulator();
    }

    if (termExe.isEmpty()) {
        emit processError("No supported external terminal emulator found. Please configure a terminal command in settings.", false);
        return false;
    }

    m_state = Running;
    emit stateChanged(true, "Running in External Terminal...");
    emit processStarted(command, false);

    emit outputReceived(QString("=== RUN STARTED (External Terminal) ===\nCommand: %1\nWorking Directory: %2\nTerminal: %3\n")
                            .arg(command)
                            .arg(workingDir)
                            .arg(termExe), false);

    QString formattedCmd;
    QString baseTerm = QFileInfo(termExe).fileName();

    if (baseTerm == "gnome-terminal") {
        formattedCmd = QString("%1 --working-directory=\"%2\" -- bash -c \"%3; echo; read -p 'Process finished. Press Enter to exit...'\"")
                           .arg(termExe, workingDir, command);
    } else if (baseTerm == "konsole") {
        formattedCmd = QString("%1 --workdir \"%2\" -e bash -c \"%3; echo; read -p 'Process finished. Press Enter to exit...'\"")
                           .arg(termExe, workingDir, command);
    } else if (baseTerm == "xfce4-terminal") {
        formattedCmd = QString("%1 --working-directory=\"%2\" -e \"bash -c '%3; echo; read -p \\\"Process finished. Press Enter to exit...\\\"'\"")
                           .arg(termExe, workingDir, command);
    } else if (baseTerm == "alacritty") {
        formattedCmd = QString("%1 --working-directory \"%2\" -e bash -c \"%3; echo; read -p 'Process finished. Press Enter to exit...'\"")
                           .arg(termExe, workingDir, command);
    } else if (baseTerm == "kitty") {
        formattedCmd = QString("%1 --directory \"%2\" bash -c \"%3; echo; read -p 'Process finished. Press Enter to exit...'\"")
                           .arg(termExe, workingDir, command);
    } else { // xterm, x-terminal-emulator or custom
        formattedCmd = QString("%1 -e bash -c \"cd '%2' && %3; echo; read -p 'Process finished. Press Enter to exit...'\"")
                           .arg(termExe, workingDir, command);
    }

    bool success = QProcess::startDetached("sh", QStringList() << "-c" << formattedCmd);
    if (!success) {
        m_state = Idle;
        emit stateChanged(false, "Failed to launch external terminal.");
        emit processError(QString("Failed to launch external terminal executable '%1'.").arg(termExe), false);
        return false;
    }

    emit outputReceived("[External Terminal] Process launched in external terminal session.\nNote: External terminal exit status is handled by the terminal session.\n", false);

    m_state = Idle;
    emit stateChanged(false, "Idle");
    emit processFinished(0, QProcess::NormalExit, false);
    return true;
}

void BuildRunManager::stop() {
    if (m_process) {
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            if (!m_process->waitForFinished(1000)) {
                m_process->kill();
            }
        }
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_runPendingAfterBuild = false;
    if (m_state != Idle) {
        m_state = Idle;
        emit stateChanged(false, "Process Stopped");
    }
}

void BuildRunManager::writeStdin(const QString &data) {
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->write(data.toUtf8());
    }
}

void BuildRunManager::onReadyReadStandardOutput() {
    if (!m_process) return;
    QByteArray data = m_process->readAllStandardOutput();
    emit outputReceived(QString::fromUtf8(data), m_state == Building);
}

void BuildRunManager::onReadyReadStandardError() {
    if (!m_process) return;
    QByteArray data = m_process->readAllStandardError();
    emit outputReceived(QString::fromUtf8(data), m_state == Building);
}

void BuildRunManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    bool wasBuild = (m_state == Building);

    emit outputReceived(QString("\n=== %1 (Exit code: %2) ===\n")
                            .arg(exitStatus == QProcess::NormalExit ? "PROCESS FINISHED" : "PROCESS CRASHED")
                            .arg(exitCode), wasBuild);

    emit processFinished(exitCode, exitStatus, wasBuild);

    m_state = Idle;
    emit stateChanged(false, "Idle");

    if (wasBuild) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit && m_runPendingAfterBuild) {
            m_runPendingAfterBuild = false;
            // Schedule run process asynchronously to allow process cleanup
            QString fPath = m_currentFilePath;
            QString wPath = m_currentWorkspacePath;
            WorkspaceConfig cfg = m_currentConfig;
            QTimer::singleShot(0, this, [this, fPath, wPath, cfg]() {
                run(fPath, wPath, cfg);
            });
        } else if (m_runPendingAfterBuild) {
            m_runPendingAfterBuild = false;
            emit outputReceived("=== BUILD FAILED. Execution aborted. ===\n", true);
            emit processError("Build failed. Binary execution was cancelled to prevent running stale executable.", false);
        }
    }
}

void BuildRunManager::onProcessErrorOccurred(QProcess::ProcessError error) {
    bool wasBuild = (m_state == Building);
    QString errorMsg;

    switch (error) {
        case QProcess::FailedToStart:
            errorMsg = "Process failed to start. Required binary, compiler, interpreter, or build tool was not found.";
            break;
        case QProcess::Crashed:
            errorMsg = "Process crashed unexpectedly.";
            break;
        case QProcess::Timedout:
            errorMsg = "Process operation timed out.";
            break;
        default:
            errorMsg = "An unknown process error occurred.";
            break;
    }

    emit outputReceived(QString("\n=== ERROR: %1 ===\n").arg(errorMsg), wasBuild);
    emit processError(errorMsg, wasBuild);

    if (m_runPendingAfterBuild) {
        m_runPendingAfterBuild = false;
        emit outputReceived("=== BUILD FAILED. Execution aborted. ===\n", true);
    }

    m_state = Idle;
    emit stateChanged(false, "Idle");
}

} // namespace CinderDE
