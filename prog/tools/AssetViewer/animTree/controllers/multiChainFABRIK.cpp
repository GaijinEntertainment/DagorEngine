// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "multiChainFABRIK.h"
#include "../animTreeUtils.h"
#include "../animTree.h"
#include "../animTreePanelPids.h"
#include "../animParamData.h"

#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

static void fill_chain_fields(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings, const DataBlock &parent_settings,
  dag::Vector<DependentParamData> &params)
{
  const bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  panel->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_EFFECTOR, "effector", settings.getStr("effector", ""), isEditable);
  panel->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_START, "start", settings.getStr("start", ""), isEditable);
  panel->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_END, "end", settings.getStr("end", ""), isEditable);
  const char *defaultWtModulate = parent_settings.getStr("wtModulate", "");
  const bool defaultWtModulateInverse = parent_settings.getBool("wtModulateInverse", false);
  const char *wtModulateValue = settings.getStr("wtModulate", defaultWtModulate);
  panel->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_WT_MODULATE, "wtModulate", wtModulateValue, isEditable);
  const bool wtModulateInverseValue = settings.getBool("wtModulateInverse", defaultWtModulateInverse);
  panel->createCheckBox(PID_CTRLS_MULTI_CHAIN_FABRIK_WT_MODULATE_INVERSE, "wtModulateInverse", wtModulateInverseValue, isEditable);
  params.emplace_back(DependentParamData{
    PID_CTRLS_MULTI_CHAIN_FABRIK_WT_MODULATE, PID_CTRLS_DEPENDENT_WT_MODULATE, strcmp(wtModulateValue, defaultWtModulate) == 0});
  params.emplace_back(DependentParamData{PID_CTRLS_MULTI_CHAIN_FABRIK_WT_MODULATE_INVERSE, PID_CTRLS_DEPENDENT_WT_MODULATE_INVERSE,
    wtModulateInverseValue == defaultWtModulateInverse});
}

static void fill_main_chain_fields(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings)
{
  const bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  panel->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_EFFECTOR, "effector", settings.getStr("effector", ""), isEditable);
  panel->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_START, "start", settings.getStr("start", ""), isEditable);
  panel->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_END, "end", settings.getStr("end", ""), isEditable);
  const DataBlock *startTri = settings.getBlockByNameEx("startTriangle");
  auto *startTriGroup = panel->createGroup(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_GROUP, "startTriangle");
  startTriGroup->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_NODE0, "node0", startTri->getStr("node0", ""), isEditable);
  startTriGroup->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_NODE1, "node1", startTri->getStr("node1", ""), isEditable);
  startTriGroup->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_ADD, "add", startTri->getStr("add", ""), isEditable);
  startTriGroup->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_SUB, "sub", startTri->getStr("sub", ""), isEditable);
  const DataBlock *endTri = settings.getBlockByNameEx("endTriangle");
  auto *endTriGroup = panel->createGroup(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_GROUP, "endTriangle");
  endTriGroup->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_NODE0, "node0", endTri->getStr("node0", ""), isEditable);
  endTriGroup->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_NODE1, "node1", endTri->getStr("node1", ""), isEditable);
  endTriGroup->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_ADD, "add", endTri->getStr("add", ""), isEditable);
  endTriGroup->createEditBox(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_SUB, "sub", endTri->getStr("sub", ""), isEditable);
}

static void update_chain_fields(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings,
  const DataBlock &parent_settings)
{
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_EFFECTOR, settings.getStr("effector", ""));
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_START, settings.getStr("start", ""));
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_END, settings.getStr("end", ""));
  const char *defaultWtModulate = parent_settings.getStr("wtModulate", "");
  const bool defaultWtModulateInverse = parent_settings.getBool("wtModulateInverse", false);
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_WT_MODULATE, settings.getStr("wtModulate", defaultWtModulate));
  panel->setBool(PID_CTRLS_MULTI_CHAIN_FABRIK_WT_MODULATE_INVERSE, settings.getBool("wtModulateInverse", defaultWtModulateInverse));
}

static void update_main_chain_fields(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings)
{
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_EFFECTOR, settings.getStr("effector", ""));
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_START, settings.getStr("start", ""));
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_END, settings.getStr("end", ""));
  const DataBlock *startTri = settings.getBlockByNameEx("startTriangle");
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_NODE0, startTri->getStr("node0", ""));
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_NODE1, startTri->getStr("node1", ""));
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_ADD, startTri->getStr("add", ""));
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_SUB, startTri->getStr("sub", ""));
  const DataBlock *endTri = settings.getBlockByNameEx("endTriangle");
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_NODE0, endTri->getStr("node0", ""));
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_NODE1, endTri->getStr("node1", ""));
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_ADD, endTri->getStr("add", ""));
  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_SUB, endTri->getStr("sub", ""));
}

void multi_chain_fabrik_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "wtModulate");
  add_edit_bool_if_not_exists(params, panel, field_idx, "wtModulateInverse");
}

void multi_chain_fabrik_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "wtModulate");
  remove_param_if_default_bool(params, panel, "wtModulateInverse");
}

void multi_chain_fabrik_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params)
{
  Tab<String> effectorNames;
  const int chainNid = settings->getNameId("chain");
  const int mainChainNid = settings->getNameId("mainChain");
  const DataBlock *defaultSettings = nullptr;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if (child->getBlockNameId() == chainNid || child->getBlockNameId() == mainChainNid)
    {
      effectorNames.emplace_back(child->getStr("effector", ""));
      if (!defaultSettings)
        defaultSettings = child;
    }
  }

  const bool isEditable = !effectorNames.empty();
  const bool isDefaultMainChain = defaultSettings ? defaultSettings->getNameId() == mainChainNid : false;
  const bool isBlockTypeEditable = isEditable && (!settings->blockExists("mainChain") || isDefaultMainChain);
  Tab<String> blockTypeNames{String("chain"), String("mainChain")};
  panel->createList(PID_CTRLS_NODES_LIST, "Nodes", effectorNames, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add chain");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createCombo(PID_CTRLS_MULTI_CHAIN_FABRIK_BLOCK_TYPE_COMBO_SELECT, "Block type", blockTypeNames, isDefaultMainChain ? 1 : 0,
    isBlockTypeEditable);

  if (!defaultSettings)
    defaultSettings = &DataBlock::emptyBlock;

  if (isDefaultMainChain)
    fill_main_chain_fields(panel, *defaultSettings);
  else
    fill_chain_fields(panel, *defaultSettings, *settings, params);
}

void multi_chain_fabrik_change_block_type(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params)
{
  const SimpleString selectedType = panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_BLOCK_TYPE_COMBO_SELECT);
  const SimpleString selectedName = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *selectedBlock = &DataBlock::emptyBlock;
  const int chainNid = settings->getNameId("chain");
  const int mainChainNid = settings->getNameId("mainChain");
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if ((child->getBlockNameId() == chainNid || child->getBlockNameId() == mainChainNid) &&
        selectedName == child->getStr("effector", nullptr))
    {
      selectedBlock = child;
      break;
    }
  }

  params.clear();
  if (selectedType == "mainChain")
    fill_main_chain_fields(panel, *selectedBlock);
  else
    fill_chain_fields(panel, *selectedBlock, *settings, params);
}

void multi_chain_fabrik_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (listName.empty())
    return;

  const int chainNid = settings->getNameId("chain");
  const int mainChainNid = settings->getNameId("mainChain");
  DataBlock *selectedBlock = nullptr;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    DataBlock *child = settings->getBlock(i);
    if ((child->getBlockNameId() == chainNid || child->getBlockNameId() == mainChainNid) &&
        listName == child->getStr("effector", nullptr))
      selectedBlock = child;
  }

  const SimpleString effectorValue = panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_EFFECTOR);
  const SimpleString startValue = panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_START);
  const SimpleString endValue = panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_END);
  const SimpleString comboValue = panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_BLOCK_TYPE_COMBO_SELECT);
  const bool isMainChainSelected = comboValue == "mainChain";
  if (!selectedBlock)
    selectedBlock = settings->addNewBlock(isMainChainSelected ? "mainChain" : "chain");

  bool wasMainChain = selectedBlock->getBlockNameId() == mainChainNid;

  for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
    selectedBlock->removeParam(i);
  for (int i = selectedBlock->blockCount() - 1; i >= 0; --i)
    selectedBlock->removeBlock(i);

  selectedBlock->setStr("effector", effectorValue.c_str());
  selectedBlock->setStr("start", startValue.c_str());
  selectedBlock->setStr("end", endValue.c_str());

  if (isMainChainSelected)
  {
    DataBlock *startTri = selectedBlock->addNewBlock("startTriangle");
    startTri->setStr("node0", panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_NODE0));
    startTri->setStr("node1", panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_NODE1));
    startTri->setStr("add", panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_ADD));
    startTri->setStr("sub", panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_START_TRI_SUB));
    DataBlock *endTri = selectedBlock->addNewBlock("endTriangle");
    endTri->setStr("node0", panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_NODE0));
    endTri->setStr("node1", panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_NODE1));
    endTri->setStr("add", panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_ADD));
    endTri->setStr("sub", panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_SUB));
  }
  else
  {
    const SimpleString wtModulateValue = panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_WT_MODULATE);
    const bool wtModulateInverseValue = panel->getBool(PID_CTRLS_MULTI_CHAIN_FABRIK_WT_MODULATE_INVERSE);
    if (wtModulateValue != settings->getStr("wtModulate", nullptr))
      selectedBlock->setStr("wtModulate", wtModulateValue.c_str());
    if (wtModulateInverseValue != settings->getBool("wtModulateInverse", false))
      selectedBlock->setBool("wtModulateInverse", wtModulateInverseValue);
  }

  if (listName != effectorValue || wasMainChain != isMainChainSelected)
    panel->setText(PID_CTRLS_NODES_LIST, effectorValue.c_str());

  if (wasMainChain != isMainChainSelected)
    selectedBlock->changeBlockName(isMainChainSelected ? "mainChain" : "chain");
}

void multi_chain_fabrik_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params, dag::ConstSpan<AnimParamData> base_params)
{
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *selectedBlock = &DataBlock::emptyBlock;
  const int chainNid = settings->getNameId("chain");
  const int mainChainNid = settings->getNameId("mainChain");
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if ((child->getBlockNameId() == chainNid || child->getBlockNameId() == mainChainNid) && name == child->getStr("effector", nullptr))
      selectedBlock = child;
  }

  const SimpleString comboValue = panel->getText(PID_CTRLS_MULTI_CHAIN_FABRIK_BLOCK_TYPE_COMBO_SELECT);
  const bool wasMainChain = comboValue == "mainChain";
  bool isMainChainSelected = selectedBlock != &DataBlock::emptyBlock && selectedBlock->getBlockNameId() == mainChainNid;
  const bool isBlockTypeEditable = !settings->blockExists("mainChain") || isMainChainSelected;

  panel->setText(PID_CTRLS_MULTI_CHAIN_FABRIK_BLOCK_TYPE_COMBO_SELECT, isMainChainSelected ? "mainChain" : "chain");
  panel->setEnabledById(PID_CTRLS_MULTI_CHAIN_FABRIK_BLOCK_TYPE_COMBO_SELECT, isBlockTypeEditable);
  if (isMainChainSelected != wasMainChain)
  {
    remove_fields(panel, PID_CTRLS_MULTI_CHAIN_FABRIK_EFFECTOR, PID_CTRLS_SETTINGS_SAVE);
    PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
    params.clear();
    if (isMainChainSelected)
    {
      fill_main_chain_fields(group, *selectedBlock);
      group->moveById(PID_CTRLS_SETTINGS_SAVE, PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_SUB, /*after*/ true);
    }
    else
    {
      fill_chain_fields(group, *selectedBlock, *settings, params);
      group->moveById(PID_CTRLS_SETTINGS_SAVE, PID_CTRLS_MULTI_CHAIN_FABRIK_WT_MODULATE_INVERSE, /*after*/ true);
      for (DependentParamData &param : params)
      {
        if (param.dependentParamPid == PID_CTRLS_DEPENDENT_WT_MODULATE)
          update_dependent_param_value_by_name(panel, base_params, param, "wtModulate");
        else if (param.dependentParamPid == PID_CTRLS_DEPENDENT_WT_MODULATE_INVERSE)
          update_dependent_param_value_by_name(panel, base_params, param, "wtModulateInverse");
      }
    }
  }
  else
  {
    if (isMainChainSelected)
      update_main_chain_fields(panel, *selectedBlock);
    else
    {
      const char *defaultWtModulate = settings->getStr("wtModulate", "");
      const bool defaultWtModulateInverse = settings->getBool("wtModulateInverse", false);
      update_chain_fields(panel, *selectedBlock, *settings);
      const char *wtModulateValue = selectedBlock->getStr("wtModulate", defaultWtModulate);
      const bool wtModulateInverseValue = selectedBlock->getBool("wtModulateInverse", defaultWtModulateInverse);
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

    const bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
    for (int i = PID_CTRLS_MULTI_CHAIN_FABRIK_EFFECTOR; i <= PID_CTRLS_MULTI_CHAIN_FABRIK_END_TRI_SUB; ++i)
      panel->setEnabledById(i, isEditable);
  }
}

void multi_chain_fabrik_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  const int chainNid = settings->getNameId("chain");
  const int mainChainNid = settings->getNameId("mainChain");
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if ((child->getBlockNameId() == chainNid || child->getBlockNameId() == mainChainNid) &&
        removeName == child->getStr("effector", nullptr))
      settings->removeBlock(i);
  }
}

void AnimTreePlugin::changeMultiChainFABRIKBlockType(PropPanel::ContainerPropertyControl *panel)
{
  remove_fields(panel, PID_CTRLS_MULTI_CHAIN_FABRIK_EFFECTOR, PID_CTRLS_ACTION_FIELD_SIZE);
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  TLeafHandle parent = tree->getParentLeaf(leaf);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath);
  if (DataBlock *ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent))
    if (DataBlock *settings = find_block_by_name(ctrlsProps, tree->getCaption(leaf)))
      multi_chain_fabrik_change_block_type(group, settings, ctrlDependentParams);
  group->createButton(PID_CTRLS_SETTINGS_SAVE, "Save");
}
