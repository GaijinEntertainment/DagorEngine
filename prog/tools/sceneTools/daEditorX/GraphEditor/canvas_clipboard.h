// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <graphEditor/graph_data.h>

#include <EASTL/vector.h>

#include <imgui/imgui.h>

class GraphPanel;

struct CanvasClipboard
{
  eastl::vector<GraphData::Node> nodes;
  eastl::vector<GraphData::Edge> edges;

  bool empty() const { return nodes.empty(); }

  void captureSelection(const GraphPanel &panel, const GraphData &graph);
  void paste(GraphPanel &panel, const ImVec2 &paste_origin_canvas) const;
};
