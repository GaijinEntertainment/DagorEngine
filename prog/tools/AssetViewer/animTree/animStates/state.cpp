// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "state.h"
#include "animStatesData.h"
#include "animStatesType.h"
#include "../animTreeUtils.h"
#include "../animParamData.h"
#include "../animTreePanelPids.h"
#include "../animTree.h"

#include <propPanel/control/container.h>

static const float DEFAULT_DEF_MORPH_TIME = 0.15f;
static const float DEFAULT_FORCE_DUR = -1.f;
static const float DEFAULT_FORCE_SPD = -1.f;

void state_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx,
  const DataBlock &props)
{
  const float defMorphTime = props.getReal("defMorphTime", DEFAULT_DEF_MORPH_TIME);
  const float defMinTimeScale = props.getReal("minTimeScale", 0.f);
  const float defMaxTimeScale = props.getReal("maxTimeScale", FLT_MAX);
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "tag");
  add_edit_float_if_not_exists(params, panel, field_idx, "forceDur", DEFAULT_FORCE_DUR);
  add_edit_float_if_not_exists(params, panel, field_idx, "forceSpd", DEFAULT_FORCE_SPD);
  add_edit_float_if_not_exists(params, panel, field_idx, "morphTime", defMorphTime);
  add_edit_float_if_not_exists(params, panel, field_idx, "minTimeScale", defMinTimeScale);
  add_edit_float_if_not_exists(params, panel, field_idx, "maxTimeScale", defMaxTimeScale);
}

void state_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const DataBlock &props)
{
  const float defMorphTime = props.getReal("defMorphTime", DEFAULT_DEF_MORPH_TIME);
  const float defMinTimeScale = props.getReal("minTimeScale", 0.f);
  const float defMaxTimeScale = props.getReal("maxTimeScale", FLT_MAX);
  remove_param_if_default_str(params, panel, "tag");
  remove_param_if_default_float(params, panel, "forceDur", DEFAULT_FORCE_DUR);
  remove_param_if_default_float(params, panel, "forceSpd", DEFAULT_FORCE_SPD);
  remove_param_if_default_float(params, panel, "morphTime", defMorphTime);
  remove_param_if_default_float(params, panel, "minTimeScale", defMinTimeScale);
  remove_param_if_default_float(params, panel, "maxTimeScale", defMaxTimeScale);
}

static Tab<String> fill_free_channels_combo(PropPanel::ContainerPropertyControl *tree, const DataBlock &selected_block,
  dag::ConstSpan<String> names, dag::ConstSpan<AnimStatesData> states_data)
{
  Tab<String> freeChannelsCombo;
  if (selected_block.getBlockName())
    freeChannelsCombo.emplace_back(selected_block.getBlockName());

  for (const AnimStatesData &data : states_data)
    if (data.type == AnimStatesType::CHAN)
    {
      const String channelName = tree->getCaption(data.handle);
      if (eastl::find(names.begin(), names.end(), channelName) == names.end())
        freeChannelsCombo.emplace_back(channelName);
    }

  return freeChannelsCombo;
}

void state_init_block_settings(PropPanel::ContainerPropertyControl *prop_panel, const DataBlock &settings, const DataBlock &state_desc,
  dag::ConstSpan<AnimStatesData> states_data)
{
  PropPanel::ContainerPropertyControl *tree = prop_panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *panel = prop_panel->getById(PID_ANIM_STATES_SETTINGS_GROUP)->getContainer();
  Tab<String> names;
  names.reserve(settings.blockCount());
  const DataBlock *defaultBlock = &DataBlock::emptyBlock;
  for (int i = 0; i < settings.blockCount(); ++i)
  {
    const DataBlock *block = settings.getBlock(i);
    if (block->paramExists("name"))
    {
      names.emplace_back(block->getBlockName());
      if (defaultBlock == &DataBlock::emptyBlock)
        defaultBlock = block;
    }
  }
  Tab<String> freeChannelsCombo = fill_free_channels_combo(tree, *defaultBlock, names, states_data);

  const float defForceDur = settings.getReal("forceDur", DEFAULT_FORCE_DUR);
  const float defForceSpd = settings.getReal("forceSpd", DEFAULT_FORCE_SPD);
  const float defMorphTime = settings.getReal("morphTime", state_desc.getReal("defMorphTime", DEFAULT_DEF_MORPH_TIME));
  const float defMinTimeScale = settings.getReal("minTimeScale", state_desc.getReal("minTimeScale", 0.f));
  const float defMaxTimeScale = settings.getReal("maxTimeScale", state_desc.getReal("maxTimeScale", FLT_MAX));

  panel->createList(PID_STATES_NODES_LIST, "Channels", names, 0);
  panel->createButton(PID_STATES_NODES_LIST_ADD, "Add");
  panel->createButton(PID_STATES_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createCombo(PID_STATES_STATE_NODE_CHANNEL, "Channel", freeChannelsCombo, 0);
  panel->createEditBox(PID_STATES_STATE_NODE_NAME, "name", defaultBlock->getStr("name", ""));
  panel->createEditFloat(PID_STATES_STATE_NODE_FORCE_DUR, "forceDur", defaultBlock->getReal("forceDur", defForceDur));
  panel->createEditFloat(PID_STATES_STATE_NODE_FORCE_SPD, "forceSpd", defaultBlock->getReal("forceSpd", defForceSpd));
  panel->createEditFloat(PID_STATES_STATE_NODE_MORPH_TIME, "morphTime", defaultBlock->getReal("morphTime", defMorphTime));
  panel->createEditFloat(PID_STATES_STATE_NODE_MIN_TIME_SCALE, "minTimeScale", defaultBlock->getReal("minTimeScale", defMinTimeScale));
  panel->createEditFloat(PID_STATES_STATE_NODE_MAX_TIME_SCALE, "maxTimeScale", defaultBlock->getReal("maxTimeScale", defMaxTimeScale));
  if (names.empty())
    for (int i = PID_STATES_STATE_NODE_CHANNEL; i <= PID_STATES_STATE_NODE_MAX_TIME_SCALE; ++i)
      panel->setEnabledById(i, /*enabled*/ false);
}

void state_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings,
  const DataBlock &state_desc, dag::ConstSpan<AnimStatesData> states_data)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
  const SimpleString name = panel->getText(PID_STATES_NODES_LIST);
  const DataBlock *selectedBlock = &DataBlock::emptyBlock;
  for (int i = 0; i < settings.blockCount(); ++i)
  {
    const DataBlock *child = settings.getBlock(i);
    if (name == child->getBlockName())
      selectedBlock = child;
  }

  const float defForceDur = settings.getReal("forceDur", DEFAULT_FORCE_DUR);
  const float defForceSpd = settings.getReal("forceSpd", DEFAULT_FORCE_SPD);
  const float defMorphTime = settings.getReal("morphTime", state_desc.getReal("defMorphTime", DEFAULT_DEF_MORPH_TIME));
  const float defMinTimeScale = settings.getReal("minTimeScale", state_desc.getReal("minTimeScale", 0.f));
  const float defMaxTimeScale = settings.getReal("maxTimeScale", state_desc.getReal("maxTimeScale", FLT_MAX));

  dag::ConstSpan<String> names = panel->getStrings(PID_STATES_NODES_LIST);
  Tab<String> freeChannelsCombo = fill_free_channels_combo(tree, *selectedBlock, names, states_data);

  panel->setStrings(PID_STATES_STATE_NODE_CHANNEL, freeChannelsCombo);
  panel->setInt(PID_STATES_STATE_NODE_CHANNEL, 0);
  panel->setText(PID_STATES_STATE_NODE_NAME, selectedBlock->getStr("name", ""));
  panel->setFloat(PID_STATES_STATE_NODE_FORCE_DUR, selectedBlock->getReal("forceDur", defForceDur));
  panel->setFloat(PID_STATES_STATE_NODE_FORCE_SPD, selectedBlock->getReal("forceSpd", defForceSpd));
  panel->setFloat(PID_STATES_STATE_NODE_MORPH_TIME, selectedBlock->getReal("morphTime", defMorphTime));
  panel->setFloat(PID_STATES_STATE_NODE_MIN_TIME_SCALE, selectedBlock->getReal("minTimeScale", defMinTimeScale));
  panel->setFloat(PID_STATES_STATE_NODE_MAX_TIME_SCALE, selectedBlock->getReal("maxTimeScale", defMaxTimeScale));
}

void AnimTreePlugin::stateSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock &settings, AnimStatesData &data,
  const DataBlock &state_desc)
{
  const SimpleString listName = panel->getText(PID_STATES_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_STATES_NODES_LIST);
  if (listName.empty())
    return;

  DataBlock *selectedBlock = nullptr;
  for (int i = 0; i < settings.blockCount(); ++i)
  {
    DataBlock *child = settings.getBlock(i);
    if (listName == child->getBlockName())
      selectedBlock = child;
  }

  const float defForceDur = settings.getReal("forceDur", DEFAULT_FORCE_DUR);
  const float defForceSpd = settings.getReal("forceSpd", DEFAULT_FORCE_SPD);
  const float defMorphTime = settings.getReal("morphTime", state_desc.getReal("defMorphTime", DEFAULT_DEF_MORPH_TIME));
  const float defMinTimeScale = settings.getReal("minTimeScale", state_desc.getReal("minTimeScale", 0.f));
  const float defMaxTimeScale = settings.getReal("maxTimeScale", state_desc.getReal("maxTimeScale", FLT_MAX));

  const SimpleString channelName = panel->getText(PID_STATES_STATE_NODE_CHANNEL);
  const SimpleString fieldName = panel->getText(PID_STATES_STATE_NODE_NAME);
  const float forceDurValue = panel->getFloat(PID_STATES_STATE_NODE_FORCE_DUR);
  const float forceSpdValue = panel->getFloat(PID_STATES_STATE_NODE_FORCE_SPD);
  const float morphTimeValue = panel->getFloat(PID_STATES_STATE_NODE_MORPH_TIME);
  const float minTimeScaleValue = panel->getFloat(PID_STATES_STATE_NODE_MIN_TIME_SCALE);
  const float maxTimeScaleValue = panel->getFloat(PID_STATES_STATE_NODE_MAX_TIME_SCALE);

  if (!selectedBlock)
    selectedBlock = settings.addNewBlock(panel->getText(PID_STATES_STATE_NODE_CHANNEL));
  if (fieldName != selectedBlock->getStr("name", ""))
    data.childs[selectedIdx] = find_state_child_idx_by_name(panel, data.handle, controllersData, blendNodesData, fieldName);

  check_state_child_idx(data.childs[selectedIdx], settings.getStr("name"), fieldName);

  for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
    selectedBlock->removeParam(i);

  if (listName != channelName)
  {
    selectedBlock->changeBlockName(channelName.c_str());
    panel->setText(PID_STATES_NODES_LIST, channelName.c_str());
  }
  selectedBlock->setStr("name", fieldName);
  if (!is_equal_float(forceDurValue, defForceDur))
    selectedBlock->setReal("forceDur", forceDurValue);
  if (!is_equal_float(forceSpdValue, defForceSpd))
    selectedBlock->setReal("forceSpd", forceSpdValue);
  if (!is_equal_float(morphTimeValue, defMorphTime))
    selectedBlock->setReal("morphTime", morphTimeValue);
  if (!is_equal_float(minTimeScaleValue, defMinTimeScale))
    selectedBlock->setReal("minTimeScale", minTimeScaleValue);
  if (!is_equal_float(maxTimeScaleValue, defMaxTimeScale))
    selectedBlock->setReal("maxTimeScale", maxTimeScaleValue);
}

void state_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock &settings)
{
  const SimpleString listName = panel->getText(PID_STATES_NODES_LIST);
  for (int i = 0; i < settings.blockCount(); ++i)
  {
    DataBlock *child = settings.getBlock(i);
    if (listName == child->getBlockName())
    {
      settings.removeBlock(i);
      return;
    }
  }
}

const char *state_get_child_name_by_idx(const DataBlock &settings, int idx) { return settings.getBlock(idx)->getStr("name"); }

void state_update_child_name(DataBlock &settings, const char *name, const String &old_name)
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
