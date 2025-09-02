// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "eyeCtrl.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"

void eye_ctrl_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_float_if_not_exists(params, panel, field_idx, "horizontal_reaction_factor");
  add_edit_float_if_not_exists(params, panel, field_idx, "blinking_reaction_factor");
  add_edit_point2_if_not_exists(params, panel, field_idx, "vertical_reaction_factor");
  add_edit_box_if_not_exists(params, panel, field_idx, "eye_direction");
  add_edit_box_if_not_exists(params, panel, field_idx, "eyelid_start_top");
  add_edit_box_if_not_exists(params, panel, field_idx, "eyelid_start_bottom");
  add_edit_box_if_not_exists(params, panel, field_idx, "eyelids_horizontal");
  add_edit_box_if_not_exists(params, panel, field_idx, "eyelid_vertical_top");
  add_edit_box_if_not_exists(params, panel, field_idx, "eyelid_vertical_bottom");
  add_edit_box_if_not_exists(params, panel, field_idx, "blink_source");
  add_edit_box_if_not_exists(params, panel, field_idx, "eyelid_blink_top");
  add_edit_box_if_not_exists(params, panel, field_idx, "eyelid_blink_bottom");
}

void eye_ctrl_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_float(params, panel, "horizontal_reaction_factor");
  remove_param_if_default_float(params, panel, "blinking_reaction_factor");
  remove_param_if_default_point2(params, panel, "vertical_reaction_factor");
  remove_param_if_default_str(params, panel, "eye_direction");
  remove_param_if_default_str(params, panel, "eyelid_start_top");
  remove_param_if_default_str(params, panel, "eyelid_start_bottom");
  remove_param_if_default_str(params, panel, "eyelids_horizontal");
  remove_param_if_default_str(params, panel, "eyelid_vertical_top");
  remove_param_if_default_str(params, panel, "eyelid_vertical_bottom");
  remove_param_if_default_str(params, panel, "blink_source");
  remove_param_if_default_str(params, panel, "eyelid_blink_top");
  remove_param_if_default_str(params, panel, "eyelid_blink_bottom");
}
