#ifndef CINDERDE_LSP_MANAGER_H
#define CINDERDE_LSP_MANAGER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QTimer>
#include <QList>
#include "cinderde/lsp_types.h"
#include "cinderde/lsp_client.h"

namespace CinderDE {

class LspManager : public QObject {
    Q_OBJECT

public:
    static LspManager& instance();

    void setWorkspacePath(const QString &workspacePath);
    QString workspacePath() const { return m_workspacePath; }

    LspClient* getClientForDocument(const QString &filePath, bool createIfMissing = true);
    LspClient* getClientForConfig(const QString &configId);

    QList<LspClient*> activeClients() const;

    void notifyDocumentOpened(const QString &filePath, const QString &text);
    void notifyDocumentChanged(const QString &filePath, const QString &text);
    void notifyDocumentSaved(const QString &filePath, const QString &text = QString());
    void notifyDocumentClosed(const QString &filePath);

    void stopServer(const QString &configId);
    void restartServer(const QString &configId);
    void stopAllServers();

    LspServerConfig findConfigForFile(const QString &filePath) const;
    QString languageIdForFile(const QString &filePath) const;

signals:
    void diagnosticsUpdated(const QString &filePath, const QList<LspDiagnostic> &diagnostics);
    void serverStatusChanged(const QString &configId, const LspServerStatus &status);
    void logMessage(const QString &message);

private slots:
    void onDebouncingTimerTimeout();

private:
    LspManager();
    ~LspManager() override;

    QString m_workspacePath;
    // Key: config.id -> LspClient*
    QMap<QString, LspClient*> m_clients;

    // Track open documents and document versions
    struct OpenDocumentInfo {
        QString filePath;
        QString languageId;
        int version = 1;
        bool isOpen = false;
        QString pendingText;
    };
    QMap<QString, OpenDocumentInfo> m_openDocs; // filePath -> info

    // Debounce timer for changes
    QTimer *m_debounceTimer;
    QMap<QString, QString> m_pendingDocChanges; // filePath -> newText

    LspClient* createAndStartClient(const LspServerConfig &config);
};

} // namespace CinderDE

#endif // CINDERDE_LSP_MANAGER_H
