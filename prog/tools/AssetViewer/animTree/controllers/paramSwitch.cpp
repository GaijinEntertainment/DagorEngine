// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "paramSwitch.h"
#include "../animTreeUtils.h"
#include "../animTree.h"
#include "../animTreePanelPids.h"

static const bool DEFAULT_SPLIT_CHANS = true;
static const float DEFAULT_MORPH_TIME = 0.15f;

void param_switch_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "varname");
  add_edit_float_if_not_exists(params, panel, field_idx, "morphTime", DEFAULT_MORPH_TIME);
  add_edit_bool_if_not_exists(params, panel, field_idx, "splitChans", DEFAULT_SPLIT_CHANS);
  add_edit_box_if_not_exists(params, panel, field_idx, "accept_name_mask_re");
  add_edit_box_if_not_exists(params, panel, field_idx, "decline_name_mask_re");
}

void param_switch_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_float(params, panel, "morphTime", DEFAULT_MORPH_TIME);
  remove_param_if_default_bool(params, panel, "splitChans", DEFAULT_SPLIT_CHANS);
  remove_param_if_default_str(params, panel, "accept_name_mask_re");
  remove_param_if_default_str(params, panel, "decline_name_mask_re");
}

void param_switch_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const SimpleString nodeName = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *nodes = settings->getBlockByNameEx("nodes");
  const DataBlock *selectedNode = nodes->getBlockByNameEx(nodeName.c_str());
  panel->setText(PID_CTRLS_PARAM_SWITCH_NODE_ENUM_ITEM, nodeName.c_str());
  panel->setText(PID_CTRLS_PARAM_SWITCH_NODE_NAME, selectedNode->getStr("name", ""));
  panel->setFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_FROM, selectedNode->getReal("rangeFrom", 0.f));
  panel->setFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_TO, selectedNode->getReal("rangeTo", 0.f));
}

void param_switch_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  DataBlock *nodesBlk = settings->getBlockByName("nodes");
  if (!nodesBlk)
    return;
  nodesBlk->removeBlock(removeName.c_str());
}

void AnimTreePlugin::paramSwitchFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings)
{
  const DataBlock *nodes = settings.getBlockByNameEx("nodes");
  for (int i = 0; i < nodes->blockCount(); ++i)
  {
    const char *childName = nodes->getBlock(i)->getStr("name", nullptr);
    if (childName)
      add_ctrl_child_idx_by_name(panel, data, controllersData, blendNodesData, childName);
  }
}

const char *param_switch_get_child_name_by_idx(const DataBlock &settings, int idx)
{
  const DataBlock *nodes = settings.getBlockByNameEx("nodes");
  return nodes->getBlock(idx)->getStr("name");
}

String param_switch_get_child_prefix_name(const DataBlock &settings, int idx)
{
  const DataBlock *nodes = settings.getBlockByNameEx("nodes");
  return String(0, "[%s] ", nodes->getBlock(idx)->getBlockName());
}

static void fill_param_switch_enum_list(PropPanel::ContainerPropertyControl *panel, const DataBlock *nodes)
{
  Tab<String> nodeNames;
  const int defBlockIdx = 0;
  const DataBlock *defBlock = nullptr;
  for (int i = 0; i < nodes->blockCount(); ++i)
  {
    const DataBlock *settings = nodes->getBlock(i);
    if (settings->paramExists("name"))
    {
      nodeNames.emplace_back(settings->getBlockName());
      if (!defBlock)
        defBlock = settings;
    }
  }
  panel->createList(PID_CTRLS_NODES_LIST, "Nodes", nodeNames, defBlockIdx);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_PARAM_SWITCH_NODE_ENUM_ITEM, "Enum item", defBlock ? defBlock->getBlockName() : "");
  panel->createEditBox(PID_CTRLS_PARAM_SWITCH_NODE_NAME, "name", defBlock ? defBlock->getStr("name", "") : "");
  panel->createEditFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_FROM, "rangeFrom", defBlock ? defBlock->getReal("rangeFrom", 0.f) : 0.f);
  panel->createEditFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_TO, "rangeTo", defBlock ? defBlock->getReal("rangeTo", 0.f) : 0.f);
}

void AnimTreePlugin::paramSwitchFillBlockSettings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  const DataBlock *nodes = settings->getBlockByNameEx("nodes");
  const char *enumGenField = "enum_gen";
  const bool isEnumGenType = nodes->paramExists(enumGenField);
  panel->createSeparator(PID_CTRLS_SEPARATOR);
  panel->createCombo(PID_CTRLS_PARAM_SWITCH_TYPE_COMBO_SELECT, "Param swtich type", param_switch_type,
    isEnumGenType ? PARAM_SWITCH_TYPE_ENUM_GEN : PARAM_SWITCH_TYPE_ENUM_LIST);
  if (isEnumGenType)
    fillParamSwitchEnumGen(panel, nodes);
  else
    fill_param_switch_enum_list(panel, nodes);
}

void AnimTreePlugin::fillParamSwitchEnumGen(PropPanel::ContainerPropertyControl *panel, const DataBlock *nodes)
{
  const char *enumGenField = "enum_gen";
  const char *nameTemplateField = "name_template";
  if (!enumsRootLeaf)
  {
    logerr("Enums root leaf not found. Please add enums to the anim states tree first");
    return;
  }
  PropPanel::ContainerPropertyControl *tree = getPluginPanel()->getById(PID_ANIM_STATES_TREE)->getContainer();
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, tree->getRootLeaf(), fullPath);
  if (fullPath.empty())
    return;

  const DataBlock *initAnimState = props.getBlockByNameEx("initAnimState");
  const DataBlock *enumBlock = initAnimState->getBlockByNameEx("enum");
  Tab<String> enumNames;
  for (int i = 0; i < enumBlock->blockCount(); ++i)
    enumNames.emplace_back(enumBlock->getBlock(i)->getBlockName());
  panel->createCombo(PID_CTRLS_PARAM_SWITCH_ENUM_GEN_COMBO_SELECT, enumGenField, enumNames, nodes->getStr(enumGenField, ""));
  panel->createEditBox(PID_CTRLS_PARAM_SWITCH_NAME_TEMPLATE, nameTemplateField, nodes->getStr(nameTemplateField, ""));
}

void AnimTreePlugin::changeParamSwitchType(PropPanel::ContainerPropertyControl *panel)
{
  remove_fields(panel, PID_CTRLS_PARAM_SWITCH_ENUM_GEN_COMBO_SELECT, PID_CTRLS_ACTION_FIELD_SIZE, false);
  ParamSwitchType type = static_cast<ParamSwitchType>(panel->getInt(PID_CTRLS_PARAM_SWITCH_TYPE_COMBO_SELECT));
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  TLeafHandle parent = tree->getParentLeaf(leaf);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, parent, fullPath);
  if (DataBlock *ctrlsProps = get_props_from_include_leaf_ctrl_node(controllersData, includePaths, *curAsset, props, tree, parent))
    if (DataBlock *settings = find_block_by_name(ctrlsProps, tree->getCaption(leaf)))
    {
      const DataBlock *nodes = settings->getBlockByNameEx("nodes");
      if (type == PARAM_SWITCH_TYPE_ENUM_GEN)
        fillParamSwitchEnumGen(group, nodes);
      else
        fill_param_switch_enum_list(group, nodes);
    }
  group->createButton(PID_CTRLS_SETTINGS_SAVE, "Save");
}

bool is_enum_item_name_exist(const DataBlock &props, const SimpleString &name)
{
  const DataBlock *initAnimState = props.getBlockByNameEx("initAnimState");
  const DataBlock *enums = initAnimState->getBlockByNameEx("enum");
  for (int i = 0; i < enums->blockCount(); ++i)
  {
    const DataBlock *enumBlk = enums->getBlock(i);
    for (int j = 0; j < enumBlk->blockCount(); ++j)
      if (name == enumBlk->getBlock(j)->getBlockName())
        return true;
  }
  return false;
}

void AnimTreePlugin::paramSwitchSaveBlockSettings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings, AnimCtrlData &data)
{
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  if (selectedIdx == -1)
    return;

  ParamSwitchType type = static_cast<ParamSwitchType>(panel->getInt(PID_CTRLS_PARAM_SWITCH_TYPE_COMBO_SELECT));
  DataBlock *nodes = settings->getBlockByName("nodes");
  if (!nodes)
    nodes = settings->addBlock("nodes");

  const char *enumGenField = "enum_gen";
  const char *nameTemplateField = "name_template";
  if (type == PARAM_SWITCH_TYPE_ENUM_GEN)
  {
    // if enumGenField not found then it can be enum list and we want remove all this blocks
    if (!nodes->paramExists(enumGenField))
      for (int i = nodes->blockCount() - 1; i >= 0; --i)
        nodes->removeBlock(i);
    const SimpleString eunmGenSelect = panel->getText(PID_CTRLS_PARAM_SWITCH_ENUM_GEN_COMBO_SELECT);
    const SimpleString nameTemplate = panel->getText(PID_CTRLS_PARAM_SWITCH_NAME_TEMPLATE);
    nodes->setStr(enumGenField, eunmGenSelect.c_str());
    nodes->setStr(nameTemplateField, nameTemplate.c_str());
  }
  else
  {
    const int selectedChild = panel->getInt(PID_CTRLS_NODES_LIST);
    const SimpleString enumNameList = panel->getText(PID_CTRLS_NODES_LIST);
    const SimpleString enumItemName = panel->getText(PID_CTRLS_PARAM_SWITCH_NODE_ENUM_ITEM);
    PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
    String fullPath;
    DataBlock props = get_props_from_include_leaf(includePaths, *curAsset, tree, tree->getRootLeaf(), fullPath);
    if (fullPath.empty())
      return;

    if (!is_enum_item_name_exist(props, enumItemName))
    {
      logerr("Can't save settings, <%s> enum item not found in props", enumItemName.c_str());
      return;
    }

    if (nodes->paramExists(enumGenField))
      nodes->removeParam(enumGenField);
    if (nodes->paramExists(nameTemplateField))
      nodes->removeParam(nameTemplateField);

    const SimpleString nodeName = panel->getText(PID_CTRLS_PARAM_SWITCH_NODE_NAME);
    DataBlock *enumItem = nodes->getBlockByName(enumNameList.c_str());
    if (!enumItem)
    {
      enumItem = nodes->addBlock(enumItemName.c_str());
      add_ctrl_child_idx_by_name(panel, data, controllersData, blendNodesData, nodeName.c_str());
    }
    else if (nodeName != enumItem->getStr("name"))
      data.childs[selectedChild] = find_ctrl_child_idx_by_name(panel, data, controllersData, blendNodesData, nodeName.c_str());

    if (enumNameList != enumItemName)
    {
      enumItem->changeBlockName(enumItemName.c_str());
      panel->setText(PID_CTRLS_NODES_LIST, enumItemName.c_str());
    }

    const float rangeFrom = panel->getFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_FROM);
    const float rangeTo = panel->getFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_TO);
    enumItem->setStr("name", nodeName.c_str());
    if (rangeFrom > 0.0f || rangeTo > 0.0f)
    {
      enumItem->setReal("rangeFrom", rangeFrom);
      enumItem->setReal("rangeTo", rangeTo);
    }
    else
    {
      enumItem->removeParam("rangeFrom");
      enumItem->removeParam("rangeTo");
    }
  }
}
