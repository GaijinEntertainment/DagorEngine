// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rotateAroundNode.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "../animParamData.h"
#include <propPanel/control/container.h>

static const float DEFAULT_K_MUL = 1.0f;
static const float DEFAULT_TARGET_NODE_WT = 1.0f;
static const Point3 DEFAULT_ROT_AXIS = Point3(0.f, 1.f, 0.f);

void rotate_around_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_float_if_not_exists(params, panel, field_idx, "targetNodeWt", DEFAULT_TARGET_NODE_WT);
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "param");
  add_edit_float_if_not_exists(params, panel, field_idx, "kMul", DEFAULT_K_MUL);
  add_edit_float_if_not_exists(params, panel, field_idx, "kAdd");
  add_edit_bool_if_not_exists(params, panel, field_idx, "inRadian");
  add_edit_point3_if_not_exists(params, panel, field_idx, "rotAxis", DEFAULT_ROT_AXIS);

  add_params_target_node_wt(params, panel, field_idx);
  int targetNodeCount = 0;
  for (const AnimParamData &param : params)
    if (param.name == "targetNode")
      ++targetNodeCount;

  panel->createButton(PID_CTRLS_TARGET_NODE_ADD, "Add target node");
  panel->createButton(PID_CTRLS_TARGET_NODE_REMOVE, "Remove last target node", targetNodeCount != 1);
}

void rotate_around_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_params_if_default_target_node_wt(params, panel);
  remove_param_if_default_float(params, panel, "targetNodeWt", DEFAULT_TARGET_NODE_WT);
  remove_param_if_default_float(params, panel, "kMul", DEFAULT_K_MUL);
  remove_param_if_default_float(params, panel, "kAdd");
  remove_param_if_default_bool(params, panel, "inRadian");
  remove_param_if_default_point3(params, panel, "rotAxis", DEFAULT_ROT_AXIS);
}
