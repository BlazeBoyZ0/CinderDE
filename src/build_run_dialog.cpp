#include "cinderde/build_run_dialog.h"
#include "cinderde/preferences.h"
#include "cinderde/build_run_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QMessageBox>

namespace CinderDE {

BuildRunDialog::BuildRunDialog(const QString &workspacePath, const WorkspaceConfig &config, QWidget *parent)
    : QDialog(parent),
      m_workspacePath(workspacePath)
{
    setWindowTitle("Build & Run Settings");
    resize(580, 420);
    setupUi(config);
    applyTheme();
}

void BuildRunDialog::setupUi(const WorkspaceConfig &config) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // Project Commands Group
    QGroupBox *cmdGroup = new QGroupBox("Project Commands", this);
    QFormLayout *cmdLayout = new QFormLayout(cmdGroup);
    cmdLayout->setSpacing(8);

    m_buildCommandEdit = new QLineEdit(cmdGroup);
    m_buildCommandEdit->setPlaceholderText("e.g. cmake -B build -S . && cmake --build build OR g++ -g ${file} -o ${fileDir}/${fileBase}");
    if (config.hasBuildCommand) {
        m_buildCommandEdit->setText(config.buildCommand);
    }

    m_runCommandEdit = new QLineEdit(cmdGroup);
    m_runCommandEdit->setPlaceholderText("e.g. ${fileDir}/${fileBase} OR python3 ${file}");
    if (config.hasRunCommand) {
        m_runCommandEdit->setText(config.runCommand);
    }

    m_workingDirEdit = new QLineEdit(cmdGroup);
    m_workingDirEdit->setPlaceholderText("e.g. ${workspace} OR ${fileDir} (default: auto)");
    if (config.hasWorkingDirectory) {
        m_workingDirEdit->setText(config.workingDirectory);
    }

    cmdLayout->addRow("Build Command:", m_buildCommandEdit);
    cmdLayout->addRow("Run Command:", m_runCommandEdit);
    cmdLayout->addRow("Working Directory:", m_workingDirEdit);

    // Execution Mode Group
    QGroupBox *execGroup = new QGroupBox("Execution Mode & Terminal", this);
    QFormLayout *execLayout = new QFormLayout(execGroup);
    execLayout->setSpacing(8);

    m_executionModeCombo = new QComboBox(execGroup);
    m_executionModeCombo->addItem("Output Panel (Default)", "output_panel");
    m_executionModeCombo->addItem("External Linux Terminal", "external_terminal");

    QString currentMode = config.hasExecutionMode ? config.executionMode : Preferences::instance().executionMode();
    int modeIdx = m_executionModeCombo->findData(currentMode);
    if (modeIdx != -1) {
        m_executionModeCombo->setCurrentIndex(modeIdx);
    }

    QHBoxLayout *termLayout = new QHBoxLayout();
    m_terminalCommandEdit = new QLineEdit(execGroup);
    m_terminalCommandEdit->setPlaceholderText("e.g. gnome-terminal, konsole, xterm (auto-detected if blank)");
    QString currentTerm = config.hasTerminalCommand ? config.terminalCommand : Preferences::instance().terminalCommand();
    m_terminalCommandEdit->setText(currentTerm);

    QPushButton *autoDetectBtn = new QPushButton("Auto-Detect", execGroup);
    connect(autoDetectBtn, &QPushButton::clicked, this, &BuildRunDialog::onAutoDetectTerminal);

    termLayout->addWidget(m_terminalCommandEdit, 1);
    termLayout->addWidget(autoDetectBtn);

    execLayout->addRow("Execution Mode:", m_executionModeCombo);
    execLayout->addRow("Terminal Command:", termLayout);

    // Help text
    QLabel *helpLabel = new QLabel(
        "Macro Variables:\n"
        "  ${file} : Full active file path       ${fileDir} : File directory\n"
        "  ${fileBase} : File name without ext   ${workspace} : Workspace directory\n"
        "  ${workingDir} : Resolved working dir  ${execPath} : Target binary path", this);
    helpLabel->setStyleSheet("color: #888888; font-size: 11px; font-family: monospace; padding: 4px; background: #252526; border: 1px solid #333333; border-radius: 3px;");

    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(cmdGroup);
    mainLayout->addWidget(execGroup);
    mainLayout->addWidget(helpLabel);
    mainLayout->addWidget(buttonBox);
}

void BuildRunDialog::applyTheme() {
    setStyleSheet(R"(
        QDialog {
            background-color: #1e1e1e;
            color: #cccccc;
        }
        QGroupBox {
            color: #ff6b35;
            font-weight: bold;
            border: 1px solid #3c3c3c;
            border-radius: 4px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
        QLineEdit, QComboBox {
            background-color: #252526;
            color: #ffffff;
            border: 1px solid #3c3c3c;
            border-radius: 3px;
            padding: 4px 6px;
            font-size: 12px;
        }
        QLineEdit:focus, QComboBox:focus {
            border: 1px solid #ff6b35;
        }
        QPushButton {
            background-color: #3c3c3c;
            color: #ffffff;
            border: 1px solid #454545;
            border-radius: 3px;
            padding: 4px 10px;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #ff6b35;
        }
    )");
}

void BuildRunDialog::onAutoDetectTerminal() {
    QString detected = BuildRunManager::detectTerminalEmulator();
    if (!detected.isEmpty()) {
        m_terminalCommandEdit->setText(detected);
        QMessageBox::information(this, "Terminal Detected", QString("Detected terminal emulator:\n%1").arg(detected));
    } else {
        QMessageBox::warning(this, "No Terminal Found", "Could not automatically detect an installed terminal emulator.\nPlease specify one manually (e.g. xterm, gnome-terminal, konsole).");
    }
}

WorkspaceConfig BuildRunDialog::config() const {
    WorkspaceConfig cfg;
    QString buildStr = m_buildCommandEdit->text().trimmed();
    if (!buildStr.isEmpty()) {
        cfg.hasBuildCommand = true;
        cfg.buildCommand = buildStr;
    }

    QString runStr = m_runCommandEdit->text().trimmed();
    if (!runStr.isEmpty()) {
        cfg.hasRunCommand = true;
        cfg.runCommand = runStr;
    }

    QString workStr = m_workingDirEdit->text().trimmed();
    if (!workStr.isEmpty()) {
        cfg.hasWorkingDirectory = true;
        cfg.workingDirectory = workStr;
    }

    cfg.hasExecutionMode = true;
    cfg.executionMode = executionMode();

    QString termStr = m_terminalCommandEdit->text().trimmed();
    if (!termStr.isEmpty()) {
        cfg.hasTerminalCommand = true;
        cfg.terminalCommand = termStr;
    }

    return cfg;
}

QString BuildRunDialog::executionMode() const {
    return m_executionModeCombo->currentData().toString();
}

QString BuildRunDialog::terminalCommand() const {
    return m_terminalCommandEdit->text().trimmed();
}

} // namespace CinderDE
