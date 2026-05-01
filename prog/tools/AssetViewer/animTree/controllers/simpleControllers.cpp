// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "simpleControllers.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"
#include "../animParamData.h"
#include <propPanel/control/container.h>

namespace AlignEx
{
static const float DEFAULT_TARGET_NODE_WT = 1.0f;
static const char *DEFAULT_HLP_X = "x";
static const char *DEFAULT_HLP_Y = "y";
static const char *DEFAULT_HLP_Z = "z";
} // namespace AlignEx

void align_ex_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "hlpNode");
  add_edit_float_if_not_exists(params, panel, field_idx, "targetNodeWt", AlignEx::DEFAULT_TARGET_NODE_WT);
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "hlpX", AlignEx::DEFAULT_HLP_X);
  add_edit_box_if_not_exists(params, panel, field_idx, "hlpY", AlignEx::DEFAULT_HLP_Y);
  add_edit_box_if_not_exists(params, panel, field_idx, "hlpZ", AlignEx::DEFAULT_HLP_Z);
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

void align_ex_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_params_if_default_target_node_wt(params, panel);
  remove_param_if_default_float(params, panel, "targetNodeWt", AlignEx::DEFAULT_TARGET_NODE_WT);
  remove_param_if_default_str(params, panel, "hlpX", AlignEx::DEFAULT_HLP_X);
  remove_param_if_default_str(params, panel, "hlpY", AlignEx::DEFAULT_HLP_Y);
  remove_param_if_default_str(params, panel, "hlpZ", AlignEx::DEFAULT_HLP_Z);
  remove_param_if_default_str(params, panel, "accept_name_mask_re");
  remove_param_if_default_str(params, panel, "decline_name_mask_re");
}

namespace AlignNode
{
static const bool DEFAULT_LEFT_TM = true;
}

void align_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "srcNode");
  add_edit_point3_if_not_exists(params, panel, field_idx, "rot_euler");
  add_edit_point3_if_not_exists(params, panel, field_idx, "scale");
  add_edit_bool_if_not_exists(params, panel, field_idx, "useLocal");
  add_edit_bool_if_not_exists(params, panel, field_idx, "leftTm", AlignNode::DEFAULT_LEFT_TM);
  add_edit_bool_if_not_exists(params, panel, field_idx, "binaryWt");
  add_edit_bool_if_not_exists(params, panel, field_idx, "copyBlendResultToSrc");
  add_edit_bool_if_not_exists(params, panel, field_idx, "copyPos");
  add_edit_box_if_not_exists(params, panel, field_idx, "srcSlot");
  add_edit_box_if_not_exists(params, panel, field_idx, "wtModulate");
  add_edit_bool_if_not_exists(params, panel, field_idx, "wtModulateInverse");
}

void align_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "targetNode");
  remove_param_if_default_str(params, panel, "srcNode");
  remove_param_if_default_point3(params, panel, "rot_euler");
  remove_param_if_default_point3(params, panel, "scale");
  remove_param_if_default_bool(params, panel, "useLocal");
  remove_param_if_default_bool(params, panel, "leftTm", AlignNode::DEFAULT_LEFT_TM);
  remove_param_if_default_bool(params, panel, "binaryWt");
  remove_param_if_default_bool(params, panel, "copyBlendResultToSrc");
  remove_param_if_default_bool(params, panel, "copyPos");
  remove_param_if_default_str(params, panel, "srcSlot");
  remove_param_if_default_str(params, panel, "wtModulate");
  remove_param_if_default_bool(params, panel, "wtModulateInverse");
}

namespace CompoundRotateShift
{
const float DEFAULT_MUL = 1.f;
const Point3 DEFAULT_SCALE = Point3(1.f, 1.f, 1.f);
} // namespace CompoundRotateShift

void compound_rotate_shift_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "alignAsNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "moveAlongNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "yaw");
  add_edit_float_if_not_exists(params, panel, field_idx, "yaw_mul", CompoundRotateShift::DEFAULT_MUL);
  add_edit_box_if_not_exists(params, panel, field_idx, "pitch");
  add_edit_float_if_not_exists(params, panel, field_idx, "pitch_mul", CompoundRotateShift::DEFAULT_MUL);
  add_edit_box_if_not_exists(params, panel, field_idx, "lean");
  add_edit_float_if_not_exists(params, panel, field_idx, "lean_mul", CompoundRotateShift::DEFAULT_MUL);
  add_edit_box_if_not_exists(params, panel, field_idx, "ofsX");
  add_edit_float_if_not_exists(params, panel, field_idx, "ofsX_mul", CompoundRotateShift::DEFAULT_MUL);
  add_edit_box_if_not_exists(params, panel, field_idx, "ofsY");
  add_edit_float_if_not_exists(params, panel, field_idx, "ofsY_mul", CompoundRotateShift::DEFAULT_MUL);
  add_edit_box_if_not_exists(params, panel, field_idx, "ofsZ");
  add_edit_float_if_not_exists(params, panel, field_idx, "ofsZ_mul", CompoundRotateShift::DEFAULT_MUL);
  add_edit_point3_if_not_exists(params, panel, field_idx, "preRotEuler");
  add_edit_point3_if_not_exists(params, panel, field_idx, "preScale", CompoundRotateShift::DEFAULT_SCALE);
  add_edit_point3_if_not_exists(params, panel, field_idx, "postRotEuler");
  add_edit_point3_if_not_exists(params, panel, field_idx, "postScale", CompoundRotateShift::DEFAULT_SCALE);
}

void compound_rotate_shift_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "targetNode");
  remove_param_if_default_str(params, panel, "alignAsNode");
  remove_param_if_default_str(params, panel, "moveAlongNode");
  remove_param_if_default_str(params, panel, "yaw");
  remove_param_if_default_float(params, panel, "yaw_mul", CompoundRotateShift::DEFAULT_MUL);
  remove_param_if_default_str(params, panel, "pitch");
  remove_param_if_default_float(params, panel, "pitch_mul", CompoundRotateShift::DEFAULT_MUL);
  remove_param_if_default_str(params, panel, "lean");
  remove_param_if_default_float(params, panel, "lean_mul", CompoundRotateShift::DEFAULT_MUL);
  remove_param_if_default_str(params, panel, "ofsX");
  remove_param_if_default_float(params, panel, "ofsX_mul", CompoundRotateShift::DEFAULT_MUL);
  remove_param_if_default_str(params, panel, "ofsY");
  remove_param_if_default_float(params, panel, "ofsY_mul", CompoundRotateShift::DEFAULT_MUL);
  remove_param_if_default_str(params, panel, "ofsZ");
  remove_param_if_default_float(params, panel, "ofsZ_mul", CompoundRotateShift::DEFAULT_MUL);
  remove_param_if_default_point3(params, panel, "preRotEuler");
  remove_param_if_default_point3(params, panel, "preScale", CompoundRotateShift::DEFAULT_SCALE);
  remove_param_if_default_point3(params, panel, "preRotEuler");
  remove_param_if_default_point3(params, panel, "preScale", CompoundRotateShift::DEFAULT_SCALE);
}

namespace DeltaAnglesCalc
{
static const int DEFAULT_SIDE_AXIS_IDX = 1;
static const float DEFAULT_SCALE_ROTATE = 1.0f;
static const float DEFAULT_SCALE_PITCH = 1.0f;
static const float DEFAULT_SCALE_LEAN = 1.0f;
static const bool DEFAULT_DEGREES = true;
} // namespace DeltaAnglesCalc

void delta_angles_calc_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "refNode");
  add_edit_int_if_not_exists(params, panel, field_idx, "fwdAxisIdx");
  add_edit_int_if_not_exists(params, panel, field_idx, "sideAxisIdx", DeltaAnglesCalc::DEFAULT_SIDE_AXIS_IDX);
  add_edit_bool_if_not_exists(params, panel, field_idx, "fwdAxisInverse");
  add_edit_bool_if_not_exists(params, panel, field_idx, "sideAxisInverse");
  add_edit_box_if_not_exists(params, panel, field_idx, "destRotateVar");
  add_edit_box_if_not_exists(params, panel, field_idx, "destPitchVar");
  add_edit_box_if_not_exists(params, panel, field_idx, "destLeanVar");
  add_edit_float_if_not_exists(params, panel, field_idx, "scaleRotate", DeltaAnglesCalc::DEFAULT_SCALE_ROTATE);
  add_edit_float_if_not_exists(params, panel, field_idx, "scalePitch", DeltaAnglesCalc::DEFAULT_SCALE_PITCH);
  add_edit_float_if_not_exists(params, panel, field_idx, "scaleLean", DeltaAnglesCalc::DEFAULT_SCALE_LEAN);
  add_edit_bool_if_not_exists(params, panel, field_idx, "degrees", DeltaAnglesCalc::DEFAULT_DEGREES);
}

void delta_angles_calc_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "refNode");
  remove_param_if_default_int(params, panel, "fwdAxisIdx");
  remove_param_if_default_int(params, panel, "sideAxisIdx", DeltaAnglesCalc::DEFAULT_SIDE_AXIS_IDX);
  remove_param_if_default_bool(params, panel, "fwdAxisInverse");
  remove_param_if_default_bool(params, panel, "sideAxisInverse");
  remove_param_if_default_str(params, panel, "destRotateVar");
  remove_param_if_default_str(params, panel, "destPitchVar");
  remove_param_if_default_str(params, panel, "destLeanVar");
  remove_param_if_default_float(params, panel, "scaleRotate", DeltaAnglesCalc::DEFAULT_SCALE_ROTATE);
  remove_param_if_default_float(params, panel, "scalePitch", DeltaAnglesCalc::DEFAULT_SCALE_PITCH);
  remove_param_if_default_float(params, panel, "scaleLean", DeltaAnglesCalc::DEFAULT_SCALE_LEAN);
  remove_param_if_default_bool(params, panel, "degrees", DeltaAnglesCalc::DEFAULT_DEGREES);
}

void delta_rotate_shift_calc_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "yaw");
  add_edit_box_if_not_exists(params, panel, field_idx, "pitch");
  add_edit_box_if_not_exists(params, panel, field_idx, "lean");
  add_edit_box_if_not_exists(params, panel, field_idx, "ofsX");
  add_edit_box_if_not_exists(params, panel, field_idx, "ofsY");
  add_edit_box_if_not_exists(params, panel, field_idx, "ofsZ");
  add_edit_bool_if_not_exists(params, panel, field_idx, "relativeToOrig");
}

void delta_rotate_shift_calc_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "targetNode");
  remove_param_if_default_str(params, panel, "yaw");
  remove_param_if_default_str(params, panel, "pitch");
  remove_param_if_default_str(params, panel, "lean");
  remove_param_if_default_str(params, panel, "ofsX");
  remove_param_if_default_str(params, panel, "ofsY");
  remove_param_if_default_str(params, panel, "ofsZ");
  remove_param_if_default_bool(params, panel, "relativeToOrig");
}

void effector_from_child_ik_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "parentNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "childNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "srcVar");
  add_edit_box_if_not_exists(params, panel, field_idx, "destVar");
  add_edit_box_if_not_exists(params, panel, field_idx, "resetEffByVal");
  add_edit_bool_if_not_exists(params, panel, field_idx, "resetEffInvVal");
  add_edit_box_if_not_exists(params, panel, field_idx, "varSlot");
}

void effector_from_child_ik_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "parentNode");
  remove_param_if_default_str(params, panel, "childNode");
  remove_param_if_default_str(params, panel, "srcVar");
  remove_param_if_default_str(params, panel, "destVar");
  remove_param_if_default_str(params, panel, "resetEffByVal");
  remove_param_if_default_bool(params, panel, "resetEffInvVal");
  remove_param_if_default_str(params, panel, "varSlot");
}

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

void fifo3_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "varname");
}

void fifo3_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "name");
  remove_param_if_default_str(params, panel, "varname");
}

void has_attachment_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "slot");
  add_edit_box_if_not_exists(params, panel, field_idx, "destVar");
  add_edit_bool_if_not_exists(params, panel, field_idx, "invertVal");
}

void has_attachment_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "slot");
  remove_param_if_default_str(params, panel, "destVar");
  remove_param_if_default_bool(params, panel, "invertVal");
}

namespace HumanAim
{
static const Point3 DEFAULT_OFFSET_MUL = Point3(1.f, 1.f, 1.f);
}

void human_aim_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_matrix_if_not_exists(params, panel, field_idx, "alignNodeTm");
  add_edit_box_if_not_exists(params, panel, field_idx, "rotateAroundNodeName");
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNodeName");
  add_edit_box_if_not_exists(params, panel, field_idx, "alignNodeName");
  add_edit_point3_if_not_exists(params, panel, field_idx, "rotateAroundNodeOffset");
  add_edit_box_if_not_exists(params, panel, field_idx, "pitchVar");
  add_edit_box_if_not_exists(params, panel, field_idx, "yawVar");
  add_edit_box_if_not_exists(params, panel, field_idx, "rollVar");
  add_edit_box_if_not_exists(params, panel, field_idx, "offsetXVar");
  add_edit_box_if_not_exists(params, panel, field_idx, "offsetYVar");
  add_edit_box_if_not_exists(params, panel, field_idx, "offsetZVar");
  add_edit_point3_if_not_exists(params, panel, field_idx, "offsetMul", HumanAim::DEFAULT_OFFSET_MUL);
  add_edit_point3_if_not_exists(params, panel, field_idx, "offsetAdd");
}

void human_aim_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_matrix(params, panel, "alignNodeTm");
  remove_param_if_default_str(params, panel, "rotateAroundNodeName");
  remove_param_if_default_str(params, panel, "targetNodeName");
  remove_param_if_default_str(params, panel, "alignNodeName");
  remove_param_if_default_point3(params, panel, "rotateAroundNodeOffset");
  remove_param_if_default_str(params, panel, "pitchVar");
  remove_param_if_default_str(params, panel, "yawVar");
  remove_param_if_default_str(params, panel, "rollVar");
  remove_param_if_default_str(params, panel, "offsetXVar");
  remove_param_if_default_str(params, panel, "offsetYVar");
  remove_param_if_default_str(params, panel, "offsetZVar");
  remove_param_if_default_point3(params, panel, "offsetMul", HumanAim::DEFAULT_OFFSET_MUL);
  remove_param_if_default_point3(params, panel, "offsetAdd");
}

namespace Lookat
{
static const bool DEFAULT_LEFT_TM = true;
}

void lookat_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "param");
  add_edit_point3_if_not_exists(params, panel, field_idx, "rot_euler");
  add_edit_bool_if_not_exists(params, panel, field_idx, "leftTm", Lookat::DEFAULT_LEFT_TM);
  add_edit_bool_if_not_exists(params, panel, field_idx, "swapYZ");

  // Move all tragetNode params in one place
  int lastTargetNodeWtPid = 0;
  int targetNodeCount = 0;
  for (const AnimParamData &param : params)
  {
    if (param.name == "targetNode")
    {
      if (lastTargetNodeWtPid)
        panel->moveById(param.pid, lastTargetNodeWtPid, /*after*/ true);
      lastTargetNodeWtPid = param.pid;
      ++targetNodeCount;
    }
  }

  panel->createButton(PID_CTRLS_TARGET_NODE_ADD, "Add target node");
  panel->createButton(PID_CTRLS_TARGET_NODE_REMOVE, "Remove last target node", targetNodeCount != 1);
}

void lookat_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_point3(params, panel, "rot_euler");
  remove_param_if_default_bool(params, panel, "leftTm", Lookat::DEFAULT_LEFT_TM);
  remove_param_if_default_bool(params, panel, "swapYZ");
}

namespace LookatNode
{
static const int DEFAULT_LOOKAT_AXIS = 2;
static const int DEFAULT_UP_AXIS = 1;
static const Point3 DEFAULT_UP_VECTOR = Point3(0.f, 1.f, 0.f);
} // namespace LookatNode

void lookat_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "lookatNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "upNode");
  add_edit_int_if_not_exists(params, panel, field_idx, "lookatAxis", LookatNode::DEFAULT_LOOKAT_AXIS);
  add_edit_int_if_not_exists(params, panel, field_idx, "upAxis", LookatNode::DEFAULT_UP_AXIS);
  add_edit_point3_if_not_exists(params, panel, field_idx, "upVector", LookatNode::DEFAULT_UP_VECTOR);
  add_edit_bool_if_not_exists(params, panel, field_idx, "negX");
  add_edit_bool_if_not_exists(params, panel, field_idx, "negY");
  add_edit_bool_if_not_exists(params, panel, field_idx, "negZ");
  add_edit_box_if_not_exists(params, panel, field_idx, "accept_name_mask_re");
  add_edit_box_if_not_exists(params, panel, field_idx, "decline_name_mask_re");
}

void lookat_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "upNode");
  remove_param_if_default_int(params, panel, "lookatAxis", LookatNode::DEFAULT_LOOKAT_AXIS);
  remove_param_if_default_int(params, panel, "upAxis", LookatNode::DEFAULT_UP_AXIS);
  remove_param_if_default_point3(params, panel, "upVector", LookatNode::DEFAULT_UP_VECTOR);
  remove_param_if_default_bool(params, panel, "negX");
  remove_param_if_default_bool(params, panel, "negY");
  remove_param_if_default_bool(params, panel, "negZ");
  remove_param_if_default_str(params, panel, "accept_name_mask_re");
  remove_param_if_default_str(params, panel, "decline_name_mask_re");
}

void mat_from_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "destSlot");
  add_edit_box_if_not_exists(params, panel, field_idx, "srcSlot");
  add_edit_box_if_not_exists(params, panel, field_idx, "destVar");
  add_edit_box_if_not_exists(params, panel, field_idx, "srcNode");
}

void mat_from_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "destSlot");
  remove_param_if_default_str(params, panel, "srcSlot");
  remove_param_if_default_str(params, panel, "destVar");
  remove_param_if_default_str(params, panel, "srcNode");
}

namespace MoveNode
{
static const char *LOCAL_ADDITIVE_PARAM = "localAdditive";
static const char *LOCAL_ADDITIVE_2_PARAM = "localAdditive2";
static const float DEFAULT_TARGET_NODE_WT = 1.0f;
static const float DEFAULT_K_MUL = 1.0f;
static const Point3 DEFAULT_ROT_AXIS = Point3(0.f, 1.f, 0.f);
static const Point3 DEFAULT_DIR_AXIS = Point3(0.f, 0.f, 1.f);
} // namespace MoveNode

void move_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_float_if_not_exists(params, panel, field_idx, "targetNodeWt", MoveNode::DEFAULT_TARGET_NODE_WT);
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "param");
  add_edit_box_if_not_exists(params, panel, field_idx, "axis_course");
  add_edit_float_if_not_exists(params, panel, field_idx, "kCourseAdd");
  add_edit_float_if_not_exists(params, panel, field_idx, "kMul", MoveNode::DEFAULT_K_MUL);
  add_edit_float_if_not_exists(params, panel, field_idx, "kAdd");
  add_edit_point3_if_not_exists(params, panel, field_idx, "rotAxis", MoveNode::DEFAULT_ROT_AXIS);
  add_edit_point3_if_not_exists(params, panel, field_idx, "dirAxis", MoveNode::DEFAULT_DIR_AXIS);
  add_edit_bool_if_not_exists(params, panel, field_idx, MoveNode::LOCAL_ADDITIVE_PARAM);
  { // additive param default value depends on value in localAdditive param
    const bool localAdditiveValue = get_bool_param_by_name(params, panel, MoveNode::LOCAL_ADDITIVE_PARAM);
    add_edit_bool_if_not_exists(params, panel, field_idx, "additive", localAdditiveValue);
  }
  add_edit_bool_if_not_exists(params, panel, field_idx, MoveNode::LOCAL_ADDITIVE_2_PARAM);
  { // saveOtherAxisMove param default value depends on value in localAdditive2 param
    const bool localAdditive2Value = get_bool_param_by_name(params, panel, MoveNode::LOCAL_ADDITIVE_2_PARAM);
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
  const bool localAdditiveValue = get_bool_param_by_name(params, panel, MoveNode::LOCAL_ADDITIVE_PARAM);
  const bool localAdditive2Value = get_bool_param_by_name(params, panel, MoveNode::LOCAL_ADDITIVE_2_PARAM);

  remove_params_if_default_target_node_wt(params, panel);
  remove_param_if_default_float(params, panel, "targetNodeWt", MoveNode::DEFAULT_TARGET_NODE_WT);
  remove_param_if_default_str(params, panel, "axis_course");
  remove_param_if_default_float(params, panel, "kCourseAdd");
  remove_param_if_default_float(params, panel, "kMul", MoveNode::DEFAULT_K_MUL);
  remove_param_if_default_float(params, panel, "kAdd");
  remove_param_if_default_point3(params, panel, "rotAxis", MoveNode::DEFAULT_ROT_AXIS);
  remove_param_if_default_point3(params, panel, "dirAxis", MoveNode::DEFAULT_DIR_AXIS);
  remove_param_if_default_bool(params, panel, MoveNode::LOCAL_ADDITIVE_PARAM);
  remove_param_if_default_bool(params, panel, "additive", localAdditiveValue);
  remove_param_if_default_bool(params, panel, MoveNode::LOCAL_ADDITIVE_2_PARAM);
  remove_param_if_default_bool(params, panel, "saveOtherAxisMove", localAdditive2Value);
  remove_param_if_default_str(params, panel, "accept_name_mask_re");
  remove_param_if_default_str(params, panel, "decline_name_mask_re");
}

void null_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
}

void param_from_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "slot");
  add_edit_box_if_not_exists(params, panel, field_idx, "node");
  add_edit_box_if_not_exists(params, panel, field_idx, "destVar");
  add_edit_float_if_not_exists(params, panel, field_idx, "defVal");
  add_edit_bool_if_not_exists(params, panel, field_idx, "invertVal");
}

void param_from_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "slot");
  remove_param_if_default_str(params, panel, "node");
  remove_param_if_default_str(params, panel, "destVar");
  remove_param_if_default_float(params, panel, "defVal");
  remove_param_if_default_bool(params, panel, "invertVal");
}

namespace RotateAround
{
static const float DEFAULT_K_MUL = 1.0f;
static const float DEFAULT_TARGET_NODE_WT = 1.0f;
static const Point3 DEFAULT_ROT_AXIS = Point3(0.f, 1.f, 0.f);
} // namespace RotateAround

void rotate_around_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_float_if_not_exists(params, panel, field_idx, "targetNodeWt", RotateAround::DEFAULT_TARGET_NODE_WT);
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "param");
  add_edit_float_if_not_exists(params, panel, field_idx, "kMul", RotateAround::DEFAULT_K_MUL);
  add_edit_float_if_not_exists(params, panel, field_idx, "kAdd");
  add_edit_bool_if_not_exists(params, panel, field_idx, "inRadian");
  add_edit_point3_if_not_exists(params, panel, field_idx, "rotAxis", RotateAround::DEFAULT_ROT_AXIS);

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
  remove_param_if_default_float(params, panel, "targetNodeWt", RotateAround::DEFAULT_TARGET_NODE_WT);
  remove_param_if_default_float(params, panel, "kMul", RotateAround::DEFAULT_K_MUL);
  remove_param_if_default_float(params, panel, "kAdd");
  remove_param_if_default_bool(params, panel, "inRadian");
  remove_param_if_default_point3(params, panel, "rotAxis", RotateAround::DEFAULT_ROT_AXIS);
}

namespace RotateNode
{
static const char *SWAP_Y_Z_PARAM = "swapYZ";
static const char *RELATIVE_ROR_PARAM = "relativeRot";
static const float DEFAULT_K_MUL = 1.0f;
static const float DEFAULT_TARGET_NODE_WT = 1.0f;
static const int DEFAULT_AXIS_FROM_CHAR_INDEX = -1;
static const Point3 DEFAULT_ROT_AXIS = Point3(0.f, 1.f, 0.f);
static const Point3 DEFAULT_DIR_AXIS = Point3(0.f, 0.f, 1.f);
} // namespace RotateNode

void rotate_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_float_if_not_exists(params, panel, field_idx, "targetNodeWt", RotateNode::DEFAULT_TARGET_NODE_WT);
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "param");
  add_edit_box_if_not_exists(params, panel, field_idx, "axis_course");
  add_edit_float_if_not_exists(params, panel, field_idx, "kCourseAdd");
  add_edit_float_if_not_exists(params, panel, field_idx, "kMul", RotateNode::DEFAULT_K_MUL);
  add_edit_float_if_not_exists(params, panel, field_idx, "kAdd");
  add_edit_float_if_not_exists(params, panel, field_idx, "kMul2", RotateNode::DEFAULT_K_MUL);
  add_edit_float_if_not_exists(params, panel, field_idx, "kAdd2");
  add_edit_bool_if_not_exists(params, panel, field_idx, "saveScale");
  add_edit_bool_if_not_exists(params, panel, field_idx, "inRadian");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramAdd");
  add_edit_box_if_not_exists(params, panel, field_idx, "paramAdd2");
  add_edit_point3_if_not_exists(params, panel, field_idx, "rotAxis", RotateNode::DEFAULT_ROT_AXIS);
  add_edit_point3_if_not_exists(params, panel, field_idx, "dirAxis", RotateNode::DEFAULT_DIR_AXIS);
  add_edit_bool_if_not_exists(params, panel, field_idx, RotateNode::SWAP_Y_Z_PARAM);
  add_edit_bool_if_not_exists(params, panel, field_idx, RotateNode::RELATIVE_ROR_PARAM);
  const bool swapYZValue = get_bool_param_by_name(params, panel, RotateNode::SWAP_Y_Z_PARAM);
  const bool relativeRotValue = get_bool_param_by_name(params, panel, RotateNode::RELATIVE_ROR_PARAM);
  const bool defaultLeftTm = !swapYZValue && !relativeRotValue;
  add_edit_bool_if_not_exists(params, panel, field_idx, "leftTm", defaultLeftTm);
  add_edit_int_if_not_exists(params, panel, field_idx, "axisFromCharIndex", RotateNode::DEFAULT_AXIS_FROM_CHAR_INDEX);
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
  const bool swapYZValue = get_bool_param_by_name(params, panel, RotateNode::SWAP_Y_Z_PARAM);
  const bool relativeRotValue = get_bool_param_by_name(params, panel, RotateNode::RELATIVE_ROR_PARAM);
  const bool defaultLeftTm = !swapYZValue && !relativeRotValue;

  remove_params_if_default_target_node_wt(params, panel);
  remove_param_if_default_float(params, panel, "targetNodeWt", RotateNode::DEFAULT_TARGET_NODE_WT);
  remove_param_if_default_str(params, panel, "axis_course");
  remove_param_if_default_float(params, panel, "kCourseAdd");
  remove_param_if_default_float(params, panel, "kMul", RotateNode::DEFAULT_K_MUL);
  remove_param_if_default_float(params, panel, "kAdd");
  remove_param_if_default_float(params, panel, "kMul2", RotateNode::DEFAULT_K_MUL);
  remove_param_if_default_float(params, panel, "kAdd2");
  remove_param_if_default_bool(params, panel, "saveScale");
  remove_param_if_default_bool(params, panel, "inRadian");
  remove_param_if_default_str(params, panel, "paramAdd");
  remove_param_if_default_str(params, panel, "paramAdd2");
  remove_param_if_default_point3(params, panel, "rotAxis", RotateNode::DEFAULT_ROT_AXIS);
  remove_param_if_default_point3(params, panel, "dirAxis", RotateNode::DEFAULT_DIR_AXIS);
  remove_param_if_default_bool(params, panel, RotateNode::SWAP_Y_Z_PARAM);
  remove_param_if_default_bool(params, panel, RotateNode::RELATIVE_ROR_PARAM);
  remove_param_if_default_bool(params, panel, "leftTm", defaultLeftTm);
  remove_param_if_default_int(params, panel, "axisFromCharIndex", RotateNode::DEFAULT_AXIS_FROM_CHAR_INDEX);
  remove_param_if_default_str(params, panel, "accept_name_mask_re");
  remove_param_if_default_str(params, panel, "decline_name_mask_re");
}

namespace ScaleNode
{
static const float DEFAULT_TARGET_NODE_WT = 1.0f;
static const float DEFAULT_VALUE = 1.0f;
} // namespace ScaleNode

void scale_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_float_if_not_exists(params, panel, field_idx, "targetNodeWt", ScaleNode::DEFAULT_TARGET_NODE_WT);
  add_edit_box_if_not_exists(params, panel, field_idx, "targetNode");
  add_edit_box_if_not_exists(params, panel, field_idx, "param");
  add_edit_point3_if_not_exists(params, panel, field_idx, "scaleAxis");
  add_edit_float_if_not_exists(params, panel, field_idx, "defaultValue", ScaleNode::DEFAULT_VALUE);
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

void scale_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_params_if_default_target_node_wt(params, panel);
  remove_param_if_default_float(params, panel, "targetNodeWt", ScaleNode::DEFAULT_TARGET_NODE_WT);
  remove_param_if_default_point3(params, panel, "scaleAxis");
  remove_param_if_default_float(params, panel, "defaultValue", ScaleNode::DEFAULT_VALUE);
  remove_param_if_default_str(params, panel, "accept_name_mask_re");
  remove_param_if_default_str(params, panel, "decline_name_mask_re");
}

void set_motion_matching_tag_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "mmTag");
}

namespace SetParam
{
const float DEFAULT_MIW_WEIGHT = 1e-4;
}

void set_param_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  Tab<String> names{String("srcVar"), String("namedValue"), String("slotIdValue"), String("value")};
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_bool_if_not_exists(params, panel, field_idx, "addNewParamsTypeInt");
  add_edit_float_if_not_exists(params, panel, field_idx, "minWeight", SetParam::DEFAULT_MIW_WEIGHT);
  add_edit_bool_if_not_exists(params, panel, field_idx, "pullParamValueFromSlot");
  add_edit_box_if_not_exists(params, panel, field_idx, "destSlot");
  add_edit_box_if_not_exists(params, panel, field_idx, "destVar");
  int selectedType = names.size() - 1; // Last is default param
  for (int i = 0; i < names.size(); ++i)
  {
    AnimParamData *param = find_param_by_name(params, names[i]);
    if (param != params.end())
    {
      selectedType = i;
      for (int j = i + 1; j < names.size(); ++j)
      {
        AnimParamData *hiddenParam = find_param_by_name(params, names[j]);
        if (hiddenParam != params.end())
        {
          AnimParamData *nameParam = find_param_by_name(params, "name");
          panel->removeById(hiddenParam->pid);
          params.erase(hiddenParam);
          logerr("In controller <%s> param with name <%s> can be hidden by another param", panel->getText(nameParam->pid), names[j]);
        }
      }
      break;
    }
  }
  panel->createCombo(field_idx, "Param type", names, selectedType);
  params.emplace_back(AnimParamData{field_idx, DataBlock::TYPE_STRING, String("Param type")});
  ++field_idx;
  if (names[selectedType] != "value")
    add_edit_box_if_not_exists(params, panel, field_idx, names[selectedType]);
  else
    add_edit_float_if_not_exists(params, panel, field_idx, names[selectedType]);
}

void set_param_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_bool(params, panel, "addNewParamsTypeInt");
  remove_param_if_default_float(params, panel, "minWeight", SetParam::DEFAULT_MIW_WEIGHT);
  remove_param_if_default_bool(params, panel, "pullParamValueFromSlot");
  remove_param_if_default_str(params, panel, "destSlot");
  remove_param_if_default_str(params, panel, "destVar");
  auto paramCombo = find_param_by_name(params, "Param type");
  if (paramCombo != params.end())
    params.erase(paramCombo);
  remove_param_if_default_str(params, panel, "srcVar");
  remove_param_if_default_str(params, panel, "namedValue");
  remove_param_if_default_str(params, panel, "slotIdValue");
  remove_param_if_default_float(params, panel, "value");
}

void twist_ctrl_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx)
{
  add_edit_box_if_not_exists(params, panel, field_idx, "name");
  add_edit_box_if_not_exists(params, panel, field_idx, "node0");
  add_edit_box_if_not_exists(params, panel, field_idx, "node1");
  add_edit_box_if_not_exists(params, panel, field_idx, "twistNode");
  add_edit_float_if_not_exists(params, panel, field_idx, "angDiff");
  add_edit_bool_if_not_exists(params, panel, field_idx, "optional");

  // Move all twistNode params in one place
  int lastTwistNodeWtPid = 0;
  int targetNodeCount = 0;
  for (const AnimParamData &param : params)
  {
    if (param.name == "twistNode")
    {
      if (lastTwistNodeWtPid)
        panel->moveById(param.pid, lastTwistNodeWtPid, /*after*/ true);
      lastTwistNodeWtPid = param.pid;
      ++targetNodeCount;
    }
  }

  panel->createButton(PID_CTRLS_TARGET_NODE_ADD, "Add twist node");
  panel->createButton(PID_CTRLS_TARGET_NODE_REMOVE, "Remove last twist node", targetNodeCount != 1);
}

void twist_ctrl_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel)
{
  remove_param_if_default_str(params, panel, "node0");
  remove_param_if_default_str(params, panel, "node1");
  remove_param_if_default_float(params, panel, "angDiff");
  remove_param_if_default_bool(params, panel, "optional");
}
