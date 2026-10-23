#include "cinderde/lsp_manager.h"
#include "cinderde/preferences.h"
#include "cinderde/file_type_detector.h"
#include "cinderde/language_pack.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

namespace CinderDE {

LspManager& LspManager::instance() {
    static LspManager inst;
    return inst;
}

LspManager::LspManager()
    : QObject(nullptr)
{
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(300); // 300ms debounce interval
    connect(m_debounceTimer, &QTimer::timeout, this, &LspManager::onDebouncingTimerTimeout);
}

LspManager::~LspManager() {
    stopAllServers();
}

void LspManager::setWorkspacePath(const QString &workspacePath) {
    if (m_workspacePath == workspacePath) {
        return;
    }

    // Stop all active servers when workspace changes
    stopAllServers();

    m_workspacePath = workspacePath;
    m_openDocs.clear();
    m_pendingDocChanges.clear();
}

LspServerConfig LspManager::findConfigForFile(const QString &filePath) const {
    if (filePath.isEmpty()) return LspServerConfig();

    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    QString langId = languageIdForFile(filePath);

    QList<LspServerConfig> configs = Preferences::instance().lspServerConfigs();
    for (const auto &cfg : configs) {
        if (!cfg.enabled || !cfg.isValid()) continue;

        // Check languageId match
        if (!cfg.languageId.isEmpty() && cfg.languageId.compare(langId, Qt::CaseInsensitive) == 0) {
            return cfg;
        }

        // Check extension match
        for (const QString &e : cfg.extensions) {
            QString cleanExt = e.startsWith('.') ? e.mid(1) : e;
            if (cleanExt.compare(ext, Qt::CaseInsensitive) == 0) {
                return cfg;
            }
        }
    }

    return LspServerConfig();
}

QString LspManager::languageIdForFile(const QString &filePath) const {
    if (filePath.isEmpty()) return "plaintext";

    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();

    LanguageDefinition langDef = LanguageRegistry::instance().definitionForExtension(ext);
    if (langDef.isValid() && !langDef.id.isEmpty()) {
        return langDef.id;
    }

    static const QMap<QString, QString> extToLspLang = {
        {"cpp", "cpp"}, {"cc", "cpp"}, {"cxx", "cpp"}, {"h", "cpp"}, {"hpp", "cpp"}, {"hxx", "cpp"},
        {"c", "c"},
        {"py", "python"},
        {"js", "javascript"}, {"ts", "typescript"},
        {"java", "java"},
        {"kt", "kotlin"}, {"kts", "kotlin"},
        {"sh", "shellscript"}, {"bash", "shellscript"},
        {"json", "json"},
        {"xml", "xml"},
        {"html", "html"}, {"htm", "html"},
        {"css", "css"},
        {"md", "markdown"}, {"markdown", "markdown"},
        {"cmake", "cmake"},
        {"yaml", "yaml"}, {"yml", "yaml"}
    };

    if (extToLspLang.contains(ext)) {
        return extToLspLang.value(ext);
    }

    return ext.isEmpty() ? "plaintext" : ext;
}

LspClient* LspManager::getClientForConfig(const QString &configId) {
    return m_clients.value(configId, nullptr);
}

LspClient* LspManager::getClientForDocument(const QString &filePath, bool createIfMissing) {
    if (filePath.isEmpty()) return nullptr;

    LspServerConfig cfg = findConfigForFile(filePath);
    if (!cfg.isValid()) return nullptr;

    if (m_clients.contains(cfg.id)) {
        LspClient *client = m_clients.value(cfg.id);
        if (client->isRunning()) {
            return client;
        }
        if (!createIfMissing) {
            return client;
        }
    }

    if (!createIfMissing) {
        return nullptr;
    }

    return createAndStartClient(cfg);
}

LspClient* LspManager::createAndStartClient(const LspServerConfig &config) {
    if (m_clients.contains(config.id)) {
        LspClient *existing = m_clients.take(config.id);
        existing->stop();
        existing->deleteLater();
    }

    LspClient *client = new LspClient(config, m_workspacePath, this);

    connect(client, &LspClient::statusChanged, this, [this, config, client](const LspServerStatus &status) {
        emit serverStatusChanged(config.id, status);

        // A server can crash after documents were opened. Clear the per-server
        // sent flags only if this is still the manager's current client; an old
        // client's delayed shutdown must not invalidate documents already sent
        // to a replacement client.
        if ((status.state == LspServerState::Failed ||
             status.state == LspServerState::Stopped) &&
            m_clients.value(config.id, nullptr) == client) {
            for (auto it = m_openDocs.begin(); it != m_openDocs.end(); ++it) {
                OpenDocumentInfo &info = it.value();
                if (info.isOpen && findConfigForFile(info.filePath).id == config.id) {
                    info.didOpenSent = false;
                }
            }
        }

        // Documents may be opened before initialize completes. Flush their latest
        // contents once the server is ready, and never send didOpen twice.
        if (status.state == LspServerState::Running) {
            for (auto it = m_openDocs.begin(); it != m_openDocs.end(); ++it) {
                OpenDocumentInfo &info = it.value();
                if (!info.isOpen || info.didOpenSent ||
                    findConfigForFile(info.filePath).id != config.id) {
                    continue;
                }
                client->notifyDidOpen(info.filePath, info.languageId, info.version, info.pendingText);
                info.didOpenSent = true;
            }
        }
    });

    connect(client, &LspClient::diagnosticsReceived, this, [this](const QString &filePath, const QList<LspDiagnostic> &diagnostics) {
        emit diagnosticsUpdated(filePath, diagnostics);
    });

    connect(client, &LspClient::logMessage, this, [this](const QString &msg) {
        emit logMessage(msg);
    });

    m_clients[config.id] = client;
    client->start();

    return client;
}

QList<LspClient*> LspManager::activeClients() const {
    return m_clients.values();
}

void LspManager::notifyDocumentOpened(const QString &filePath, const QString &text) {
    if (filePath.isEmpty()) return;

    const QString langId = languageIdForFile(filePath);
    if (m_openDocs.contains(filePath)) {
        OpenDocumentInfo &existing = m_openDocs[filePath];
        existing.isOpen = true;
        existing.languageId = langId;
        if (existing.pendingText != text) {
            existing.pendingText = text;
            if (existing.didOpenSent) {
                notifyDocumentChanged(filePath, text);
            }
        }
        return;
    }
    OpenDocumentInfo info;
    info.filePath = filePath;
    info.languageId = langId;
    info.version = 1;
    info.isOpen = true;
    info.pendingText = text;
    info.didOpenSent = false;
    m_openDocs[filePath] = info;

    LspClient *client = getClientForDocument(filePath, true);
    if (client && client->isRunning()) {
        OpenDocumentInfo &storedInfo = m_openDocs[filePath];
        if (!storedInfo.didOpenSent) {
            client->notifyDidOpen(filePath, langId, storedInfo.version, storedInfo.pendingText);
            storedInfo.didOpenSent = true;
        }
    }
}

void LspManager::notifyDocumentChanged(const QString &filePath, const QString &text) {
    if (filePath.isEmpty() || !m_openDocs.contains(filePath)) return;

    m_pendingDocChanges[filePath] = text;
    m_debounceTimer->start();
}

void LspManager::onDebouncingTimerTimeout() {
    for (auto it = m_pendingDocChanges.begin(); it != m_pendingDocChanges.end(); ++it) {
        const QString &filePath = it.key();
        const QString &text = it.value();

        if (m_openDocs.contains(filePath)) {
            OpenDocumentInfo &info = m_openDocs[filePath];
            if (!info.didOpenSent) {
                info.pendingText = text;
                continue;
            }
            info.version++;

            LspClient *client = getClientForDocument(filePath, false);
            if (client && client->isRunning()) {
                client->notifyDidChange(filePath, info.version, text);
            }
        }
    }
    m_pendingDocChanges.clear();
}

void LspManager::notifyDocumentSaved(const QString &filePath, const QString &text) {
    if (filePath.isEmpty()) return;

    // Flush any pending change for this doc immediately
    if (m_pendingDocChanges.contains(filePath)) {
        QString pendingText = m_pendingDocChanges.take(filePath);
        if (m_openDocs.contains(filePath)) {
            OpenDocumentInfo &info = m_openDocs[filePath];
            if (!info.didOpenSent) {
                info.pendingText = pendingText;
            } else {
                info.version++;
                LspClient *client = getClientForDocument(filePath, false);
                if (client && client->isRunning()) {
                    client->notifyDidChange(filePath, info.version, pendingText);
                }
            }
        }
    }

    LspClient *client = getClientForDocument(filePath, false);
    if (client && client->isRunning()) {
        client->notifyDidSave(filePath, text);
    }
}

void LspManager::notifyDocumentClosed(const QString &filePath) {
    if (filePath.isEmpty()) return;

    m_pendingDocChanges.remove(filePath);

    if (m_openDocs.contains(filePath)) {
        m_openDocs.remove(filePath);

        LspClient *client = getClientForDocument(filePath, false);
        if (client && client->isRunning()) {
            client->notifyDidClose(filePath);
        }
    }
}

void LspManager::stopServer(const QString &configId) {
    if (!m_clients.contains(configId)) {
        return;
    }

    for (auto it = m_openDocs.begin(); it != m_openDocs.end(); ++it) {
        if (findConfigForFile(it.value().filePath).id == configId) {
            it.value().didOpenSent = false;
        }
    }

    LspClient *client = m_clients.take(configId);
    // Keep the client alive until its asynchronous shutdown has actually finished.
    connect(client, &LspClient::statusChanged, client,
            [client](const LspServerStatus &status) {
                if (status.state == LspServerState::Stopped ||
                    status.state == LspServerState::Failed) {
                    client->deleteLater();
                }
            });
    client->stop();
}

void LspManager::restartServer(const QString &configId) {
    stopServer(configId);

    QList<LspServerConfig> configs = Preferences::instance().lspServerConfigs();
    for (const auto &cfg : configs) {
        if (cfg.id == configId && cfg.enabled) {
            createAndStartClient(cfg);
            break;
        }
    }
}

void LspManager::stopAllServers() {
    m_debounceTimer->stop();
    m_pendingDocChanges.clear();

    const QList<LspClient*> clients = m_clients.values();
    m_clients.clear();
    for (LspClient *client : clients) {
        if (!client) {
            continue;
        }
        // Keep each client alive to complete its asynchronous shutdown.
        connect(client, &LspClient::statusChanged, client,
                [client](const LspServerStatus &status) {
                    if (status.state == LspServerState::Stopped ||
                        status.state == LspServerState::Failed) {
                        client->deleteLater();
                    }
                });
        client->stop();
    }
}

} // namespace CinderDE
