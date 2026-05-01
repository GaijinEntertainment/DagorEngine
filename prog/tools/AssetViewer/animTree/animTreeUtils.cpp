// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animTreeUtils.h"
#include "animTreePanelPids.h"
#include "animParamData.h"
#include "controllers/ctrlType.h"
#include "controllers/animCtrlData.h"
#include "blendNodes/blendNodeType.h"
#include "blendNodes/blendNodeData.h"
#include "animStates/animStatesData.h"
#include "animStates/animStatesType.h"

#include <osApiWrappers/dag_direct.h>
#include <propPanel/control/container.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <anim/dag_animBlend.h>        // getAnimMaxTime
#include <gameRes/dag_gameResources.h> // get_game_resource_ex
#include <gameRes/dag_stdGameResId.h>  // Anim2DataGameResClassId
#include <propPanel/imguiHelper.h>     // getStringIndexInTab
#include <util/dag_lookup.h>           // lup
#include <ioSys/dag_dataBlockCommentsDef.h>

bool is_include_ctrl(const AnimCtrlData &data) { return data.type == ctrl_type_include || data.type == ctrl_type_inner_include; }

const AnimParamData *find_param_by_name(dag::ConstSpan<AnimParamData> params, const char *name)
{
  return eastl::find_if(params.begin(), params.end(), [name](const AnimParamData &value) { return value.name == name; });
}

AnimParamData *find_param_by_name(dag::Vector<AnimParamData> &params, const char *name)
{
  return eastl::find_if(params.begin(), params.end(), [name](const AnimParamData &value) { return value.name == name; });
}

bool add_edit_box_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, const char *default_value)
{
  auto param = find_param_by_name(params, name);
  if (param == params.end())
  {
    panel->createEditBox(pid, name, default_value);
    params.emplace_back(AnimParamData{pid, DataBlock::TYPE_STRING, String(name)});
    ++pid;
    return true;
  }
  else
    panel->moveById(param->pid);

  return false;
}

bool add_edit_float_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, float default_value)
{
  auto param = find_param_by_name(params, name);
  if (param == params.end())
  {
    panel->createEditFloat(pid, name, default_value);
    params.emplace_back(AnimParamData{pid, DataBlock::TYPE_REAL, String(name)});
    ++pid;
    return true;
  }
  else
    panel->moveById(param->pid);

  return false;
}

bool add_edit_int_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, int default_value)
{
  auto param = find_param_by_name(params, name);
  if (param == params.end())
  {
    panel->createEditInt(pid, name, default_value);
    params.emplace_back(AnimParamData{pid, DataBlock::TYPE_INT, String(name)});
    ++pid;
    return true;
  }
  else
    panel->moveById(param->pid);

  return false;
}

bool add_edit_point2_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, Point2 default_value)
{
  auto param = find_param_by_name(params, name);
  if (param == params.end())
  {
    panel->createPoint2(pid, name, default_value);
    params.emplace_back(AnimParamData{pid, DataBlock::TYPE_POINT2, String(name)});
    ++pid;
    return true;
  }
  else
    panel->moveById(param->pid);

  return false;
}

bool add_edit_point3_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, Point3 default_value)
{
  auto param = find_param_by_name(params, name);
  if (param == params.end())
  {
    panel->createPoint3(pid, name, default_value);
    params.emplace_back(AnimParamData{pid, DataBlock::TYPE_POINT3, String(name)});
    ++pid;
    return true;
  }
  else
    panel->moveById(param->pid);

  return false;
}

bool add_edit_point4_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, Point4 default_value)
{
  auto param = find_param_by_name(params, name);
  if (param == params.end())
  {
    panel->createPoint4(pid, name, default_value);
    params.emplace_back(AnimParamData{pid, DataBlock::TYPE_POINT4, String(name)});
    ++pid;
    return true;
  }
  else
    panel->moveById(param->pid);

  return false;
}

bool add_edit_bool_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, bool default_value)
{
  auto param = find_param_by_name(params, name);
  if (param == params.end())
  {
    panel->createCheckBox(pid, name, default_value);
    params.emplace_back(AnimParamData{pid, DataBlock::TYPE_BOOL, String(name)});
    ++pid;
    return true;
  }
  else
    panel->moveById(param->pid);

  return false;
}

bool add_edit_matrix_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, const TMatrix &default_value)
{
  auto param = find_param_by_name(params, name);
  if (param == params.end())
  {
    panel->createMatrix(pid, name, default_value);
    params.emplace_back(AnimParamData{pid, DataBlock::TYPE_MATRIX, String(name)});
    ++pid;
    return true;
  }
  else
    panel->moveById(param->pid);

  return false;
}

void add_target_node(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const char *targetNodeParam = "targetNode";
  const float defaultTargetNodeWt = 1.f;
  const float targetNodeWtValue = get_float_param_by_name_optional(params, panel, "targetNodeWt", defaultTargetNodeWt);
  SimpleString nodeTypeName = panel->getText(PID_CTRLS_TYPE_COMBO_SELECT);
  CtrlType type = static_cast<CtrlType>(lup(nodeTypeName.c_str(), ctrl_type, ctrl_type_not_found));
  if (type == ctrl_type_twistCtrl)
    targetNodeParam = "twistNode"; // Replace param name for twistCtrl with same logic
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  int freePid = params.back().pid + 1;
  int targetNodeCount = 0;
  AnimParamData *lastNodeForMove = nullptr;
  for (AnimParamData &param : params)
    if (param.name == targetNodeParam)
    {
      ++targetNodeCount;
      lastNodeForMove = &param; // Save last targetNode param for add new one in lookat ctrl
    }

  if (type != ctrl_type_lookat && type != ctrl_type_twistCtrl)
  {
    lastNodeForMove = find_param_by_name(params, String(0, "targetNodeWt%d", targetNodeCount));
    G_ASSERTF(lastNodeForMove != params.end(), "Can't find node <targetNodeWt%d> for move new node", targetNodeCount);
  }

  group->createEditBox(freePid, targetNodeParam);
  params.emplace_back(AnimParamData{freePid, DataBlock::TYPE_STRING, String(targetNodeParam)});
  group->moveById(freePid, lastNodeForMove->pid, /*after*/ true);

  if (type != ctrl_type_lookat && type != ctrl_type_twistCtrl)
  {
    freePid = freePid + 1;
    group->createEditFloat(freePid, String(0, "targetNodeWt%d", targetNodeCount + 1), targetNodeWtValue);
    params.emplace_back(AnimParamData{freePid, DataBlock::TYPE_REAL, String(0, "targetNodeWt%d", targetNodeCount + 1)});
    group->moveById(freePid, freePid - 1, /*after*/ true);
  }

  if (targetNodeCount == 1)
    group->setEnabledById(PID_CTRLS_TARGET_NODE_REMOVE, true);
}

void add_params_target_node_wt(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid)
{
  const float defaultTargetNodeWt = 1.f;
  int targetNodeWtIdx = 1;
  int lastTargetNodeWtPid = 0;
  float targetNodeWtValue = get_float_param_by_name_optional(params, panel, "targetNodeWt", defaultTargetNodeWt);
  for (const AnimParamData &param : params)
  {
    if (param.name == "targetNode")
    {
      if (lastTargetNodeWtPid)
        panel->moveById(param.pid, lastTargetNodeWtPid, /*after*/ true);
      const int targetNodeWtFieldIdx = pid;
      String targetNodeWtN = String(0, "targetNodeWt%d", targetNodeWtIdx);
      if (add_edit_float_if_not_exists(params, panel, pid, targetNodeWtN, targetNodeWtValue))
      {
        panel->moveById(targetNodeWtFieldIdx, param.pid, /*after*/ true);
        lastTargetNodeWtPid = targetNodeWtFieldIdx;
      }
      else
      {
        auto paramTargetNodeWtN = find_param_by_name(params, targetNodeWtN);
        panel->moveById(paramTargetNodeWtN->pid, param.pid, /*after*/ true);
        lastTargetNodeWtPid = paramTargetNodeWtN->pid;
      }
      ++targetNodeWtIdx;
    }
  }
}

bool get_bool_param_by_name(const dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name)
{
  auto data = find_param_by_name(params, name);
  if (data != params.end())
    return panel->getBool(data->pid);
  else
    G_ASSERT_FAIL("Can't find param <%s>", name);

  return false;
}

bool get_bool_param_by_name_optional(const dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel,
  const char *name, bool default_value)
{
  auto data = find_param_by_name(params, name);
  if (data != params.end())
    return panel->getBool(data->pid);

  return default_value;
}

float get_float_param_by_name_optional(const dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel,
  const char *name, float default_value)
{
  auto data = find_param_by_name(params, name);
  if (data != params.end())
    return panel->getFloat(data->pid);

  return default_value;
}

int get_int_param_by_name_optional(const dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel,
  const char *name, int default_value)
{
  auto data = find_param_by_name(params, name);
  if (data != params.end())
    return panel->getInt(data->pid);

  return default_value;
}

SimpleString get_str_param_by_name_optional(const dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel,
  const char *name, const char *default_value)
{
  auto data = find_param_by_name(params, name);
  if (data != params.end())
    return panel->getText(data->pid);

  return SimpleString(default_value);
}

void set_str_param_by_name_if_default(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  const char *str, const char *default_value)
{
  AnimParamData *data = find_param_by_name(params, name);
  if (data != params.end())
  {
    const SimpleString value = panel->getText(data->pid);
    if (value == default_value)
    {
      panel->setText(data->pid, str);
      data->dependent = true;
    }
  }
}

void set_float_param_by_name_if_default(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel,
  const char *name, float set_value, float default_value)
{
  AnimParamData *data = find_param_by_name(params, name);
  if (data != params.end())
  {
    const float value = panel->getFloat(data->pid);
    if (is_equal_float(value, default_value))
    {
      panel->setFloat(data->pid, set_value);
      data->dependent = true;
    }
  }
}

void remove_param_if_default_float(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  float default_value)
{
  auto param = find_param_by_name(params, name);
  if (param != params.end())
  {
    const float value = panel->getFloat(param->pid);
    if (is_equal_float(value, default_value))
      params.erase(param);
  }
}

void remove_param_if_default_int(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  int default_value)
{
  auto param = find_param_by_name(params, name);
  if (param != params.end())
  {
    const int value = panel->getInt(param->pid);
    if (value == default_value)
      params.erase(param);
  }
}

void remove_param_if_default_str(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  const char *default_value)
{
  auto param = find_param_by_name(params, name);
  if (param != params.end())
  {
    const SimpleString value = panel->getText(param->pid);
    if (value == default_value)
      params.erase(param);
  }
}

void remove_param_if_default_point2(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  Point2 default_value)
{
  auto param = find_param_by_name(params, name);
  if (param != params.end())
  {
    const Point2 value = panel->getPoint2(param->pid);
    if (value == default_value)
      params.erase(param);
  }
}

void remove_param_if_default_point3(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  Point3 default_value)
{
  auto param = find_param_by_name(params, name);
  if (param != params.end())
  {
    const Point3 value = panel->getPoint3(param->pid);
    if (value == default_value)
      params.erase(param);
  }
}

void remove_param_if_default_point4(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  Point4 default_value)
{
  auto param = find_param_by_name(params, name);
  if (param != params.end())
  {
    const Point4 value = panel->getPoint4(param->pid);
    if (value == default_value)
      params.erase(param);
  }
}

void remove_param_if_default_bool(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  bool default_value)
{
  auto param = find_param_by_name(params, name);
  if (param != params.end())
  {
    const bool value = panel->getBool(param->pid);
    if (value == default_value)
      params.erase(param);
  }
}

void remove_param_if_default_matrix(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  TMatrix default_value)
{
  auto param = find_param_by_name(params, name);
  if (param != params.end())
  {
    const TMatrix value = panel->getMatrix(param->pid);
    if (value == default_value)
      params.erase(param);
  }
}

void remove_params_if_default_target_node_wt(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const float defaultTargetNodeWt = 1.f;
  const float targetNodeWtValue = get_float_param_by_name_optional(params, panel, "targetNodeWt", defaultTargetNodeWt);

  int targetNodeWtIdx = 1;
  for (const AnimParamData &param : params)
  {
    if (param.name == "targetNode")
    {
      remove_param_if_default_float(params, panel, String(0, "targetNodeWt%d", targetNodeWtIdx), targetNodeWtValue);
      ++targetNodeWtIdx;
    }
  }
}

void remove_fields(PropPanel::ContainerPropertyControl *panel, int field_start, int field_end, bool break_if_not_found)
{
  for (int i = field_start; i < field_end; ++i)
    if (!panel->removeById(i) && break_if_not_found)
      return;
}

void remove_nodes_fields(PropPanel::ContainerPropertyControl *panel)
{
  const bool breakIfNotFound = true;
  remove_fields(panel, PID_NODES_PARAMS_FIELD, PID_NODES_PARAMS_FIELD_SIZE, breakIfNotFound);
  remove_fields(panel, PID_NODES_ACTION_FIELD, PID_NODES_ACTION_FIELD_SIZE, !breakIfNotFound);
}

void remove_ctrls_fields(PropPanel::ContainerPropertyControl *panel, int last_param_pid)
{
  const bool breakIfNotFound = true;
  remove_fields(panel, PID_CTRLS_PARAMS_FIELD, last_param_pid + 1, !breakIfNotFound);
  remove_fields(panel, PID_CTRLS_ACTION_FIELD, PID_CTRLS_ACTION_FIELD_SIZE, !breakIfNotFound);
}

void remove_target_node(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const char *targetNodeParam = "targetNode";
  SimpleString nodeTypeName = panel->getText(PID_CTRLS_TYPE_COMBO_SELECT);
  CtrlType type = static_cast<CtrlType>(lup(nodeTypeName.c_str(), ctrl_type, ctrl_type_not_found));
  if (type == ctrl_type_twistCtrl)
    targetNodeParam = "twistNode"; // Replace param name for twistCtrl with same logic
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  int targetNodeCount = 0;
  int targetNodeLastIdx = -1;
  for (int i = 0; i < params.size(); ++i)
  {
    if (params[i].name == targetNodeParam)
    {
      targetNodeLastIdx = i;
      ++targetNodeCount;
    }
  }

  G_ASSERTF(targetNodeLastIdx != -1, "Not found any <%s> param", targetNodeParam);

  group->removeById(params[targetNodeLastIdx].pid);
  params.erase(params.begin() + targetNodeLastIdx);

  if (type != ctrl_type_lookat && type != ctrl_type_twistCtrl)
  {
    auto targetNodeWtLast = find_param_by_name(params, String(0, "targetNodeWt%d", targetNodeCount));
    G_ASSERTF(targetNodeWtLast != params.end(), "Can't find node <targetNodeWt%d> for remove", targetNodeCount);
    group->removeById(targetNodeWtLast->pid);
    params.erase(targetNodeWtLast);
  }

  if (targetNodeCount == 2)
    group->setEnabledById(PID_CTRLS_TARGET_NODE_REMOVE, false);
}

void update_dependent_param_value(PropPanel::ContainerPropertyControl *panel, const AnimParamData &param, int dependent_pid)
{
  if (param.type == DataBlock::TYPE_STRING)
  {
    const SimpleString value = panel->getText(param.pid);
    panel->setText(dependent_pid, value);
  }
  else if (param.type == DataBlock::TYPE_BOOL)
  {
    const bool value = panel->getBool(param.pid);
    panel->setBool(dependent_pid, value);
  }
  else if (param.type == DataBlock::TYPE_REAL)
  {
    const float value = panel->getFloat(param.pid);
    panel->setFloat(dependent_pid, value);
  }
  else if (param.type == DataBlock::TYPE_INT)
  {
    const int value = panel->getInt(param.pid);
    panel->setInt(dependent_pid, value);
  }
}

void update_dependent_param_value_by_name(PropPanel::ContainerPropertyControl *panel, dag::ConstSpan<AnimParamData> base_params,
  const DependentParamData &param, const char *param_name)
{
  if (param.dependent)
  {
    const AnimParamData *baseParam = find_param_by_name(base_params, param_name);
    if (baseParam == base_params.end())
      return;

    update_dependent_param_value(panel, *baseParam, param.pid);
  }
}

void update_dependent_param_value_by_pid(PropPanel::ContainerPropertyControl *panel, dag::ConstSpan<DependentParamData> params,
  AnimParamData &param, int pid)
{
  const DependentParamData *dependentParam =
    eastl::find_if(params.begin(), params.end(), [pid](const DependentParamData &value) { return value.dependentParamPid == pid; });
  if (dependentParam != params.end() && dependentParam->dependent)
    update_dependent_param_value(panel, param, dependentParam->pid);
}

void update_dependent_param_state(PropPanel::ContainerPropertyControl *panel, dag::ConstSpan<AnimParamData> params,
  DependentParamData &param, const char *parent_name)
{
  const AnimParamData *parentParam = find_param_by_name(params, parent_name);
  if (parentParam != params.end())
  {
    if (parentParam->type == DataBlock::TYPE_STRING)
    {
      const SimpleString parentValue = panel->getText(parentParam->pid);
      const SimpleString value = panel->getText(param.pid);
      param.dependent = parentValue == value;
    }
    else if (parentParam->type == DataBlock::TYPE_BOOL)
    {
      const bool parentValue = panel->getBool(parentParam->pid);
      const bool value = panel->getBool(param.pid);
      param.dependent = parentValue == value;
    }
    else if (parentParam->type == DataBlock::TYPE_REAL)
    {
      const float parentValue = panel->getFloat(parentParam->pid);
      const float value = panel->getFloat(param.pid);
      param.dependent = is_equal_float(parentValue, value);
    }
    else if (parentParam->type == DataBlock::TYPE_INT)
    {
      const int parentValue = panel->getInt(parentParam->pid);
      const int value = panel->getInt(param.pid);
      param.dependent = parentValue == value;
    }
  }
}

DataBlock *find_block_by_name(DataBlock *props, const String &name, bool should_exist)
{
  for (int i = 0; i < props->blockCount(); ++i)
  {
    DataBlock *settings = props->getBlock(i);
    if (name == settings->getStr("name", ""))
      return settings;
  }

  G_ASSERTF(!should_exist, "Can't find block with name <%s>", name);
  return nullptr;
}

DataBlock *find_animate_and_proc_block(DataBlock *props, const String &name)
{
  for (int i = 0; i < props->blockCount(); ++i)
  {
    DataBlock *settings = props->getBlock(i);
    if (strcmp(settings->getBlockName(), "animateAndProcNode") != 0)
      continue;
    if (const DataBlock *animateNode = settings->getBlockByName("animateNode"))
      if (name == animateNode->getStr("name", ""))
        return settings;
  }
  return nullptr;
}

DataBlock *find_block_by_block_name(DataBlock *props, const String &name, bool should_exist)
{
  for (int i = 0; i < props->blockCount(); ++i)
  {
    DataBlock *settings = props->getBlock(i);
    if (name == settings->getBlockName())
      return settings;
  }

  G_ASSERTF(!should_exist, "Can't find block with name <%s>", name);
  return nullptr;
}

static DataBlock *find_a2d_or_blend_node_settings(DataBlock &props, const char *name, bool return_a2d)
{
  for (int i = 0; i < props.blockCount(); ++i)
  {
    DataBlock *node = props.getBlock(i);
    if (!strcmp(node->getBlockName(), "AnimBlendNodeLeaf"))
    {
      for (int j = 0; j < node->blockCount(); ++j)
      {
        DataBlock *settings = node->getBlock(j);
        if (!strcmp(settings->getStr("name", ""), name))
          return return_a2d ? node : settings;
      }
    }
  }

  return nullptr;
}

DataBlock *find_blend_node_settings(DataBlock &props, const char *name)
{
  return find_a2d_or_blend_node_settings(props, name, /*return_a2d*/ false);
}

DataBlock *find_a2d_node_settings(DataBlock &props, const char *name)
{
  return find_a2d_or_blend_node_settings(props, name, /*return_a2d*/ true);
}

String get_folder_path_based_on_parent(dag::ConstSpan<String> paths, const DagorAsset &asset,
  PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle include_leaf)
{
  PropPanel::TLeafHandle parent = tree->getParentLeaf(include_leaf);
  PropPanel::TLeafHandle root = tree->getRootLeaf();
  char fileLocationBuf[DAGOR_MAX_PATH];
  if (include_leaf == root || parent == root)
    return String(dd_get_fname_location(fileLocationBuf, asset.getSrcFilePath().c_str()));

  String fullPath;
  String name = tree->getCaption(parent);
  const String *path = eastl::find_if(paths.begin(), paths.end(), [&name](const String &value) {
    char fileNameBuf[DAGOR_MAX_PATH];
    return name == dd_get_fname_without_path_and_ext(fileNameBuf, sizeof(fileNameBuf), value.c_str());
  });
  if (path != paths.end())
  {
    fullPath.setStr(path->c_str());
    if (!DataBlock::resolveIncludePath(fullPath))
      return get_folder_path_based_on_parent(paths, asset, tree, parent);
    else
      return String(dd_get_fname_location(fileLocationBuf, fullPath.c_str()));
  }
  else
    logerr("can't find include with name: %s", name.c_str());

  return fullPath;
}

String find_full_path(dag::ConstSpan<String> paths, const DagorAsset &asset, PropPanel::ContainerPropertyControl *tree,
  PropPanel::TLeafHandle include_leaf)
{
  if (include_leaf == tree->getRootLeaf())
    return asset.getSrcFilePath();

  String fullPath;
  const String name = tree->getCaption(include_leaf);
  const String *path = eastl::find_if(paths.begin(), paths.end(), [&name](const String &value) {
    char fileNameBuf[DAGOR_MAX_PATH];
    return name == dd_get_fname_without_path_and_ext(fileNameBuf, sizeof(fileNameBuf), value.c_str());
  });
  if (path != paths.end())
  {
    fullPath.setStr(path->c_str());
    if (!DataBlock::resolveIncludePath(fullPath))
    {
      String folderPath = get_folder_path_based_on_parent(paths, asset, tree, include_leaf);
      fullPath = String(0, "%s%s", folderPath.c_str(), path->c_str());
    }
  }
  else
    logerr("can't find include with name: %s", name.c_str());

  dd_simplify_fname_c(fullPath.c_str());
  return fullPath;
}

String find_folder_path(dag::ConstSpan<String> paths, const String &name)
{
  const String *path = eastl::find_if(paths.begin(), paths.end(), [&name](const String &value) {
    char fileNameBuf[DAGOR_MAX_PATH];
    return name == dd_get_fname_without_path_and_ext(fileNameBuf, sizeof(fileNameBuf), value.c_str());
  });
  char fileLocationBuf[DAGOR_MAX_PATH];
  return (path != paths.end()) ? String(dd_get_fname_location(fileLocationBuf, path->c_str())) : String("");
}

String find_resolved_folder_path(dag::ConstSpan<String> paths, const DagorAsset &asset, PropPanel::ContainerPropertyControl *tree,
  PropPanel::TLeafHandle include_leaf)
{
  const String name = tree->getCaption(include_leaf);
  const String *path = eastl::find_if(paths.begin(), paths.end(), [&name](const String &value) {
    char fileNameBuf[DAGOR_MAX_PATH];
    return name == dd_get_fname_without_path_and_ext(fileNameBuf, sizeof(fileNameBuf), value.c_str());
  });
  if (path == paths.end())
    return String("");

  String fullPath;
  fullPath.setStr(path->c_str());
  if (!DataBlock::resolveIncludePath(fullPath))
  {
    String folderPath = get_folder_path_based_on_parent(paths, asset, tree, include_leaf);
    fullPath = String(0, "%s%s", folderPath.c_str(), path->c_str());
  }

  char fileLocationBuf[DAGOR_MAX_PATH];
  return String(dd_get_fname_location(fileLocationBuf, fullPath.c_str()));
}

DataBlock get_props_from_include_leaf(dag::ConstSpan<String> paths, const DagorAsset &asset, PropPanel::ContainerPropertyControl *tree,
  PropPanel::TLeafHandle include_leaf, String &full_path, bool only_includes)
{
  if (!include_leaf)
  {
    logerr("can't find include leaf");
    return DataBlock::emptyBlock;
  }
  full_path = find_full_path(paths, asset, tree, include_leaf);
  const bool curParseIncludesAsParams = DataBlock::parseIncludesAsParams;
  const bool curParseOverridesNotApply = DataBlock::parseOverridesNotApply;
  const bool curParseCommentsAsParams = DataBlock::parseCommentsAsParams;
  DataBlock::parseIncludesAsParams = true;
  if (!only_includes)
  {
    DataBlock::parseOverridesNotApply = true;
    DataBlock::parseCommentsAsParams = true;
  }

  DataBlock props(full_path);
  DataBlock::parseIncludesAsParams = curParseIncludesAsParams;
  if (!only_includes)
  {
    DataBlock::parseOverridesNotApply = curParseOverridesNotApply;
    DataBlock::parseCommentsAsParams = curParseCommentsAsParams;
  }
  return props;
}

DataBlock get_props_from_include_state_data(dag::ConstSpan<String> paths, dag::ConstSpan<AnimCtrlData> ctrls, const DagorAsset &asset,
  PropPanel::ContainerPropertyControl *ctrls_tree, const AnimStatesData &state_data, String &full_path, bool only_includes)
{
  // Just ignore this state types
  if (state_data.type == AnimStatesType::PREVIEW || state_data.type == AnimStatesType::INCLUDE_ROOT)
    return DataBlock::emptyBlock;

  if (state_data.fileName.empty())
  {
    logerr("can't find state fileName source");
    return DataBlock::emptyBlock;
  }

  PropPanel::TLeafHandle includeLeaf;
  if (state_data.fileName == "Root")
    includeLeaf = ctrls_tree->getRootLeaf();
  else
  {
    const AnimCtrlData *ctrlData = eastl::find_if(ctrls.begin(), ctrls.end(), [ctrls_tree, state_data](const AnimCtrlData &data) {
      return data.type == ctrl_type_include && state_data.fileName == ctrls_tree->getCaption(data.handle);
    });

    if (ctrlData == ctrls.end())
    {
      logerr("can't find include leaf from ctrls tree");
      return DataBlock::emptyBlock;
    }
    includeLeaf = ctrlData->handle;
  }

  return get_props_from_include_leaf(paths, asset, ctrls_tree, includeLeaf, full_path, only_includes);
}

static bool is_inner_include(dag::ConstSpan<AnimCtrlData> ctrls, PropPanel::TLeafHandle include_leaf)
{
  const AnimCtrlData *data =
    eastl::find_if(ctrls.begin(), ctrls.end(), [include_leaf](const AnimCtrlData &data) { return data.handle == include_leaf; });

  if (data != ctrls.end())
    return data->type == ctrl_type_inner_include;

  return false;
}

DataBlock *get_props_from_include_leaf_ctrl_node(dag::ConstSpan<AnimCtrlData> ctrls, dag::ConstSpan<String> paths,
  const DagorAsset &asset, DataBlock &props, PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle include_leaf)
{
  DataBlock *ctrlsBlk;
  if (props.getNameId("AnimBlendCtrl") != -1)
    ctrlsBlk = props.getBlockByName("AnimBlendCtrl");
  else if (include_leaf != tree->getRootLeaf() && is_inner_include(ctrls, include_leaf))
    ctrlsBlk = &props;
  else
    ctrlsBlk = props.addBlock("AnimBlendCtrl");

  if (!ctrlsBlk)
  {
    logerr("Can't find blocks with controllers in <%s> blk", tree->getCaption(include_leaf));
    return nullptr;
  }
  return ctrlsBlk;
}

eastl::string_view name_without_node_mask_suffix(eastl::string_view str)
{
  size_t pos = str.find(':');
  if (pos != eastl::string_view::npos)
    return str.substr(0, pos);
  else
    return str;
}

CtrlChildSearchResult find_child_idx_and_icon_by_name(PropPanel::ContainerPropertyControl *ctrls_tree,
  PropPanel::ContainerPropertyControl *nodes_tree, PropPanel::TLeafHandle parent_handle, dag::ConstSpan<AnimCtrlData> controllers,
  dag::ConstSpan<BlendNodeData> blend_nodes, const char *name)
{
  const AnimCtrlData *childCtrl = eastl::find_if(controllers.begin(), controllers.end(), [name, ctrls_tree](const AnimCtrlData &data) {
    return name_without_node_mask_suffix(ctrls_tree->getCaption(data.handle).c_str()) == name_without_node_mask_suffix(name);
  });
  if (childCtrl != controllers.end())
  {
    const AnimCtrlData *parentData = find_data_by_handle(controllers, parent_handle);
    if (parentData != controllers.end())
    {
      const int childIdx = eastl::distance(controllers.begin(), childCtrl);
      const int parentIdx = eastl::distance(controllers.begin(), parentData);
      if (childIdx == parentIdx)
      {
        logerr("Controller <%s> can't use self as child", ctrls_tree->getCaption(parent_handle));
        return {AnimCtrlData::CHILD_AS_SELF, nullptr, BlendNodeType::UNSET, childCtrl->type};
      }
    }

    return {childCtrl->id, get_ctrl_icon_name(childCtrl->type), BlendNodeType::UNSET, childCtrl->type};
  }
  else
  {
    const BlendNodeData *childNode =
      eastl::find_if(blend_nodes.begin(), blend_nodes.end(), [name, nodes_tree](const BlendNodeData &data) {
        return name_without_node_mask_suffix(nodes_tree->getCaption(data.handle).c_str()) == name_without_node_mask_suffix(name);
      });
    if (childNode != blend_nodes.end())
      return {childNode->id, get_blend_node_icon_name(childNode->type), childNode->type, ctrl_type_not_found};
  }

  return {AnimCtrlData::NOT_FOUND_CHILD, nullptr, BlendNodeType::UNSET, ctrl_type_not_found};
}

CtrlChildSearchResult find_child_idx_and_icon_by_name(PropPanel::ContainerPropertyControl *panel, PropPanel::TLeafHandle parent_handle,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name)
{
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *nodesTree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  return find_child_idx_and_icon_by_name(ctrlsTree, nodesTree, parent_handle, controllers, blend_nodes, name);
}

int find_child_idx_by_name(PropPanel::ContainerPropertyControl *panel, PropPanel::TLeafHandle parent_handle,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name)
{
  return find_child_idx_and_icon_by_name(panel, parent_handle, controllers, blend_nodes, name).id;
}

int find_state_child_idx_by_name(PropPanel::ContainerPropertyControl *panel, PropPanel::TLeafHandle parent_handle,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name)
{
  if (!strcmp(name, ""))
    return AnimStatesData::EMPTY_CHILD_TYPE;
  else if (!strcmp(name, "*"))
    return AnimStatesData::LEAVE_CUR_CHILD_TYPE;
  else
    return find_child_idx_by_name(panel, parent_handle, controllers, blend_nodes, name);
}

int find_init_fifo3_child_idx_by_name(PropPanel::ContainerPropertyControl *panel, PropPanel::TLeafHandle parent_handle,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name)
{
  if (!strcmp(name, "default"))
    return AnimStatesData::DEFAULT_INIT_FIFO3;
  else
    return find_child_idx_by_name(panel, parent_handle, controllers, blend_nodes, name);
}

int add_ctrl_child_idx_by_name(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name)
{
  return data.childs.emplace_back(find_child_idx_by_name(panel, data.handle, controllers, blend_nodes, name));
}

void check_ctrl_child_idx(int idx, const char *ctrl_name, const char *child_name, bool is_optional)
{
  if (idx == AnimCtrlData::NOT_FOUND_CHILD && !is_optional)
    logerr("can't find child idx for ctrl <%s>, child <%s>", ctrl_name, child_name);
}

void check_state_child_idx(int idx, const char *state_name, const char *child_name)
{
  if (idx == AnimStatesData::NOT_FOUND_CHILD)
    logerr("can't find child idx for state <%s>, child <%s>", state_name, child_name);
}

void check_init_anim_state_child_idx(int idx, const char *block_name, const char *child_name)
{
  if (idx == AnimStatesData::NOT_FOUND_CHILD)
    logerr("can't find child idx for <%s> in initAnimState block, child <%s>", block_name, child_name);
}

void check_fifo3_ctrl_child_idx(int idx, const char *fifo3_name)
{
  if (idx == AnimStatesData::NOT_FOUND_CHILD)
    logerr("can't find fifo3 ctrl child idx for fifo3 in initAnimState block, child <%s>", fifo3_name);
}

float get_default_anim_time(const char *a2d_name, const DagorAssetMgr &mgr)
{
  if (!strcmp(a2d_name, "") || mgr.getAssetNameId(DagorAsset::fpath2asset(a2d_name)) == -1)
    return 1.0f;

  AnimV20::AnimData *animData =
    (AnimV20::AnimData *)::get_game_resource_ex(DagorAsset::fpath2asset(a2d_name), Anim2DataGameResClassId);
  int a2dTicks = 0;
  if (animData)
    a2dTicks = getAnimMaxTime(animData->anim);
  return a2dTicks > 0 ? a2dTicks / (float)AnimV20::TIME_TicksPerSec : 1.0f;
}

static int get_filter_pid_by_tree(int tree_pid)
{
  if (tree_pid == PID_ANIM_BLEND_CTRLS_TREE)
    return PID_ANIM_BLEND_CTRLS_FILTER;
  else if (tree_pid == PID_ANIM_STATES_TREE)
    return PID_ANIM_STATES_FILTER;
  else if (tree_pid == PID_ANIM_BLEND_NODES_TREE)
    return PID_ANIM_BLEND_NODES_FILTER;
  else if (tree_pid == PID_NODE_MASKS_FILTER)
    return PID_NODE_MASKS_TREE;

  G_ASSERT_FAIL("Unknown tree pid %d, can't return filter pid", tree_pid);
  return -1;
}

void focus_selected_node(PropPanel::ControlEventHandler *plugin_event_handler, PropPanel::ContainerPropertyControl *plugin_panel,
  PropPanel::TLeafHandle selected_leaf, int group_pid, int focus_panel_pid, int tree_pid)
{
  PropPanel::ContainerPropertyControl *tree = plugin_panel->getById(tree_pid)->getContainer();
  // Check if group minimized and open it
  if (plugin_panel->getBool(group_pid))
    plugin_panel->setBool(group_pid, false);
  const int filterPid = get_filter_pid_by_tree(tree_pid);
  SimpleString filter = plugin_panel->getText(filterPid);
  if (!filter.empty() && !strstr(tree->getCaption(selected_leaf), filter))
  {
    plugin_panel->setText(filterPid, "");
    plugin_event_handler->onChange(filterPid, plugin_panel);
  }
  tree->setSelLeaf(selected_leaf);
  PropPanel::focus_helper.requestFocus(plugin_panel->getById(focus_panel_pid)->getContainer());
  plugin_event_handler->onChange(tree_pid, plugin_panel);
}

eastl::string_view get_node_mask_suffix_from_name(eastl::string_view name)
{
  size_t pos = name.find(':');
  if (pos != eastl::string_view::npos)
    return name.substr(pos);

  return "";
}

bool get_updated_child_name(const char *new_name, const String &old_name, const char *child_name, String &write_name)
{
  if (old_name == child_name)
  {
    write_name = new_name;
    return true;
  }
  else if (name_without_node_mask_suffix(old_name.c_str()) == child_name)
  {
    eastl::string_view view = name_without_node_mask_suffix(new_name);
    write_name = String(view.data(), view.length());
    return true;
  }
  else if (eastl::string_view view = name_without_node_mask_suffix(child_name); old_name == String(view.data(), view.length()))
  {
    write_name = String(0, "%s%s", new_name, get_node_mask_suffix_from_name(child_name));
    return true;
  }

  return false;
}

// Need make convertation becuse settings block can contain blocks with comments before selected block
int get_child_block_idx_by_list_idx(const DataBlock *settings, int list_idx)
{
  int childIdx = -1;
  for (int i = 0; i < settings->blockCount(); ++i)
  {
    // Here we want count only child block with name param
    if (settings->getBlock(i)->paramExists("name"))
    {
      ++childIdx;
      if (childIdx == list_idx)
        return i;
    }
  }

  // In case when block not already created
  return -1;
}

int get_param_name_idx_by_list_name(const DataBlock &settings, const SimpleString &list_name, int selected_idx)
{
  for (int i = selected_idx; i < settings.paramCount(); ++i)
    if (list_name == settings.getParamName(i))
      return i;

  return -1;
}

void fill_child_names(Tab<String> &child_names, PropPanel::ContainerPropertyControl *panel, dag::ConstSpan<BlendNodeData> blend_nodes,
  dag::ConstSpan<AnimCtrlData> controllers)
{
  PropPanel::ContainerPropertyControl *blendNodesTree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  auto addChildName = [&child_names](PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle handle) {
    const String childNameRaw = tree->getCaption(handle);
    eastl::string_view childName = name_without_node_mask_suffix(childNameRaw.c_str());
    child_names.emplace_back(String(childName.data(), childName.length()));
  };
  for (const BlendNodeData &data : blend_nodes)
    if (data.type == BlendNodeType::CONTINUOUS || data.type == BlendNodeType::SINGLE || data.type == BlendNodeType::PARAMETRIC ||
        data.type == BlendNodeType::STILL)
      addChildName(blendNodesTree, data.handle);
  for (const AnimCtrlData &data : controllers)
    if (data.type != ctrl_type_include && data.type != ctrl_type_inner_include && data.type != ctrl_type_not_found)
      addChildName(ctrlsTree, data.handle);
}

void fill_param_names_without_comments(Tab<String> &names, const DataBlock &settings)
{
  names.reserve(settings.paramCount());
  for (int i = 0; i < settings.paramCount(); ++i)
    if (!CHECK_COMMENT_PREFIX(settings.getParamName(i)))
      names.emplace_back(settings.getParamName(i));
}

int get_selected_name_idx_combo(Tab<String> &names, const char *name)
{
  int index = PropPanel::ImguiHelper::getStringIndexInTab(names, name);
  if (index == -1 && name && !*name) // Can't find empty name in list becuse for empty ImGui uses "##"
    index = 0;
  else if (index == -1) // Add value to Tab if not found for display it in control
  {
    names.emplace_back(String(name));
    index = names.size() - 1;
  }
  return index;
}

SimpleString get_text_from_combo(PropPanel::ContainerPropertyControl *panel, int pid)
{
  SimpleString value = panel->getText(pid);
  if (value == "##")
    return SimpleString("");

  return value;
}

template <typename SwapFunc>
void move_item_blk(DataBlock &blk, int from, int to, SwapFunc swapFunc)
{
  if (from == to)
    return;

  int curPos = from;
  while (curPos != to)
  {
    int nextPos = curPos < to ? curPos + 1 : curPos - 1;
    swapFunc(blk, curPos, nextPos);
    curPos = nextPos;
  }
}

void move_param_blk(DataBlock &blk, int from, int to)
{
  move_item_blk(blk, from, to, [](DataBlock &b, int from, int to) { b.swapParams(from, to); });
}

void move_block_blk(DataBlock &blk, int from, int to)
{
  move_item_blk(blk, from, to, [](DataBlock &b, int from, int to) { b.swapBlocks(from, to); });
}

bool is_comp_op_needs_p1(const char *op) { return strcmp(op, "inside") == 0 || strcmp(op, "outside") == 0 || strcmp(op, "dist") == 0; }

CtrlType get_selected_ctrl_type(PropPanel::ContainerPropertyControl *panel, bool is_proc_child)
{
  if (is_proc_child)
  {
    SimpleString name = panel->getText(PID_CTRLS_TYPE_COMBO_SELECT);
    return static_cast<CtrlType>(lup(name.c_str(), ctrl_type, ctrl_type_not_found));
  }
  return static_cast<CtrlType>(panel->getInt(PID_CTRLS_TYPE_COMBO_SELECT));
}
