// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_critSec.h>

#include <graphEditor/graph_data.h>

#include <EASTL/algorithm.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/string.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>

#include "graph_edit_types.h"
#include "graph_resolutions.h"

class GraphEditorPlg;
class GraphTab;

// One open graph and the lock that guards it.
class GraphDocument
{
public:
  GraphDocument(GraphEditorPlg &plg, GraphTab &canvas);

  // Brings the tab holding this graph to the front. Every undo entry does it before applying, so
  // the change lands on a canvas the user can see.
  void activateTab();

  // Lock-free on purpose: main is the only writer of the source fields, and the texgen worker reads
  // them under the mutex. The compiled BLKs are not covered by that -- see graphMutex.
  const GraphData &getGraphData() const { return graphData; }

  // The only legal write path for the source fields. Without the mutex the texgen worker can read
  // a half-mutated graph.
  template <class Fn>
  void mutateGraphData(Fn &&fn)
  {
    WinAutoLock lock(graphMutex);
    fn(graphData);
    interlocked_increment(structureRevision);
    dropSeedsOfGoneInstances();
  }

  // constraint: only for fields no derived cache reads - block size, sourcePath, compiled output (node x/y: writeNodePositions).
  // The assert catches added or removed nodes and edges, not an in-place rewrite.
  template <class Fn>
  void mutateWithoutInvalidating(Fn &&fn)
  {
    WinAutoLock lock(graphMutex);
    const size_t nodesBefore = graphData.nodes.size();
    const size_t edgesBefore = graphData.edges.size();
    fn(graphData);
    G_ASSERT(graphData.nodes.size() == nodesBefore && graphData.edges.size() == edgesBefore);
    G_UNUSED(nodesBefore);
    G_UNUSED(edgesBefore);
  }

  // Counts only the writes a derived cache depends on. Relaxed: losing a race costs one late frame.
  uint64_t getStructureRevision() const { return interlocked_relaxed_load(structureRevision); }
  // Moves leave the structure revision alone, so whatever shows the node order counts them here.
  uint64_t getPositionsStamp() const { return positionsStamp; }
  // applyNodePositions without the push to ne, for positions read from it.
  void writeNodePositions(const eastl::vector<NodePos> &positions);

  // For worker-thread readers that must not race a main-thread write.
  template <class Fn>
  void readGraphData(Fn &&fn)
  {
    WinAutoLock lock(graphMutex);
    fn(static_cast<const GraphData &>(graphData));
  }

  // The service keeps this pointer for the life of the graph and writes mainGraphBlk through it,
  // so it cannot take a copy. The one mutable handle that escapes.
  GraphData *dataForTexGenService() { return &graphData; }

  // Keyed by node id, so a node deleted between compile and drain drops out.
  struct CompileResults
  {
    eastl::vector<eastl::pair<int, eastl::vector<eastl::string>>> pinCustomTextureNames;
    // Empty unless the compile expanded subgraphs; still replaces the stored map, unless failed.
    eastl::vector<eastl::pair<int, NodeResolution>> nodeResolutions;
    // The last good compile's names and resolutions stay, as its BLKs do.
    bool failed = false;
    eastl::vector<eastl::string> failedSubgraphs;
  };

  // Worker thread, already inside the critical section.
  void setPendingCompileResults(CompileResults results)
  {
    // constraint: a failed compile commits no BLKs, so a good result main has not drained yet still describes the ones in use.
    if (results.failed && interlocked_relaxed_load(hasPendingCompileResults) && !pendingCompileResults.failed)
    {
      pendingCompileResults.failedSubgraphs = eastl::move(results.failedSubgraphs);
      return;
    }
    pendingCompileResults = eastl::move(results);
    interlocked_release_store(hasPendingCompileResults, 1);
  }

  // constraint: main thread only - the texture preview reads Pin::customTextureName without the
  // mutex, so main stays the only writer of nodes and pins.
  void applyPendingCompileResults()
  {
    if (!interlocked_relaxed_load(hasPendingCompileResults))
    {
      return;
    }
    mutateWithoutInvalidating([&](GraphData &gd) {
      failedSubgraphs = eastl::move(pendingCompileResults.failedSubgraphs);
      if (pendingCompileResults.failed)
      {
        pendingCompileResults = CompileResults();
        interlocked_release_store(hasPendingCompileResults, 0);
        return;
      }
      eastl::hash_map<int, int> idToIdx;
      build_node_index(gd, idToIdx);
      for (auto &entry : pendingCompileResults.pinCustomTextureNames)
      {
        const auto it = idToIdx.find(entry.first);
        if (it == idToIdx.end())
        {
          continue;
        }
        GraphData::Node &n = gd.nodes[it->second];
        const int count = static_cast<int>(eastl::min(entry.second.size(), n.pins.size()));
        for (int j = 0; j < count; ++j)
        {
          n.pins[j].customTextureName = eastl::move(entry.second[j]);
        }
      }
      const bool wasEmpty = compiledNodeResolutions.empty();
      compiledNodeResolutions.clear();
      compiledNodeResolutions.reserve(pendingCompileResults.nodeResolutions.size());
      // constraint: an id match is an identity match only because the one edit that re-adds a removed id,
      // the instance re-sync, re-adds the same instance; any other would inherit the old node's pending result.
      for (auto &entry : pendingCompileResults.nodeResolutions)
      {
        if (idToIdx.find(entry.first) != idToIdx.end())
        {
          compiledNodeResolutions[entry.first] = eastl::move(entry.second);
        }
      }
      // An empty map draining onto an empty one changes nothing the canvas would rebuild for.
      if (!wasEmpty || !compiledNodeResolutions.empty())
      {
        ++compiledResolutionsStamp;
      }
      pendingCompileResults = CompileResults();
      interlocked_release_store(hasPendingCompileResults, 0);
    });
  }

  // constraint: node ids restart per file, so compile output kept across a load aliases the next
  // graph's nodes. Replace the graph only through here.
  template <class Fn>
  void replaceGraphData(Fn &&fn)
  {
    WinAutoLock lock(graphMutex);
    fn(graphData);
    interlocked_increment(structureRevision);
    pendingCompileResults = CompileResults();
    interlocked_release_store(hasPendingCompileResults, 0);
    compiledNodeResolutions.clear();
    ++compiledResolutionsStamp;
    failedSubgraphs.clear();
  }

  // Changes whenever the seed map does, so a drain that replaces it and a load that clears it both
  // move it.
  uint64_t getCompiledResolutionsStamp() const { return compiledResolutionsStamp; }

  // The last good compile's per-node resolutions; may be one edit out of date.
  const eastl::hash_map<int, NodeResolution> &getCompiledNodeResolutions() const { return compiledNodeResolutions; }
  // Stems, top-level instance first, down to the subgraph the last compile could not splice; empty after any other outcome.
  const eastl::vector<eastl::string> &getFailedSubgraphs() const { return failedSubgraphs; }

  // Snapshotted by the PropertiesPanel before a graph-field edit, so this one is not undo-only.
  void getGraphSettings(GraphSettings &out) const;
  // Appends what removing node_ids takes out: those nodes and every edge touching one, each with its list
  // index, ascending, as UndoDeleteNodes takes them.
  void snapshotRemoval(const eastl::hash_set<int> &node_ids, eastl::vector<GraphData::Node> &out_nodes,
    eastl::vector<int> &out_node_indices, eastl::vector<GraphData::Edge> &out_edges, eastl::vector<int> &out_edge_indices) const;

  // Pushes the heightmap fields, the unset ones as their defaults (a raw UNSET_HEIGHT would reach the
  // landscape preview). Does nothing unless the texgen service compiles this document.
  void pushHeightmapParams();

  // Edits that apply the change and record one undo entry for it.
  // The caller builds the node, which needs the base-node blk; this adds it and records.
  void createNode(GraphData::Node node);
  void deleteNodes(const eastl::vector<int> &node_ids);
  // constraint: an added id is unused before the removal or re-adds a removed one (a re-synced instance, a
  // rewired edge). A re-added id keeps its place in the list and, without select_added, its selection.
  void replaceNodes(const eastl::vector<int> &removed_node_ids, eastl::vector<GraphData::Node> added_nodes,
    eastl::vector<GraphData::Edge> added_edges, const char *undo_name, bool select_added);
  // Rebuilds, as one entry, the instances of subgraphs registered after since_generation.
  void resyncSubgraphInstances(uint32_t since_generation);
  // Connects the two pins and evicts whatever a single-connect end already held, as one entry.
  void recordConnectEdge(GraphData::Edge edge);
  void deleteEdges(const eastl::vector<int> &edge_ids);
  void toggleEdgeMuted(int edge_id);
  // The context menu acts on a whole selection, so this cannot be a toggle.
  void setEdgesMuted(const eastl::vector<int> &edge_ids, bool muted);
  void setPinComment(int node_id, int pin_index, const eastl::string &new_comment);
  // Takes the whole settings block, so one edit gesture is one entry. No-op when nothing changed.
  void setGraphSettings(const GraphSettings &settings);
  // One entry per edit gesture: every control but text entry writes per change, and the panel nests
  // those writes in one UndoEditGesture. The whole propertyValues vector is snapshotted, so the first
  // write of an edit covers the rest and they pass record_undo=false; a value written with no snapshot
  // behind it cannot be undone. Restoring the whole vector also undoes a freshly-set value back to its
  // descriptor default instead of leaving it as an explicit one.
  void setNodeProperty(int node_id, const eastl::string &prop_name, eastl::string value, bool record_undo = true);

  // Edits handed over after the caller applied them, so one entry covers the whole gesture.
  // commitNodeTransforms is the exception: block sizes arrive already applied, positions do not.
  void recordPaste(eastl::vector<GraphData::Node> pasted_nodes, eastl::vector<GraphData::Edge> pasted_edges,
    const char *undo_name = "Paste");
  // Insert, wire to the source pin, and evict what a singleConnect source pin held, as one entry. It
  // owns the insert because find_connectable_pin needs the node already in the graph. Returns the pin
  // it wired, or -1 -- the node is still created, unconnected.
  int recordAddConnectedNode(GraphData::Node node, int source_node, int source_pin);
  // Splices `node` into the connections at (anchor_node, anchor_pin): its source feeds the node, the
  // consumers move onto the node's output. anchor_edge_id narrows that to one wire, -1 takes the
  // pin's lot. Returns the pin it wired, or -1 -- the node is still created, unconnected.
  int recordInsertNodeOnPin(GraphData::Node node, int anchor_node, int anchor_pin, int anchor_edge_id);
  void recordRemoveKeepingConnections(eastl::vector<GraphData::Node> removed_nodes, eastl::vector<int> node_indices,
    eastl::vector<GraphData::Edge> removed_edges, eastl::vector<int> edge_indices, eastl::vector<GraphData::Edge> bridge_edges);
  // The caller applied both halves already, so this only evicts what a single-connect end of the
  // new edge held, and records the whole reroute as one entry.
  void recordReconnectEdge(const GraphData::Edge &removed_edge, int removed_edge_index, const GraphData::Edge *added_edge);
  void recordSelectionChange(GraphSelection old_selection, GraphSelection new_selection);
  void commitNodeTransforms(eastl::vector<NodePos> old_positions, eastl::vector<NodePos> new_positions,
    eastl::vector<BlockSize> old_sizes, eastl::vector<BlockSize> new_sizes);

private:
  // Node ids are recycled from the top of the range, so a seed left behind by a deleted instance
  // would attach to the next instance that takes its id. Called under graphMutex, and cheap: the map
  // is empty unless the graph holds subgraph instances, and then it holds one entry per instance.
  void dropSeedsOfGoneInstances()
  {
    if (compiledNodeResolutions.empty())
    {
      return;
    }
    const size_t before = compiledNodeResolutions.size();
    for (auto it = compiledNodeResolutions.begin(); it != compiledNodeResolutions.end();)
    {
      const GraphData::Node *const n = find_node_by_id(graphData, it->first);
      it = (n && is_subgraph_instance(*n)) ? ++it : compiledNodeResolutions.erase(it);
    }
    if (compiledNodeResolutions.size() != before)
    {
      ++compiledResolutionsStamp;
    }
  }

  // Edges that a new one on (node_id, pin_index) replaces, appended to out. validate_new_edge only
  // hides the incumbent of a single-connect pin, so removing it is the caller's job; a multi-connect
  // pin replaces nothing. A muted incumbent goes with the rest: unmuting it later would put two
  // live edges on the pin. except_edge_id skips an edge the caller has already inserted.
  void collectReplacedEdges(int node_id, int pin_index, int except_edge_id, eastl::vector<GraphData::Edge> &out) const;

  // The apply half: the smallest steps that change the graph. The undo entries replay them, and the
  // edits above apply through them. Node insert and erase go through the canvas so it refreshes.

  // These change what the compiler sees, so each regenerates.
  void reinsertNode(const GraphData::Node &node, int index = -1);
  void reinsertEdge(const GraphData::Edge &edge, int index = -1);
  void eraseNode(int node_id);
  int eraseEdge(int edge_id); // returns the index the edge had, or -1
  // In order; each index is the one its edge had when erased, as UndoDeleteEdges takes them.
  eastl::vector<int> eraseEdges(const eastl::vector<GraphData::Edge> &edges);
  void eraseNodes(const eastl::vector<int> &node_ids);
  void restoreNodesAndEdges(const eastl::vector<GraphData::Node> &nodes, const eastl::vector<int> &node_indices,
    const eastl::vector<GraphData::Edge> &edges, const eastl::vector<int> &edge_indices);
  void applyNodeProperties(int node_id, const eastl::vector<eastl::pair<eastl::string, eastl::string>> &props);
  // writeGraphSettings is the shared body; applyGraphSettings adds the panel rebuild, which only
  // undo needs -- an edit came from the panel and must not rebuild under it.
  void writeGraphSettings(const GraphSettings &settings);
  void applyGraphSettings(const GraphSettings &settings);
  void recordGraphSettingsChange(GraphSettings old_settings);
  void applyEdgeMuted(int edge_id, bool muted);

  // Display-only, so these do not regenerate.
  void applyNodePositions(const eastl::vector<NodePos> &positions);
  void applyBlockSizes(const eastl::vector<BlockSize> &sizes);
  void applySelection(const GraphSelection &selection);
  void applyPinComment(int node_id, int pin_index, const eastl::string &comment);

  void getNodeProperties(int node_id, eastl::vector<eastl::pair<eastl::string, eastl::string>> &out) const;

  // A new entry class has to be listed here, which is the point: it makes widening the apply
  // surface a deliberate edit rather than a side effect.
  friend class UndoCreateNode;
  friend class UndoDeleteNodes;
  friend class UndoMoveNodes;
  friend class UndoCreateEdge;
  friend class UndoDeleteEdges;
  friend class UndoToggleEdgeMuted;
  friend class UndoSelection;
  friend class UndoNodeProps;
  friend class UndoGraphSettings;
  friend class UndoPinComment;
  friend class UndoBlockResize;

  const GraphSelection &recordedSelection() const;
  void foldSelectionChange(GraphSelection new_selection);
  // Takes erased edges out of the recorded link selection, so no entry keeps an id that resolves to
  // nothing and undo reselects them with their edges. Same call window as foldSelectionChange.
  void dropErasedLinksFromSelection(const eastl::vector<GraphData::Edge> &erased);

  GraphEditorPlg &plugin;
  // A document is one tab's graph, so there is always a canvas showing it.
  GraphTab &view;

  GraphData graphData;

  // Guarded by graphMutex: filled inside the worker's critical section, drained inside main's.
  CompileResults pendingCompileResults;
  volatile int hasPendingCompileResults = 0;
  eastl::hash_map<int, NodeResolution> compiledNodeResolutions;
  uint64_t compiledResolutionsStamp = 0;
  uint64_t positionsStamp = 0;
  eastl::vector<eastl::string> failedSubgraphs;

  // Also covers mainGraphBlk / shaderListBlk on the write side, so a main-thread load cannot wipe
  // them while the worker commits. The texgen service reads and writes those two under its own
  // stateLock rather than this mutex, so those accesses race a main-thread load.
  WinCritSec graphMutex;

  volatile uint64_t structureRevision = 0;
};
