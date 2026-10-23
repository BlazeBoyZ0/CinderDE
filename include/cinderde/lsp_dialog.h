#ifndef CINDERDE_LSP_DIALOG_H
#define CINDERDE_LSP_DIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextEdit>
#include <QLabel>
#include <QTimer>
#include "cinderde/lsp_types.h"

namespace CinderDE {

class LspConfigEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit LspConfigEditDialog(const LspServerConfig &config = LspServerConfig(), QWidget *parent = nullptr);
    LspServerConfig config() const;

private slots:
    void browseExecutable();
    void validateAndAccept();

private:
    QLineEdit *m_nameEdit;
    QLineEdit *m_languageIdEdit;
    QLineEdit *m_extensionsEdit;
    QLineEdit *m_executableEdit;
    QLineEdit *m_argumentsEdit;
    QLineEdit *m_envEdit;
    QCheckBox *m_enabledCB;

    QString m_configId;

    void setupUi();
};

class LspDialog : public QDialog {
    Q_OBJECT

public:
    explicit LspDialog(QWidget *parent = nullptr);
    ~LspDialog() override;

private slots:
    void addConfig();
    void editConfig();
    void removeConfig();
    void toggleEnabled();
    void startServer();
    void stopServer();
    void refreshTable();
    void onTableSelectionChanged();

private:
    QTableWidget *m_tableWidget;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_removeBtn;
    QPushButton *m_enableBtn;
    QPushButton *m_startBtn;
    QPushButton *m_stopBtn;
    QLabel *m_statusDetailsLabel;
    QTextEdit *m_logEdit;
    QTimer *m_refreshTimer;

    void setupUi();
    void updateButtons();
    LspServerConfig selectedConfig() const;
    int selectedRow() const;
    void applyTheme();
};

} // namespace CinderDE

#endif // CINDERDE_LSP_DIALOG_H
