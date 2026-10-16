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
#include "cinderde/file_type_detector.h"
#include "cinderde/syntax_highlighter.h"

namespace CinderDE {

class CodeEditor;

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

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *m_lineNumberArea;
};

class EditorTab : public QWidget {
    Q_OBJECT

public:
    explicit EditorTab(QWidget *parent = nullptr);

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
