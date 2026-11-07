// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodeListEventHandler.h"
#include "animTreePanelPids.h"

int NodeListEventHandler::onMenuItemClick(unsigned id)
{
  if (id == ContextMenu::EDIT)
    editSelectedNode();
  return 0;
}

void NodeListEventHandler::setEventHandler(PropPanel::ContainerPropertyControl *panel, AnimTreeEditor *event_handler)
{
  pluginPanel = panel;
  eventHandler = event_handler;
}

void NodeListEventHandler::createEditMenu(PropPanel::IListBoxInterface &list_box_interface)
{
  PropPanel::IMenu &menu = list_box_interface.createContextMenu();
  menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::EDIT, "Edit");
  menu.setEventHandler(this);
}
