// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;
struct AnimCtrlData;
class DataBlock;
class AnimTreePlugin;
class IListReorderHandler;

void def_clamp_ctrl_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void def_clamp_ctrl_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void def_clamp_ctrl_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
void def_clamp_ctrl_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void def_clamp_ctrl_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
IListReorderHandler *def_clamp_ctrl_get_reorder_handler(AnimTreePlugin &plugin, dag::ConstSpan<AnimCtrlData> controllers,
  PropPanel::ContainerPropertyControl *panel);