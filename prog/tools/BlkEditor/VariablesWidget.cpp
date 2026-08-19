#include "VariablesWidget.h"

#include <QAbstractItemView>
#include <QFont>
#include <QHeaderView>
#include <QLineEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include "BlkEditorRoles.h"

#include <utility>

namespace
{
constexpr int VariableNameRole = Qt::UserRole;
}

VariablesWidget::VariablesWidget(QWidget *parent) : QWidget(parent)
{
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(4);

  filterEdit = new QLineEdit(this);
  filterEdit->setPlaceholderText(tr("Filter variables"));
  layout->addWidget(filterEdit);

  tree = new QTreeWidget(this);
  tree->setColumnCount(3);
  tree->setHeaderLabels({tr("Name"), tr("Type"), tr("Used In")});
  tree->setRootIsDecorated(false);
  tree->setUniformRowHeights(true);
  tree->setAlternatingRowColors(true);
  tree->setSelectionMode(QAbstractItemView::SingleSelection);
  tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
  tree->header()->setStretchLastSection(true);
  tree->setSortingEnabled(true);
  tree->sortByColumn(0, Qt::AscendingOrder);
  layout->addWidget(tree);

  connect(filterEdit, &QLineEdit::textChanged, this, &VariablesWidget::applyFilter);
  connect(tree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item, int) {
    if (!activationCallback)
      return;
    const QString name = item ? item->data(0, VariableNameRole).toString() : QString();
    if (!name.isEmpty())
      activationCallback(name);
  });
}

void VariablesWidget::updateVariables(const QVector<VariableRow> &rows, const QString &currentBlockPath)
{
  cachedRows = rows;
  contextPath = currentBlockPath;
  rebuildTree();
}

void VariablesWidget::setCurrentBlockPath(const QString &currentBlockPath)
{
  if (contextPath == currentBlockPath)
    return;

  contextPath = currentBlockPath;
  updateHighlights();
}

void VariablesWidget::clearContents()
{
  cachedRows.clear();
  tree->clear();
  filterEdit->clear();
  contextPath.clear();
}

void VariablesWidget::setActivationCallback(ActivationCallback callback)
{
  activationCallback = std::move(callback);
}

void VariablesWidget::rebuildTree()
{
  tree->clear();

  for (const VariableRow &row : cachedRows)
  {
    const bool matchesFilter = activeFilter.isEmpty() || row.name.contains(activeFilter, Qt::CaseInsensitive) ||
                               row.type.contains(activeFilter, Qt::CaseInsensitive);
    if (!matchesFilter)
      continue;

    auto *item = new QTreeWidgetItem(tree);
    item->setText(0, row.name);
    item->setText(1, row.type);

    const int usageCount = row.blockPaths.size();
    if (usageCount > 0)
    {
      item->setText(2, tr("%1 block%2").arg(usageCount).arg(usageCount == 1 ? QString() : QStringLiteral("s")));
      item->setToolTip(2, row.blockPaths.join(QStringLiteral("\n")));
    }
    else
    {
      item->setText(2, tr("<unknown>"));
    }

    item->setData(0, VariableNameRole, row.name);
    item->setData(0, BlkEditorRoles::VariableBlockList, row.blockPaths);
  }

  tree->header()->resizeSections(QHeaderView::ResizeToContents);
  tree->sortItems(0, Qt::AscendingOrder);
  updateHighlights();
}

void VariablesWidget::applyFilter(const QString &filterText)
{
  activeFilter = filterText.trimmed();
  rebuildTree();
}

void VariablesWidget::updateHighlights()
{
  for (int i = 0; i < tree->topLevelItemCount(); ++i)
  {
    auto *item = tree->topLevelItem(i);
    const QStringList blocks = item->data(0, BlkEditorRoles::VariableBlockList).toStringList();
    const bool highlight = !contextPath.isEmpty() && blocks.contains(contextPath);
    QFont font = item->font(0);
    font.setBold(highlight);
    item->setFont(0, font);
    item->setFont(1, font);
    item->setFont(2, font);
  }
}
