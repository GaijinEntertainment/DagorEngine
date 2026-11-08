// Copyright (C) 2023 - present WD Studios Corp. All Rights Reserved.
#include "MainWindow.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyleFactory>

#include <ioSys/dag_dataBlock.h>

namespace
{
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
