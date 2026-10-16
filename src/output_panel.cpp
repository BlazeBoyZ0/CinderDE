#include "cinderde/output_panel.h"
#include <QPlainTextEdit>
#include <QMouseEvent>
#include <QTextCursor>
#include <QTextBlock>
#include <QScrollBar>

namespace CinderDE {

OutputPanel::OutputPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    applyTheme();
}

void OutputPanel::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Toolbar header
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setObjectName("output_header");
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(8, 4, 8, 4);
    headerLayout->setSpacing(8);

    QLabel *titleLabel = new QLabel("OUTPUT", headerWidget);
    titleLabel->setStyleSheet("font-weight: bold; color: #aaaaaa; font-size: 11px;");

    m_statusLabel = new QLabel("Idle", headerWidget);
    m_statusLabel->setStyleSheet("color: #888888; font-size: 11px;");

    m_stopButton = new QToolButton(headerWidget);
    m_stopButton->setText("Stop");
    m_stopButton->setToolTip("Stop Active Process (Shift+F5)");
    m_stopButton->setEnabled(false);

    m_clearButton = new QToolButton(headerWidget);
    m_clearButton->setText("Clear");
    m_clearButton->setToolTip("Clear Output Log");

    m_closeButton = new QToolButton(headerWidget);
    m_closeButton->setText("×");
    m_closeButton->setToolTip("Close Output Panel");

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(m_statusLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_stopButton);
    headerLayout->addWidget(m_clearButton);
    headerLayout->addWidget(m_closeButton);

    // Output text area
    m_outputText = new QPlainTextEdit(this);
    m_outputText->setReadOnly(true);
    m_outputText->setMaximumBlockCount(5000); // Prevent unbounded memory growth
    m_outputText->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    m_outputText->viewport()->installEventFilter(this);

    // Interactive Stdin area
    QWidget *stdinWidget = new QWidget(this);
    stdinWidget->setObjectName("stdin_widget");
    QHBoxLayout *stdinLayout = new QHBoxLayout(stdinWidget);
    stdinLayout->setContentsMargins(6, 2, 6, 4);
    stdinLayout->setSpacing(6);

    QLabel *stdinPrompt = new QLabel("Input:", stdinWidget);
    stdinPrompt->setStyleSheet("color: #aaaaaa; font-size: 11px;");

    m_stdinInput = new QLineEdit(stdinWidget);
    m_stdinInput->setPlaceholderText("Type input for active program and press Enter...");
    m_stdinInput->setEnabled(false);

    m_sendButton = new QPushButton("Send", stdinWidget);
    m_sendButton->setEnabled(false);

    m_stdinInfoLabel = new QLabel("(Note: Interactive console apps needing TTY should use External Terminal mode)", stdinWidget);
    m_stdinInfoLabel->setStyleSheet("color: #777777; font-size: 10px; font-style: italic;");

    stdinLayout->addWidget(stdinPrompt);
    stdinLayout->addWidget(m_stdinInput, 1);
    stdinLayout->addWidget(m_sendButton);
    stdinLayout->addWidget(m_stdinInfoLabel);

    mainLayout->addWidget(headerWidget);
    mainLayout->addWidget(m_outputText, 1);
    mainLayout->addWidget(stdinWidget);

    connect(m_stopButton, &QToolButton::clicked, this, &OutputPanel::stopRequested);
    connect(m_clearButton, &QToolButton::clicked, this, &OutputPanel::clearOutput);
    connect(m_closeButton, &QToolButton::clicked, this, &OutputPanel::closeRequested);
    connect(m_sendButton, &QPushButton::clicked, this, &OutputPanel::onSendStdin);
    connect(m_stdinInput, &QLineEdit::returnPressed, this, &OutputPanel::onSendStdin);
}

void OutputPanel::applyTheme() {
    setStyleSheet(R"(
        QWidget#output_header {
            background-color: #252526;
            border-top: 1px solid #333333;
            border-bottom: 1px solid #333333;
        }
        QPlainTextEdit {
            background-color: #1e1e1e;
            color: #d4d4d4;
            font-family: monospace;
            font-size: 12px;
            border: none;
        }
        QWidget#stdin_widget {
            background-color: #252526;
            border-top: 1px solid #333333;
        }
        QLineEdit {
            background-color: #1e1e1e;
            color: #cccccc;
            border: 1px solid #3c3c3c;
            border-radius: 2px;
            padding: 2px 5px;
            font-family: monospace;
            font-size: 12px;
        }
        QLineEdit:focus {
            border: 1px solid #ff6b35;
        }
        QPushButton {
            background-color: #3c3c3c;
            color: #ffffff;
            border: 1px solid #454545;
            border-radius: 2px;
            padding: 2px 8px;
            font-size: 11px;
        }
        QPushButton:hover {
            background-color: #ff6b35;
        }
        QToolButton {
            background-color: #3c3c3c;
            color: #cccccc;
            border: 1px solid #454545;
            border-radius: 2px;
            padding: 2px 6px;
            font-size: 11px;
        }
        QToolButton:hover {
            background-color: #ff6b35;
            color: #ffffff;
        }
        QToolButton:disabled {
            background-color: #2a2a2a;
            color: #555555;
            border: 1px solid #333333;
        }
    )");
}

void OutputPanel::appendBuildOutput(const QString &text, const QString &workingDir) {
    if (!workingDir.isEmpty()) {
        m_workingDir = workingDir;
    }
    m_outputText->appendPlainText(text);
    m_outputText->verticalScrollBar()->setValue(m_outputText->verticalScrollBar()->maximum());
}

void OutputPanel::appendRunOutput(const QString &text) {
    m_outputText->appendPlainText(text);
    m_outputText->verticalScrollBar()->setValue(m_outputText->verticalScrollBar()->maximum());
}

void OutputPanel::appendSystemMessage(const QString &text) {
    m_outputText->appendPlainText(text);
    m_outputText->verticalScrollBar()->setValue(m_outputText->verticalScrollBar()->maximum());
}

void OutputPanel::clearOutput() {
    m_outputText->clear();
}

void OutputPanel::setProcessRunning(bool running, const QString &processName) {
    m_stopButton->setEnabled(running);
    m_stdinInput->setEnabled(running);
    m_sendButton->setEnabled(running);

    if (running) {
        m_statusLabel->setText(processName.isEmpty() ? "Running..." : QString("Running: %1").arg(processName));
        m_statusLabel->setStyleSheet("color: #ff6b35; font-size: 11px; font-weight: bold;");
    } else {
        m_statusLabel->setText("Idle");
        m_statusLabel->setStyleSheet("color: #888888; font-size: 11px;");
    }
}

void OutputPanel::onSendStdin() {
    QString input = m_stdinInput->text();
    if (input.isEmpty()) return;

    m_outputText->appendPlainText(QString("> %1").arg(input));
    emit stdinSubmitted(input + "\n");
    m_stdinInput->clear();
}

bool OutputPanel::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_outputText->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QTextCursor cursor = m_outputText->cursorForPosition(mouseEvent->pos());
        QString lineText = cursor.block().text();

        auto diag = DiagnosticParser::parseLine(lineText, m_workingDir);
        if (diag.has_value() && diag->isValid()) {
            emit diagnosticClicked(diag->filePath, diag->line, diag->column);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void OutputPanel::onTextDoubleClicked() {
    handleLineAtCursor();
}

void OutputPanel::handleLineAtCursor() {
    QTextCursor cursor = m_outputText->textCursor();
    QString lineText = cursor.block().text();
    auto diag = DiagnosticParser::parseLine(lineText, m_workingDir);
    if (diag.has_value() && diag->isValid()) {
        emit diagnosticClicked(diag->filePath, diag->line, diag->column);
    }
}

} // namespace CinderDE
