#ifndef CINDERDE_LANGUAGE_PACK_H
#define CINDERDE_LANGUAGE_PACK_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QList>
#include <QMap>

namespace CinderDE {

enum class LanguagePackSource {
    Builtin,
    User
};

struct LanguagePackInfo {
    QString id;
    QString name;
    QStringList extensions;
    QString filePath;
    LanguagePackSource source{LanguagePackSource::User};
    bool loaded{false};
    QString statusMessage;
};

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

    static QString userLanguagesDirectory();

    bool registerLanguage(const LanguageDefinition &def, const QString &filePath = QString(), LanguagePackSource source = LanguagePackSource::Builtin);
    bool loadFromFile(const QString &filePath, LanguagePackSource source = LanguagePackSource::User);
    bool loadFromJson(const QJsonObject &json, const QString &filePath = QString(), LanguagePackSource source = LanguagePackSource::User);
    int loadFromDirectory(const QString &dirPath, LanguagePackSource source = LanguagePackSource::User);
    void registerBuiltinLanguages();
    void discoverPacks();

    LanguageDefinition definitionForExtension(const QString &extension) const;
    LanguageDefinition definitionForId(const QString &id) const;
    QList<LanguageDefinition> allDefinitions() const;
    QList<LanguagePackInfo> packInfos() const;

    bool hasDefinition(const QString &id) const;
    void clear();

private:
    LanguageRegistry();
    ~LanguageRegistry() = default;

    LanguageRegistry(const LanguageRegistry&) = delete;
    LanguageRegistry& operator=(const LanguageRegistry&) = delete;

    QMap<QString, LanguageDefinition> m_definitionsById;
    QMap<QString, QString> m_extensionToIdMap;
    QList<LanguagePackInfo> m_packInfos;
    bool m_builtinsLoaded{false};
};

} // namespace CinderDE

#endif // CINDERDE_LANGUAGE_PACK_H
