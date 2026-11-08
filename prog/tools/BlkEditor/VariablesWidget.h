#pragma once

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>

class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

class VariablesWidget : public QWidget
{
public:
  using ActivationCallback = std::function<void(const QString &)>;

  struct VariableRow
  {
    QString name;
    QString type;
    QStringList blockPaths;
  };

  explicit VariablesWidget(QWidget *parent = nullptr);

  void updateVariables(const QVector<VariableRow> &rows, const QString &currentBlockPath);
  void setCurrentBlockPath(const QString &currentBlockPath);
  void clearContents();
  void setActivationCallback(ActivationCallback callback);

private:
  void rebuildTree();
  void applyFilter(const QString &filterText);
  void updateHighlights();

  QVector<VariableRow> cachedRows;
  QString contextPath;
  QString activeFilter;
  ActivationCallback activationCallback;

  QLineEdit *filterEdit = nullptr;
  QTreeWidget *tree = nullptr;
};
