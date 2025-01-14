// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "randomSwitch.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "../animTree.h"
#include "animCtrlData.h"

#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

static const bool DEFAULT_SPLIT_CHANS = true;

void random_switch_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "varname");
  add_edit_bool_if_not_exists(params, panel, field_idx, "splitChans", DEFAULT_SPLIT_CHANS);
  add_edit_box_if_not_exists(params, panel, field_idx, "accept_name_mask_re");
  add_edit_box_if_not_exists(params, panel, field_idx, "decline_name_mask_re");
}

void random_switch_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "varname");
  remove_param_if_default_bool(params, panel, "splitChans", DEFAULT_SPLIT_CHANS);
  remove_param_if_default_str(params, panel, "accept_name_mask_re");
  remove_param_if_default_str(params, panel, "decline_name_mask_re");
}

void random_switch_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const DataBlock *weights = settings->getBlockByName("weight");
  const DataBlock *maxrepeats = settings->getBlockByName("maxrepeat");
  const char *defaultName = weights ? weights->getParamName(0) : "";
  const float defaultWeight = weights ? weights->getReal(0) : 0.f;
  const int defaultMaxRepeat = maxrepeats ? maxrepeats->getInt(defaultName, 1) : 1;
  Tab<String> names;
  if (weights)
    for (int i = 0; i < weights->paramCount(); ++i)
      names.emplace_back(weights->getParamName(i));
  panel->createList(PID_CTRLS_NODES_LIST, "Nodes", names, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_RANDOM_SWITCH_NODE_NAME, "name", defaultName);
  panel->createEditFloat(PID_CTRLS_RANDOM_SWITCH_NODE_WEIGHT, "weight", defaultWeight);
  panel->createEditInt(PID_CTRLS_RANDOM_SWITCH_NODE_MAX_REPEAT, "maxrepeat", defaultMaxRepeat);
}

void AnimTreePlugin::randomSwitchSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data)
{
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  if (listName.empty())
    return;

  const SimpleString fieldName = panel->getText(PID_CTRLS_RANDOM_SWITCH_NODE_NAME);
  DataBlock *weights = settings->getBlockByName("weight");
  DataBlock *maxrepeats = settings->getBlockByName("maxrepeat");

  if (weights && listName != fieldName)
  {
    weights->changeParamName(selectedIdx, fieldName.c_str());
    data.childs[selectedIdx] = find_ctrl_child_idx_by_name(panel, data, controllersData, blendNodesData, fieldName.c_str());
  }
  if (maxrepeats && listName != fieldName)
  {
    int listNameId = maxrepeats->getNameId(listName.c_str());
    for (int i = 0; i < maxrepeats->paramCount(); ++i)
      if (maxrepeats->getParamNameId(i) == listNameId)
        maxrepeats->changeParamName(i, fieldName.c_str());
  }

  const float weightValue = panel->getFloat(PID_CTRLS_RANDOM_SWITCH_NODE_WEIGHT);
  const float maxRepeatValue = panel->getInt(PID_CTRLS_RANDOM_SWITCH_NODE_MAX_REPEAT);

  if (!weights)
  {
    weights = settings->addBlock("weight");
    add_ctrl_child_idx_by_name(panel, data, controllersData, blendNodesData, fieldName.c_str());
  }

  weights->setReal(fieldName.c_str(), weightValue);

  if (maxRepeatValue != 1)
  {
    if (!maxrepeats)
      maxrepeats = settings->addBlock("maxrepeat");

    maxrepeats->setInt(fieldName.c_str(), maxRepeatValue);
  }

  if (maxrepeats && maxrepeats->isEmpty())
    settings->removeBlock("maxrepeat");

  if (listName != fieldName)
    panel->setText(PID_CTRLS_NODES_LIST, fieldName.c_str());
}

void random_switch_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const DataBlock *weights = settings->getBlockByNameEx("weight");
  const DataBlock *maxrepeats = settings->getBlockByNameEx("maxrepeat");
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  panel->setText(PID_CTRLS_RANDOM_SWITCH_NODE_NAME, name.c_str());
  panel->setFloat(PID_CTRLS_RANDOM_SWITCH_NODE_WEIGHT, weights->getReal(name.c_str(), 0.f));
  panel->setInt(PID_CTRLS_RANDOM_SWITCH_NODE_MAX_REPEAT, maxrepeats->getInt(name.c_str(), 1));
}

void random_switch_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  DataBlock *weights = settings->getBlockByName("weight");
  DataBlock *maxrepeats = settings->getBlockByName("maxrepeat");

  if (weights)
    weights->removeParam(removeName.c_str());
  if (maxrepeats)
    maxrepeats->removeParam(removeName.c_str());
}

void AnimTreePlugin::randomSwitchFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings)
{
  const DataBlock *nodes = settings.getBlockByNameEx("weight");
  for (int i = 0; i < nodes->paramCount(); ++i)
    if (nodes->getParamType(i) == DataBlock::TYPE_REAL)
      add_ctrl_child_idx_by_name(panel, data, controllersData, blendNodesData, nodes->getParamName(i));
}

const char *random_switch_get_child_name_by_idx(const DataBlock &settings, int idx)
{
  return settings.getBlockByNameEx("weight")->getParamName(idx);
}

String random_switch_get_child_prefix_name(const DataBlock &settings, int idx)
{
  const DataBlock *weights = settings.getBlockByNameEx("weight");
  float sum = 0.f;
  for (int i = 0; i < weights->paramCount(); ++i)
    sum += weights->getReal(i);

  const float value = safediv(weights->getReal(idx), sum) * 100.f;

  return String(0, "[%d%%] ", value);
}
