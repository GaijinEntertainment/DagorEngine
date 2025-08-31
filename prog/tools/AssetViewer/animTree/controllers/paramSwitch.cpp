// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "paramSwitch.h"
#include "../animTreeUtils.h"
#include "../animTree.h"
#include "../animTreePanelPids.h"
#include "../animStates/animStatesType.h"

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
  // Find child block based on idx, becuse names can duplicate
  int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  const SimpleString nodeName = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *nodes = settings->getBlockByNameEx("nodes");
  const DataBlock *selectedNode = &DataBlock::emptyBlock;
  int selectedBlock = get_child_block_idx_by_list_idx(nodes, selectedIdx);
  if (selectedBlock >= 0)
  {
    selectedNode = nodes->getBlock(selectedBlock);
    G_ASSERTF(nodeName == selectedNode->getBlockName(), "Incorrect selected block idx in blk, expected <%s> but take <%s> from blk",
      nodeName, selectedNode->getBlockName());
  }

  panel->setText(PID_CTRLS_PARAM_SWITCH_NODE_ENUM_ITEM, nodeName.c_str());
  panel->setText(PID_CTRLS_PARAM_SWITCH_NODE_NAME, selectedNode->getStr("name", ""));
  panel->setBool(PID_CTRLS_PARAM_SWITCH_NODE_OPTIONAL, selectedNode->getBool("optional", false));
  panel->setFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_FROM, selectedNode->getReal("rangeFrom", 0.f));
  panel->setFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_TO, selectedNode->getReal("rangeTo", 0.f));
  bool isEditable = selectedIdx >= 0;
  for (int i = PID_CTRLS_PARAM_SWITCH_NODE_ENUM_ITEM; i <= PID_CTRLS_PARAM_SWITCH_NODE_RANGE_TO; ++i)
    panel->setEnabledById(i, isEditable);
}

void param_switch_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  const int selectedIdx = panel->getInt(PID_CTRLS_NODES_LIST);
  DataBlock *nodes = settings->getBlockByName("nodes");
  if (!nodes)
    return;

  const int removeIdx = get_child_block_idx_by_list_idx(nodes, selectedIdx);
  if (removeIdx == -1)
  {
    logerr("Can't find block with idx from list <%d> and name <%s>", selectedIdx, removeName.c_str());
    return;
  }
  G_ASSERTF(removeName == nodes->getBlock(removeIdx)->getBlockName(), "Remove wrong child block in blk: %s, expected from list: %s",
    nodes->getBlock(removeIdx)->getBlockName(), removeName);
  nodes->removeBlock(removeIdx);
}

static String find_node_name_from_enum(const DataBlock &enum_props, const char *controller_name, int idx)
{
  String nodeName;
  const DataBlock *enumItemProps = enum_props.getBlock(idx);
  const char *enumItemName = enumItemProps->getBlockName();
  nodeName = enumItemProps->getStr(controller_name, "");
  if (nodeName.empty())
  {
    String parentEnumBlockName(enumItemProps->getStr("_use", ""));
    // if recursionCount will become more than then blockCount, so we are in deadlock
    int recursionCount = 0;
    while (!parentEnumBlockName.empty() && recursionCount < enum_props.blockCount())
    {
      recursionCount++;
      if (const DataBlock *parentBlock = enum_props.getBlockByName(parentEnumBlockName.c_str()))
      {
        nodeName = parentBlock->getStr(controller_name, "");
        parentEnumBlockName = nodeName.empty() ? parentBlock->getStr("_use", "") : "";
      }
      else
        parentEnumBlockName = "";
    }
    if (recursionCount == enum_props.blockCount())
      logerr("Enum item <%s> has cycle in _use param, while search for controller <%s>", enumItemName, controller_name);
  }
  return nodeName;
}

void AnimTreePlugin::paramSwitchFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings,
  bool find_enum_gen_parent)
{
  const DataBlock *nodes = settings.getBlockByNameEx("nodes");
  if (nodes->paramExists("enum_gen"))
  {
    const char *nameTemplate = nodes->getStr("name_template", nullptr);
    if (nameTemplate)
    {
      PropPanel::ContainerPropertyControl *tree = getPluginPanel()->getById(PID_ANIM_STATES_TREE)->getContainer();
      AnimStatesData *initAnimStateData = eastl::find_if(statesData.begin(), statesData.end(),
        [](const AnimStatesData &data) { return data.type == AnimStatesType::INIT_ANIM_STATE; });
      String fullPath;
      DataBlock props = getPropsAnimStates(getPluginPanel(), *initAnimStateData, fullPath);
      if (fullPath.empty())
      {
        logerr("Can't find enum props for find enum_gen paramSwitch childs");
        // Place not found child for trigger error when validate dependent nodes
        data.childs.emplace_back(AnimCtrlData::NOT_FOUND_CHILD);
        return;
      }

      const DataBlock *enumRootProps = props.getBlockByNameEx("initAnimState")->getBlockByNameEx("enum");
      const char *enumGen = nodes->getStr("enum_gen");
      const DataBlock *enumProps = enumRootProps->getBlockByName(enumGen);
      if (find_enum_gen_parent)
      {
        AnimStatesData *enumData = eastl::find_if(statesData.begin(), statesData.end(), [tree, enumGen](const AnimStatesData &data) {
          return data.type == AnimStatesType::ENUM && tree->getCaption(data.handle) == enumGen;
        });
        if (enumData != statesData.end())
          enumData->childs.emplace_back(data.id);
        else
          logerr("Can't find enum <%s> from enum_gen declared in paramSwitch controller <%s>", enumGen, settings.getStr("name", ""));
      }

      const char *controllerName = settings.getStr("name");
      for (int i = 0; i < enumProps->blockCount(); ++i)
      {
        String nodeName;
        const DataBlock *enumItemProps = enumProps->getBlock(i);
        const char *enumItemName = enumItemProps->getBlockName();
        if (strcmp(nameTemplate, "*") == 0)
        {
          nodeName = find_node_name_from_enum(*enumProps, controllerName, i);
          if (nodeName.empty())
          {
            logerr("Can't find value in enum item <%s> for controller <%s>", enumItemName, controllerName);
            data.childs.emplace_back(AnimCtrlData::NOT_FOUND_CHILD);
            continue;
          }
        }
        else
        {
          nodeName = nameTemplate;
          nodeName.replace("$1", enumItemName);
        }
        add_ctrl_child_idx_by_name(panel, data, controllersData, blendNodesData, nodeName);
        check_ctrl_child_idx(data.childs[i], settings.getStr("name"), nodeName);
      }
    }
  }
  else
  {
    for (int i = 0; i < nodes->blockCount(); ++i)
    {
      const DataBlock *childBlock = nodes->getBlock(i);
      const char *childName = childBlock->getStr("name", nullptr);
      if (childName)
      {
        add_ctrl_child_idx_by_name(panel, data, controllersData, blendNodesData, childName);
        check_ctrl_child_idx(data.childs[i], settings.getStr("name"), childName, childBlock->getBool("optional", false));
      }
    }
  }
}

const char *param_switch_get_child_name_by_idx(const DataBlock &settings, int idx)
{
  const DataBlock *nodes = settings.getBlockByNameEx("nodes");
  return nodes->getBlock(idx)->getStr("name");
}

String param_switch_get_enum_gen_child_name_by_idx(const DataBlock &enum_props, const char *name, int idx)
{
  String nodeName = find_node_name_from_enum(enum_props, name, idx);
  if (nodeName.empty())
    logerr("Can't find value in enum item <%s> for controller <%s>", enum_props.getBlock(idx)->getBlockName(), name);
  return nodeName;
}

bool param_switch_get_child_is_optional_by_idx(const DataBlock &settings, int idx)
{
  const DataBlock *nodes = settings.getBlockByNameEx("nodes");
  if (nodes->paramExists("enum_gen"))
    return false;

  return nodes->getBlock(idx)->getBool("optional", false);
}

String param_switch_get_child_prefix_name(const DataBlock &settings, int idx, const DataBlock &enum_root_props)
{
  const DataBlock *nodes = settings.getBlockByNameEx("nodes");
  if (nodes->paramExists("enum_gen"))
    nodes = enum_root_props.getBlockByName(nodes->getStr("enum_gen"));

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
  if (!defBlock)
    defBlock = &DataBlock::emptyBlock;

  bool isEditable = !nodeNames.empty();
  panel->createList(PID_CTRLS_NODES_LIST, "Nodes", nodeNames, defBlockIdx);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_PARAM_SWITCH_NODE_ENUM_ITEM, "Enum item",
    defBlock != &DataBlock::emptyBlock ? defBlock->getBlockName() : "", isEditable);
  panel->createEditBox(PID_CTRLS_PARAM_SWITCH_NODE_NAME, "name", defBlock->getStr("name", ""), isEditable);
  panel->createCheckBox(PID_CTRLS_PARAM_SWITCH_NODE_OPTIONAL, "optional", defBlock->getBool("optional", false), isEditable);
  panel->createEditFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_FROM, "rangeFrom", defBlock->getReal("rangeFrom", 0.f), /*prec*/ 2,
    isEditable);
  panel->createEditFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_TO, "rangeTo", defBlock->getReal("rangeTo", 0.f), /*prec*/ 2, isEditable);
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
  AnimStatesData *initAnimStateData = eastl::find_if(statesData.begin(), statesData.end(),
    [](const AnimStatesData &data) { return data.type == AnimStatesType::INIT_ANIM_STATE; });
  String fullPath;
  DataBlock props = getPropsAnimStates(getPluginPanel(), *initAnimStateData, fullPath);
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
    const SimpleString enumGenSelect = panel->getText(PID_CTRLS_PARAM_SWITCH_ENUM_GEN_COMBO_SELECT);
    const SimpleString nameTemplate = panel->getText(PID_CTRLS_PARAM_SWITCH_NAME_TEMPLATE);
    // if enumGenField not found then it can be enum list and we want remove all this blocks
    if (!nodes->paramExists(enumGenField))
      for (int i = nodes->blockCount() - 1; i >= 0; --i)
        nodes->removeBlock(i);
    else
    {
      PropPanel::ContainerPropertyControl *tree = getPluginPanel()->getById(PID_ANIM_STATES_TREE)->getContainer();
      const char *oldEnumGenValue = nodes->getStr(enumGenField);
      if (enumGenSelect != oldEnumGenValue)
      {
        AnimStatesData *enumData =
          eastl::find_if(statesData.begin(), statesData.end(), [tree, oldEnumGenValue](const AnimStatesData &data) {
            return data.type == AnimStatesType::ENUM && tree->getCaption(data.handle) == oldEnumGenValue;
          });
        if (enumData != statesData.end())
          enumData->childs.erase(eastl::remove(enumData->childs.begin(), enumData->childs.end(), data.id), enumData->childs.end());
        else
          logerr("Can't find enum <%s> from enum_gen declared in paramSwitch controller <%s>", oldEnumGenValue,
            settings->getStr("name", ""));
      }
    }
    nodes->setStr(enumGenField, enumGenSelect.c_str());
    nodes->setStr(nameTemplateField, nameTemplate.c_str());
  }
  else
  {
    int selectedBlock = get_child_block_idx_by_list_idx(nodes, selectedIdx);
    const SimpleString enumNameList = panel->getText(PID_CTRLS_NODES_LIST);
    const SimpleString enumItemName = panel->getText(PID_CTRLS_PARAM_SWITCH_NODE_ENUM_ITEM);
    PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_STATES_TREE)->getContainer();
    AnimStatesData *initAnimStateData = find_data_by_type(statesData, AnimStatesType::INIT_ANIM_STATE);
    DataBlock props;
    if (initAnimStateData && !initAnimStateData->fileName.empty())
    {
      String fullPath;
      props = getPropsAnimStates(getPluginPanel(), *initAnimStateData, fullPath);
      if (fullPath.empty())
        return;
    }

    const SimpleString nodeName = panel->getText(PID_CTRLS_PARAM_SWITCH_NODE_NAME);
    const float rangeFrom = panel->getFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_FROM);
    const float rangeTo = panel->getFloat(PID_CTRLS_PARAM_SWITCH_NODE_RANGE_TO);
    const bool optional = panel->getBool(PID_CTRLS_PARAM_SWITCH_NODE_OPTIONAL);
    if (is_equal_float(rangeFrom, 0.f) && is_equal_float(rangeTo, 0.f) && !is_enum_item_name_exist(props, enumItemName))
    {
      logerr("Can't save settings, <%s> enum item not found in props or use rangeFrom or rangeTo params!", enumItemName.c_str());
      return;
    }

    if (nodes->paramExists(enumGenField))
      nodes->removeParam(enumGenField);
    if (nodes->paramExists(nameTemplateField))
      nodes->removeParam(nameTemplateField);

    DataBlock *enumItem = selectedBlock >= 0 ? nodes->getBlock(selectedBlock) : nullptr;
    if (!enumItem)
      enumItem = nodes->addNewBlock(enumItemName.c_str());
    if (nodeName != enumItem->getStr("name", "") || data.childs[selectedIdx] == AnimCtrlData::NOT_FOUND_CHILD)
      data.childs[selectedIdx] = find_child_idx_by_name(panel, data.handle, controllersData, blendNodesData, nodeName.c_str());

    check_ctrl_child_idx(data.childs[selectedIdx], settings->getStr("name"), nodeName, optional);

    if (enumNameList != enumItemName)
    {
      enumItem->changeBlockName(enumItemName.c_str());
      panel->setText(PID_CTRLS_NODES_LIST, enumItemName.c_str());
    }

    enumItem->setStr("name", nodeName.c_str());
    if (!is_equal_float(rangeFrom, 0.f) || !is_equal_float(rangeTo, 0.f))
    {
      enumItem->setReal("rangeFrom", rangeFrom);
      enumItem->setReal("rangeTo", rangeTo);
    }
    else
    {
      enumItem->removeParam("rangeFrom");
      enumItem->removeParam("rangeTo");
    }
    if (optional)
      enumItem->setBool("optional", optional);
    else
      enumItem->removeParam("optional");
  }
}

void param_switch_update_child_name(DataBlock &settings, const char *name, const String &old_name)
{
  DataBlock *nodes = settings.getBlockByName("nodes");
  for (int i = 0; i < nodes->blockCount(); ++i)
  {
    DataBlock *child = nodes->getBlock(i);
    const char *childName = child->getStr("name", nullptr);
    String writeName;
    if (childName && get_updated_child_name(name, old_name, childName, writeName))
      child->setStr("name", writeName.c_str());
  }
}

void param_switch_update_enum_gen_child_name(DataBlock &enum_props, const char *controller_name, const char *name,
  const String &old_name, dag::Vector<int> &dependent_items)
{
  int controllerNid = enum_props.getNameId(controller_name);
  for (int i = 0; i < enum_props.blockCount(); ++i)
  {
    DataBlock *enumItemProps = enum_props.getBlock(i);
    for (int j = 0; j < enumItemProps->paramCount(); ++j)
      if (enumItemProps->getParamNameId(j) == controllerNid && enumItemProps->getStr(j) == old_name)
      {
        enumItemProps->setStr(j, name);
        dependent_items.emplace_back(i);
        break;
      }
  }
}
