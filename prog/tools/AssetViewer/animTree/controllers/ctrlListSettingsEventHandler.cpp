// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ctrlListSettingsEventHandler.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"

#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>
#include <osApiWrappers/dag_clipboard.h>
#include <osApiWrappers/dag_direct.h>
#include <assets/assetUtils.h>

namespace ContextMenu
{
enum
{
  EDIT = 0,
};
}

CtrlListSettingsEventHandler::CtrlListSettingsEventHandler(dag::Vector<AnimCtrlData> &controllers, dag::Vector<BlendNodeData> &nodes) :
  controllers(controllers), nodes(nodes)
{}

bool CtrlListSettingsEventHandler::onListBoxContextMenu(int pcb_id, PropPanel::IListBoxInterface &list_box_interface)
{
  if (pcb_id == PID_CTRLS_NODES_LIST)
  {
    PropPanel::IMenu &menu = list_box_interface.createContextMenu();
    menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::EDIT, "Edit");
    menu.setEventHandler(this);
    return true;
  }
  return false;
}

int CtrlListSettingsEventHandler::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case ContextMenu::EDIT: editSelectedNode(); break;
  }
  return 0;
}

void CtrlListSettingsEventHandler::setPluginEventHandler(PropPanel::ContainerPropertyControl *panel,
  PropPanel::ControlEventHandler *event_handler)
{
  pluginPanel = panel;
  pluginEventHandler = event_handler;
}

void CtrlListSettingsEventHandler::editSelectedNode()
{
  PropPanel::ContainerPropertyControl *ctrlsTree = pluginPanel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *nodesTree = pluginPanel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  const SimpleString name = pluginPanel->getText(PID_CTRLS_NODES_LIST);
  AnimCtrlData *ctrlData = eastl::find_if(controllers.begin(), controllers.end(),
    [ctrlsTree, &name](const AnimCtrlData &data) { return ctrlsTree->getCaption(data.handle) == name.c_str(); });
  if (ctrlData != controllers.end())
    focus_selected_node(pluginEventHandler, pluginPanel, ctrlData->handle, PID_ANIM_BLEND_CTRLS_GROUP, PID_ANIM_BLEND_CTRLS_GROUP,
      PID_ANIM_BLEND_CTRLS_TREE);
  else
  {
    BlendNodeData *nodeData = eastl::find_if(nodes.begin(), nodes.end(),
      [nodesTree, &name](const BlendNodeData &data) { return nodesTree->getCaption(data.handle) == name.c_str(); });
    if (nodeData != nodes.end())
      focus_selected_node(pluginEventHandler, pluginPanel, nodeData->handle, PID_ANIM_BLEND_NODES_GROUP, PID_ANIM_BLEND_NODES_GROUP,
        PID_ANIM_BLEND_NODES_TREE);
  }
}
