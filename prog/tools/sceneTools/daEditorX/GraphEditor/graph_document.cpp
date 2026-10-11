// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_document.h"

#include <de3_interface.h>
#include <EditorCore/ec_interface.h>
#include <libTools/util/undo.h>

#include "graph_panel.h"
#include "graph_tab.h"
#include "graph_undo.h"
#include "graph_validation.h"
#include "plugin.h"

#include "pluginService/graph_tex_gen_service.h"

#include <EASTL/algorithm.h>
#include <EASTL/hash_set.h>
#include <EASTL/sort.h>

namespace
{
// The graph-level texture fields seed every node's texture declaration in compile_graph_to_blks, so
// one of them moving means the graph has to be rebuilt. Nothing else in GraphSettings reaches it.
bool compile_inputs_differ(const GraphSettings &a, const GraphSettings &b)
{
  return a.graphTextureWidth != b.graphTextureWidth || a.graphTextureHeight != b.graphTextureHeight ||
         a.graphTextureDepth != b.graphTextureDepth || a.graphTextureType != b.graphTextureType ||
         a.graphTextureWrap != b.graphTextureWrap;
}

bool heightmap_differs(const GraphSettings &a, const GraphSettings &b)
{
  return a.heightmapScale != b.heightmapScale || a.heightmapMin != b.heightmapMin || a.heightmapCellSize != b.heightmapCellSize;
}

// A re-added id goes back to the index it is removed from. Those come first, in removal order, so each index
// still holds when it is filled; the rest keep their order and append (-1).
template <class T>
void order_for_insert(const eastl::vector<T> &removed, const eastl::vector<int> &removed_indices, eastl::vector<T> &added,
  eastl::vector<int> &out_indices)
{
  eastl::vector<T> ordered;
  ordered.reserve(added.size());
  out_indices.clear();
  int dropped = 0;
  for (size_t r = 0; r < removed.size(); ++r)
  {
    const auto it = eastl::find_if(added.begin(), added.end(), [&](const T &a) { return a.id == removed[r].id; });
    if (it == added.end())
    {
      ++dropped;
      continue;
    }
    out_indices.push_back(removed_indices[r] - dropped);
    ordered.push_back(eastl::move(*it));
    added.erase(it);
  }
  for (T &a : added)
  {
    out_indices.push_back(-1);
    ordered.push_back(eastl::move(a));
  }
  added = eastl::move(ordered);
}
} // namespace

GraphDocument::GraphDocument(GraphEditorPlg &plg, GraphTab &canvas) : plugin(plg), view(canvas) {}

void GraphDocument::activateTab() { plugin.getGraphPanel().activateTabOf(*this); }

void GraphDocument::reinsertNode(const GraphData::Node &node, int index)
{
  view.addNode(node, index);
  plugin.markGraphDirtyAndRegen(*this);
}

void GraphDocument::reinsertEdge(const GraphData::Edge &edge, int index)
{
  view.addEdge(edge, index); // mutates + marks dirty + cull
}

void GraphDocument::eraseNode(int node_id)
{
  view.removeNodeById(node_id); // strips incident edges + regens internally
}

int GraphDocument::eraseEdge(int edge_id)
{
  const GraphData::Edge *const edge = find_edge_by_id(graphData, edge_id);
  const int index = edge ? static_cast<int>(edge - graphData.edges.begin()) : -1;
  view.removeEdgeById(edge_id); // mutates + marks dirty + cull
  return index;
}

eastl::vector<int> GraphDocument::eraseEdges(const eastl::vector<GraphData::Edge> &edges)
{
  eastl::vector<int> indices;
  indices.reserve(edges.size());
  for (const GraphData::Edge &e : edges)
  {
    indices.push_back(eraseEdge(e.id));
  }
  return indices;
}

void GraphDocument::eraseNodes(const eastl::vector<int> &node_ids)
{
  for (int id : node_ids)
  {
    eraseNode(id);
  }
}

void GraphDocument::restoreNodesAndEdges(const eastl::vector<GraphData::Node> &nodes, const eastl::vector<int> &node_indices,
  const eastl::vector<GraphData::Edge> &edges, const eastl::vector<int> &edge_indices)
{
  // Nodes first, then edges, so a restored edge never references a not-yet-present node.
  for (size_t i = 0; i < nodes.size(); ++i)
  {
    reinsertNode(nodes[i], node_indices[i]);
  }
  for (size_t i = 0; i < edges.size(); ++i)
  {
    reinsertEdge(edges[i], edge_indices[i]);
  }
}

void GraphDocument::applyNodePositions(const eastl::vector<NodePos> &positions)
{
  if (positions.empty())
  {
    return;
  }
  writeNodePositions(positions);
  eastl::vector<int> ids;
  ids.reserve(positions.size());
  for (const NodePos &p : positions)
  {
    ids.push_back(p.nodeId);
  }
  view.markPositionsPending(ids);
  // Node position is display-only -- do not markGraphDirtyAndRegen.
}

void GraphDocument::writeNodePositions(const eastl::vector<NodePos> &positions)
{
  mutateWithoutInvalidating([&positions](GraphData &gd) {
    for (const NodePos &p : positions)
    {
      for (GraphData::Node &n : gd.nodes)
      {
        if (n.id == p.nodeId)
        {
          n.x = p.x;
          n.y = p.y;
          break;
        }
      }
    }
  });
  ++positionsStamp;
}

void GraphDocument::applyBlockSizes(const eastl::vector<BlockSize> &sizes)
{
  mutateWithoutInvalidating([&](GraphData &gd) {
    for (const BlockSize &bs : sizes)
    {
      for (GraphData::Node &n : gd.nodes)
      {
        if (n.id == bs.nodeId)
        {
          n.blockWidth = bs.width;
          n.blockHeight = bs.height;
          break;
        }
      }
    }
  });
  // The cull cache holds the pre-undo bounds; draw_block_node pushes the restored size to ne itself.
  view.markBlockSizesChanged();
  // Block size is display-only; no regen needed.
}

void GraphDocument::applySelection(const GraphSelection &selection) { view.setPendingSelection(selection); }

void GraphDocument::getNodeProperties(int node_id, eastl::vector<eastl::pair<eastl::string, eastl::string>> &out) const
{
  out.clear();
  // Main-thread read of node data (see the snapshot note in replaceNodes).
  if (const GraphData::Node *const node = find_node_by_id(graphData, node_id))
  {
    out = node->propertyValues;
  }
}

void GraphDocument::applyNodeProperties(int node_id, const eastl::vector<eastl::pair<eastl::string, eastl::string>> &props)
{
  bool found = false;
  mutateGraphData([&](GraphData &gd) {
    for (auto &node : gd.nodes)
    {
      if (node.id == node_id)
      {
        node.propertyValues = props;
        found = true;
        break;
      }
    }
  });
  if (found)
  {
    plugin.markGraphDirtyAndRegen(*this);
  }
  // The displayed controls still show the pre-undo values; force the panel to rebuild them.
  plugin.invalidatePropertiesPanel();
}

void GraphDocument::getGraphSettings(GraphSettings &out) const
{
  // Main-thread read of graph data (see the snapshot note in replaceNodes).
  out.renderDir = graphData.renderDir;
  out.entityDir = graphData.entityDir;
  out.heightmapScale = graphData.heightmapScale;
  out.heightmapMin = graphData.heightmapMin;
  out.heightmapCellSize = graphData.heightmapCellSize;
  out.graphTextureWidth = graphData.graphTextureWidth;
  out.graphTextureHeight = graphData.graphTextureHeight;
  out.graphTextureDepth = graphData.graphTextureDepth;
  out.graphTextureType = graphData.graphTextureType;
  out.graphTextureWrap = graphData.graphTextureWrap;
}

void GraphDocument::pushHeightmapParams()
{
  IGraphTexGenService *texGen = plugin.getGraphPanel().texGenServiceFor(*this);
  if (!texGen)
  {
    return;
  }
  texGen->setHeightmapParams(effective_height(graphData.heightmapScale, DEFAULT_HEIGHT_SCALE),
    effective_height(graphData.heightmapMin, DEFAULT_HEIGHT_MIN), effective_height(graphData.heightmapCellSize, DEFAULT_CELL_SIZE));
}

void GraphDocument::writeGraphSettings(const GraphSettings &settings)
{
  GraphSettings before;
  getGraphSettings(before);
  mutateGraphData([&](GraphData &gd) {
    gd.renderDir = settings.renderDir;
    gd.entityDir = settings.entityDir;
    gd.heightmapScale = settings.heightmapScale;
    gd.heightmapMin = settings.heightmapMin;
    gd.heightmapCellSize = settings.heightmapCellSize;
    gd.graphTextureWidth = settings.graphTextureWidth;
    gd.graphTextureHeight = settings.graphTextureHeight;
    gd.graphTextureDepth = settings.graphTextureDepth;
    gd.graphTextureType = settings.graphTextureType;
    gd.graphTextureWrap = settings.graphTextureWrap;
  });
  pushHeightmapParams();
  // Which fields moved decides the work, not which control was edited. The export dirs are read when
  // textures are saved, so a change to one needs neither pass.
  if (compile_inputs_differ(before, settings))
  {
    plugin.markGraphDirtyAndRegen(*this);
  }
  else if (heightmap_differs(before, settings))
  {
    if (IGraphTexGenService *texGen = plugin.getGraphPanel().texGenServiceFor(*this))
    {
      texGen->requestRegenerate();
    }
  }
}

void GraphDocument::applyGraphSettings(const GraphSettings &settings)
{
  writeGraphSettings(settings);
  plugin.invalidatePropertiesPanel();
}

void GraphDocument::setGraphSettings(const GraphSettings &settings)
{
  GraphSettings before;
  getGraphSettings(before);
  if (settings == before)
  {
    return;
  }
  writeGraphSettings(settings);
  recordGraphSettingsChange(eastl::move(before));
}

// Invalidates on purpose: the pin jump menu re-collects its rows, comment text included, only when
// the structure revision moves.
void GraphDocument::applyPinComment(int node_id, int pin_index, const eastl::string &comment)
{
  mutateGraphData([&](GraphData &gd) {
    for (GraphData::Node &n : gd.nodes)
    {
      if (n.id == node_id && pin_index >= 0 && pin_index < static_cast<int>(n.pins.size()))
      {
        n.pins[pin_index].comment = comment;
        break;
      }
    }
  });
  // Display-only annotation: the canvas re-reads graphData each frame, so no regen is needed.
}

void GraphDocument::applyEdgeMuted(int edge_id, bool muted)
{
  bool changed = false;
  mutateGraphData([edge_id, muted, &changed](GraphData &gd) {
    auto it = eastl::find_if(gd.edges.begin(), gd.edges.end(), [edge_id](const GraphData::Edge &e) { return e.id == edge_id; });
    if (it != gd.edges.end() && it->muted != muted)
    {
      it->muted = muted;
      changed = true;
    }
  });
  if (!changed)
  {
    return;
  }
  // Endpoints did not move, so the cull cache stays valid -- but the compile result does not.
  // The canvas picks up the new dead-path set through the structure revision bumped above.
  plugin.markGraphDirtyAndRegen(*this);
}

// The selection as of frame start.
const GraphSelection &GraphDocument::recordedSelection() const { return view.getRecordedSelection(); }

// Folds this edit's selection change into the operation being recorded, so one Ctrl+Z covers both the
// edit and the reselection. Must be called between begin() and accept().
void GraphDocument::dropErasedLinksFromSelection(const eastl::vector<GraphData::Edge> &erased)
{
  if (erased.empty())
  {
    return;
  }
  const GraphSelection &oldSel = recordedSelection();

  eastl::hash_set<int> erasedIds;
  erasedIds.reserve(erased.size());
  for (const GraphData::Edge &e : erased)
  {
    erasedIds.insert(e.id);
  }
  GraphSelection newSel;
  newSel.nodes = oldSel.nodes;
  for (int id : oldSel.links)
  {
    if (erasedIds.find(id) == erasedIds.end())
    {
      newSel.links.push_back(id);
    }
  }
  foldSelectionChange(eastl::move(newSel));
}

void GraphDocument::foldSelectionChange(GraphSelection new_selection)
{
  G_ASSERT(EDITORCORE->getUndoSystem()->is_holding());
  const GraphSelection &oldSel = view.getRecordedSelection();
  if (oldSel != new_selection)
  {
    EDITORCORE->getUndoSystem()->put<UndoSelection>(*this, oldSel, eastl::move(new_selection));
  }
  view.suppressSelectionUndoThisFrame();
}

void GraphDocument::deleteNodes(const eastl::vector<int> &node_ids)
{
  replaceNodes(node_ids, {}, {}, "Delete nodes", /*select_added=*/false);
}

void GraphDocument::snapshotRemoval(const eastl::hash_set<int> &node_ids, eastl::vector<GraphData::Node> &out_nodes,
  eastl::vector<int> &out_node_indices, eastl::vector<GraphData::Edge> &out_edges, eastl::vector<int> &out_edge_indices) const
{
  out_nodes.reserve(node_ids.size());
  for (int i = 0; i < static_cast<int>(graphData.nodes.size()); ++i)
  {
    if (node_ids.count(graphData.nodes[i].id))
    {
      out_nodes.push_back(graphData.nodes[i]);
      out_node_indices.push_back(i);
    }
  }
  for (int i = 0; i < static_cast<int>(graphData.edges.size()); ++i)
  {
    const GraphData::Edge &e = graphData.edges[i];
    if (node_ids.count(e.elemA) || node_ids.count(e.elemB))
    {
      out_edges.push_back(e);
      out_edge_indices.push_back(i);
    }
  }
}

void GraphDocument::replaceNodes(const eastl::vector<int> &removed_node_ids, eastl::vector<GraphData::Node> added_nodes,
  eastl::vector<GraphData::Edge> added_edges, const char *undo_name, bool select_added)
{
  // Snapshot before erasing, lock-free: only main writes nodes and edges, and the texgen worker only the compiled BLKs.
  const eastl::hash_set<int> idSet(removed_node_ids.begin(), removed_node_ids.end());
  eastl::vector<GraphData::Node> removedNodes;
  eastl::vector<int> removedNodeIndices;
  eastl::vector<GraphData::Edge> removedEdges;
  eastl::vector<int> removedEdgeIndices;
  snapshotRemoval(idSet, removedNodes, removedNodeIndices, removedEdges, removedEdgeIndices);
  if (removedNodes.empty() && added_nodes.empty())
  {
    return;
  }

  eastl::vector<int> addedNodeIndices;
  eastl::vector<int> addedEdgeIndices;
  order_for_insert(removedNodes, removedNodeIndices, added_nodes, addedNodeIndices);
  order_for_insert(removedEdges, removedEdgeIndices, added_edges, addedEdgeIndices);

  eraseNodes(removed_node_ids);
  for (size_t i = 0; i < added_nodes.size(); ++i)
  {
    reinsertNode(added_nodes[i], addedNodeIndices[i]);
  }
  for (size_t i = 0; i < added_edges.size(); ++i)
  {
    reinsertEdge(added_edges[i], addedEdgeIndices[i]);
  }

  GraphSelection newSel;
  if (select_added)
  {
    for (const GraphData::Node &n : added_nodes)
    {
      newSel.nodes.push_back(n.id);
    }
    eastl::sort(newSel.nodes.begin(), newSel.nodes.end());
    view.setPendingSelection(newSel); // foldSelectionChange only records it
  }
  else
  {
    eastl::hash_set<int> removedEdgeIds;
    removedEdgeIds.reserve(removedEdges.size());
    for (const GraphData::Edge &e : removedEdges)
    {
      removedEdgeIds.insert(e.id);
    }
    // A re-added id (a re-synced instance) stays selected.
    eastl::hash_set<int> addedNodeIds;
    for (const GraphData::Node &n : added_nodes)
    {
      addedNodeIds.insert(n.id);
    }
    for (const GraphData::Edge &e : added_edges)
    {
      removedEdgeIds.erase(e.id);
    }
    const GraphSelection &oldSel = recordedSelection();
    for (int id : oldSel.nodes)
    {
      if (!idSet.count(id) || addedNodeIds.count(id))
      {
        newSel.nodes.push_back(id);
      }
    }
    for (int id : oldSel.links)
    {
      if (!removedEdgeIds.count(id))
      {
        newSel.links.push_back(id);
      }
    }
  }

  // Composed from the existing entries in apply order; undo replays them backwards.
  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  if (!removedNodes.empty())
  {
    undoSystem->put<UndoDeleteNodes>(*this, eastl::move(removedNodes), eastl::move(removedNodeIndices), eastl::move(removedEdges),
      eastl::move(removedEdgeIndices));
  }
  for (size_t i = 0; i < added_nodes.size(); ++i)
  {
    undoSystem->put<UndoCreateNode>(*this, eastl::move(added_nodes[i]), addedNodeIndices[i]);
  }
  for (size_t i = 0; i < added_edges.size(); ++i)
  {
    undoSystem->put<UndoCreateEdge>(*this, added_edges[i], addedEdgeIndices[i]);
  }
  foldSelectionChange(eastl::move(newSel));
  undoSystem->accept(undo_name);
}

void GraphDocument::resyncSubgraphInstances(uint32_t since_generation)
{
  eastl::vector<int> candidateIds;
  for (const GraphData::Node &n : graphData.nodes)
  {
    // Only subgraphs saved since; an older mismatch waits for its own save.
    if (is_subgraph_instance(n) && plugin.getSubgraphRegistrationGeneration(n.templateUid.c_str()) > since_generation)
    {
      candidateIds.push_back(n.id);
    }
  }

  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  bool recording = false;
  for (int id : candidateIds)
  {
    // Read after the previous replaceNodes: an edge between two instances must keep both remaps.
    GraphData::Node node;
    eastl::vector<GraphData::Edge> edges;
    eastl::vector<DroppedInstanceEdge> dropped;
    if (!resync_subgraph_instance(graphData, id, plugin.getBaseNodesBlk(), node, edges, dropped))
    {
      continue;
    }
    for (const DroppedInstanceEdge &d : dropped)
    {
      DAEDITOR3.conWarning("GraphEditor: instance '%s' id=%d: edge %d dropped, pin '%s' no longer exists", node.descName.c_str(), id,
        d.edgeId, d.pinName.c_str());
    }
    // One entry: each replaceNodes nests inside it.
    if (!recording)
    {
      undoSystem->begin();
      recording = true;
    }
    eastl::vector<GraphData::Node> added;
    added.push_back(eastl::move(node));
    replaceNodes({id}, eastl::move(added), eastl::move(edges), "Update subgraph instances", /*select_added=*/false);
  }
  if (recording)
  {
    undoSystem->accept("Update subgraph instances");
  }
}

void GraphDocument::collectReplacedEdges(int node_id, int pin_index, int except_edge_id, eastl::vector<GraphData::Edge> &out) const
{
  const GraphData::Pin *const pin = find_pin(graphData, node_id, pin_index);
  if (!pin || !pin->singleConnect)
  {
    return;
  }

  eastl::vector<PinEdge> pinEdges;
  collect_pin_edges(graphData, node_id, pin_index, pinEdges);
  for (const PinEdge &pinEdge : pinEdges)
  {
    // Callers ask about both ends, so an edge between two single-connect pins must not list twice.
    const bool listed = eastl::any_of(out.begin(), out.end(), [&](const GraphData::Edge &e) { return e.id == pinEdge.edgeId; });
    if (pinEdge.edgeId == except_edge_id || listed)
    {
      continue;
    }
    if (const GraphData::Edge *const e = find_edge_by_id(graphData, pinEdge.edgeId))
    {
      out.push_back(*e);
    }
  }
}

void GraphDocument::recordConnectEdge(GraphData::Edge edge)
{
  // hideSingleConnections lets the drag through by hiding the incumbent of either end, so both ends
  // are asked here. Without this the pin keeps two edges and the single-driver contract is a lie.
  eastl::vector<GraphData::Edge> replacedEdges;
  collectReplacedEdges(edge.elemA, edge.pinA, /*except_edge_id=*/-1, replacedEdges);
  collectReplacedEdges(edge.elemB, edge.pinB, /*except_edge_id=*/-1, replacedEdges);

  eastl::vector<int> replacedIndices = eraseEdges(replacedEdges);
  reinsertEdge(edge);

  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  // Apply order; undo replays it backwards, so the evicted edge comes back last.
  if (!replacedEdges.empty())
  {
    dropErasedLinksFromSelection(replacedEdges);
    undoSystem->put<UndoDeleteEdges>(*this, replacedEdges, eastl::move(replacedIndices));
  }
  undoSystem->put<UndoCreateEdge>(*this, edge);
  undoSystem->accept("Create edge");
}

void GraphDocument::deleteEdges(const eastl::vector<int> &edge_ids)
{
  if (edge_ids.empty())
  {
    return;
  }

  // Snapshot the edges before erasing so undo can restore them. Main-thread read of edge data (see
  // the snapshot note in replaceNodes).
  eastl::vector<GraphData::Edge> removedEdges;
  removedEdges.reserve(edge_ids.size());
  for (int id : edge_ids)
  {
    if (const GraphData::Edge *const edge = find_edge_by_id(graphData, id))
    {
      removedEdges.push_back(*edge);
    }
  }
  if (removedEdges.empty())
  {
    return;
  }

  eastl::vector<int> removedIndices = eraseEdges(removedEdges);

  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  dropErasedLinksFromSelection(removedEdges);
  undoSystem->put<UndoDeleteEdges>(*this, eastl::move(removedEdges), eastl::move(removedIndices));
  undoSystem->accept("Delete edges");
}

void GraphDocument::toggleEdgeMuted(int edge_id)
{
  // Main-thread read of edge data (see the snapshot note in replaceNodes).
  const GraphData::Edge *const edge = find_edge_by_id(graphData, edge_id);
  if (!edge)
  {
    return;
  }
  const bool oldMuted = edge->muted;

  applyEdgeMuted(edge_id, !oldMuted);

  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  undoSystem->put<UndoToggleEdgeMuted>(*this, edge_id, oldMuted);
  undoSystem->accept(oldMuted ? "Unmute edge" : "Mute edge");
}

void GraphDocument::setEdgesMuted(const eastl::vector<int> &edge_ids, bool muted)
{
  // Main-thread read of edge data (see the snapshot note in replaceNodes). Edges already in
  // the wanted state are skipped, or undo would restore ones this never touched.
  eastl::vector<int> wanted(edge_ids);
  eastl::sort(wanted.begin(), wanted.end());

  eastl::vector<int> changed;
  changed.reserve(wanted.size());
  for (const GraphData::Edge &e : graphData.edges)
  {
    if (e.muted != muted && eastl::binary_search(wanted.begin(), wanted.end(), e.id))
    {
      changed.push_back(e.id);
    }
  }
  if (changed.empty())
  {
    return;
  }

  // One mutation and one regen for the batch: applyEdgeMuted per edge would bump the structure revision
  // and wake the compile worker once per edge.
  // Selected by the same test that built `changed`, off the sorted `wanted`: two filters that have
  // to agree would be one more thing to keep in step.
  mutateGraphData([&wanted, muted](GraphData &gd) {
    for (GraphData::Edge &e : gd.edges)
    {
      if (e.muted != muted && eastl::binary_search(wanted.begin(), wanted.end(), e.id))
      {
        e.muted = muted;
      }
    }
  });
  plugin.markGraphDirtyAndRegen(*this);

  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  for (int edgeId : changed)
  {
    undoSystem->put<UndoToggleEdgeMuted>(*this, edgeId, !muted);
  }
  undoSystem->accept(muted ? "Mute edges" : "Unmute edges");
}

void GraphDocument::setPinComment(int node_id, int pin_index, const eastl::string &new_comment)
{
  // Read the current comment as the undo's old value (main-thread read; see replaceNodes).
  eastl::string oldComment;
  bool found = false;
  const GraphData::Node *const node = find_node_by_id(graphData, node_id);
  if (node && pin_index >= 0 && pin_index < static_cast<int>(node->pins.size()))
  {
    oldComment = node->pins[pin_index].comment;
    found = true;
  }
  if (!found || oldComment == new_comment)
  {
    return;
  }

  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  undoSystem->put<UndoPinComment>(*this, node_id, pin_index, oldComment, new_comment);
  applyPinComment(node_id, pin_index, new_comment);
  undoSystem->accept("Edit pin comment");
}

void GraphDocument::setNodeProperty(int node_id, const eastl::string &prop_name, eastl::string value, bool record_undo)
{
  if (!find_node_by_id(graphData, node_id))
  {
    return;
  }

  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  if (record_undo)
  {
    undoSystem->put<UndoNodeProps>(*this, node_id);
  }

  // Re-find the node inside mutateGraphData so the mutable reference comes from the locked-access
  // path. node_id is stable across the lambda; main is the only writer that could remove a node.
  mutateGraphData([&](GraphData &gd) {
    for (auto &node : gd.nodes)
    {
      if (node.id != node_id)
      {
        continue;
      }
      bool found = false;
      for (auto &pv : node.propertyValues)
      {
        if (pv.first == prop_name)
        {
          pv.second = eastl::move(value);
          found = true;
          break;
        }
      }
      if (!found)
      {
        node.propertyValues.emplace_back(prop_name, eastl::move(value));
      }
      break;
    }
  });

  plugin.markGraphDirtyAndRegen(*this);
  undoSystem->accept("Change property");
}

void GraphDocument::createNode(GraphData::Node node)
{
  reinsertNode(node);

  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  undoSystem->put<UndoCreateNode>(*this, eastl::move(node));
  undoSystem->accept("Create node");
}

void GraphDocument::recordPaste(eastl::vector<GraphData::Node> pasted_nodes, eastl::vector<GraphData::Edge> pasted_edges,
  const char *undo_name)
{
  if (pasted_nodes.empty())
  {
    return;
  }

  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  for (const GraphData::Node &n : pasted_nodes)
  {
    undoSystem->put<UndoCreateNode>(*this, n);
  }
  for (const GraphData::Edge &e : pasted_edges)
  {
    undoSystem->put<UndoCreateEdge>(*this, e);
  }
  // Paste clears the selection and selects the pasted nodes; fold that so undo restores the prior
  // selection (and removes the nodes) while redo reselects the paste.
  GraphSelection newSel;
  newSel.nodes.reserve(pasted_nodes.size());
  for (const GraphData::Node &n : pasted_nodes)
  {
    newSel.nodes.push_back(n.id);
  }
  foldSelectionChange(eastl::move(newSel));
  undoSystem->accept(undo_name);
}

int GraphDocument::recordAddConnectedNode(GraphData::Node node, int source_node, int source_pin)
{
  const int nodeId = node.id;

  // Before the pin is chosen: validate_new_edge is a whole-graph solver. Also regenerates, which is
  // what covers the unconnected case.
  reinsertNode(node);

  const int targetPin = find_connectable_pin(graphData, source_node, source_pin, nodeId);

  const GraphData::Pin *const srcPin = find_pin(graphData, source_node, source_pin);
  const bool srcIsOutput = srcPin && srcPin->role == PinRole::Out;

  // Only the source can hold one: the target pin belongs to the node just inserted.
  eastl::vector<GraphData::Edge> replacedEdges;
  if (targetPin >= 0)
  {
    collectReplacedEdges(source_node, source_pin, /*except_edge_id=*/-1, replacedEdges);
  }

  GraphData::Edge added;
  bool haveEdge = false;
  if (targetPin >= 0)
  {
    // Stored orientation is always (Out -> In), as in the load path.
    added.id = next_edge_id(graphData);
    added.elemA = srcIsOutput ? source_node : nodeId;
    added.pinA = srcIsOutput ? source_pin : targetPin;
    added.elemB = srcIsOutput ? nodeId : source_node;
    added.pinB = srcIsOutput ? targetPin : source_pin;
    haveEdge = true;
  }

  eastl::vector<int> replacedIndices = eraseEdges(replacedEdges);
  if (haveEdge)
  {
    reinsertEdge(added);
  }

  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  // Put in apply order; undo replays it backwards, so the evicted edge comes back last.
  if (!replacedEdges.empty())
  {
    undoSystem->put<UndoDeleteEdges>(*this, replacedEdges, eastl::move(replacedIndices));
  }
  undoSystem->put<UndoCreateNode>(*this, eastl::move(node));
  if (haveEdge)
  {
    undoSystem->put<UndoCreateEdge>(*this, added);
  }
  GraphSelection newSel;
  newSel.nodes.push_back(nodeId);
  foldSelectionChange(eastl::move(newSel));
  undoSystem->accept("Add node");

  return targetPin;
}

int GraphDocument::recordInsertNodeOnPin(GraphData::Node node, int anchor_node, int anchor_pin, int anchor_edge_id)
{
  // A stale anchor leaves the feed pointing at nothing, which find_connectable_pin refuses below --
  // so it lands on the unconnected-create path rather than returning a node that was never made.
  SpliceEnds ends;
  splice_ends(graphData, anchor_node, anchor_pin, anchor_edge_id, ends);

  const int nodeId = node.id;

  reinsertNode(node); // before any pin is chosen: the two solvers below are whole-graph

  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  // An input, except on an In anchor with no driver: there the anchor feeds and takes an output.
  const int wiredPin = find_connectable_pin(graphData, ends.feed.node, ends.feed.pin, nodeId);
  if (wiredPin < 0)
  {
    // Keeping the node beats discarding what the user dropped; every edge stays as it was.
    undoSystem->begin();
    undoSystem->put<UndoCreateNode>(*this, eastl::move(node));
    GraphSelection unconnectedSel;
    unconnectedSel.nodes.push_back(nodeId);
    foldSelectionChange(eastl::move(unconnectedSel));
    undoSystem->accept("Add node");
    return -1;
  }

  const GraphData::Pin *const feedPinDesc = find_pin(graphData, ends.feed.node, ends.feed.pin);
  const bool feedIsOutput = feedPinDesc && feedPinDesc->role == PinRole::Out;

  eastl::vector<GraphData::Edge> erasedEdges;
  eastl::vector<int> erasedIndices;
  eastl::vector<GraphData::Edge> addedEdges;

  // The feed goes in before the consumers are asked: check_type_correct narrows over the whole
  // graph, so a node whose pins share a typeGroup reports its real output type only once fed.
  collectReplacedEdges(ends.feed.node, ends.feed.pin, /*except_edge_id=*/-1, erasedEdges);
  erasedIndices = eraseEdges(erasedEdges);

  GraphData::Edge feed;
  feed.id = next_edge_id(graphData);
  feed.elemA = feedIsOutput ? ends.feed.node : nodeId;
  feed.pinA = feedIsOutput ? ends.feed.pin : wiredPin;
  feed.elemB = feedIsOutput ? nodeId : ends.feed.node;
  feed.pinB = feedIsOutput ? wiredPin : ends.feed.pin;
  reinsertEdge(feed);
  addedEdges.push_back(feed);

  for (const SplicePin &consumer : ends.sinks)
  {
    int outPin = -1;
    for (int i = 0; i < static_cast<int>(node.pins.size()); ++i)
    {
      if (node.pins[i].role == PinRole::Out && validate_new_edge(graphData, consumer.node, consumer.pin, nodeId, i))
      {
        outPin = i;
        break;
      }
    }
    if (outPin < 0)
    {
      continue; // keeps its edge, unless a single-connect feed pin already evicted it
    }

    if (const GraphData::Edge *const original = find_edge_by_id(graphData, consumer.edgeId))
    {
      erasedEdges.push_back(*original);
      erasedIndices.push_back(eraseEdge(consumer.edgeId));
    }

    GraphData::Edge moved;
    moved.id = next_edge_id(graphData);
    moved.elemA = nodeId;
    moved.pinA = outPin;
    moved.elemB = consumer.node;
    moved.pinB = consumer.pin;
    moved.muted = consumer.muted; // the mute rides this hop, so a muted path stays off
    reinsertEdge(moved);
    addedEdges.push_back(moved);

    if (node.pins[outPin].singleConnect)
    {
      break; // a second would evict the first
    }
  }

  undoSystem->begin();
  if (!erasedEdges.empty())
  {
    undoSystem->put<UndoDeleteEdges>(*this, erasedEdges, eastl::move(erasedIndices));
  }
  undoSystem->put<UndoCreateNode>(*this, eastl::move(node));
  for (const GraphData::Edge &added : addedEdges)
  {
    undoSystem->put<UndoCreateEdge>(*this, added);
  }
  GraphSelection newSel;
  newSel.nodes.push_back(nodeId);
  foldSelectionChange(eastl::move(newSel));
  const bool spliced = addedEdges.size() > 1; // the feed is always there; more means a wire moved
  undoSystem->accept(spliced ? "Insert node into connection" : "Add node");

  return wiredPin;
}

void GraphDocument::recordRemoveKeepingConnections(eastl::vector<GraphData::Node> removed_nodes, eastl::vector<int> node_indices,
  eastl::vector<GraphData::Edge> removed_edges, eastl::vector<int> edge_indices, eastl::vector<GraphData::Edge> bridge_edges)
{
  if (removed_nodes.empty())
  {
    return;
  }

  // One UndoDeleteNodes (the removed nodes + their incident edges) followed by a UndoCreateEdge per
  // bridge edge. The holder redoes forward (delete, then add bridges) and undoes in reverse (remove
  // bridges, then restore the sub-graph), reproducing/reverting the splice exactly.
  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  undoSystem->put<UndoDeleteNodes>(*this, eastl::move(removed_nodes), eastl::move(node_indices), eastl::move(removed_edges),
    eastl::move(edge_indices));
  for (const GraphData::Edge &e : bridge_edges)
  {
    undoSystem->put<UndoCreateEdge>(*this, e);
  }
  // The splice clears the selection (it removed the selected nodes); fold that so undo brings the
  // nodes back selected and redo clears again.
  foldSelectionChange(GraphSelection());
  undoSystem->accept("Remove keeping connections");
}

void GraphDocument::recordReconnectEdge(const GraphData::Edge &removed_edge, int removed_edge_index, const GraphData::Edge *added_edge)
{
  // The caller dropped the rerouted edge and inserted the new one already, so only what that new
  // edge displaces is left, and it belongs in this same entry. A reroute stores a new edge rather
  // than moving one, so the anchor end takes a new connection too and both ends are asked.
  eastl::vector<GraphData::Edge> replacedEdges;
  if (added_edge)
  {
    collectReplacedEdges(added_edge->elemA, added_edge->pinA, added_edge->id, replacedEdges);
    collectReplacedEdges(added_edge->elemB, added_edge->pinB, added_edge->id, replacedEdges);
  }
  eastl::vector<int> replacedIndices = eraseEdges(replacedEdges);

  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  eastl::vector<GraphData::Edge> removed;
  removed.push_back(removed_edge);
  undoSystem->put<UndoDeleteEdges>(*this, eastl::move(removed), eastl::vector<int>{removed_edge_index});
  if (!replacedEdges.empty())
  {
    dropErasedLinksFromSelection(replacedEdges);
    undoSystem->put<UndoDeleteEdges>(*this, replacedEdges, eastl::move(replacedIndices));
  }
  if (added_edge)
  {
    undoSystem->put<UndoCreateEdge>(*this, *added_edge);
  }
  undoSystem->accept("Reconnect edge");
}

void GraphDocument::recordSelectionChange(GraphSelection old_selection, GraphSelection new_selection)
{
  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  undoSystem->put<UndoSelection>(*this, eastl::move(old_selection), eastl::move(new_selection));
  undoSystem->accept("Select");
}

void GraphDocument::recordGraphSettingsChange(GraphSettings old_settings)
{
  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  undoSystem->put<UndoGraphSettings>(*this, eastl::move(old_settings));
  undoSystem->accept("Change graph settings");
}

void GraphDocument::commitNodeTransforms(eastl::vector<NodePos> old_positions, eastl::vector<NodePos> new_positions,
  eastl::vector<BlockSize> old_sizes, eastl::vector<BlockSize> new_sizes)
{
  applyNodePositions(new_positions); // commit positions to graphData + push to ne (no-op if empty)
  // Sizes are already committed to graphData live by syncBlockSizes; only the undo entry is needed.

  // Fold a corner resize (which moves and resizes the same block) into one entry, so a single Ctrl+Z
  // restores both -- the deferred SetNodePosition + SetGroupSize on undo re-anchor the block correctly.
  const bool hasResize = !old_sizes.empty();
  UndoSystem *undoSystem = EDITORCORE->getUndoSystem();
  undoSystem->begin();
  if (!old_positions.empty())
  {
    undoSystem->put<UndoMoveNodes>(*this, eastl::move(old_positions), eastl::move(new_positions));
  }
  if (hasResize)
  {
    undoSystem->put<UndoBlockResize>(*this, eastl::move(old_sizes), eastl::move(new_sizes));
  }
  undoSystem->accept(hasResize ? "Resize block" : "Move nodes");
}
