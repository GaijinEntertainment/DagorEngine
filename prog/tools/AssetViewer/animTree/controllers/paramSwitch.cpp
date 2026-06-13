// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "paramSwitch.h"
#include "../animTreeUtils.h"
#include "../animTree.h"
#include "../animTreePanelPids.h"
#include "../animStates/animStatesType.h"
#include "../animMorphType.h"
#include <util/dag_lookup.h>

static const bool DEFAULT_SPLIT_CHANS = true;
static const float DEFAULT_MORPH_TIME = 0.15f;

static void check_param_switch_child(BlendNodeType child_type, CtrlType param_switch_type, const char *param_switch_name,
  const char *name)
{
  if (child_type == BlendNodeType::SINGLE && param_switch_type == ctrl_type_paramSwitch)
    logerr("paramSwitch <%s> has single blend node <%s>, but tuned for continuous mode. Use paramSwitchS", param_switch_name, name);
}

static void param_switch_add_child_by_name(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name, const char *param_switch_name)
{
  CtrlChildSearchResult result = find_child_idx_and_icon_by_name(panel, data.handle, controllers, blend_nodes, name);
  data.childs.emplace_back(result.id);
  check_param_switch_child(result.blendNodeType, data.type, param_switch_name, name);
  check_ctrl_child_idx(result.id, param_switch_name, name);
}

void param_switch_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "varname");
  add_edit_float_if_not_exists(params, panel, field_idx, "morphTime", DEFAULT_MORPH_TIME);
  add_combo_if_not_exists(params, panel, field_idx, "morphType", morph_type_names, DEFAULT_MORPH_TYPE);
  add_edit_bool_if_not_exists(params, panel, field_idx, "splitChans", DEFAULT_SPLIT_CHANS);
  add_edit_box_if_not_exists(params, panel, field_idx, "accept_name_mask_re");
  add_edit_box_if_not_exists(params, panel, field_idx, "decline_name_mask_re");
}

void param_switch_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_float(params, panel, "morphTime", DEFAULT_MORPH_TIME);
  remove_param_if_default_str(params, panel, "morphType", DEFAULT_MORPH_TYPE);
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

static String make_node_name_from_template(const char *name_template, const DataBlock &enum_props, const char *controller_name,
  int idx)
{
  if (strcmp(name_template, "*") == 0)
    return find_node_name_from_enum(enum_props, controller_name, idx);
  String nodeName(name_template);
  nodeName.replace("$1", enum_props.getBlock(idx)->getBlockName());
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
      DataBlock props = getPropsAnimStates(getPluginPanel(), *initAnimStateData, fullPath, /*only_includes*/ true);
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
        const char *enumItemName = enumProps->getBlock(i)->getBlockName();
        String nodeName = make_node_name_from_template(nameTemplate, *enumProps, controllerName, i);
        if (nodeName.empty())
        {
          logerr("Can't find value in enum item <%s> for controller <%s>", enumItemName, controllerName);
          data.childs.emplace_back(AnimCtrlData::NOT_FOUND_CHILD);
          continue;
        }
        param_switch_add_child_by_name(panel, data, controllersData, blendNodesData, nodeName, settings.getStr("name"));
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
        param_switch_add_child_by_name(panel, data, controllersData, blendNodesData, childName, settings.getStr("name"));
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
  paramSwitchFillMorphTimes(panel, *settings);
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
  DataBlock props = getPropsAnimStates(getPluginPanel(), *initAnimStateData, fullPath, /*only_includes*/ true);
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
  const int savedMorphIdx = panel->getInt(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_LIST);
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  group->saveState(ctrlsSettingsPanelState);
  remove_fields(panel, PID_CTRLS_PARAM_SWITCH_ENUM_GEN_COMBO_SELECT, PID_CTRLS_ACTION_FIELD_SIZE, false);
  ParamSwitchType type = static_cast<ParamSwitchType>(panel->getInt(PID_CTRLS_PARAM_SWITCH_TYPE_COMBO_SELECT));
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
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

      paramSwitchFillMorphTimes(group, *settings);
      if (savedMorphIdx >= 0)
      {
        group->setInt(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_LIST, savedMorphIdx);
        paramSwitchSetSelectedMorphTimeSettings(panel);
      }
    }
  group->createButton(PID_CTRLS_SETTINGS_SAVE, "Save");
  group->loadState(ctrlsSettingsPanelState);
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
    {
      CtrlChildSearchResult result = find_child_idx_and_icon_by_name(panel, data.handle, controllersData, blendNodesData, nodeName);
      data.childs[selectedIdx] = result.id;
      check_param_switch_child(result.blendNodeType, data.type, settings->getStr("name"), nodeName);
    }

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

  const int selectedMorphIdx = panel->getInt(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_LIST);
  if (selectedMorphIdx >= 0)
  {
    DataBlock *morphTimes = settings->getBlockByName("morphTimes");
    if (morphTimes && selectedMorphIdx < morphTimes->blockCount())
    {
      DataBlock *entry = morphTimes->getBlock(selectedMorphIdx);
      const SimpleString from = panel->getText(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_FROM);
      const SimpleString to = panel->getText(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TO);
      const float morphTimeVal = panel->getFloat(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_VALUE);
      const SimpleString morphTypeVal = panel->getText(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TYPE);
      entry->setStr("from", from);
      entry->setStr("to", to);
      entry->setReal("morphTime", morphTimeVal);
      if (morphTypeVal != DEFAULT_MORPH_TYPE)
        entry->setStr("morphType", morphTypeVal);
      else
        entry->removeParam("morphType");
      panel->setText(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_LIST, String(0, "%s -> %s", from, to));
    }
  }

  if (type == PARAM_SWITCH_TYPE_ENUM_LIST)
    paramSwitchRefreshMorphTimesCombo(panel, settings);
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

static void fill_param_switch_morph_times_panel(PropPanel::ContainerPropertyControl *panel, const Tab<String> &nodeNames,
  const DataBlock &morph_times_blk)
{
  PropPanel::ContainerPropertyControl *group = panel->createGroup(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_GROUP, "Morph times");
  Tab<String> listLabels;
  for (int i = 0; i < morph_times_blk.blockCount(); ++i)
  {
    const DataBlock *entry = morph_times_blk.getBlock(i);
    listLabels.emplace_back(String(0, "%s -> %s", entry->getStr("from", ""), entry->getStr("to", "")));
  }
  const bool hasItems = !listLabels.empty();
  group->createList(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_LIST, "Nodes", listLabels, hasItems ? 0 : -1);
  group->createButton(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_ADD, "Add");
  group->createButton(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_REMOVE, "Remove", hasItems, /*new_line*/ false);
  const DataBlock *firstEntry = hasItems ? morph_times_blk.getBlock(0) : nullptr;
  const char *fromVal = firstEntry ? firstEntry->getStr("from", "") : "";
  const char *toVal = firstEntry ? firstEntry->getStr("to", "") : "";
  const float morphTimeVal = firstEntry ? firstEntry->getReal("morphTime", 0.f) : 0.f;
  const char *morphTypeVal = firstEntry ? firstEntry->getStr("morphType", DEFAULT_MORPH_TYPE) : DEFAULT_MORPH_TYPE;
  group->createCombo(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_FROM, "from", nodeNames, fromVal);
  group->createCombo(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TO, "to", nodeNames, toVal);
  group->createEditFloat(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_VALUE, "morphTime", morphTimeVal, /*prec*/ 2, hasItems);
  group->createCombo(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TYPE, "morphType", morph_type_names, lup(morphTypeVal, morph_type_names, 0),
    hasItems);
  group->setEnabledById(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_FROM, hasItems);
  group->setEnabledById(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TO, hasItems);
}

void validate_param_switch_morph_times_names(const Tab<String> &nodeNames, const DataBlock &morph_times_blk, const char *ctrl_name)
{
  for (int i = 0; i < morph_times_blk.blockCount(); ++i)
  {
    const DataBlock *entry = morph_times_blk.getBlock(i);
    const char *fromVal = entry->getStr("from", "");
    const char *toVal = entry->getStr("to", "");
    if (*fromVal && eastl::find(nodeNames.begin(), nodeNames.end(), fromVal) == nodeNames.end())
      logerr("In paramSwitch controller \"%s\": invalid morphTimes entry #%d from:t=\"%s\" doesn't match any node. "
             "Make sure the node name exists in the controller's node list",
        ctrl_name, i, fromVal);
    if (*toVal && eastl::find(nodeNames.begin(), nodeNames.end(), toVal) == nodeNames.end())
      logerr("In paramSwitch controller \"%s\": invalid morphTimes entry #%d to:t=\"%s\" doesn't match any node. "
             "Make sure the node name exists in the controller's node list",
        ctrl_name, i, toVal);
  }
}

Tab<String> AnimTreePlugin::paramSwitchGetNodeNames(const DataBlock &settings)
{
  Tab<String> nodeNames;
  auto addUnique = [&nodeNames](const char *name) {
    if (eastl::find(nodeNames.begin(), nodeNames.end(), name) == nodeNames.end())
      nodeNames.emplace_back(name);
  };
  const DataBlock &nodes = *settings.getBlockByNameEx("nodes");
  if (nodes.paramExists("enum_gen"))
  {
    AnimStatesData *initAnimStateData = eastl::find_if(statesData.begin(), statesData.end(),
      [](const AnimStatesData &data) { return data.type == AnimStatesType::INIT_ANIM_STATE; });
    if (initAnimStateData == statesData.end())
      return nodeNames;
    String fullPath;
    DataBlock props = getPropsAnimStates(getPluginPanel(), *initAnimStateData, fullPath, /*only_includes*/ true);
    if (fullPath.empty())
      return nodeNames;
    const DataBlock *enumProps =
      props.getBlockByNameEx("initAnimState")->getBlockByNameEx("enum")->getBlockByName(nodes.getStr("enum_gen"));
    if (!enumProps)
      return nodeNames;
    const char *nameTemplate = nodes.getStr("name_template", nullptr);
    if (!nameTemplate)
      return nodeNames;
    const char *controllerName = settings.getStr("name", "");
    for (int i = 0; i < enumProps->blockCount(); ++i)
    {
      String nodeName = make_node_name_from_template(nameTemplate, *enumProps, controllerName, i);
      if (nodeName.empty())
        continue;
      addUnique(nodeName);
    }
  }
  else
  {
    for (int i = 0; i < nodes.blockCount(); ++i)
    {
      const char *childName = nodes.getBlock(i)->getStr("name", nullptr);
      if (childName && *childName)
        addUnique(childName);
    }
  }
  return nodeNames;
}

void AnimTreePlugin::paramSwitchFillMorphTimes(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings)
{
  Tab<String> nodeNames = paramSwitchGetNodeNames(settings);
  validate_param_switch_morph_times_names(nodeNames, *settings.getBlockByNameEx("morphTimes"), settings.getStr("name", ""));
  fill_param_switch_morph_times_panel(panel, nodeNames, *settings.getBlockByNameEx("morphTimes"));
}

void AnimTreePlugin::paramSwitchSetSelectedMorphTimeSettings(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;
  const AnimCtrlData *data = find_data_by_handle(controllersData, leaf);
  if (data == controllersData.end())
    return;
  bool isProcChild = false;
  DataBlock props;
  String fullPath;
  DataBlock *settings = findCtrlSettings(tree, leaf, data->type, props, fullPath, isProcChild);
  if (!settings)
    settings = findCtrlSettings(tree, leaf, data->type, props, fullPath, isProcChild, /*only_includes*/ true);
  if (!settings)
    return;

  const int selectedIdx = panel->getInt(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_LIST);
  const bool isEditable = selectedIdx >= 0;
  for (int pid = PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_REMOVE; pid <= PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TYPE; ++pid)
    panel->setEnabledById(pid, isEditable);
  if (!isEditable)
    return;

  const DataBlock &morphTimesBlk = *settings->getBlockByNameEx("morphTimes");
  if (selectedIdx >= morphTimesBlk.blockCount())
    return;
  const DataBlock *entry = morphTimesBlk.getBlock(selectedIdx);
  panel->setText(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_FROM, entry->getStr("from", ""));
  panel->setText(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TO, entry->getStr("to", ""));
  panel->setFloat(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_VALUE, entry->getReal("morphTime", 0.f));
  panel->setInt(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TYPE, lup(entry->getStr("morphType", DEFAULT_MORPH_TYPE), morph_type_names, 0));
}

void AnimTreePlugin::paramSwitchAddMorphTime(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;
  const AnimCtrlData *data = find_data_by_handle(controllersData, leaf);
  if (data == controllersData.end())
    return;
  bool isProcChild = false;
  DataBlock props;
  String fullPath;
  DataBlock *settings = findCtrlSettings(tree, leaf, data->type, props, fullPath, isProcChild);
  if (!settings)
    return;

  DataBlock *morphTimes = settings->getBlockByName("morphTimes");
  if (!morphTimes)
    morphTimes = settings->addBlock("morphTimes");

  Tab<String> nodeNames = paramSwitchGetNodeNames(*settings);
  const char *defaultName = nodeNames.empty() ? "" : nodeNames[0].c_str();
  DataBlock *newEntry = morphTimes->addNewBlock("item");
  newEntry->setStr("from", defaultName);
  newEntry->setStr("to", defaultName);
  newEntry->setReal("morphTime", 0.f);
  saveProps(props, fullPath);
  const int idx = panel->addString(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_LIST, String(0, "%s -> %s", defaultName, defaultName).c_str());
  panel->setInt(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_LIST, idx);

  for (int pid = PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_REMOVE; pid <= PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TYPE; ++pid)
    panel->setEnabledById(pid, true);
  panel->setText(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_FROM, defaultName);
  panel->setText(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TO, defaultName);
  panel->setFloat(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_VALUE, 0.f);
  panel->setInt(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TYPE, 0);
}

void AnimTreePlugin::paramSwitchRefreshMorphTimesCombo(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings)
{
  if (!settings)
  {
    PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
    TLeafHandle leaf = tree->getSelLeaf();
    if (!leaf)
      return;
    const AnimCtrlData *data = find_data_by_handle(controllersData, leaf);
    if (data == controllersData.end())
      return;
    if (data->type != ctrl_type_paramSwitch && data->type != ctrl_type_paramSwitchS)
      return;
    bool isProcChild = false;
    DataBlock props;
    String fullPath;
    settings = findCtrlSettings(tree, leaf, data->type, props, fullPath, isProcChild);
    if (!settings)
      return;
  }

  Tab<String> nodeNames = paramSwitchGetNodeNames(*settings);
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  if (group->getById(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_FROM))
  {
    const SimpleString fromText = group->getText(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_FROM);
    group->setStrings(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_FROM, nodeNames);
    group->setText(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_FROM, fromText);
  }
  if (group->getById(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TO))
  {
    const SimpleString toText = group->getText(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TO);
    group->setStrings(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TO, nodeNames);
    group->setText(PID_CTRLS_PARAM_SWITCH_MORPH_TIME_TO, toText);
  }
}

void AnimTreePlugin::paramSwitchRemoveMorphTime(PropPanel::ContainerPropertyControl *panel)
{
  const int removeIdx = panel->getInt(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_LIST);
  if (removeIdx < 0)
    return;
  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  TLeafHandle leaf = tree->getSelLeaf();
  if (!leaf)
    return;
  const AnimCtrlData *data = find_data_by_handle(controllersData, leaf);
  if (data == controllersData.end())
    return;

  bool isProcChild = false;
  DataBlock props;
  String fullPath;
  DataBlock *settings = findCtrlSettings(tree, leaf, data->type, props, fullPath, isProcChild);
  if (!settings)
    return;

  DataBlock *morphTimes = settings->getBlockByName("morphTimes");
  if (morphTimes)
  {
    morphTimes->removeBlock(removeIdx);
    if (morphTimes->isEmpty())
      settings->removeBlock("morphTimes");
  }

  panel->removeString(PID_CTRLS_PARAM_SWITCH_MORPH_TIMES_LIST, removeIdx);
  paramSwitchSetSelectedMorphTimeSettings(panel);
  saveProps(props, fullPath);
}
