#ifndef CINDERDE_SYNTAX_HIGHLIGHTER_H
#define CINDERDE_SYNTAX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QVector>
#include "cinderde/syntax_theme.h"
#include "cinderde/language_pack.h"

namespace CinderDE {

enum class Language {
    None,
    C,
    Cpp,
    Python
};

struct HighlightingRule {
    QRegularExpression pattern;
    QTextCharFormat format;
};

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);

    void setLanguage(Language lang);
    void setLanguageDefinition(const LanguageDefinition &def);

    Language language() const { return m_language; }
    LanguageDefinition languageDefinition() const { return m_languageDef; }

    static Language languageFromExtension(const QString &extension);
    static LanguageDefinition definitionFromExtension(const QString &extension);

protected:
    void highlightBlock(const QString &text) override;

private:
    Language m_language;
    LanguageDefinition m_languageDef;
    SyntaxTheme m_theme;

    QVector<HighlightingRule> m_rules;
    QVector<HighlightingRule> m_typeRules;

    void rebuildRulesFromDefinition();
    void highlightCLikeBlock(const QString &text);
    void highlightPythonBlock(const QString &text);
    void highlightGenericBlock(const QString &text);
    void applyMaskedRules(const QString &text, const QVector<HighlightingRule> &rules, const QVector<bool> &isMasked);
};

} // namespace CinderDE

#endif // CINDERDE_SYNTAX_HIGHLIGHTER_H
