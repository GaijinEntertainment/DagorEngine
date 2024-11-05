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
struct BlendNodeData;
class DataBlock;

void hub_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void hub_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void hub_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void hub_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void hub_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
const char *hub_get_child_name_by_idx(const DataBlock &settings, int idx);
