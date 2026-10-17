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
    registerBuiltinLanguages();
}

bool LanguageRegistry::registerLanguage(const LanguageDefinition &def) {
    if (!def.isValid()) return false;

    m_definitionsById[def.id] = def;
    for (const QString &ext : def.extensions) {
        m_extensionToIdMap[ext.toLower()] = def.id;
    }
    return true;
}

bool LanguageRegistry::loadFromFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    return loadFromJson(doc.object());
}

bool LanguageRegistry::loadFromJson(const QJsonObject &json) {
    bool ok = false;
    LanguageDefinition def = LanguageDefinition::fromJson(json, &ok);
    if (!ok) return false;
    return registerLanguage(def);
}

int LanguageRegistry::loadFromDirectory(const QString &dirPath) {
    QDir dir(dirPath);
    if (!dir.exists()) return 0;

    int loadedCount = 0;
    QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &fileName : files) {
        if (loadFromFile(dir.filePath(fileName))) {
            loadedCount++;
        }
    }
    return loadedCount;
}

void LanguageRegistry::registerBuiltinLanguages() {
    if (m_builtinsLoaded) return;
    m_builtinsLoaded = true;

    // Check Qt resources directory :/resources/languages and :/languages
    loadFromDirectory(":/resources/languages");
    loadFromDirectory(":/languages");

    // Search local directory paths
    QStringList searchPaths = {
        "./languages",
        "../share/cinderde/languages",
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).filePath("languages")
    };

    for (const QString &path : searchPaths) {
        loadFromDirectory(path);
    }

    // Fallback built-in C definition if not present
    if (!hasDefinition("c")) {
        LanguageDefinition cDef;
        cDef.id = "c";
        cDef.name = "C Source";
        cDef.extensions = {"c"};
        cDef.keywords = {
            "auto", "break", "case", "const", "continue", "default", "do",
            "else", "enum", "extern", "for", "goto", "if", "inline", "register",
            "restrict", "return", "sizeof", "static", "struct", "switch", "typedef",
            "union", "volatile", "while", "_Alignas", "_Alignof", "_Atomic",
            "_Generic", "_Noreturn", "_Static_assert", "_Thread_local", "NULL", "nullptr"
        };
        cDef.types = {
            "char", "double", "float", "int", "long", "short", "signed",
            "unsigned", "void", "_Bool", "_Complex", "_Imaginary", "bool",
            "true", "false", "size_t", "ssize_t", "int8_t", "int16_t", "int32_t", "int64_t",
            "uint8_t", "uint16_t", "uint32_t", "uint64_t", "intptr_t", "uintptr_t",
            "ptrdiff_t", "FILE", "time_t"
        };
        cDef.comments = {"//", "/*", "*/"};
        cDef.indentation = {4, true, true};
        cDef.buildRun = {"gcc -g \"${file}\" -o \"${fileDir}/${fileBase}\"", "\"${fileDir}/${fileBase}\"", "workspace"};
        registerLanguage(cDef);
    }

    // Fallback built-in C++ definition if not present
    if (!hasDefinition("cpp")) {
        LanguageDefinition cppDef;
        cppDef.id = "cpp";
        cppDef.name = "C++ Source";
        cppDef.extensions = {"cpp", "cc", "cxx", "hpp", "hxx", "h"};
        cppDef.keywords = {
            "auto", "break", "case", "const", "continue", "default", "do",
            "else", "enum", "extern", "for", "goto", "if", "inline", "register",
            "restrict", "return", "sizeof", "static", "struct", "switch", "typedef",
            "union", "volatile", "while", "_Alignas", "_Alignof", "_Atomic",
            "_Generic", "_Noreturn", "_Static_assert", "_Thread_local", "NULL", "nullptr",
            "alignas", "alignof", "and", "and_eq", "asm", "bitand", "bitor",
            "catch", "class", "compl", "concept", "consteval", "constexpr",
            "constinit", "const_cast", "decltype", "delete", "dynamic_cast",
            "explicit", "export", "friend", "mutable", "namespace", "new",
            "noexcept", "not", "not_eq", "operator", "or", "or_eq", "private",
            "protected", "public", "reinterpret_cast", "requires", "static_assert",
            "static_cast", "template", "this", "thread_local", "throw", "try",
            "typeid", "typename", "using", "virtual", "xor", "xor_eq",
            "co_await", "co_return", "co_yield", "std", "override", "final"
        };
        cppDef.types = {
            "char", "double", "float", "int", "long", "short", "signed",
            "unsigned", "void", "_Bool", "_Complex", "_Imaginary", "bool",
            "true", "false", "size_t", "ssize_t", "int8_t", "int16_t", "int32_t", "int64_t",
            "uint8_t", "uint16_t", "uint32_t", "uint64_t", "intptr_t", "uintptr_t",
            "ptrdiff_t", "FILE", "time_t",
            "char16_t", "char32_t", "char8_t", "wchar_t",
            "string", "wstring", "u16string", "u32string", "string_view",
            "vector", "map", "set", "unordered_map", "unordered_set",
            "shared_ptr", "unique_ptr", "weak_ptr", "optional", "variant",
            "any", "pair", "tuple", "function"
        };
        cppDef.comments = {"//", "/*", "*/"};
        cppDef.indentation = {4, true, true};
        cppDef.buildRun = {"g++ -g \"${file}\" -o \"${fileDir}/${fileBase}\"", "\"${fileDir}/${fileBase}\"", "workspace"};
        registerLanguage(cppDef);
    }

    // Fallback built-in Python definition if not present
    if (!hasDefinition("python")) {
        LanguageDefinition pyDef;
        pyDef.id = "python";
        pyDef.name = "Python Source";
        pyDef.extensions = {"py"};
        pyDef.keywords = {
            "False", "None", "True", "and", "as", "assert", "async", "await",
            "break", "class", "continue", "def", "del", "elif", "else", "except",
            "finally", "for", "from", "global", "if", "import", "in", "is",
            "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
            "while", "with", "yield"
        };
        pyDef.builtins = {
            "abs", "all", "any", "ascii", "bin", "bool", "breakpoint", "bytearray",
            "bytes", "callable", "chr", "classmethod", "compile", "complex", "delattr",
            "dict", "dir", "divmod", "enumerate", "eval", "exec", "filter", "float",
            "format", "frozenset", "getattr", "globals", "hasattr", "hash", "help",
            "hex", "id", "input", "int", "isinstance", "issubclass", "iter", "len",
            "list", "locals", "map", "max", "memoryview", "min", "next", "object",
            "oct", "open", "ord", "pow", "print", "property", "range", "repr",
            "reversed", "round", "set", "setattr", "slice", "sorted", "staticmethod",
            "str", "sum", "super", "tuple", "type", "vars", "zip", "self"
        };
        pyDef.comments = {"#", "", ""};
        pyDef.indentation = {4, true, false};
        pyDef.buildRun = {"", "python3 \"${file}\"", "workspace"};
        registerLanguage(pyDef);
    }
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
    m_builtinsLoaded = false;
}

} // namespace CinderDE
