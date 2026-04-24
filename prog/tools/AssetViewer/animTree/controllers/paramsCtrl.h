// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>

class DataBlock;
namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;

void params_ctrl_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void params_ctrl_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
void params_ctrl_set_selected_change_param(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void params_ctrl_remove_change_param(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
void params_ctrl_change_rate_src_changed(PropPanel::ContainerPropertyControl *panel);
void params_ctrl_change_scale_src_changed(PropPanel::ContainerPropertyControl *panel);
void params_ctrl_block_type_changed(PropPanel::ContainerPropertyControl *group, const DataBlock *settings);
void params_ctrl_add_mapping(PropPanel::ContainerPropertyControl *panel);
void params_ctrl_remove_mapping(PropPanel::ContainerPropertyControl *panel);
void save_if_math_fields(PropPanel::ContainerPropertyControl *panel, DataBlock &settings);
