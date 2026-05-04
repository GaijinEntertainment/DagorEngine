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
struct DependentParamData;
class DataBlock;

void animate_node_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void animate_node_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void animate_node_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params);
void animate_node_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
void animate_node_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params, dag::ConstSpan<AnimParamData> base_params);
void animate_node_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
void animate_node_add_anim_node(PropPanel::ContainerPropertyControl *panel);
void animate_node_remove_anim_node(PropPanel::ContainerPropertyControl *panel);
void animate_node_add_anim_node_re(PropPanel::ContainerPropertyControl *panel);
void animate_node_remove_anim_node_re(PropPanel::ContainerPropertyControl *panel);
const char *animate_node_get_child_name_by_idx(const DataBlock &settings, int idx);
String animate_node_get_child_prefix_name(const DataBlock &settings, int idx);
void animate_node_update_child_name(DataBlock &settings, const char *name, const String &old_name);
