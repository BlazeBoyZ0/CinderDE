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

    // C Keywords
    QStringList keywords = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "inline", "int", "long", "register", "restrict", "return", "short",
        "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
        "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof",
        "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary", "_Noreturn",
        "_Static_assert", "_Thread_local", "bool", "true", "false", "NULL", "nullptr"
    };

    for (const QString &keyword : keywords) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(keyword));
        rule.format = m_theme.keywordFormat;
        m_cRules.append(rule);
    }

    // Preprocessor Directives
    HighlightingRule prepRule;
    prepRule.pattern = QRegularExpression(R"(^\s*#\s*[a-zA-Z_]\w*)");
    prepRule.format = m_theme.preprocessorFormat;
    m_cRules.append(prepRule);

    // Numbers (hex, octal, binary, decimal, floating point with suffixes)
    HighlightingRule numRule;
    numRule.pattern = QRegularExpression(R"(\b(0[xX][0-9a-fA-F]+|0[bB][01]+|0[0-7]+|[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?)[uUlLfF]*\b)");
    numRule.format = m_theme.numberFormat;
    m_cRules.append(numRule);

    // Operators and Punctuation
    HighlightingRule opRule;
    opRule.pattern = QRegularExpression(R"([\+\-\*\/\%=\&\!\|\^\<\>\~]+)");
    opRule.format = m_theme.operatorFormat;
    m_cRules.append(opRule);
}

void SyntaxHighlighter::setupCppRules() {
    m_cppRules = m_cRules; // Inherit C rules

    // Additional C++ Keywords
    QStringList cppKeywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "bitand", "bitor", "bool",
        "catch", "char16_t", "char32_t", "char8_t", "class", "compl", "concept",
        "consteval", "constexpr", "constinit", "const_cast", "decltype", "delete",
        "dynamic_cast", "explicit", "export", "friend", "mutable", "namespace",
        "new", "noexcept", "not", "not_eq", "operator", "or", "or_eq", "private",
        "protected", "public", "reinterpret_cast", "requires", "static_assert",
        "static_cast", "template", "this", "thread_local", "throw", "try",
        "typeid", "typename", "using", "virtual", "wchar_t", "xor", "xor_eq",
        "co_await", "co_return", "co_yield", "std"
    };

    for (const QString &keyword : cppKeywords) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString(R"(\b%1\b)").arg(keyword));
        rule.format = m_theme.keywordFormat;
        m_cppRules.append(rule);
    }
}

void SyntaxHighlighter::setupPythonRules() {
    m_pythonRules.clear();

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

    // Python Builtins
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
        rule.format = m_theme.keywordFormat;
        m_pythonRules.append(rule);
    }

    // Decorators (@decorator_name)
    HighlightingRule decRule;
    decRule.pattern = QRegularExpression(R"(@[a-zA-Z_]\w*)");
    decRule.format = m_theme.decoratorFormat;
    m_pythonRules.append(decRule);

    // Numbers
    HighlightingRule numRule;
    numRule.pattern = QRegularExpression(R"(\b(0[xX][0-9a-fA-F]+|0[bB][01]+|0[oO][0-7]+|[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?)\b)");
    numRule.format = m_theme.numberFormat;
    m_pythonRules.append(numRule);

    // Operators
    HighlightingRule opRule;
    opRule.pattern = QRegularExpression(R"([\+\-\*\/\%=\&\!\|\^\<\>\~]+)");
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

void SyntaxHighlighter::highlightCLikeBlock(const QString &text) {
    // 1. Apply rules (keywords, numbers, operators, preprocessor)
    const auto &rules = (m_language == Language::Cpp) ? m_cppRules : m_cRules;
    for (const HighlightingRule &rule : rules) {
        QRegularExpressionMatchIterator i = rule.pattern.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // 2. Stateful lexical scan for quotes, line comments, and block comments
    // States: 0 = normal, 1 = inside multiline comment /* ... */
    int state = previousBlockState();
    if (state < 0) state = 0;

    int i = 0;
    int length = text.length();

    while (i < length) {
        if (state == 1) { // Inside multiline comment
            int endIdx = text.indexOf("*/", i);
            if (endIdx != -1) {
                int commentLen = (endIdx + 2) - i;
                setFormat(i, commentLen, m_theme.commentFormat);
                i = endIdx + 2;
                state = 0;
            } else {
                setFormat(i, length - i, m_theme.commentFormat);
                i = length;
            }
        } else { // Normal mode
            QChar ch = text.at(i);

            // Single line comment
            if (ch == '/' && i + 1 < length && text.at(i + 1) == '/') {
                setFormat(i, length - i, m_theme.commentFormat);
                break;
            }

            // Start of block comment
            if (ch == '/' && i + 1 < length && text.at(i + 1) == '*') {
                state = 1;
                int endIdx = text.indexOf("*/", i + 2);
                if (endIdx != -1) {
                    int commentLen = (endIdx + 2) - i;
                    setFormat(i, commentLen, m_theme.commentFormat);
                    i = endIdx + 2;
                    state = 0;
                } else {
                    setFormat(i, length - i, m_theme.commentFormat);
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
                setFormat(start, i - start, m_theme.stringFormat);
                continue;
            }

            i++;
        }
    }

    setCurrentBlockState(state);
}

void SyntaxHighlighter::highlightPythonBlock(const QString &text) {
    // 1. Apply rules (keywords, builtins, decorators, numbers, operators)
    for (const HighlightingRule &rule : m_pythonRules) {
        QRegularExpressionMatchIterator i = rule.pattern.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // 2. Stateful lexical scan for Python strings, triple quotes, and # comments
    // States: 0 = normal, 1 = inside """...""", 2 = inside '''...'''
    int state = previousBlockState();
    if (state < 0) state = 0;

    int i = 0;
    int length = text.length();

    while (i < length) {
        if (state == 1) { // Inside """
            int endIdx = text.indexOf("\"\"\"", i);
            if (endIdx != -1) {
                int strLen = (endIdx + 3) - i;
                setFormat(i, strLen, m_theme.stringFormat);
                i = endIdx + 3;
                state = 0;
            } else {
                setFormat(i, length - i, m_theme.stringFormat);
                i = length;
            }
        } else if (state == 2) { // Inside '''
            int endIdx = text.indexOf("'''", i);
            if (endIdx != -1) {
                int strLen = (endIdx + 3) - i;
                setFormat(i, strLen, m_theme.stringFormat);
                i = endIdx + 3;
                state = 0;
            } else {
                setFormat(i, length - i, m_theme.stringFormat);
                i = length;
            }
        } else { // Normal mode
            QChar ch = text.at(i);

            // Single line comment #
            if (ch == '#') {
                setFormat(i, length - i, m_theme.commentFormat);
                break;
            }

            // Triple quote """
            if (i + 2 < length && text.mid(i, 3) == "\"\"\"") {
                state = 1;
                int endIdx = text.indexOf("\"\"\"", i + 3);
                if (endIdx != -1) {
                    int strLen = (endIdx + 3) - i;
                    setFormat(i, strLen, m_theme.stringFormat);
                    i = endIdx + 3;
                    state = 0;
                } else {
                    setFormat(i, length - i, m_theme.stringFormat);
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
                    setFormat(i, strLen, m_theme.stringFormat);
                    i = endIdx + 3;
                    state = 0;
                } else {
                    setFormat(i, length - i, m_theme.stringFormat);
                    i = length;
                }
                continue;
            }

            // Single quote string ("..." or '...')
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
                setFormat(start, i - start, m_theme.stringFormat);
                continue;
            }

            i++;
        }
    }

    setCurrentBlockState(state);
}

} // namespace CinderDE
