#ifndef CINDERDE_OUTPUT_PANEL_H
#define CINDERDE_OUTPUT_PANEL_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include "cinderde/diagnostic_parser.h"

namespace CinderDE {

class OutputPanel : public QWidget {
    Q_OBJECT

public:
    explicit OutputPanel(QWidget *parent = nullptr);

    void appendBuildOutput(const QString &text, const QString &workingDir = QString());
    void appendRunOutput(const QString &text);
    void appendSystemMessage(const QString &text);
    void clearOutput();
    void setProcessRunning(bool running, const QString &processName = QString());
    void setWorkingDir(const QString &dir) { m_workingDir = dir; }

signals:
    void stopRequested();
    void stdinSubmitted(const QString &text);
    void diagnosticClicked(const QString &filePath, int line, int column);
    void closeRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onSendStdin();
    void onTextDoubleClicked();

private:
    QPlainTextEdit *m_outputText;
    QLineEdit *m_stdinInput;
    QPushButton *m_sendButton;
    QToolButton *m_stopButton;
    QToolButton *m_clearButton;
    QToolButton *m_closeButton;
    QLabel *m_statusLabel;
    QLabel *m_stdinInfoLabel;

    QString m_workingDir;

    void setupUi();
    void applyTheme();
    void handleLineAtCursor();
};

} // namespace CinderDE

#endif // CINDERDE_OUTPUT_PANEL_H
