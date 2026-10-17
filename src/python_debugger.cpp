#include "cinderde/python_debugger.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>
#include <QProcess>
#include <QTimer>

namespace CinderDE {

PythonDebugger::PythonDebugger(QObject *parent)
    : QObject(parent), m_client(new DebugAdapterClient(this))
{
    connect(m_client, &DebugAdapterClient::started, this, [this]() {
        emit outputReceived(QStringLiteral("Debug adapter started. Initializing DAP..."));
        m_client->sendRequest(QStringLiteral("initialize"), QJsonObject{
            {QStringLiteral("clientID"), QStringLiteral("cinderde")},
            {QStringLiteral("clientName"), QStringLiteral("CinderDE")},
            {QStringLiteral("adapterID"), QStringLiteral("python")},
            {QStringLiteral("pathFormat"), QStringLiteral("path")},
            {QStringLiteral("linesStartAt1"), true},
            {QStringLiteral("columnsStartAt1"), true},
            {QStringLiteral("supportsConfigurationDoneRequest"), true},
            {QStringLiteral("supportsEvaluateForHovers"), false}
        });
    });
    connect(m_client, &DebugAdapterClient::responseReceived, this,
            [this](int requestSeq, const QString &command, bool success, const QJsonObject &body, const QString &message) {
        if (!success) {
            reportResponseFailure(command, message);
            if (command == QStringLiteral("launch") || command == QStringLiteral("initialize")) {
                m_stopping = true;
                m_client->stop();
            }
            return;
        }

        if (command == QStringLiteral("initialize")) {
            m_client->sendRequest(QStringLiteral("launch"), QJsonObject{
                {QStringLiteral("program"), m_programPath},
                {QStringLiteral("cwd"), m_workingDirectory},
                {QStringLiteral("console"), QStringLiteral("internalConsole")},
                {QStringLiteral("justMyCode"), true},
                {QStringLiteral("stopOnEntry"), false}
            });
        } else if (command == QStringLiteral("setBreakpoints")) {
            emit outputReceived(QStringLiteral("Breakpoints configured."));
        } else if (command == QStringLiteral("stackTrace")) {
            const QJsonArray frames = body.value(QStringLiteral("stackFrames")).toArray();
            QStringList lines;
            for (const QJsonValue &value : frames) {
                const QJsonObject frame = value.toObject();
                const QString name = frame.value(QStringLiteral("name")).toString(QStringLiteral("<frame>"));
                const int line = frame.value(QStringLiteral("line")).toInt();
                const QString source = frame.value(QStringLiteral("source")).toObject().value(QStringLiteral("path")).toString();
                lines.append(QStringLiteral("  %1 — %2:%3").arg(name, source, QString::number(line)));
            }
            emit outputReceived(QStringLiteral("Call stack:\n%1").arg(lines.join(QLatin1Char('\n'))));
            if (!frames.isEmpty()) {
                const QJsonObject topFrame = frames.first().toObject();
                m_frameId = topFrame.value(QStringLiteral("id")).toInt();
                const QString path = topFrame.value(QStringLiteral("source")).toObject().value(QStringLiteral("path")).toString(m_programPath);
                const int line = topFrame.value(QStringLiteral("line")).toInt();
                if (line > 0)
                    emit currentLineChanged(path, line);
                requestScopes(m_frameId);
            }
        } else if (command == QStringLiteral("scopes")) {
            const QJsonArray scopes = body.value(QStringLiteral("scopes")).toArray();
            if (scopes.isEmpty()) {
                emit outputReceived(QStringLiteral("No scopes are available for this frame."));
            } else {
                for (const QJsonValue &value : scopes) {
                    const QJsonObject scope = value.toObject();
                    const int reference = scope.value(QStringLiteral("variablesReference")).toInt();
                    const QString name = scope.value(QStringLiteral("name")).toString(QStringLiteral("Variables"));
                    if (reference != 0)
                        requestVariables(reference, name);
                }
            }
        } else if (command == QStringLiteral("variables")) {
            const QString scopeName = m_variableRequestNames.take(requestSeq);
            const QJsonArray variables = body.value(QStringLiteral("variables")).toArray();
            QStringList lines;
            for (const QJsonValue &value : variables) {
                const QJsonObject variable = value.toObject();
                QString line = QStringLiteral("  %1 = %2").arg(
                    variable.value(QStringLiteral("name")).toString(),
                    variable.value(QStringLiteral("value")).toString());
                if (variable.value(QStringLiteral("variablesReference")).toInt() > 0)
                    line += QStringLiteral("  [expandable]");
                lines.append(line);
            }
            emit outputReceived(QStringLiteral("%1:\n%2")
                                .arg(scopeName.isEmpty() ? QStringLiteral("Variables") : scopeName,
                                     lines.join(QLatin1Char('\n'))));
        } else if (command == QStringLiteral("disconnect")) {
            m_client->stop();
        }
    });

    connect(m_client, &DebugAdapterClient::requestReceived, this,
            [this](int requestSeq, const QString &command, const QJsonObject &) {
        emit outputReceived(QStringLiteral("Adapter requested unsupported operation: %1").arg(command));
        m_client->sendResponse(requestSeq, command, false, QJsonObject{},
                               QStringLiteral("This DAP request is not supported by CinderDE yet."));
    });

    connect(m_client, &DebugAdapterClient::eventReceived, this,
            [this](const QString &event, const QJsonObject &body) {
        if (event == QStringLiteral("initialized")) {
            m_initialized = true;
            sendBreakpoints();
            m_client->sendRequest(QStringLiteral("configurationDone"));
            emit outputReceived(QStringLiteral("Debugger initialized."));
            emit stateChanged(true, QStringLiteral("Debugging"));
        } else if (event == QStringLiteral("stopped")) {
            m_threadId = body.value(QStringLiteral("threadId")).toInt();
            emit outputReceived(QStringLiteral("Execution paused: %1").arg(
                body.value(QStringLiteral("reason")).toString(QStringLiteral("breakpoint"))));
            requestStackTrace(m_threadId);
            emit stateChanged(true, QStringLiteral("Paused"));
        } else if (event == QStringLiteral("continued")) {
            emit stateChanged(true, QStringLiteral("Running"));
        } else if (event == QStringLiteral("output")) {
            const QString text = body.value(QStringLiteral("output")).toString();
            if (!text.isEmpty())
                emit outputReceived(text.trimmed());
        } else if (event == QStringLiteral("exited")) {
            emit outputReceived(QStringLiteral("Debugged program exited (code %1).").arg(
                body.value(QStringLiteral("exitCode")).toInt()));
        } else if (event == QStringLiteral("terminated")) {
            emit outputReceived(QStringLiteral("Debugging session terminated."));
            if (m_client->isRunning())
                m_client->stop();
        } else if (event == QStringLiteral("thread")) {
            const int threadId = body.value(QStringLiteral("threadId")).toInt();
            if (body.value(QStringLiteral("reason")).toString() == QStringLiteral("started"))
                m_threadId = threadId;
        } else if (event == QStringLiteral("breakpoint")) {
            emit outputReceived(QStringLiteral("Breakpoint status changed."));
        }
    });

    connect(m_client, &DebugAdapterClient::outputReceived, this,
            [this](const QString &text, bool standardError) {
        emit outputReceived(standardError ? QStringLiteral("[adapter] %1").arg(text.trimmed()) : text.trimmed());
    });
    connect(m_client, &DebugAdapterClient::processError, this, [this](const QString &message) {
        emit outputReceived(QStringLiteral("Debugger error: %1").arg(message));
        if (!m_client->isRunning() && m_launching) {
            m_launching = false;
            emit stateChanged(false, QStringLiteral("Debugger failed to start"));
            emit sessionFinished();
        }
    });
    connect(m_client, &DebugAdapterClient::protocolError, this, [this](const QString &message) {
        emit outputReceived(QStringLiteral("Debugger protocol error: %1").arg(message));
    });
    connect(m_client, &DebugAdapterClient::finished, this,
            [this](int exitCode, QProcess::ExitStatus) {
        if (!m_stopping && exitCode != 0)
            emit outputReceived(QStringLiteral("Debug adapter exited with code %1.").arg(exitCode));
        m_initialized = false;
        m_launching = false;
        m_stopping = false;
        emit stateChanged(false, QStringLiteral("Debugger stopped"));
        emit sessionFinished();
    });
}

bool PythonDebugger::start(const DebuggerConfiguration &config, const QString &programPath,
                           const QString &workspacePath, const QSet<int> &breakpointLines)
{
    if (m_client->isRunning() || m_launching)
        return false;
    const QFileInfo programInfo(programPath);
    if (!programInfo.exists() || !programInfo.isFile()) {
        emit outputReceived(QStringLiteral("Save the current file before starting debugging."));
        return false;
    }
    if (config.executable.trimmed().isEmpty()) {
        emit outputReceived(QStringLiteral("Choose a debugger adapter in Tools → Debuggers first."));
        return false;
    }

    m_programPath = programInfo.absoluteFilePath();
    m_workingDirectory = config.workingDirectory.trimmed().isEmpty()
        ? (workspacePath.isEmpty() ? programInfo.absolutePath() : QDir(workspacePath).absolutePath())
        : (QDir::isAbsolutePath(config.workingDirectory)
            ? QDir::cleanPath(config.workingDirectory)
            : QDir(workspacePath).absoluteFilePath(config.workingDirectory));
    m_breakpointLines = breakpointLines;
    m_threadId = 0;
    m_frameId = 0;
    m_variableRequestNames.clear();
    m_initialized = false;
    m_launching = true;
    emit outputReceived(QStringLiteral("Starting debugger adapter: %1").arg(config.executable));
    emit stateChanged(true, QStringLiteral("Starting debugger"));
    if (!m_client->start(config.executable, config.arguments, m_workingDirectory)) {
        m_launching = false;
        emit stateChanged(false, QStringLiteral("Debugger failed to start"));
        return false;
    }
    return true;
}

bool PythonDebugger::isActive() const
{
    return m_client->isRunning() || m_launching;
}

void PythonDebugger::sendBreakpoints()
{
    if (!m_initialized)
        return;
    QJsonArray breakpoints;
    for (int line : m_breakpointLines) {
        if (line > 0)
            breakpoints.append(QJsonObject{{QStringLiteral("line"), line}});
    }
    m_client->sendRequest(QStringLiteral("setBreakpoints"), QJsonObject{
        {QStringLiteral("source"), QJsonObject{{QStringLiteral("path"), m_programPath}}},
        {QStringLiteral("breakpoints"), breakpoints},
        {QStringLiteral("sourceModified"), false}
    });
}

void PythonDebugger::setBreakpoints(const QSet<int> &lines)
{
    m_breakpointLines = lines;
    sendBreakpoints();
}

void PythonDebugger::requestStackTrace(int threadId)
{
    if (threadId <= 0)
        return;
    m_client->sendRequest(QStringLiteral("stackTrace"), QJsonObject{
        {QStringLiteral("threadId"), threadId},
        {QStringLiteral("startFrame"), 0},
        {QStringLiteral("levels"), 20}
    });
}

void PythonDebugger::requestScopes(int frameId)
{
    m_client->sendRequest(QStringLiteral("scopes"), QJsonObject{{QStringLiteral("frameId"), frameId}});
}

void PythonDebugger::requestVariables(int variablesReference, const QString &scopeName)
{
    const int requestSeq = m_client->sendRequest(QStringLiteral("variables"), QJsonObject{
        {QStringLiteral("variablesReference"), variablesReference}
    });
    m_variableRequestNames.insert(requestSeq, scopeName);
}

void PythonDebugger::continueExecution()
{
    if (m_initialized && m_threadId > 0)
        m_client->sendRequest(QStringLiteral("continue"), QJsonObject{{QStringLiteral("threadId"), m_threadId}});
}

void PythonDebugger::pauseExecution()
{
    if (m_initialized && m_threadId > 0)
        m_client->sendRequest(QStringLiteral("pause"), QJsonObject{{QStringLiteral("threadId"), m_threadId}});
}

void PythonDebugger::stepOver()
{
    if (m_initialized && m_threadId > 0)
        m_client->sendRequest(QStringLiteral("next"), QJsonObject{{QStringLiteral("threadId"), m_threadId}});
}

void PythonDebugger::stepInto()
{
    if (m_initialized && m_threadId > 0)
        m_client->sendRequest(QStringLiteral("stepIn"), QJsonObject{{QStringLiteral("threadId"), m_threadId}});
}

void PythonDebugger::stepOut()
{
    if (m_initialized && m_threadId > 0)
        m_client->sendRequest(QStringLiteral("stepOut"), QJsonObject{{QStringLiteral("threadId"), m_threadId}});
}

void PythonDebugger::stop()
{
    if (!m_client->isRunning()) {
        m_launching = false;
        return;
    }
    m_stopping = true;
    if (m_initialized) {
        m_client->sendRequest(QStringLiteral("disconnect"), QJsonObject{
            {QStringLiteral("restart"), false},
            {QStringLiteral("terminateDebuggee"), true}
        });
        QTimer::singleShot(1500, this, [this]() {
            if (m_client->isRunning())
                m_client->stop();
        });
    } else {
        m_client->stop();
    }
}

void PythonDebugger::reportResponseFailure(const QString &command, const QString &message)
{
    emit outputReceived(QStringLiteral("Debugger request '%1' failed: %2")
                        .arg(command, message.isEmpty() ? QStringLiteral("unspecified adapter error") : message));
}

} // namespace CinderDE
