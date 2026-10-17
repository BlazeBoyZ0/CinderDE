#include "cinderde/lsp_dialog.h"
#include "cinderde/lsp_manager.h"
#include "cinderde/preferences.h"
#include "cinderde/language_pack.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QProcessEnvironment>
#include <QFileInfo>
#include <QUuid>

namespace CinderDE {

// LspConfigEditDialog Implementation
LspConfigEditDialog::LspConfigEditDialog(const LspServerConfig &config, QWidget *parent)
    : QDialog(parent), m_configId(config.id)
{
    setWindowTitle(config.id.isEmpty() ? "Add Language Server" : "Edit Language Server");
    setMinimumWidth(480);
    setupUi();

    if (!config.id.isEmpty()) {
        m_nameEdit->setText(config.name);
        int idx = m_languageCombo->findData(config.languageId);
        if (idx != -1) {
            m_languageCombo->setCurrentIndex(idx);
        } else {
            m_languageCombo->setCurrentText(config.languageId);
        }
        m_extensionsEdit->setText(config.extensions.join(", "));
        m_executableEdit->setText(config.executable);
        m_argumentsEdit->setText(config.arguments.join(" "));
        m_enabledCB->setChecked(config.enabled);

        QStringList envPairs;
        for (auto it = config.environment.begin(); it != config.environment.end(); ++it) {
            envPairs.append(QString("%1=%2").arg(it.key(), it.value()));
        }
        m_envEdit->setText(envPairs.join("; "));
    } else {
        m_configId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_enabledCB->setChecked(true);
    }
}

void LspConfigEditDialog::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("e.g. Pyright, Clangd, Rust Analyzer");

    m_languageCombo = new QComboBox(this);
    m_languageCombo->setEditable(true);
    populateLanguageCombo();

    m_extensionsEdit = new QLineEdit(this);
    m_extensionsEdit->setPlaceholderText("e.g. py, c, cpp, h (comma separated)");

    QHBoxLayout *execLayout = new QHBoxLayout();
    m_executableEdit = new QLineEdit(this);
    m_executableEdit->setPlaceholderText("Executable name in PATH or full path");
    QPushButton *browseBtn = new QPushButton("Browse...", this);
    connect(browseBtn, &QPushButton::clicked, this, &LspConfigEditDialog::browseExecutable);
    execLayout->addWidget(m_executableEdit);
    execLayout->addWidget(browseBtn);

    m_argumentsEdit = new QLineEdit(this);
    m_argumentsEdit->setPlaceholderText("Space-separated arguments (optional)");

    m_envEdit = new QLineEdit(this);
    m_envEdit->setPlaceholderText("KEY1=VAL1; KEY2=VAL2 (optional)");

    m_enabledCB = new QCheckBox("Enable server", this);

    formLayout->addRow("Display Name:", m_nameEdit);
    formLayout->addRow("Language ID:", m_languageCombo);
    formLayout->addRow("File Extensions:", m_extensionsEdit);
    formLayout->addRow("Executable:", execLayout);
    formLayout->addRow("Arguments:", m_argumentsEdit);
    formLayout->addRow("Environment:", m_envEdit);
    formLayout->addRow("", m_enabledCB);

    mainLayout->addLayout(formLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    QPushButton *saveBtn = new QPushButton("Save", this);
    saveBtn->setDefault(true);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &LspConfigEditDialog::validateAndAccept);

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    mainLayout->addLayout(btnLayout);
}

void LspConfigEditDialog::populateLanguageCombo() {
    static const QList<QPair<QString, QString>> langs = {
        {"C / C++", "cpp"},
        {"Python", "python"},
        {"JavaScript", "javascript"},
        {"TypeScript", "typescript"},
        {"Java", "java"},
        {"Kotlin", "kotlin"},
        {"Shell Script", "shellscript"},
        {"JSON", "json"},
        {"CMake", "cmake"},
        {"HTML", "html"},
        {"CSS", "css"},
        {"YAML", "yaml"}
    };

    for (const auto &pair : langs) {
        m_languageCombo->addItem(pair.first, pair.second);
    }
}

void LspConfigEditDialog::browseExecutable() {
    QString path = QFileDialog::getOpenFileName(this, "Select Executable");
    if (!path.isEmpty()) {
        m_executableEdit->setText(path);
    }
}

void LspConfigEditDialog::validateAndAccept() {
    QString name = m_nameEdit->text().trimmed();
    QString langId = m_languageCombo->currentData().toString();
    if (langId.isEmpty()) {
        langId = m_languageCombo->currentText().trimmed();
    }
    QString exec = m_executableEdit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a display name for the server.");
        m_nameEdit->setFocus();
        return;
    }

    if (langId.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please specify a language ID.");
        m_languageCombo->setFocus();
        return;
    }

    if (exec.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter an executable name or full path.");
        m_executableEdit->setFocus();
        return;
    }

    // Resolve executable path if practical
    bool resolved = false;
    if (QFileInfo::exists(exec) && QFileInfo(exec).isExecutable()) {
        resolved = true;
    } else {
        QString fullPath = QStandardPaths::findExecutable(exec);
        if (!fullPath.isEmpty()) {
            resolved = true;
        }
    }

    if (!resolved) {
        QMessageBox::StandardButton reply = QMessageBox::warning(
            this, "Executable Warning",
            QString("The executable '%1' could not be found in PATH or as an existing executable file.\n\nDo you still want to save this configuration?").arg(exec),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            m_executableEdit->setFocus();
            return;
        }
    }

    accept();
}

LspServerConfig LspConfigEditDialog::config() const {
    LspServerConfig cfg;
    cfg.id = m_configId;
    cfg.name = m_nameEdit->text().trimmed();

    QString langId = m_languageCombo->currentData().toString();
    if (langId.isEmpty()) {
        langId = m_languageCombo->currentText().trimmed();
    }
    cfg.languageId = langId;

    QString rawExts = m_extensionsEdit->text().trimmed();
    if (!rawExts.isEmpty()) {
        for (const QString &e : rawExts.split(',')) {
            QString clean = e.trimmed();
            if (clean.startsWith('.')) clean = clean.mid(1);
            if (!clean.isEmpty()) cfg.extensions.append(clean);
        }
    }

    cfg.executable = m_executableEdit->text().trimmed();

    QString rawArgs = m_argumentsEdit->text().trimmed();
    if (!rawArgs.isEmpty()) {
        cfg.arguments = rawArgs.split(' ', Qt::SkipEmptyParts);
    }

    cfg.enabled = m_enabledCB->isChecked();

    QString rawEnv = m_envEdit->text().trimmed();
    if (!rawEnv.isEmpty()) {
        for (const QString &pair : rawEnv.split(';', Qt::SkipEmptyParts)) {
            int eq = pair.indexOf('=');
            if (eq != -1) {
                QString k = pair.left(eq).trimmed();
                QString v = pair.mid(eq + 1).trimmed();
                if (!k.isEmpty()) {
                    cfg.environment[k] = v;
                }
            }
        }
    }

    return cfg;
}

// LspDialog Implementation
LspDialog::LspDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Language Servers (LSP)");
    resize(700, 480);
    setupUi();
    applyTheme();
    refreshTable();

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(2000);
    connect(m_refreshTimer, &QTimer::timeout, this, &LspDialog::refreshTable);
    m_refreshTimer->start();

    connect(&LspManager::instance(), &LspManager::logMessage, this, [this](const QString &msg) {
        m_logEdit->append(msg);
    });
}

LspDialog::~LspDialog() {
}

void LspDialog::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(6);
    m_tableWidget->setHorizontalHeaderLabels({"Name", "Language", "Executable", "Enabled", "Status", "RAM Estimate"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, &LspDialog::onTableSelectionChanged);

    QHBoxLayout *actionBtnLayout = new QHBoxLayout();
    m_addBtn = new QPushButton("Add...", this);
    m_editBtn = new QPushButton("Edit...", this);
    m_enableBtn = new QPushButton("Enable/Disable", this);
    m_removeBtn = new QPushButton("Remove", this);
    m_startBtn = new QPushButton("Start", this);
    m_stopBtn = new QPushButton("Stop", this);

    connect(m_addBtn, &QPushButton::clicked, this, &LspDialog::addConfig);
    connect(m_editBtn, &QPushButton::clicked, this, &LspDialog::editConfig);
    connect(m_enableBtn, &QPushButton::clicked, this, &LspDialog::toggleEnabled);
    connect(m_removeBtn, &QPushButton::clicked, this, &LspDialog::removeConfig);
    connect(m_startBtn, &QPushButton::clicked, this, &LspDialog::startServer);
    connect(m_stopBtn, &QPushButton::clicked, this, &LspDialog::stopServer);

    actionBtnLayout->addWidget(m_addBtn);
    actionBtnLayout->addWidget(m_editBtn);
    actionBtnLayout->addWidget(m_enableBtn);
    actionBtnLayout->addWidget(m_removeBtn);
    actionBtnLayout->addSpacing(10);
    actionBtnLayout->addWidget(m_startBtn);
    actionBtnLayout->addWidget(m_stopBtn);
    actionBtnLayout->addStretch();

    m_statusDetailsLabel = new QLabel(this);
    m_statusDetailsLabel->setWordWrap(true);
    m_statusDetailsLabel->setStyleSheet("color: #ff8888; font-size: 11px;");

    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(100);
    m_logEdit->setPlaceholderText("Language server activity & stderr logs...");

    mainLayout->addWidget(m_tableWidget, 1);
    mainLayout->addLayout(actionBtnLayout);
    mainLayout->addWidget(m_statusDetailsLabel);
    mainLayout->addWidget(m_logEdit);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    QPushButton *closeBtn = new QPushButton("Close", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn);

    mainLayout->addLayout(bottomLayout);

    updateButtons();
}

void LspDialog::applyTheme() {
    setStyleSheet(R"(
        QDialog { background-color: #1e1e1e; color: #cccccc; }
        QTableWidget { background-color: #252526; color: #cccccc; gridline-color: #383838; border: 1px solid #3c3c3c; }
        QHeaderView::section { background-color: #2d2d2d; color: #cccccc; padding: 4px; border: 1px solid #383838; }
        QPushButton { background-color: #3c3c3c; color: #ffffff; border: 1px solid #555; border-radius: 3px; padding: 4px 10px; }
        QPushButton:hover { background-color: #4c4c4c; }
        QPushButton:pressed { background-color: #ff6b35; color: #ffffff; }
        QPushButton:disabled { background-color: #2a2a2a; color: #666666; border: 1px solid #3a3a3a; }
        QTextEdit { background-color: #181818; color: #d4d4d4; border: 1px solid #333; font-family: Monospace; font-size: 11px; }
    )");
}

void LspDialog::refreshTable() {
    QList<LspServerConfig> configs = Preferences::instance().lspServerConfigs();

    int selectedIdIdx = -1;
    LspServerConfig sel = selectedConfig();

    m_tableWidget->setRowCount(configs.size());

    for (int i = 0; i < configs.size(); ++i) {
        const auto &cfg = configs[i];
        if (cfg.id == sel.id) selectedIdIdx = i;

        m_tableWidget->setItem(i, 0, new QTableWidgetItem(cfg.name));
        m_tableWidget->setItem(i, 1, new QTableWidgetItem(cfg.languageId));
        m_tableWidget->setItem(i, 2, new QTableWidgetItem(cfg.executable));
        m_tableWidget->setItem(i, 3, new QTableWidgetItem(cfg.enabled ? "Yes" : "No"));

        LspClient *client = LspManager::instance().getClientForConfig(cfg.id);
        LspServerStatus st;
        if (client) {
            st = client->status();
        }

        QString statusStr = "Stopped";
        if (st.state == LspServerState::Starting) statusStr = "Starting";
        else if (st.state == LspServerState::Running) statusStr = QString("Running (PID %1)").arg(st.pid);
        else if (st.state == LspServerState::Stopping) statusStr = "Stopping";
        else if (st.state == LspServerState::Failed) statusStr = "Failed";

        m_tableWidget->setItem(i, 4, new QTableWidgetItem(statusStr));

        QString ramStr = "N/A";
        if (st.ramBytes > 0) {
            double mb = st.ramBytes / (1024.0 * 1024.0);
            ramStr = QString("~%1 MB").arg(mb, 0, 'f', 1);
        }
        m_tableWidget->setItem(i, 5, new QTableWidgetItem(ramStr));

        m_tableWidget->item(i, 0)->setData(Qt::UserRole, cfg.id);
    }

    if (selectedIdIdx != -1) {
        m_tableWidget->selectRow(selectedIdIdx);
    }

    updateButtons();
}

int LspDialog::selectedRow() const {
    auto items = m_tableWidget->selectedItems();
    if (items.isEmpty()) return -1;
    return items.first()->row();
}

LspServerConfig LspDialog::selectedConfig() const {
    int row = selectedRow();
    if (row < 0) return LspServerConfig();

    QTableWidgetItem *item = m_tableWidget->item(row, 0);
    if (!item) return LspServerConfig();

    QString id = item->data(Qt::UserRole).toString();
    QList<LspServerConfig> configs = Preferences::instance().lspServerConfigs();
    for (const auto &cfg : configs) {
        if (cfg.id == id) return cfg;
    }

    return LspServerConfig();
}

void LspDialog::onTableSelectionChanged() {
    updateButtons();
}

void LspDialog::updateButtons() {
    LspServerConfig cfg = selectedConfig();
    bool hasSel = !cfg.id.isEmpty();

    m_editBtn->setEnabled(hasSel);
    m_enableBtn->setEnabled(hasSel);
    m_removeBtn->setEnabled(hasSel);

    LspClient *client = hasSel ? LspManager::instance().getClientForConfig(cfg.id) : nullptr;
    bool isRunning = client && client->isRunning();

    m_startBtn->setEnabled(hasSel && cfg.enabled && !isRunning);
    m_stopBtn->setEnabled(hasSel && isRunning);

    if (client && !client->status().lastError.isEmpty() && client->status().state == LspServerState::Failed) {
        m_statusDetailsLabel->setText(QString("Last error: %1").arg(client->status().lastError));
    } else {
        m_statusDetailsLabel->clear();
    }
}

void LspDialog::addConfig() {
    LspConfigEditDialog dlg(LspServerConfig(), this);
    if (dlg.exec() == QDialog::Accepted) {
        LspServerConfig newCfg = dlg.config();
        Preferences::instance().saveLspServerConfig(newCfg);
        refreshTable();
    }
}

void LspDialog::editConfig() {
    LspServerConfig cfg = selectedConfig();
    if (cfg.id.isEmpty()) return;

    LspConfigEditDialog dlg(cfg, this);
    if (dlg.exec() == QDialog::Accepted) {
        LspServerConfig updatedCfg = dlg.config();
        Preferences::instance().saveLspServerConfig(updatedCfg);
        refreshTable();
    }
}

void LspDialog::toggleEnabled() {
    LspServerConfig cfg = selectedConfig();
    if (cfg.id.isEmpty()) return;

    cfg.enabled = !cfg.enabled;
    Preferences::instance().saveLspServerConfig(cfg);

    if (!cfg.enabled) {
        LspManager::instance().stopServer(cfg.id);
    }

    refreshTable();
}

void LspDialog::removeConfig() {
    LspServerConfig cfg = selectedConfig();
    if (cfg.id.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Remove Language Server",
        QString("Are you sure you want to remove language server '%1'?").arg(cfg.name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        LspManager::instance().stopServer(cfg.id);
        Preferences::instance().removeLspServerConfig(cfg.id);
        refreshTable();
    }
}

void LspDialog::startServer() {
    LspServerConfig cfg = selectedConfig();
    if (cfg.id.isEmpty() || !cfg.enabled) return;

    LspManager::instance().restartServer(cfg.id);
    refreshTable();
}

void LspDialog::stopServer() {
    LspServerConfig cfg = selectedConfig();
    if (cfg.id.isEmpty()) return;

    LspManager::instance().stopServer(cfg.id);
    refreshTable();
}

} // namespace CinderDE
