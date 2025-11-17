// Copyright (C) 2023 - present WD Studios Corp. All Rights Reserved.
#include "MainWindow.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyleFactory>

#include <ioSys/dag_dataBlock.h>

namespace
{
const char kDarkStyleSheet[] = R"(
QWidget {
  background-color: #1f2127;
  color: #d8dce3;
  selection-background-color: #325c85;
  selection-color: #ffffff;
}

QMainWindow::separator {
  background: #15171d;
  width: 1px;
  height: 1px;
}

QMenuBar {
  background-color: #252932;
  border-bottom: 1px solid #303641;
}

QMenuBar::item {
  padding: 4px 14px;
  background: transparent;
}

QMenuBar::item:selected {
  background: #325c85;
}

QMenu {
  background-color: #252932;
  border: 1px solid #303641;
}

QMenu::item {
  padding: 4px 22px 4px 28px;
}

QMenu::item:selected {
  background-color: #325c85;
}

QToolBar {
  background-color: #252932;
  border-bottom: 1px solid #303641;
  spacing: 6px;
}

QStatusBar {
  background-color: #1a1d23;
  color: #8e96a3;
}

QTreeView,
QListView,
QTableView {
  background-color: #1f2127;
  alternate-background-color: #22252d;
  border: 1px solid #2d313b;
  selection-background-color: #325c85;
  selection-color: #ffffff;
  gridline-color: #2d313b;
}

QTreeView::item:selected,
QTableView::item:selected {
  background-color: #35608b;
  color: #ffffff;
}

QHeaderView::section {
  background-color: #242831;
  color: #ced4dc;
  padding: 4px;
  border: none;
  border-right: 1px solid #303641;
}

QLineEdit,
QPlainTextEdit,
QTextEdit {
  background-color: #1c1e24;
  border: 1px solid #2d313b;
  padding: 4px;
  selection-background-color: #325c85;
}

QLabel {
  color: #d8dce3;
}

QPushButton,
QToolButton,
QComboBox,
QSpinBox,
QDoubleSpinBox {
  background-color: #272b34;
  border: 1px solid #343944;
  padding: 4px 10px;
}

QPushButton:hover,
QToolButton:hover,
QComboBox:hover {
  border-color: #3f4653;
}

QPushButton:pressed,
QToolButton:pressed {
  background-color: #325c85;
  border-color: #325c85;
}

QScrollBar:vertical {
  background: #1b1d24;
  width: 14px;
  margin: 14px 0 14px 0;
}

QScrollBar::handle:vertical {
  background: #2f3641;
  min-height: 26px;
  border-radius: 6px;
}

QScrollBar::handle:vertical:hover {
  background: #3a4350;
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical {
  background: none;
  height: 0;
}

QScrollBar:horizontal {
  background: #1b1d24;
  height: 14px;
  margin: 0 14px 0 14px;
}

QScrollBar::handle:horizontal {
  background: #2f3641;
  min-width: 26px;
  border-radius: 6px;
}

QScrollBar::handle:horizontal:hover {
  background: #3a4350;
}

QToolTip {
  background-color: #2b303a;
  color: #eef2f7;
  border: 1px solid #3a3f4b;
}

QTabWidget::pane {
  border: 1px solid #2d313b;
  background: #1f2127;
}

QTabBar::tab {
  background: #242831;
  color: #cdd3dc;
  padding: 5px 18px;
}

QTabBar::tab:selected {
  background: #325c85;
  color: #ffffff;
}

QGroupBox {
  border: 1px solid #2d313b;
  margin-top: 16px;
}

QGroupBox::title {
  subcontrol-origin: margin;
  subcontrol-position: top left;
  padding: 0 6px;
}
)";

void applyDarkTheme(QApplication &app)
{
  app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

  QPalette palette;
  palette.setColor(QPalette::Window, QColor(31, 33, 39));
  palette.setColor(QPalette::WindowText, QColor(216, 220, 227));
  palette.setColor(QPalette::Base, QColor(28, 30, 36));
  palette.setColor(QPalette::AlternateBase, QColor(34, 37, 45));
  palette.setColor(QPalette::Text, QColor(216, 220, 227));
  palette.setColor(QPalette::Button, QColor(39, 43, 52));
  palette.setColor(QPalette::ButtonText, QColor(221, 226, 233));
  palette.setColor(QPalette::Highlight, QColor(50, 92, 133));
  palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
  palette.setColor(QPalette::Link, QColor(109, 169, 255));
  palette.setColor(QPalette::ToolTipBase, QColor(43, 48, 58));
  palette.setColor(QPalette::ToolTipText, QColor(238, 242, 247));

  palette.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 125, 133));
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 125, 133));
  palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 125, 133));

  app.setPalette(palette);
  app.setStyleSheet(QString::fromLatin1(kDarkStyleSheet));
}
}

int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  QApplication::setOrganizationName(QStringLiteral("WD Studios & Gajin Ent."));
  QApplication::setApplicationName(QStringLiteral("BlkEditor"));
  QApplication::setApplicationDisplayName(QStringLiteral("BlkEditor"));
  ::applyDarkTheme(app);

  
  DataBlock::fatalOnMissingFile = false;
  DataBlock::fatalOnLoadFailed = false;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::fatalOnMissingVar = false;

  MainWindow window;
  window.setWindowIcon(QIcon(QStringLiteral(":/Icons/BlkEditor_logo.png")));
  window.show();

  return app.exec();
}
