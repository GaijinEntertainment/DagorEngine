// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "still.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include <util/dag_simpleString.h>

const int DEFAULT_START = -1;

void still_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx,
  bool default_foreign)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "key");
  add_edit_int_if_not_exists(params, panel, field_idx, "start", DEFAULT_START);
  add_edit_box_if_not_exists(params, panel, field_idx, "apply_node_mask");
  add_edit_bool_if_not_exists(params, panel, field_idx, "disable_origin_vel");
  add_edit_bool_if_not_exists(params, panel, field_idx, "foreignAnimation", default_foreign);
  add_edit_float_if_not_exists(params, panel, field_idx, "addMoveDirH");
  add_edit_float_if_not_exists(params, panel, field_idx, "addMoveDirV");
  add_edit_float_if_not_exists(params, panel, field_idx, "addMoveDist");
}

void still_set_dependent_defaults(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
  set_str_param_by_name_if_default(params, panel, "key", nameValue.c_str());
}

void still_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, bool default_foreign)
{
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");

  remove_param_if_default_str(params, panel, "key", nameValue.c_str());
  remove_param_if_default_int(params, panel, "start", DEFAULT_START);
  remove_param_if_default_str(params, panel, "apply_node_mask");
  remove_param_if_default_bool(params, panel, "disable_origin_vel");
  remove_param_if_default_bool(params, panel, "foreignAnimation", default_foreign);
  remove_param_if_default_float(params, panel, "addMoveDirH");
  remove_param_if_default_float(params, panel, "addMoveDirV");
  remove_param_if_default_float(params, panel, "addMoveDist");
}
