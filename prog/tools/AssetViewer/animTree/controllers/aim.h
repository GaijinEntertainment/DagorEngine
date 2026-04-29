// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;
class DataBlock;

void aim_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void aim_set_dependent_defaults(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void aim_update_dependent_fields(AnimParamData &param, dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void aim_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void aim_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void aim_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
void aim_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void aim_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
