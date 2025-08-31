// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <util/dag_string.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;
struct AnimCtrlData;
struct BlendNodeData;
class DataBlock;

inline const Tab<String> param_switch_type({String("Generated from enum"), String("Enum list")});
enum ParamSwitchType
{
  PARAM_SWITCH_TYPE_ENUM_GEN,
  PARAM_SWITCH_TYPE_ENUM_LIST,
};

void param_switch_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void param_switch_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void param_switch_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void param_switch_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
const char *param_switch_get_child_name_by_idx(const DataBlock &settings, int idx);
String param_switch_get_enum_gen_child_name_by_idx(const DataBlock &enum_root, const char *name, int idx);
bool param_switch_get_child_is_optional_by_idx(const DataBlock &settings, int idx);
String param_switch_get_child_prefix_name(const DataBlock &settings, int idx, const DataBlock &enum_root_props);
void param_switch_update_child_name(DataBlock &settings, const char *name, const String &old_name);
void param_switch_update_enum_gen_child_name(DataBlock &enum_props, const char *controller_name, const char *name,
  const String &old_name, dag::Vector<int> &dependent_items);
