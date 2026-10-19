#include "ParamValueDelegate.h"

#include <QCompleter>
#include <QLineEdit>

#include <algorithm>

#include <ioSys/dag_dataBlock.h>

#include "BlkEditorRoles.h"

namespace
{
const QStringList kBoolSuggestions = {QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("1"), QStringLiteral("0")};
}

ParamValueDelegate::ParamValueDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void ParamValueDelegate::setGlobalSuggestions(const QStringList &suggestions)
{
  globalSuggestions = suggestions;
}

void ParamValueDelegate::setParamSpecificSuggestions(const QHash<QString, QStringList> &suggestions)
{
  paramSpecific = suggestions;
}

void ParamValueDelegate::setIncludeSuggestions(const QStringList &suggestions)
{
  includeSuggestions = suggestions;
}

QWidget *ParamValueDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QWidget *editor = QStyledItemDelegate::createEditor(parent, option, index);
  auto *lineEdit = qobject_cast<QLineEdit *>(editor);
  if (!lineEdit)
    return editor;

  QStringList completions;
  const int paramType = index.data(BlkEditorRoles::ParamType).toInt();
  if (paramType == DataBlock::TYPE_BOOL)
    completions.append(kBoolSuggestions);

  completions.append(globalSuggestions);

  const QString paramName = index.data(BlkEditorRoles::ParamName).toString();
  const QString key = normalizedKey(paramName);
  if (!key.isEmpty())
  {
    if (const auto it = paramSpecific.constFind(key); it != paramSpecific.constEnd())
      completions.append(it.value());

    if (paramType == DataBlock::TYPE_STRING)
    {
      if (key.contains(QStringLiteral("include")) || key.contains(QStringLiteral("file")) || key.contains(QStringLiteral("path")))
        completions.append(includeSuggestions);
    }
  }

  completions.removeAll(QString());
  completions.removeDuplicates();
  std::sort(completions.begin(), completions.end(), [](const QString &lhs, const QString &rhs) {
    return lhs.compare(rhs, Qt::CaseInsensitive) < 0;
  });

  if (!completions.isEmpty())
  {
    auto *completer = new QCompleter(completions, lineEdit);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    lineEdit->setCompleter(completer);
  }

  return lineEdit;
}

QString ParamValueDelegate::normalizedKey(const QString &name) const
{
  QString key = name.trimmed().toLower();
  return key;
}
