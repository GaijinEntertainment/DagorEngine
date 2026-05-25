// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resetRandomSwitchListEventHandler.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>
#include <util/dag_simpleString.h>

namespace ContextMenu
{
enum
{
  EDIT = 0,
};
}

ResetRandomSwitchListEventHandler::ResetRandomSwitchListEventHandler(dag::Vector<AnimCtrlData> &controllers) : controllers(controllers)
{}

bool ResetRandomSwitchListEventHandler::onListBoxContextMenu(int pcb_id, PropPanel::IListBoxInterface &list_box_interface)
{
  if (pcb_id == PID_NODES_RESET_RANDOM_SWITCH_LIST)
  {
    PropPanel::ContainerPropertyControl *ctrlsTree = pluginPanel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
    const SimpleString selectedName = pluginPanel->getText(PID_NODES_RESET_RANDOM_SWITCH_LIST);
    if (selectedName.empty())
      return false;

    PropPanel::IMenu &menu = list_box_interface.createContextMenu();
    menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::EDIT, "Edit");
    menu.setEventHandler(this);
    return true;
  }
  return false;
}

int ResetRandomSwitchListEventHandler::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case ContextMenu::EDIT: editSelectedNode(); break;
  }
  return 0;
}

void ResetRandomSwitchListEventHandler::setPluginEventHandler(PropPanel::ContainerPropertyControl *panel,
  PropPanel::ControlEventHandler *event_handler)
{
  pluginPanel = panel;
  pluginEventHandler = event_handler;
}

void ResetRandomSwitchListEventHandler::editSelectedNode()
{
  PropPanel::ContainerPropertyControl *ctrlsTree = pluginPanel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  const SimpleString name = pluginPanel->getText(PID_NODES_RESET_RANDOM_SWITCH_LIST);
  AnimCtrlData *ctrlData = eastl::find_if(controllers.begin(), controllers.end(),
    [ctrlsTree, &name](const AnimCtrlData &data) { return ctrlsTree->getCaption(data.handle) == name.c_str(); });
  if (ctrlData != controllers.end())
    focus_selected_node(pluginEventHandler, pluginPanel, ctrlData->handle, PID_ANIM_BLEND_CTRLS_GROUP, PID_ANIM_BLEND_CTRLS_GROUP,
      PID_ANIM_BLEND_CTRLS_TREE);
}
