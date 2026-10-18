#include "cinderde/debugger_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QUuid>

namespace CinderDE {

DebuggerConfigEditDialog::DebuggerConfigEditDialog(const DebuggerConfiguration &config, QWidget *parent)
    : QDialog(parent), m_id(config.id)
{
    setWindowTitle(config.id.isEmpty() ? QStringLiteral("Add Debugger") : QStringLiteral("Edit Debugger"));
    setMinimumWidth(500);
    setStyleSheet(R"(
        QDialog { background-color: #1e1e1e; color: #cccccc; }
        QLabel { color: #cccccc; }
        QLineEdit, QComboBox { background-color: #252526; color: #dddddd; border: 1px solid #444444; padding: 4px; }
        QPushButton { background-color: #333333; color: #eeeeee; border: 1px solid #484848; padding: 5px 10px; }
        QPushButton:hover { background-color: #ff6b35; color: #ffffff; }
        QCheckBox { color: #cccccc; }
    )");

    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    m_nameEdit = new QLineEdit(config.name, this);
    m_nameEdit->setPlaceholderText(QStringLiteral("e.g. C++ GDB, Python debugpy, Kotlin adapter"));
    m_languageCombo = new QComboBox(this);
    m_languageCombo->setEditable(true);
    m_languageCombo->setInsertPolicy(QComboBox::NoInsert);
    m_languageCombo->addItem(QStringLiteral("Python"), QStringLiteral("python"));
    m_languageCombo->addItem(QStringLiteral("C++"), QStringLiteral("cpp"));
    m_languageCombo->addItem(QStringLiteral("C"), QStringLiteral("c"));
    m_languageCombo->addItem(QStringLiteral("Kotlin"), QStringLiteral("kotlin"));
    m_languageCombo->addItem(QStringLiteral("Java"), QStringLiteral("java"));
    m_languageCombo->addItem(QStringLiteral("JavaScript"), QStringLiteral("javascript"));
    m_languageCombo->addItem(QStringLiteral("Rust"), QStringLiteral("rust"));
    m_languageCombo->addItem(QStringLiteral("Go"), QStringLiteral("go"));
    if (!config.languageId.isEmpty() && m_languageCombo->findData(config.languageId) < 0)
        m_languageCombo->setEditText(config.languageId);
    const int languageIndex = m_languageCombo->findData(config.languageId);
    if (languageIndex >= 0)
        m_languageCombo->setCurrentIndex(languageIndex);

    auto *executableRow = new QHBoxLayout;
    m_executableEdit = new QLineEdit(config.executable, this);
    m_executableEdit->setPlaceholderText(QStringLiteral("Executable name in PATH or full path"));
    auto *browseExecutableButton = new QPushButton(QStringLiteral("Browse..."), this);
    executableRow->addWidget(m_executableEdit, 1);
    executableRow->addWidget(browseExecutableButton);
    connect(browseExecutableButton, &QPushButton::clicked, this, [this]() { browseExecutable(); });

    auto *runtimeRow = new QHBoxLayout;
    m_runtimeExecutableEdit = new QLineEdit(config.runtimeExecutable, this);
    m_runtimeExecutableEdit->setPlaceholderText(QStringLiteral("Optional runtime / interpreter path"));
    auto *browseRuntimeButton = new QPushButton(QStringLiteral("Browse..."), this);
    runtimeRow->addWidget(m_runtimeExecutableEdit, 1);
    runtimeRow->addWidget(browseRuntimeButton);
    connect(browseRuntimeButton, &QPushButton::clicked, this, [this]() { browseRuntimeExecutable(); });

    m_argumentsEdit = new QLineEdit(config.arguments.join(QLatin1Char(' ')), this);
    m_argumentsEdit->setPlaceholderText(QStringLiteral("Arguments separated by spaces"));
    auto *directoryRow = new QHBoxLayout;
    m_workingDirectoryEdit = new QLineEdit(config.workingDirectory, this);
    auto *browseDirectoryButton = new QPushButton(QStringLiteral("Browse..."), this);
    directoryRow->addWidget(m_workingDirectoryEdit, 1);
    directoryRow->addWidget(browseDirectoryButton);
    connect(browseDirectoryButton, &QPushButton::clicked, this, [this]() { browseWorkingDirectory(); });
    m_enabledCheck = new QCheckBox(QStringLiteral("Enable this debugger"), this);
    m_enabledCheck->setChecked(config.id.isEmpty() ? true : config.enabled);

    form->addRow(QStringLiteral("Name:"), m_nameEdit);
    form->addRow(QStringLiteral("Language:"), m_languageCombo);
    form->addRow(QStringLiteral("Adapter executable:"), executableRow);
    form->addRow(QStringLiteral("Runtime / interpreter (optional):"), runtimeRow);
    form->addRow(QStringLiteral("Arguments:"), m_argumentsEdit);
    form->addRow(QStringLiteral("Working directory:"), directoryRow);
    form->addRow(QString(), m_enabledCheck);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() { validateAndAccept(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void DebuggerConfigEditDialog::browseExecutable()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Select Debug Adapter Executable"));
    if (!path.isEmpty())
        m_executableEdit->setText(path);
}

void DebuggerConfigEditDialog::browseRuntimeExecutable()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Select Runtime / Interpreter"));
    if (!path.isEmpty())
        m_runtimeExecutableEdit->setText(path);
}

void DebuggerConfigEditDialog::browseWorkingDirectory()
{
    const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("Select Working Directory"));
    if (!path.isEmpty())
        m_workingDirectoryEdit->setText(path);
}

void DebuggerConfigEditDialog::validateAndAccept()
{
    if (m_nameEdit->text().trimmed().isEmpty() || m_executableEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Missing Information"),
                             QStringLiteral("Enter a debugger name and adapter executable."));
        return;
    }
    const QString executable = m_executableEdit->text().trimmed();
    const bool executableFound =
        (QFileInfo::exists(executable) && QFileInfo(executable).isExecutable()) ||
        !QStandardPaths::findExecutable(executable).isEmpty();
    if (!executableFound) {
        const auto choice = QMessageBox::warning(
            this, QStringLiteral("Executable Not Found"),
            QStringLiteral("The adapter executable '%1' could not be found. Save this configuration anyway?")
                .arg(executable),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (choice != QMessageBox::Yes)
            return;
    }
    accept();
}

DebuggerConfiguration DebuggerConfigEditDialog::configuration() const
{
    DebuggerConfiguration config;
    config.id = m_id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : m_id;
    config.name = m_nameEdit->text().trimmed();
    const int languageIndex = m_languageCombo->currentIndex();
    if (languageIndex >= 0 &&
        m_languageCombo->currentText() == m_languageCombo->itemText(languageIndex) &&
        !m_languageCombo->itemData(languageIndex).toString().isEmpty()) {
        config.languageId = m_languageCombo->itemData(languageIndex).toString();
    } else {
        config.languageId = m_languageCombo->currentText().trimmed();
    }
    config.executable = m_executableEdit->text().trimmed();
    config.runtimeExecutable = m_runtimeExecutableEdit->text().trimmed();
    config.arguments = m_argumentsEdit->text().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    config.workingDirectory = m_workingDirectoryEdit->text().trimmed();
    config.enabled = m_enabledCheck->isChecked();
    return config;
}

DebuggerDialog::DebuggerDialog(const QString &workspacePath, QWidget *parent)
    : QDialog(parent), m_workspacePath(workspacePath),
      m_configurations(DebuggerConfigStore::load(workspacePath))
{
    setWindowTitle(QStringLiteral("Workspace Debuggers"));
    resize(720, 360);
    setStyleSheet(R"(
        QDialog { background-color: #1e1e1e; color: #cccccc; }
        QTableWidget { background-color: #252526; color: #cccccc; gridline-color: #383838; }
        QHeaderView::section { background-color: #333333; color: #dddddd; padding: 4px; border: 1px solid #444444; }
        QPushButton { background-color: #333333; color: #eeeeee; border: 1px solid #484848; padding: 5px 10px; }
        QPushButton:hover { background-color: #ff6b35; color: #ffffff; }
    )");
    auto *layout = new QVBoxLayout(this);
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("Language"),
                                        QStringLiteral("Adapter"), QStringLiteral("Runtime / Interpreter"),
                                        QStringLiteral("Enabled"), QStringLiteral("Working Directory")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_table, 1);

    auto *actions = new QHBoxLayout;
    auto *addButton = new QPushButton(QStringLiteral("Add..."), this);
    m_editButton = new QPushButton(QStringLiteral("Edit..."), this);
    m_removeButton = new QPushButton(QStringLiteral("Remove"), this);
    m_toggleButton = new QPushButton(QStringLiteral("Enable/Disable"), this);
    actions->addWidget(addButton);
    actions->addWidget(m_editButton);
    actions->addWidget(m_toggleButton);
    actions->addWidget(m_removeButton);
    actions->addStretch();
    layout->addLayout(actions);
    auto *closeButton = new QPushButton(QStringLiteral("Close"), this);
    auto *bottom = new QHBoxLayout;
    bottom->addStretch();
    bottom->addWidget(closeButton);
    layout->addLayout(bottom);

    connect(addButton, &QPushButton::clicked, this, [this]() { addConfiguration(); });
    connect(m_editButton, &QPushButton::clicked, this, [this]() { editConfiguration(); });
    connect(m_removeButton, &QPushButton::clicked, this, [this]() { removeConfiguration(); });
    connect(m_toggleButton, &QPushButton::clicked, this, [this]() { toggleEnabled(); });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() {
        const bool selected = selectedRow() >= 0;
        m_editButton->setEnabled(selected);
        m_removeButton->setEnabled(selected);
        m_toggleButton->setEnabled(selected);
    });
    refreshTable();
}

int DebuggerDialog::selectedRow() const
{
    return m_table->currentRow();
}

void DebuggerDialog::refreshTable()
{
    m_table->setRowCount(0);
    for (const DebuggerConfiguration &config : m_configurations) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(config.name));
        m_table->setItem(row, 1, new QTableWidgetItem(config.languageId));
        m_table->setItem(row, 2, new QTableWidgetItem(config.executable));
        m_table->setItem(row, 3, new QTableWidgetItem(config.runtimeExecutable));
        m_table->setItem(row, 4, new QTableWidgetItem(config.enabled ? QStringLiteral("Yes") : QStringLiteral("No")));
        m_table->setItem(row, 5, new QTableWidgetItem(config.workingDirectory));
    }
    const bool selected = selectedRow() >= 0;
    m_editButton->setEnabled(selected);
    m_removeButton->setEnabled(selected);
    m_toggleButton->setEnabled(selected);
}

void DebuggerDialog::persist()
{
    if (!DebuggerConfigStore::save(m_workspacePath, m_configurations))
        QMessageBox::warning(this, QStringLiteral("Save Failed"),
                             QStringLiteral("Could not save debugger settings for this workspace."));
}

void DebuggerDialog::addConfiguration()
{
    DebuggerConfigEditDialog dialog({}, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_configurations.append(dialog.configuration());
    persist();
    refreshTable();
}

void DebuggerDialog::editConfiguration()
{
    const int row = selectedRow();
    if (row < 0 || row >= m_configurations.size())
        return;
    DebuggerConfigEditDialog dialog(m_configurations.at(row), this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_configurations[row] = dialog.configuration();
    persist();
    refreshTable();
    m_table->selectRow(row);
}

void DebuggerDialog::removeConfiguration()
{
    const int row = selectedRow();
    if (row < 0 || row >= m_configurations.size())
        return;
    m_configurations.removeAt(row);
    persist();
    refreshTable();
}

void DebuggerDialog::toggleEnabled()
{
    const int row = selectedRow();
    if (row < 0 || row >= m_configurations.size())
        return;
    m_configurations[row].enabled = !m_configurations.at(row).enabled;
    persist();
    refreshTable();
    m_table->selectRow(row);
}

} // namespace CinderDE
