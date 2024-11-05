// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "continuous.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include <util/dag_string.h>
#include <util/dag_simpleString.h>

const int DEFAULT_START = -1;
const int DEFAULT_END = -1;
const float DEFAULT_TIME = 1.0f;
const bool DEFAULT_REWIND = true;

void continuous_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx,
  bool default_foreign)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_bool_if_not_exists(params, panel, field_idx, "own_timer");
  add_edit_bool_if_not_exists(params, panel, field_idx, "eoa_irq");
  add_edit_box_if_not_exists(params, panel, field_idx, "timeScaleParam");
  add_edit_box_if_not_exists(params, panel, field_idx, "timer");
  add_edit_box_if_not_exists(params, panel, field_idx, "apply_node_mask");
  add_edit_bool_if_not_exists(params, panel, field_idx, "disable_origin_vel");
  add_edit_box_if_not_exists(params, panel, field_idx, "key");
  add_edit_box_if_not_exists(params, panel, field_idx, "key_start");
  add_edit_box_if_not_exists(params, panel, field_idx, "key_end");
  add_edit_int_if_not_exists(params, panel, field_idx, "start", DEFAULT_START);
  add_edit_int_if_not_exists(params, panel, field_idx, "end", DEFAULT_END);
  add_edit_float_if_not_exists(params, panel, field_idx, "time", DEFAULT_TIME);
  add_edit_float_if_not_exists(params, panel, field_idx, "moveDist");
  add_edit_box_if_not_exists(params, panel, field_idx, "sync_time");
  add_edit_bool_if_not_exists(params, panel, field_idx, "additive");
  add_edit_bool_if_not_exists(params, panel, field_idx, "foreignAnimation", default_foreign);
  add_edit_float_if_not_exists(params, panel, field_idx, "addMoveDirH");
  add_edit_float_if_not_exists(params, panel, field_idx, "addMoveDirV");
  add_edit_float_if_not_exists(params, panel, field_idx, "addMoveDist");
  add_edit_bool_if_not_exists(params, panel, field_idx, "rewind", DEFAULT_REWIND);
  add_edit_bool_if_not_exists(params, panel, field_idx, "rand_start");
}

void continuous_set_dependent_defaults(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
  set_str_param_by_name_if_default(params, panel, "key", nameValue.c_str());
  const SimpleString keyValue = get_str_param_by_name_optional(params, panel, "key");
  const String defaultKeyStart(0, "%s_start", keyValue.c_str());
  const String defaultKeyEnd(0, "%s_end", keyValue.c_str());
  set_str_param_by_name_if_default(params, panel, "key_start", defaultKeyStart.c_str());
  set_str_param_by_name_if_default(params, panel, "key_end", defaultKeyEnd.c_str());
  const SimpleString keyStartValue = get_str_param_by_name_optional(params, panel, "key_start");
  set_str_param_by_name_if_default(params, panel, "sync_time", keyStartValue.c_str());
}

void continuous_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, bool default_foreign)
{
  const SimpleString nameValue = get_str_param_by_name_optional(params, panel, "name");
  const SimpleString keyValue = get_str_param_by_name_optional(params, panel, "key", nameValue.c_str());
  const String defaultKeyStart(0, "%s_start", keyValue.c_str());
  const String defaultKeyEnd(0, "%s_end", keyValue.c_str());
  const SimpleString keyStartValue = get_str_param_by_name_optional(params, panel, "key_start", defaultKeyStart.c_str());

  remove_param_if_default_bool(params, panel, "own_timer");
  remove_param_if_default_bool(params, panel, "eoa_irq");
  remove_param_if_default_str(params, panel, "timeScaleParam");
  remove_param_if_default_str(params, panel, "timer");
  remove_param_if_default_str(params, panel, "apply_node_mask");
  remove_param_if_default_bool(params, panel, "disable_origin_vel");
  remove_param_if_default_str(params, panel, "key", nameValue.c_str());
  remove_param_if_default_str(params, panel, "key_start", defaultKeyStart.c_str());
  remove_param_if_default_str(params, panel, "key_end", defaultKeyEnd.c_str());
  remove_param_if_default_int(params, panel, "start", DEFAULT_START);
  remove_param_if_default_int(params, panel, "end", DEFAULT_END);
  remove_param_if_default_float(params, panel, "time", DEFAULT_TIME);
  remove_param_if_default_float(params, panel, "moveDist");
  remove_param_if_default_str(params, panel, "sync_time", keyStartValue.c_str());
  remove_param_if_default_bool(params, panel, "additive");
  remove_param_if_default_bool(params, panel, "foreignAnimation", default_foreign);
  remove_param_if_default_float(params, panel, "addMoveDirH");
  remove_param_if_default_float(params, panel, "addMoveDirV");
  remove_param_if_default_float(params, panel, "addMoveDist");
  remove_param_if_default_bool(params, panel, "rewind", DEFAULT_REWIND);
  remove_param_if_default_bool(params, panel, "rand_start");
}
