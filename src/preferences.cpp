#include "cinderde/preferences.h"
#include "cinderde/lsp_types.h"
#include <QDir>
#include <QVariant>
#include <QVariantMap>

namespace CinderDE {

Preferences::Preferences()
    : m_settings("CinderDE", "CinderDE")
{
}

Preferences& Preferences::instance() {
    static Preferences inst;
    return inst;
}

static QString canonicalizePath(const QString &p) {
    QDir d(p);
    if (d.exists()) {
        QString canon = d.canonicalPath();
        if (!canon.isEmpty()) return canon;
    }
    return QDir::cleanPath(p);
}

int Preferences::fontSize() const {
    return m_settings.value("editor/fontSize", DEFAULT_FONT_SIZE).toInt();
}

void Preferences::setFontSize(int size) {
    if (size < 6) size = 6;
    if (size > 72) size = 72;
    m_settings.setValue("editor/fontSize", size);
}

int Preferences::tabSize() const {
    return m_settings.value("editor/tabSize", DEFAULT_TAB_SIZE).toInt();
}

void Preferences::setTabSize(int size) {
    if (size < 1) size = 1;
    if (size > 16) size = 16;
    m_settings.setValue("editor/tabSize", size);
}

bool Preferences::wordWrap() const {
    return m_settings.value("editor/wordWrap", false).toBool();
}

void Preferences::setWordWrap(bool enable) {
    m_settings.setValue("editor/wordWrap", enable);
}

bool Preferences::explorerVisible() const {
    return m_settings.value("ui/explorerVisible", true).toBool();
}

void Preferences::setExplorerVisible(bool visible) {
    m_settings.setValue("ui/explorerVisible", visible);
}

QString Preferences::lastWorkspace() const {
    return m_settings.value("workspace/lastPath", "").toString();
}

void Preferences::setLastWorkspace(const QString &path) {
    m_settings.setValue("workspace/lastPath", path);
}

QStringList Preferences::recentWorkspaces() const {
    return m_settings.value("workspace/recentList").toStringList();
}

void Preferences::addRecentWorkspace(const QString &path) {
    if (path.isEmpty()) return;
    QDir dir(path);
    if (!dir.exists()) return;

    QString target = canonicalizePath(path);
    QStringList current = recentWorkspaces();
    QStringList updated;

    for (const QString &item : current) {
        if (canonicalizePath(item) != target) {
            updated.append(item);
        }
    }

    updated.prepend(target);
    while (updated.size() > MAX_RECENT_WORKSPACES) {
        updated.removeLast();
    }

    m_settings.setValue("workspace/recentList", updated);
}

void Preferences::removeRecentWorkspace(const QString &path) {
    if (path.isEmpty()) return;
    QString target = canonicalizePath(path);
    QStringList current = recentWorkspaces();
    QStringList updated;

    for (const QString &item : current) {
        if (canonicalizePath(item) != target) {
            updated.append(item);
        }
    }

    m_settings.setValue("workspace/recentList", updated);
}

void Preferences::clearRecentWorkspaces() {
    m_settings.remove("workspace/recentList");
}

bool Preferences::sessionRestoreEnabled() const {
    return m_settings.value("session/restoreEnabled", true).toBool();
}

void Preferences::setSessionRestoreEnabled(bool enable) {
    m_settings.setValue("session/restoreEnabled", enable);
}

QString Preferences::sessionWorkspace() const {
    return m_settings.value("session/workspace", "").toString();
}

void Preferences::setSessionWorkspace(const QString &path) {
    m_settings.setValue("session/workspace", path);
}

QStringList Preferences::sessionOpenTabs() const {
    return m_settings.value("session/openTabs").toStringList();
}

void Preferences::setSessionOpenTabs(const QStringList &tabs) {
    m_settings.setValue("session/openTabs", tabs);
}

int Preferences::sessionActiveTab() const {
    return m_settings.value("session/activeTab", 0).toInt();
}

void Preferences::setSessionActiveTab(int index) {
    m_settings.setValue("session/activeTab", index);
}

QList<QPair<int, int>> Preferences::sessionCursorPositions() const {
    QList<QPair<int, int>> result;
    QVariantList list = m_settings.value("session/cursorPositions").toList();
    for (const QVariant &var : list) {
        QVariantList pair = var.toList();
        if (pair.size() >= 2) {
            result.append({pair[0].toInt(), pair[1].toInt()});
        }
    }
    return result;
}

void Preferences::setSessionCursorPositions(const QList<QPair<int, int>> &positions) {
    QVariantList list;
    for (const auto &pos : positions) {
        QVariantList pair;
        pair << pos.first << pos.second;
        list.append(QVariant(pair));
    }
    m_settings.setValue("session/cursorPositions", list);
}

QByteArray Preferences::windowGeometry() const {
    return m_settings.value("ui/geometry").toByteArray();
}

void Preferences::setWindowGeometry(const QByteArray &geometry) {
    m_settings.setValue("ui/geometry", geometry);
}

QByteArray Preferences::windowState() const {
    return m_settings.value("ui/windowState").toByteArray();
}

void Preferences::setWindowState(const QByteArray &state) {
    m_settings.setValue("ui/windowState", state);
}

QString Preferences::executionMode() const {
    return m_settings.value("buildrun/executionMode", "output_panel").toString();
}

void Preferences::setExecutionMode(const QString &mode) {
    m_settings.setValue("buildrun/executionMode", mode);
}

QString Preferences::terminalCommand() const {
    return m_settings.value("buildrun/terminalCommand", "").toString();
}

void Preferences::setTerminalCommand(const QString &cmd) {
    m_settings.setValue("buildrun/terminalCommand", cmd);
}

QList<LspServerConfig> Preferences::lspServerConfigs() const {
    QList<LspServerConfig> result;
    int size = m_settings.beginReadArray("lsp/servers");
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        LspServerConfig cfg;
        cfg.id = m_settings.value("id").toString();
        cfg.name = m_settings.value("name").toString();
        cfg.languageId = m_settings.value("languageId").toString();
        cfg.extensions = m_settings.value("extensions").toStringList();
        cfg.executable = m_settings.value("executable").toString();
        cfg.arguments = m_settings.value("arguments").toStringList();
        cfg.enabled = m_settings.value("enabled", true).toBool();

        QVariantMap envMap = m_settings.value("environment").toMap();
        for (auto it = envMap.begin(); it != envMap.end(); ++it) {
            cfg.environment[it.key()] = it.value().toString();
        }

        if (!cfg.id.isEmpty()) {
            result.append(cfg);
        }
    }
    m_settings.endArray();
    return result;
}

void Preferences::setLspServerConfigs(const QList<LspServerConfig> &configs) {
    m_settings.beginWriteArray("lsp/servers");
    for (int i = 0; i < configs.size(); ++i) {
        m_settings.setArrayIndex(i);
        const auto &cfg = configs[i];
        m_settings.setValue("id", cfg.id);
        m_settings.setValue("name", cfg.name);
        m_settings.setValue("languageId", cfg.languageId);
        m_settings.setValue("extensions", cfg.extensions);
        m_settings.setValue("executable", cfg.executable);
        m_settings.setValue("arguments", cfg.arguments);
        m_settings.setValue("enabled", cfg.enabled);

        QVariantMap envMap;
        for (auto it = cfg.environment.begin(); it != cfg.environment.end(); ++it) {
            envMap[it.key()] = it.value();
        }
        m_settings.setValue("environment", envMap);
    }
    m_settings.endArray();
}

void Preferences::saveLspServerConfig(const LspServerConfig &config) {
    QList<LspServerConfig> list = lspServerConfigs();
    bool found = false;
    for (int i = 0; i < list.size(); ++i) {
        if (list[i].id == config.id) {
            list[i] = config;
            found = true;
            break;
        }
    }
    if (!found) {
        list.append(config);
    }
    setLspServerConfigs(list);
}

void Preferences::removeLspServerConfig(const QString &id) {
    QList<LspServerConfig> list = lspServerConfigs();
    QList<LspServerConfig> updated;
    for (const auto &cfg : list) {
        if (cfg.id != id) {
            updated.append(cfg);
        }
    }
    setLspServerConfigs(updated);
}

} // namespace CinderDE
