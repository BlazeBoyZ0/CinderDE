#ifndef CINDERDE_BUILD_RUN_DIALOG_H
#define CINDERDE_BUILD_RUN_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "cinderde/workspace_config.h"

namespace CinderDE {

class BuildRunDialog : public QDialog {
    Q_OBJECT

public:
    explicit BuildRunDialog(const QString &workspacePath, const WorkspaceConfig &config, QWidget *parent = nullptr);

    WorkspaceConfig config() const;
    QString executionMode() const;
    QString terminalCommand() const;

private slots:
    void onAutoDetectTerminal();

private:
    QLineEdit *m_buildCommandEdit;
    QLineEdit *m_runCommandEdit;
    QLineEdit *m_workingDirEdit;
    QComboBox *m_executionModeCombo;
    QLineEdit *m_terminalCommandEdit;
    QString m_workspacePath;

    void setupUi(const WorkspaceConfig &config);
    void applyTheme();
};

} // namespace CinderDE

#endif // CINDERDE_BUILD_RUN_DIALOG_H
