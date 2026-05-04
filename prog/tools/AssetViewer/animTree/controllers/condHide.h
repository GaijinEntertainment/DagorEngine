// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;
class DataBlock;

void cond_hide_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void cond_hide_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void cond_hide_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
void cond_hide_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void cond_hide_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
