// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/util/undo.h>
#include <memory/dag_mem.h>

#include <graphEditor/graph_data.h>

#include "graph_edit_types.h"

class GraphDocument;

// Undo entry for a single node creation (drag-drop spawn). Holds a full Node snapshot so redo
// re-inserts it with the same id and spawn position; the editor's global UndoSystem owns and
// deletes the object. Operations are owned by the GraphTab that recorded them
// (GraphPanel::stampUndoOwner), so closing that tab drops them via remove_ops_by_owner without a
// stale-id guard, and each one activates its tab before it applies.
// This is the home for future graph undo entries (delete, move, edge, property edit, paste) --
// each is a separate UndoRedoObject bracketed by begin()/accept().
class UndoCreateNode : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoCreateNode(GraphDocument &doc, GraphData::Node node_snapshot, int node_index = -1);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphDocument &doc;
  GraphData::Node node;
  int nodeIndex;
};

// Undo entry for deleting one or more nodes. Snapshots the removed nodes plus every edge incident
// to any of them, so undo restores the exact sub-graph (ids, positions, connections) and redo
// removes it again. One entry covers a whole Delete action -- a single node, a multi-selection, or
// a block plus its contained children -- so the batch undoes/redoes atomically.
class UndoDeleteNodes : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  // node_indices and edge_indices are the ascending list positions the removed items had.
  UndoDeleteNodes(GraphDocument &doc, eastl::vector<GraphData::Node> removed_nodes, eastl::vector<int> node_indices,
    eastl::vector<GraphData::Edge> removed_edges, eastl::vector<int> edge_indices);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphDocument &doc;
  eastl::vector<GraphData::Node> nodes;
  eastl::vector<int> nodeIndices;
  eastl::vector<GraphData::Edge> edges;
  eastl::vector<int> edgeIndices;
};

// Undo entry for moving (dragging) one or more nodes. Records each moved node's old and new canvas
// position; one entry per drag (every node that moved together, including a block's children).
// restore() puts the nodes back at their old positions, redo() at the new ones. Node position is
// display-only, so applying a move does not retrigger texgen.
class UndoMoveNodes : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoMoveNodes(GraphDocument &doc, eastl::vector<NodePos> old_positions, eastl::vector<NodePos> new_positions);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphDocument &doc;
  eastl::vector<NodePos> oldPositions;
  eastl::vector<NodePos> newPositions;
};

// Undo entry for creating a single edge (link drag). restore() removes it, redo() re-adds it.
class UndoCreateEdge : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoCreateEdge(GraphDocument &doc, const GraphData::Edge &created_edge, int edge_index = -1);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphDocument &doc;
  GraphData::Edge edge;
  int edgeIndex;
};

// Undo entry for deleting one or more edges (link Delete). Snapshots the removed edges so restore()
// re-adds them and redo() removes them again. One entry per Delete action's links.
class UndoDeleteEdges : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  // removed_edges in erase order; edge_indices[i] is the index removed_edges[i] had when it was erased.
  UndoDeleteEdges(GraphDocument &doc, eastl::vector<GraphData::Edge> removed_edges, eastl::vector<int> edge_indices);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphDocument &doc;
  eastl::vector<GraphData::Edge> edges;
  eastl::vector<int> edgeIndices;
};

// Undo entry for muting / unmuting a single edge (link double-click). Only the id and the previous
// flag are stored -- the edge itself stays in the graph either way.
class UndoToggleEdgeMuted : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoToggleEdgeMuted(GraphDocument &doc, int edge_id, bool old_muted);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphDocument &doc;
  int edgeId;
  bool oldMuted;
};

// Undo entry for a selection change (nodes and/or links). Records the selection before and after the
// change; restore reapplies the old set, redo the new. Selection lives in imgui-node-editor and its
// select calls are valid only in-frame, so applying is deferred: the entry hands the target set to
// doc.applySelection, which the GraphTab pushes to ne on its next render pass -- so a set
// restored alongside re-added nodes or edges (same undo entry) reselects them once they exist again.
// Deliberate selection changes record one of these alone; a change caused by an edit rides that
// edit's entry.
class UndoSelection : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoSelection(GraphDocument &doc, GraphSelection old_selection, GraphSelection new_selection);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;
  // The selection is not written to the graph file, so it must not mark the tab modified.
  bool changes_saved_data() const override { return false; }

private:
  GraphDocument &doc;
  GraphSelection oldSelection;
  GraphSelection newSelection;
};

// Undo entry for editing a node's properties. Follows the daEditorX ObjectEditor UndoPropsChange
// pattern (HeightmapLand/hmlEntity.h): snapshot the node's whole propertyValues vector -- old in the
// ctor (before the edit applies), redo lazily on the first restore. Restoring the whole vector keeps
// originally-absent properties absent, so a freshly-set value undoes back to its descriptor default.
// One per edit: the panel nests the per-change writes of an edit in a single UndoEditGesture and asks
// for the snapshot only on the first of them, which covers the rest.
class UndoNodeProps : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoNodeProps(GraphDocument &doc, int node_id);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphDocument &doc;
  int nodeId;
  eastl::vector<eastl::pair<eastl::string, eastl::string>> oldProps;
  eastl::vector<eastl::pair<eastl::string, eastl::string>> redoProps;
};

// Undo entry for a graph-level settings change. Follows the UndoNodeProps pattern, snapshotting the
// GraphData scalar fields rather than a node: old held explicitly (snapshotted before the edit), redo
// captured lazily on the first restore. applyGraphSettings re-pushes heightmap params and regenerates,
// so the landscape preview and texgen follow the restored values.
class UndoGraphSettings : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoGraphSettings(GraphDocument &doc, GraphSettings old_settings);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphDocument &doc;
  GraphSettings oldSettings;
  GraphSettings redoSettings;
};

// Undo entry for editing a pin's comment (the "Comment a pin" tool). Holds the comment before and
// after for one pin; restore/redo write it back. Pin comments are display-only annotations, so this
// does not regenerate -- the canvas re-reads graphData each frame.
class UndoPinComment : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoPinComment(GraphDocument &doc, int node_id, int pin_index, eastl::string old_comment, eastl::string new_comment);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphDocument &doc;
  int nodeId;
  int pinIndex;
  eastl::string oldComment;
  eastl::string newComment;
};

// Undo entry for resizing one or more block (group) nodes by dragging a border. Records each block's
// old and new size; one entry per resize drag. restore/redo go through applyBlockSizes. Block size is
// display-only, so applying a resize does not regenerate.
class UndoBlockResize : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoBlockResize(GraphDocument &doc, eastl::vector<BlockSize> old_sizes, eastl::vector<BlockSize> new_sizes);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphDocument &doc;
  eastl::vector<BlockSize> oldSizes;
  eastl::vector<BlockSize> newSizes;
};
