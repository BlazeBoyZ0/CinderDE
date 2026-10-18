#ifndef CINDERDE_DEBUGGER_DIALOG_H
#define CINDERDE_DEBUGGER_DIALOG_H

#include <QDialog>
#include <QList>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include "cinderde/debugger_config.h"

namespace CinderDE {

class DebuggerConfigEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit DebuggerConfigEditDialog(const DebuggerConfiguration &config = DebuggerConfiguration(),
                                      QWidget *parent = nullptr);
    DebuggerConfiguration configuration() const;

private:
    QLineEdit *m_nameEdit;
    QComboBox *m_languageCombo;
    QLineEdit *m_executableEdit;
    QLineEdit *m_runtimeExecutableEdit;
    QLineEdit *m_argumentsEdit;
    QLineEdit *m_extensionsEdit;
    QLineEdit *m_launchArgumentsEdit;
    QLineEdit *m_workingDirectoryEdit;
    QCheckBox *m_enabledCheck;
    QString m_id;
    void browseExecutable();
    void browseRuntimeExecutable();
    void browseWorkingDirectory();
    void validateAndAccept();
};

class DebuggerDialog : public QDialog {
    Q_OBJECT
public:
    explicit DebuggerDialog(const QString &workspacePath, QWidget *parent = nullptr);

private slots:
    void addConfiguration();
    void editConfiguration();
    void removeConfiguration();
    void toggleEnabled();

private:
    QString m_workspacePath;
    QList<DebuggerConfiguration> m_configurations;
    QTableWidget *m_table;
    QPushButton *m_editButton;
    QPushButton *m_removeButton;
    QPushButton *m_toggleButton;
    void refreshTable();
    void persist();
    int selectedRow() const;
};

} // namespace CinderDE

#endif // CINDERDE_DEBUGGER_DIALOG_H
