#include "cinderde/build_run_manager.h"
#include "cinderde/preferences.h"
#include "cinderde/language_pack.h"
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QMap>

namespace CinderDE {

BuildRunManager::BuildRunManager(QObject *parent)
    : QObject(parent),
      m_process(nullptr),
      m_state(Idle),
      m_runPendingAfterBuild(false),
      m_isExternalTerminal(false)
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
        "xterm",
        "lxterminal",
        "mate-terminal",
        "tilix",
        "foot",
        "wezterm"
    };

    for (const QString &candidate : candidates) {
        if (!QStandardPaths::findExecutable(candidate).isEmpty()) {
            return candidate;
        }
    }
    return QString();
}

QString BuildRunManager::findCMakeExecutable(const QString &workspacePath, const QString &workingDir) {
    QString wsPath = workspacePath;
    if (wsPath.isEmpty()) wsPath = workingDir;
    if (wsPath.isEmpty()) return QString();

    QString cmakeListsPath = QDir(wsPath).filePath("CMakeLists.txt");
    if (!QFile::exists(cmakeListsPath)) {
        return QString();
    }

    QFile file(cmakeListsPath);
    QString content;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        content = QString::fromUtf8(file.readAll());
        file.close();
    }

    QString projectName;
    QRegularExpression projRx(R"(project\s*\(\s*([A-Za-z0-9_.#+-]+))", QRegularExpression::CaseInsensitiveOption);
    auto projMatch = projRx.match(content);
    if (projMatch.hasMatch()) {
        projectName = projMatch.captured(1);
    }

    QStringList targets;
    QRegularExpression execRx(R"(add_executable\s*\(\s*([A-Za-z0-9_.#+-]+))", QRegularExpression::CaseInsensitiveOption);
    auto it = execRx.globalMatch(content);
    while (it.hasNext()) {
        auto match = it.next();
        QString targetName = match.captured(1);
        targets.append(targetName);
    }

    auto isTestTarget = [](const QString &t) {
        QString lower = t.toLower();
        return lower.contains("test") || lower.contains("check") || lower.contains("gtest") ||
               lower.contains("catch") || lower.contains("benchmark") || lower.contains("autogen");
    };

    QString chosenTarget;
    for (const QString &t : targets) {
        if (!isTestTarget(t) && t.compare(projectName, Qt::CaseInsensitive) == 0) {
            chosenTarget = t;
            break;
        }
    }

    if (chosenTarget.isEmpty()) {
        for (const QString &t : targets) {
            if (!isTestTarget(t)) {
                chosenTarget = t;
                break;
            }
        }
    }

    QString buildDir = QDir(wsPath).filePath("build");

    if (!chosenTarget.isEmpty()) {
        QStringList candidatePaths = {
            QDir(buildDir).filePath(chosenTarget),
            QDir(buildDir).filePath("bin/" + chosenTarget),
            QDir(buildDir).filePath("src/" + chosenTarget),
            QDir(buildDir).filePath("Debug/" + chosenTarget),
            QDir(buildDir).filePath("Release/" + chosenTarget)
        };

        for (const QString &cand : candidatePaths) {
            QFileInfo info(cand);
            if (info.exists() && info.isFile() && info.isExecutable()) {
                return cand;
            }
        }

        return QDir(buildDir).filePath(chosenTarget);
    }

    if (QDir(buildDir).exists()) {
        QDirIterator dirIt(buildDir, QDir::Files | QDir::Executable, QDirIterator::Subdirectories);
        while (dirIt.hasNext()) {
            dirIt.next();
            QFileInfo info = dirIt.fileInfo();
            QString fileName = info.fileName();
            QString path = info.filePath();

            if (isTestTarget(fileName)) continue;
            if (path.contains("/CMakeFiles/") || path.contains("_autogen/")) continue;
            if (fileName.endsWith(".so") || fileName.endsWith(".a") || fileName.endsWith(".o") || fileName.endsWith(".cmake")) continue;

            return path;
        }
    }

    return QString();
}

QString BuildRunManager::substituteMacros(const QString &cmd, const QString &filePath, const QString &workspacePath, const QString &workingDir) {
    if (cmd.isEmpty()) return QString();

    QFileInfo fileInfo(filePath);
    const QString fileDir = fileInfo.absolutePath();
    const QString fileName = fileInfo.fileName();
    const QString fileBase = fileInfo.completeBaseName();
    const QString fileExt = fileInfo.suffix();

    QString resolvedWorkingDir = workingDir;
    if (resolvedWorkingDir.isEmpty()) {
        resolvedWorkingDir = !workspacePath.isEmpty() ? workspacePath : fileDir;
    }

    const QMap<QString, QString> macros = {
        {"${file}", filePath},
        {"${fileDir}", fileDir},
        {"${fileName}", fileName},
        {"${fileBase}", fileBase},
        {"${fileExt}", fileExt},
        {"${workspace}", workspacePath},
        {"${workingDir}", resolvedWorkingDir},
        {"${execPath}", QDir(fileDir).filePath(fileBase)}
    };

    auto needsShellQuote = [](const QString &val) {
        if (val.isEmpty()) return false;
        static const QRegularExpression rx(R"([\s"'\\$&|;<>()`*?~#])");
        return val.contains(rx);
    };

    auto shellQuote = [](const QString &value) {
        QString escaped = value;
        escaped.replace("'", "'\\''");
        return QString("'") + escaped + "'";
    };

    QString result;
    result.reserve(cmd.size() + 64);
    enum class QuoteState { Unquoted, SingleQuoted, DoubleQuoted };
    QuoteState quoteState = QuoteState::Unquoted;
    bool escaped = false;

    for (qsizetype i = 0; i < cmd.size();) {
        if (!escaped) {
            bool replaced = false;
            for (auto it = macros.cbegin(); it != macros.cend(); ++it) {
                if (cmd.mid(i, it.key().size()) != it.key()) continue;

                QString value = it.value();
                if (quoteState == QuoteState::Unquoted) {
                    if (needsShellQuote(value)) {
                        result += shellQuote(value);
                    } else {
                        result += value;
                    }
                } else if (quoteState == QuoteState::SingleQuoted) {
                    value.replace("'", "'\\''");
                    result += value;
                } else {
                    value.replace("\\", "\\\\")
                         .replace("\"", "\\\"")
                         .replace("$", "\\$")
                         .replace("`", QStringLiteral("\\`"));
                    result += value;
                }

                i += it.key().size();
                replaced = true;
                break;
            }
            if (replaced) continue;
        }

        const QChar ch = cmd.at(i++);
        result += ch;

        if (escaped) {
            escaped = false;
            continue;
        }

        if (quoteState == QuoteState::SingleQuoted) {
            if (ch == '\'') quoteState = QuoteState::Unquoted;
            continue;
        }

        if (ch == '\\') {
            escaped = true;
        } else if (quoteState == QuoteState::DoubleQuoted) {
            if (ch == '"') quoteState = QuoteState::Unquoted;
        } else if (ch == '\'') {
            quoteState = QuoteState::SingleQuoted;
        } else if (ch == '"') {
            quoteState = QuoteState::DoubleQuoted;
        }
    }

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
        QString absoluteWorkspace = QDir::cleanPath(QFileInfo(workspacePath).absoluteFilePath());
        auto shellQuote = [](QString path) {
            path.replace("'", "'\\''");
            return QString("'") + path + "'";
        };
        QString quotedWorkspace = shellQuote(absoluteWorkspace);
        QString quotedBuildDir = shellQuote(QDir(absoluteWorkspace).filePath("build"));
        buildCmd = QString("cmake -B %1 -S %2 && cmake --build %1")
                       .arg(quotedBuildDir, quotedWorkspace);
        if (config.hasRunCommand && !config.runCommand.trimmed().isEmpty()) {
            runCmd = config.runCommand;
        } else {
            QString workingDir = resolveWorkingDirectory(filePath, workspacePath, config);
            runCmd = findCMakeExecutable(workspacePath, workingDir);
        }
        return {buildCmd, runCmd};
    }

    // Single-file fallback defaults based on extension
    QFileInfo info(filePath);
    QString ext = info.suffix().toLower();

    LanguageDefinition langDef = LanguageRegistry::instance().definitionForExtension(ext);
    if (langDef.isValid() && (!langDef.buildRun.buildCommand.isEmpty() || !langDef.buildRun.runCommand.isEmpty())) {
        buildCmd = langDef.buildRun.buildCommand;
        runCmd = langDef.buildRun.runCommand;
        return {buildCmd, runCmd};
    }

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
        emit outputReceived("=== ERROR: No run command configured or determined for this project. ===\n", false);
        emit processError("No run command configured or determined for this file/project.", false);
        return;
    }

    // Resolve path-like executables against the process working directory,
    // not CDE's own current directory. Never substitute a different executable
    // when an explicitly configured path is missing: that can run the wrong program.
    QString exeBin = QProcess::splitCommand(runCmd).value(0);
    if (!exeBin.isEmpty() && (exeBin.contains('/') || exeBin.contains('\\'))) {
        QString resolvedExe = QFileInfo(exeBin).isAbsolute()
                                  ? exeBin
                                  : QDir(workingDir).filePath(exeBin);
        if (!QFileInfo::exists(resolvedExe)) {
            emit outputReceived(QString("=== ERROR: Executable '%1' not found (checked relative to '%2') ===\n")
                                    .arg(exeBin, workingDir), false);
            emit processError(QString("Executable '%1' was not found relative to '%2'.")
                                  .arg(exeBin, workingDir), false);
            return;
        }
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
    m_isExternalTerminal = false;

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
        emit outputReceived("=== ERROR: No external terminal emulator found ===\n", false);
        emit processError("No supported external terminal emulator found. Please install a terminal emulator or configure a terminal command in settings.", false);
        return false;
    }

    QStringList splitCmd = QProcess::splitCommand(termExe);
    QString program = splitCmd.isEmpty() ? termExe : splitCmd.takeFirst();
    QStringList userArgs = splitCmd;

    QString programPath = QStandardPaths::findExecutable(program);
    if (programPath.isEmpty() && QFile::exists(program)) {
        programPath = program;
    }

    if (programPath.isEmpty()) {
        emit outputReceived(QString("=== ERROR: Terminal executable '%1' not found ===\n").arg(program), false);
        emit processError(QString("Failed to launch external terminal. Executable '%1' was not found.").arg(program), false);
        return false;
    }

    QFileInfo fi(programPath);
    QString canonical = fi.canonicalFilePath();
    QString baseTerm = QFileInfo(canonical.isEmpty() ? programPath : canonical).fileName().toLower();
    bool isWrapper = baseTerm.endsWith(".wrapper") || program.contains("x-terminal-emulator") || programPath.contains("x-terminal-emulator");

    if (baseTerm.endsWith(".wrapper")) baseTerm.chop(8);
    if (baseTerm.endsWith(".real")) baseTerm.chop(5);

    QString escapedDir = workingDir;
    escapedDir.replace('\\', "\\\\").replace('"', "\\\"").replace('$', "\\$").replace('`', "\\`");

    QString pauseCmd;
    if (!workingDir.trimmed().isEmpty()) {
        pauseCmd = QString("cd \"%1\" && %2 ; echo ; read -p 'Process finished. Press Enter to exit...'")
                       .arg(escapedDir, command);
    } else {
        pauseCmd = QString("%1 ; echo ; read -p 'Process finished. Press Enter to exit...'")
                       .arg(command);
    }

    QStringList args = userArgs;

    if (isWrapper || baseTerm == "x-terminal-emulator") {
        args << "-e" << "bash" << "-c" << pauseCmd;
    } else if (baseTerm == "xfce4-terminal") {
        if (!workingDir.trimmed().isEmpty()) {
            args << "--working-directory=" + workingDir;
        }
        args << "-x" << "bash" << "-c" << pauseCmd;
    } else if (baseTerm == "gnome-terminal" || baseTerm == "mate-terminal") {
        if (!workingDir.trimmed().isEmpty()) {
            args << "--working-directory=" + workingDir;
        }
        args << "--" << "bash" << "-c" << pauseCmd;
    } else if (baseTerm == "konsole") {
        if (!workingDir.trimmed().isEmpty()) {
            args << "--workdir" << workingDir;
        }
        args << "-e" << "bash" << "-c" << pauseCmd;
    } else if (baseTerm == "alacritty") {
        if (!workingDir.trimmed().isEmpty()) {
            args << "--working-directory" << workingDir;
        }
        args << "-e" << "bash" << "-c" << pauseCmd;
    } else if (baseTerm == "kitty") {
        if (!workingDir.trimmed().isEmpty()) {
            args << "--directory" << workingDir;
        }
        args << "bash" << "-c" << pauseCmd;
    } else if (baseTerm == "lxterminal") {
        if (!workingDir.trimmed().isEmpty()) {
            args << "--working-directory=" + workingDir;
        }
        args << "-e" << "bash" << "-c" << pauseCmd;
    } else if (baseTerm == "tilix") {
        if (!workingDir.trimmed().isEmpty()) {
            args << "-w" << workingDir;
        }
        args << "-e" << "bash" << "-c" << pauseCmd;
    } else if (baseTerm == "foot") {
        if (!workingDir.trimmed().isEmpty()) {
            args << "-D" << workingDir;
        }
        args << "bash" << "-c" << pauseCmd;
    } else if (baseTerm == "wezterm") {
        args << "start";
        if (!workingDir.trimmed().isEmpty()) {
            args << "--cwd" << workingDir;
        }
        args << "--" << "bash" << "-c" << pauseCmd;
    } else { // xterm, rxvt, st, or fallback
        args << "-e" << "bash" << "-c" << pauseCmd;
    }

    stop();

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(workingDir);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildRunManager::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildRunManager::onReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &BuildRunManager::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &BuildRunManager::onProcessErrorOccurred);

    m_state = Running;
    m_isExternalTerminal = true;
    emit stateChanged(true, "Running in External Terminal...");
    emit processStarted(command, false);

    emit outputReceived(QString("=== RUN STARTED (External Terminal) ===\nCommand: %1\nWorking Directory: %2\nTerminal: %3\n")
                            .arg(command)
                            .arg(workingDir)
                            .arg(programPath), false);

    m_process->start(programPath, args);
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
    m_isExternalTerminal = false;
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
    bool wasExternalTerminal = m_isExternalTerminal;

    if (wasExternalTerminal) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            emit outputReceived("\n=== EXTERNAL TERMINAL LAUNCHED SUCCESSFULLY ===\n(Program execution and output managed in terminal window)\n", false);
        } else {
            emit outputReceived(QString("\n=== EXTERNAL TERMINAL LAUNCHER FAILED (Exit code: %1) ===\n").arg(exitCode), false);
        }
        emit processFinished(exitCode, exitStatus, false);
    } else {
        emit outputReceived(QString("\n=== %1 (Exit code: %2) ===\n")
                                .arg(exitStatus == QProcess::NormalExit ? "PROCESS FINISHED" : "PROCESS CRASHED")
                                .arg(exitCode), wasBuild);

        emit processFinished(exitCode, exitStatus, wasBuild);
    }

    m_state = Idle;
    m_isExternalTerminal = false;
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
