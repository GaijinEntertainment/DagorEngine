// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "canvas_clipboard.h"

#include "graph_panel.h"

#include <EASTL/algorithm.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>

#include <imgui_node_editor.h>

namespace ne = ax::NodeEditor;

namespace
{
int extractNodeIdFromNeNodeId(uint64_t ne_node_id) { return static_cast<int>(ne_node_id) - 1; }
} // namespace

void CanvasClipboard::captureSelection(const GraphPanel &panel, const GraphData &graph)
{
  const int objCount = ne::GetSelectedObjectCount();
  if (objCount == 0)
  {
    return;
  }

  eastl::vector<ne::NodeId> selected;
  selected.resize(objCount);
  const int n = ne::GetSelectedNodes(selected.data(), objCount);
  selected.resize(n);
  if (selected.empty())
  {
    return;
  }

  eastl::hash_set<int> ids;
  for (ne::NodeId ne_id : selected)
  {
    const int node_id = extractNodeIdFromNeNodeId(ne_id.Get());
    ids.insert(node_id);
    const auto it =
      eastl::find_if(graph.nodes.begin(), graph.nodes.end(), [node_id](const GraphData::Node &nd) { return nd.id == node_id; });
    if (it != graph.nodes.end() && it->descName == "block")
    {
      eastl::vector<int> children;
      panel.collectNodesInsideBlock(node_id, children);
      for (int c : children)
      {
        ids.insert(c);
      }
    }
  }

  nodes.clear();
  edges.clear();
  for (const GraphData::Node &nd : graph.nodes)
  {
    if (ids.find(nd.id) != ids.end())
    {
      nodes.push_back(nd);
    }
  }
  for (const GraphData::Edge &e : graph.edges)
  {
    if (ids.find(e.elemA) != ids.end() && ids.find(e.elemB) != ids.end())
    {
      edges.push_back(e);
    }
  }
}

void CanvasClipboard::paste(GraphPanel &panel, const ImVec2 &paste_origin_canvas) const
{
  if (nodes.empty())
  {
    return;
  }

  float minX = FLT_MAX;
  float minY = FLT_MAX;
  for (const GraphData::Node &nd : nodes)
  {
    minX = eastl::min(minX, nd.x);
    minY = eastl::min(minY, nd.y);
  }
  const float dx = paste_origin_canvas.x - minX;
  const float dy = paste_origin_canvas.y - minY;

  // One consecutive id block for the whole paste batch -- allocateNodeId returns
  // max(existing) + 1 once; we increment locally for each subsequent node.
  const int baseId = panel.allocateNodeId();
  eastl::hash_map<int, int> idRemap;
  idRemap.reserve(nodes.size());
  for (int i = 0; i < (int)nodes.size(); ++i)
  {
    idRemap[nodes[i].id] = baseId + i;
  }

  eastl::vector<int> pasted;
  pasted.reserve(nodes.size());
  for (const GraphData::Node &original : nodes)
  {
    GraphData::Node copy = original; // value-copy: pins / properties / blockSize preserved
    copy.id = idRemap[original.id];
    copy.x += dx;
    copy.y += dy;
    pasted.push_back(copy.id);
    panel.addNode(eastl::move(copy)); // also pushes the new id into pendingPositionIds
  }

  const int edgeBase = panel.allocateEdgeId();
  for (int i = 0; i < (int)edges.size(); ++i)
  {
    GraphData::Edge copy = edges[i];
    copy.id = edgeBase + i;
    copy.elemA = idRemap[copy.elemA];
    copy.elemB = idRemap[copy.elemB];
    panel.addEdge(eastl::move(copy));
  }

  ne::ClearSelection();
  for (int id : pasted)
  {
    ne::SelectNode(ne::NodeId(GraphPanel::makeNodeId(id)), /*append=*/true);
  }
}
