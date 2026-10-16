#include "cinderde/preferences.h"

namespace CinderDE {

Preferences::Preferences()
    : m_settings("CinderDE", "CinderDE")
{
}

Preferences& Preferences::instance() {
    static Preferences inst;
    return inst;
}

int Preferences::fontSize() const {
    return m_settings.value("editor/fontSize", DEFAULT_FONT_SIZE).toInt();
}

void Preferences::setFontSize(int size) {
    if (size < 6) size = 6;
    if (size > 72) size = 72;
    m_settings.setValue("editor/fontSize", size);
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

} // namespace CinderDE
