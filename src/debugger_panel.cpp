#include "cinderde/debugger_panel.h"

#include <QLabel>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QSplitter>
#include <QHeaderView>

namespace CinderDE {
namespace {
constexpr int FrameIdRole = Qt::UserRole;
constexpr int PathRole = Qt::UserRole + 1;
constexpr int LineRole = Qt::UserRole + 2;
constexpr int ReferenceRole = Qt::UserRole;
constexpr int LoadedRole = Qt::UserRole + 1;
constexpr int PlaceholderRole = Qt::UserRole + 2;
}

DebuggerPanel::DebuggerPanel(QWidget *parent)
    : QWidget(parent), m_stackTree(new QTreeWidget(this)), m_variablesTree(new QTreeWidget(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    auto *splitter = new QSplitter(Qt::Vertical, this);

    auto *stackSection = new QWidget(splitter);
    auto *stackLayout = new QVBoxLayout(stackSection);
    stackLayout->setContentsMargins(0, 0, 0, 0);
    auto *stackLabel = new QLabel(QStringLiteral("CALL STACK"), stackSection);
    stackLabel->setStyleSheet(QStringLiteral("color: #ff8a50; font-weight: bold; padding: 3px;"));
    m_stackTree->setColumnCount(2);
    m_stackTree->setHeaderLabels({QStringLiteral("Frame"), QStringLiteral("Location")});
    m_stackTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_stackTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_stackTree->setRootIsDecorated(false);
    stackLayout->addWidget(stackLabel);
    stackLayout->addWidget(m_stackTree);

    auto *variablesSection = new QWidget(splitter);
    auto *variablesLayout = new QVBoxLayout(variablesSection);
    variablesLayout->setContentsMargins(0, 0, 0, 0);
    auto *variablesLabel = new QLabel(QStringLiteral("VARIABLES"), variablesSection);
    variablesLabel->setStyleSheet(QStringLiteral("color: #ff8a50; font-weight: bold; padding: 3px;"));
    m_variablesTree->setColumnCount(2);
    m_variablesTree->setHeaderLabels({QStringLiteral("Name"), QStringLiteral("Value")});
    m_variablesTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_variablesTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    variablesLayout->addWidget(variablesLabel);
    variablesLayout->addWidget(m_variablesTree);

    splitter->addWidget(stackSection);
    splitter->addWidget(variablesSection);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter);

    setStyleSheet(R"(
        QWidget { background-color: #1e1e1e; color: #cccccc; }
        QTreeWidget { background-color: #252526; color: #dddddd; border: 1px solid #383838; }
        QTreeWidget::item:selected { background-color: #454545; }
        QHeaderView::section { background-color: #333333; color: #dddddd; padding: 4px; border: 1px solid #444444; }
    )");

    connect(m_stackTree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
        emit frameSelected(item->data(0, FrameIdRole).toInt(),
                           item->data(0, PathRole).toString(),
                           item->data(0, LineRole).toInt());
    });
    connect(m_variablesTree, &QTreeWidget::itemExpanded, this, &DebuggerPanel::onVariableExpanded);
}

void DebuggerPanel::setStackFrames(const QJsonArray &frames)
{
    m_stackTree->clear();
    for (const QJsonValue &value : frames) {
        const QJsonObject frame = value.toObject();
        auto *item = new QTreeWidgetItem(m_stackTree);
        item->setText(0, frame.value(QStringLiteral("name")).toString(QStringLiteral("<frame>")));
        const QString path = frame.value(QStringLiteral("source")).toObject().value(QStringLiteral("path")).toString();
        const int line = frame.value(QStringLiteral("line")).toInt();
        item->setText(1, QStringLiteral("%1:%2").arg(path).arg(line));
        item->setData(0, FrameIdRole, frame.value(QStringLiteral("id")).toInt());
        item->setData(0, PathRole, path);
        item->setData(0, LineRole, line);
    }
}

void DebuggerPanel::setScopes(const QJsonArray &scopes)
{
    m_variablesTree->clear();
    m_referenceItems.clear();
    QTreeWidgetItem *firstLocals = nullptr;
    for (const QJsonValue &value : scopes) {
        const QJsonObject scope = value.toObject();
        const int reference = scope.value(QStringLiteral("variablesReference")).toInt();
        auto *item = new QTreeWidgetItem(m_variablesTree);
        const QString name = scope.value(QStringLiteral("name")).toString(QStringLiteral("Variables"));
        item->setText(0, name);
        item->setText(1, scope.value(QStringLiteral("expensive")).toBool()
                             ? QStringLiteral("Expand to load")
                             : QStringLiteral("Loading..."));
        item->setData(0, ReferenceRole, reference);
        item->setData(0, LoadedRole, false);
        m_referenceItems.insert(reference, item);
        if (reference > 0) {
            auto *placeholder = new QTreeWidgetItem(item);
            placeholder->setText(0, QStringLiteral("Expand to load"));
            placeholder->setData(0, PlaceholderRole, true);
        }
        if (!firstLocals && reference > 0 &&
            name.compare(QStringLiteral("Locals"), Qt::CaseInsensitive) == 0 &&
            !scope.value(QStringLiteral("expensive")).toBool())
            firstLocals = item;
    }
    if (firstLocals) {
        firstLocals->setData(0, LoadedRole, true);
        emit expandRequested(firstLocals->data(0, ReferenceRole).toInt(), firstLocals->text(0));
    }
}

QTreeWidgetItem *DebuggerPanel::findVariableItem(int reference) const
{
    return m_referenceItems.value(reference, nullptr);
}

void DebuggerPanel::populateVariables(int parentReference, const QString &, const QJsonArray &variables)
{
    QTreeWidgetItem *parent = findVariableItem(parentReference);
    if (!parent)
        return;

    parent->takeChildren();
    parent->setData(0, LoadedRole, true);
    if (variables.isEmpty()) {
        parent->setText(1, QStringLiteral("(empty)"));
        return;
    }

    for (const QJsonValue &value : variables) {
        const QJsonObject variable = value.toObject();
        auto *item = new QTreeWidgetItem(parent);
        item->setText(0, variable.value(QStringLiteral("name")).toString());
        item->setText(1, variable.value(QStringLiteral("value")).toString());
        const int reference = variable.value(QStringLiteral("variablesReference")).toInt();
        item->setData(0, ReferenceRole, reference);
        item->setData(0, LoadedRole, false);
        if (reference > 0) {
            auto *placeholder = new QTreeWidgetItem(item);
            placeholder->setText(0, QStringLiteral("Expand to load"));
            placeholder->setData(0, PlaceholderRole, true);
            m_referenceItems.insert(reference, item);
        }
    }
    parent->setExpanded(true);
}

void DebuggerPanel::onVariableExpanded(QTreeWidgetItem *item)
{
    if (!item || item->data(0, LoadedRole).toBool())
        return;
    const int reference = item->data(0, ReferenceRole).toInt();
    if (reference <= 0)
        return;
    item->setData(0, LoadedRole, true);
    item->takeChildren();
    emit expandRequested(reference, item->text(0));
}

} // namespace CinderDE
