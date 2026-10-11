// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

struct ImRect;
struct ImVec2;

// Whether the key does something in this state, which is not always the same as its dispatch
// firing: Tab stays live while the pin jump menu handles it directly.
struct GraphHotkeyContext
{
  bool pinJumpAvailable = false;
  bool addNodeAvailable = false;
  bool addTransitNodeAvailable = false;
  bool createSubgraphAvailable = false;
  bool openSubgraphAvailable = false;
  bool focusSubgraphAvailable = false;
  bool explodeSubgraphAvailable = false;
  bool parentGraphAvailable = false;
};

// From the left edge of the canvas to the last `end_width` px of its row (graph_hotkeys_bar_end), which stay free; the
// first entry that does not fit ends the row.
void draw_graph_hotkeys_bar(const ImVec2 &canvas_min, const ImVec2 &canvas_max, const GraphHotkeyContext &context, float end_width);
// Margin included.
float graph_hotkeys_bar_height();
// The last `width` px of the bar's row.
ImRect graph_hotkeys_bar_end(const ImVec2 &canvas_max, float width);
