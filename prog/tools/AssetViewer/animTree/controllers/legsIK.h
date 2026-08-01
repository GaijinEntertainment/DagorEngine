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

void legs_ik_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void legs_ik_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void legs_ik_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void legs_ik_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
void legs_ik_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void legs_ik_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
IListReorderHandler *legs_ik_get_reorder_handler(AnimTreePlugin &plugin, dag::ConstSpan<AnimCtrlData> controllers,
  PropPanel::ContainerPropertyControl *panel);
