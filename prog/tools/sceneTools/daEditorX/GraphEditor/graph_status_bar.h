// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

struct GraphData;
struct TexGenPipelineStatus;

// Height (already DPI-scaled) the bar reserves below the node-editor canvas. The panel queries
// this to size the canvas, then draws the bar in the reserved strip.
float graph_status_bar_height();

// Draws the bar into the current ImGui window. Call with plain ImGui state current -- NOT inside
// an ax::NodeEditor SetCurrentEditor scope. `selected_object_count` is the live ne selection size
// (nodes + links) the panel captured while the editor was current. `bar_height` is the
// already-scaled strip height (graph_status_bar_height()). `pipeline` is a default one when there
// is no texgen service (the bar then shows just the Items counter and empty memory donuts).
void draw_graph_status_bar(const GraphData &graph_data, const TexGenPipelineStatus &pipeline, int selected_object_count,
  float bar_height);
