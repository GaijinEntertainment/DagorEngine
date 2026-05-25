// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_tab.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

struct AnimParamData;
struct AnimCtrlData;
class DataBlock;

void parametric_init_panel(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, int field_idx,
  bool default_foreign);
void parametric_set_dependent_defaults(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel);
void parametric_prepare_params(dag::Vector<AnimParamData> &params, PropPanel::ContainerPropertyControl *panel, bool default_foreign);
void parametric_init_block_settings(PropPanel::ContainerPropertyControl *prop_panel, const DataBlock *settings,
  dag::ConstSpan<AnimCtrlData> controllers);
void parametric_set_selected_reset_random_switch_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings,
  dag::ConstSpan<AnimCtrlData> controllers);
void parametric_save_block_settings(PropPanel::ContainerPropertyControl *panel, DataBlock &settings);
void parametric_add_reset_random_switch_to_list(PropPanel::ContainerPropertyControl *panel);
void parametric_remove_reset_random_switch_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock &settings);
