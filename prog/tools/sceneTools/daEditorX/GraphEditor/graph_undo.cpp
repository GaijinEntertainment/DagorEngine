// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_undo.h"
#include "plugin.h"

UndoCreateNode::UndoCreateNode(GraphEditorPlg &plg, GraphData::Node node_snapshot) : plugin(plg), node(eastl::move(node_snapshot)) {}

void UndoCreateNode::restore(bool /*save_redo_data*/)
{
  // We already hold the full snapshot, so there is no redo data to capture here -- undo just
  // removes the created node (and its incident edges).
  plugin.eraseNode(node.id);
}

void UndoCreateNode::redo() { plugin.reinsertNode(node); }

size_t UndoCreateNode::size()
{
  // sizeof(*this) already includes the embedded GraphData::Node struct, which approx_node_size also
  // counts -- subtract it once so the struct is not double-counted.
  return sizeof(*this) - sizeof(GraphData::Node) + approx_node_size(node);
}

void UndoCreateNode::get_description(String &s) { s = "Create node"; }

UndoDeleteNodes::UndoDeleteNodes(GraphEditorPlg &plg, eastl::vector<GraphData::Node> removed_nodes,
  eastl::vector<GraphData::Edge> removed_edges) :
  plugin(plg), nodes(eastl::move(removed_nodes)), edges(eastl::move(removed_edges))
{}

void UndoDeleteNodes::restore(bool /*save_redo_data*/) { plugin.restoreNodesAndEdges(nodes, edges); }

void UndoDeleteNodes::redo()
{
  // Re-delete by id; eraseNodes strips each node's incident edges (exactly the ones restore()
  // re-added), leaving the graph as the original delete did.
  eastl::vector<int> ids;
  ids.reserve(nodes.size());
  for (const GraphData::Node &n : nodes)
  {
    ids.push_back(n.id);
  }
  plugin.eraseNodes(ids);
}

size_t UndoDeleteNodes::size()
{
  size_t sz = sizeof(*this);
  for (const GraphData::Node &n : nodes)
  {
    sz += approx_node_size(n);
  }
  sz += edges.size() * sizeof(GraphData::Edge);
  return sz;
}

void UndoDeleteNodes::get_description(String &s) { s = "Delete nodes"; }

UndoMoveNodes::UndoMoveNodes(GraphEditorPlg &plg, eastl::vector<NodePos> old_positions, eastl::vector<NodePos> new_positions) :
  plugin(plg), oldPositions(eastl::move(old_positions)), newPositions(eastl::move(new_positions))
{}

void UndoMoveNodes::restore(bool /*save_redo_data*/) { plugin.applyNodePositions(oldPositions); }

void UndoMoveNodes::redo() { plugin.applyNodePositions(newPositions); }

size_t UndoMoveNodes::size() { return sizeof(*this) + (oldPositions.size() + newPositions.size()) * sizeof(NodePos); }

void UndoMoveNodes::get_description(String &s) { s = "Move nodes"; }

UndoCreateEdge::UndoCreateEdge(GraphEditorPlg &plg, const GraphData::Edge &created_edge) : plugin(plg), edge(created_edge) {}

void UndoCreateEdge::restore(bool /*save_redo_data*/) { plugin.eraseEdge(edge.id); }

void UndoCreateEdge::redo() { plugin.reinsertEdge(edge); }

size_t UndoCreateEdge::size() { return sizeof(*this); }

void UndoCreateEdge::get_description(String &s) { s = "Create edge"; }

UndoDeleteEdges::UndoDeleteEdges(GraphEditorPlg &plg, eastl::vector<GraphData::Edge> removed_edges) :
  plugin(plg), edges(eastl::move(removed_edges))
{}

void UndoDeleteEdges::restore(bool /*save_redo_data*/)
{
  for (const GraphData::Edge &e : edges)
  {
    plugin.reinsertEdge(e);
  }
}

void UndoDeleteEdges::redo()
{
  for (const GraphData::Edge &e : edges)
  {
    plugin.eraseEdge(e.id);
  }
}

size_t UndoDeleteEdges::size() { return sizeof(*this) + edges.size() * sizeof(GraphData::Edge); }

void UndoDeleteEdges::get_description(String &s) { s = "Delete edges"; }

UndoSelection::UndoSelection(GraphEditorPlg &plg, GraphSelection old_selection, GraphSelection new_selection) :
  plugin(plg), oldSelection(eastl::move(old_selection)), newSelection(eastl::move(new_selection))
{}

void UndoSelection::restore(bool /*save_redo_data*/) { plugin.applySelection(oldSelection); }

void UndoSelection::redo() { plugin.applySelection(newSelection); }

size_t UndoSelection::size()
{
  return sizeof(*this) +
         (oldSelection.nodes.size() + oldSelection.links.size() + newSelection.nodes.size() + newSelection.links.size()) * sizeof(int);
}

void UndoSelection::get_description(String &s) { s = "Select"; }

UndoNodeProps::UndoNodeProps(GraphEditorPlg &plg, int node_id) : plugin(plg), nodeId(node_id)
{
  plugin.getNodeProperties(nodeId, oldProps);
  redoProps = oldProps;
}

void UndoNodeProps::restore(bool save_redo_data)
{
  if (save_redo_data)
  {
    plugin.getNodeProperties(nodeId, redoProps);
  }
  plugin.setNodeProperties(nodeId, oldProps);
}

void UndoNodeProps::redo() { plugin.setNodeProperties(nodeId, redoProps); }

size_t UndoNodeProps::size()
{
  size_t sz = sizeof(*this);
  for (const auto &pv : oldProps)
  {
    sz += pv.first.length() + pv.second.length();
  }
  for (const auto &pv : redoProps)
  {
    sz += pv.first.length() + pv.second.length();
  }
  return sz;
}

void UndoNodeProps::get_description(String &s) { s = "Change property"; }

UndoGraphSettings::UndoGraphSettings(GraphEditorPlg &plg, GraphSettings old_settings) :
  plugin(plg), oldSettings(eastl::move(old_settings)), redoSettings(oldSettings)
{}

void UndoGraphSettings::restore(bool save_redo_data)
{
  if (save_redo_data)
  {
    plugin.getGraphSettings(redoSettings);
  }
  plugin.setGraphSettings(oldSettings);
}

void UndoGraphSettings::redo() { plugin.setGraphSettings(redoSettings); }

size_t UndoGraphSettings::size()
{
  return sizeof(*this) + oldSettings.renderDir.length() + oldSettings.entityDir.length() + oldSettings.graphTextureType.length() +
         oldSettings.graphTextureWrap.length() + redoSettings.renderDir.length() + redoSettings.entityDir.length() +
         redoSettings.graphTextureType.length() + redoSettings.graphTextureWrap.length();
}

void UndoGraphSettings::get_description(String &s) { s = "Change graph settings"; }

UndoPinComment::UndoPinComment(GraphEditorPlg &plg, int node_id, int pin_index, eastl::string old_comment, eastl::string new_comment) :
  plugin(plg), nodeId(node_id), pinIndex(pin_index), oldComment(eastl::move(old_comment)), newComment(eastl::move(new_comment))
{}

void UndoPinComment::restore(bool /*save_redo_data*/) { plugin.setPinComment(nodeId, pinIndex, oldComment); }

void UndoPinComment::redo() { plugin.setPinComment(nodeId, pinIndex, newComment); }

size_t UndoPinComment::size() { return sizeof(*this) + oldComment.length() + newComment.length(); }

void UndoPinComment::get_description(String &s) { s = "Edit pin comment"; }

UndoBlockResize::UndoBlockResize(GraphEditorPlg &plg, eastl::vector<BlockSize> old_sizes, eastl::vector<BlockSize> new_sizes) :
  plugin(plg), oldSizes(eastl::move(old_sizes)), newSizes(eastl::move(new_sizes))
{}

void UndoBlockResize::restore(bool /*save_redo_data*/) { plugin.applyBlockSizes(oldSizes); }

void UndoBlockResize::redo() { plugin.applyBlockSizes(newSizes); }

size_t UndoBlockResize::size() { return sizeof(*this) + (oldSizes.size() + newSizes.size()) * sizeof(BlockSize); }

void UndoBlockResize::get_description(String &s) { s = "Resize block"; }
