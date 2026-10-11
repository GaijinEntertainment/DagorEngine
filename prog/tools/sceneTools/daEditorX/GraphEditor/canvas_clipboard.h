// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <graphEditor/graph_data.h>

#include <EASTL/vector.h>

#include <imgui/imgui.h>

class GraphTab;

struct CanvasClipboard
{
  eastl::vector<GraphData::Node> nodes;
  eastl::vector<GraphData::Edge> edges;

  bool empty() const { return nodes.empty(); }

  void captureSelection(const GraphTab &graph_tab, const GraphData &graph, bool with_input_edges);

  // Lands the captured nodes with their top-left corner at paste_origin_canvas.
  void paste(GraphTab &graph_tab, const GraphData &graph, const ImVec2 &paste_origin_canvas, eastl::vector<GraphData::Node> &out_nodes,
    eastl::vector<GraphData::Edge> &out_edges) const;

  // Lands them shifted by `delta` from where they were captured, which a duplicate wants and a
  // paste-at-the-cursor does not.
  void pasteShifted(GraphTab &graph_tab, const GraphData &graph, const ImVec2 &delta, eastl::vector<GraphData::Node> &out_nodes,
    eastl::vector<GraphData::Edge> &out_edges) const;
};
