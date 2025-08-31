// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <generic/dag_span.h>
#include <propPanel/c_common.h>
#include <EASTL/string_view.h>

namespace PropPanel
{
class ContainerPropertyControl;
class ControlEventHandler;
} // namespace PropPanel

struct AnimParamData;
struct AnimCtrlData;
struct BlendNodeData;
struct AnimStatesData;
class DataBlock;
class String;
class DagorAsset;
class DagorAssetMgr;

bool is_include_ctrl(const AnimCtrlData &data);

const AnimParamData *find_param_by_name(const dag::Vector<AnimParamData> &params, const char *name);
AnimParamData *find_param_by_name(dag::Vector<AnimParamData> &params, const char *name);
template <typename T>
T *find_data_by_handle(dag::Vector<T> &data_vec, PropPanel::TLeafHandle handle)
{
  return eastl::find_if(data_vec.begin(), data_vec.end(), [handle](const T &data) { return data.handle == handle; });
}
template <typename T>
const T *find_data_by_handle(dag::ConstSpan<T> &data_vec, PropPanel::TLeafHandle handle)
{
  return eastl::find_if(data_vec.begin(), data_vec.end(), [handle](const T &data) { return data.handle == handle; });
}

// Returns nullptr if not found
template <typename Data, typename Type>
Data *find_data_by_type(dag::Vector<Data> &data_vec, Type type)
{
  Data *data = eastl::find_if(data_vec.begin(), data_vec.end(), [type](const Data &data) { return data.type == type; });
  if (data != data_vec.end())
    return data;
  else
    return nullptr;
}

bool add_edit_box_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, const char *default_value = "");
bool add_edit_float_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, float default_value = 0.f);
bool add_edit_int_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, int default_value = 0);
bool add_edit_point2_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, Point2 default_value = Point2(0.f, 0.f));
bool add_edit_point3_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, Point3 default_value = Point3(0.f, 0.f, 0.f));
bool add_edit_bool_if_not_exists(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid,
  const char *name, bool default_value = false);
void add_target_node(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
// Add for each target node targetNodeWtN if not exists
void add_params_target_node_wt(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int &pid);

bool get_bool_param_by_name(const dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name);
bool get_bool_param_by_name_optional(const dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel,
  const char *name, bool default_value = false);
float get_float_param_by_name_optional(const dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel,
  const char *name, float default_value = 0.f);
int get_int_param_by_name_optional(const dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel,
  const char *name, int default_value = 0);
SimpleString get_str_param_by_name_optional(const dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel,
  const char *name, const char *default_value = "");

void set_str_param_by_name_if_default(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  const char *str, const char *default_value = "");
void set_float_param_by_name_if_default(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel,
  const char *name, float set_value, float default_value = 0.f);

void remove_param_if_default_float(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  float default_value = 0.f);
void remove_param_if_default_int(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  int default_value = 0);
void remove_param_if_default_str(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  const char *default_value = "");
void remove_param_if_default_point2(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  Point2 default_value = Point2(0.f, 0.f));
void remove_param_if_default_point3(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  Point3 default_value = Point3(0.f, 0.f, 0.f));
void remove_param_if_default_bool(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const char *name,
  bool default_value = false);
void remove_params_if_default_target_node_wt(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void remove_fields(PropPanel::ContainerPropertyControl *panel, int field_start, int field_end, bool break_if_not_found = false);
void remove_nodes_fields(PropPanel::ContainerPropertyControl *panel);
void remove_ctrls_fields(PropPanel::ContainerPropertyControl *panel, int last_param_pid);
void remove_target_node(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

DataBlock *find_block_by_name(DataBlock *props, const String &name, bool should_exist = true);
DataBlock *find_block_by_block_name(DataBlock *props, const String &name, bool should_exist = true);
DataBlock *find_blend_node_settings(DataBlock &props, const char *name);
DataBlock *find_a2d_node_settings(DataBlock &props, const char *name);

String get_folder_path_based_on_parent(dag::ConstSpan<String> paths, const DagorAsset &asset,
  PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle include_leaf);
String find_full_path(dag::ConstSpan<String> paths, const DagorAsset &asset, PropPanel::ContainerPropertyControl *tree,
  PropPanel::TLeafHandle include_leaf);
String find_folder_path(dag::ConstSpan<String> paths, const String &name);
String find_resolved_folder_path(dag::ConstSpan<String> paths, const DagorAsset &asset, PropPanel::ContainerPropertyControl *tree,
  PropPanel::TLeafHandle include_leaf);

// use force_parent if for selected include need parent include props
DataBlock get_props_from_include_leaf(dag::ConstSpan<String> paths, const DagorAsset &asset, PropPanel::ContainerPropertyControl *tree,
  PropPanel::TLeafHandle include_leaf, String &full_path, bool only_includes = false);
DataBlock get_props_from_include_state_data(dag::ConstSpan<String> paths, dag::ConstSpan<AnimCtrlData> ctrls, const DagorAsset &asset,
  PropPanel::ContainerPropertyControl *ctrls_tree, const AnimStatesData &state_data, String &full_path, bool only_includes = false);
DataBlock *get_props_from_include_leaf_ctrl_node(dag::ConstSpan<AnimCtrlData> ctrls, dag::ConstSpan<String> paths,
  const DagorAsset &asset, DataBlock &props, PropPanel::ContainerPropertyControl *tree, PropPanel::TLeafHandle include_leaf);

struct CtrlChildSearchResult
{
  int id;
  const char *iconName;
};

eastl::string_view name_without_node_mask_suffix(eastl::string_view str);
CtrlChildSearchResult find_child_idx_and_icon_by_name(PropPanel::ContainerPropertyControl *ctrls_tree,
  PropPanel::ContainerPropertyControl *nodes_tree, PropPanel::TLeafHandle parent_handle, dag::ConstSpan<AnimCtrlData> controllers,
  dag::ConstSpan<BlendNodeData> blend_nodes, const char *name);
CtrlChildSearchResult find_child_idx_and_icon_by_name(PropPanel::ContainerPropertyControl *panel, PropPanel::TLeafHandle parent_handle,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name);
int find_child_idx_by_name(PropPanel::ContainerPropertyControl *panel, PropPanel::TLeafHandle parent_handle,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name);
int find_state_child_idx_by_name(PropPanel::ContainerPropertyControl *panel, PropPanel::TLeafHandle parent_handle,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name);
int add_ctrl_child_idx_by_name(PropPanel::ContainerPropertyControl *panel, AnimCtrlData &data,
  dag::ConstSpan<AnimCtrlData> controllers, dag::ConstSpan<BlendNodeData> blend_nodes, const char *name);
void check_ctrl_child_idx(int idx, const char *ctrl_name, const char *child_name, bool is_optional = false);
void check_state_child_idx(int idx, const char *state_name, const char *child_name);

float get_default_anim_time(const char *a2d_name, const DagorAssetMgr &mgr);
void focus_selected_node(PropPanel::ControlEventHandler *plugin_event_handler, PropPanel::ContainerPropertyControl *plugin_panel,
  PropPanel::TLeafHandle selected_leaf, int group_pid, int focus_panel_pid, int tree_pid);

bool get_updated_child_name(const char *new_name, const String &old_name, const char *child_name, String &write_name);
eastl::string_view get_node_mask_suffix_from_name(eastl::string_view name);

int get_child_block_idx_by_list_idx(const DataBlock *settings, int list_idx);

void fill_child_names(Tab<String> &child_names, PropPanel::ContainerPropertyControl *panel, dag::ConstSpan<BlendNodeData> blend_nodes,
  dag::ConstSpan<AnimCtrlData> controllers);
// names must have empty string as 0 element
int get_selected_name_idx_combo(Tab<String> &names, const char *name);
