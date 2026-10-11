// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "canvas_clipboard.h"

#include "graph_tab.h"

#include <EASTL/algorithm.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>

#include <imgui_node_editor.h>

namespace ne = ax::NodeEditor;

void CanvasClipboard::captureSelection(const GraphTab &graph_tab, const GraphData &graph, bool with_input_edges)
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

  // Only ids that resolve: pasteShifted reads an id absent from the remap as an outside node and
  // keeps it, so a stale selection entry would be written into a pasted edge verbatim.
  eastl::hash_set<int> ids;
  for (ne::NodeId ne_id : selected)
  {
    const int node_id = decode_node_id(ne_id.Get());
    const GraphData::Node *const node = find_node_by_id(graph, node_id);
    if (!node)
    {
      continue;
    }
    ids.insert(node_id);
    if (node->descName == "block")
    {
      eastl::vector<int> children;
      graph_tab.collectNodesInsideBlock(node_id, children);
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
    const bool aCaptured = ids.find(e.elemA) != ids.end();
    const bool bCaptured = ids.find(e.elemB) != ids.end();
    if (aCaptured && bCaptured)
    {
      edges.push_back(e);
      continue;
    }
    if (!with_input_edges || (!aCaptured && !bCaptured))
    {
      continue;
    }

    // Crossing the selection boundary. Stored edges are not oriented out->in, so ask which end
    // produces rather than assume A.
    int srcNode = 0;
    int srcPin = 0;
    if (!edge_source_pin(graph, e, srcNode, srcPin) || ids.find(srcNode) != ids.end())
    {
      continue; // no producer at all, or the captured end is it -- an outgoing edge, which is dropped
    }
    int dstNode = 0;
    int dstPin = 0;
    if (!edge_opposite_end(e, srcNode, srcPin, dstNode, dstPin))
    {
      continue;
    }
    // The consumer role is not implied: an out-to-out edge would answer the producer question with
    // the outside end and otherwise pass. A single-connect producer takes no second consumer.
    const GraphData::Pin *const consumer = find_pin(graph, dstNode, dstPin);
    const GraphData::Pin *const producer = find_pin(graph, srcNode, srcPin);
    if (consumer && consumer->role == PinRole::In && producer && !producer->singleConnect)
    {
      edges.push_back(e);
    }
  }
}

void CanvasClipboard::paste(GraphTab &graph_tab, const GraphData &graph, const ImVec2 &paste_origin_canvas,
  eastl::vector<GraphData::Node> &out_nodes, eastl::vector<GraphData::Edge> &out_edges) const
{
  if (nodes.empty())
  {
    out_nodes.clear();
    out_edges.clear();
    return;
  }

  float minX = FLT_MAX;
  float minY = FLT_MAX;
  for (const GraphData::Node &nd : nodes)
  {
    minX = eastl::min(minX, nd.x);
    minY = eastl::min(minY, nd.y);
  }
  pasteShifted(graph_tab, graph, ImVec2(paste_origin_canvas.x - minX, paste_origin_canvas.y - minY), out_nodes, out_edges);
}

void CanvasClipboard::pasteShifted(GraphTab &graph_tab, const GraphData &graph, const ImVec2 &delta,
  eastl::vector<GraphData::Node> &out_nodes, eastl::vector<GraphData::Edge> &out_edges) const
{
  out_nodes.clear();
  out_edges.clear();
  if (nodes.empty())
  {
    return;
  }

  const float dx = delta.x;
  const float dy = delta.y;

  // One consecutive id block for the whole paste batch -- allocateNodeId returns
  // max(existing) + 1 once; we increment locally for each subsequent node.
  const int baseId = graph_tab.allocateNodeId();
  eastl::hash_map<int, int> idRemap;
  idRemap.reserve(nodes.size());
  for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
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
    out_nodes.push_back(copy);            // snapshot for the paste undo entry
    graph_tab.addNode(eastl::move(copy)); // also pushes the new id into pendingPositionIds
  }

  // An id the remap does not know belongs to a node that already exists -- the producer of an input
  // edge, which only a same-graph capture carries. captureSelection puts every captured id in the
  // remap, so an unknown id is never one of them.
  const auto remapEnd = [&idRemap](int node_id) {
    const auto it = idRemap.find(node_id);
    return it != idRemap.end() ? it->second : node_id;
  };

  int edgeId = next_edge_id(graph);
  for (const GraphData::Edge &original : edges)
  {
    GraphData::Edge copy = original;
    copy.id = edgeId++;
    copy.elemA = remapEnd(copy.elemA);
    copy.elemB = remapEnd(copy.elemB);
    out_edges.push_back(copy); // snapshot for the paste undo entry
    graph_tab.addEdge(eastl::move(copy));
  }

  GraphSelection justPasted;
  justPasted.nodes = eastl::move(pasted);
  graph_tab.setPendingSelection(justPasted);
}
