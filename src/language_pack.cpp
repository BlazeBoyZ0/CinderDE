#include "cinderde/language_pack.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>

namespace CinderDE {

LanguageDefinition LanguageDefinition::fromJson(const QJsonObject &json, bool *ok) {
    LanguageDefinition def;

    if (ok) *ok = false;

    def.id = json.value("id").toString().trimmed().toLower();
    def.name = json.value("name").toString().trimmed();

    if (!def.isValid()) {
        return def;
    }

    // Extensions
    if (json.contains("extensions")) {
        QJsonValue extVal = json.value("extensions");
        if (extVal.isArray()) {
            QJsonArray extArr = extVal.toArray();
            for (const QJsonValue &v : extArr) {
                QString ext = v.toString().trimmed().toLower();
                if (ext.startsWith('.')) ext = ext.mid(1);
                if (!ext.isEmpty() && !def.extensions.contains(ext)) {
                    def.extensions.append(ext);
                }
            }
        } else if (extVal.isString()) {
            QString ext = extVal.toString().trimmed().toLower();
            if (ext.startsWith('.')) ext = ext.mid(1);
            if (!ext.isEmpty()) {
                def.extensions.append(ext);
            }
        }
    }

    // Keywords, Types, Builtins
    auto parseStringList = [](const QJsonObject &obj, const QString &key) -> QStringList {
        QStringList list;
        QJsonValue val = obj.value(key);
        if (val.isArray()) {
            QJsonArray arr = val.toArray();
            for (const QJsonValue &v : arr) {
                QString str = v.toString().trimmed();
                if (!str.isEmpty() && !list.contains(str)) {
                    list.append(str);
                }
            }
        }
        return list;
    };

    def.keywords = parseStringList(json, "keywords");
    def.types = parseStringList(json, "types");
    def.builtins = parseStringList(json, "builtins");

    // Comments
    if (json.contains("comments") && json.value("comments").isObject()) {
        QJsonObject commentsObj = json.value("comments").toObject();
        def.comments.linePrefix = commentsObj.value("line").toString();
        if (def.comments.linePrefix.isEmpty()) {
            def.comments.linePrefix = commentsObj.value("line_prefix").toString();
        }
        def.comments.blockStart = commentsObj.value("block_start").toString();
        def.comments.blockEnd = commentsObj.value("block_end").toString();
    }

    // Indentation
    if (json.contains("indentation") && json.value("indentation").isObject()) {
        QJsonObject indentObj = json.value("indentation").toObject();
        if (indentObj.contains("tab_size")) {
            def.indentation.tabSize = indentObj.value("tab_size").toInt(4);
        } else if (indentObj.contains("indent_width")) {
            def.indentation.tabSize = indentObj.value("indent_width").toInt(4);
        }
        if (indentObj.contains("use_spaces")) {
            def.indentation.useSpaces = indentObj.value("use_spaces").toBool(true);
        }
        if (indentObj.contains("auto_indent_on_brace")) {
            def.indentation.autoIndentOnBrace = indentObj.value("auto_indent_on_brace").toBool(true);
        }
    }

    // Build/Run Profile
    if (json.contains("build_run") && json.value("build_run").isObject()) {
        QJsonObject brObj = json.value("build_run").toObject();
        def.buildRun.buildCommand = brObj.value("build_command").toString();
        def.buildRun.runCommand = brObj.value("run_command").toString();
        def.buildRun.workingDirMode = brObj.value("working_dir_mode").toString("workspace");
    }

    if (ok) *ok = true;
    return def;
}

QJsonObject LanguageDefinition::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;

    QJsonArray extArr;
    for (const QString &ext : extensions) {
        extArr.append(ext);
    }
    json["extensions"] = extArr;

    auto listToArray = [](const QStringList &list) -> QJsonArray {
        QJsonArray arr;
        for (const QString &str : list) arr.append(str);
        return arr;
    };

    json["keywords"] = listToArray(keywords);
    json["types"] = listToArray(types);
    json["builtins"] = listToArray(builtins);

    QJsonObject commentsObj;
    commentsObj["line"] = comments.linePrefix;
    commentsObj["block_start"] = comments.blockStart;
    commentsObj["block_end"] = comments.blockEnd;
    json["comments"] = commentsObj;

    QJsonObject indentObj;
    indentObj["tab_size"] = indentation.tabSize;
    indentObj["use_spaces"] = indentation.useSpaces;
    indentObj["auto_indent_on_brace"] = indentation.autoIndentOnBrace;
    json["indentation"] = indentObj;

    QJsonObject brObj;
    brObj["build_command"] = buildRun.buildCommand;
    brObj["run_command"] = buildRun.runCommand;
    brObj["working_dir_mode"] = buildRun.workingDirMode;
    json["build_run"] = brObj;

    return json;
}

LanguageRegistry& LanguageRegistry::instance() {
    static LanguageRegistry registry;
    return registry;
}

LanguageRegistry::LanguageRegistry() {
    discoverPacks();
}

QString LanguageRegistry::userLanguagesDirectory() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QString userLangDir = dir.filePath("languages");
    QDir langDir(userLangDir);
    if (!langDir.exists()) {
        langDir.mkpath(".");
    }
    return userLangDir;
}

bool LanguageRegistry::registerLanguage(const LanguageDefinition &def, const QString &filePath, LanguagePackSource source) {
    LanguagePackInfo info;
    info.id = def.id;
    info.name = def.name;
    info.extensions = def.extensions;
    info.filePath = filePath;
    info.source = source;

    if (!def.isValid()) {
        info.loaded = false;
        info.statusMessage = "Invalid definition: 'id' or 'name' missing";
        m_packInfos.append(info);
        return false;
    }

    if (m_definitionsById.contains(def.id)) {
        info.loaded = false;
        info.statusMessage = QString("Duplicate identifier '%1' ignored").arg(def.id);
        m_packInfos.append(info);
        return false;
    }

    m_definitionsById[def.id] = def;
    for (const QString &ext : def.extensions) {
        if (!m_extensionToIdMap.contains(ext.toLower())) {
            m_extensionToIdMap[ext.toLower()] = def.id;
        }
    }

    info.loaded = true;
    info.statusMessage = "Loaded successfully";
    m_packInfos.append(info);
    return true;
}

bool LanguageRegistry::loadFromFile(const QString &filePath, LanguagePackSource source) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LanguagePackInfo info;
        info.filePath = filePath;
        info.source = source;
        info.loaded = false;
        info.statusMessage = QString("Failed to open file: %1").arg(file.errorString());
        m_packInfos.append(info);
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        LanguagePackInfo info;
        info.filePath = filePath;
        info.source = source;
        info.loaded = false;
        info.statusMessage = QString("Invalid JSON format: %1").arg(error.errorString());
        m_packInfos.append(info);
        return false;
    }

    return loadFromJson(doc.object(), filePath, source);
}

bool LanguageRegistry::loadFromJson(const QJsonObject &json, const QString &filePath, LanguagePackSource source) {
    bool ok = false;
    LanguageDefinition def = LanguageDefinition::fromJson(json, &ok);
    if (!ok) {
        LanguagePackInfo info;
        info.id = json.value("id").toString();
        info.name = json.value("name").toString();
        info.filePath = filePath;
        info.source = source;
        info.loaded = false;
        info.statusMessage = "Failed to parse language pack definition";
        m_packInfos.append(info);
        return false;
    }
    return registerLanguage(def, filePath, source);
}

int LanguageRegistry::loadFromDirectory(const QString &dirPath, LanguagePackSource source) {
    QDir dir(dirPath);
    if (!dir.exists()) return 0;

    int loadedCount = 0;
    QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &fileName : files) {
        if (loadFromFile(dir.filePath(fileName), source)) {
            loadedCount++;
        }
    }
    return loadedCount;
}

void LanguageRegistry::discoverPacks() {
    clear();
    // CinderDE ships no built-in language packs. Load only user-installed packs.

    // Search user-installed languages directory
    QString userDir = userLanguagesDirectory();
    loadFromDirectory(userDir, LanguagePackSource::User);
}

void LanguageRegistry::registerBuiltinLanguages() {
    // Intentionally empty: v1.0 ships no built-in language support packs.
}

QList<LanguagePackInfo> LanguageRegistry::packInfos() const {
    return m_packInfos;
}

LanguageDefinition LanguageRegistry::definitionForExtension(const QString &extension) const {
    QString ext = extension.toLower();
    if (ext.startsWith('.')) ext = ext.mid(1);

    if (m_extensionToIdMap.contains(ext)) {
        QString id = m_extensionToIdMap.value(ext);
        return m_definitionsById.value(id);
    }
    return LanguageDefinition();
}

LanguageDefinition LanguageRegistry::definitionForId(const QString &id) const {
    return m_definitionsById.value(id.toLower());
}

QList<LanguageDefinition> LanguageRegistry::allDefinitions() const {
    return m_definitionsById.values();
}

bool LanguageRegistry::hasDefinition(const QString &id) const {
    return m_definitionsById.contains(id.toLower());
}

void LanguageRegistry::clear() {
    m_definitionsById.clear();
    m_extensionToIdMap.clear();
    m_packInfos.clear();
    m_builtinsLoaded = false;
}

} // namespace CinderDE
