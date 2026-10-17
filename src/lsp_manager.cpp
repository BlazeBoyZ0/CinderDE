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

    connect(client, &LspClient::statusChanged, this, [this, config](const LspServerStatus &status) {
        emit serverStatusChanged(config.id, status);
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

    QString langId = languageIdForFile(filePath);
    OpenDocumentInfo info;
    info.filePath = filePath;
    info.languageId = langId;
    info.version = 1;
    info.isOpen = true;
    m_openDocs[filePath] = info;

    LspClient *client = getClientForDocument(filePath, true);
    if (client && client->isRunning()) {
        client->notifyDidOpen(filePath, langId, info.version, text);
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
            info.version++;
            LspClient *client = getClientForDocument(filePath, false);
            if (client && client->isRunning()) {
                client->notifyDidChange(filePath, info.version, pendingText);
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
    if (m_clients.contains(configId)) {
        LspClient *client = m_clients.take(configId);
        client->stop();
        client->deleteLater();
    }
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

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        LspClient *client = it.value();
        if (client) {
            client->stop();
            client->deleteLater();
        }
    }
    m_clients.clear();
}

} // namespace CinderDE
