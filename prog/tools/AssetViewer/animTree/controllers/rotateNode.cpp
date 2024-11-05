// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rotateNode.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "../animTree.h"

static const char *SWAP_Y_Z_PARAM = "swapYZ";
static const char *RELATIVE_ROR_PARAM = "relativeRot";
static const float DEFAULT_K_MUL = 1.0f;
static const float DEFAULT_TARGET_NODE_WT = 1.0f;
static const int DEFAULT_AXIS_FROM_CHAR_INDEX = -1;
static const Point3 DEFAULT_ROT_AXIS = Point3(0.f, 1.f, 0.f);
static const Point3 DEFAULT_DIR_AXIS = Point3(0.f, 0.f, 1.f);

void rotate_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_float_if_not_exists(params, panel, field_idx, "targetNodeWt", DEFAULT_TARGET_NODE_WT);
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "param");
  add_edit_box_if_not_exists(params, panel, field_idx, "axis_course");
  add_edit_float_if_not_exists(params, panel, field_idx, "kCourseAdd");
  add_edit_float_if_not_exists(params, panel, field_idx, "kMul", DEFAULT_K_MUL);
  add_edit_float_if_not_exists(params, panel, field_idx, "kAdd");
  add_edit_float_if_not_exists(params, panel, field_idx, "kMul2", DEFAULT_K_MUL);
  add_edit_float_if_not_exists(params, panel, field_idx, "kAdd2");
  add_edit_bool_if_not_exists(params, panel, field_idx, "saveScale");
  add_edit_bool_if_not_exists(params, panel, field_idx, "inRadian");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramAdd");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramAdd2");
  add_edit_point3_if_not_exists(params, panel, field_idx, "rotAxis", DEFAULT_ROT_AXIS);
  add_edit_point3_if_not_exists(params, panel, field_idx, "dirAxis", DEFAULT_DIR_AXIS);
  add_edit_bool_if_not_exists(params, panel, field_idx, SWAP_Y_Z_PARAM);
  add_edit_bool_if_not_exists(params, panel, field_idx, RELATIVE_ROR_PARAM);
  const bool swapYZValue = get_bool_param_by_name(params, panel, SWAP_Y_Z_PARAM);
  const bool relativeRotValue = get_bool_param_by_name(params, panel, RELATIVE_ROR_PARAM);
  const bool defaultLeftTm = !swapYZValue && !relativeRotValue;
  add_edit_bool_if_not_exists(params, panel, field_idx, "leftTm", defaultLeftTm);
  add_edit_int_if_not_exists(params, panel, field_idx, "axisFromCharIndex", DEFAULT_AXIS_FROM_CHAR_INDEX);
  add_edit_box_if_not_exists(params, panel, field_idx, "accept_name_mask_re");
  add_edit_box_if_not_exists(params, panel, field_idx, "decline_name_mask_re");

  // Here we want add for each targetNode targetNodeWtN
  add_params_target_node_wt(params, panel, field_idx);

  int targetNodeCount = 0;
  for (const AnimParamData &param : params)
    if (param.name == "targetNode")
      ++targetNodeCount;

  panel->createButton(PID_CTRLS_TARGET_NODE_ADD, "Add target node");
  panel->createButton(PID_CTRLS_TARGET_NODE_REMOVE, "Remove last target node", targetNodeCount != 1);
}

void rotate_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const bool swapYZValue = get_bool_param_by_name(params, panel, SWAP_Y_Z_PARAM);
  const bool relativeRotValue = get_bool_param_by_name(params, panel, RELATIVE_ROR_PARAM);
  const bool defaultLeftTm = !swapYZValue && !relativeRotValue;

  remove_params_if_default_target_node_wt(params, panel);
  remove_param_if_default_float(params, panel, "targetNodeWt", DEFAULT_TARGET_NODE_WT);
  remove_param_if_default_str(params, panel, "axis_course");
  remove_param_if_default_float(params, panel, "kCourseAdd");
  remove_param_if_default_float(params, panel, "kMul", DEFAULT_K_MUL);
  remove_param_if_default_float(params, panel, "kAdd");
  remove_param_if_default_float(params, panel, "kMul2", DEFAULT_K_MUL);
  remove_param_if_default_float(params, panel, "kAdd2");
  remove_param_if_default_bool(params, panel, "saveScale");
  remove_param_if_default_bool(params, panel, "inRadian");
  remove_param_if_default_str(params, panel, "paramAdd");
  remove_param_if_default_str(params, panel, "paramAdd2");
  remove_param_if_default_point3(params, panel, "rotAxis", DEFAULT_ROT_AXIS);
  remove_param_if_default_point3(params, panel, "dirAxis", DEFAULT_DIR_AXIS);
  remove_param_if_default_bool(params, panel, SWAP_Y_Z_PARAM);
  remove_param_if_default_bool(params, panel, RELATIVE_ROR_PARAM);
  remove_param_if_default_bool(params, panel, "leftTm", defaultLeftTm);
  remove_param_if_default_int(params, panel, "axisFromCharIndex", DEFAULT_AXIS_FROM_CHAR_INDEX);
  remove_param_if_default_str(params, panel, "accept_name_mask_re");
  remove_param_if_default_str(params, panel, "decline_name_mask_re");
}
