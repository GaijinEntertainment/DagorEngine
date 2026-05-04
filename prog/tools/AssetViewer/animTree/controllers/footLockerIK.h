// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;
class DataBlock;

void foot_locker_ik_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void foot_locker_ik_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void foot_locker_ik_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void foot_locker_ik_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
void foot_locker_ik_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void foot_locker_ik_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);