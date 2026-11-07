// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ctrlListSettingsEventHandler.h"
#include "../animTreeEditor.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"

#include <propPanel/control/container.h>

CtrlListSettingsEventHandler::CtrlListSettingsEventHandler(dag::Vector<AnimCtrlData> &controllers) : controllers(controllers) {}

bool CtrlListSettingsEventHandler::onListBoxContextMenu(int pcb_id, PropPanel::IListBoxInterface &list_box_interface)
{
  if (pcb_id == PID_CTRLS_NODES_LIST)
  {
    PropPanel::ContainerPropertyControl *ctrlsTree = pluginPanel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
    PropPanel::TLeafHandle selLeaf = ctrlsTree->getSelLeaf();
    if (!selLeaf)
      return false;
    AnimCtrlData *ctrlData = find_data_by_handle(controllers, selLeaf);
    if (ctrlData == controllers.end() || ctrlData->childs.empty())
      return false;

    createEditMenu(list_box_interface);
    return true;
  }
  return false;
}

void CtrlListSettingsEventHandler::editSelectedNode()
{
  const SimpleString name = pluginPanel->getText(PID_CTRLS_NODES_LIST);
  eventHandler->focusNodeByName(name);
}
