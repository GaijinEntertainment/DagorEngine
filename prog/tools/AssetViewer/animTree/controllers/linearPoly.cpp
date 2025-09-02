// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "linearPoly.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "../animTree.h"
#include "animCtrlData.h"

#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

static const bool DEFAULT_SPLIT_CHANS = true;

void linear_poly_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "varname");
  add_edit_float_if_not_exists(params, panel, field_idx, "morphTime");
  add_edit_bool_if_not_exists(params, panel, field_idx, "enclosed");
  add_edit_float_if_not_exists(params, panel, field_idx, "paramTau");
  add_edit_float_if_not_exists(params, panel, field_idx, "paramSpeed");
  add_edit_bool_if_not_exists(params, panel, field_idx, "splitChans", DEFAULT_SPLIT_CHANS);
  add_edit_box_if_not_exists(params, panel, field_idx, "accept_name_mask_re");
  add_edit_box_if_not_exists(params, panel, field_idx, "decline_name_mask_re");
}

void linear_poly_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "varname");
  remove_param_if_default_float(params, panel, "morphTime");
  remove_param_if_default_bool(params, panel, "enclosed");
  remove_param_if_default_float(params, panel, "paramTau");
  remove_param_if_default_float(params, panel, "paramSpeed");
  remove_param_if_default_bool(params, panel, "splitChans", DEFAULT_SPLIT_CHANS);
  remove_param_if_default_str(params, panel, "accept_name_mask_re");
  remove_param_if_default_str(params, panel, "decline_name_mask_re");
}

void linear_poly_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  Tab<String> names;
  names.reserve(settings->blockCount());
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *block = settings->getBlock(i);
    if (block->paramExists("name"))
      names.emplace_back(block->getStr("name"));
  }

  const DataBlock *defaultBlock = settings->getBlockByNameEx("child");

  bool isEditable = !names.empty();
  panel->createList(PID_CTRLS_NODES_LIST, "Childs", names, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_LINEAR_POLY_NODE_NAME, "name", defaultBlock->getStr("name", ""), isEditable);
  panel->createEditFloat(PID_CTRLS_LINEAR_POLY_NODE_VAL, "val", defaultBlock->getReal("val", 0.f), /*prec*/ 2, isEditable);
}

void AnimTreePlugin::linearPolySaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data)
{
  // Find child block based on idx, becuse names can duplicate
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (selectedIdx == -1)
    return;

  const SimpleString selectedName = panel->getText(PID_CTRLS_NODES_LIST);
  const SimpleString fieldName = panel->getText(PID_CTRLS_LINEAR_POLY_NODE_NAME);
  const int blockIdx = get_child_block_idx_by_list_idx(settings, selectedIdx);
  DataBlock *selectedBlock = settings->getBlock(blockIdx);

  if (!selectedBlock)
  {
    selectedBlock = settings->addNewBlock("child");
    selectedBlock->setStr("name", selectedName.c_str());
  }
  if (selectedName != fieldName || data.childs[selectedIdx] == AnimCtrlData::NOT_FOUND_CHILD)
  {
    data.childs[selectedIdx] = find_child_idx_by_name(panel, data.handle, controllersData, blendNodesData, fieldName);
    check_ctrl_child_idx(data.childs[selectedIdx], settings->getStr("name"), fieldName);
  }

  const char *blockName = selectedBlock->getStr("name");
  G_ASSERTF(selectedName == blockName, "Wrong order blocks in blk: %s, expected from list: %s", blockName, selectedName.c_str());

  for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
    selectedBlock->removeParam(i);

  const float val = panel->getFloat(PID_CTRLS_LINEAR_POLY_NODE_VAL);
  selectedBlock->setStr("name", fieldName.c_str());
  if (float_nonzero(val))
    selectedBlock->setReal("val", val);

  if (selectedName != fieldName)
    panel->setText(PID_CTRLS_NODES_LIST, fieldName.c_str());
}

void linear_poly_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  // Find child block based on idx, becuse names can duplicate
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  const int blockIdx = get_child_block_idx_by_list_idx(settings, selectedIdx);
  const DataBlock *selectedBlock = settings->getBlock(blockIdx);

  // In case if we add new_node
  if (!selectedBlock)
    selectedBlock = &DataBlock::emptyBlock;

  panel->setText(PID_CTRLS_LINEAR_POLY_NODE_NAME, name.c_str());
  panel->setFloat(PID_CTRLS_LINEAR_POLY_NODE_VAL, selectedBlock->getReal("val", 0.f));
  bool isEditable = selectedIdx >= 0;
  panel->setEnabledById(PID_CTRLS_LINEAR_POLY_NODE_NAME, isEditable);
  panel->setEnabledById(PID_CTRLS_LINEAR_POLY_NODE_VAL, isEditable);
}

void linear_poly_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (selectedIdx < settings->blockCount())
  {
    const int removeIdx = get_child_block_idx_by_list_idx(settings, selectedIdx);
    if (removeIdx == -1)
    {
      logerr("Can't find block with idx from list <%d> and name <%s>", selectedIdx, removeName.c_str());
      return;
    }

    const char *blockName = settings->getBlock(removeIdx)->getStr("name");
    G_ASSERTF(removeName == blockName, "Remove wrong child block in blk: %s, expected from list: %s", blockName, removeName.c_str());
    settings->removeBlock(removeIdx);
  }
}

void AnimTreePlugin::linearPolyFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings)
{
  int childNid = settings.getNameId("child");
  for (int i = 0; i < settings.blockCount(); ++i)
    if (settings.getBlock(i)->getNameId() == childNid)
    {
      const char *childName = settings.getBlock(i)->getStr("name");
      int idx = add_ctrl_child_idx_by_name(panel, data, controllersData, blendNodesData, childName);
      check_ctrl_child_idx(idx, settings.getStr("name"), childName);
    }
}

const char *linear_poly_get_child_name_by_idx(const DataBlock &settings, int idx) { return settings.getBlock(idx)->getStr("name"); }

String linear_poly_get_child_prefix_name(const DataBlock &settings, int idx)
{
  return String(0, "[%f] ", settings.getBlock(idx)->getReal("val", 0.f));
}

void linear_poly_update_child_name(DataBlock &settings, const char *name, const String &old_name)
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
