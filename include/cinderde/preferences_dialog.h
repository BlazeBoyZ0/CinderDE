#ifndef CINDERDE_PREFERENCES_DIALOG_H
#define CINDERDE_PREFERENCES_DIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QTabWidget>

namespace CinderDE {

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

    void populateLanguagePacksTable();

private slots:
    void openLanguagePacksFolder();
    void applySettings();

private:
    QTabWidget *m_tabWidget;

    // Editor settings controls
    QSpinBox *m_fontSizeSpin;
    QSpinBox *m_tabSizeSpin;
    QCheckBox *m_wordWrapCB;
    QComboBox *m_executionModeCombo;
    QLineEdit *m_terminalCommandEdit;

    // Language Packs table
    QTableWidget *m_packsTable;
    QPushButton *m_openPacksFolderBtn;

    void setupUi();
    void setupEditorTab();
    void setupLanguagePacksTab();
    void applyTheme();
};

} // namespace CinderDE

#endif // CINDERDE_PREFERENCES_DIALOG_H
