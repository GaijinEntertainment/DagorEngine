// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <generic/dag_tab.h>
#include <EASTL/unique_ptr.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

class DataBlock;
struct AnimCtrlData;
struct AnimStatesData;
class IListReorderHandler;
class AnimTreePlugin;

Tab<String> post_blend_ctrl_order_prepare_combo_values(PropPanel::ContainerPropertyControl *panel,
  dag::ConstSpan<AnimCtrlData> controllers);
void post_blend_ctrl_order_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings,
  Tab<String> &combo_names);
void post_blend_ctrl_order_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings);
void post_blend_ctrl_order_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock &settings);
void post_blend_ctrl_order_update_child_name(DataBlock &settings, const char *name, const String &old_name);
IListReorderHandler *post_blend_ctrl_order_get_reorder_handler(AnimTreePlugin &plugin, dag::ConstSpan<AnimStatesData> states,
  PropPanel::ContainerPropertyControl *panel);
