// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_mainWindow.h>

#include <debug/dag_debug.h>
#include <EditorCore/ec_wndPublic.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_symHlp.h>

EditorMainWindow::EditorMainWindow(IWndManagerEventHandler &event_handler) : eventHandler(event_handler) {}

void EditorMainWindow::onMainWindowCreated()
{
  G_ASSERT(!wndManager);
  wndManager.reset(createWindowManager());

  if (!::symhlp_load("daKernel-dev.dll"))
    DEBUG_CTX("can't load sym for: %s", "daKernel-dev.dll");

  DataBlock::fatalOnMissingFile = false;
  DataBlock::fatalOnLoadFailed = false;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::fatalOnMissingVar = false;

  eventHandler.onInit(wndManager.get());
}
