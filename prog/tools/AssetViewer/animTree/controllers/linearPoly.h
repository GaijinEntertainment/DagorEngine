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

void linear_poly_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void linear_poly_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void linear_poly_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void linear_poly_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void linear_poly_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
const char *linear_poly_get_child_name_by_idx(const DataBlock &settings, int idx);
