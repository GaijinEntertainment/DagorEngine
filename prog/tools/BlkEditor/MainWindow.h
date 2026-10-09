#pragma once

#include <QtCore/Qt>
#include <QHash>
#include <QMainWindow>
#include <QSet>
#include <QHash>
#include <QMainWindow>
#include <QSet>
#include <QString>
#include <QStringList>

class QAction;
class QCloseEvent;
class QDockWidget;
class QMenu;
class QPlainTextEdit;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QToolBar;
class QCompleter;
class QStringListModel;
class QTableWidgetItem;

#include "BlkEditorRoles.h"

class DataBlock;
class ParamValueDelegate;
class VariablesWidget;
class BlkCodeEditor;
class BlkSyntaxHighlighter;

class MainWindow : public QMainWindow
{
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

  void closeEvent(QCloseEvent *event) override;

private:
  void createActions();
  void createUi();
  void openFile();
  void save();
  void saveAs();
  bool saveDocument();
  bool saveDocumentAs();
  bool maybeSave();
  void loadDocument(const QString &filePath);
  void clearDocument();
  void populateTree();
  QTreeWidgetItem *createTreeItem(DataBlock &block, const QString &blockPath);
  QString buildChildPath(const QString &parentPath, const DataBlock &child) const;
  void onCurrentBlockChanged(QTreeWidgetItem *current);
  void displayBlockParameters(DataBlock *block);
  void onParamItemChanged(QTableWidgetItem *item);
  QString paramTypeToString(int type) const;
  QString valueToDisplayString(DataBlock &block, std::uint32_t paramIndex) const;
  bool applyValueChange(DataBlock &block, std::uint32_t paramIndex, int type, const QString &valueText);
  bool isEditableType(int type) const;
  void markDocumentAsModified();
  void updateWindowTitle();
  void createDockWidgets();
  void createTextEditorDock();
  QWidget *buildParameterPanel();
  void showAboutDialog();
  void updateTextEditorActions();
  void refreshTextEditorFromDocument(bool preserveView);
  void loadTextEditorFromFile(const QString &filePath);
  QString serializeDocumentToText() const;
  bool applyTextEditorChanges();
  bool ensureTextEditorReadyForDocumentMutation();
  bool ensureTextEditorAppliedBeforeSave();
  void setTextEditorContent(const QString &text, bool preserveView);
  void onTextEditorTextChanged();
  void rebuildVariableCatalog();
  void collectVariables(DataBlock &block, const QString &currentPath);
  void collectExternalVariables();
  void collectVariablesFromFile(const QString &filePath, const QString &virtualRootLabel);
  void collectVariablesFromDirectory(const QString &rootDir, int maxFiles);
  void onVariableActivated(const QString &name);
  void addParameterInteractive();
  void addBlockInteractive();
  void removeSelectedParameter();
  bool addParameterToBlock(DataBlock &block, const QString &name, int type, const QString &valueText);
  void selectBlockByPath(const QString &blockPath);
  QTreeWidgetItem *findTreeItemByPath(const QString &blockPath, QTreeWidgetItem *root) const;
  void rebuildTextEditorCompletions();
  void rebuildIncludeSuggestions();
  void ensureTextEditorHasHighlighter();
  QStringList gatherDirectiveTokens() const;
  QStringList gatherTypeTokens() const;
  void onParamSelectionChanged();
  void updateEditingActionStates();

  std::unique_ptr<DataBlock> blkDocument;
  QString currentFilePath;
  QString currentBlockPath = QStringLiteral("/");

  QTreeWidget *blockTree = nullptr;
  QTableWidget *paramTable = nullptr;
  QWidget *paramPanel = nullptr;
  QToolBar *paramToolbar = nullptr;
  QDockWidget *variablesDock = nullptr;
  VariablesWidget *variablesWidget = nullptr;
  QDockWidget *textEditorDock = nullptr;
  BlkCodeEditor *textEditor = nullptr;
  std::unique_ptr<BlkSyntaxHighlighter> syntaxHighlighter;
  ParamValueDelegate *valueDelegate = nullptr;
  QMenu *viewMenu = nullptr;
  QCompleter *textCompleter = nullptr;
  QStringListModel *textCompletionModel = nullptr;

  QAction *openAction = nullptr;
  QAction *saveAction = nullptr;
  QAction *saveAsAction = nullptr;
  QAction *aboutAction = nullptr;
  QAction *applyTextChangesAction = nullptr;
  QAction *reloadTextFromDocumentAction = nullptr;
  QAction *addParamAction = nullptr;
  QAction *addBlockAction = nullptr;
  QAction *removeParamAction = nullptr;

  bool updatingParamTable = false;
  bool documentModified = false;
  bool updatingTextEditor = false;
  bool textEditorDirty = false;

  struct VariableUsageInfo
  {
    QString typeLabel;
    QSet<QString> blockPaths;
    bool mixedType = false;
    QSet<QString> sampleValues;
    bool sampleOverflow = false;
  };

  QHash<QString, VariableUsageInfo> variableUsage;
  QStringList variableCompletions;
  QStringList includeSuggestions;
  QStringList textEditorKeywordPool;
  QStringList directiveTokens;
  QStringList typeTokens;
  QHash<QString, QStringList> paramSpecificSuggestions;
  QSet<QString> scannedVariableFiles;
  const int maxEnumSamples = 16;
  const int maxExternalFiles = 80;
};
