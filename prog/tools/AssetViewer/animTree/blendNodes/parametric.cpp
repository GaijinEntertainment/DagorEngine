// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "parametric.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockCommentsDef.h>
#include <propPanel/control/container.h>

const float DEFAULT_P_END = 1.0f;
const float DEFAULT_MULK = 1.0f;

void parametric_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx,
  bool default_foreign)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "varname");
  add_edit_box_if_not_exists(params, panel, field_idx, "key");
  add_edit_box_if_not_exists(params, panel, field_idx, "key_start");
  add_edit_box_if_not_exists(params, panel, field_idx, "key_end");
  add_edit_float_if_not_exists(params, panel, field_idx, "p_start");
  add_edit_float_if_not_exists(params, panel, field_idx, "p_end", DEFAULT_P_END);
  add_edit_bool_if_not_exists(params, panel, field_idx, "looping");
  add_edit_bool_if_not_exists(params, panel, field_idx, "eoa_irq");
  add_edit_box_if_not_exists(params, panel, field_idx, "apply_node_mask");
  add_edit_bool_if_not_exists(params, panel, field_idx, "disable_origin_vel");
  add_edit_bool_if_not_exists(params, panel, field_idx, "foreignAnimation", default_foreign);
  add_edit_float_if_not_exists(params, panel, field_idx, "addMoveDirH");
  add_edit_float_if_not_exists(params, panel, field_idx, "addMoveDirV");
  add_edit_float_if_not_exists(params, panel, field_idx, "addMoveDist");
  add_edit_float_if_not_exists(params, panel, field_idx, "mulk", DEFAULT_MULK);
  add_edit_float_if_not_exists(params, panel, field_idx, "addk");
}

void parametric_set_dependent_defaults(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
  set_str_param_by_name_if_default(params, panel, "key", nameValue.c_str());
  const SimpleString keyValue = get_str_param_by_name_optional(params, panel, "key");
  const String defaultKeyStart(0, "%s_start", keyValue.c_str());
  const String defaultKeyEnd(0, "%s_end", keyValue.c_str());
  set_str_param_by_name_if_default(params, panel, "key_start", defaultKeyStart.c_str());
  set_str_param_by_name_if_default(params, panel, "key_end", defaultKeyEnd.c_str());
}

void parametric_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, bool default_foreign)
{
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
  const SimpleString keyValue = get_str_param_by_name_optional(params, panel, "key");
  const String defaultKeyStart(0, "%s_start", keyValue.c_str());
  const String defaultKeyEnd(0, "%s_end", keyValue.c_str());

  remove_param_if_default_str(params, panel, "key", nameValue.c_str());
  remove_param_if_default_str(params, panel, "key_start", defaultKeyStart.c_str());
  remove_param_if_default_str(params, panel, "key_end", defaultKeyEnd.c_str());
  remove_param_if_default_float(params, panel, "p_start");
  remove_param_if_default_float(params, panel, "p_end", DEFAULT_P_END);
  remove_param_if_default_bool(params, panel, "looping");
  remove_param_if_default_bool(params, panel, "eoa_irq");
  remove_param_if_default_str(params, panel, "apply_node_mask");
  remove_param_if_default_bool(params, panel, "disable_origin_vel");
  remove_param_if_default_bool(params, panel, "foreignAnimation", default_foreign);
  remove_param_if_default_float(params, panel, "addMoveDirH");
  remove_param_if_default_float(params, panel, "addMoveDirV");
  remove_param_if_default_float(params, panel, "addMoveDist");
  remove_param_if_default_float(params, panel, "mulk", DEFAULT_MULK);
  remove_param_if_default_float(params, panel, "addk");
}

void parametric_init_block_settings(PropPanel::ContainerPropertyControl *prop_panel, const DataBlock *settings,
  dag::ConstSpan<AnimCtrlData> controllers)
{
  PropPanel::ContainerPropertyControl *panel = prop_panel->getById(PID_ANIM_BLEND_NODES_SETTINGS_GROUP)->getContainer();
  Tab<String> names;
  const DataBlock *resetBlock = settings->getBlockByName("resetRandomSwitchersOnAnimationEnd");
  if (resetBlock)
    for (int i = 0; i < resetBlock->blockCount(); ++i)
    {
      const DataBlock *block = resetBlock->getBlock(i);
      if (!CHECK_COMMENT_PREFIX(block->getBlockName()))
        names.emplace_back(String(resetBlock->getBlock(i)->getBlockName()));
    }

  bool isEditable = !names.empty();
  const char *defaultComboValue = names.empty() ? "" : names[0].c_str();
  Tab<String> randomSwitchNames = collect_random_switch_names(prop_panel, controllers, settings, defaultComboValue);
  panel->createList(PID_NODES_RESET_RANDOM_SWITCH_LIST, "resetRandomSwitchersOnAnimationEnd", names, 0);
  panel->createButton(PID_NODES_RESET_RANDOM_SWITCH_LIST_ADD, "Add");
  panel->createButton(PID_NODES_RESET_RANDOM_SWITCH_LIST_REMOVE, "Remove", isEditable, /*new_line*/ false);
  panel->createCombo(PID_NODES_RESET_RANDOM_SWITCH_COMBO, "randomSwitch", randomSwitchNames, defaultComboValue, isEditable);
}

void parametric_set_selected_reset_random_switch_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings,
  dag::ConstSpan<AnimCtrlData> controllers)
{
  const SimpleString listName = panel->getText(PID_NODES_RESET_RANDOM_SWITCH_LIST);
  const int selectedIdx = panel->getInt(PID_NODES_RESET_RANDOM_SWITCH_LIST);
  const DataBlock *resetBlock = settings.getBlockByNameEx("resetRandomSwitchersOnAnimationEnd");
  int blockIdx = get_block_name_idx_by_list_name(*resetBlock, listName, selectedIdx);
  const char *selectedName = blockIdx != -1 ? resetBlock->getBlock(blockIdx)->getBlockName() : "";
  Tab<String> randomSwitchNames = collect_random_switch_names(panel, controllers, &settings, selectedName);
  panel->setStrings(PID_NODES_RESET_RANDOM_SWITCH_COMBO, randomSwitchNames);
  panel->setText(PID_NODES_RESET_RANDOM_SWITCH_COMBO, selectedName);
}

void parametric_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock &settings)
{
  const SimpleString listName = panel->getText(PID_NODES_RESET_RANDOM_SWITCH_LIST);
  const int selectedIdx = panel->getInt(PID_NODES_RESET_RANDOM_SWITCH_LIST);
  const SimpleString comboValue = panel->getText(PID_NODES_RESET_RANDOM_SWITCH_COMBO);
  if (listName.empty() || selectedIdx < 0 || comboValue.empty())
    return;

  DataBlock *resetBlock = settings.getBlockByName("resetRandomSwitchersOnAnimationEnd");
  if (!resetBlock)
    resetBlock = settings.addBlock("resetRandomSwitchersOnAnimationEnd");

  int blockIdx = get_block_name_idx_by_list_name(*resetBlock, listName, selectedIdx);
  if (blockIdx >= 0)
    resetBlock->getBlock(blockIdx)->changeBlockName(comboValue.c_str());
  else
    resetBlock->addBlock(comboValue.c_str());

  if (listName != comboValue)
    panel->setText(PID_NODES_RESET_RANDOM_SWITCH_LIST, comboValue.c_str());

  panel->setEnabledById(PID_NODES_RESET_RANDOM_SWITCH_LIST_ADD, /*enabled*/ true);
}

void parametric_add_reset_random_switch_to_list(PropPanel::ContainerPropertyControl *panel)
{
  dag::ConstSpan<String> names = panel->getStrings(PID_NODES_RESET_RANDOM_SWITCH_LIST);
  if (names.empty())
    panel->setEnabledById(PID_NODES_RESET_RANDOM_SWITCH_LIST_REMOVE, /*enabled*/ true);
  const int idx = panel->addString(PID_NODES_RESET_RANDOM_SWITCH_LIST, "##");
  panel->setInt(PID_NODES_RESET_RANDOM_SWITCH_LIST, idx);
  panel->setEnabledById(PID_NODES_RESET_RANDOM_SWITCH_COMBO, /*enabled*/ true);
  panel->setEnabledById(PID_NODES_RESET_RANDOM_SWITCH_LIST_ADD, /*enabled*/ false);
}

void parametric_remove_reset_random_switch_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock &settings)
{
  const int idx = panel->getInt(PID_NODES_RESET_RANDOM_SWITCH_LIST);
  if (idx < 0)
    return;

  DataBlock *resetBlock = settings.getBlockByName("resetRandomSwitchersOnAnimationEnd");
  if (!resetBlock)
    return;

  dag::ConstSpan<String> names = panel->getStrings(PID_NODES_RESET_RANDOM_SWITCH_LIST);
  if (idx >= names.size())
    return;

  const bool isRemovedBlock = resetBlock->removeBlock(names[idx].c_str());
  if (isRemovedBlock)
    panel->setEnabledById(PID_NODES_RESET_RANDOM_SWITCH_LIST_ADD, /*enabled*/ true);
  panel->removeString(PID_NODES_RESET_RANDOM_SWITCH_LIST, idx);
  const bool isLastRemoved = names.size() == 1;
  panel->setEnabledById(PID_NODES_RESET_RANDOM_SWITCH_COMBO, !isLastRemoved);
  panel->setEnabledById(PID_NODES_RESET_RANDOM_SWITCH_LIST_REMOVE, !isLastRemoved);
  if (isLastRemoved)
    settings.removeBlock("resetRandomSwitchersOnAnimationEnd");
}
