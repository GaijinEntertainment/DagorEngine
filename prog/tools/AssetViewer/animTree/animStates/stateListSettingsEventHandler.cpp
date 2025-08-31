// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "stateListSettingsEventHandler.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "animStatesType.h"

#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>
#include <osApiWrappers/dag_clipboard.h>
#include <osApiWrappers/dag_direct.h>
#include <assets/assetUtils.h>
#include <ioSys/dag_dataBlock.h>

namespace ContextMenu
{
enum
{
  EDIT = 0,
};
}

StateListSettingsEventHandler::StateListSettingsEventHandler(dag::Vector<AnimStatesData> &states,
  dag::Vector<AnimCtrlData> &controllers, dag::Vector<BlendNodeData> &nodes, dag::Vector<String> &paths) :
  states(states), controllers(controllers), nodes(nodes), paths(paths)
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
    const SimpleString channelName = pluginPanel->getText(PID_STATES_NODES_LIST);
    const char *name = get_ctrl_name(*settings, channelName);
    if (!name)
      return false;

    if (strcmp(name, "") != 0 && strcmp(name, "*") != 0)
    {
      PropPanel::IMenu &menu = list_box_interface.createContextMenu();
      menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::EDIT, "Edit");
      menu.setEventHandler(this);
    }
    return true;
  }
  return false;
}

int StateListSettingsEventHandler::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case ContextMenu::EDIT: editSelectedNode(); break;
  }
  return 0;
}

void StateListSettingsEventHandler::setPluginEventHandler(PropPanel::ContainerPropertyControl *panel, DagorAsset *cur_asset,
  PropPanel::ControlEventHandler *event_handler)
{
  pluginPanel = panel;
  asset = cur_asset;
  pluginEventHandler = event_handler;
}

void StateListSettingsEventHandler::editSelectedNode()
{
  PropPanel::ContainerPropertyControl *ctrlsTree = pluginPanel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *nodesTree = pluginPanel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *statesTree = pluginPanel->getById(PID_ANIM_STATES_TREE)->getContainer();
  AnimStatesData *stateDescData =
    eastl::find_if(states.begin(), states.end(), [](const AnimStatesData &data) { return data.type == AnimStatesType::STATE_DESC; });
  String fullPath;
  DataBlock props =
    get_props_from_include_state_data(paths, controllers, *asset, ctrlsTree, *stateDescData, fullPath, /*only_includes*/ true);
  DataBlock *stateDesc = props.addBlock("stateDesc");
  DataBlock *settings = find_block_by_name(stateDesc, statesTree->getCaption(statesTree->getSelLeaf()));
  const SimpleString channelName = pluginPanel->getText(PID_STATES_NODES_LIST);
  const char *name = get_ctrl_name(*settings, channelName);
  AnimCtrlData *ctrlData = eastl::find_if(controllers.begin(), controllers.end(),
    [ctrlsTree, name](const AnimCtrlData &data) { return ctrlsTree->getCaption(data.handle) == name; });
  if (ctrlData != controllers.end())
    focus_selected_node(pluginEventHandler, pluginPanel, ctrlData->handle, PID_ANIM_BLEND_CTRLS_GROUP, PID_ANIM_BLEND_CTRLS_GROUP,
      PID_ANIM_BLEND_CTRLS_TREE);
  else
  {
    BlendNodeData *nodeData = eastl::find_if(nodes.begin(), nodes.end(),
      [nodesTree, name](const BlendNodeData &data) { return nodesTree->getCaption(data.handle) == name; });
    if (nodeData != nodes.end())
      focus_selected_node(pluginEventHandler, pluginPanel, nodeData->handle, PID_ANIM_BLEND_NODES_GROUP, PID_ANIM_BLEND_NODES_GROUP,
        PID_ANIM_BLEND_NODES_TREE);
  }
}
