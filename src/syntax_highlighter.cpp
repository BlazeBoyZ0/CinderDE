#include "cinderde/syntax_highlighter.h"
#include <QTextDocument>

namespace CinderDE {

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent),
      m_language(Language::None),
      m_theme(SyntaxTheme::darkTheme())
{
}

Language SyntaxHighlighter::languageFromExtension(const QString &extension) {
    LanguageDefinition def = LanguageRegistry::instance().definitionForExtension(extension);
    if (def.id == "c") return Language::C;
    if (def.id == "cpp") return Language::Cpp;
    if (def.id == "python") return Language::Python;

    QString ext = extension.toLower();
    if (ext.startsWith('.')) {
        ext = ext.mid(1);
    }

    if (ext == "c") {
        return Language::C;
    } else if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "hpp" || ext == "hxx" || ext == "h") {
        return Language::Cpp;
    } else if (ext == "py") {
        return Language::Python;
    }

    return Language::None;
}

LanguageDefinition SyntaxHighlighter::definitionFromExtension(const QString &extension) {
    return LanguageRegistry::instance().definitionForExtension(extension);
}

void SyntaxHighlighter::setLanguage(Language lang) {
    m_language = lang;
    if (lang == Language::None) {
        setLanguageDefinition(LanguageDefinition());
    }
}

void SyntaxHighlighter::setLanguageDefinition(const LanguageDefinition &def) {
    m_languageDef = def;

    if (def.id == "c") m_language = Language::C;
    else if (def.id == "cpp") m_language = Language::Cpp;
    else if (def.id == "python") m_language = Language::Python;
    else m_language = Language::None;

    rebuildRulesFromDefinition();
    rehighlight();
}

void SyntaxHighlighter::rebuildRulesFromDefinition() {
    m_rules.clear();
    m_typeRules.clear();

    if (!m_languageDef.isValid()) return;

    // Keywords
    for (const QString &keyword : m_languageDef.keywords) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(keyword));
        rule.format = m_theme.keywordFormat;
        m_rules.append(rule);
    }

    // Types
    for (const QString &type : m_languageDef.types) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(type));
        rule.format = m_theme.typeFormat;
        m_typeRules.append(rule);
    }

    // Builtins
    for (const QString &builtin : m_languageDef.builtins) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(builtin));
        rule.format = m_theme.typeFormat;
        m_typeRules.append(rule);
    }

    // Functions
    HighlightingRule funcRule;
    funcRule.pattern = QRegularExpression(R"(\b[a-zA-Z_]\w*(?=\s*\())");
    funcRule.format = m_theme.functionFormat;
    m_rules.append(funcRule);

    // Numbers
    HighlightingRule numRule;
    numRule.pattern = QRegularExpression(R"(\b(0[xX][0-9a-fA-F]+|0[bB][01]+|0[oO][0-7]+|[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?)[uUlLfF]*\b)");
    numRule.format = m_theme.numberFormat;
    m_rules.append(numRule);

    // Operators
    HighlightingRule opRule;
    opRule.pattern = QRegularExpression(R"([\+\-\*\/\%=\&\!\|\^\<\>\~\?:]+)");
    opRule.format = m_theme.operatorFormat;
    m_rules.append(opRule);
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    if (!m_languageDef.isValid()) {
        setCurrentBlockState(0);
        return;
    }

    highlightGenericBlock(text);
}

void SyntaxHighlighter::applyMaskedRules(const QString &text, const QVector<HighlightingRule> &rules, const QVector<bool> &isMasked) {
    int length = text.length();
    for (const HighlightingRule &rule : rules) {
        QRegularExpressionMatchIterator i = rule.pattern.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            int start = match.capturedStart();
            int len = match.capturedLength();
            bool masked = false;
            for (int k = start; k < start + len && k < length; ++k) {
                if (isMasked[k]) {
                    masked = true;
                    break;
                }
            }
            if (!masked) {
                setFormat(start, len, rule.format);
            }
        }
    }
}


void SyntaxHighlighter::highlightGenericBlock(const QString &text) {
    int length = text.length();
    QVector<bool> isMasked(length, false);
    struct FormatSpan {
        int start;
        int length;
        QTextCharFormat format;
    };
    QVector<FormatSpan> stringCommentSpans;

    int state = previousBlockState();
    if (state < 0) state = 0;

    QString lineComment = m_languageDef.comments.linePrefix;
    QString blockStart = m_languageDef.comments.blockStart;
    QString blockEnd = m_languageDef.comments.blockEnd;
    bool hasBlockComments = !blockStart.isEmpty() && !blockEnd.isEmpty();

    int i = 0;

    while (i < length) {
        if (state == 1 && hasBlockComments) { // Inside multiline block comment
            int endIdx = text.indexOf(blockEnd, i);
            if (endIdx != -1) {
                int commentLen = (endIdx + blockEnd.length()) - i;
                for (int k = i; k < endIdx + blockEnd.length(); ++k) isMasked[k] = true;
                stringCommentSpans.append({i, commentLen, m_theme.commentFormat});
                i = endIdx + blockEnd.length();
                state = 0;
            } else {
                for (int k = i; k < length; ++k) isMasked[k] = true;
                stringCommentSpans.append({i, length - i, m_theme.commentFormat});
                i = length;
            }
        } else { // Normal mode
            // Check single line comment
            if (!lineComment.isEmpty() && text.mid(i).startsWith(lineComment)) {
                for (int k = i; k < length; ++k) isMasked[k] = true;
                stringCommentSpans.append({i, length - i, m_theme.commentFormat});
                break;
            }

            // Check start of block comment
            if (hasBlockComments && text.mid(i).startsWith(blockStart)) {
                state = 1;
                int endIdx = text.indexOf(blockEnd, i + blockStart.length());
                if (endIdx != -1) {
                    int commentLen = (endIdx + blockEnd.length()) - i;
                    for (int k = i; k < endIdx + blockEnd.length(); ++k) isMasked[k] = true;
                    stringCommentSpans.append({i, commentLen, m_theme.commentFormat});
                    i = endIdx + blockEnd.length();
                    state = 0;
                } else {
                    for (int k = i; k < length; ++k) isMasked[k] = true;
                    stringCommentSpans.append({i, length - i, m_theme.commentFormat});
                    i = length;
                }
                continue;
            }

            // String or Char literal ('...' or "...")
            QChar ch = text.at(i);
            if (ch == '"' || ch == '\'') {
                QChar quote = ch;
                int start = i;
                i++;
                while (i < length) {
                    if (text.at(i) == '\\') {
                        i += 2; // skip escaped character
                        continue;
                    }
                    if (text.at(i) == quote) {
                        i++; // consume closing quote
                        break;
                    }
                    i++;
                }
                for (int k = start; k < i && k < length; ++k) isMasked[k] = true;
                stringCommentSpans.append({start, i - start, m_theme.stringFormat});
                continue;
            }

            i++;
        }
    }

    setCurrentBlockState(state);

    applyMaskedRules(text, m_rules, isMasked);
    applyMaskedRules(text, m_typeRules, isMasked);

    for (const auto &span : stringCommentSpans) {
        setFormat(span.start, span.length, span.format);
    }
}

} // namespace CinderDE
