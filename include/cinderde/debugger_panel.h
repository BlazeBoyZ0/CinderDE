#ifndef CINDERDE_DEBUGGER_PANEL_H
#define CINDERDE_DEBUGGER_PANEL_H

#include <QWidget>
#include <QJsonArray>
#include <QHash>

class QTreeWidget;
class QTreeWidgetItem;

namespace CinderDE {

class DebuggerPanel : public QWidget {
    Q_OBJECT
public:
    explicit DebuggerPanel(QWidget *parent = nullptr);
    void setStackFrames(const QJsonArray &frames);
    void setScopes(const QJsonArray &scopes);
    void populateVariables(int parentReference, const QString &scopeName, const QJsonArray &variables);

signals:
    void frameSelected(int frameId, const QString &path, int line);
    void expandRequested(int variablesReference, const QString &name);

private:
    QTreeWidget *m_stackTree;
    QTreeWidget *m_variablesTree;
    QHash<int, QTreeWidgetItem *> m_referenceItems;

    void onVariableExpanded(QTreeWidgetItem *item);
    QTreeWidgetItem *findVariableItem(int reference) const;
};

} // namespace CinderDE

#endif // CINDERDE_DEBUGGER_PANEL_H
