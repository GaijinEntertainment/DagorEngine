// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hub.h"
#include "../animTreeUtils.h"
#include "../animTreeDragListHandler.h"
#include "../animTreePanelPids.h"
#include "../animTree.h"
#include "animCtrlData.h"

#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

static const bool DEFAULT_CONST = true;
static const bool DEFAULT_SPLIT_CHANS = false;
static const bool DEFAULT_OPTIONAL = false;
static const bool DEFAULT_ENABLED = true;
static const float DEFAULT_WEIGHT = 1.0f;

void hub_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_bool_if_not_exists(params, panel, field_idx, "const", DEFAULT_CONST);
  add_edit_bool_if_not_exists(params, panel, field_idx, "splitChans", DEFAULT_SPLIT_CHANS);
  add_edit_box_if_not_exists(params, panel, field_idx, "accept_name_mask_re");
  add_edit_box_if_not_exists(params, panel, field_idx, "decline_name_mask_re");
}

void hub_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_bool(params, panel, "const", DEFAULT_CONST);
  remove_param_if_default_bool(params, panel, "splitChans", DEFAULT_SPLIT_CHANS);
  remove_param_if_default_str(params, panel, "accept_name_mask_re");
  remove_param_if_default_str(params, panel, "decline_name_mask_re");
}

void hub_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  Tab<String> names;
  names.reserve(settings->blockCount());
  const int childNid = settings->getNameId("child");
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *block = settings->getBlock(i);
    if (block->getNameId() == childNid)
      names.emplace_back(block->getStr("name", ""));
  }

  const DataBlock *defaultBlock = settings->getBlockByNameEx("child");

  bool isEditable = !names.empty();
  panel->createList(PID_CTRLS_NODES_LIST, "Childs", names, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_HUB_NODE_NAME, "name", defaultBlock->getStr("name", ""), isEditable);
  panel->createCheckBox(PID_CTRLS_HUB_NODE_OPTIONAL, "optional", defaultBlock->getBool("optional", DEFAULT_OPTIONAL), isEditable);
  panel->createCheckBox(PID_CTRLS_HUB_NODE_ENABLED, "enabled", defaultBlock->getBool("enabled", DEFAULT_ENABLED), isEditable);
  panel->createEditFloat(PID_CTRLS_HUB_NODE_WEIGHT, "weight", defaultBlock->getReal("weight", DEFAULT_WEIGHT), /*prec*/ 2, isEditable);
}

void AnimTreePlugin::hubSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data)
{
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (listName.empty())
    return;

  DataBlock *selectedBlock = nullptr;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    DataBlock *child = settings->getBlock(i);
    if (listName == child->getStr("name", nullptr))
      selectedBlock = child;
  }
  const SimpleString fieldName = panel->getText(PID_CTRLS_HUB_NODE_NAME);
  const bool optional = panel->getBool(PID_CTRLS_HUB_NODE_OPTIONAL);
  const bool enabled = panel->getBool(PID_CTRLS_HUB_NODE_ENABLED);
  const float weight = panel->getFloat(PID_CTRLS_HUB_NODE_WEIGHT);
  if (!selectedBlock)
    selectedBlock = settings->addNewBlock("child");
  if (listName != fieldName || data.childs[selectedIdx] == AnimCtrlData::NOT_FOUND_CHILD)
    data.childs[selectedIdx] = find_child_idx_by_name(panel, data.handle, controllersData, blendNodesData, fieldName);

  check_ctrl_child_idx(data.childs[selectedIdx], settings->getStr("name"), fieldName, optional);

  for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
    selectedBlock->removeParam(i);

  selectedBlock->setStr("name", fieldName.c_str());
  if (optional != DEFAULT_OPTIONAL)
    selectedBlock->setBool("optional", optional);
  if (enabled != DEFAULT_ENABLED)
    selectedBlock->setBool("enabled", enabled);
  if (!is_equal_float(weight, DEFAULT_WEIGHT))
    selectedBlock->setReal("weight", weight);

  if (listName != fieldName)
    panel->setText(PID_CTRLS_NODES_LIST, fieldName.c_str());
}

void hub_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *selectedBlock = &DataBlock::emptyBlock;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if (name == child->getStr("name", nullptr))
      selectedBlock = child;
  }
  panel->setText(PID_CTRLS_HUB_NODE_NAME, name.c_str());
  panel->setBool(PID_CTRLS_HUB_NODE_OPTIONAL, selectedBlock->getBool("optional", DEFAULT_OPTIONAL));
  panel->setBool(PID_CTRLS_HUB_NODE_ENABLED, selectedBlock->getBool("enabled", DEFAULT_ENABLED));
  panel->setFloat(PID_CTRLS_HUB_NODE_WEIGHT, selectedBlock->getReal("weight", DEFAULT_WEIGHT));
  bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  for (int i = PID_CTRLS_HUB_NODE_NAME; i <= PID_CTRLS_HUB_NODE_WEIGHT; ++i)
    panel->setEnabledById(i, isEditable);
}

void hub_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const int removeIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  dag::Vector<int> positions = collect_block_positions_by_name(*settings, "child");
  if (removeIdx >= 0 && removeIdx < positions.size())
    settings->removeBlock(positions[removeIdx]);
}

void AnimTreePlugin::hubFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings)
{
  int childNid = settings.getNameId("child");
  for (int i = 0; i < settings.blockCount(); ++i)
    if (settings.getBlock(i)->getNameId() == childNid)
    {
      const DataBlock *childBlock = settings.getBlock(i);
      const char *childName = childBlock->getStr("name", nullptr);
      if (childName)
      {
        const int idx = add_ctrl_child_idx_by_name(panel, data, controllersData, blendNodesData, childName);
        check_ctrl_child_idx(idx, settings.getStr("name"), childName, childBlock->getBool("optional", false));
      }
      else
      {
        data.childs.emplace_back(AnimCtrlData::NOT_FOUND_CHILD);
        logerr("hub <%s>: child block without name param", settings.getStr("name"));
      }
    }
}

const char *hub_get_child_name_by_idx(const DataBlock &settings, int idx) { return settings.getBlock(idx)->getStr("name", nullptr); }

bool hub_get_child_is_optional_by_idx(const DataBlock &settings, int idx)
{
  return settings.getBlock(idx)->getBool("optional", false);
}

void hub_update_child_name(DataBlock &settings, const char *name, const String &old_name)
{
  for (int i = 0; i < settings.blockCount(); ++i)
  {
    DataBlock *child = settings.getBlock(i);
    const char *childName = child->getStr("name", nullptr);
    String writeName;
    if (childName && get_updated_child_name(name, old_name, childName, writeName))
      child->setStr("name", writeName.c_str());
  }
}

class HubReorderHandler : public BaseCtrlReorderHandler
{
public:
  HubReorderHandler(AnimTreePlugin &plugin, dag::ConstSpan<AnimCtrlData> controllers, PropPanel::ContainerPropertyControl *panel,
    AnimCtrlData *ctrl_data) :
    BaseCtrlReorderHandler(plugin, controllers, panel), ctrlData(ctrl_data)
  {}

protected:
  void handleSpecificReorder(DataBlock &settings, int from, int to) override
  {
    dag::Vector<int> positions = collect_block_positions_by_name(settings, "child");
    move_block_at_positions(settings, positions, from, to);
    if (ctrlData && positions.size() == ctrlData->childs.size())
      move_childs(ctrlData->childs, from, to);
  }

  AnimCtrlData *ctrlData;
};

IListReorderHandler *hub_get_reorder_handler(AnimTreePlugin &plugin, dag::ConstSpan<AnimCtrlData> controllers,
  PropPanel::ContainerPropertyControl *panel, AnimCtrlData *ctrl_data)
{
  return new HubReorderHandler(plugin, controllers, panel, ctrl_data);
}
