// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;
struct AnimStatesData;
struct DependentParamData;
class DataBlock;
class IListReorderHandler;
class AnimTreePlugin;

void state_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx,
  const DataBlock &props);
void state_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, const DataBlock &props);
void state_init_block_settings(PropPanel::ContainerPropertyControl *prop_panel, const DataBlock &settings, const DataBlock &state_desc,
  dag::ConstSpan<AnimStatesData> states_data, dag::Vector<DependentParamData> &params);
void state_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings,
  const DataBlock &state_desc, dag::ConstSpan<AnimStatesData> states_data, dag::Vector<DependentParamData> &params,
  dag::ConstSpan<AnimParamData> base_params);
void state_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock &settings);
const char *state_get_child_name_by_idx(const DataBlock &settings, int idx);
void state_update_child_name(DataBlock &settings, const char *name, const String &old_name);
IListReorderHandler *state_get_reorder_handler(AnimTreePlugin &plugin, dag::ConstSpan<AnimStatesData> states,
  PropPanel::ContainerPropertyControl *panel);
