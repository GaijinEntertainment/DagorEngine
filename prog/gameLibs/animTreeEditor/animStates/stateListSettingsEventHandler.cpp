// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "stateListSettingsEventHandler.h"
#include "../animTreeEditor.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "animStatesType.h"

#include <propPanel/control/container.h>
#include <osApiWrappers/dag_clipboard.h>
#include <osApiWrappers/dag_direct.h>
#include <assets/assetUtils.h>
#include <ioSys/dag_dataBlock.h>

StateListSettingsEventHandler::StateListSettingsEventHandler(dag::Vector<AnimStatesData> &states,
  dag::Vector<AnimCtrlData> &controllers, dag::Vector<String> &paths) :
  states(states), controllers(controllers), paths(paths)
{}

const char *get_ctrl_name(const DataBlock &settings, const SimpleString &channel)
{
  for (int i = 0; i < settings.blockCount(); ++i)
  {
    if (channel == settings.getBlock(i)->getBlockName())
      return settings.getBlock(i)->getStr("name", "");
  }

  return nullptr;
}

bool StateListSettingsEventHandler::onListBoxContextMenu(int pcb_id, PropPanel::IListBoxInterface &list_box_interface)
{
  if (pcb_id == PID_STATES_NODES_LIST)
  {
    PropPanel::ContainerPropertyControl *ctrlsTree = pluginPanel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
    PropPanel::ContainerPropertyControl *statesTree = pluginPanel->getById(PID_ANIM_STATES_TREE)->getContainer();
    AnimStatesData *stateDescData =
      eastl::find_if(states.begin(), states.end(), [](const AnimStatesData &data) { return data.type == AnimStatesType::STATE_DESC; });
    String fullPath;
    DataBlock props =
      get_props_from_include_state_data(paths, controllers, *asset, ctrlsTree, *stateDescData, fullPath, /*only_includes*/ true);
    DataBlock *stateDesc = props.addBlock("stateDesc");
    DataBlock *settings = find_block_by_name(stateDesc, statesTree->getCaption(statesTree->getSelLeaf()));
    if (!settings)
      return false;
    const SimpleString channelName = pluginPanel->getText(PID_STATES_NODES_LIST);
    const char *name = get_ctrl_name(*settings, channelName);
    if (!name)
      return false;

    if (strcmp(name, "") != 0 && strcmp(name, "*") != 0)
      createEditMenu(list_box_interface);
    return true;
  }
  return false;
}

void StateListSettingsEventHandler::setEventHandler(PropPanel::ContainerPropertyControl *panel, DagorAsset *cur_asset,
  AnimTreeEditor *event_handler)
{
  NodeListEventHandler::setEventHandler(panel, event_handler);
  asset = cur_asset;
}

void StateListSettingsEventHandler::editSelectedNode()
{
  PropPanel::ContainerPropertyControl *ctrlsTree = pluginPanel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *statesTree = pluginPanel->getById(PID_ANIM_STATES_TREE)->getContainer();
  AnimStatesData *stateDescData =
    eastl::find_if(states.begin(), states.end(), [](const AnimStatesData &data) { return data.type == AnimStatesType::STATE_DESC; });
  String fullPath;
  DataBlock props =
    get_props_from_include_state_data(paths, controllers, *asset, ctrlsTree, *stateDescData, fullPath, /*only_includes*/ true);
  DataBlock *stateDesc = props.addBlock("stateDesc");
  DataBlock *settings = find_block_by_name(stateDesc, statesTree->getCaption(statesTree->getSelLeaf()));
  if (!settings)
    return;
  const SimpleString channelName = pluginPanel->getText(PID_STATES_NODES_LIST);
  if (const char *name = get_ctrl_name(*settings, channelName))
    eventHandler->focusNodeByName(name);
}
