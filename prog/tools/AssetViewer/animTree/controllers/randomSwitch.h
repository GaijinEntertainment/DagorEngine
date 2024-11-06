// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <util/dag_string.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;
struct AnimCtrlData;
struct BlendNodeData;
class DataBlock;

void random_switch_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void random_switch_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void random_switch_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void random_switch_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings);
void random_switch_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
const char *random_switch_get_child_name_by_idx(const DataBlock &settings, int idx);
String random_switch_get_child_prefix_name(const DataBlock &settings, int idx);
