#ifndef CINDERDE_LSP_TYPES_H
#define CINDERDE_LSP_TYPES_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QUrl>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

namespace CinderDE {

struct LspServerConfig {
    QString id;
    QString name;
    QString languageId;
    QStringList extensions;
    QString executable;
    QStringList arguments;
    QMap<QString, QString> environment;
    bool enabled = true;

    bool isValid() const {
        return !id.isEmpty() && !executable.isEmpty() && !languageId.isEmpty();
    }
};

enum class LspServerState {
    Stopped,
    Starting,
    Running,
    Stopping,
    Failed
};

struct LspServerStatus {
    LspServerState state = LspServerState::Stopped;
    qint64 pid = 0;
    qint64 ramBytes = -1; // -1 means unavailable
    QString lastError;
};

struct LspPosition {
    int line = 0;      // 0-based
    int character = 0; // 0-based

    static LspPosition fromJson(const QJsonObject &obj) {
        LspPosition pos;
        pos.line = obj.value("line").toInt(0);
        pos.character = obj.value("character").toInt(0);
        return pos;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["line"] = line;
        obj["character"] = character;
        return obj;
    }

    bool operator==(const LspPosition &other) const {
        return line == other.line && character == other.character;
    }
};

struct LspRange {
    LspPosition start;
    LspPosition end;

    static LspRange fromJson(const QJsonObject &obj) {
        LspRange range;
        range.start = LspPosition::fromJson(obj.value("start").toObject());
        range.end = LspPosition::fromJson(obj.value("end").toObject());
        return range;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["start"] = start.toJson();
        obj["end"] = end.toJson();
        return obj;
    }
};

struct LspLocation {
    QString uri;
    LspRange range;

    static LspLocation fromJson(const QJsonObject &obj) {
        LspLocation loc;
        loc.uri = obj.value("uri").toString();
        loc.range = LspRange::fromJson(obj.value("range").toObject());
        return loc;
    }
};

struct LspDiagnostic {
    LspRange range;
    int severity = 1; // 1: Error, 2: Warning, 3: Information, 4: Hint
    QString code;
    QString source;
    QString message;

    static LspDiagnostic fromJson(const QJsonObject &obj) {
        LspDiagnostic diag;
        diag.range = LspRange::fromJson(obj.value("range").toObject());
        diag.severity = obj.value("severity").toInt(1);
        if (obj.contains("code")) {
            if (obj.value("code").isString()) {
                diag.code = obj.value("code").toString();
            } else {
                diag.code = QString::number(obj.value("code").toInt());
            }
        }
        diag.source = obj.value("source").toString();
        diag.message = obj.value("message").toString();
        return diag;
    }
};

struct LspCompletionItem {
    QString label;
    int kind = 0;
    QString detail;
    QString documentation;
    QString insertText;

    static LspCompletionItem fromJson(const QJsonObject &obj) {
        LspCompletionItem item;
        item.label = obj.value("label").toString();
        item.kind = obj.value("kind").toInt(0);
        item.detail = obj.value("detail").toString();

        QJsonValue docVal = obj.value("documentation");
        if (docVal.isString()) {
            item.documentation = docVal.toString();
        } else if (docVal.isObject()) {
            item.documentation = docVal.toObject().value("value").toString();
        }

        if (obj.contains("insertText") && !obj.value("insertText").toString().isEmpty()) {
            item.insertText = obj.value("insertText").toString();
        } else {
            item.insertText = item.label;
        }

        return item;
    }
};

struct LspHover {
    QString contents;

    static LspHover fromJson(const QJsonObject &obj) {
        LspHover hover;
        QJsonValue val = obj.value("contents");
        if (val.isString()) {
            hover.contents = val.toString();
        } else if (val.isObject()) {
            hover.contents = val.toObject().value("value").toString();
        } else if (val.isArray()) {
            QStringList lines;
            for (const QJsonValue &v : val.toArray()) {
                if (v.isString()) lines.append(v.toString());
                else if (v.isObject()) lines.append(v.toObject().value("value").toString());
            }
            hover.contents = lines.join("\n");
        }
        return hover;
    }
};

struct LspTextEdit {
    LspRange range;
    QString newText;

    static LspTextEdit fromJson(const QJsonObject &obj) {
        LspTextEdit edit;
        edit.range = LspRange::fromJson(obj.value("range").toObject());
        edit.newText = obj.value("newText").toString();
        return edit;
    }
};

struct LspWorkspaceEdit {
    QMap<QString, QList<LspTextEdit>> changes; // URI -> List of edits

    static LspWorkspaceEdit fromJson(const QJsonObject &obj) {
        LspWorkspaceEdit edit;
        if (obj.contains("changes") && obj.value("changes").isObject()) {
            QJsonObject changesObj = obj.value("changes").toObject();
            for (auto it = changesObj.begin(); it != changesObj.end(); ++it) {
                QString uri = it.key();
                QList<LspTextEdit> edits;
                for (const QJsonValue &v : it.value().toArray()) {
                    edits.append(LspTextEdit::fromJson(v.toObject()));
                }
                edit.changes[uri] = edits;
            }
        }
        if (obj.contains("documentChanges") && obj.value("documentChanges").isArray()) {
            QJsonArray docChanges = obj.value("documentChanges").toArray();
            for (const QJsonValue &v : docChanges) {
                if (v.isObject()) {
                    QJsonObject changeObj = v.toObject();
                    if (changeObj.contains("textDocument") && changeObj.contains("edits")) {
                        QString uri = changeObj.value("textDocument").toObject().value("uri").toString();
                        QList<LspTextEdit> edits;
                        for (const QJsonValue &ev : changeObj.value("edits").toArray()) {
                            edits.append(LspTextEdit::fromJson(ev.toObject()));
                        }
                        edit.changes[uri] = edits;
                    }
                }
            }
        }
        return edit;
    }
};

struct LspServerCapabilities {
    bool completionProvider = false;
    bool hoverProvider = false;
    bool definitionProvider = false;
    bool referencesProvider = false;
    bool renameProvider = false;
    int textDocumentSync = 1; // 0 = None, 1 = Full, 2 = Incremental

    static LspServerCapabilities fromJson(const QJsonObject &capsObj) {
        LspServerCapabilities caps;
        if (capsObj.contains("completionProvider")) {
            caps.completionProvider = true;
        }
        if (capsObj.contains("hoverProvider")) {
            caps.hoverProvider = capsObj.value("hoverProvider").toBool(true);
        }
        if (capsObj.contains("definitionProvider")) {
            caps.definitionProvider = capsObj.value("definitionProvider").toBool(true);
        }
        if (capsObj.contains("referencesProvider")) {
            caps.referencesProvider = capsObj.value("referencesProvider").toBool(true);
        }
        if (capsObj.contains("renameProvider")) {
            caps.renameProvider = true;
        }

        if (capsObj.contains("textDocumentSync")) {
            QJsonValue syncVal = capsObj.value("textDocumentSync");
            if (syncVal.isBool()) {
                caps.textDocumentSync = syncVal.toBool() ? 1 : 0;
            } else if (syncVal.isDouble()) {
                caps.textDocumentSync = syncVal.toInt(1);
            } else if (syncVal.isObject()) {
                caps.textDocumentSync = syncVal.toObject().value("change").toInt(1);
            }
        }
        return caps;
    }
};

inline QString pathToUri(const QString &filePath) {
    if (filePath.isEmpty()) return QString();
    return QUrl::fromLocalFile(filePath).toString();
}

inline QString uriToPath(const QString &uri) {
    if (uri.isEmpty()) return QString();
    return QUrl(uri).toLocalFile();
}

} // namespace CinderDE

#endif // CINDERDE_LSP_TYPES_H
