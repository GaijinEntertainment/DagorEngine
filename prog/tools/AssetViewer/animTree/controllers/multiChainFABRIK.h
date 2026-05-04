// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;
struct DependentParamData;
class DataBlock;

void multi_chain_fabrik_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void multi_chain_fabrik_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void multi_chain_fabrik_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params);
void multi_chain_fabrik_change_block_type(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params);
void multi_chain_fabrik_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
void multi_chain_fabrik_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params, dag::ConstSpan<AnimParamData> base_params);
void multi_chain_fabrik_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
