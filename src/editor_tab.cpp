#include "cinderde/editor_tab.h"
#include "cinderde/code_completion.h"
#include "cinderde/preferences.h"
#include "cinderde/lsp_manager.h"
#include "cinderde/lsp_types.h"
#include <QPainter>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextCursor>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <QFileInfo>
#include <QMessageBox>
#include <QShortcut>
#include <QToolTip>
#include <QMenu>
#include <QInputDialog>
#include <QApplication>
#include <QPointer>

namespace CinderDE {

// LineNumberArea implementation
LineNumberArea::LineNumberArea(CodeEditor *editor)
    : QWidget(editor), m_editor(editor) {}

QSize LineNumberArea::sizeHint() const {
    return QSize(m_editor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event) {
    m_editor->lineNumberAreaPaintEvent(event);
}

// CodeEditor implementation
CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent),
      m_language(Language::None),
      m_completionPopup(nullptr),
      m_autoCompletionTimer(new QTimer(this)),
      m_hoverTimer(new QTimer(this)),
      m_completionRequestGeneration(0)
{
    m_lineNumberArea = new LineNumberArea(this);
    m_completionPopup = new CodeCompletionPopup(this);
    setMouseTracking(true);

    m_autoCompletionTimer->setSingleShot(true);
    m_autoCompletionTimer->setInterval(180);
    connect(m_autoCompletionTimer, &QTimer::timeout, this, &CodeEditor::triggerCompletion);
    m_hoverTimer->setSingleShot(true);
    m_hoverTimer->setInterval(500);
    connect(m_hoverTimer, &QTimer::timeout, this, [this]() {
        QTextCursor hoverCursor = cursorForPosition(m_hoverPoint);
        const int line = hoverCursor.blockNumber();
        const int col = hoverCursor.positionInBlock();
        LspClient *client = LspManager::instance().getClientForDocument(m_filePath, false);
        if (!client || !client->isRunning() || !client->capabilities().hoverProvider) return;

        QPointer<CodeEditor> guard(this);
        const QPoint point = m_hoverPoint;
        client->requestHover(m_filePath, line, col, [guard, hoverCursor, point](const QJsonValue &result, const QJsonObject &, bool isError) {
            if (!guard || isError || guard->m_hoverPoint != point) return;
            if (result.isObject()) {
                LspHover hover = LspHover::fromJson(result.toObject());
                if (!hover.contents.isEmpty()) {
                    const QRect rect = guard->cursorRect(hoverCursor);
                    QToolTip::showText(guard->mapToGlobal(rect.bottomRight()), hover.contents, guard.data());
                }
            }
        });
    });

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLineAndBracket);

    updateLineNumberAreaWidth(0);
    highlightCurrentLineAndBracket();
}

int CodeEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        digits++;
    }
    int space = 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::setEditorFontSize(int size) {
    QFont f = font();
    f.setPointSize(size);
    setFont(f);
    updateLineNumberAreaWidth(0);
}

void CodeEditor::setEditorTabSize(int size) {
    if (size < 1) size = 1;
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * size);
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLineAndBracket() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor("#2a2d2e");
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);

        highlightMatchingBracket(extraSelections);
    }

    // Add diagnostic underlines
    for (const auto &diag : m_diagnostics) {
        QTextBlock startBlock = document()->findBlockByLineNumber(diag.range.start.line);
        QTextBlock endBlock = document()->findBlockByLineNumber(diag.range.end.line);

        if (startBlock.isValid() && endBlock.isValid()) {
            QTextEdit::ExtraSelection sel;
            QTextCharFormat fmt;

            QColor color = (diag.severity == 1) ? QColor("#ff4444") :
                          (diag.severity == 2) ? QColor("#ffaa00") : QColor("#44aaff");

            fmt.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            fmt.setUnderlineColor(color);
            fmt.setToolTip(diag.message);

            sel.format = fmt;
            sel.cursor = textCursor();

            int startPos = startBlock.position() + qMin(diag.range.start.character, startBlock.length() - 1);
            int endPos = endBlock.position() + qMin(diag.range.end.character, endBlock.length() - 1);

            if (startPos == endPos && startPos < document()->characterCount()) {
                endPos = startPos + 1;
            }

            sel.cursor.setPosition(startPos);
            sel.cursor.setPosition(endPos, QTextCursor::KeepAnchor);
            extraSelections.append(sel);
        }
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::setDiagnostics(const QList<LspDiagnostic> &diagnostics) {
    m_diagnostics = diagnostics;
    highlightCurrentLineAndBracket();
}

void CodeEditor::highlightMatchingBracket(QList<QTextEdit::ExtraSelection> &extraSelections) {
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) return;

    QString text = document()->toPlainText();
    int pos = cursor.position();
    int docLen = text.length();

    if (docLen == 0) return;

    // Check character before and after cursor
    int matchPos = -1;
    int targetPos = -1;
    QChar targetChar;

    auto isBracket = [](QChar c) {
        return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}';
    };

    if (pos > 0 && isBracket(text.at(pos - 1))) {
        targetPos = pos - 1;
        targetChar = text.at(targetPos);
    } else if (pos < docLen && isBracket(text.at(pos))) {
        targetPos = pos;
        targetChar = text.at(targetPos);
    }

    if (targetPos == -1) return;

    QChar openChar, closeChar;
    bool forward = true;

    if (targetChar == '(') { openChar = '('; closeChar = ')'; forward = true; }
    else if (targetChar == ')') { openChar = '('; closeChar = ')'; forward = false; }
    else if (targetChar == '[') { openChar = '['; closeChar = ']'; forward = true; }
    else if (targetChar == ']') { openChar = '['; closeChar = ']'; forward = false; }
    else if (targetChar == '{') { openChar = '{'; closeChar = '}'; forward = true; }
    else if (targetChar == '}') { openChar = '{'; closeChar = '}'; forward = false; }

    int depth = 0;
    if (forward) {
        for (int i = targetPos; i < docLen; ++i) {
            QChar c = text.at(i);
            if (c == openChar) depth++;
            else if (c == closeChar) {
                depth--;
                if (depth == 0) {
                    matchPos = i;
                    break;
                }
            }
        }
    } else {
        for (int i = targetPos; i >= 0; --i) {
            QChar c = text.at(i);
            if (c == closeChar) depth++;
            else if (c == openChar) {
                depth--;
                if (depth == 0) {
                    matchPos = i;
                    break;
                }
            }
        }
    }

    if (matchPos != -1) {
        QTextCharFormat fmt;
        fmt.setBackground(QColor("#3e4b5b"));
        fmt.setForeground(QColor("#ffffff"));
        fmt.setFontWeight(QFont::Bold);

        QTextEdit::ExtraSelection sel1;
        sel1.format = fmt;
        sel1.cursor = textCursor();
        sel1.cursor.setPosition(targetPos);
        sel1.cursor.setPosition(targetPos + 1, QTextCursor::KeepAnchor);
        extraSelections.append(sel1);

        QTextEdit::ExtraSelection sel2;
        sel2.format = fmt;
        sel2.cursor = textCursor();
        sel2.cursor.setPosition(matchPos);
        sel2.cursor.setPosition(matchPos + 1, QTextCursor::KeepAnchor);
        extraSelections.append(sel2);
    }
}

void CodeEditor::handleSmartEnter() {
    QTextCursor cursor = textCursor();
    QTextBlock block = cursor.block();
    QString text = block.text();

    int cursorCol = cursor.positionInBlock();
    QString leftText = text.left(cursorCol);
    QString rightText = text.mid(cursorCol);

    // Compute leading whitespace of current line
    QString indent;
    for (QChar c : leftText) {
        if (c.isSpace()) indent += c;
        else break;
    }

    QString trimmedLeft = leftText.trimmed();
    bool increaseIndent = false;

    if (m_languageDef.isValid()) {
        if (m_languageDef.indentation.autoIndentOnBrace) {
            if (trimmedLeft.endsWith('{') || trimmedLeft.endsWith(':')) {
                increaseIndent = true;
            }
        }
    } else if (m_language == Language::C || m_language == Language::Cpp) {
        if (trimmedLeft.endsWith('{')) {
            increaseIndent = true;
        }
    } else if (m_language == Language::Python) {
        if (trimmedLeft.endsWith(':')) {
            increaseIndent = true;
        }
    } else {
        if (trimmedLeft.endsWith('{') || trimmedLeft.endsWith(':')) {
            increaseIndent = true;
        }
    }

    int tabWidth = m_languageDef.isValid() ? m_languageDef.indentation.tabSize : 4;
    if (tabWidth < 1) tabWidth = 4;
    QString indentStep = (m_languageDef.isValid() && !m_languageDef.indentation.useSpaces) ? "\t" : QString(tabWidth, ' ');

    cursor.beginEditBlock();
    cursor.insertBlock();

    if (increaseIndent) {
        if (trimmedLeft.endsWith('{') && rightText.trimmed().startsWith('}')) {
            // Block expansion: {\n    cursor\n}
            cursor.insertText(indent + indentStep);
            int midPos = cursor.position();
            cursor.insertBlock();
            cursor.insertText(indent);
            cursor.setPosition(midPos);
            cursor.endEditBlock();
            setTextCursor(cursor);
            return;
        } else {
            indent += indentStep;
        }
    }

    cursor.insertText(indent);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

void CodeEditor::handleSmartBackspace(QKeyEvent *e) {
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        QPlainTextEdit::keyPressEvent(e);
        return;
    }

    int pos = cursor.position();
    int posInBlock = cursor.positionInBlock();
    QString text = cursor.block().text();

    // Check if cursor is inside empty pair like (), [], {}, "", ''
    if (posInBlock > 0 && posInBlock < text.length()) {
        QChar left = text.at(posInBlock - 1);
        QChar right = text.at(posInBlock);
        if ((left == '(' && right == ')') ||
            (left == '[' && right == ']') ||
            (left == '{' && right == '}') ||
            (left == '"' && right == '"') ||
            (left == '\'' && right == '\'')) {
            cursor.beginEditBlock();
            cursor.deleteChar();
            cursor.deletePreviousChar();
            cursor.endEditBlock();
            return;
        }
    }

    // Check leading spaces unindent
    QString leading = text.left(posInBlock);
    if (!leading.isEmpty() && leading.trimmed().isEmpty() && leading.contains(' ')) {
        int spaces = leading.length();
        int tabWidth = m_languageDef.isValid() ? m_languageDef.indentation.tabSize : 4;
        if (tabWidth < 1) tabWidth = 4;
        int deleteCount = spaces % tabWidth;
        if (deleteCount == 0) deleteCount = tabWidth;
        deleteCount = qMin(deleteCount, spaces);

        cursor.beginEditBlock();
        for (int i = 0; i < deleteCount; ++i) {
            cursor.deletePreviousChar();
        }
        cursor.endEditBlock();
        return;
    }

    QPlainTextEdit::keyPressEvent(e);
}

bool CodeEditor::handleBracketPairing(QKeyEvent *e) {
    QString typed = e->text();
    if (typed.isEmpty()) return false;

    QChar ch = typed.at(0);
    if (ch != '(' && ch != ')' && ch != '[' && ch != ']' &&
        ch != '{' && ch != '}' && ch != '"' && ch != '\'') {
        return false;
    }

    QTextCursor cursor = textCursor();
    QString text = document()->toPlainText();
    int pos = cursor.position();

    // 1. If text selection exists, wrap selection
    if (cursor.hasSelection()) {
        QChar openCh = ch;
        QChar closeCh = ch;
        if (ch == '(' || ch == ')') { openCh = '('; closeCh = ')'; }
        else if (ch == '[' || ch == ']') { openCh = '['; closeCh = ']'; }
        else if (ch == '{' || ch == '}') { openCh = '{'; closeCh = '}'; }

        int selStart = cursor.selectionStart();
        int selEnd = cursor.selectionEnd();
        QString selectedText = cursor.selectedText();

        cursor.beginEditBlock();
        cursor.setPosition(selStart);
        cursor.insertText(QString(openCh));
        cursor.setPosition(selStart + 1);
        cursor.insertText(selectedText);
        cursor.insertText(QString(closeCh));

        cursor.setPosition(selStart + 1);
        cursor.setPosition(selEnd + 1, QTextCursor::KeepAnchor);
        cursor.endEditBlock();
        setTextCursor(cursor);
        return true;
    }

    // 2. Skip over if typing closing character or quote that already exists immediately at cursor
    if (pos < text.length()) {
        QChar nextCh = text.at(pos);
        if (ch == nextCh && (ch == ')' || ch == ']' || ch == '}' || ch == '"' || ch == '\'')) {
            cursor.movePosition(QTextCursor::Right);
            setTextCursor(cursor);
            return true;
        }
    }

    // 3. Auto unindent closing brace '}' if alone on line
    if (ch == '}' && ((m_language == Language::C || m_language == Language::Cpp) || (m_languageDef.isValid() && m_languageDef.indentation.autoIndentOnBrace))) {
        QString lineText = cursor.block().text();
        int posInBlock = cursor.positionInBlock();
        QString leftText = lineText.left(posInBlock);
        if (!leftText.isEmpty() && leftText.trimmed().isEmpty()) {
            int tabWidth = m_languageDef.isValid() ? m_languageDef.indentation.tabSize : 4;
            if (tabWidth < 1) tabWidth = 4;
            if (leftText.length() >= tabWidth) {
                cursor.beginEditBlock();
                for (int i = 0; i < tabWidth; ++i) cursor.deletePreviousChar();
                cursor.insertText("}");
                cursor.endEditBlock();
                return true;
            }
        }
    }

    // 4. Insert opening + closing pair for opening brackets/quotes
    QChar openCh = ch;
    QChar closeCh;
    if (ch == '(') closeCh = ')';
    else if (ch == '[') closeCh = ']';
    else if (ch == '{') closeCh = '}';
    else if (ch == '"' || ch == '\'') closeCh = ch;

    if (!closeCh.isNull()) {
        // Do not auto-pair quote if previous char is identifier (e.g. don't pair inside don't -> don''t)
        if ((ch == '"' || ch == '\'') && pos > 0 && text.at(pos - 1).isLetterOrNumber()) {
            return false;
        }

        // Do not auto-pair if next char is an identifier char
        if (pos < text.length() && text.at(pos).isLetterOrNumber()) {
            return false;
        }

        cursor.beginEditBlock();
        cursor.insertText(QString(openCh) + QString(closeCh));
        cursor.movePosition(QTextCursor::Left);
        cursor.endEditBlock();
        setTextCursor(cursor);
        return true;
    }

    return false;
}

void CodeEditor::toggleComment() {
    QTextCursor cursor = textCursor();
    int startBlock = document()->findBlock(cursor.selectionStart()).blockNumber();
    int endBlock = document()->findBlock(cursor.selectionEnd()).blockNumber();

    QString commentPrefix = "// ";
    if (m_languageDef.isValid() && !m_languageDef.comments.linePrefix.isEmpty()) {
        commentPrefix = m_languageDef.comments.linePrefix + " ";
    } else if (m_language == Language::Python) {
        commentPrefix = "# ";
    }

    // Check if all non-empty lines in selection are already commented
    bool allCommented = true;
    for (int i = startBlock; i <= endBlock; ++i) {
        QTextBlock block = document()->findBlockByNumber(i);
        QString line = block.text().trimmed();
        if (!line.isEmpty()) {
            if (!line.startsWith(commentPrefix.trimmed())) {
                allCommented = false;
                break;
            }
        }
    }

    cursor.beginEditBlock();
    for (int i = startBlock; i <= endBlock; ++i) {
        QTextBlock block = document()->findBlockByNumber(i);
        QString text = block.text();

        if (allCommented) {
            int prefixIdx = text.indexOf(commentPrefix.trimmed());
            if (prefixIdx != -1) {
                QTextCursor lineCursor(block);
                lineCursor.setPosition(block.position() + prefixIdx);
                int removeLen = commentPrefix.trimmed().length();
                if (prefixIdx + removeLen < text.length() && text.at(prefixIdx + removeLen) == ' ') {
                    removeLen++;
                }
                lineCursor.setPosition(block.position() + prefixIdx + removeLen, QTextCursor::KeepAnchor);
                lineCursor.removeSelectedText();
            }
        } else {
            // Find leading indent level
            int firstNonSpace = 0;
            while (firstNonSpace < text.length() && text.at(firstNonSpace).isSpace()) {
                firstNonSpace++;
            }
            QTextCursor lineCursor(block);
            lineCursor.setPosition(block.position() + firstNonSpace);
            lineCursor.insertText(commentPrefix);
        }
    }
    cursor.endEditBlock();
}

void CodeEditor::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_F12 && e->modifiers() == Qt::NoModifier) {
        triggerGoToDefinition();
        return;
    }
    if (e->key() == Qt::Key_F12 && e->modifiers() == Qt::ShiftModifier) {
        triggerFindReferences();
        return;
    }
    if (e->key() == Qt::Key_F2 && e->modifiers() == Qt::NoModifier) {
        triggerRename();
        return;
    }
    if ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Space) {
        triggerCompletion();
        return;
    }

    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        ++m_completionRequestGeneration;
        if (m_autoCompletionTimer) m_autoCompletionTimer->stop();
        handleSmartEnter();
        if (m_completionPopup) m_completionPopup->hide();
        return;
    }

    if (e->key() == Qt::Key_Backspace) {
        ++m_completionRequestGeneration;
        if (m_autoCompletionTimer) m_autoCompletionTimer->stop();
        handleSmartBackspace(e);
        LspClient *backspaceClient = LspManager::instance().getClientForDocument(m_filePath, false);
        const bool backspaceHasLsp = backspaceClient && backspaceClient->isRunning()
            && backspaceClient->capabilities().completionProvider;
        if (m_completionPopup && !backspaceHasLsp) {
            QTextCursor cursor = textCursor();
            QString line = cursor.block().text().left(cursor.positionInBlock());
            static const QRegularExpression wordRx(R"(\b[a-zA-Z_]\w*$)");
            auto match = wordRx.match(line);
            QString prefix = match.hasMatch() ? match.captured(0) : "";
            if (m_languageDef.isValid()) {
                m_completionPopup->updateSuggestions(prefix, m_languageDef);
            } else {
                m_completionPopup->updateSuggestions(prefix, m_language);
            }
        } else if (m_completionPopup) {
            m_completionPopup->hide();
        }
        return;
    }

    if (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::ShiftModifier) {
        if (handleBracketPairing(e)) {
            ++m_completionRequestGeneration;
            if (m_autoCompletionTimer) m_autoCompletionTimer->stop();
            if (m_completionPopup) m_completionPopup->hide();
            return;
        }
    }

    QPlainTextEdit::keyPressEvent(e);

    // Keep built-in word suggestions for the lightweight no-LSP mode. When an
    // LSP is active, let its response own the popup instead of flashing unrelated words.
    LspClient *activeCompletionClient = LspManager::instance().getClientForDocument(m_filePath, false);
    const bool hasRunningCompletionLsp = activeCompletionClient && activeCompletionClient->isRunning()
        && activeCompletionClient->capabilities().completionProvider;
    if (!hasRunningCompletionLsp && m_completionPopup && !e->text().isEmpty() && (e->text().at(0).isLetterOrNumber() || e->text().at(0) == '_')) {
        QTextCursor cursor = textCursor();
        QString line = cursor.block().text().left(cursor.positionInBlock());
        static const QRegularExpression wordRx(R"(\b[a-zA-Z_]\w*$)");
        auto match = wordRx.match(line);
        if (match.hasMatch()) {
            QString prefix = match.captured(0);
            if (m_languageDef.isValid()) {
                m_completionPopup->updateSuggestions(prefix, m_languageDef);
            } else {
                m_completionPopup->updateSuggestions(prefix, m_language);
            }
        } else {
            m_completionPopup->hide();
        }
    } else if (m_completionPopup && e->key() != Qt::Key_Up && e->key() != Qt::Key_Down) {
        m_completionPopup->hide();
    }

    // Invalidate any in-flight completion as soon as the editor changes, so
    // an older response cannot replace suggestions for the current cursor.
    ++m_completionRequestGeneration;

    // Debounce ordinary typing so language servers are not queried for every
    // keystroke. Trigger characters such as '.' request completion immediately.
    if (m_autoCompletionTimer && !e->text().isEmpty()) {
        const QString typed = e->text();
        const QChar ch = typed.back();
        if (ch.isLetterOrNumber() || ch == '_') {
            m_autoCompletionTimer->start();
        } else if (ch == '.') {
            m_autoCompletionTimer->start(0);
        } else if (ch == ':') {
            const QString line = textCursor().block().text().left(textCursor().positionInBlock());
            if (line.endsWith("::")) {
                m_autoCompletionTimer->start(0);
            } else {
                m_autoCompletionTimer->stop();
            }
        } else {
            m_autoCompletionTimer->stop();
        }
    }
}

void CodeEditor::triggerCompletion() {
    if (m_autoCompletionTimer) m_autoCompletionTimer->stop();
    const int requestGeneration = ++m_completionRequestGeneration;
    QTextCursor cursor = textCursor();
    int line = cursor.blockNumber();
    int col = cursor.columnNumber();

    QString lineText = cursor.block().text().left(cursor.positionInBlock());
    static const QRegularExpression wordRx(R"(\b[a-zA-Z_]\w*$)");
    auto match = wordRx.match(lineText);
    QString prefix = match.hasMatch() ? match.captured(0) : "";

    LspClient *client = LspManager::instance().getClientForDocument(m_filePath, false);
    if (client && client->isRunning()) {
        QPointer<CodeEditor> guard(this);
        const QString beforeCursor = cursor.block().text().left(cursor.positionInBlock());
        QString triggerCharacter;
        if (beforeCursor.endsWith(".")) triggerCharacter = ".";
        else if (beforeCursor.endsWith("::")) triggerCharacter = ":";
        client->requestCompletion(m_filePath, line, col, [guard, prefix, requestGeneration](const QJsonValue &result, const QJsonObject &, bool isError) {
            if (!guard || guard->m_completionRequestGeneration != requestGeneration) return;
            if (!isError && !result.isNull()) {
                QList<LspCompletionItem> items;
                QJsonArray itemsArray;
                if (result.isObject() && result.toObject().contains("items") && result.toObject().value("items").isArray()) {
                    itemsArray = result.toObject().value("items").toArray();
                } else if (result.isArray()) {
                    itemsArray = result.toArray();
                }
                for (const QJsonValue &v : itemsArray) {
                    items.append(LspCompletionItem::fromJson(v.toObject()));
                }
                if (!items.isEmpty() && guard->m_completionPopup) {
                    guard->m_completionPopup->updateLspSuggestions(prefix, items);
                    return;
                }
            }
            if (guard->m_completionPopup) {
                if (guard->m_languageDef.isValid()) {
                    guard->m_completionPopup->updateSuggestions(prefix, guard->m_languageDef);
                } else {
                    guard->m_completionPopup->updateSuggestions(prefix, guard->m_language);
                }
            }
        }, triggerCharacter);
    } else {
        if (m_completionPopup) {
            if (m_languageDef.isValid()) {
                m_completionPopup->updateSuggestions(prefix, m_languageDef);
            } else {
                m_completionPopup->updateSuggestions(prefix, m_language);
            }
        }
    }
}

void CodeEditor::cancelPendingCompletion() {
    ++m_completionRequestGeneration;
    if (m_autoCompletionTimer) m_autoCompletionTimer->stop();
}

void CodeEditor::triggerHover() {
    QTextCursor cursor = textCursor();
    int line = cursor.blockNumber();
    int col = cursor.columnNumber();

    LspClient *client = LspManager::instance().getClientForDocument(m_filePath, false);
    if (client && client->isRunning()) {
        QPointer<CodeEditor> guard(this);
        client->requestHover(m_filePath, line, col, [guard](const QJsonValue &result, const QJsonObject &, bool isError) {
            if (!guard) return;
            if (!isError && result.isObject()) {
                LspHover hover = LspHover::fromJson(result.toObject());
                if (!hover.contents.isEmpty()) {
                    QToolTip::showText(guard->mapToGlobal(guard->cursorRect().bottomRight()), hover.contents, guard.data());
                }
            }
        });
    }
}

void CodeEditor::triggerGoToDefinition() {
    QTextCursor cursor = textCursor();
    int line = cursor.blockNumber();
    int col = cursor.columnNumber();

    LspClient *client = LspManager::instance().getClientForDocument(m_filePath, false);
    if (client && client->isRunning()) {
        QPointer<CodeEditor> guard(this);
        client->requestDefinition(m_filePath, line, col, [guard](const QJsonValue &result, const QJsonObject &, bool isError) {
            if (!guard || isError) return;

            QList<LspLocation> locations;
            if (result.isArray()) {
                for (const QJsonValue &v : result.toArray()) {
                    locations.append(LspLocation::fromJson(v.toObject()));
                }
            } else if (result.isObject()) {
                locations.append(LspLocation::fromJson(result.toObject()));
            }

            if (!locations.isEmpty()) {
                const auto &loc = locations.first();
                QString targetPath = uriToPath(loc.uri);
                int targetLine = loc.range.start.line + 1;
                int targetCol = loc.range.start.character + 1;

                QWidget *top = guard->window();
                if (top) {
                    QMetaObject::invokeMethod(top, "navigateToDiagnostic",
                        Q_ARG(QString, targetPath),
                        Q_ARG(int, targetLine),
                        Q_ARG(int, targetCol));
                }
            }
        });
    }
}

void CodeEditor::triggerFindReferences() {
    QTextCursor cursor = textCursor();
    int line = cursor.blockNumber();
    int col = cursor.columnNumber();

    LspClient *client = LspManager::instance().getClientForDocument(m_filePath, false);
    if (client && client->isRunning()) {
        QPointer<CodeEditor> guard(this);
        client->requestReferences(m_filePath, line, col, [guard](const QJsonValue &result, const QJsonObject &, bool isError) {
            if (!guard || isError) return;

            QList<LspLocation> locations;
            if (result.isArray()) {
                for (const QJsonValue &v : result.toArray()) {
                    locations.append(LspLocation::fromJson(v.toObject()));
                }
            }

            if (!locations.isEmpty()) {
                QString logMsg = QString("Found %1 reference(s):\n").arg(locations.size());
                for (const auto &loc : locations) {
                    logMsg += QString("%1:%2:%3: Reference\n")
                        .arg(uriToPath(loc.uri))
                        .arg(loc.range.start.line + 1)
                        .arg(loc.range.start.character + 1);
                }
                emit LspManager::instance().logMessage(logMsg);
            }
        });
    }
}

void CodeEditor::triggerRename() {
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    QString currentName = cursor.selectedText();

    bool ok = false;
    QString newName = QInputDialog::getText(this, "Rename Symbol", "New symbol name:", QLineEdit::Normal, currentName, &ok);
    if (!ok || newName.trimmed().isEmpty() || newName == currentName) {
        return;
    }

    int line = cursor.blockNumber();
    int col = cursor.columnNumber();

    LspClient *client = LspManager::instance().getClientForDocument(m_filePath, false);
    if (client && client->isRunning()) {
        QPointer<CodeEditor> guard(this);
        client->requestRename(m_filePath, line, col, newName.trimmed(), [guard](const QJsonValue &resultVal, const QJsonObject &error, bool isError) {
            if (!guard) return;
            if (isError) {
                QString errMsg = error.value("message").toString("Rename failed");
                QMessageBox::warning(guard.data(), "Rename Failed", errMsg);
                return;
            }

            QJsonObject result = resultVal.isObject() ? resultVal.toObject() : QJsonObject();
            LspWorkspaceEdit edit = LspWorkspaceEdit::fromJson(result);
            if (edit.changes.isEmpty()) {
                QMessageBox::information(guard.data(), "Rename Symbol", "No edits returned by language server.");
                return;
            }

            for (auto it = edit.changes.begin(); it != edit.changes.end(); ++it) {
                QString path = uriToPath(it.key());
                QList<LspTextEdit> edits = it.value();

                std::sort(edits.begin(), edits.end(), [](const LspTextEdit &a, const LspTextEdit &b) {
                    if (a.range.start.line != b.range.start.line)
                        return a.range.start.line > b.range.start.line;
                    return a.range.start.character > b.range.start.character;
                });

                if (path == guard->m_filePath) {
                    QTextCursor c(guard->document());
                    c.beginEditBlock();
                    for (const auto &e : edits) {
                        QTextBlock sb = guard->document()->findBlockByLineNumber(e.range.start.line);
                        QTextBlock eb = guard->document()->findBlockByLineNumber(e.range.end.line);
                        if (sb.isValid() && eb.isValid()) {
                            int sp = sb.position() + qMin(e.range.start.character, sb.length() - 1);
                            int ep = eb.position() + qMin(e.range.end.character, eb.length() - 1);
                            c.setPosition(sp);
                            c.setPosition(ep, QTextCursor::KeepAnchor);
                            c.insertText(e.newText);
                        }
                    }
                    c.endEditBlock();
                } else {
                    QFile file(path);
                    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QString text = file.readAll();
                        file.close();
                        QStringList lines = text.split('\n');
                        for (const auto &e : edits) {
                            if (e.range.start.line < lines.size() && e.range.end.line < lines.size()) {
                                QString &lineStr = lines[e.range.start.line];
                                int len = e.range.end.character - e.range.start.character;
                                lineStr.replace(e.range.start.character, len, e.newText);
                            }
                        }
                        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                            QTextStream out(&file);
                            out << lines.join('\n');
                            file.close();
                        }
                    }
                }
            }
        });
    }
}

void CodeEditor::contextMenuEvent(QContextMenuEvent *event) {
    QMenu *menu = createStandardContextMenu();
    menu->addSeparator();

    LspClient *client = LspManager::instance().getClientForDocument(m_filePath, false);
    bool hasLsp = client && client->isRunning();

    QAction *defAct = menu->addAction("Go to Definition", QKeySequence(Qt::Key_F12), this, &CodeEditor::triggerGoToDefinition);
    defAct->setEnabled(hasLsp && client->capabilities().definitionProvider);

    QAction *refAct = menu->addAction("Find References", QKeySequence(Qt::SHIFT | Qt::Key_F12), this, &CodeEditor::triggerFindReferences);
    refAct->setEnabled(hasLsp && client->capabilities().referencesProvider);

    QAction *renameAct = menu->addAction("Rename Symbol", QKeySequence(Qt::Key_F2), this, &CodeEditor::triggerRename);
    renameAct->setEnabled(hasLsp && client->capabilities().renameProvider);

    QAction *hoverAct = menu->addAction("Show Hover Info", this, &CodeEditor::triggerHover);
    hoverAct->setEnabled(hasLsp && client->capabilities().hoverProvider);

    menu->exec(event->globalPos());
    delete menu;
}

void CodeEditor::mouseMoveEvent(QMouseEvent *event) {
    QPlainTextEdit::mouseMoveEvent(event);
    m_hoverPoint = event->pos();
    QToolTip::hideText();
    if (m_hoverTimer) m_hoverTimer->start();
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor("#1e1e1e"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            if (blockNumber == textCursor().blockNumber()) {
                painter.setPen(QColor("#e0e0e0")); // Current line number highlight
            } else {
                painter.setPen(QColor("#606060")); // Muted line numbers
            }
            painter.setFont(font());
            painter.drawText(0, top, m_lineNumberArea->width() - 6, fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        blockNumber++;
    }

    // Right border line for line number margin
    painter.setPen(QColor("#333333"));
    painter.drawLine(event->rect().topRight(), event->rect().bottomRight());
}

// EditorTab implementation
EditorTab::EditorTab(QWidget *parent)
    : QWidget(parent), m_fontSize(11), m_tabSize(4), m_replaceMode(false)
{
    m_editor = new CodeEditor(this);
    m_highlighter = new SyntaxHighlighter(m_editor->document());

    // Default font
    QFont font("Monospace");
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(m_fontSize);
    m_editor->setFont(font);
    m_editor->setEditorTabSize(m_tabSize);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    createFindReplaceBar();
    mainLayout->addWidget(m_findReplaceWidget);
    mainLayout->addWidget(m_editor);

    connect(m_editor->document(), &QTextDocument::contentsChanged, this, &EditorTab::onTextChanged);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, &EditorTab::onCursorPositionChanged);

    connect(&LspManager::instance(), &LspManager::diagnosticsUpdated, this, [this](const QString &filePath, const QList<LspDiagnostic> &diags) {
        if (filePath == m_filePath) {
            m_editor->setDiagnostics(diags);
        }
    });

    m_typeInfo = FileTypeDetector::detectType("");
    updateHighlighterLanguage();
}

EditorTab::~EditorTab() {
    if (!m_filePath.isEmpty()) {
        LspManager::instance().notifyDocumentClosed(m_filePath);
    }
}

void EditorTab::updateHighlighterLanguage() {
    LanguageDefinition def = LanguageRegistry::instance().definitionForExtension(m_typeInfo.extension);
    if (m_highlighter) {
        if (def.isValid()) {
            m_highlighter->setLanguageDefinition(def);
        } else {
            Language lang = SyntaxHighlighter::languageFromExtension(m_typeInfo.extension);
            m_highlighter->setLanguage(lang);
        }
    }
    if (m_editor) {
        if (def.isValid()) {
            m_editor->setLanguageDefinition(def);
            m_editor->setLanguage(m_highlighter ? m_highlighter->language() : Language::None);
            if (def.indentation.tabSize > 0) {
                m_editor->setEditorTabSize(def.indentation.tabSize);
            }
        } else {
            Language lang = SyntaxHighlighter::languageFromExtension(m_typeInfo.extension);
            m_editor->setLanguage(lang);
        }
    }
}

void EditorTab::createFindReplaceBar() {
    m_findReplaceWidget = new QWidget(this);
    m_findReplaceWidget->setStyleSheet(
        "QWidget { background-color: #252526; color: #cccccc; font-size: 12px; }"
        "QLineEdit { background-color: #3c3c3c; color: #ffffff; border: 1px solid #555; border-radius: 2px; padding: 3px; }"
        "QPushButton { background-color: #3c3c3c; color: #ffffff; border: 1px solid #555; border-radius: 2px; padding: 3px 8px; }"
        "QPushButton:hover { background-color: #4c4c4c; }"
        "QPushButton:pressed { background-color: #ff6b35; color: #ffffff; }"
    );

    QVBoxLayout *barLayout = new QVBoxLayout(m_findReplaceWidget);
    barLayout->setContentsMargins(6, 4, 6, 4);
    barLayout->setSpacing(4);

    QHBoxLayout *findLayout = new QHBoxLayout();
    m_findEdit = new QLineEdit(this);
    m_findEdit->setPlaceholderText("Find...");
    m_findNextBtn = new QPushButton("Next", this);
    m_findPrevBtn = new QPushButton("Previous", this);
    m_caseSensitiveCB = new QCheckBox("Match Case", this);
    m_matchCountLabel = new QLabel("", this);
    m_closeFindBtn = new QPushButton("✕", this);
    m_closeFindBtn->setFixedWidth(24);

    findLayout->addWidget(new QLabel("Find:", this));
    findLayout->addWidget(m_findEdit, 1);
    findLayout->addWidget(m_findNextBtn);
    findLayout->addWidget(m_findPrevBtn);
    findLayout->addWidget(m_caseSensitiveCB);
    findLayout->addWidget(m_matchCountLabel);
    findLayout->addWidget(m_closeFindBtn);

    QHBoxLayout *replaceLayout = new QHBoxLayout();
    m_replaceEdit = new QLineEdit(this);
    m_replaceEdit->setPlaceholderText("Replace with...");
    m_replaceBtn = new QPushButton("Replace", this);
    m_replaceAllBtn = new QPushButton("Replace All", this);

    replaceLayout->addWidget(new QLabel("Replace:", this));
    replaceLayout->addWidget(m_replaceEdit, 1);
    replaceLayout->addWidget(m_replaceBtn);
    replaceLayout->addWidget(m_replaceAllBtn);

    barLayout->addLayout(findLayout);
    barLayout->addLayout(replaceLayout);

    connect(m_findEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateMatchCount(); });
    connect(m_findEdit, &QLineEdit::returnPressed, this, &EditorTab::findNext);
    connect(m_findNextBtn, &QPushButton::clicked, this, &EditorTab::findNext);
    connect(m_findPrevBtn, &QPushButton::clicked, this, &EditorTab::findPrevious);
    connect(m_replaceBtn, &QPushButton::clicked, this, &EditorTab::replaceOne);
    connect(m_replaceAllBtn, &QPushButton::clicked, this, &EditorTab::replaceAll);
    connect(m_closeFindBtn, &QPushButton::clicked, this, &EditorTab::hideFindReplaceBar);
    connect(m_caseSensitiveCB, &QCheckBox::toggled, this, [this](bool) { updateMatchCount(); });

    m_findReplaceWidget->hide();
}

void EditorTab::showFindReplaceBar(bool showReplace) {
    m_replaceMode = showReplace;
    m_findReplaceWidget->layout()->itemAt(1)->layout()->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < m_findReplaceWidget->layout()->itemAt(1)->layout()->count(); ++i) {
        QWidget *w = m_findReplaceWidget->layout()->itemAt(1)->layout()->itemAt(i)->widget();
        if (w) w->setVisible(showReplace);
    }
    m_findReplaceWidget->show();
    m_findEdit->setFocus();
    m_findEdit->selectAll();
    updateMatchCount();
}

void EditorTab::hideFindReplaceBar() {
    m_findReplaceWidget->hide();
    m_editor->setFocus();
}

void EditorTab::updateMatchCount() {
    QString query = m_findEdit->text();
    if (query.isEmpty()) {
        m_matchCountLabel->setText("");
        return;
    }

    int count = 0;
    QString text = m_editor->toPlainText();
    Qt::CaseSensitivity cs = m_caseSensitiveCB->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int pos = 0;
    while ((pos = text.indexOf(query, pos, cs)) != -1) {
        count++;
        pos += query.length();
    }

    m_matchCountLabel->setText(QString("%1 found").arg(count));
}

void EditorTab::findNext() {
    QString query = m_findEdit->text();
    if (query.isEmpty()) return;

    QTextDocument::FindFlags flags;
    if (m_caseSensitiveCB->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }

    bool found = m_editor->find(query, flags);
    if (!found) {
        // Wrap around from beginning
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::Start);
        m_editor->setTextCursor(cursor);
        m_editor->find(query, flags);
    }
}

void EditorTab::findPrevious() {
    QString query = m_findEdit->text();
    if (query.isEmpty()) return;

    QTextDocument::FindFlags flags = QTextDocument::FindBackward;
    if (m_caseSensitiveCB->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }

    bool found = m_editor->find(query, flags);
    if (!found) {
        // Wrap around from end
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_editor->setTextCursor(cursor);
        m_editor->find(query, flags);
    }
}

void EditorTab::replaceOne() {
    QString query = m_findEdit->text();
    if (query.isEmpty()) return;

    QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection() &&
        cursor.selectedText().compare(query, m_caseSensitiveCB->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive) == 0) {
        cursor.insertText(m_replaceEdit->text());
    }
    findNext();
    updateMatchCount();
}

void EditorTab::replaceAll() {
    QString query = m_findEdit->text();
    if (query.isEmpty()) return;

    QString replaceText = m_replaceEdit->text();
    QTextCursor cursor = m_editor->textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start);
    m_editor->setTextCursor(cursor);

    QTextDocument::FindFlags flags;
    if (m_caseSensitiveCB->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }

    int replacements = 0;
    while (m_editor->find(query, flags)) {
        QTextCursor c = m_editor->textCursor();
        c.insertText(replaceText);
        replacements++;
    }
    cursor.endEditBlock();
    updateMatchCount();
}

bool EditorTab::openFile(const QString &filePath, bool showDialog) {
    if (FileTypeDetector::isLikelyBinary(filePath)) {
        if (showDialog) {
            QMessageBox::StandardButton res = QMessageBox::warning(
                this, "Binary File Warning",
                QString("The file '%1' appears to be a binary file. Opening binary files may display invalid text.\n\nDo you still want to open it?").arg(QFileInfo(filePath).fileName()),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (res != QMessageBox::Yes) return false;
        } else {
            return false;
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (showDialog) {
            QMessageBox::critical(this, "Error Opening File", QString("Failed to open file:\n%1\nError: %2").arg(filePath, file.errorString()));
        }
        return false;
    }

    QTextStream in(&file);
    const QString loadedText = in.readAll();
    const bool readFailed = in.status() != QTextStream::Ok ||
                            file.error() != QFileDevice::NoError;
    const QString readError = file.errorString();
    file.close();

    if (readFailed) {
        if (showDialog) {
            QMessageBox::critical(this, "Error Opening File",
                QString("Failed to read file:\n%1\nError: %2").arg(filePath, readError));
        }
        return false;
    }

    m_editor->setPlainText(loadedText);
    m_filePath = filePath;
    m_editor->setFilePath(filePath);
    m_editor->document()->setModified(false);
    m_typeInfo = FileTypeDetector::detectType(filePath);
    updateHighlighterLanguage();

    LspManager::instance().notifyDocumentOpened(m_filePath, m_editor->toPlainText());

    emit fileTypeChanged(m_typeInfo);
    emit modificationChanged(false);
    onCursorPositionChanged();

    return true;
}

void EditorTab::setFilePath(const QString &filePath) {
    m_filePath = filePath;
    m_typeInfo = FileTypeDetector::detectType(filePath);
    updateHighlighterLanguage();
    emit fileTypeChanged(m_typeInfo);
}

void EditorTab::clearFilePathAndMarkModified() {
    m_filePath.clear();
    m_typeInfo = FileTypeDetector::detectType("");
    updateHighlighterLanguage();
    m_editor->document()->setModified(true);
    emit modificationChanged(true);
}

bool EditorTab::save(bool showDialog) {
    if (isUntitled()) {
        return false; // Requires saveAs
    }

    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (showDialog) {
            QMessageBox::critical(this, "Error Saving File", QString("Failed to save file:\n%1\nError: %2").arg(m_filePath, file.errorString()));
        }
        return false;
    }

    QTextStream out(&file);
    out << m_editor->toPlainText();
    out.flush();

    if (out.status() != QTextStream::Ok || file.error() != QFileDevice::NoError) {
        const QString error = file.errorString();
        file.cancelWriting();
        if (showDialog) {
            QMessageBox::critical(this, "Error Saving File", QString("Failed to write to file:\n%1\nError: %2").arg(m_filePath, error));
        }
        return false;
    }

    if (!file.commit()) {
        if (showDialog) {
            QMessageBox::critical(this, "Error Saving File", QString("Failed to commit file:\n%1\nError: %2").arg(m_filePath, file.errorString()));
        }
        return false;
    }

    m_editor->document()->setModified(false);
    LspManager::instance().notifyDocumentSaved(m_filePath, m_editor->toPlainText());
    emit modificationChanged(false);

    return true;
}

bool EditorTab::saveAs(const QString &filePath, bool showDialog) {
    if (filePath.isEmpty()) return false;

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (showDialog) {
            QMessageBox::critical(this, "Error Saving File", QString("Failed to save file:\n%1\nError: %2").arg(filePath, file.errorString()));
        }
        return false;
    }

    QTextStream out(&file);
    out << m_editor->toPlainText();
    out.flush();

    if (out.status() != QTextStream::Ok || file.error() != QFileDevice::NoError) {
        const QString error = file.errorString();
        file.cancelWriting();
        if (showDialog) {
            QMessageBox::critical(this, "Error Saving File", QString("Failed to write to file:\n%1\nError: %2").arg(filePath, error));
        }
        return false;
    }

    if (!file.commit()) {
        if (showDialog) {
            QMessageBox::critical(this, "Error Saving File", QString("Failed to commit file:\n%1\nError: %2").arg(filePath, file.errorString()));
        }
        return false;
    }

    m_filePath = filePath;
    m_editor->setFilePath(filePath);
    m_typeInfo = FileTypeDetector::detectType(filePath);
    updateHighlighterLanguage();
    m_editor->document()->setModified(false);

    LspManager::instance().notifyDocumentSaved(m_filePath, m_editor->toPlainText());

    emit fileTypeChanged(m_typeInfo);
    emit modificationChanged(false);

    return true;
}

QString EditorTab::fileName() const {
    if (m_filePath.isEmpty()) {
        return "Untitled";
    }
    return QFileInfo(m_filePath).fileName();
}

bool EditorTab::isModified() const {
    return m_editor->document()->isModified();
}

void EditorTab::setFontSize(int size) {
    if (size < 6) size = 6;
    if (size > 72) size = 72;
    m_fontSize = size;
    m_editor->setEditorFontSize(size);
    m_editor->setEditorTabSize(m_tabSize);
}

void EditorTab::setTabSize(int size) {
    if (size < 1) size = 1;
    if (size > 16) size = 16;
    m_tabSize = size;
    m_editor->setEditorTabSize(size);
}

void EditorTab::zoomIn(int range) {
    setFontSize(m_fontSize + range);
}

void EditorTab::zoomOut(int range) {
    setFontSize(m_fontSize - range);
}

void EditorTab::resetFontSize() {
    setFontSize(Preferences::DEFAULT_FONT_SIZE);
    Preferences::instance().setFontSize(Preferences::DEFAULT_FONT_SIZE);
}

void EditorTab::setWordWrap(bool wrap) {
    m_editor->setLineWrapMode(wrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

bool EditorTab::wordWrap() const {
    return m_editor->lineWrapMode() != QPlainTextEdit::NoWrap;
}

void EditorTab::onTextChanged() {
    LspManager::instance().notifyDocumentChanged(m_filePath, m_editor->toPlainText());
    emit modificationChanged(m_editor->document()->isModified());
}

void EditorTab::onCursorPositionChanged() {
    QTextCursor cursor = m_editor->textCursor();
    int line = cursor.blockNumber() + 1;
    int col = cursor.columnNumber() + 1;
    emit cursorPositionChanged(line, col);
}

} // namespace CinderDE
