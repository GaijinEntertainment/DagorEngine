// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;

void align_ex_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void align_ex_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void align_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void align_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void compound_rotate_shift_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void compound_rotate_shift_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void delta_angles_calc_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void delta_angles_calc_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void delta_rotate_shift_calc_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void delta_rotate_shift_calc_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void effector_from_child_ik_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void effector_from_child_ik_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void eye_ctrl_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void eye_ctrl_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void fifo3_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void fifo3_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void has_attachment_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void has_attachment_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void human_aim_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void human_aim_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void lookat_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void lookat_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void lookat_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void lookat_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void mat_from_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void mat_from_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void move_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void move_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void null_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);

void param_from_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void param_from_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void rotate_around_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void rotate_around_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void rotate_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void rotate_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void scale_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void scale_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void set_motion_matching_tag_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);

void set_param_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void set_param_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);

void twist_ctrl_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void twist_ctrl_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
