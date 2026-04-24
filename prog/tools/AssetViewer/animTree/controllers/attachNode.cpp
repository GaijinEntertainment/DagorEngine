// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "effFromAttachment.h"
#include "../animTreeUtils.h"
#include "../animParamData.h"

#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

const Point3 DEFAULT_NODE_SCALE = Point3(1, 1, 1);
const bool DEFAULT_WT_MODULATE_INVERSE = false;

void attach_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_point3_if_not_exists(params, panel, field_idx, "minNodeScale", DEFAULT_NODE_SCALE);
  add_edit_point3_if_not_exists(params, panel, field_idx, "maxNodeScale", DEFAULT_NODE_SCALE);
  add_edit_box_if_not_exists(params, panel, field_idx, "wtModulate");
  add_edit_bool_if_not_exists(params, panel, field_idx, "wtModulateInverse");
}

void attach_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_point3(params, panel, "minNodeScale", DEFAULT_NODE_SCALE);
  remove_param_if_default_point3(params, panel, "maxNodeScale", DEFAULT_NODE_SCALE);
  remove_param_if_default_str(params, panel, "wtModulate");
  remove_param_if_default_bool(params, panel, "wtModulateInverse");
}

void attach_node_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params)
{
  Tab<String> names;
  const char *defaultWtModulate = settings->getStr("wtModulate", "");
  const bool defaultWtModulateInverse = settings->getBool("wtModulateInverse", DEFAULT_WT_MODULATE_INVERSE);

  const int attachNid = settings->getNameId("attach");
  for (int i = 0; i < settings->blockCount(); ++i)
    if (settings->getBlock(i)->getBlockNameId() == attachNid)
      names.emplace_back(settings->getBlock(i)->getStr("node", ""));

  bool isEditable = !names.empty();
  const DataBlock *defaultBlock = settings->getBlockByNameEx("attach");
  panel->createList(PID_CTRLS_NODES_LIST, "Nodes", names, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add node");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_ATTACH_NODE_NODE, "node", defaultBlock->getStr("node", ""), isEditable);
  panel->createEditBox(PID_CTRLS_ATTACH_NODE_VAR, "var", defaultBlock->getStr("var", ""), isEditable);
  const char *wtModulateValue = defaultBlock->getStr("wtModulate", defaultWtModulate);
  panel->createEditBox(PID_CTRLS_ATTACH_NODE_WT_MODULATE, "wtModulate", wtModulateValue, isEditable);
  const bool wtModulateInverseValue = defaultBlock->getBool("wtModulateInverse", defaultWtModulateInverse);
  panel->createCheckBox(PID_CTRLS_ATTACH_NODE_WT_MODULATE_INVERSE, "wtModulateInverse", wtModulateInverseValue, isEditable);

  params.emplace_back(DependentParamData{
    PID_CTRLS_ATTACH_NODE_WT_MODULATE, PID_CTRLS_DEPENDENT_WT_MODULATE, strcmp(wtModulateValue, defaultWtModulate) == 0});
  params.emplace_back(DependentParamData{PID_CTRLS_ATTACH_NODE_WT_MODULATE_INVERSE, PID_CTRLS_DEPENDENT_WT_MODULATE_INVERSE,
    wtModulateInverseValue == defaultWtModulateInverse});
}

void attach_node_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const char *defaultWtModulate = settings->getStr("wtModulate", "");
  const bool defaultWtModulateInverse = settings->getBool("wtModulateInverse", DEFAULT_WT_MODULATE_INVERSE);
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (listName.empty())
    return;

  DataBlock *selectedBlock = nullptr;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    DataBlock *child = settings->getBlock(i);
    if (listName == child->getStr("node", nullptr))
      selectedBlock = child;
  }
  const SimpleString nodeValue = panel->getText(PID_CTRLS_ATTACH_NODE_NODE);
  const SimpleString varValue = panel->getText(PID_CTRLS_ATTACH_NODE_VAR);
  const SimpleString wtModulateValue = panel->getText(PID_CTRLS_ATTACH_NODE_WT_MODULATE);
  const bool wtModulateInverseValue = panel->getBool(PID_CTRLS_ATTACH_NODE_WT_MODULATE_INVERSE);
  if (!selectedBlock)
    selectedBlock = settings->addNewBlock("attach");
  for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
    selectedBlock->removeParam(i);

  selectedBlock->setStr("node", nodeValue.c_str());
  selectedBlock->setStr("var", varValue.c_str());
  if (wtModulateValue != "")
    selectedBlock->setStr("wtModulate", wtModulateValue.c_str());
  if (wtModulateInverseValue != defaultWtModulateInverse)
    selectedBlock->setBool("wtModulateInverse", wtModulateInverseValue);

  if (listName != nodeValue)
    panel->setText(PID_CTRLS_NODES_LIST, nodeValue.c_str());
}

void attach_node_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params, dag::ConstSpan<AnimParamData> base_params)
{
  const char *defaultWtModulate = settings->getStr("wtModulate", "");
  const bool defaultWtModulateInverse = settings->getBool("wtModulateInverse", DEFAULT_WT_MODULATE_INVERSE);
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *selectedBlock = &DataBlock::emptyBlock;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if (name == child->getStr("node", nullptr))
      selectedBlock = child;
  }
  panel->setText(PID_CTRLS_ATTACH_NODE_NODE, name);
  panel->setText(PID_CTRLS_ATTACH_NODE_VAR, selectedBlock->getStr("var", ""));
  const char *wtModulateValue = selectedBlock->getStr("wtModulate", defaultWtModulate);
  panel->setText(PID_CTRLS_ATTACH_NODE_WT_MODULATE, wtModulateValue);
  const bool wtModulateInverseValue = selectedBlock->getBool("wtModulateInverse", defaultWtModulateInverse);
  panel->setBool(PID_CTRLS_ATTACH_NODE_WT_MODULATE_INVERSE, wtModulateInverseValue);
  bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  for (int i = PID_CTRLS_ATTACH_NODE_NODE; i <= PID_CTRLS_ATTACH_NODE_WT_MODULATE_INVERSE; ++i)
    panel->setEnabledById(i, isEditable);

  for (DependentParamData &param : params)
  {
    if (param.dependentParamPid == PID_CTRLS_DEPENDENT_WT_MODULATE)
    {
      param.dependent = strcmp(wtModulateValue, defaultWtModulate) == 0;
      update_dependent_param_value_by_name(panel, base_params, param, "wtModulate");
    }
    else if (param.dependentParamPid == PID_CTRLS_DEPENDENT_WT_MODULATE_INVERSE)
    {
      param.dependent = wtModulateInverseValue == defaultWtModulateInverse;
      update_dependent_param_value_by_name(panel, base_params, param, "wtModulateInverse");
    }
  }
}

void attach_node_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  for (int i = 0; i < settings->blockCount(); ++i)
    if (removeName == settings->getBlock(i)->getStr("node", nullptr))
      settings->removeBlock(i);
}
