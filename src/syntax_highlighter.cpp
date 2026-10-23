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

LanguageDefinition SyntaxHighlighter::fallbackDefinitionForLanguage(Language lang) {
    LanguageDefinition def;
    if (lang == Language::C) {
        def.id = "c";
        def.name = "C";
        def.extensions = QStringList() << "c" << "h";
        def.keywords = QStringList() << "auto" << "break" << "case" << "const" << "continue"
                                     << "default" << "do" << "else" << "enum" << "extern"
                                     << "for" << "goto" << "if" << "inline" << "register"
                                     << "restrict" << "return" << "sizeof" << "static" << "struct"
                                     << "switch" << "typedef" << "union" << "volatile" << "while";
        def.types = QStringList() << "char" << "double" << "float" << "int" << "long"
                                  << "short" << "signed" << "unsigned" << "void" << "bool"
                                  << "int8_t" << "int16_t" << "int32_t" << "int64_t"
                                  << "uint8_t" << "uint16_t" << "uint32_t" << "uint64_t"
                                  << "size_t" << "ptrdiff_t" << "intptr_t" << "uintptr_t";
        def.comments.linePrefix = "//";
        def.comments.blockStart = "/*";
        def.comments.blockEnd = "*/";
    } else if (lang == Language::Cpp) {
        def.id = "cpp";
        def.name = "C++";
        def.extensions = QStringList() << "cpp" << "cc" << "cxx" << "hpp" << "hxx" << "h";
        def.keywords = QStringList() << "alignas" << "alignof" << "and" << "and_eq" << "asm"
                                     << "auto" << "bitand" << "bitor" << "break" << "case"
                                     << "catch" << "class" << "compl" << "concept" << "const"
                                     << "consteval" << "constexpr" << "constinit" << "const_cast" << "continue"
                                     << "co_await" << "co_return" << "co_yield" << "decltype" << "default"
                                     << "delete" << "do" << "dynamic_cast" << "else" << "enum"
                                     << "explicit" << "export" << "extern" << "false" << "for"
                                     << "friend" << "goto" << "if" << "inline" << "mutable"
                                     << "namespace" << "new" << "noexcept" << "not" << "not_eq"
                                     << "nullptr" << "operator" << "or" << "or_eq" << "private"
                                     << "protected" << "public" << "reinterpret_cast" << "requires" << "return"
                                     << "sizeof" << "static" << "static_assert" << "static_cast" << "struct"
                                     << "switch" << "template" << "this" << "thread_local" << "throw"
                                     << "true" << "try" << "typedef" << "typeid" << "typename"
                                     << "union" << "using" << "virtual" << "volatile" << "while"
                                     << "xor" << "xor_eq";
        def.types = QStringList() << "bool" << "char" << "char8_t" << "char16_t" << "char32_t"
                                  << "double" << "float" << "int" << "long" << "short"
                                  << "signed" << "unsigned" << "void" << "wchar_t"
                                  << "int8_t" << "int16_t" << "int32_t" << "int64_t"
                                  << "uint8_t" << "uint16_t" << "uint32_t" << "uint64_t"
                                  << "size_t" << "std";
        def.comments.linePrefix = "//";
        def.comments.blockStart = "/*";
        def.comments.blockEnd = "*/";
    } else if (lang == Language::Python) {
        def.id = "python";
        def.name = "Python";
        def.extensions = QStringList() << "py" << "pyw" << "pyi";
        def.keywords = QStringList() << "and" << "as" << "assert" << "async" << "await"
                                     << "break" << "class" << "continue" << "def" << "del"
                                     << "elif" << "else" << "except" << "False" << "finally"
                                     << "for" << "from" << "global" << "if" << "import"
                                     << "in" << "is" << "lambda" << "None" << "nonlocal"
                                     << "not" << "or" << "pass" << "raise" << "return"
                                     << "True" << "try" << "while" << "with" << "yield";
        def.builtins = QStringList() << "print" << "len" << "range" << "str" << "int"
                                     << "float" << "list" << "dict" << "set" << "tuple"
                                     << "bool" << "type" << "super" << "isinstance" << "open";
        def.comments.linePrefix = "#";
        def.comments.blockStart = "\"\"\"";
        def.comments.blockEnd = "\"\"\"";
    }
    return def;
}

void SyntaxHighlighter::setLanguage(Language lang) {
    m_language = lang;
    LanguageDefinition def;
    if (lang == Language::C) {
        def = LanguageRegistry::instance().definitionForId("c");
    } else if (lang == Language::Cpp) {
        def = LanguageRegistry::instance().definitionForId("cpp");
    } else if (lang == Language::Python) {
        def = LanguageRegistry::instance().definitionForId("python");
    }

    if (def.isValid()) {
        setLanguageDefinition(def);
    } else {
        LanguageDefinition fallback = fallbackDefinitionForLanguage(lang);
        if (fallback.isValid()) {
            setLanguageDefinition(fallback);
            m_language = lang;
        } else {
            setLanguageDefinition(LanguageDefinition());
        }
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

    // Preprocessor Directives (C/C++ or languages with preprocessor)
    if (m_languageDef.id == "c" || m_languageDef.id == "cpp") {
        HighlightingRule prepRule;
        prepRule.pattern = QRegularExpression(R"(^\s*#\s*[a-zA-Z_]\w*)");
        prepRule.format = m_theme.preprocessorFormat;
        m_rules.append(prepRule);
    }

    // Decorators (@decorator_name)
    if (m_languageDef.id == "python") {
        HighlightingRule decRule;
        decRule.pattern = QRegularExpression(R"(@[a-zA-Z_]\w*)");
        decRule.format = m_theme.decoratorFormat;
        m_rules.append(decRule);
    }

    // Functions
    HighlightingRule funcRule;
    funcRule.pattern = QRegularExpression(R"(\b[a-zA-Z_]\w*(?=\s*\())");
    funcRule.format = m_theme.functionFormat;
    m_rules.append(funcRule);

    // Numbers
    HighlightingRule numRule;
    if (m_languageDef.id == "python") {
        numRule.pattern = QRegularExpression(R"(\b(0[xX][0-9a-fA-F]+|0[bB][01]+|0[oO][0-7]+|[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?)\b)");
    } else {
        numRule.pattern = QRegularExpression(R"(\b(0[xX][0-9a-fA-F]+|0[bB][01]+|0[0-7]+|[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?)[uUlLfF]*\b)");
    }
    numRule.format = m_theme.numberFormat;
    m_rules.append(numRule);

    // Operators
    HighlightingRule opRule;
    if (m_languageDef.id == "python") {
        opRule.pattern = QRegularExpression(R"([\+\-\*\/\%=\&\!\|\^\<\>\~\:]+)");
    } else {
        opRule.pattern = QRegularExpression(R"([\+\-\*\/\%=\&\!\|\^\<\>\~\?]+)");
    }
    opRule.format = m_theme.operatorFormat;
    m_rules.append(opRule);
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    if (!m_languageDef.isValid()) {
        setCurrentBlockState(0);
        return;
    }

    if (m_languageDef.id == "c" || m_languageDef.id == "cpp") {
        highlightCLikeBlock(text);
    } else if (m_languageDef.id == "python") {
        highlightPythonBlock(text);
    } else {
        highlightGenericBlock(text);
    }
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

void SyntaxHighlighter::highlightCLikeBlock(const QString &text) {
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

    int i = 0;

    while (i < length) {
        if (state == 1) { // Inside multiline comment
            int endIdx = text.indexOf("*/", i);
            if (endIdx != -1) {
                int commentLen = (endIdx + 2) - i;
                for (int k = i; k < endIdx + 2; ++k) isMasked[k] = true;
                stringCommentSpans.append({i, commentLen, m_theme.commentFormat});
                i = endIdx + 2;
                state = 0;
            } else {
                for (int k = i; k < length; ++k) isMasked[k] = true;
                stringCommentSpans.append({i, length - i, m_theme.commentFormat});
                i = length;
            }
        } else { // Normal mode
            QChar ch = text.at(i);

            // Single line comment //
            if (ch == '/' && i + 1 < length && text.at(i + 1) == '/') {
                for (int k = i; k < length; ++k) isMasked[k] = true;
                stringCommentSpans.append({i, length - i, m_theme.commentFormat});
                break;
            }

            // Start of block comment /*
            if (ch == '/' && i + 1 < length && text.at(i + 1) == '*') {
                state = 1;
                int endIdx = text.indexOf("*/", i + 2);
                if (endIdx != -1) {
                    int commentLen = (endIdx + 2) - i;
                    for (int k = i; k < endIdx + 2; ++k) isMasked[k] = true;
                    stringCommentSpans.append({i, commentLen, m_theme.commentFormat});
                    i = endIdx + 2;
                    state = 0;
                } else {
                    for (int k = i; k < length; ++k) isMasked[k] = true;
                    stringCommentSpans.append({i, length - i, m_theme.commentFormat});
                    i = length;
                }
                continue;
            }

            // String or Char literal ('...' or "...")
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

void SyntaxHighlighter::highlightPythonBlock(const QString &text) {
    int length = text.length();
    QVector<bool> isMasked(length, false);
    struct FormatSpan {
        int start;
        int length;
        QTextCharFormat format;
    };
    QVector<FormatSpan> stringCommentSpans;

    // States: 0 = normal, 1 = inside """...""", 2 = inside '''...'''
    int state = previousBlockState();
    if (state < 0) state = 0;

    int i = 0;

    while (i < length) {
        if (state == 1) { // Inside """
            int endIdx = text.indexOf("\"\"\"", i);
            if (endIdx != -1) {
                int strLen = (endIdx + 3) - i;
                for (int k = i; k < endIdx + 3; ++k) isMasked[k] = true;
                stringCommentSpans.append({i, strLen, m_theme.stringFormat});
                i = endIdx + 3;
                state = 0;
            } else {
                for (int k = i; k < length; ++k) isMasked[k] = true;
                stringCommentSpans.append({i, length - i, m_theme.stringFormat});
                i = length;
            }
        } else if (state == 2) { // Inside '''
            int endIdx = text.indexOf("'''", i);
            if (endIdx != -1) {
                int strLen = (endIdx + 3) - i;
                for (int k = i; k < endIdx + 3; ++k) isMasked[k] = true;
                stringCommentSpans.append({i, strLen, m_theme.stringFormat});
                i = endIdx + 3;
                state = 0;
            } else {
                for (int k = i; k < length; ++k) isMasked[k] = true;
                stringCommentSpans.append({i, length - i, m_theme.stringFormat});
                i = length;
            }
        } else { // Normal mode
            QChar ch = text.at(i);

            // Single line comment #
            if (ch == '#') {
                for (int k = i; k < length; ++k) isMasked[k] = true;
                stringCommentSpans.append({i, length - i, m_theme.commentFormat});
                break;
            }

            // Triple quote """
            if (i + 2 < length && text.mid(i, 3) == "\"\"\"") {
                state = 1;
                int endIdx = text.indexOf("\"\"\"", i + 3);
                if (endIdx != -1) {
                    int strLen = (endIdx + 3) - i;
                    for (int k = i; k < endIdx + 3; ++k) isMasked[k] = true;
                    stringCommentSpans.append({i, strLen, m_theme.stringFormat});
                    i = endIdx + 3;
                    state = 0;
                } else {
                    for (int k = i; k < length; ++k) isMasked[k] = true;
                    stringCommentSpans.append({i, length - i, m_theme.stringFormat});
                    i = length;
                }
                continue;
            }

            // Triple quote '''
            if (i + 2 < length && text.mid(i, 3) == "'''") {
                state = 2;
                int endIdx = text.indexOf("'''", i + 3);
                if (endIdx != -1) {
                    int strLen = (endIdx + 3) - i;
                    for (int k = i; k < endIdx + 3; ++k) isMasked[k] = true;
                    stringCommentSpans.append({i, strLen, m_theme.stringFormat});
                    i = endIdx + 3;
                    state = 0;
                } else {
                    for (int k = i; k < length; ++k) isMasked[k] = true;
                    stringCommentSpans.append({i, length - i, m_theme.stringFormat});
                    i = length;
                }
                continue;
            }

            // Single quote string ("..." or '...') - optional f/r/b prefix
            if (ch == '"' || ch == '\'') {
                int start = i;
                if (i > 0 && (text.at(i-1) == 'f' || text.at(i-1) == 'r' || text.at(i-1) == 'b' || text.at(i-1) == 'u' ||
                              text.at(i-1) == 'F' || text.at(i-1) == 'R' || text.at(i-1) == 'B')) {
                    start = i - 1;
                }
                QChar quote = ch;
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
