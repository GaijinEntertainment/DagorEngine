// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "moveNode.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "../animTree.h"

static const char *LOCAL_ADDITIVE_PARAM = "localAdditive";
static const char *LOCAL_ADDITIVE_2_PARAM = "localAdditive2";
static const float DEFAULT_TARGET_NODE_WT = 1.0f;
static const float DEFAULT_K_MUL = 1.0f;
static const Point3 DEFAULT_ROT_AXIS = Point3(0.f, 1.f, 0.f);
static const Point3 DEFAULT_DIR_AXIS = Point3(0.f, 0.f, 1.f);

void move_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_float_if_not_exists(params, panel, field_idx, "targetNodeWt", DEFAULT_TARGET_NODE_WT);
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "param");
  add_edit_box_if_not_exists(params, panel, field_idx, "axis_course");
  add_edit_float_if_not_exists(params, panel, field_idx, "kCourseAdd");
  add_edit_float_if_not_exists(params, panel, field_idx, "kMul", DEFAULT_K_MUL);
  add_edit_float_if_not_exists(params, panel, field_idx, "kAdd");
  add_edit_point3_if_not_exists(params, panel, field_idx, "rotAxis", DEFAULT_ROT_AXIS);
  add_edit_point3_if_not_exists(params, panel, field_idx, "dirAxis", DEFAULT_DIR_AXIS);
  add_edit_bool_if_not_exists(params, panel, field_idx, LOCAL_ADDITIVE_PARAM);
  { // additive param default value depends on value in localAdditive param
    const bool localAdditiveValue = get_bool_param_by_name(params, panel, LOCAL_ADDITIVE_PARAM);
    add_edit_bool_if_not_exists(params, panel, field_idx, "additive", localAdditiveValue);
  }
  add_edit_bool_if_not_exists(params, panel, field_idx, LOCAL_ADDITIVE_2_PARAM);
  { // saveOtherAxisMove param default value depends on value in localAdditive2 param
    const bool localAdditive2Value = get_bool_param_by_name(params, panel, LOCAL_ADDITIVE_2_PARAM);
    add_edit_bool_if_not_exists(params, panel, field_idx, "saveOtherAxisMove", localAdditive2Value);
  }
  add_edit_box_if_not_exists(params, panel, field_idx, "accept_name_mask_re");
  add_edit_box_if_not_exists(params, panel, field_idx, "decline_name_mask_re");
  add_params_target_node_wt(params, panel, field_idx);

  int targetNodeCount = 0;
  for (const AnimParamData &param : params)
    if (param.name == "targetNode")
      ++targetNodeCount;

  panel->createButton(PID_CTRLS_TARGET_NODE_ADD, "Add target node");
  panel->createButton(PID_CTRLS_TARGET_NODE_REMOVE, "Remove last target node", targetNodeCount != 1);
}

void move_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  const bool localAdditiveValue = get_bool_param_by_name(params, panel, LOCAL_ADDITIVE_PARAM);
  const bool localAdditive2Value = get_bool_param_by_name(params, panel, LOCAL_ADDITIVE_2_PARAM);

  remove_params_if_default_target_node_wt(params, panel);
  remove_param_if_default_float(params, panel, "targetNodeWt", DEFAULT_TARGET_NODE_WT);
  remove_param_if_default_str(params, panel, "axis_course");
  remove_param_if_default_float(params, panel, "kCourseAdd");
  remove_param_if_default_float(params, panel, "kMul", DEFAULT_K_MUL);
  remove_param_if_default_float(params, panel, "kAdd");
  remove_param_if_default_point3(params, panel, "rotAxis", DEFAULT_ROT_AXIS);
  remove_param_if_default_point3(params, panel, "dirAxis", DEFAULT_DIR_AXIS);
  remove_param_if_default_bool(params, panel, LOCAL_ADDITIVE_PARAM);
  remove_param_if_default_bool(params, panel, "additive", localAdditiveValue);
  remove_param_if_default_bool(params, panel, LOCAL_ADDITIVE_2_PARAM);
  remove_param_if_default_bool(params, panel, "saveOtherAxisMove", localAdditive2Value);
  remove_param_if_default_str(params, panel, "accept_name_mask_re");
  remove_param_if_default_str(params, panel, "decline_name_mask_re");
}
