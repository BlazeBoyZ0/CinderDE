#ifndef CINDERDE_SYNTAX_THEME_H
#define CINDERDE_SYNTAX_THEME_H

#include <QTextCharFormat>
#include <QColor>
#include <QFont>

namespace CinderDE {

struct SyntaxTheme {
    QTextCharFormat keywordFormat;
    QTextCharFormat typeFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat operatorFormat;
    QTextCharFormat preprocessorFormat;
    QTextCharFormat decoratorFormat;
    QTextCharFormat functionFormat;

    static SyntaxTheme darkTheme() {
        SyntaxTheme theme;

        // Keywords (clean vibrant blue)
        theme.keywordFormat.setForeground(QColor("#569cd6"));
        theme.keywordFormat.setFontWeight(QFont::Bold);

        // Types & Builtins (vibrant teal/cyan)
        theme.typeFormat.setForeground(QColor("#4ec9b0"));

        // Strings & Chars (warm terra cotta)
        theme.stringFormat.setForeground(QColor("#ce9178"));

        // Comments (muted green, italic)
        theme.commentFormat.setForeground(QColor("#6a9955"));
        theme.commentFormat.setFontItalic(true);

        // Numbers (light sage green)
        theme.numberFormat.setForeground(QColor("#b5cea8"));

        // Operators & Punctuation (amber/gold tint)
        theme.operatorFormat.setForeground(QColor("#d8a657"));

        // Preprocessor (soft magenta/violet)
        theme.preprocessorFormat.setForeground(QColor("#c586c0"));

        // Decorators in Python (ember-orange accent)
        theme.decoratorFormat.setForeground(QColor("#ff6b35"));
        theme.decoratorFormat.setFontItalic(true);

        // Functions & Methods (soft yellow)
        theme.functionFormat.setForeground(QColor("#dcdcaa"));

        return theme;
    }
};

} // namespace CinderDE

#endif // CINDERDE_SYNTAX_THEME_H
