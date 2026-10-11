// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_undo.h"
#include "graph_document.h"

UndoCreateNode::UndoCreateNode(GraphDocument &document, GraphData::Node node_snapshot, int node_index) :
  doc(document), node(eastl::move(node_snapshot)), nodeIndex(node_index)
{}

void UndoCreateNode::restore(bool /*save_redo_data*/)
{
  doc.activateTab();
  // We already hold the full snapshot, so there is no redo data to capture here -- undo just
  // removes the created node (and its incident edges).
  doc.eraseNode(node.id);
}

void UndoCreateNode::redo()
{
  doc.activateTab();
  doc.reinsertNode(node, nodeIndex);
}

size_t UndoCreateNode::size()
{
  // sizeof(*this) already includes the embedded GraphData::Node struct, which approx_node_size also
  // counts -- subtract it once so the struct is not double-counted.
  return sizeof(*this) - sizeof(GraphData::Node) + approx_node_size(node);
}

void UndoCreateNode::get_description(String &s) { s = "Create node"; }

UndoDeleteNodes::UndoDeleteNodes(GraphDocument &document, eastl::vector<GraphData::Node> removed_nodes,
  eastl::vector<int> node_indices, eastl::vector<GraphData::Edge> removed_edges, eastl::vector<int> edge_indices) :
  doc(document),
  nodes(eastl::move(removed_nodes)),
  nodeIndices(eastl::move(node_indices)),
  edges(eastl::move(removed_edges)),
  edgeIndices(eastl::move(edge_indices))
{
  G_ASSERT(nodes.size() == nodeIndices.size() && edges.size() == edgeIndices.size());
}

void UndoDeleteNodes::restore(bool /*save_redo_data*/)
{
  doc.activateTab();
  doc.restoreNodesAndEdges(nodes, nodeIndices, edges, edgeIndices);
}

void UndoDeleteNodes::redo()
{
  doc.activateTab();
  // Re-delete by id; eraseNodes strips each node's incident edges (exactly the ones restore()
  // re-added), leaving the graph as the original delete did.
  eastl::vector<int> ids;
  ids.reserve(nodes.size());
  for (const GraphData::Node &n : nodes)
  {
    ids.push_back(n.id);
  }
  doc.eraseNodes(ids);
}

size_t UndoDeleteNodes::size()
{
  size_t sz = sizeof(*this);
  for (const GraphData::Node &n : nodes)
  {
    sz += approx_node_size(n);
  }
  sz += edges.size() * sizeof(GraphData::Edge);
  sz += (nodeIndices.size() + edgeIndices.size()) * sizeof(int);
  return sz;
}

void UndoDeleteNodes::get_description(String &s) { s = "Delete nodes"; }

UndoMoveNodes::UndoMoveNodes(GraphDocument &document, eastl::vector<NodePos> old_positions, eastl::vector<NodePos> new_positions) :
  doc(document), oldPositions(eastl::move(old_positions)), newPositions(eastl::move(new_positions))
{}

void UndoMoveNodes::restore(bool /*save_redo_data*/)
{
  doc.activateTab();
  doc.applyNodePositions(oldPositions);
}

void UndoMoveNodes::redo()
{
  doc.activateTab();
  doc.applyNodePositions(newPositions);
}

size_t UndoMoveNodes::size() { return sizeof(*this) + (oldPositions.size() + newPositions.size()) * sizeof(NodePos); }

void UndoMoveNodes::get_description(String &s) { s = "Move nodes"; }

UndoCreateEdge::UndoCreateEdge(GraphDocument &document, const GraphData::Edge &created_edge, int edge_index) :
  doc(document), edge(created_edge), edgeIndex(edge_index)
{}

void UndoCreateEdge::restore(bool /*save_redo_data*/)
{
  doc.activateTab();
  doc.eraseEdge(edge.id);
}

void UndoCreateEdge::redo()
{
  doc.activateTab();
  doc.reinsertEdge(edge, edgeIndex);
}

size_t UndoCreateEdge::size() { return sizeof(*this); }

void UndoCreateEdge::get_description(String &s) { s = "Create edge"; }

UndoDeleteEdges::UndoDeleteEdges(GraphDocument &document, eastl::vector<GraphData::Edge> removed_edges,
  eastl::vector<int> edge_indices) :
  doc(document), edges(eastl::move(removed_edges)), edgeIndices(eastl::move(edge_indices))
{
  G_ASSERT(edges.size() == edgeIndices.size());
}

void UndoDeleteEdges::restore(bool /*save_redo_data*/)
{
  doc.activateTab();
  // Backwards: an index holds only once the edges erased after it are back.
  for (int i = static_cast<int>(edges.size()) - 1; i >= 0; --i)
  {
    doc.reinsertEdge(edges[i], edgeIndices[i]);
  }
}

void UndoDeleteEdges::redo()
{
  doc.activateTab();
  for (const GraphData::Edge &e : edges)
  {
    doc.eraseEdge(e.id);
  }
}

size_t UndoDeleteEdges::size() { return sizeof(*this) + edges.size() * sizeof(GraphData::Edge) + edgeIndices.size() * sizeof(int); }

void UndoDeleteEdges::get_description(String &s) { s = "Delete edges"; }

UndoToggleEdgeMuted::UndoToggleEdgeMuted(GraphDocument &document, int edge_id, bool old_muted) :
  doc(document), edgeId(edge_id), oldMuted(old_muted)
{}

void UndoToggleEdgeMuted::restore(bool /*save_redo_data*/)
{
  doc.activateTab();
  doc.applyEdgeMuted(edgeId, oldMuted);
}

void UndoToggleEdgeMuted::redo()
{
  doc.activateTab();
  doc.applyEdgeMuted(edgeId, !oldMuted);
}

size_t UndoToggleEdgeMuted::size() { return sizeof(*this); }

void UndoToggleEdgeMuted::get_description(String &s) { s = oldMuted ? "Unmute edge" : "Mute edge"; }

UndoSelection::UndoSelection(GraphDocument &document, GraphSelection old_selection, GraphSelection new_selection) :
  doc(document), oldSelection(eastl::move(old_selection)), newSelection(eastl::move(new_selection))
{}

void UndoSelection::restore(bool /*save_redo_data*/)
{
  doc.activateTab();
  doc.applySelection(oldSelection);
}

void UndoSelection::redo()
{
  doc.activateTab();
  doc.applySelection(newSelection);
}

size_t UndoSelection::size()
{
  return sizeof(*this) +
         (oldSelection.nodes.size() + oldSelection.links.size() + newSelection.nodes.size() + newSelection.links.size()) * sizeof(int);
}

void UndoSelection::get_description(String &s) { s = "Select"; }

UndoNodeProps::UndoNodeProps(GraphDocument &document, int node_id) : doc(document), nodeId(node_id)
{
  doc.getNodeProperties(nodeId, oldProps);
  redoProps = oldProps;
}

void UndoNodeProps::restore(bool save_redo_data)
{
  doc.activateTab();
  if (save_redo_data)
  {
    doc.getNodeProperties(nodeId, redoProps);
  }
  doc.applyNodeProperties(nodeId, oldProps);
}

void UndoNodeProps::redo()
{
  doc.activateTab();
  doc.applyNodeProperties(nodeId, redoProps);
}

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

UndoGraphSettings::UndoGraphSettings(GraphDocument &document, GraphSettings old_settings) :
  doc(document), oldSettings(eastl::move(old_settings)), redoSettings(oldSettings)
{}

void UndoGraphSettings::restore(bool save_redo_data)
{
  doc.activateTab();
  if (save_redo_data)
  {
    doc.getGraphSettings(redoSettings);
  }
  doc.applyGraphSettings(oldSettings);
}

void UndoGraphSettings::redo()
{
  doc.activateTab();
  doc.applyGraphSettings(redoSettings);
}

size_t UndoGraphSettings::size()
{
  return sizeof(*this) + oldSettings.renderDir.length() + oldSettings.entityDir.length() + oldSettings.graphTextureType.length() +
         oldSettings.graphTextureWrap.length() + redoSettings.renderDir.length() + redoSettings.entityDir.length() +
         redoSettings.graphTextureType.length() + redoSettings.graphTextureWrap.length();
}

void UndoGraphSettings::get_description(String &s) { s = "Change graph settings"; }

UndoPinComment::UndoPinComment(GraphDocument &document, int node_id, int pin_index, eastl::string old_comment,
  eastl::string new_comment) :
  doc(document), nodeId(node_id), pinIndex(pin_index), oldComment(eastl::move(old_comment)), newComment(eastl::move(new_comment))
{}

void UndoPinComment::restore(bool /*save_redo_data*/)
{
  doc.activateTab();
  doc.applyPinComment(nodeId, pinIndex, oldComment);
}

void UndoPinComment::redo()
{
  doc.activateTab();
  doc.applyPinComment(nodeId, pinIndex, newComment);
}

size_t UndoPinComment::size() { return sizeof(*this) + oldComment.length() + newComment.length(); }

void UndoPinComment::get_description(String &s) { s = "Edit pin comment"; }

UndoBlockResize::UndoBlockResize(GraphDocument &document, eastl::vector<BlockSize> old_sizes, eastl::vector<BlockSize> new_sizes) :
  doc(document), oldSizes(eastl::move(old_sizes)), newSizes(eastl::move(new_sizes))
{}

void UndoBlockResize::restore(bool /*save_redo_data*/)
{
  doc.activateTab();
  doc.applyBlockSizes(oldSizes);
}

void UndoBlockResize::redo()
{
  doc.activateTab();
  doc.applyBlockSizes(newSizes);
}

size_t UndoBlockResize::size() { return sizeof(*this) + (oldSizes.size() + newSizes.size()) * sizeof(BlockSize); }

void UndoBlockResize::get_description(String &s) { s = "Resize block"; }
