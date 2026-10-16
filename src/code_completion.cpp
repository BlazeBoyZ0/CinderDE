#include "cinderde/code_completion.h"
#include "cinderde/editor_tab.h"
#include <QRegularExpression>
#include <QTextCursor>
#include <QKeyEvent>
#include <QAbstractItemView>

namespace CinderDE {

CodeCompletionPopup::CodeCompletionPopup(CodeEditor *editor)
    : QListWidget(editor), m_editor(editor)
{
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setStyleSheet(R"(
        QListWidget {
            background-color: #252526;
            color: #d4d4d4;
            border: 1px solid #454545;
            font-family: Monospace;
            font-size: 11px;
            padding: 2px;
        }
        QListWidget::item {
            padding: 2px 6px;
            border-radius: 2px;
        }
        QListWidget::item:selected {
            background-color: #04395e;
            color: #ffffff;
        }
    )");

    hide();

    if (m_editor) {
        m_editor->installEventFilter(this);
    }

    connect(this, &QListWidget::itemActivated, this, &CodeCompletionPopup::insertCurrentSuggestion);
}

QStringList CodeCompletionPopup::getKeywordsForLanguage(Language lang) const {
    if (lang == Language::C) {
        return {
            "auto", "break", "case", "char", "const", "continue", "default", "do",
            "double", "else", "enum", "extern", "float", "for", "goto", "if",
            "inline", "int", "long", "register", "restrict", "return", "short",
            "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
            "unsigned", "void", "volatile", "while", "bool", "true", "false", "NULL", "nullptr"
        };
    } else if (lang == Language::Cpp) {
        return {
            "auto", "break", "case", "char", "const", "continue", "default", "do",
            "double", "else", "enum", "extern", "float", "for", "goto", "if",
            "inline", "int", "long", "register", "restrict", "return", "short",
            "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
            "unsigned", "void", "volatile", "while", "bool", "true", "false", "NULL", "nullptr",
            "alignas", "alignof", "and", "catch", "class", "concept", "constexpr",
            "const_cast", "decltype", "delete", "dynamic_cast", "explicit", "export",
            "friend", "mutable", "namespace", "new", "noexcept", "operator", "private",
            "protected", "public", "reinterpret_cast", "static_assert", "static_cast",
            "template", "this", "throw", "try", "typename", "using", "virtual", "std"
        };
    } else if (lang == Language::Python) {
        return {
            "False", "None", "True", "and", "as", "assert", "async", "await",
            "break", "class", "continue", "def", "del", "elif", "else", "except",
            "finally", "for", "from", "global", "if", "import", "in", "is",
            "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
            "while", "with", "yield", "print", "len", "range", "enumerate", "self", "super"
        };
    }
    return {};
}

QSet<QString> CodeCompletionPopup::extractDocumentIdentifiers() const {
    QSet<QString> identifiers;
    if (!m_editor) return identifiers;

    QString text = m_editor->toPlainText();
    static const QRegularExpression rx(R"(\b[a-zA-Z_]\w{2,}\b)");
    auto i = rx.globalMatch(text);
    while (i.hasNext()) {
        auto match = i.next();
        identifiers.insert(match.captured(0));
    }
    return identifiers;
}

void CodeCompletionPopup::updateSuggestions(const QString &prefix, Language lang) {
    m_currentPrefix = prefix;
    clear();

    if (prefix.length() < 2) {
        hide();
        return;
    }

    QSet<QString> candidates;

    // Add language keywords
    for (const QString &kw : getKeywordsForLanguage(lang)) {
        if (kw.startsWith(prefix, Qt::CaseInsensitive) && kw != prefix) {
            candidates.insert(kw);
        }
    }

    // Add document identifiers
    for (const QString &ident : extractDocumentIdentifiers()) {
        if (ident.startsWith(prefix, Qt::CaseInsensitive) && ident != prefix) {
            candidates.insert(ident);
        }
    }

    if (candidates.isEmpty()) {
        hide();
        return;
    }

    QStringList sortedCandidates = candidates.values();
    sortedCandidates.sort(Qt::CaseInsensitive);

    for (const QString &cand : sortedCandidates) {
        addItem(cand);
    }

    setCurrentRow(0);
    positionPopup();
    show();
}

void CodeCompletionPopup::positionPopup() {
    if (!m_editor) return;

    QRect cursorRect = m_editor->cursorRect();
    QPoint globalPos = m_editor->mapToParent(cursorRect.bottomRight());

    int width = 220;
    int height = qMin(160, count() * 22 + 8);

    setGeometry(globalPos.x() + 10, globalPos.y() + 5, width, height);
}

void CodeCompletionPopup::insertCurrentSuggestion() {
    if (!m_editor || !currentItem()) return;

    QString selectedText = currentItem()->text();
    QTextCursor cursor = m_editor->textCursor();

    cursor.beginEditBlock();
    if (!m_currentPrefix.isEmpty()) {
        for (int i = 0; i < m_currentPrefix.length(); ++i) {
            cursor.deletePreviousChar();
        }
    }
    cursor.insertText(selectedText);
    cursor.endEditBlock();

    m_editor->setTextCursor(cursor);
    hide();
}

bool CodeCompletionPopup::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_editor && isVisible()) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

            if (keyEvent->key() == Qt::Key_Down) {
                int nextRow = (currentRow() + 1) % count();
                setCurrentRow(nextRow);
                return true;
            } else if (keyEvent->key() == Qt::Key_Up) {
                int prevRow = (currentRow() - 1 + count()) % count();
                setCurrentRow(prevRow);
                return true;
            } else if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                insertCurrentSuggestion();
                return true;
            } else if (keyEvent->key() == Qt::Key_Escape) {
                hide();
                return true;
            }
        }
    }

    return QListWidget::eventFilter(obj, event);
}

} // namespace CinderDE
