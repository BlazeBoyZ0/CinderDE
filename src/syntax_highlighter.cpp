#include "cinderde/syntax_highlighter.h"
#include <QTextDocument>

namespace CinderDE {

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent),
      m_language(Language::None),
      m_theme(SyntaxTheme::darkTheme())
{
    setupCRules();
    setupCppRules();
    setupPythonRules();
}

Language SyntaxHighlighter::languageFromExtension(const QString &extension) {
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

void SyntaxHighlighter::setLanguage(Language lang) {
    if (m_language != lang) {
        m_language = lang;
        rehighlight();
    }
}

void SyntaxHighlighter::setupCRules() {
    m_cRules.clear();
    m_cTypeRules.clear();

    // C Control Flow / Storage Keywords
    QStringList keywords = {
        "auto", "break", "case", "const", "continue", "default", "do",
        "else", "enum", "extern", "for", "goto", "if", "inline", "register",
        "restrict", "return", "sizeof", "static", "struct", "switch", "typedef",
        "union", "volatile", "while", "_Alignas", "_Alignof", "_Atomic",
        "_Generic", "_Noreturn", "_Static_assert", "_Thread_local", "NULL", "nullptr"
    };

    for (const QString &keyword : keywords) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(keyword));
        rule.format = m_theme.keywordFormat;
        m_cRules.append(rule);
    }

    // C Types & Built-in Primitive Types
    QStringList cTypes = {
        "char", "double", "float", "int", "long", "short", "signed",
        "unsigned", "void", "_Bool", "_Complex", "_Imaginary", "bool",
        "true", "false", "size_t", "ssize_t", "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t", "intptr_t", "uintptr_t",
        "ptrdiff_t", "FILE", "time_t"
    };

    for (const QString &type : cTypes) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(type));
        rule.format = m_theme.typeFormat;
        m_cTypeRules.append(rule);
    }

    // Preprocessor Directives
    HighlightingRule prepRule;
    prepRule.pattern = QRegularExpression(R"(^\s*#\s*[a-zA-Z_]\w*)");
    prepRule.format = m_theme.preprocessorFormat;
    m_cRules.append(prepRule);

    // Functions
    HighlightingRule funcRule;
    funcRule.pattern = QRegularExpression(R"(\b[a-zA-Z_]\w*(?=\s*\())");
    funcRule.format = m_theme.functionFormat;
    m_cRules.append(funcRule);

    // Numbers (hex, octal, binary, decimal, floating point with suffixes)
    HighlightingRule numRule;
    numRule.pattern = QRegularExpression(R"(\b(0[xX][0-9a-fA-F]+|0[bB][01]+|0[0-7]+|[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?)[uUlLfF]*\b)");
    numRule.format = m_theme.numberFormat;
    m_cRules.append(numRule);

    // Operators and Punctuation
    HighlightingRule opRule;
    opRule.pattern = QRegularExpression(R"([\+\-\*\/\%=\&\!\|\^\<\>\~\?]+)");
    opRule.format = m_theme.operatorFormat;
    m_cRules.append(opRule);
}

void SyntaxHighlighter::setupCppRules() {
    setupCRules();
    m_cppRules = m_cRules;
    m_cppTypeRules = m_cTypeRules;

    // Additional C++ Keywords
    QStringList cppKeywords = {
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

    for (const QString &keyword : cppKeywords) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(keyword));
        rule.format = m_theme.keywordFormat;
        m_cppRules.append(rule);
    }

    // Additional C++ Types
    QStringList cppTypes = {
        "char16_t", "char32_t", "char8_t", "wchar_t",
        "string", "wstring", "u16string", "u32string", "string_view",
        "vector", "map", "set", "unordered_map", "unordered_set",
        "shared_ptr", "unique_ptr", "weak_ptr", "optional", "variant",
        "any", "pair", "tuple", "function"
    };

    for (const QString &type : cppTypes) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(type));
        rule.format = m_theme.typeFormat;
        m_cppTypeRules.append(rule);
    }
}

void SyntaxHighlighter::setupPythonRules() {
    m_pythonRules.clear();
    m_pythonBuiltinRules.clear();

    // Python Keywords
    QStringList keywords = {
        "False", "None", "True", "and", "as", "assert", "async", "await",
        "break", "class", "continue", "def", "del", "elif", "else", "except",
        "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
        "while", "with", "yield"
    };

    for (const QString &keyword : keywords) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(keyword));
        rule.format = m_theme.keywordFormat;
        m_pythonRules.append(rule);
    }

    // Python Builtins & Types
    QStringList builtins = {
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

    for (const QString &builtin : builtins) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(builtin));
        rule.format = m_theme.typeFormat;
        m_pythonBuiltinRules.append(rule);
    }

    // Decorators (@decorator_name)
    HighlightingRule decRule;
    decRule.pattern = QRegularExpression(R"(@[a-zA-Z_]\w*)");
    decRule.format = m_theme.decoratorFormat;
    m_pythonRules.append(decRule);

    // Functions
    HighlightingRule funcRule;
    funcRule.pattern = QRegularExpression(R"(\b[a-zA-Z_]\w*(?=\s*\())");
    funcRule.format = m_theme.functionFormat;
    m_pythonRules.append(funcRule);

    // Numbers
    HighlightingRule numRule;
    numRule.pattern = QRegularExpression(R"(\b(0[xX][0-9a-fA-F]+|0[bB][01]+|0[oO][0-7]+|[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?)\b)");
    numRule.format = m_theme.numberFormat;
    m_pythonRules.append(numRule);

    // Operators
    HighlightingRule opRule;
    opRule.pattern = QRegularExpression(R"([\+\-\*\/\%=\&\!\|\^\<\>\~\:]+)");
    opRule.format = m_theme.operatorFormat;
    m_pythonRules.append(opRule);
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    if (m_language == Language::None) {
        setCurrentBlockState(0);
        return;
    }

    if (m_language == Language::C || m_language == Language::Cpp) {
        highlightCLikeBlock(text);
    } else if (m_language == Language::Python) {
        highlightPythonBlock(text);
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

    // Apply C/C++ regex rules only to unmasked text
    const auto &rules = (m_language == Language::Cpp) ? m_cppRules : m_cRules;
    const auto &typeRules = (m_language == Language::Cpp) ? m_cppTypeRules : m_cTypeRules;

    applyMaskedRules(text, rules, isMasked);
    applyMaskedRules(text, typeRules, isMasked);

    // Finally apply string and comment formatting over masked regions
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

    // Apply Python rules to unmasked text
    applyMaskedRules(text, m_pythonRules, isMasked);
    applyMaskedRules(text, m_pythonBuiltinRules, isMasked);

    // Apply string and comment formatting over masked regions
    for (const auto &span : stringCommentSpans) {
        setFormat(span.start, span.length, span.format);
    }
}

} // namespace CinderDE
