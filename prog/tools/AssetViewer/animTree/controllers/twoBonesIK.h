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

void two_bones_ik_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void two_bones_ik_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void two_bones_ik_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
void two_bones_ik_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void two_bones_ik_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
IListReorderHandler *two_bones_ik_get_reorder_handler(AnimTreePlugin &plugin, dag::ConstSpan<AnimCtrlData> controllers,
  PropPanel::ContainerPropertyControl *panel);
