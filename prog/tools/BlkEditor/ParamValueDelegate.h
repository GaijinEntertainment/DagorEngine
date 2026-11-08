#pragma once

#include <QHash>
#include <QStyledItemDelegate>
#include <QStringList>

class ParamValueDelegate : public QStyledItemDelegate
{
public:
  explicit ParamValueDelegate(QObject *parent = nullptr);

  void setGlobalSuggestions(const QStringList &suggestions);
  void setParamSpecificSuggestions(const QHash<QString, QStringList> &suggestions);
  void setIncludeSuggestions(const QStringList &suggestions);

protected:
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
  QString normalizedKey(const QString &name) const;
  QStringList globalSuggestions;
  QHash<QString, QStringList> paramSpecific;
  QStringList includeSuggestions;
};
