#include "MainWindow.h"

#include <QAbstractItemView>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QScrollBar>
#include <QSplitter>
#include <QStatusBar>
#include <QStringList>
#include <QStringListModel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVariant>
#include <QCompleter>
#include <QVBoxLayout>
#include <QTextCursor>

#include <algorithm>

#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>

#include <cstdint>

#include "BlkCodeEditor.h"
#include "BlkEditorRoles.h"
#include "BlkSyntaxHighlighter.h"
#include "ParamValueDelegate.h"
#include "VariablesWidget.h"

namespace
{
QString blockDisplayName(const DataBlock &block)
{
  const char *name = block.getBlockName();
  if (name && *name)
    return QString::fromUtf8(name);
  return QObject::tr("<root>");
}

QString formatPoint(const Point2 &value)
{
  return QStringLiteral("(%1, %2)").arg(value.x).arg(value.y);
}

QString formatPoint(const Point3 &value)
{
  return QStringLiteral("(%1, %2, %3)").arg(value.x).arg(value.y).arg(value.z);
}

QString formatPoint(const Point4 &value)
{
  return QStringLiteral("(%1, %2, %3, %4)").arg(value.x).arg(value.y).arg(value.z).arg(value.w);
}

QString formatPoint(const IPoint2 &value)
{
  return QStringLiteral("(%1, %2)").arg(value.x).arg(value.y);
}

QString formatPoint(const IPoint3 &value)
{
  return QStringLiteral("(%1, %2, %3)").arg(value.x).arg(value.y).arg(value.z);
}

QString formatPoint(const IPoint4 &value)
{
  return QStringLiteral("(%1, %2, %3, %4)").arg(value.x).arg(value.y).arg(value.z).arg(value.w);
}

QString formatMatrix(const TMatrix &value)
{
  QStringList rows;
  for (int i = 0; i < 4; ++i)
  {
    rows.append(QStringLiteral("[%1, %2, %3]").arg(value.m[i][0]).arg(value.m[i][1]).arg(value.m[i][2]));
  }
  return rows.join(QStringLiteral("; "));
}

QString formatColor(const E3DCOLOR &value)
{
  return QStringLiteral("rgba(%1, %2, %3, %4)").arg(int(value.r)).arg(int(value.g)).arg(int(value.b)).arg(int(value.a));
}

struct ParamTypeOption
{
  const char *label;
  int type;
};

const ParamTypeOption kParamTypeOptions[] = {
  {"String", DataBlock::TYPE_STRING},  {"Integer", DataBlock::TYPE_INT},      {"Float", DataBlock::TYPE_REAL},
  {"Bool", DataBlock::TYPE_BOOL},      {"Int64", DataBlock::TYPE_INT64},      {"Color", DataBlock::TYPE_E3DCOLOR},
  {"Point2", DataBlock::TYPE_POINT2},  {"Point3", DataBlock::TYPE_POINT3},    {"Point4", DataBlock::TYPE_POINT4},
  {"IPoint2", DataBlock::TYPE_IPOINT2},{"IPoint3", DataBlock::TYPE_IPOINT3},  {"IPoint4", DataBlock::TYPE_IPOINT4},
  {"Matrix", DataBlock::TYPE_MATRIX}};

QString paramSummary(const DataBlock &block)
{
  return QObject::tr("%1 blocks, %2 params").arg(block.blockCount()).arg(block.paramCount());
}

QStringList splitComponents(const QString &text)
{
  QString cleanedInput = text.trimmed();
  int firstParen = cleanedInput.indexOf('(');
  int firstBracket = cleanedInput.indexOf('[');

  int start = -1;
  if (firstParen >= 0)
    start = firstParen;
  if (firstBracket >= 0)
    start = (start < 0) ? firstBracket : std::min(start, firstBracket);
  if (start > 0)
    cleanedInput = cleanedInput.mid(start);

  QStringList components;
  QString current;
  for (QChar ch : cleanedInput)
  {
    if (ch == '(' || ch == ')' || ch == '[' || ch == ']')
      continue;

    if (ch == ',' || ch == ';' || ch.isSpace())
    {
      if (!current.isEmpty())
      {
        components.push_back(current);
        current.clear();
      }
    }
    else
    {
      current.append(ch);
    }
  }

  if (!current.isEmpty())
    components.push_back(current);

  return components;
}

bool parseFloatComponents(const QString &text, int expectedCount, float *outValues)
{
  const QStringList parts = splitComponents(text);
  if (parts.size() != expectedCount)
    return false;

  for (int i = 0; i < expectedCount; ++i)
  {
    bool ok = false;
    const float value = parts[i].toFloat(&ok);
    if (!ok)
      return false;
    outValues[i] = value;
  }

  return true;
}

bool parseIntComponents(const QString &text, int expectedCount, int *outValues)
{
  const QStringList parts = splitComponents(text);
  if (parts.size() != expectedCount)
    return false;

  for (int i = 0; i < expectedCount; ++i)
  {
    bool ok = false;
    const int value = parts[i].toInt(&ok);
    if (!ok)
      return false;
    outValues[i] = value;
  }

  return true;
}
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
  directiveTokens = gatherDirectiveTokens();
  typeTokens = gatherTypeTokens();
  createActions();
  createUi();
  createDockWidgets();
  ensureTextEditorHasHighlighter();
  updateWindowTitle();
  rebuildVariableCatalog();
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *event)
{
  if (!maybeSave())
  {
    event->ignore();
    return;
  }
  QMainWindow::closeEvent(event);
}

void MainWindow::createActions()
{
  aboutAction = new QAction(QObject::tr("&About BlkEditor"), this);
  connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

  openAction = new QAction(QObject::tr("&Open..."), this);
  openAction->setShortcut(QKeySequence::Open);
  connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

  saveAction = new QAction(QObject::tr("&Save"), this);
  saveAction->setShortcut(QKeySequence::Save);
  connect(saveAction, &QAction::triggered, this, &MainWindow::save);
  saveAction->setEnabled(false);

  saveAsAction = new QAction(QObject::tr("Save &As..."), this);
  saveAsAction->setShortcut(QKeySequence::SaveAs);
  connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveAs);
  saveAsAction->setEnabled(false);

  applyTextChangesAction = new QAction(QObject::tr("Apply Text Changes"), this);
  applyTextChangesAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
  applyTextChangesAction->setEnabled(false);
  connect(applyTextChangesAction, &QAction::triggered, this, [this]() { applyTextEditorChanges(); });

  reloadTextFromDocumentAction = new QAction(QObject::tr("Reload Text From Document"), this);
  reloadTextFromDocumentAction->setEnabled(false);
  connect(reloadTextFromDocumentAction, &QAction::triggered, this, [this]() { refreshTextEditorFromDocument(false); });

  addParamAction = new QAction(QObject::tr("Add Parameter"), this);
  addParamAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_P));
  addParamAction->setEnabled(false);
  connect(addParamAction, &QAction::triggered, this, &MainWindow::addParameterInteractive);

  addBlockAction = new QAction(QObject::tr("Add Child Block"), this);
  addBlockAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_B));
  addBlockAction->setEnabled(false);
  connect(addBlockAction, &QAction::triggered, this, &MainWindow::addBlockInteractive);

  removeParamAction = new QAction(QObject::tr("Remove Parameter"), this);
  removeParamAction->setShortcut(QKeySequence::Delete);
  removeParamAction->setEnabled(false);
  connect(removeParamAction, &QAction::triggered, this, &MainWindow::removeSelectedParameter);
}

void MainWindow::createUi()
{
  auto *fileMenu = menuBar()->addMenu(QObject::tr("&File"));
  fileMenu->addAction(openAction);
  fileMenu->addSeparator();
  fileMenu->addAction(saveAction);
  fileMenu->addAction(saveAsAction);
  fileMenu->addSeparator();
  auto *exitAction = fileMenu->addAction(QObject::tr("E&xit"));
  connect(exitAction, &QAction::triggered, this, &QWidget::close);

  auto *editMenu = menuBar()->addMenu(QObject::tr("&Edit"));
  editMenu->addAction(applyTextChangesAction);
  editMenu->addAction(reloadTextFromDocumentAction);
  editMenu->addSeparator();
  editMenu->addAction(addParamAction);
  editMenu->addAction(addBlockAction);
  editMenu->addAction(removeParamAction);

  viewMenu = menuBar()->addMenu(QObject::tr("&View"));
  auto *helpMenu = menuBar()->addMenu(QObject::tr("&Help"));
  helpMenu->addAction(aboutAction);

  auto *fileToolbar = addToolBar(QObject::tr("File"));
  fileToolbar->setMovable(false);
  fileToolbar->addAction(openAction);
  fileToolbar->addAction(saveAction);

  blockTree = new QTreeWidget(this);
  blockTree->setColumnCount(2);
  blockTree->setHeaderLabels({QObject::tr("Block"), QObject::tr("Summary")});
  blockTree->setUniformRowHeights(true);
  blockTree->setSelectionMode(QAbstractItemView::SingleSelection);
  blockTree->setAlternatingRowColors(true);

  auto *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->addWidget(blockTree);
  splitter->addWidget(buildParameterPanel());
  splitter->setStretchFactor(0, 2);
  splitter->setStretchFactor(1, 3);
  setCentralWidget(splitter);

  connect(blockTree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
    onCurrentBlockChanged(current);
  });

  statusBar()->showMessage(QObject::tr("Ready"));
  resize(1024, 768);
}

void MainWindow::createDockWidgets()
{
  createTextEditorDock();

  variablesWidget = new VariablesWidget(this);
  variablesDock = new QDockWidget(QObject::tr("Variables"), this);
  variablesDock->setObjectName(QStringLiteral("VariablesDock"));
  variablesDock->setAllowedAreas(Qt::AllDockWidgetAreas);
  variablesDock->setWidget(variablesWidget);
  addDockWidget(Qt::RightDockWidgetArea, variablesDock);

  if (viewMenu && variablesDock)
    viewMenu->addAction(variablesDock->toggleViewAction());

  variablesWidget->setActivationCallback([this](const QString &name) { onVariableActivated(name); });
}

QWidget *MainWindow::buildParameterPanel()
{
  if (paramPanel)
    return paramPanel;

  paramPanel = new QWidget(this);
  auto *layout = new QVBoxLayout(paramPanel);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(2);

  paramToolbar = new QToolBar(QObject::tr("Parameters"), paramPanel);
  paramToolbar->setMovable(false);
  paramToolbar->addAction(addParamAction);
  paramToolbar->addAction(addBlockAction);
  paramToolbar->addAction(removeParamAction);
  layout->addWidget(paramToolbar);

  paramTable = new QTableWidget(paramPanel);
  paramTable->setColumnCount(3);
  paramTable->setHorizontalHeaderLabels({QObject::tr("Name"), QObject::tr("Type"), QObject::tr("Value")});
  paramTable->horizontalHeader()->setStretchLastSection(true);
  paramTable->horizontalHeader()->setHighlightSections(false);
  paramTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  paramTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
  paramTable->setAlternatingRowColors(true);
  paramTable->setShowGrid(false);
  valueDelegate = new ParamValueDelegate(this);
  paramTable->setItemDelegateForColumn(2, valueDelegate);
  layout->addWidget(paramTable);

  connect(paramTable, &QTableWidget::itemChanged, this, &MainWindow::onParamItemChanged);
  connect(paramTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onParamSelectionChanged);

  return paramPanel;
}

void MainWindow::showAboutDialog()
{
  QDialog dialog(this);
  dialog.setWindowTitle(QObject::tr("About BlkEditor"));
  dialog.setModal(true);
  dialog.setMinimumWidth(420);

  auto *layout = new QVBoxLayout(&dialog);
  layout->setContentsMargins(24, 24, 24, 16);
  layout->setSpacing(12);

  auto *iconLabel = new QLabel(&dialog);
  iconLabel->setAlignment(Qt::AlignCenter);
  const QPixmap logoPixmap(QStringLiteral(":/Icons/BlkEditor_logo.png"));
  if (!logoPixmap.isNull())
    iconLabel->setPixmap(logoPixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  else
    iconLabel->setText(QObject::tr("BlkEditor"));
  layout->addWidget(iconLabel);

  auto *titleLabel = new QLabel(QObject::tr("<b>BlkEditor</b>"), &dialog);
  titleLabel->setAlignment(Qt::AlignCenter);
  layout->addWidget(titleLabel);

  auto *bodyLabel = new QLabel(
    QObject::tr("BlkEditor is a tool for editing .blk files used across Dagor-based game engines.\n\n"
                "Copyright WD Studios Corp. && Gaijin Entertainment. All rights reserved.\n"
                "BlkEditor is distributed under the BSD 3-Clause License."),
    &dialog);
  bodyLabel->setWordWrap(true);
  bodyLabel->setAlignment(Qt::AlignCenter);
  layout->addWidget(bodyLabel);

  auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  layout->addWidget(buttonBox);

  dialog.exec();
}

void MainWindow::openFile()
{
  if (!maybeSave())
    return;

  const QString startDir = currentFilePath.isEmpty() ? QDir::currentPath() : QFileInfo(currentFilePath).absolutePath();
  const QString filePath = QFileDialog::getOpenFileName(this, QObject::tr("Open BLK File"), startDir,
    QObject::tr("BLK files (*.blk);;All files (*)"));

  if (filePath.isEmpty())
    return;

  loadDocument(filePath);
}

void MainWindow::save()
{
  saveDocument();
}

void MainWindow::saveAs()
{
  saveDocumentAs();
}

bool MainWindow::saveDocument()
{
  if (!ensureTextEditorAppliedBeforeSave())
    return false;

  if (!blkDocument)
    return true;

  if (currentFilePath.isEmpty())
    return saveDocumentAs();

  const QByteArray encodedPath = QFile::encodeName(currentFilePath);
  if (!blkDocument->saveToTextFile(encodedPath.constData()))
  {
    QMessageBox::critical(this, QObject::tr("Save Failed"),
      QObject::tr("Failed to save BLK file:\n%1").arg(QDir::toNativeSeparators(currentFilePath)));
    return false;
  }

  documentModified = false;
  statusBar()->showMessage(QObject::tr("Saved %1").arg(QDir::toNativeSeparators(currentFilePath)), 4000);
  updateWindowTitle();
  return true;
}

bool MainWindow::saveDocumentAs()
{
  if (!ensureTextEditorAppliedBeforeSave())
    return false;

  if (!blkDocument)
    return true;

  const QString filePath = QFileDialog::getSaveFileName(this, QObject::tr("Save BLK File"), currentFilePath,
    QObject::tr("BLK files (*.blk);;All files (*)"));
  if (filePath.isEmpty())
    return false;

  const QString previousPath = currentFilePath;
  currentFilePath = filePath;
  if (!saveDocument())
  {
    currentFilePath = previousPath;
    return false;
  }

  return true;
}

bool MainWindow::maybeSave()
{
  if (!documentModified)
    return true;

  const QMessageBox::StandardButton choice = QMessageBox::warning(this, QObject::tr("Unsaved Changes"),
    QObject::tr("The current document has unsaved changes.\nDo you want to save them?"),
    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);

  if (choice == QMessageBox::Cancel)
    return false;

  if (choice == QMessageBox::Save)
    return saveDocument();

  return true;
}

void MainWindow::loadDocument(const QString &filePath)
{
  auto document = std::make_unique<DataBlock>();
  const QByteArray encodedPath = QFile::encodeName(filePath);
  if (!document->load(encodedPath.constData()))
  {
    QMessageBox::critical(this, QObject::tr("Load Failed"),
      QObject::tr("Failed to load BLK file:\n%1").arg(QDir::toNativeSeparators(filePath)));
    return;
  }

  blkDocument = std::move(document);
  currentFilePath = filePath;
  documentModified = false;
  currentBlockPath = QStringLiteral("/");
  saveAction->setEnabled(true);
  saveAsAction->setEnabled(true);

  populateTree();
  loadTextEditorFromFile(filePath);
  updateEditingActionStates();
  statusBar()->showMessage(QObject::tr("Loaded %1").arg(QDir::toNativeSeparators(filePath)), 4000);
}

void MainWindow::clearDocument()
{
  blkDocument.reset();
  blockTree->clear();
  paramTable->clearContents();
  paramTable->setRowCount(0);
  currentFilePath.clear();
  currentBlockPath.clear();
  documentModified = false;
  saveAction->setEnabled(false);
  saveAsAction->setEnabled(false);
  variableUsage.clear();
  variableCompletions.clear();
  if (variablesWidget)
  {
    variablesWidget->clearContents();
    variablesWidget->setCurrentBlockPath(QString());
  }
  if (valueDelegate)
  {
    valueDelegate->setGlobalSuggestions({});
    valueDelegate->setParamSpecificSuggestions({});
    valueDelegate->setIncludeSuggestions({});
  }
  setTextEditorContent(QString(), false);
  updateEditingActionStates();
  updateWindowTitle();
}

void MainWindow::populateTree()
{
  const QString previousPath = currentBlockPath;
  blockTree->clear();
  if (!blkDocument)
  {
    paramTable->clearContents();
    paramTable->setRowCount(0);
    updateWindowTitle();
    rebuildVariableCatalog();
    return;
  }

  auto *rootItem = createTreeItem(*blkDocument, QStringLiteral("/"));
  blockTree->addTopLevelItem(rootItem);
  blockTree->expandItem(rootItem);
  if (previousPath.isEmpty())
    blockTree->setCurrentItem(rootItem);
  else
    selectBlockByPath(previousPath);
  blockTree->resizeColumnToContents(0);
  blockTree->resizeColumnToContents(1);

  rebuildVariableCatalog();
  updateWindowTitle();
}

QTreeWidgetItem *MainWindow::createTreeItem(DataBlock &block, const QString &blockPath)
{
  auto *item = new QTreeWidgetItem();
  item->setText(0, blockDisplayName(block));
  item->setText(1, paramSummary(block));
  item->setData(0, BlkEditorRoles::BlockPointer, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(&block)));
  item->setData(0, BlkEditorRoles::BlockPath, blockPath);

  for (std::uint32_t i = 0; i < block.blockCount(); ++i)
  {
    if (auto *child = block.getBlock(i))
    {
      const QString childPath = buildChildPath(blockPath, *child);
      item->addChild(createTreeItem(*child, childPath));
    }
  }

  return item;
}

QString MainWindow::buildChildPath(const QString &parentPath, const DataBlock &child) const
{
  QString component;
  if (const char *rawName = child.getBlockName())
    component = QString::fromUtf8(rawName);
  if (component.isEmpty())
    component = QObject::tr("<unnamed>");

  if (parentPath.isEmpty() || parentPath == QStringLiteral("/"))
    return QStringLiteral("/") + component;

  QString normalized = parentPath;
  if (!normalized.endsWith(QChar('/')))
    normalized.append(QChar('/'));
  normalized.append(component);
  return normalized;
}

void MainWindow::onCurrentBlockChanged(QTreeWidgetItem *current)
{
  if (!current)
  {
    currentBlockPath.clear();
    if (variablesWidget)
      variablesWidget->setCurrentBlockPath(currentBlockPath);
    displayBlockParameters(nullptr);
    updateEditingActionStates();
    return;
  }

  currentBlockPath = current->data(0, BlkEditorRoles::BlockPath).toString();
  if (variablesWidget)
    variablesWidget->setCurrentBlockPath(currentBlockPath);

  const QVariant data = current->data(0, BlkEditorRoles::BlockPointer);
  if (!data.isValid())
  {
    displayBlockParameters(nullptr);
    return;
  }

  auto *block = reinterpret_cast<DataBlock *>(data.value<quintptr>());
  displayBlockParameters(block);
  updateEditingActionStates();
}

void MainWindow::displayBlockParameters(DataBlock *block)
{
  updatingParamTable = true;
  paramTable->clearContents();

  if (!block)
  {
    paramTable->setRowCount(0);
    updatingParamTable = false;
    return;
  }

  const std::uint32_t count = block->paramCount();
  paramTable->setRowCount(static_cast<int>(count));

  for (std::uint32_t i = 0; i < count; ++i)
  {
    const char *name = block->getParamName(i);
    auto *nameItem = new QTableWidgetItem(name ? QString::fromUtf8(name) : QString());
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
    paramTable->setItem(static_cast<int>(i), 0, nameItem);

    const int paramType = block->getParamType(i);
    auto *typeItem = new QTableWidgetItem(paramTypeToString(paramType));
    typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
    paramTable->setItem(static_cast<int>(i), 1, typeItem);

    auto *valueItem = new QTableWidgetItem(valueToDisplayString(*block, i));
    if (!isEditableType(paramType))
      valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);

    valueItem->setData(BlkEditorRoles::BlockPointer, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(block)));
    valueItem->setData(BlkEditorRoles::ParamIndex, static_cast<int>(i));
    valueItem->setData(BlkEditorRoles::ParamType, paramType);
    valueItem->setData(BlkEditorRoles::ParamName, nameItem->text());

    paramTable->setItem(static_cast<int>(i), 2, valueItem);
  }

  paramTable->resizeColumnsToContents();
  updatingParamTable = false;
}

void MainWindow::onParamItemChanged(QTableWidgetItem *item)
{
  if (updatingParamTable || !item || item->column() != 2)
    return;

  const QVariant blockData = item->data(BlkEditorRoles::BlockPointer);
  if (!blockData.isValid())
    return;

  auto *block = reinterpret_cast<DataBlock *>(blockData.value<quintptr>());
  if (!block)
    return;

  const std::uint32_t paramIndex = static_cast<std::uint32_t>(item->data(BlkEditorRoles::ParamIndex).toInt());
  const int paramType = item->data(BlkEditorRoles::ParamType).toInt();

  if (!ensureTextEditorReadyForDocumentMutation())
  {
    updatingParamTable = true;
    item->setText(valueToDisplayString(*block, paramIndex));
    updatingParamTable = false;
    return;
  }

  if (!applyValueChange(*block, paramIndex, paramType, item->text()))
  {
    updatingParamTable = true;
    item->setText(valueToDisplayString(*block, paramIndex));
    updatingParamTable = false;

    QMessageBox::warning(this, QObject::tr("Invalid Value"),
      QObject::tr("The provided value could not be applied to parameter %1.").arg(paramTable->item(item->row(), 0)->text()));
    return;
  }

  updatingParamTable = true;
  item->setText(valueToDisplayString(*block, paramIndex));
  updatingParamTable = false;

  markDocumentAsModified();
  rebuildVariableCatalog();
  refreshTextEditorFromDocument(true);
  statusBar()->showMessage(QObject::tr("Updated %1").arg(paramTable->item(item->row(), 0)->text()), 3000);
}

QString MainWindow::paramTypeToString(int type) const
{
  switch (type)
  {
  case DataBlock::TYPE_NONE:
    return QObject::tr("None");
  case DataBlock::TYPE_STRING:
    return QObject::tr("String");
  case DataBlock::TYPE_INT:
    return QObject::tr("Integer");
  case DataBlock::TYPE_REAL:
    return QObject::tr("Float");
  case DataBlock::TYPE_POINT2:
    return QObject::tr("Point2");
  case DataBlock::TYPE_POINT3:
    return QObject::tr("Point3");
  case DataBlock::TYPE_POINT4:
    return QObject::tr("Point4");
  case DataBlock::TYPE_IPOINT2:
    return QObject::tr("IPoint2");
  case DataBlock::TYPE_IPOINT3:
    return QObject::tr("IPoint3");
  case DataBlock::TYPE_BOOL:
    return QObject::tr("Bool");
  case DataBlock::TYPE_E3DCOLOR:
    return QObject::tr("Color");
  case DataBlock::TYPE_MATRIX:
    return QObject::tr("Matrix");
  case DataBlock::TYPE_INT64:
    return QObject::tr("Int64");
  case DataBlock::TYPE_IPOINT4:
    return QObject::tr("IPoint4");
  default:
    return QObject::tr("Unknown (%1)").arg(type);
  }
}

QString MainWindow::valueToDisplayString(DataBlock &block, std::uint32_t paramIndex) const
{
  const int type = block.getParamType(paramIndex);
  switch (type)
  {
  case DataBlock::TYPE_NONE:
    return QString();
  case DataBlock::TYPE_STRING:
  {
    const char *value = block.getStr(paramIndex);
    return value ? QString::fromUtf8(value) : QString();
  }
  case DataBlock::TYPE_INT:
    return QString::number(block.getInt(paramIndex));
  case DataBlock::TYPE_REAL:
    return QString::number(block.getReal(paramIndex), 'g', 6);
  case DataBlock::TYPE_POINT2:
    return formatPoint(block.getPoint2(paramIndex));
  case DataBlock::TYPE_POINT3:
    return formatPoint(block.getPoint3(paramIndex));
  case DataBlock::TYPE_POINT4:
    return formatPoint(block.getPoint4(paramIndex));
  case DataBlock::TYPE_IPOINT2:
    return formatPoint(block.getIPoint2(paramIndex));
  case DataBlock::TYPE_IPOINT3:
    return formatPoint(block.getIPoint3(paramIndex));
  case DataBlock::TYPE_BOOL:
    return block.getBool(paramIndex) ? QStringLiteral("true") : QStringLiteral("false");
  case DataBlock::TYPE_E3DCOLOR:
    return formatColor(block.getE3dcolor(paramIndex));
  case DataBlock::TYPE_MATRIX:
    return formatMatrix(block.getTm(paramIndex));
  case DataBlock::TYPE_INT64:
    return QString::number(static_cast<qlonglong>(block.getInt64(paramIndex)));
  case DataBlock::TYPE_IPOINT4:
    return formatPoint(block.getIPoint4(paramIndex));
  default:
    return QObject::tr("<unsupported>");
  }
}

bool MainWindow::applyValueChange(DataBlock &block, std::uint32_t paramIndex, int type, const QString &valueText)
{
  const QString trimmed = valueText.trimmed();

  switch (type)
  {
  case DataBlock::TYPE_STRING:
  {
    const QByteArray data = valueText.toUtf8();
    return block.setStr(paramIndex, data.constData());
  }
  case DataBlock::TYPE_INT:
  {
    bool ok = false;
    const int value = trimmed.toInt(&ok);
    return ok && block.setInt(paramIndex, value);
  }
  case DataBlock::TYPE_INT64:
  {
    bool ok = false;
    const qlonglong value = trimmed.toLongLong(&ok);
    return ok && block.setInt64(paramIndex, static_cast<int64_t>(value));
  }
  case DataBlock::TYPE_REAL:
  {
    bool ok = false;
    const float value = trimmed.toFloat(&ok);
    return ok && block.setReal(paramIndex, value);
  }
  case DataBlock::TYPE_POINT2:
  {
    float components[2];
    if (!parseFloatComponents(trimmed, 2, components))
      return false;
    Point2 value(components[0], components[1]);
    return block.setPoint2(paramIndex, value);
  }
  case DataBlock::TYPE_POINT3:
  {
    float components[3];
    if (!parseFloatComponents(trimmed, 3, components))
      return false;
    Point3 value(components[0], components[1], components[2]);
    return block.setPoint3(paramIndex, value);
  }
  case DataBlock::TYPE_POINT4:
  {
    float components[4];
    if (!parseFloatComponents(trimmed, 4, components))
      return false;
    Point4 value(components[0], components[1], components[2], components[3]);
    return block.setPoint4(paramIndex, value);
  }
  case DataBlock::TYPE_IPOINT2:
  {
    int components[2];
    if (!parseIntComponents(trimmed, 2, components))
      return false;
    IPoint2 value(components[0], components[1]);
    return block.setIPoint2(paramIndex, value);
  }
  case DataBlock::TYPE_IPOINT3:
  {
    int components[3];
    if (!parseIntComponents(trimmed, 3, components))
      return false;
    IPoint3 value(components[0], components[1], components[2]);
    return block.setIPoint3(paramIndex, value);
  }
  case DataBlock::TYPE_IPOINT4:
  {
    int components[4];
    if (!parseIntComponents(trimmed, 4, components))
      return false;
    IPoint4 value(components[0], components[1], components[2], components[3]);
    return block.setIPoint4(paramIndex, value);
  }
  case DataBlock::TYPE_BOOL:
  {
    const QString lower = trimmed.toLower();
    if (lower == QStringLiteral("true") || lower == QStringLiteral("1"))
      return block.setBool(paramIndex, true);
    if (lower == QStringLiteral("false") || lower == QStringLiteral("0"))
      return block.setBool(paramIndex, false);
    return false;
  }
  case DataBlock::TYPE_E3DCOLOR:
  {
    const QStringList parts = splitComponents(trimmed);
    if (parts.size() != 3 && parts.size() != 4)
      return false;

    int components[4] = {0, 0, 0, 255};
    for (int i = 0; i < parts.size(); ++i)
    {
      bool ok = false;
      const int value = parts[i].toInt(&ok);
      if (!ok)
        return false;
      components[i] = std::clamp(value, 0, 255);
    }

    E3DCOLOR color(static_cast<unsigned char>(components[0]), static_cast<unsigned char>(components[1]),
      static_cast<unsigned char>(components[2]), static_cast<unsigned char>(components[3]));
    return block.setE3dcolor(paramIndex, color);
  }
  case DataBlock::TYPE_MATRIX:
  {
    float components[12];
    if (!parseFloatComponents(trimmed, 12, components))
      return false;

    TMatrix value;
    value.zero();
    for (int row = 0; row < 4; ++row)
    {
      for (int col = 0; col < 3; ++col)
        value.m[row][col] = components[row * 3 + col];
    }

    return block.setTm(paramIndex, value);
  }
  default:
    return false;
  }
}

bool MainWindow::isEditableType(int type) const
{
  switch (type)
  {
  case DataBlock::TYPE_STRING:
  case DataBlock::TYPE_INT:
  case DataBlock::TYPE_REAL:
  case DataBlock::TYPE_BOOL:
  case DataBlock::TYPE_INT64:
  case DataBlock::TYPE_POINT2:
  case DataBlock::TYPE_POINT3:
  case DataBlock::TYPE_POINT4:
  case DataBlock::TYPE_IPOINT2:
  case DataBlock::TYPE_IPOINT3:
  case DataBlock::TYPE_IPOINT4:
  case DataBlock::TYPE_E3DCOLOR:
  case DataBlock::TYPE_MATRIX:
    return true;
  default:
    return false;
  }
}

void MainWindow::rebuildVariableCatalog()
{
  variableUsage.clear();
  variableCompletions.clear();
  includeSuggestions.clear();
  paramSpecificSuggestions.clear();
  textEditorKeywordPool.clear();

  if (!blkDocument)
  {
    if (variablesWidget)
      variablesWidget->clearContents();
    if (valueDelegate)
    {
      valueDelegate->setGlobalSuggestions({});
      valueDelegate->setParamSpecificSuggestions({});
      valueDelegate->setIncludeSuggestions({});
    }
    rebuildIncludeSuggestions();
    rebuildTextEditorCompletions();
    return;
  }

  collectVariables(*blkDocument, QStringLiteral("/"));
  collectExternalVariables();
  rebuildIncludeSuggestions();

  QVector<VariablesWidget::VariableRow> rows;
  rows.reserve(variableUsage.size());

  for (auto it = variableUsage.constBegin(); it != variableUsage.constEnd(); ++it)
  {
    VariablesWidget::VariableRow row;
    row.name = it.key();
    row.type = it.value().mixedType ? QObject::tr("Mixed") : it.value().typeLabel;
    row.blockPaths = it.value().blockPaths.values();
    std::sort(row.blockPaths.begin(), row.blockPaths.end(), [](const QString &lhs, const QString &rhs) {
      return lhs.compare(rhs, Qt::CaseInsensitive) < 0;
    });
    rows.push_back(row);

    variableCompletions.push_back(QStringLiteral("#%1").arg(row.name));
    variableCompletions.push_back(row.name);

    if (!it.value().sampleOverflow && !it.value().sampleValues.isEmpty())
    {
      QStringList samples = it.value().sampleValues.values();
      samples.removeAll(QString());
      samples.sort(Qt::CaseInsensitive);
      if (!samples.isEmpty())
      {
        QString key = row.name.trimmed().toLower();
        QStringList merged = paramSpecificSuggestions.value(key);
        merged.append(samples);
        merged.removeAll(QString());
        merged.removeDuplicates();
        std::sort(merged.begin(), merged.end(), [](const QString &lhs, const QString &rhs) {
          return lhs.compare(rhs, Qt::CaseInsensitive) < 0;
        });
        paramSpecificSuggestions.insert(key, merged);
      }
    }
  }

  std::sort(rows.begin(), rows.end(), [](const VariablesWidget::VariableRow &lhs, const VariablesWidget::VariableRow &rhs) {
    return lhs.name.compare(rhs.name, Qt::CaseInsensitive) < 0;
  });

  if (variablesWidget)
    variablesWidget->updateVariables(rows, currentBlockPath);

  std::sort(variableCompletions.begin(), variableCompletions.end(), [](const QString &lhs, const QString &rhs) {
    return lhs.compare(rhs, Qt::CaseInsensitive) < 0;
  });
  variableCompletions.removeDuplicates();

  if (valueDelegate)
  {
    valueDelegate->setGlobalSuggestions(variableCompletions);
    valueDelegate->setParamSpecificSuggestions(paramSpecificSuggestions);
    valueDelegate->setIncludeSuggestions(includeSuggestions);
  }

  textEditorKeywordPool = variableCompletions;
  for (const auto &row : rows)
    textEditorKeywordPool.push_back(row.name);
  textEditorKeywordPool.removeDuplicates();

  rebuildTextEditorCompletions();
}

void MainWindow::collectVariables(DataBlock &block, const QString &currentPath)
{
  const std::uint32_t count = block.paramCount();
  for (std::uint32_t i = 0; i < count; ++i)
  {
    const char *name = block.getParamName(i);
    if (!name || !*name)
      continue;

    const QString nameStr = QString::fromUtf8(name);
    if (nameStr.isEmpty())
      continue;

    auto &usage = variableUsage[nameStr];
    const QString typeLabel = paramTypeToString(block.getParamType(i));
    if (usage.typeLabel.isEmpty())
      usage.typeLabel = typeLabel;
    else if (usage.typeLabel != typeLabel)
      usage.mixedType = true;

    usage.blockPaths.insert(currentPath);

    const int paramType = block.getParamType(i);
    if (paramType == DataBlock::TYPE_STRING)
    {
      if (!usage.sampleOverflow)
      {
        usage.sampleValues.insert(valueToDisplayString(block, i));
        if (usage.sampleValues.size() > maxEnumSamples)
        {
          usage.sampleOverflow = true;
          usage.sampleValues.clear();
        }
      }
    }
  }

  for (std::uint32_t i = 0; i < block.blockCount(); ++i)
  {
    if (auto *child = block.getBlock(i))
      collectVariables(*child, buildChildPath(currentPath, *child));
  }
}

void MainWindow::collectExternalVariables()
{
  if (currentFilePath.isEmpty())
    return;

  const QFileInfo info(currentFilePath);
  collectVariablesFromDirectory(info.absoluteDir().absolutePath(), maxExternalFiles);
}

void MainWindow::collectVariablesFromFile(const QString &filePath, const QString &virtualRootLabel)
{
  auto external = std::make_unique<DataBlock>();
  const QByteArray encoded = QFile::encodeName(filePath);
  if (!external->load(encoded.constData()))
    return;

  collectVariables(*external, QStringLiteral("file:%1").arg(virtualRootLabel));
}

void MainWindow::collectVariablesFromDirectory(const QString &rootDir, int maxFiles)
{
  if (rootDir.isEmpty() || maxFiles <= 0)
    return;

  int processed = 0;
  QDirIterator it(rootDir, QStringList{QStringLiteral("*.blk")}, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext() && processed < maxFiles)
  {
    const QString filePath = it.next();
    if (QFileInfo(filePath).absoluteFilePath() == QFileInfo(currentFilePath).absoluteFilePath())
      continue;
    collectVariablesFromFile(filePath, QFileInfo(filePath).fileName());
    ++processed;
  }
}

void MainWindow::rebuildIncludeSuggestions()
{
  includeSuggestions.clear();
  if (currentFilePath.isEmpty())
    return;

  const QFileInfo info(currentFilePath);
  QDir baseDir = info.absoluteDir();
  QDirIterator it(baseDir.absolutePath(), QStringList{QStringLiteral("*.blk")}, QDir::Files, QDirIterator::Subdirectories);
  int processed = 0;
  while (it.hasNext() && processed < maxExternalFiles)
  {
    const QString fullPath = it.next();
    QString relative = baseDir.relativeFilePath(fullPath);
    if (!relative.isEmpty())
      includeSuggestions.push_back(relative);
    ++processed;
  }

  includeSuggestions.removeDuplicates();
  includeSuggestions.sort(Qt::CaseInsensitive);
}

void MainWindow::rebuildTextEditorCompletions()
{
  if (!textCompletionModel)
    return;

  QStringList completions = textEditorKeywordPool;
  for (const QString &includePath : includeSuggestions)
  {
    completions.push_back(includePath);
    completions.push_back(QStringLiteral("include %1").arg(includePath));
  }

  completions.append(directiveTokens);
  completions.append(typeTokens);
  completions.removeAll(QString());
  completions.removeDuplicates();
  std::sort(completions.begin(), completions.end(), [](const QString &lhs, const QString &rhs) {
    return lhs.compare(rhs, Qt::CaseInsensitive) < 0;
  });

  textCompletionModel->setStringList(completions);
}

void MainWindow::addParameterInteractive()
{
  if (!blockTree)
    return;

  QTreeWidgetItem *current = blockTree->currentItem();
  if (!current)
    return;

  const QVariant data = current->data(0, BlkEditorRoles::BlockPointer);
  if (!data.isValid())
    return;

  auto *block = reinterpret_cast<DataBlock *>(data.value<quintptr>());
  if (!block)
    return;

  if (!ensureTextEditorReadyForDocumentMutation())
    return;

  QDialog dialog(this);
  dialog.setWindowTitle(QObject::tr("Add Parameter"));
  auto *form = new QFormLayout(&dialog);

  auto *nameEdit = new QLineEdit(&dialog);
  form->addRow(QObject::tr("Name"), nameEdit);

  auto *typeCombo = new QComboBox(&dialog);
  for (const ParamTypeOption &option : kParamTypeOptions)
    typeCombo->addItem(QObject::tr(option.label), option.type);
  form->addRow(QObject::tr("Type"), typeCombo);

  auto *valueEdit = new QLineEdit(&dialog);
  form->addRow(QObject::tr("Value"), valueEdit);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  form->addWidget(buttons);

  if (dialog.exec() != QDialog::Accepted)
    return;

  const QString name = nameEdit->text().trimmed();
  if (name.isEmpty())
  {
    QMessageBox::warning(this, QObject::tr("Missing Name"), QObject::tr("Parameter name cannot be empty."));
    return;
  }

  const int type = typeCombo->currentData().toInt();
  if (!addParameterToBlock(*block, name, type, valueEdit->text()))
  {
    QMessageBox::warning(this, QObject::tr("Add Failed"), QObject::tr("Could not add parameter %1.").arg(name));
    return;
  }

  markDocumentAsModified();
  displayBlockParameters(block);
  rebuildVariableCatalog();
  refreshTextEditorFromDocument(true);
  updateEditingActionStates();
  statusBar()->showMessage(QObject::tr("Added parameter %1").arg(name), 2500);
}

void MainWindow::addBlockInteractive()
{
  if (!blockTree)
    return;

  QTreeWidgetItem *current = blockTree->currentItem();
  if (!current)
    return;

  const QVariant data = current->data(0, BlkEditorRoles::BlockPointer);
  if (!data.isValid())
    return;

  auto *block = reinterpret_cast<DataBlock *>(data.value<quintptr>());
  if (!block)
    return;

  if (!ensureTextEditorReadyForDocumentMutation())
    return;

  QDialog dialog(this);
  dialog.setWindowTitle(QObject::tr("Add Child Block"));
  auto *form = new QFormLayout(&dialog);

  auto *nameEdit = new QLineEdit(&dialog);
  form->addRow(QObject::tr("Block Name"), nameEdit);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  form->addWidget(buttons);

  if (dialog.exec() != QDialog::Accepted)
    return;

  const QString blockName = nameEdit->text().trimmed();
  if (blockName.isEmpty())
  {
    QMessageBox::warning(this, QObject::tr("Missing Name"), QObject::tr("Block name cannot be empty."));
    return;
  }

  const QByteArray utf8Name = blockName.toUtf8();
  DataBlock *child = block->addNewBlock(utf8Name.constData());
  if (!child)
  {
    QMessageBox::warning(this, QObject::tr("Add Failed"), QObject::tr("Could not create block %1.").arg(blockName));
    return;
  }

  markDocumentAsModified();
  const QString newPath = buildChildPath(currentBlockPath, *child);
  populateTree();
  selectBlockByPath(newPath);
  rebuildVariableCatalog();
  refreshTextEditorFromDocument(true);
  updateEditingActionStates();
  statusBar()->showMessage(QObject::tr("Added block %1").arg(blockName), 2500);
}

void MainWindow::removeSelectedParameter()
{
  if (!paramTable || !blockTree)
    return;

  QTableWidgetItem *item = paramTable->currentItem();
  if (!item)
    return;

  if (!ensureTextEditorReadyForDocumentMutation())
    return;

  const QVariant ptrData = item->data(BlkEditorRoles::BlockPointer);
  if (!ptrData.isValid())
    return;

  auto *block = reinterpret_cast<DataBlock *>(ptrData.value<quintptr>());
  if (!block)
    return;

  const QString paramName = paramTable->item(item->row(), 0) ? paramTable->item(item->row(), 0)->text() : QString();
  const std::uint32_t paramIndex = static_cast<std::uint32_t>(item->data(BlkEditorRoles::ParamIndex).toInt());
  if (!block->removeParam(paramIndex))
  {
    QMessageBox::warning(this, QObject::tr("Remove Failed"), QObject::tr("Unable to remove parameter."));
    return;
  }

  markDocumentAsModified();
  displayBlockParameters(block);
  rebuildVariableCatalog();
  refreshTextEditorFromDocument(true);
  updateEditingActionStates();
  statusBar()->showMessage(QObject::tr("Removed parameter %1").arg(paramName), 2000);
}

bool MainWindow::addParameterToBlock(DataBlock &block, const QString &name, int type, const QString &valueText)
{
  const QByteArray utf8Name = name.toUtf8();
  int paramIndex = -1;
  switch (type)
  {
  case DataBlock::TYPE_STRING:
    paramIndex = block.addStr(utf8Name.constData(), "");
    break;
  case DataBlock::TYPE_INT:
    paramIndex = block.addInt(utf8Name.constData(), 0);
    break;
  case DataBlock::TYPE_INT64:
    paramIndex = block.addInt64(utf8Name.constData(), 0);
    break;
  case DataBlock::TYPE_REAL:
    paramIndex = block.addReal(utf8Name.constData(), 0.f);
    break;
  case DataBlock::TYPE_BOOL:
    paramIndex = block.addBool(utf8Name.constData(), false);
    break;
  case DataBlock::TYPE_POINT2:
    paramIndex = block.addPoint2(utf8Name.constData(), Point2(0.0f, 0.0f));
    break;
  case DataBlock::TYPE_POINT3:
    paramIndex = block.addPoint3(utf8Name.constData(), Point3(0.0f, 0.0f, 0.0f));
    break;
  case DataBlock::TYPE_POINT4:
    paramIndex = block.addPoint4(utf8Name.constData(), Point4(0.0f, 0.0f, 0.0f, 0.0f));
    break;
  case DataBlock::TYPE_IPOINT2:
    paramIndex = block.addIPoint2(utf8Name.constData(), IPoint2(0, 0));
    break;
  case DataBlock::TYPE_IPOINT3:
    paramIndex = block.addIPoint3(utf8Name.constData(), IPoint3(0, 0, 0));
    break;
  case DataBlock::TYPE_IPOINT4:
    paramIndex = block.addIPoint4(utf8Name.constData(), IPoint4(0, 0, 0, 0));
    break;
  case DataBlock::TYPE_E3DCOLOR:
    paramIndex = block.addE3dcolor(utf8Name.constData(), E3DCOLOR(0, 0, 0, 255));
    break;
  case DataBlock::TYPE_MATRIX:
  {
    TMatrix matrix;
    matrix.identity();
    paramIndex = block.addTm(utf8Name.constData(), matrix);
    break;
  }
  default:
    return false;
  }

  if (paramIndex < 0)
    return false;

  const QString trimmed = valueText.trimmed();
  if (type == DataBlock::TYPE_STRING)
  {
    if (!trimmed.isEmpty())
      block.setStr(static_cast<std::uint32_t>(paramIndex), trimmed.toUtf8().constData());
    return true;
  }

  if (trimmed.isEmpty())
    return true;

  if (!applyValueChange(block, static_cast<std::uint32_t>(paramIndex), type, trimmed))
  {
    block.removeParam(static_cast<std::uint32_t>(paramIndex));
    return false;
  }

  return true;
}

QStringList MainWindow::gatherDirectiveTokens() const
{
  return {QStringLiteral("@include"), QStringLiteral("@override"), QStringLiteral("@delete"), QStringLiteral("@clone-last"),
    QStringLiteral("@if"), QStringLiteral("@else"), QStringLiteral("@endif")};
}

QStringList MainWindow::gatherTypeTokens() const
{
  return {QStringLiteral("t"),   QStringLiteral("b"),    QStringLiteral("c"),   QStringLiteral("r"),    QStringLiteral("m"),
    QStringLiteral("p2"), QStringLiteral("p3"), QStringLiteral("p4"), QStringLiteral("ip2"), QStringLiteral("ip3"),
    QStringLiteral("ip4"), QStringLiteral("i"),  QStringLiteral("i64"), QStringLiteral("bool"), QStringLiteral("string")};
}

void MainWindow::onVariableActivated(const QString &name)
{
  if (name.isEmpty())
    return;

  const QString snippet = name.startsWith(QChar('#')) ? name : QStringLiteral("#%1").arg(name);

  if (auto *item = paramTable->currentItem(); item && item->column() == 2 && item->flags().testFlag(Qt::ItemIsEditable))
  {
    item->setText(snippet);
    return;
  }

  QGuiApplication::clipboard()->setText(snippet);
  statusBar()->showMessage(QObject::tr("Copied %1 to clipboard").arg(snippet), 2500);
}

void MainWindow::selectBlockByPath(const QString &blockPath)
{
  if (!blockTree || blockPath.isEmpty())
    return;

  QTreeWidgetItem *root = blockTree->invisibleRootItem();
  if (!root)
    return;

  if (QTreeWidgetItem *target = findTreeItemByPath(blockPath, root))
  {
    blockTree->setCurrentItem(target);
    blockTree->scrollToItem(target);
  }
}

QTreeWidgetItem *MainWindow::findTreeItemByPath(const QString &blockPath, QTreeWidgetItem *root) const
{
  if (!root)
    return nullptr;

  for (int i = 0; i < root->childCount(); ++i)
  {
    QTreeWidgetItem *child = root->child(i);
    if (!child)
      continue;

    const QString value = child->data(0, BlkEditorRoles::BlockPath).toString();
    if (value == blockPath)
      return child;

    if (QTreeWidgetItem *descendant = findTreeItemByPath(blockPath, child))
      return descendant;
  }

  return nullptr;
}

void MainWindow::markDocumentAsModified()
{
  if (!documentModified)
  {
    documentModified = true;
    updateWindowTitle();
  }
}

void MainWindow::updateWindowTitle()
{
  QString title = QObject::tr("BlkEditor");
  if (blkDocument)
  {
    const QString fileName = currentFilePath.isEmpty() ? QObject::tr("Untitled") : QFileInfo(currentFilePath).fileName();
    title += QStringLiteral(" - %1").arg(fileName);
    if (documentModified)
      title += QStringLiteral("*");
  }
  else if (documentModified)
  {
    title += QStringLiteral("*");
  }
  setWindowTitle(title);
}

void MainWindow::createTextEditorDock()
{
  if (textEditor)
    return;

  textEditor = new BlkCodeEditor(this);
  textEditor->setObjectName(QStringLiteral("BlkTextEditor"));
  textEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
  textEditor->setTabChangesFocus(false);
  QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  textEditor->setFont(monoFont);
  connect(textEditor, &QPlainTextEdit::textChanged, this, &MainWindow::onTextEditorTextChanged);

  textCompletionModel = new QStringListModel(this);
  textCompleter = new QCompleter(textCompletionModel, this);
  textCompleter->setCaseSensitivity(Qt::CaseInsensitive);
  textCompleter->setFilterMode(Qt::MatchContains);
  textCompleter->setCompletionMode(QCompleter::PopupCompletion);
  textEditor->setCompleter(textCompleter);
  ensureTextEditorHasHighlighter();

  textEditorDock = new QDockWidget(QObject::tr("BLK Text"), this);
  textEditorDock->setObjectName(QStringLiteral("BlkTextDock"));
  textEditorDock->setAllowedAreas(Qt::AllDockWidgetAreas);
  textEditorDock->setWidget(textEditor);
  addDockWidget(Qt::BottomDockWidgetArea, textEditorDock);

  if (viewMenu)
    viewMenu->addAction(textEditorDock->toggleViewAction());

  updateTextEditorActions();
}

void MainWindow::setTextEditorContent(const QString &text, bool preserveView)
{
  if (!textEditor)
    return;

  updatingTextEditor = true;

  QTextCursor previousCursor = textEditor->textCursor();
  const int previousVertical = textEditor->verticalScrollBar()->value();
  const int previousHorizontal = textEditor->horizontalScrollBar()->value();

  textEditor->setPlainText(text);

  if (preserveView)
  {
    QTextCursor cursor = textEditor->textCursor();
    const int maxPos = std::max(0, textEditor->document()->characterCount() - 1);
    const int targetPos = std::clamp(previousCursor.position(), 0, maxPos);
    cursor.setPosition(targetPos);
    textEditor->setTextCursor(cursor);
    textEditor->verticalScrollBar()->setValue(previousVertical);
    textEditor->horizontalScrollBar()->setValue(previousHorizontal);
  }

  updatingTextEditor = false;
  textEditorDirty = false;
  updateTextEditorActions();
}

void MainWindow::onTextEditorTextChanged()
{
  if (updatingTextEditor)
    return;

  textEditorDirty = true;
  updateTextEditorActions();
  markDocumentAsModified();
}

void MainWindow::updateTextEditorActions()
{
  if (applyTextChangesAction)
    applyTextChangesAction->setEnabled(textEditor && textEditorDirty);

  if (reloadTextFromDocumentAction)
    reloadTextFromDocumentAction->setEnabled(textEditor && blkDocument != nullptr);
}

void MainWindow::onParamSelectionChanged()
{
  updateEditingActionStates();
}

void MainWindow::updateEditingActionStates()
{
  const bool hasDocument = blkDocument != nullptr;
  bool blockSelected = false;
  if (hasDocument && blockTree)
  {
    QTreeWidgetItem *current = blockTree->currentItem();
    blockSelected = current && current->data(0, BlkEditorRoles::BlockPointer).isValid();
  }

  if (addParamAction)
    addParamAction->setEnabled(blockSelected);
  if (addBlockAction)
    addBlockAction->setEnabled(blockSelected);

  bool hasParamSelection = false;
  if (blockSelected && paramTable)
    hasParamSelection = paramTable->currentRow() >= 0;

  if (removeParamAction)
    removeParamAction->setEnabled(hasParamSelection);
}

void MainWindow::ensureTextEditorHasHighlighter()
{
  if (!textEditor)
    return;

  if (!syntaxHighlighter)
    syntaxHighlighter = std::make_unique<BlkSyntaxHighlighter>(textEditor->document());

  syntaxHighlighter->setKeywordList(typeTokens);
  syntaxHighlighter->setDirectiveList(directiveTokens);
}

void MainWindow::refreshTextEditorFromDocument(bool preserveView)
{
  if (!textEditor)
    return;

  ensureTextEditorHasHighlighter();

  if (!blkDocument)
  {
    setTextEditorContent(QString(), false);
    return;
  }

  setTextEditorContent(serializeDocumentToText(), preserveView);
}

void MainWindow::loadTextEditorFromFile(const QString &filePath)
{
  if (!textEditor)
    return;

  ensureTextEditorHasHighlighter();

  QString text;
  if (!filePath.isEmpty())
  {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly))
    {
      const QByteArray raw = file.readAll();
      bool containsNull = false;
      for (char ch : raw)
      {
        if (ch == '\0')
        {
          containsNull = true;
          break;
        }
      }
      if (!containsNull)
        text = QString::fromUtf8(raw);
    }
  }

  if (text.isEmpty())
    text = serializeDocumentToText();

  setTextEditorContent(text, false);
  updateTextEditorActions();
}

QString MainWindow::serializeDocumentToText() const
{
  if (!blkDocument)
    return QString();

  MemorySaveCB writer(64 << 10);
  if (!blkDocument->saveToTextStream(writer))
    return QString();

  MemoryChainedData *chain = writer.takeMem();
  if (!chain)
    return QString();

  const ptrdiff_t total = MemoryChainedData::calcTotalUsedSize(chain);
  QByteArray buffer;
  buffer.reserve(static_cast<int>(total));
  for (MemoryChainedData *segment = chain; segment; segment = segment->next)
  {
    if (!segment->used)
      break;
    buffer.append(segment->data, static_cast<int>(segment->used));
  }
  MemoryChainedData::deleteChain(chain);
  return QString::fromUtf8(buffer);
}

bool MainWindow::applyTextEditorChanges()
{
  if (!textEditor || !textEditorDirty)
    return true;

  auto updatedDocument = std::make_unique<DataBlock>();
  const QString text = textEditor->toPlainText();
  const QByteArray bytes = text.toUtf8();
  QByteArray encodedPath;
  const char *fname = nullptr;
  if (!currentFilePath.isEmpty())
  {
    encodedPath = QFile::encodeName(currentFilePath);
    fname = encodedPath.constData();
  }

  if (!updatedDocument->loadText(bytes.constData(), bytes.size(), fname))
  {
    QMessageBox::critical(this, QObject::tr("Parse Failed"),
      QObject::tr("The BLK text contains syntax errors. Fix them and try again."));
    return false;
  }

  blkDocument = std::move(updatedDocument);
  textEditorDirty = false;
  documentModified = true;
  saveAction->setEnabled(true);
  saveAsAction->setEnabled(true);
  populateTree();
  updateWindowTitle();
  updateTextEditorActions();
  statusBar()->showMessage(QObject::tr("Applied text editor changes"), 3000);
  return true;
}

bool MainWindow::ensureTextEditorReadyForDocumentMutation()
{
  if (!textEditor || !textEditorDirty)
    return true;

  const QMessageBox::StandardButton choice = QMessageBox::warning(this, QObject::tr("Pending Text Changes"),
    QObject::tr("Apply or discard the text editor changes before editing via the table?"),
    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);

  if (choice == QMessageBox::Cancel)
    return false;

  if (choice == QMessageBox::Yes)
    return applyTextEditorChanges();

  if (blkDocument)
  {
    refreshTextEditorFromDocument(false);
  }
  else
  {
    setTextEditorContent(QString(), false);
    documentModified = false;
  }
  updateWindowTitle();
  statusBar()->showMessage(QObject::tr("Discarded text editor changes"), 2000);
  return true;
}

bool MainWindow::ensureTextEditorAppliedBeforeSave()
{
  if (!textEditor || !textEditorDirty)
    return true;

  return applyTextEditorChanges();
}
