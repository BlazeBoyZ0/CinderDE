#ifndef CINDERDE_LANGUAGE_PACK_H
#define CINDERDE_LANGUAGE_PACK_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QList>
#include <QMap>

namespace CinderDE {

struct CommentSettings {
    QString linePrefix;
    QString blockStart;
    QString blockEnd;
};

struct IndentationSettings {
    int tabSize{4};
    bool useSpaces{true};
    bool autoIndentOnBrace{true};
};

struct BuildRunProfile {
    QString buildCommand;
    QString runCommand;
    QString workingDirMode;
};

struct LanguageDefinition {
    QString id;
    QString name;
    QStringList extensions;

    QStringList keywords;
    QStringList types;
    QStringList builtins;

    CommentSettings comments;
    IndentationSettings indentation;
    BuildRunProfile buildRun;

    bool isValid() const {
        return !id.trimmed().isEmpty() && !name.trimmed().isEmpty();
    }

    static LanguageDefinition fromJson(const QJsonObject &json, bool *ok = nullptr);
    QJsonObject toJson() const;
};

class LanguageRegistry {
public:
    static LanguageRegistry& instance();

    bool registerLanguage(const LanguageDefinition &def);
    bool loadFromFile(const QString &filePath);
    bool loadFromJson(const QJsonObject &json);
    int loadFromDirectory(const QString &dirPath);
    void registerBuiltinLanguages();

    LanguageDefinition definitionForExtension(const QString &extension) const;
    LanguageDefinition definitionForId(const QString &id) const;
    QList<LanguageDefinition> allDefinitions() const;

    bool hasDefinition(const QString &id) const;
    void clear();

private:
    LanguageRegistry();
    ~LanguageRegistry() = default;

    LanguageRegistry(const LanguageRegistry&) = delete;
    LanguageRegistry& operator=(const LanguageRegistry&) = delete;

    QMap<QString, LanguageDefinition> m_definitionsById;
    QMap<QString, QString> m_extensionToIdMap;
    bool m_builtinsLoaded{false};
};

} // namespace CinderDE

#endif // CINDERDE_LANGUAGE_PACK_H
