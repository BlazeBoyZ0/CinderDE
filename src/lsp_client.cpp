#include "cinderde/lsp_client.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QJsonArray>
#include <QDebug>
#include <unistd.h>

namespace CinderDE {

LspClient::LspClient(const LspServerConfig &config, const QString &workspacePath, QObject *parent)
    : QObject(parent),
      m_config(config),
      m_workspacePath(workspacePath),
      m_process(nullptr),
      m_nextRequestId(1)
{
    m_status.state = LspServerState::Stopped;
}

LspClient::~LspClient() {
    stop();
}

void LspClient::updateStatus(LspServerState state, const QString &errorMsg) {
    m_status.state = state;
    if (!errorMsg.isEmpty()) {
        m_status.lastError = errorMsg;
    }
    if (m_process && m_process->state() == QProcess::Running) {
        m_status.pid = m_process->processId();
        m_status.ramBytes = estimateRamUsageBytes();
    } else {
        m_status.pid = 0;
        m_status.ramBytes = -1;
    }
    emit statusChanged(m_status);
}

void LspClient::start() {
    if (m_status.state == LspServerState::Running || m_status.state == LspServerState::Starting) {
        return;
    }

    if (!m_config.isValid()) {
        updateStatus(LspServerState::Failed, "Invalid language server configuration");
        emit serverError("Invalid language server configuration");
        return;
    }

    updateStatus(LspServerState::Starting);

    m_process = new QProcess(this);
    m_process->setProgram(m_config.executable);
    m_process->setArguments(m_config.arguments);

    if (!m_workspacePath.isEmpty()) {
        m_process->setWorkingDirectory(m_workspacePath);
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (auto it = m_config.environment.begin(); it != m_config.environment.end(); ++it) {
        env.insert(it.key(), it.value());
    }
    m_process->setProcessEnvironment(env);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &LspClient::onProcessReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &LspClient::onProcessReadyReadStandardError);
    connect(m_process, &QProcess::errorOccurred, this, &LspClient::onProcessErrorOccurred);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &LspClient::onProcessFinished);

    m_readBuffer.clear();
    m_process->start();

    if (!m_process->waitForStarted(3000)) {
        QString err = m_process->errorString();
        updateStatus(LspServerState::Failed, QString("Failed to start executable '%1': %2").arg(m_config.executable, err));
        emit serverError(m_status.lastError);
        m_process->deleteLater();
        m_process = nullptr;
        return;
    }

    sendInitializeRequest();
}

void LspClient::stop() {
    if (m_status.state == LspServerState::Stopped) {
        return;
    }

    updateStatus(LspServerState::Stopping);

    if (m_process && m_process->state() == QProcess::Running) {
        sendRequest("shutdown", QJsonObject(), [this](const QJsonValue &, const QJsonObject &, bool) {
            sendNotification("exit", QJsonObject());
            if (m_process) {
                QTimer::singleShot(500, this, [this]() {
                    if (m_process && m_process->state() == QProcess::Running) {
                        m_process->terminate();
                        if (!m_process->waitForFinished(1000)) {
                            m_process->kill();
                        }
                    }
                });
            }
        });

        // Fallback timer in case server hangs on shutdown
        QTimer::singleShot(1500, this, [this]() {
            if (m_process && m_process->state() != QProcess::NotRunning) {
                m_process->kill();
            }
        });

        m_process->waitForFinished(2000);
    }

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_pendingRequests.clear();
    m_readBuffer.clear();
    updateStatus(LspServerState::Stopped);
}

void LspClient::sendJsonRpc(const QJsonObject &obj) {
    if (!m_process || m_process->state() != QProcess::Running) {
        return;
    }

    QJsonDocument doc(obj);
    QByteArray body = doc.toJson(QJsonDocument::Compact);

    QByteArray header;
    header.append("Content-Length: ");
    header.append(QByteArray::number(body.size()));
    header.append("\r\n\r\n");

    m_process->write(header);
    m_process->write(body);
}

int LspClient::sendRequest(const QString &method, const QJsonObject &params, ResponseCallback callback) {
    int id = m_nextRequestId++;
    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = id;
    req["method"] = method;
    if (!params.isEmpty()) {
        req["params"] = params;
    }

    if (callback) {
        PendingRequest pending;
        pending.callback = callback;
        pending.method = method;
        m_pendingRequests[id] = pending;
    }

    sendJsonRpc(req);
    return id;
}

void LspClient::sendNotification(const QString &method, const QJsonObject &params) {
    QJsonObject notif;
    notif["jsonrpc"] = "2.0";
    notif["method"] = method;
    if (!params.isEmpty()) {
        notif["params"] = params;
    }
    sendJsonRpc(notif);
}

void LspClient::sendInitializeRequest() {
    QJsonObject params;
    params["processId"] = static_cast<qint64>(QCoreApplication::applicationPid());

    if (!m_workspacePath.isEmpty()) {
        params["rootUri"] = pathToUri(m_workspacePath);
        params["rootPath"] = m_workspacePath;
    } else {
        params["rootUri"] = QJsonValue::Null;
        params["rootPath"] = QJsonValue::Null;
    }

    QJsonObject clientCaps;
    QJsonObject textDocumentCaps;

    // Completion capabilities
    QJsonObject completionCaps;
    completionCaps["snippetSupport"] = false;
    textDocumentCaps["completion"] = completionCaps;

    // Hover capabilities
    QJsonObject hoverCaps;
    QJsonArray contentFormat;
    contentFormat.append("plaintext");
    contentFormat.append("markdown");
    hoverCaps["contentFormat"] = contentFormat;
    textDocumentCaps["hover"] = hoverCaps;

    // Definition, References, Rename
    textDocumentCaps["definition"] = QJsonObject();
    textDocumentCaps["references"] = QJsonObject();
    textDocumentCaps["rename"] = QJsonObject();

    clientCaps["textDocument"] = textDocumentCaps;
    params["capabilities"] = clientCaps;

    sendRequest("initialize", params, [this](const QJsonValue &result, const QJsonObject &error, bool isError) {
        if (isError) {
            QString msg = error.value("message").toString("Initialize failed");
            updateStatus(LspServerState::Failed, msg);
            emit serverError(msg);
            return;
        }

        QJsonObject serverCaps = result.toObject().value("capabilities").toObject();
        m_capabilities = LspServerCapabilities::fromJson(serverCaps);

        sendNotification("initialized", QJsonObject());
        updateStatus(LspServerState::Running);
    });
}

void LspClient::onProcessReadyReadStandardOutput() {
    if (!m_process) return;
    m_readBuffer.append(m_process->readAllStandardOutput());
    processIncomingBuffer();
}

void LspClient::onProcessReadyReadStandardError() {
    if (!m_process) return;
    QByteArray errData = m_process->readAllStandardError();
    QString errStr = QString::fromUtf8(errData).trimmed();
    if (!errStr.isEmpty()) {
        emit logMessage(QString("[%1 stderr] %2").arg(m_config.name, errStr));
    }
}

void LspClient::processIncomingBuffer() {
    while (true) {
        int headerEnd = m_readBuffer.indexOf("\r\n\r\n");
        if (headerEnd == -1) {
            break; // Need more data for header
        }

        QByteArray headerData = m_readBuffer.left(headerEnd);
        int contentLength = -1;

        QList<QByteArray> lines = headerData.split('\n');
        for (const QByteArray &line : lines) {
            QByteArray trimmed = line.trimmed();
            if (trimmed.startsWith("Content-Length:") || trimmed.startsWith("content-length:")) {
                int colonIdx = trimmed.indexOf(':');
                if (colonIdx != -1) {
                    contentLength = trimmed.mid(colonIdx + 1).trimmed().toInt();
                }
            }
        }

        if (contentLength < 0) {
            // Invalid header framing, drop buffer up to \r\n\r\n
            m_readBuffer.remove(0, headerEnd + 4);
            continue;
        }

        int totalMsgLength = headerEnd + 4 + contentLength;
        if (m_readBuffer.size() < totalMsgLength) {
            break; // Need more data for payload
        }

        QByteArray body = m_readBuffer.mid(headerEnd + 4, contentLength);
        m_readBuffer.remove(0, totalMsgLength);

        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(body, &parseErr);
        if (parseErr.error == QJsonParseError::NoError && doc.isObject()) {
            handleMessage(doc.object());
        }
    }
}

void LspClient::handleMessage(const QJsonObject &obj) {
    if (obj.contains("id") && (obj.contains("result") || obj.contains("error"))) {
        handleResponse(obj);
    } else if (obj.contains("method")) {
        if (obj.contains("id")) {
            handleRequest(obj);
        } else {
            handleNotification(obj);
        }
    }
}

void LspClient::handleResponse(const QJsonObject &resp) {
    int id = resp.value("id").toInt(-1);
    if (!m_pendingRequests.contains(id)) {
        return;
    }

    PendingRequest pending = m_pendingRequests.take(id);
    bool isError = resp.contains("error");
    QJsonValue result = resp.value("result");
    QJsonObject error = resp.value("error").toObject();

    if (pending.callback) {
        pending.callback(result, error, isError);
    }
}

void LspClient::handleNotification(const QJsonObject &notif) {
    QString method = notif.value("method").toString();
    QJsonObject params = notif.value("params").toObject();

    if (method == "textDocument/publishDiagnostics") {
        QString uri = params.value("uri").toString();
        QString filePath = uriToPath(uri);

        QList<LspDiagnostic> diagnostics;
        QJsonArray diagArray = params.value("diagnostics").toArray();
        for (const QJsonValue &v : diagArray) {
            diagnostics.append(LspDiagnostic::fromJson(v.toObject()));
        }

        emit diagnosticsReceived(filePath, diagnostics);
    } else if (method == "window/logMessage") {
        QString msg = params.value("message").toString();
        emit logMessage(QString("[%1] %2").arg(m_config.name, msg));
    } else if (method == "window/showMessage") {
        QString msg = params.value("message").toString();
        emit logMessage(QString("[%1 info] %2").arg(m_config.name, msg));
    }
}

void LspClient::handleRequest(const QJsonObject &req) {
    // Respond with empty/null for server-initiated requests
    int id = req.value("id").toInt(0);
    QJsonObject resp;
    resp["jsonrpc"] = "2.0";
    resp["id"] = id;
    resp["result"] = QJsonValue::Null;
    sendJsonRpc(resp);
}

void LspClient::onProcessErrorOccurred(QProcess::ProcessError error) {
    if (m_status.state == LspServerState::Stopping || m_status.state == LspServerState::Stopped) {
        return;
    }

    QString errStr = m_process ? m_process->errorString() : "Process error";
    if (error == QProcess::FailedToStart) {
        updateStatus(LspServerState::Failed, QString("Executable failed to start: %1").arg(errStr));
    } else if (error == QProcess::Crashed) {
        updateStatus(LspServerState::Failed, "Language server process crashed");
    } else {
        updateStatus(LspServerState::Failed, errStr);
    }
    emit serverError(m_status.lastError);
}

void LspClient::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (m_status.state == LspServerState::Stopping || m_status.state == LspServerState::Stopped) {
        updateStatus(LspServerState::Stopped);
        return;
    }

    if (exitStatus == QProcess::CrashExit) {
        updateStatus(LspServerState::Failed, QString("Server crashed with exit code %1").arg(exitCode));
    } else {
        updateStatus(LspServerState::Stopped, QString("Server stopped with exit code %1").arg(exitCode));
    }
}

void LspClient::notifyDidOpen(const QString &filePath, const QString &languageId, int version, const QString &text) {
    if (!isRunning()) return;

    QJsonObject params;
    QJsonObject docItem;
    docItem["uri"] = pathToUri(filePath);
    docItem["languageId"] = languageId;
    docItem["version"] = version;
    docItem["text"] = text;
    params["textDocument"] = docItem;

    sendNotification("textDocument/didOpen", params);
}

void LspClient::notifyDidChange(const QString &filePath, int version, const QString &fullText) {
    if (!isRunning()) return;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = pathToUri(filePath);
    doc["version"] = version;
    params["textDocument"] = doc;

    QJsonArray changes;
    QJsonObject change;
    change["text"] = fullText;
    changes.append(change);
    params["contentChanges"] = changes;

    sendNotification("textDocument/didChange", params);
}

void LspClient::notifyDidSave(const QString &filePath, const QString &text) {
    if (!isRunning()) return;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = pathToUri(filePath);
    params["textDocument"] = doc;

    if (!text.isNull()) {
        params["text"] = text;
    }

    sendNotification("textDocument/didSave", params);
}

void LspClient::notifyDidClose(const QString &filePath) {
    if (!isRunning()) return;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = pathToUri(filePath);
    params["textDocument"] = doc;

    sendNotification("textDocument/didClose", params);
}

int LspClient::requestCompletion(const QString &filePath, int line, int character, ResponseCallback callback) {
    if (!isRunning() || !m_capabilities.completionProvider) {
        if (callback) callback(QJsonObject(), QJsonObject(), true);
        return -1;
    }

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = pathToUri(filePath);
    params["textDocument"] = doc;
    params["position"] = LspPosition{line, character}.toJson();

    return sendRequest("textDocument/completion", params, callback);
}

int LspClient::requestHover(const QString &filePath, int line, int character, ResponseCallback callback) {
    if (!isRunning() || !m_capabilities.hoverProvider) {
        if (callback) callback(QJsonObject(), QJsonObject(), true);
        return -1;
    }

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = pathToUri(filePath);
    params["textDocument"] = doc;
    params["position"] = LspPosition{line, character}.toJson();

    return sendRequest("textDocument/hover", params, callback);
}

int LspClient::requestDefinition(const QString &filePath, int line, int character, ResponseCallback callback) {
    if (!isRunning() || !m_capabilities.definitionProvider) {
        if (callback) callback(QJsonObject(), QJsonObject(), true);
        return -1;
    }

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = pathToUri(filePath);
    params["textDocument"] = doc;
    params["position"] = LspPosition{line, character}.toJson();

    return sendRequest("textDocument/definition", params, callback);
}

int LspClient::requestReferences(const QString &filePath, int line, int character, ResponseCallback callback) {
    if (!isRunning() || !m_capabilities.referencesProvider) {
        if (callback) callback(QJsonObject(), QJsonObject(), true);
        return -1;
    }

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = pathToUri(filePath);
    params["textDocument"] = doc;
    params["position"] = LspPosition{line, character}.toJson();

    QJsonObject context;
    context["includeDeclaration"] = true;
    params["context"] = context;

    return sendRequest("textDocument/references", params, callback);
}

int LspClient::requestRename(const QString &filePath, int line, int character, const QString &newName, ResponseCallback callback) {
    if (!isRunning() || !m_capabilities.renameProvider) {
        if (callback) callback(QJsonObject(), QJsonObject(), true);
        return -1;
    }

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = pathToUri(filePath);
    params["textDocument"] = doc;
    params["position"] = LspPosition{line, character}.toJson();
    params["newName"] = newName;

    return sendRequest("textDocument/rename", params, callback);
}

qint64 LspClient::estimateRamUsageBytes() const {
    if (!m_process || m_process->state() != QProcess::Running) {
        return -1;
    }

    qint64 pid = m_process->processId();
    if (pid <= 0) return -1;

    // Estimate on Linux via /proc/<pid>/statm
    QString statmPath = QString("/proc/%1/statm").arg(pid);
    QFile file(statmPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        qint64 pages = 0;
        in >> pages; // First number in statm is total program size in pages
        file.close();
        if (pages > 0) {
            long pageSize = sysconf(_SC_PAGESIZE);
            if (pageSize <= 0) pageSize = 4096;
            return pages * pageSize;
        }
    }

    return -1;
}

} // namespace CinderDE
