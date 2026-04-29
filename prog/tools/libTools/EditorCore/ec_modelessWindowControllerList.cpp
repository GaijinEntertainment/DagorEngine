// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_modelessWindowControllerList.h>

#include <EditorCore/ec_modelessWindowController.h>

#include <ioSys/dag_dataBlock.h>

void ModelessWindowControllerList::addWindowController(IModelessWindowController &controller)
{
  const char *id = controller.getWindowId();

  // Validate id because it is used as a DataBlock parameter name in saveState().
  for (const char *idCheck = id; *idCheck != 0; ++idCheck)
    G_ASSERT(dblk::is_ident_char(*idCheck));

  G_ASSERT(getIndexById(id) < 0);
  windowControllers.push_back(&controller);
}

void ModelessWindowControllerList::toggleShowById(const char *id)
{
  const int index = getIndexById(id);
  if (index < 0)
    return;

  IModelessWindowController *controller = windowControllers[index];
  controller->showWindow(!controller->isWindowShown());
}

void ModelessWindowControllerList::releaseAllWindows()
{
  for (IModelessWindowController *controller : windowControllers)
    controller->releaseWindow();

  windowControllers.clear();
}

void ModelessWindowControllerList::setDefaultState()
{
  // Releasing the windows is needed because the default initial size is set when creating the window.
  for (IModelessWindowController *controller : windowControllers)
    controller->releaseWindow();
}

void ModelessWindowControllerList::loadState(const DataBlock &blk)
{
  setDefaultState();

  const DataBlock *windowsBlk = blk.getBlockByNameEx("windows");
  for (IModelessWindowController *controller : windowControllers)
  {
    const DataBlock *windowBlk = windowsBlk->getBlockByNameEx(controller->getWindowId());
    const bool visible = windowBlk->getBool("visible", false);
    controller->showWindow(visible);
  }
}

void ModelessWindowControllerList::saveState(DataBlock &blk)
{
  DataBlock *windowsBlk = blk.addBlock("windows");
  for (IModelessWindowController *controller : windowControllers)
  {
    DataBlock *windowBlk = windowsBlk->addBlock(controller->getWindowId());
    windowBlk->setBool("visible", controller->isWindowShown());
  }
}

int ModelessWindowControllerList::getIndexById(const char *id) const
{
  for (int i = 0; i < windowControllers.size(); ++i)
    if (strcmp(windowControllers[i]->getWindowId(), id) == 0)
      return i;
  return -1;
}
