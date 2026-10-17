#ifndef CINDERDE_LSP_CLIENT_H
#define CINDERDE_LSP_CLIENT_H

#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <QMap>
#include <QList>
#include <QTimer>
#include <functional>
#include "cinderde/lsp_types.h"

namespace CinderDE {

class LspClient : public QObject {
    Q_OBJECT

public:
    using ResponseCallback = std::function<void(const QJsonValue &result, const QJsonObject &error, bool isError)>;

    explicit LspClient(const LspServerConfig &config, const QString &workspacePath, QObject *parent = nullptr);
    ~LspClient() override;

    LspServerConfig config() const { return m_config; }
    QString workspacePath() const { return m_workspacePath; }
    LspServerStatus status() const { return m_status; }
    LspServerCapabilities capabilities() const { return m_capabilities; }

    bool isRunning() const { return m_status.state == LspServerState::Running; }

    void start();
    void stop();

    // Raw JSON-RPC methods
    int sendRequest(const QString &method, const QJsonObject &params, ResponseCallback callback = nullptr);
    void sendNotification(const QString &method, const QJsonObject &params);

    // High level LSP operations
    void notifyDidOpen(const QString &filePath, const QString &languageId, int version, const QString &text);
    void notifyDidChange(const QString &filePath, int version, const QString &fullText);
    void notifyDidSave(const QString &filePath, const QString &text = QString());
    void notifyDidClose(const QString &filePath);

    int requestCompletion(const QString &filePath, int line, int character, ResponseCallback callback, const QString &triggerCharacter = QString());
    int requestHover(const QString &filePath, int line, int character, ResponseCallback callback);
    int requestDefinition(const QString &filePath, int line, int character, ResponseCallback callback);
    int requestReferences(const QString &filePath, int line, int character, ResponseCallback callback);
    int requestRename(const QString &filePath, int line, int character, const QString &newName, ResponseCallback callback);

    qint64 estimateRamUsageBytes() const;

signals:
    void statusChanged(const LspServerStatus &status);
    void diagnosticsReceived(const QString &filePath, const QList<LspDiagnostic> &diagnostics);
    void serverError(const QString &errorMessage);
    void logMessage(const QString &message);

private slots:
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onProcessErrorOccurred(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    LspServerConfig m_config;
    QString m_workspacePath;
    LspServerStatus m_status;
    LspServerCapabilities m_capabilities;

    QProcess *m_process;
    QByteArray m_readBuffer;
    int m_nextRequestId;

    struct PendingRequest {
        ResponseCallback callback;
        QString method;
    };
    QMap<int, PendingRequest> m_pendingRequests;

    void updateStatus(LspServerState state, const QString &errorMsg = QString());
    void sendJsonRpc(const QJsonObject &obj);
    void processIncomingBuffer();
    void handleMessage(const QJsonObject &doc);
    void handleResponse(const QJsonObject &resp);
    void handleNotification(const QJsonObject &notif);
    void handleRequest(const QJsonObject &req);

    void sendInitializeRequest();
};

} // namespace CinderDE

#endif // CINDERDE_LSP_CLIENT_H
