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

void eff_from_attachment_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx);
void eff_from_attachment_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void eff_from_attachment_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params);
void eff_from_attachment_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
void eff_from_attachment_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock *settings,
  dag::Vector<DependentParamData> &params, dag::ConstSpan<AnimParamData> base_params);
void eff_from_attachment_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock *settings);
