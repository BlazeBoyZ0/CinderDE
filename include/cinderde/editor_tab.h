#ifndef CINDERDE_EDITOR_TAB_H
#define CINDERDE_EDITOR_TAB_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTimer>
#include "cinderde/file_type_detector.h"
#include "cinderde/syntax_highlighter.h"
#include "cinderde/lsp_types.h"

namespace CinderDE {

class CodeEditor;
class CodeCompletionPopup;

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CodeEditor *m_editor;
};

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth() const;
    void setEditorFontSize(int size);
    void setEditorTabSize(int size);

    void setLanguage(Language lang) { m_language = lang; }
    Language language() const { return m_language; }

    void setLanguageDefinition(const LanguageDefinition &def) { m_languageDef = def; }
    LanguageDefinition languageDefinition() const { return m_languageDef; }

    void toggleComment();

    void setFilePath(const QString &filePath) { m_filePath = filePath; }
    QString filePath() const { return m_filePath; }

    void setDiagnostics(const QList<struct LspDiagnostic> &diagnostics);
    void triggerCompletion();
    void triggerHover();
    void triggerGoToDefinition();
    void triggerFindReferences();
    void triggerRename();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLineAndBracket();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *m_lineNumberArea;
    Language m_language;
    LanguageDefinition m_languageDef;
    CodeCompletionPopup *m_completionPopup;
    QTimer *m_autoCompletionTimer;
    int m_completionRequestGeneration;
    QString m_filePath;
    QList<struct LspDiagnostic> m_diagnostics;

    void handleSmartEnter();
    void handleSmartBackspace(QKeyEvent *e);
    bool handleBracketPairing(QKeyEvent *e);
    void highlightMatchingBracket(QList<QTextEdit::ExtraSelection> &extraSelections);
};

class EditorTab : public QWidget {
    Q_OBJECT

public:
    explicit EditorTab(QWidget *parent = nullptr);
    ~EditorTab() override;

    bool openFile(const QString &filePath, bool showDialog = true);
    bool save(bool showDialog = true);
    bool saveAs(const QString &filePath, bool showDialog = true);

    void setFilePath(const QString &filePath);
    void clearFilePathAndMarkModified();

    QString filePath() const { return m_filePath; }
    QString fileName() const;
    bool isModified() const;
    bool isUntitled() const { return m_filePath.isEmpty(); }
    FileTypeInfo fileTypeInfo() const { return m_typeInfo; }

    CodeEditor* editor() const { return m_editor; }
    SyntaxHighlighter* highlighter() const { return m_highlighter; }
    void updateHighlighterLanguage();

    void setFontSize(int size);
    int fontSize() const { return m_fontSize; }
    void zoomIn(int range = 1);
    void zoomOut(int range = 1);
    void resetFontSize();

    void setTabSize(int size);
    int tabSize() const { return m_tabSize; }

    void setWordWrap(bool wrap);
    bool wordWrap() const;

    void showFindReplaceBar(bool showReplace = false);
    void hideFindReplaceBar();

signals:
    void modificationChanged(bool modified);
    void cursorPositionChanged(int line, int col);
    void fileTypeChanged(const FileTypeInfo &info);

private slots:
    void onTextChanged();
    void onCursorPositionChanged();
    void findNext();
    void findPrevious();
    void replaceOne();
    void replaceAll();

private:
    CodeEditor *m_editor;
    SyntaxHighlighter *m_highlighter;
    QString m_filePath;
    FileTypeInfo m_typeInfo;
    int m_fontSize;
    int m_tabSize;

    // Find and Replace Bar
    QWidget *m_findReplaceWidget;
    QLineEdit *m_findEdit;
    QLineEdit *m_replaceEdit;
    QPushButton *m_findNextBtn;
    QPushButton *m_findPrevBtn;
    QPushButton *m_replaceBtn;
    QPushButton *m_replaceAllBtn;
    QPushButton *m_closeFindBtn;
    QCheckBox *m_caseSensitiveCB;
    QLabel *m_matchCountLabel;
    bool m_replaceMode;

    void createFindReplaceBar();
    void updateMatchCount();
};

} // namespace CinderDE

#endif // CINDERDE_EDITOR_TAB_H
