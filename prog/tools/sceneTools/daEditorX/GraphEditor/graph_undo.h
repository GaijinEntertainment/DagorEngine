// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/util/undo.h>
#include <memory/dag_mem.h>

#include <graphEditor/graph_data.h>

class GraphEditorPlg;

// Undo entry for a single node creation (drag-drop spawn). Holds a full Node snapshot so redo
// re-inserts it with the same id and spawn position; the editor's global UndoSystem owns and
// deletes the object. Operations are attributed to the GraphEditor plugin by the engine
// (set_op_owner), so a graph swap drops them via remove_ops_by_owner without a stale-id guard.
// This is the home for future graph undo entries (delete, move, edge, property edit, paste) --
// each is a separate UndoRedoObject bracketed by begin()/accept().
class UndoCreateNode : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoCreateNode(GraphEditorPlg &plg, GraphData::Node node_snapshot);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphEditorPlg &plugin;
  GraphData::Node node;
};

// Undo entry for deleting one or more nodes. Snapshots the removed nodes plus every edge incident
// to any of them, so undo restores the exact sub-graph (ids, positions, connections) and redo
// removes it again. One entry covers a whole Delete action -- a single node, a multi-selection, or
// a block plus its contained children -- so the batch undoes/redoes atomically.
class UndoDeleteNodes : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoDeleteNodes(GraphEditorPlg &plg, eastl::vector<GraphData::Node> removed_nodes, eastl::vector<GraphData::Edge> removed_edges);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphEditorPlg &plugin;
  eastl::vector<GraphData::Node> nodes;
  eastl::vector<GraphData::Edge> edges;
};

// A node id paired with a canvas position. Used by the move-undo entry and the plugin's position
// helpers (commitNodeMoves / applyNodePositions).
struct NodePos
{
  int nodeId;
  float x;
  float y;
};

// Undo entry for moving (dragging) one or more nodes. Records each moved node's old and new canvas
// position; one entry per drag (every node that moved together, including a block's children).
// restore() puts the nodes back at their old positions, redo() at the new ones. Node position is
// display-only, so applying a move does not retrigger texgen.
class UndoMoveNodes : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoMoveNodes(GraphEditorPlg &plg, eastl::vector<NodePos> old_positions, eastl::vector<NodePos> new_positions);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphEditorPlg &plugin;
  eastl::vector<NodePos> oldPositions;
  eastl::vector<NodePos> newPositions;
};

// Undo entry for creating a single edge (link drag). restore() removes it, redo() re-adds it.
class UndoCreateEdge : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoCreateEdge(GraphEditorPlg &plg, const GraphData::Edge &created_edge);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphEditorPlg &plugin;
  GraphData::Edge edge;
};

// Undo entry for deleting one or more edges (link Delete). Snapshots the removed edges so restore()
// re-adds them and redo() removes them again. One entry per Delete action's links.
class UndoDeleteEdges : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoDeleteEdges(GraphEditorPlg &plg, eastl::vector<GraphData::Edge> removed_edges);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphEditorPlg &plugin;
  eastl::vector<GraphData::Edge> edges;
};

// A canvas selection: the selected node ids and link (edge) ids, each kept sorted so two selections
// compare as sets. imgui-node-editor owns one mixed selection, so nodes and links are undone together.
struct GraphSelection
{
  eastl::vector<int> nodes;
  eastl::vector<int> links;

  bool operator==(const GraphSelection &) const = default;
};

// Undo entry for a selection change (nodes and/or links). Records the selection before and after the
// change; restore reapplies the old set, redo the new. Selection lives in imgui-node-editor and its
// select calls are valid only in-frame, so applying is deferred: the entry hands the target set to
// plugin.applySelection, which the GraphPanel pushes to ne on its next render pass -- so a set
// restored alongside re-added nodes or edges (same undo entry) reselects them once they exist again.
// Deliberate selection changes record one of these alone; a change caused by an edit rides that
// edit's entry.
class UndoSelection : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoSelection(GraphEditorPlg &plg, GraphSelection old_selection, GraphSelection new_selection);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphEditorPlg &plugin;
  GraphSelection oldSelection;
  GraphSelection newSelection;
};

// Undo entry for editing a node's properties. Follows the daEditorX ObjectEditor UndoPropsChange
// pattern (HeightmapLand/hmlEntity.h): snapshot the node's whole propertyValues vector -- old in the
// ctor (before the edit applies), redo lazily on the first restore. Restoring the whole vector keeps
// originally-absent properties absent, so a freshly-set value undoes back to its descriptor default.
// The PropertiesPanel brackets each edit gesture with begin()/put/accept, so one gesture is one entry.
class UndoNodeProps : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoNodeProps(GraphEditorPlg &plg, int node_id);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphEditorPlg &plugin;
  int nodeId;
  eastl::vector<eastl::pair<eastl::string, eastl::string>> oldProps;
  eastl::vector<eastl::pair<eastl::string, eastl::string>> redoProps;
};

// Snapshot of the graph-level settings edited in the PropertiesPanel with no node selected: output
// directories, heightmap metadata, and the graph-default texture format. Compared as a whole for the
// changed-guard.
struct GraphSettings
{
  eastl::string renderDir;
  eastl::string entityDir;
  float heightmapScale = 0.0f;
  float heightmapMin = 0.0f;
  float heightmapCellSize = 0.0f;
  int graphTextureWidth = 0;
  int graphTextureHeight = 0;
  int graphTextureDepth = 0;
  eastl::string graphTextureType;
  eastl::string graphTextureWrap;

  bool operator==(const GraphSettings &) const = default;
};

// Undo entry for a graph-level settings change. Follows the UndoNodeProps pattern, snapshotting the
// GraphData scalar fields rather than a node: old held explicitly (snapshotted before the edit), redo
// captured lazily on the first restore. setGraphSettings re-pushes heightmap params and regenerates,
// so the landscape preview and texgen follow the restored values.
class UndoGraphSettings : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoGraphSettings(GraphEditorPlg &plg, GraphSettings old_settings);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphEditorPlg &plugin;
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

  UndoPinComment(GraphEditorPlg &plg, int node_id, int pin_index, eastl::string old_comment, eastl::string new_comment);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphEditorPlg &plugin;
  int nodeId;
  int pinIndex;
  eastl::string oldComment;
  eastl::string newComment;
};

// A block (group) node's id paired with its width/height. Used by the block-resize undo and the
// plugin's applyBlockSizes / recordBlockResizes.
struct BlockSize
{
  int nodeId;
  float width;
  float height;
};

// Undo entry for resizing one or more block (group) nodes by dragging a border. Records each block's
// old and new size; one entry per resize drag. restore/redo write the sizes into graphData -- drawBlockNode
// supplies them to ne each frame, so the block re-sizes without an explicit ne push. Block size is
// display-only, so applying a resize does not regenerate.
class UndoBlockResize : public UndoRedoObject
{
public:
  DAG_DECLARE_NEW(midmem)

  UndoBlockResize(GraphEditorPlg &plg, eastl::vector<BlockSize> old_sizes, eastl::vector<BlockSize> new_sizes);

  void restore(bool save_redo_data) override;
  void redo() override;
  size_t size() override;
  void accepted() override {}
  void get_description(String &s) override;

private:
  GraphEditorPlg &plugin;
  eastl::vector<BlockSize> oldSizes;
  eastl::vector<BlockSize> newSizes;
};
