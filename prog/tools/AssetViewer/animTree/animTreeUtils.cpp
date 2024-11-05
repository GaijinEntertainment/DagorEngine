// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animTreeUtils.h"
#include "animTreePanelPids.h"
#include "animParamData.h"
#include "controllers/ctrlType.h"
#include "controllers/animCtrlData.h"
#include "blendNodes/blendNodeType.h"
#include "blendNodes/blendNodeData.h"

#include <osApiWrappers/dag_direct.h>
#include <propPanel/control/container.h>
#include <assets/asset.h>

bool is_include_ctrl(const AnimCtrlData &data) { return data.type == ctrl_type_include || data.type == ctrl_type_inner_include; }

const AnimParamData *find_param_by_name(const dag::Vector<AnimParamData> &params, const char *name)
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

void add_target_node(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const char *targetNodeParam = "targetNode";
  const float defaultTargetNodeWt = 1.f;
  const float targetNodeWtValue = get_float_param_by_name_optional(params, panel, "targetNodeWt", defaultTargetNodeWt);
  PropPanel::ContainerPropertyControl *group = panel->getById(PID_ANIM_BLEND_CTRLS_SETTINGS_GROUP)->getContainer();
  int freePid = params.back().pid + 1;
  int targetNodeCount = 0;
  for (const AnimParamData &param : params)
    if (param.name == targetNodeParam)
      ++targetNodeCount;

  auto targetNodeWtLast = find_param_by_name(params, String(0, "targetNodeWt%d", targetNodeCount));
  G_ASSERTF(targetNodeWtLast != params.end(), "Can't find node <targetNodeWt%d> for move new node", targetNodeCount);

  group->createEditBox(freePid, targetNodeParam);
  params.emplace_back(AnimParamData{freePid, DataBlock::TYPE_STRING, String(targetNodeParam)});
  group->moveById(freePid, targetNodeWtLast->pid, /*after*/ true);

  freePid = freePid + 1;
  group->createEditFloat(freePid, String(0, "targetNodeWt%d", targetNodeCount + 1), targetNodeWtValue);
  params.emplace_back(AnimParamData{freePid, DataBlock::TYPE_REAL, String(0, "targetNodeWt%d", targetNodeCount + 1)});
  group->moveById(freePid, freePid - 1, /*after*/ true);

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

  G_ASSERTF(targetNodeLastIdx != -1, "Not found any <targetNode> param");

  group->removeById(params[targetNodeLastIdx].pid);
  params.erase(params.begin() + targetNodeLastIdx);

  auto targetNodeWtLast = find_param_by_name(params, String(0, "targetNodeWt%d", targetNodeCount));
  G_ASSERTF(targetNodeWtLast != params.end(), "Can't find node <targetNodeWt%d> for remove", targetNodeCount);
  group->removeById(targetNodeWtLast->pid);
  params.erase(targetNodeWtLast);

  if (targetNodeCount == 2)
    group->setEnabledById(PID_CTRLS_TARGET_NODE_REMOVE, false);
}

DataBlock *find_block_by_name(DataBlock *props, const String &name)
{
  for (int i = 0; i < props->blockCount(); ++i)
  {
    DataBlock *settings = props->getBlock(i);
    if (name == settings->getStr("name", ""))
      return settings;
  }

  G_ASSERT_FAIL("Can't find block with name <%s>", name);
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
  if (include_leaf == tree->getRootLeaf() && tree->getCaption(tree->getRootLeaf()) == "Root")
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

CtrlChildSearchResult find_ctrl_child_idx_and_icon_by_name(PropPanel::ContainerPropertyControl *ctrls_tree,
  PropPanel::ContainerPropertyControl *nodes_tree, const AnimCtrlData &data, dag::ConstSpan<AnimCtrlData> controllers,
  dag::ConstSpan<BlendNodeData> blend_nodes, const char *name)
{
  const AnimCtrlData *childCtrl = eastl::find_if(controllers.begin(), controllers.end(), [name, ctrls_tree](const AnimCtrlData &data) {
    return name_without_node_mask_suffix(ctrls_tree->getCaption(data.handle).c_str()) == name_without_node_mask_suffix(name);
  });
  if (childCtrl != controllers.end())
  {
    const int childIdx = eastl::distance(controllers.begin(), childCtrl);
    const int dataIdx = eastl::distance(controllers.begin(), &data);
    if (childIdx == dataIdx)
      logerr("Controller %s can't use self as child", ctrls_tree->getCaption(data.handle));
    // else if (childIdx > dataIdx)
    //   logerr("Child <%s> with position %d should be declared before controller <%s> with position %d",
    //     ctrls_tree->getCaption(childCtrl->handle), childIdx, ctrls_tree->getCaption(data.handle), dataIdx);

    return {childCtrl->id, get_ctrl_icon_name(childCtrl->type)};
  }
  else
  {
    const BlendNodeData *childNode =
      eastl::find_if(blend_nodes.begin(), blend_nodes.end(), [name, nodes_tree](const BlendNodeData &data) {
        return name_without_node_mask_suffix(nodes_tree->getCaption(data.handle).c_str()) == name_without_node_mask_suffix(name);
      });
    if (childNode != blend_nodes.end())
      return {childNode->id, get_blend_node_icon_name(childNode->type)};
    else
      logerr("can't find child idx for leaf <%s>, child <%s>", ctrls_tree->getCaption(data.handle), name);
  }

  return {-1, nullptr};
}

CtrlChildSearchResult find_ctrl_child_idx_and_icon_by_name(PropPanel::ContainerPropertyControl *panel, const AnimCtrlData &data,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name)
{
  PropPanel::ContainerPropertyControl *ctrlsTree = panel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
  PropPanel::ContainerPropertyControl *nodesTree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  return find_ctrl_child_idx_and_icon_by_name(ctrlsTree, nodesTree, data, controllers, blend_nodes, name);
}

int find_ctrl_child_idx_by_name(PropPanel::ContainerPropertyControl *panel, const AnimCtrlData &data,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name)
{
  return find_ctrl_child_idx_and_icon_by_name(panel, data, controllers, blend_nodes, name).id;
}

void add_ctrl_child_idx_by_name(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name)
{
  data.childs.emplace_back(find_ctrl_child_idx_by_name(panel, data, controllers, blend_nodes, name));
}
