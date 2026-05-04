// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_tab.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

class DataBlock;
struct BlendNodeData;
struct AnimCtrlData;
struct AnimStatesData;
class IListReorderHandler;
class AnimTreePlugin;

dag::Vector<Tab<String>> init_fifo3_prepare_combo_values(PropPanel::ContainerPropertyControl *panel,
  dag::ConstSpan<BlendNodeData> blend_nodes, dag::ConstSpan<AnimCtrlData> controllers);
void init_fifo3_init_block_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings,
  dag::Vector<Tab<String>> &combo_names);
void init_fifo3_set_selected_node_list_settings(PropPanel::ContainerPropertyControl *panel, const DataBlock &settings);
void init_fifo3_remove_node_from_list(PropPanel::ContainerPropertyControl *panel, DataBlock &settings);
void init_fifo3_update_child_name(DataBlock &settings, const char *name, const String &old_name);
IListReorderHandler *init_fifo3_get_reorder_handler(AnimTreePlugin &plugin, dag::ConstSpan<AnimStatesData> states,
  PropPanel::ContainerPropertyControl *panel);
