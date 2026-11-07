// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resetRandomSwitchListEventHandler.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include <propPanel/control/container.h>
#include <util/dag_simpleString.h>

ResetRandomSwitchListEventHandler::ResetRandomSwitchListEventHandler(dag::Vector<AnimCtrlData> &controllers) : controllers(controllers)
{}

bool ResetRandomSwitchListEventHandler::onListBoxContextMenu(int pcb_id, PropPanel::IListBoxInterface &list_box_interface)
{
  if (pcb_id == PID_NODES_RESET_RANDOM_SWITCH_LIST)
  {
    const SimpleString selectedName = pluginPanel->getText(PID_NODES_RESET_RANDOM_SWITCH_LIST);
    if (selectedName.empty())
      return false;

    createEditMenu(list_box_interface);
    return true;
  }
  return false;
}

void ResetRandomSwitchListEventHandler::editSelectedNode()
{
  PropPanel::ContainerPropertyControl *ctrlsTree = pluginPanel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  const SimpleString name = pluginPanel->getText(PID_NODES_RESET_RANDOM_SWITCH_LIST);
  AnimCtrlData *ctrlData = eastl::find_if(controllers.begin(), controllers.end(),
    [ctrlsTree, &name](const AnimCtrlData &data) { return ctrlsTree->getCaption(data.handle) == name.c_str(); });
  if (ctrlData != controllers.end())
    focus_selected_node(eventHandler, pluginPanel, ctrlData->handle, PID_ANIM_BLEND_CTRLS_GROUP, PID_ANIM_BLEND_CTRLS_GROUP,
      PID_ANIM_BLEND_CTRLS_TREE);
}
