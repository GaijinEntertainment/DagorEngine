// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "parametric.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include <util/dag_string.h>
#include <util/dag_simpleString.h>

const float DEFAULT_P_END = 1.0f;
const float DEFAULT_MULK = 1.0f;

void parametric_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx,
  bool default_foreign)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "varname");
  add_edit_box_if_not_exists(params, panel, field_idx, "key");
  add_edit_box_if_not_exists(params, panel, field_idx, "key_start");
  add_edit_box_if_not_exists(params, panel, field_idx, "key_end");
  add_edit_float_if_not_exists(params, panel, field_idx, "p_start");
  add_edit_float_if_not_exists(params, panel, field_idx, "p_end", DEFAULT_P_END);
  add_edit_bool_if_not_exists(params, panel, field_idx, "looping");
  add_edit_box_if_not_exists(params, panel, field_idx, "apply_node_mask");
  add_edit_bool_if_not_exists(params, panel, field_idx, "disable_origin_vel");
  add_edit_bool_if_not_exists(params, panel, field_idx, "foreignAnimation", default_foreign);
  add_edit_float_if_not_exists(params, panel, field_idx, "addMoveDirH");
  add_edit_float_if_not_exists(params, panel, field_idx, "addMoveDirV");
  add_edit_float_if_not_exists(params, panel, field_idx, "addMoveDist");
  add_edit_float_if_not_exists(params, panel, field_idx, "mulk", DEFAULT_MULK);
  add_edit_float_if_not_exists(params, panel, field_idx, "addk");
}

void parametric_set_dependent_defaults(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
  set_str_param_by_name_if_default(params, panel, "key", nameValue.c_str());
  const SimpleString keyValue = get_str_param_by_name_optional(params, panel, "key");
  const String defaultKeyStart(0, "%s_start", keyValue.c_str());
  const String defaultKeyEnd(0, "%s_end", keyValue.c_str());
  set_str_param_by_name_if_default(params, panel, "key_start", defaultKeyStart.c_str());
  set_str_param_by_name_if_default(params, panel, "key_end", defaultKeyEnd.c_str());
}

void parametric_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, bool default_foreign)
{
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
  const SimpleString keyValue = get_str_param_by_name_optional(params, panel, "key");
  const String defaultKeyStart(0, "%s_start", keyValue.c_str());
  const String defaultKeyEnd(0, "%s_end", keyValue.c_str());

  remove_param_if_default_str(params, panel, "key", nameValue.c_str());
  remove_param_if_default_str(params, panel, "key_start", defaultKeyStart.c_str());
  remove_param_if_default_str(params, panel, "key_end", defaultKeyEnd.c_str());
  remove_param_if_default_float(params, panel, "p_start");
  remove_param_if_default_float(params, panel, "p_end", DEFAULT_P_END);
  remove_param_if_default_bool(params, panel, "looping");
  remove_param_if_default_str(params, panel, "apply_node_mask");
  remove_param_if_default_bool(params, panel, "disable_origin_vel");
  remove_param_if_default_bool(params, panel, "foreignAnimation", default_foreign);
  remove_param_if_default_float(params, panel, "addMoveDirH");
  remove_param_if_default_float(params, panel, "addMoveDirV");
  remove_param_if_default_float(params, panel, "addMoveDist");
  remove_param_if_default_float(params, panel, "mulk", DEFAULT_MULK);
  remove_param_if_default_float(params, panel, "addk");
}
