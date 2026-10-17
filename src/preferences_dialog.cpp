#include "cinderde/preferences_dialog.h"
#include "cinderde/preferences.h"
#include "cinderde/language_pack.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QDir>
#include <QDialogButtonBox>
#include <QLabel>

namespace CinderDE {

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Preferences");
    resize(640, 480);
    setupUi();
    applyTheme();
}

void PreferencesDialog::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    m_tabWidget = new QTabWidget(this);

    setupEditorTab();
    setupLanguagePacksTab();

    mainLayout->addWidget(m_tabWidget, 1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        applySettings();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

void PreferencesDialog::setupEditorTab() {
    QWidget *editorTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(editorTab);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    Preferences &prefs = Preferences::instance();

    m_fontSizeSpin = new QSpinBox(editorTab);
    m_fontSizeSpin->setRange(6, 72);
    m_fontSizeSpin->setValue(prefs.fontSize());
    formLayout->addRow("Font Size:", m_fontSizeSpin);

    m_tabSizeSpin = new QSpinBox(editorTab);
    m_tabSizeSpin->setRange(1, 16);
    m_tabSizeSpin->setValue(prefs.tabSize());
    formLayout->addRow("Tab Size (Spaces):", m_tabSizeSpin);

    m_wordWrapCB = new QCheckBox("Enable Word Wrap", editorTab);
    m_wordWrapCB->setChecked(prefs.wordWrap());
    formLayout->addRow("", m_wordWrapCB);

    m_executionModeCombo = new QComboBox(editorTab);
    m_executionModeCombo->addItem("Integrated Output Panel", "output_panel");
    m_executionModeCombo->addItem("External Terminal Window", "external_terminal");
    int execIdx = m_executionModeCombo->findData(prefs.executionMode());
    if (execIdx != -1) m_executionModeCombo->setCurrentIndex(execIdx);
    formLayout->addRow("Default Execution Mode:", m_executionModeCombo);

    m_terminalCommandEdit = new QLineEdit(editorTab);
    m_terminalCommandEdit->setPlaceholderText("Auto-detect (or e.g. xterm, konsole, gnome-terminal)");
    m_terminalCommandEdit->setText(prefs.terminalCommand());
    formLayout->addRow("Terminal Command:", m_terminalCommandEdit);

    layout->addLayout(formLayout);
    layout->addStretch();

    m_tabWidget->addTab(editorTab, "General Editor");
}

void PreferencesDialog::setupLanguagePacksTab() {
    QWidget *packsTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(packsTab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    QHBoxLayout *topLayout = new QHBoxLayout();
    QLabel *infoLabel = new QLabel("Discovered Language Packs (Built-in and User Installed):", packsTab);
    m_openPacksFolderBtn = new QPushButton("Open Language Packs Folder", packsTab);
    connect(m_openPacksFolderBtn, &QPushButton::clicked, this, &PreferencesDialog::openLanguagePacksFolder);

    topLayout->addWidget(infoLabel, 1);
    topLayout->addWidget(m_openPacksFolderBtn);

    layout->addLayout(topLayout);

    m_packsTable = new QTableWidget(packsTab);
    m_packsTable->setColumnCount(5);
    QStringList headers = {"Language Name", "Identifier", "Extensions", "Source", "Status"};
    m_packsTable->setHorizontalHeaderLabels(headers);
    m_packsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_packsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_packsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_packsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_packsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_packsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_packsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_packsTable->setAlternatingRowColors(true);

    layout->addWidget(m_packsTable, 1);

    populateLanguagePacksTable();

    m_tabWidget->addTab(packsTab, "Language Packs");
}

void PreferencesDialog::populateLanguagePacksTable() {
    m_packsTable->setRowCount(0);

    LanguageRegistry &registry = LanguageRegistry::instance();
    registry.discoverPacks();

    QList<LanguagePackInfo> infos = registry.packInfos();
    m_packsTable->setRowCount(infos.size());

    for (int i = 0; i < infos.size(); ++i) {
        const LanguagePackInfo &info = infos[i];

        QTableWidgetItem *nameItem = new QTableWidgetItem(info.name.isEmpty() ? "(Unknown)" : info.name);
        QTableWidgetItem *idItem = new QTableWidgetItem(info.id);
        QTableWidgetItem *extItem = new QTableWidgetItem(info.extensions.join(", "));
        QTableWidgetItem *srcItem = new QTableWidgetItem(info.source == LanguagePackSource::Builtin ? "Built-in" : "User Pack");
        QTableWidgetItem *statusItem = new QTableWidgetItem(info.statusMessage);

        if (!info.loaded) {
            statusItem->setForeground(QColor("#ff4d4d"));
        } else {
            statusItem->setForeground(QColor("#85e085"));
        }

        m_packsTable->setItem(i, 0, nameItem);
        m_packsTable->setItem(i, 1, idItem);
        m_packsTable->setItem(i, 2, extItem);
        m_packsTable->setItem(i, 3, srcItem);
        m_packsTable->setItem(i, 4, statusItem);
    }
}

void PreferencesDialog::openLanguagePacksFolder() {
    QString userDir = LanguageRegistry::userLanguagesDirectory();
    QUrl url = QUrl::fromLocalFile(userDir);
    if (!QDesktopServices::openUrl(url)) {
        // Fallback using xdg-open
        QProcess::startDetached("xdg-open", QStringList() << userDir);
    }
}

void PreferencesDialog::applySettings() {
    Preferences &prefs = Preferences::instance();

    prefs.setFontSize(m_fontSizeSpin->value());
    prefs.setTabSize(m_tabSizeSpin->value());
    prefs.setWordWrap(m_wordWrapCB->isChecked());
    prefs.setExecutionMode(m_executionModeCombo->currentData().toString());
    prefs.setTerminalCommand(m_terminalCommandEdit->text().trimmed());
}

void PreferencesDialog::applyTheme() {
    setStyleSheet(R"(
        QDialog {
            background-color: #1e1e1e;
            color: #cccccc;
        }
        QTabWidget::pane {
            border: 1px solid #3c3c3c;
            background-color: #252526;
        }
        QTabBar::tab {
            background-color: #2d2d2d;
            color: #969696;
            padding: 6px 16px;
            border-right: 1px solid #252526;
        }
        QTabBar::tab:selected {
            background-color: #252526;
            color: #ffffff;
            border-top: 2px solid #ff6b35;
        }
        QTableWidget {
            background-color: #1e1e1e;
            alternate-background-color: #252526;
            color: #cccccc;
            gridline-color: #333333;
            border: 1px solid #3c3c3c;
        }
        QHeaderView::section {
            background-color: #2d2d2d;
            color: #ffffff;
            padding: 4px;
            border: 1px solid #3c3c3c;
            font-weight: bold;
        }
        QPushButton {
            background-color: #3c3c3c;
            color: #ffffff;
            border: 1px solid #555555;
            border-radius: 3px;
            padding: 5px 12px;
        }
        QPushButton:hover {
            background-color: #4c4c4c;
        }
        QPushButton:pressed {
            background-color: #ff6b35;
        }
        QLineEdit, QSpinBox, QComboBox {
            background-color: #3c3c3c;
            color: #ffffff;
            border: 1px solid #555555;
            border-radius: 2px;
            padding: 4px;
        }
        QCheckBox {
            color: #cccccc;
        }
        QLabel {
            color: #cccccc;
        }
    )");
}

} // namespace CinderDE
