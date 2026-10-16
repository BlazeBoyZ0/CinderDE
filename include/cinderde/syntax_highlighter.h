#ifndef CINDERDE_SYNTAX_HIGHLIGHTER_H
#define CINDERDE_SYNTAX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QVector>
#include "cinderde/syntax_theme.h"

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
    Language language() const { return m_language; }

    static Language languageFromExtension(const QString &extension);

protected:
    void highlightBlock(const QString &text) override;

private:
    Language m_language;
    SyntaxTheme m_theme;

    QVector<HighlightingRule> m_cRules;
    QVector<HighlightingRule> m_cppRules;
    QVector<HighlightingRule> m_pythonRules;

    QVector<HighlightingRule> m_cTypeRules;
    QVector<HighlightingRule> m_cppTypeRules;
    QVector<HighlightingRule> m_pythonBuiltinRules;

    void setupCRules();
    void setupCppRules();
    void setupPythonRules();

    void highlightCLikeBlock(const QString &text);
    void highlightPythonBlock(const QString &text);
    void applyMaskedRules(const QString &text, const QVector<HighlightingRule> &rules, const QVector<bool> &isMasked);
};

} // namespace CinderDE

#endif // CINDERDE_SYNTAX_HIGHLIGHTER_H
