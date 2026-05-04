// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animateNode.h"
#include "../animTreeUtils.h"
#include "../animParamData.h"
#include "../animTreePanelPids.h"
#include "../animTree.h"
#include "animCtrlData.h"
#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>

static const int DEFAULT_E_KEY = 100;
static const float DEFAULT_P_MUL = 1.f;

void animate_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "bnl");
  add_edit_int_if_not_exists(params, panel, field_idx, "sKey");
  add_edit_int_if_not_exists(params, panel, field_idx, "eKey", DEFAULT_E_KEY);
  add_edit_bool_if_not_exists(params, panel, field_idx, "mandatoryAnim");
}

void animate_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "bnl");
  remove_param_if_default_int(params, panel, "sKey");
  remove_param_if_default_int(params, panel, "eKey", DEFAULT_E_KEY);
  remove_param_if_default_bool(params, panel, "mandatoryAnim");
}

static void clear_anim_block_params(PropPanel::ContainerPropertyControl *group, int first_pid, int last_pid)
{
  for (int pid = first_pid; pid < last_pid; ++pid)
    if (group->getById(pid) != nullptr)
      group->removeById(pid);
}

static void init_anim_block_params(PropPanel::ContainerPropertyControl *group, const DataBlock &settings, int first_pid,
  int add_button_pid, int remove_button_pid, const char *param_name)
{
  const bool isEditable = true;
  const int paramNid = settings.getNameId(param_name);
  int pid = first_pid;
  for (int i = 0; i < settings.paramCount(); ++i)
  {
    if (settings.getParamNameId(i) == paramNid && settings.getParamType(i) == DataBlock::TYPE_STRING)
    {
      group->createEditBox(pid, param_name, settings.getStr(i), isEditable);
      group->moveById(pid, add_button_pid, /*after*/ false);
      ++pid;
    }
  }
  const int count = pid - first_pid;
  group->setEnabledById(remove_button_pid, count > 0);
}

void animate_node_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params)
{
  Tab<String> names;
  const int animNid = settings->getNameId("anim");
  for (int i = 0; i < settings->blockCount(); ++i)
    if (settings->getBlock(i)->getBlockNameId() == animNid)
      names.emplace_back(settings->getBlock(i)->getStr("param", ""));

  bool isEditable = !names.empty();
  const DataBlock *defaultBlock = settings->getBlockByNameEx("anim");
  const char *defaultBnl = settings->getStr("bnl", "");
  const int defaultSKey = settings->getInt("sKey", 0);
  const int defaultEKey = settings->getInt("eKey", DEFAULT_E_KEY);
  const bool defaultMandatoryAnim = settings->getBool("mandatoryAnim", false);
  const char *bnlValue = defaultBlock->getStr("bnl", defaultBnl);
  const int sKeyValue = defaultBlock->getInt("sKey", defaultSKey);
  const int eKeyValue = defaultBlock->getInt("eKey", defaultEKey);
  const bool mandatoryAnimValue = defaultBlock->getBool("mandatoryAnim", defaultMandatoryAnim);
  panel->createList(PID_CTRLS_NODES_LIST, "Anims", names, 0);
  panel->createButton(PID_CTRLS_NODES_LIST_ADD, "Add anim");
  panel->createButton(PID_CTRLS_NODES_LIST_REMOVE, "Remove", /*enabled*/ true, /*new_line*/ false);
  panel->createEditBox(PID_CTRLS_ANIMATE_NODE_PARAM, "param", defaultBlock->getStr("param", ""), isEditable);
  panel->createEditBox(PID_CTRLS_ANIMATE_NODE_BNL, "bnl", bnlValue, isEditable);
  panel->createEditInt(PID_CTRLS_ANIMATE_NODE_S_KEY, "sKey", sKeyValue, isEditable);
  panel->createEditInt(PID_CTRLS_ANIMATE_NODE_E_KEY, "eKey", eKeyValue, isEditable);
  panel->createEditFloat(PID_CTRLS_ANIMATE_NODE_P_MUL, "pMul", defaultBlock->getReal("pMul", DEFAULT_P_MUL), /*prec*/ 2, isEditable);
  panel->createEditFloat(PID_CTRLS_ANIMATE_NODE_P_ADD, "pAdd", defaultBlock->getReal("pAdd", 0.f), /*prec*/ 2, isEditable);
  panel->createCheckBox(PID_CTRLS_ANIMATE_NODE_ALWAYS_UPDATE, "alwaysUpdate", defaultBlock->getBool("alwaysUpdate", false),
    isEditable);
  panel->createCheckBox(PID_CTRLS_ANIMATE_NODE_MANDATORY_ANIM, "mandatoryAnim", mandatoryAnimValue, isEditable);
  panel->createCheckBox(PID_CTRLS_ANIMATE_NODE_MANDATORY_NODES, "mandatoryNodes", defaultBlock->getBool("mandatoryNodes", false),
    isEditable);
  panel->createCheckBox(PID_CTRLS_ANIMATE_NODE_ARRAY_NODES_ONLY, "arrayNodesOnly", defaultBlock->getBool("arrayNodesOnly", false),
    isEditable);

  panel->createButton(PID_CTRLS_ANIMATE_NODE_ADD_NODE, "Add node", isEditable);
  const int nodeCount = defaultBlock->paramCountByName("node");
  panel->createButton(PID_CTRLS_ANIMATE_NODE_REMOVE_NODE, "Remove last node", nodeCount > 0, /*new_line*/ false);
  init_anim_block_params(panel, *defaultBlock, PID_CTRLS_ANIMATE_NODE_NODES_FIELD, PID_CTRLS_ANIMATE_NODE_ADD_NODE,
    PID_CTRLS_ANIMATE_NODE_REMOVE_NODE, "node");

  panel->createButton(PID_CTRLS_ANIMATE_NODE_ADD_NODE_RE, "Add nodeRE", isEditable);
  const int nodeRECount = defaultBlock->paramCountByName("nodeRE");
  panel->createButton(PID_CTRLS_ANIMATE_NODE_REMOVE_NODE_RE, "Remove last nodeRE", nodeRECount > 0, /*new_line*/ false);
  init_anim_block_params(panel, *defaultBlock, PID_CTRLS_ANIMATE_NODE_RE_NODES_FIELD, PID_CTRLS_ANIMATE_NODE_ADD_NODE_RE,
    PID_CTRLS_ANIMATE_NODE_REMOVE_NODE_RE, "nodeRE");

  params.emplace_back(DependentParamData{PID_CTRLS_ANIMATE_NODE_BNL, PID_CTRLS_DEPENDENT_BNL, strcmp(bnlValue, defaultBnl) == 0});
  params.emplace_back(DependentParamData{PID_CTRLS_ANIMATE_NODE_S_KEY, PID_CTRLS_DEPENDENT_S_KEY, sKeyValue == defaultSKey});
  params.emplace_back(DependentParamData{PID_CTRLS_ANIMATE_NODE_E_KEY, PID_CTRLS_DEPENDENT_E_KEY, eKeyValue == defaultEKey});
  params.emplace_back(DependentParamData{
    PID_CTRLS_ANIMATE_NODE_MANDATORY_ANIM, PID_CTRLS_DEPENDENT_MANDATORY_ANIM, mandatoryAnimValue == defaultMandatoryAnim});
}

static void save_anim_block_params(PropPanel::ContainerPropertyControl *panel, DataBlock &settings, int first_pid, int last_pid,
  const char *param_name)
{
  for (int pid = first_pid; pid < last_pid; ++pid)
  {
    if (panel->getById(pid) == nullptr)
      break;
    const SimpleString value = panel->getText(pid);
    if (!value.empty())
      settings.addStr(param_name, value.c_str());
  }
}

void animate_node_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString listName = panel->getText(PID_CTRLS_NODES_LIST);
  if (listName.empty())
    return;

  DataBlock *selectedBlock = nullptr;
  const int animNid = settings->getNameId("anim");
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    DataBlock *child = settings->getBlock(i);
    if (child->getBlockNameId() == animNid && listName == child->getStr("param", nullptr))
      selectedBlock = child;
  }
  const SimpleString paramValue = panel->getText(PID_CTRLS_ANIMATE_NODE_PARAM);
  const SimpleString bnlValue = panel->getText(PID_CTRLS_ANIMATE_NODE_BNL);
  const int sKeyValue = panel->getInt(PID_CTRLS_ANIMATE_NODE_S_KEY);
  const int eKeyValue = panel->getInt(PID_CTRLS_ANIMATE_NODE_E_KEY);
  const float pMulValue = panel->getFloat(PID_CTRLS_ANIMATE_NODE_P_MUL);
  const float pAddValue = panel->getFloat(PID_CTRLS_ANIMATE_NODE_P_ADD);
  const bool alwaysUpdateValue = panel->getBool(PID_CTRLS_ANIMATE_NODE_ALWAYS_UPDATE);
  const bool mandatoryAnimValue = panel->getBool(PID_CTRLS_ANIMATE_NODE_MANDATORY_ANIM);
  const bool mandatoryNodesValue = panel->getBool(PID_CTRLS_ANIMATE_NODE_MANDATORY_NODES);
  const bool arrayNodesOnlyValue = panel->getBool(PID_CTRLS_ANIMATE_NODE_ARRAY_NODES_ONLY);

  if (!selectedBlock)
    selectedBlock = settings->addNewBlock("anim");

  for (int i = selectedBlock->paramCount() - 1; i >= 0; --i)
    selectedBlock->removeParam(i);

  selectedBlock->setStr("param", paramValue.c_str());
  if (!bnlValue.empty())
    selectedBlock->setStr("bnl", bnlValue.c_str());
  if (sKeyValue != 0)
    selectedBlock->setInt("sKey", sKeyValue);
  if (eKeyValue != DEFAULT_E_KEY)
    selectedBlock->setInt("eKey", eKeyValue);
  if (!is_equal_float(pMulValue, DEFAULT_P_MUL))
    selectedBlock->setReal("pMul", pMulValue);
  if (float_nonzero(pAddValue))
    selectedBlock->setReal("pAdd", pAddValue);
  if (alwaysUpdateValue)
    selectedBlock->setBool("alwaysUpdate", alwaysUpdateValue);
  if (mandatoryAnimValue)
    selectedBlock->setBool("mandatoryAnim", mandatoryAnimValue);
  if (mandatoryNodesValue)
    selectedBlock->setBool("mandatoryNodes", mandatoryNodesValue);
  if (arrayNodesOnlyValue)
    selectedBlock->setBool("arrayNodesOnly", arrayNodesOnlyValue);

  save_anim_block_params(panel, *selectedBlock, PID_CTRLS_ANIMATE_NODE_NODES_FIELD, PID_CTRLS_ANIMATE_NODE_NODES_FIELD_SIZE, "node");
  save_anim_block_params(panel, *selectedBlock, PID_CTRLS_ANIMATE_NODE_RE_NODES_FIELD, PID_CTRLS_ANIMATE_NODE_RE_NODES_FIELD_SIZE,
    "nodeRE");

  if (listName != paramValue)
    panel->setText(PID_CTRLS_NODES_LIST, paramValue.c_str());
}

void animate_node_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params, dag::ConstSpan<AnimParamData> base_params)
{
  const SimpleString name = panel->getText(PID_CTRLS_NODES_LIST);
  const DataBlock *selectedBlock = &DataBlock::emptyBlock;
  const int animNid = settings->getNameId("anim");
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    const DataBlock *child = settings->getBlock(i);
    if (child->getBlockNameId() == animNid && name == child->getStr("param", nullptr))
      selectedBlock = child;
  }
  const char *defaultBnl = settings->getStr("bnl", "");
  const int defaultSKey = settings->getInt("sKey", 0);
  const int defaultEKey = settings->getInt("eKey", DEFAULT_E_KEY);
  const bool defaultMandatoryAnim = settings->getBool("mandatoryAnim", false);
  const char *bnlValue = selectedBlock->getStr("bnl", defaultBnl);
  const int sKeyValue = selectedBlock->getInt("sKey", defaultSKey);
  const int eKeyValue = selectedBlock->getInt("eKey", defaultEKey);
  const bool mandatoryAnimValue = selectedBlock->getBool("mandatoryAnim", defaultMandatoryAnim);
  panel->setText(PID_CTRLS_ANIMATE_NODE_PARAM, name);
  panel->setText(PID_CTRLS_ANIMATE_NODE_BNL, bnlValue);
  panel->setInt(PID_CTRLS_ANIMATE_NODE_S_KEY, sKeyValue);
  panel->setInt(PID_CTRLS_ANIMATE_NODE_E_KEY, eKeyValue);
  panel->setFloat(PID_CTRLS_ANIMATE_NODE_P_MUL, selectedBlock->getReal("pMul", DEFAULT_P_MUL));
  panel->setFloat(PID_CTRLS_ANIMATE_NODE_P_ADD, selectedBlock->getReal("pAdd", 0.f));
  panel->setBool(PID_CTRLS_ANIMATE_NODE_ALWAYS_UPDATE, selectedBlock->getBool("alwaysUpdate", false));
  panel->setBool(PID_CTRLS_ANIMATE_NODE_MANDATORY_ANIM, mandatoryAnimValue);
  panel->setBool(PID_CTRLS_ANIMATE_NODE_MANDATORY_NODES, selectedBlock->getBool("mandatoryNodes", false));
  panel->setBool(PID_CTRLS_ANIMATE_NODE_ARRAY_NODES_ONLY, selectedBlock->getBool("arrayNodesOnly", false));
  bool isEditable = panel->getInt(PID_CTRLS_NODES_LIST) >= 0;
  for (int i = PID_CTRLS_ANIMATE_NODE_PARAM; i <= PID_CTRLS_ANIMATE_NODE_ARRAY_NODES_ONLY; ++i)
    panel->setEnabledById(i, isEditable);

  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  clear_anim_block_params(group, PID_CTRLS_ANIMATE_NODE_NODES_FIELD, PID_CTRLS_ANIMATE_NODE_NODES_FIELD_SIZE);
  clear_anim_block_params(group, PID_CTRLS_ANIMATE_NODE_RE_NODES_FIELD, PID_CTRLS_ANIMATE_NODE_RE_NODES_FIELD_SIZE);
  init_anim_block_params(group, *selectedBlock, PID_CTRLS_ANIMATE_NODE_NODES_FIELD, PID_CTRLS_ANIMATE_NODE_ADD_NODE,
    PID_CTRLS_ANIMATE_NODE_REMOVE_NODE, "node");
  init_anim_block_params(group, *selectedBlock, PID_CTRLS_ANIMATE_NODE_RE_NODES_FIELD, PID_CTRLS_ANIMATE_NODE_ADD_NODE_RE,
    PID_CTRLS_ANIMATE_NODE_REMOVE_NODE_RE, "nodeRE");

  for (DependentParamData &param : params)
  {
    if (param.dependentParamPid == PID_CTRLS_DEPENDENT_BNL)
    {
      param.dependent = strcmp(bnlValue, defaultBnl) == 0;
      update_dependent_param_value_by_name(panel, base_params, param, "bnl");
    }
    else if (param.dependentParamPid == PID_CTRLS_DEPENDENT_S_KEY)
    {
      param.dependent = sKeyValue == defaultSKey;
      update_dependent_param_value_by_name(panel, base_params, param, "sKey");
    }
    else if (param.dependentParamPid == PID_CTRLS_DEPENDENT_E_KEY)
    {
      param.dependent = eKeyValue == defaultEKey;
      update_dependent_param_value_by_name(panel, base_params, param, "eKey");
    }
    else if (param.dependentParamPid == PID_CTRLS_DEPENDENT_MANDATORY_ANIM)
    {
      param.dependent = mandatoryAnimValue == defaultMandatoryAnim;
      update_dependent_param_value_by_name(panel, base_params, param, "mandatoryAnim");
    }
  }
}

void animate_node_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings)
{
  const SimpleString removeName = panel->getText(PID_CTRLS_NODES_LIST);
  const int animNid = settings->getNameId("anim");
  for (int i = 0; i < settings->blockCount(); ++i)
    if (settings->getBlock(i)->getBlockNameId() == animNid && removeName == settings->getBlock(i)->getStr("param", nullptr))
      settings->removeBlock(i);
}

static int count_anim_block_params(PropPanel::ContainerPropertyControl *group, int first_pid, int last_pid)
{
  int count = 0;
  for (int pid = first_pid; pid < last_pid; ++pid)
    if (group->getById(pid) != nullptr)
      ++count;
  return count;
}

static void add_anim_block_param(PropPanel::ContainerPropertyControl *group, int first_pid, int last_pid, int add_button_pid,
  int remove_button_pid, const char *param_name)
{
  const int count = count_anim_block_params(group, first_pid, last_pid);
  if (count >= last_pid - first_pid)
    return;
  const int pid = first_pid + count;
  group->createEditBox(pid, param_name);
  group->moveById(pid, add_button_pid, /*after*/ false);
  group->setEnabledById(remove_button_pid, true);
}

static void remove_anim_block_param(PropPanel::ContainerPropertyControl *group, int first_pid, int last_pid, int remove_button_pid)
{
  for (int pid = last_pid - 1; pid >= first_pid; --pid)
  {
    if (group->getById(pid) != nullptr)
    {
      group->removeById(pid);
      const int count = count_anim_block_params(group, first_pid, last_pid);
      group->setEnabledById(remove_button_pid, count > 0);
      return;
    }
  }
}

void animate_node_add_anim_node(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  add_anim_block_param(group, PID_CTRLS_ANIMATE_NODE_NODES_FIELD, PID_CTRLS_ANIMATE_NODE_NODES_FIELD_SIZE,
    PID_CTRLS_ANIMATE_NODE_ADD_NODE, PID_CTRLS_ANIMATE_NODE_REMOVE_NODE, "node");
}

void animate_node_remove_anim_node(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  remove_anim_block_param(group, PID_CTRLS_ANIMATE_NODE_NODES_FIELD, PID_CTRLS_ANIMATE_NODE_NODES_FIELD_SIZE,
    PID_CTRLS_ANIMATE_NODE_REMOVE_NODE);
}

void animate_node_add_anim_node_re(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  add_anim_block_param(group, PID_CTRLS_ANIMATE_NODE_RE_NODES_FIELD, PID_CTRLS_ANIMATE_NODE_RE_NODES_FIELD_SIZE,
    PID_CTRLS_ANIMATE_NODE_ADD_NODE_RE, PID_CTRLS_ANIMATE_NODE_REMOVE_NODE_RE, "nodeRE");
}

void animate_node_remove_anim_node_re(PropPanel::ContainerPropertyControl *panel)
{
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  remove_anim_block_param(group, PID_CTRLS_ANIMATE_NODE_RE_NODES_FIELD, PID_CTRLS_ANIMATE_NODE_RE_NODES_FIELD_SIZE,
    PID_CTRLS_ANIMATE_NODE_REMOVE_NODE_RE);
}

void AnimTreePlugin::animateNodeFindChilds(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data, const DataBlock &settings)
{
  const int animNid = settings.getNameId("anim");
  for (int i = 0; i < settings.blockCount(); ++i)
  {
    const DataBlock *block = settings.getBlock(i);
    if (block->getBlockNameId() == animNid)
    {
      const char *bnl = block->getStr("bnl", settings.getStr("bnl", ""));
      if (*bnl)
      {
        const int childIdx = add_ctrl_child_idx_by_name(panel, data, controllersData, blendNodesData, bnl);
        check_ctrl_child_idx(childIdx, settings.getStr("name"), bnl);
      }
    }
  }
}

const char *animate_node_get_child_name_by_idx(const DataBlock &settings, int idx)
{
  return settings.getBlock(idx)->getStr("bnl", settings.getStr("bnl", nullptr));
}

String animate_node_get_child_prefix_name(const DataBlock &settings, int idx)
{
  return String(0, "[%s] ", settings.getBlock(idx)->getStr("param", ""));
}

void animate_node_update_child_name(DataBlock &settings, const char *name, const String &old_name)
{
  String writeName;
  const char *baseBnl = settings.getStr("bnl", nullptr);
  if (baseBnl && get_updated_child_name(name, old_name, baseBnl, writeName))
    settings.setStr("bnl", writeName.c_str());

  const int animNid = settings.getNameId("anim");
  for (int i = 0; i < settings.blockCount(); ++i)
  {
    DataBlock *block = settings.getBlock(i);
    if (block->getBlockNameId() == animNid)
    {
      const char *bnl = block->getStr("bnl", nullptr);
      if (bnl && get_updated_child_name(name, old_name, bnl, writeName))
        block->setStr("bnl", writeName.c_str());
    }
  }
}
