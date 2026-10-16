#include "cinderde/editor_tab.h"
#include "cinderde/preferences.h"
#include <QPainter>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextCursor>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QMessageBox>
#include <QShortcut>

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
    : QPlainTextEdit(parent)
{
    m_lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
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

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor("#2a2d2e");
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
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
    : QWidget(parent), m_fontSize(11), m_replaceMode(false)
{
    m_editor = new CodeEditor(this);

    // Default font
    QFont font("Monospace");
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(m_fontSize);
    m_editor->setFont(font);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    createFindReplaceBar();
    mainLayout->addWidget(m_findReplaceWidget);
    mainLayout->addWidget(m_editor);

    connect(m_editor->document(), &QTextDocument::contentsChanged, this, &EditorTab::onTextChanged);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, &EditorTab::onCursorPositionChanged);

    m_typeInfo = FileTypeDetector::detectType("");
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

bool EditorTab::openFile(const QString &filePath) {
    if (FileTypeDetector::isLikelyBinary(filePath)) {
        QMessageBox::StandardButton res = QMessageBox::warning(
            this, "Binary File Warning",
            QString("The file '%1' appears to be a binary file. Opening binary files may display invalid text.\n\nDo you still want to open it?").arg(QFileInfo(filePath).fileName()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (res != QMessageBox::Yes) {
            return false;
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error Opening File", QString("Failed to open file:\n%1\nError: %2").arg(filePath, file.errorString()));
        return false;
    }

    QTextStream in(&file);
    m_editor->setPlainText(in.readAll());
    file.close();

    m_filePath = filePath;
    m_editor->document()->setModified(false);
    m_typeInfo = FileTypeDetector::detectType(filePath);

    emit fileTypeChanged(m_typeInfo);
    emit modificationChanged(false);
    onCursorPositionChanged();

    return true;
}

void EditorTab::setFilePath(const QString &filePath) {
    m_filePath = filePath;
    m_typeInfo = FileTypeDetector::detectType(filePath);
    emit fileTypeChanged(m_typeInfo);
}

void EditorTab::clearFilePathAndMarkModified() {
    m_filePath.clear();
    m_editor->document()->setModified(true);
    emit modificationChanged(true);
}

bool EditorTab::save() {
    if (isUntitled()) {
        return false; // Requires saveAs
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error Saving File", QString("Failed to save file:\n%1\nError: %2").arg(m_filePath, file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out << m_editor->toPlainText();
    out.flush();

    if (file.error() != QFileDevice::NoError) {
        QMessageBox::critical(this, "Error Saving File", QString("Failed to write to file:\n%1\nError: %2").arg(m_filePath, file.errorString()));
        file.close();
        return false;
    }

    file.close();

    m_editor->document()->setModified(false);
    emit modificationChanged(false);

    return true;
}

bool EditorTab::saveAs(const QString &filePath) {
    if (filePath.isEmpty()) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error Saving File", QString("Failed to save file:\n%1\nError: %2").arg(filePath, file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out << m_editor->toPlainText();
    out.flush();

    if (file.error() != QFileDevice::NoError) {
        QMessageBox::critical(this, "Error Saving File", QString("Failed to write to file:\n%1\nError: %2").arg(filePath, file.errorString()));
        file.close();
        return false;
    }

    file.close();

    m_filePath = filePath;
    m_typeInfo = FileTypeDetector::detectType(filePath);
    m_editor->document()->setModified(false);

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
    emit modificationChanged(m_editor->document()->isModified());
}

void EditorTab::onCursorPositionChanged() {
    QTextCursor cursor = m_editor->textCursor();
    int line = cursor.blockNumber() + 1;
    int col = cursor.columnNumber() + 1;
    emit cursorPositionChanged(line, col);
}

} // namespace CinderDE
