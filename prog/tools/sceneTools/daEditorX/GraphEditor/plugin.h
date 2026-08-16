// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_wndPublic.h>
#include <oldEditor/de_interface.h>
#include <propPanel/c_control_event_handler.h>

#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_string.h>

#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>

#include <graphEditor/graph_data.h>

#include "graph_undo.h"
#include "resource_paths.hpp"

class BaseNodesPanel;
class GraphPanel;
struct IGraphCompiler;
struct IGraphTexGenService;
class TexturePreviewPanel;
class HistogramPanel;
class LandscapePreviewPanel;
class PropertiesPanel;
class ShortcutsPanel;

class GraphEditorPlg final : public IGenEditorPlugin,
                             public IGenEventHandler,
                             public PropPanel::ControlEventHandler,
                             public IWndManagerWindowHandler
{
public:
  GraphEditorPlg();
  ~GraphEditorPlg() override;

  const char *getInternalName() const override { return "graphEditor"; }
  const char *getMenuCommandName() const override { return "GraphEditor"; }
  const char *getHelpUrl() const override { return "/html/Plugins/GraphEditor/index.htm"; }

  int getRenderOrder() const override { return 100; }
  int getBuildOrder() const override { return 0; }

  bool showInTabs() const override { return true; }
  bool showSelectAll() const override { return true; }

  bool acceptSaveLoad() const override { return true; }

  void registered() override;
  void unregistered() override;
  void beforeMainLoop() override;

  bool begin(int toolbar_id, unsigned menu_id) override;
  bool end() override;
  void onNewProject() override;
  IGenEventHandler *getEventHandler() override { return this; }

  void setVisible(bool vis) override { isVisible = vis; }
  bool getVisible() const override { return isVisible; }
  bool getSelectionBox(BBox3 &box) const override { return false; }
  bool getStatusBarPos(Point3 &pos) const override { return false; }

  void clearObjects() override;
  void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) override;
  void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path) override;
  void selectAll() override;
  void deselectAll() override;
  void invertSelection() override;

  void actObjects(float dt) override;
  void beforeRenderObjects(IGenViewportWnd *vp) override;
  void renderObjects() override;
  void renderTransObjects() override;
  void updateImgui() override;

  void *queryInterfacePtr(unsigned huid) override;

  bool onPluginMenuClick(unsigned id) override;
  void handleViewportAcceleratorCommand(unsigned id) override;
  void registerEditorCommands(IEditorCommandSystem &command_system) override;
  void registerMenuAccelerators() override;

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) override;
  bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) override;
  void handleViewportPaint(IGenViewportWnd *wnd) override;
  void handleViewChange(IGenViewportWnd *wnd) override;

  void *onWmCreateWindow(int type) override;
  bool onWmDestroyWindow(void *window) override;

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  // Base-node registry (base_nodes.blk). Single source of truth for descriptor data
  // (pin shape, property constraints, hidden flags). Loaded lazily on first access.
  const DataBlock &getBaseNodesBlk();

  // Primary descriptor lookup: O(1) hash-map by stable per-template uid (`templateUid:t`
  // in base_nodes.blk). Returns nullptr if the uid is unknown.
  const DataBlock *findBaseNodeBlockByUid(const char *template_uid);

  // Legacy fallback: linear-scan by `name:t`. Used only by the graph loaders when a
  // pre-templateUid file is opened. New code MUST go through findBaseNodeBlockByUid --
  // name lookup is not stable across template renames.
  const DataBlock *findBaseNodeBlockByName(const char *desc_name);

  // Pure factory: BLK lookup by uid + populate `out` (templateUid, descName from current
  // descriptor, position, default propertyValues from BLK, and pin-name-only stubs in
  // `pins[]` matching descriptor's pin order). Does NOT assign id or insert into the
  // graph. Returns false if `template_uid` is unknown.
  bool makeNodeFromBaseBlk(const char *template_uid, float x, float y, GraphData::Node &out);

  // Builds a fresh node via makeNodeFromBaseBlk, allocates id via graphPanel, hands to
  // graphPanel->addNode. The drop handler in GraphPanel calls this. No-op if graphPanel
  // is null or template_uid is unknown. Records an undo entry ("Create node").
  void spawnBaseNode(const char *template_uid, float x, float y);

  // Undo/redo primitives used by the graph_undo.h entries. They mutate the canonical graphData
  // and kick a regen; they go through graphPanel when it exists (so the canvas refreshes), and
  // fall back to a direct mutateGraphData edit when the Graph panel is closed -- undo must keep
  // graphData consistent even with no panel open. Single-element: reinsertNode / reinsertEdge add
  // one; eraseNode removes a node and its incident edges, eraseEdge removes one edge. Batch:
  // restoreNodesAndEdges re-adds a captured sub-graph (nodes first, then edges, so no edge dangles);
  // eraseNodes removes a set.
  void reinsertNode(const GraphData::Node &node);
  void reinsertEdge(const GraphData::Edge &edge);
  void eraseNode(int node_id);
  void eraseEdge(int edge_id);
  void eraseNodes(const eastl::vector<int> &node_ids);
  void restoreNodesAndEdges(const eastl::vector<GraphData::Node> &nodes, const eastl::vector<GraphData::Edge> &edges);

  // Removes node_ids (and their incident edges) as one undoable operation: snapshots the removed
  // sub-graph, erases it, and records an UndoDeleteNodes entry. The Delete path in GraphPanel
  // calls this once per Delete action (after the block-children prompt resolves), so a whole
  // multi-selection undoes/redoes atomically.
  void deleteNodesUndoable(const eastl::vector<int> &node_ids);

  // Edge create/delete undo. addEdgeUndoable adds one edge and records an UndoCreateEdge (the
  // link-drag handler calls it); deleteEdgesUndoable snapshots edge_ids, erases them, and records
  // one UndoDeleteEdges (the link-Delete handler and the "remove edges at pin" tool call it).
  void addEdgeUndoable(GraphData::Edge edge);
  void deleteEdgesUndoable(const eastl::vector<int> &edge_ids);

  // Records undo for an already-applied paste as one grouped operation -- a UndoCreateNode per node
  // plus a UndoCreateEdge per edge under a single "Paste" entry (no dedicated paste class needed).
  // The CANVAS_PASTE handler pastes via CanvasClipboard (which adds the nodes/edges) and hands the
  // inserted fragment here. Copy is read-only (no undo) and cut undoes via the delete path.
  void recordPaste(eastl::vector<GraphData::Node> pasted_nodes, eastl::vector<GraphData::Edge> pasted_edges);

  // Records an already-applied "remove keeping connections" splice as one grouped operation: a single
  // UndoDeleteNodes for the removed nodes and their incident edges, plus a UndoCreateEdge per bridge
  // edge the splice added, under one "Remove keeping connections" entry. Like recordPaste the panel
  // applies the splice and hands the result here. No dedicated class needed.
  void recordRemoveKeepingConnections(eastl::vector<GraphData::Node> removed_nodes, eastl::vector<GraphData::Edge> removed_edges,
    eastl::vector<GraphData::Edge> bridge_edges);

  // Records an already-applied edge reconnect (the A / "Modify edge" tool) as one grouped operation.
  // The tool removes the picked edge when the drag begins and adds a replacement if the drag drops on
  // a valid pin. Pass the removed edge always; pass the added edge when one was created, or nullptr
  // when the drag was cancelled (which leaves just the removal). Recorded under "Reconnect edge":
  // redo removes the old edge then adds the new, undo removes the new then restores the old.
  void recordReconnectEdge(const GraphData::Edge &removed_edge, const GraphData::Edge *added_edge);

  // Records a deliberate selection change (click, box-select, the select / show commands) as one
  // "Select" entry. The GraphPanel's frame-end detector calls it with the selection (nodes + links)
  // before and after; selection changes that are side effects of an edit are folded into that edit.
  void recordSelectionChange(GraphSelection old_selection, GraphSelection new_selection);

  // Move/resize undo. commitNodeTransforms records a finished drag as one entry: a UndoMoveNodes for
  // nodes whose position changed and/or a UndoBlockResize for blocks whose size changed. A corner
  // resize changes both for the same block, so folding them lets one Ctrl+Z restore position and size
  // together. It commits the new positions to graphData first (sizes are already committed live by
  // syncBlockSizes); the GraphPanel drag-end detector calls it. applyNodePositions writes positions
  // into graphData and pushes them to the node editor (used by restore/redo). Display-only -- no regen.
  void commitNodeTransforms(eastl::vector<NodePos> old_positions, eastl::vector<NodePos> new_positions,
    eastl::vector<BlockSize> old_sizes, eastl::vector<BlockSize> new_sizes);
  void applyNodePositions(const eastl::vector<NodePos> &positions);

  // Applies a selection (nodes + links; used by UndoSelection). Selection is imgui-node-editor view
  // state whose select calls are in-frame only, so this hands the set to the GraphPanel to push to ne
  // on its next render pass. No-op with the Graph panel closed (selection is meaningless without a canvas).
  void applySelection(const GraphSelection &selection);

  // Node-property undo support (see UndoNodeProps). getNodeProperties copies a node's propertyValues
  // out (clears out if the node is gone); setNodeProperties replaces them, kicks a regen, and refreshes
  // the PropertiesPanel display. The PropertiesPanel brackets an edit with begin()/put(new UndoNodeProps)
  // /accept(), so one gesture is one entry; these back the undo object's restore/redo.
  void getNodeProperties(int node_id, eastl::vector<eastl::pair<eastl::string, eastl::string>> &out) const;
  void setNodeProperties(int node_id, const eastl::vector<eastl::pair<eastl::string, eastl::string>> &props);

  // Graph-settings undo (see UndoGraphSettings). getGraphSettings copies the graph-level fields out;
  // setGraphSettings writes them, re-pushes heightmap params, regenerates, and refreshes the panel.
  // recordGraphSettingsChange records one "Change graph settings" entry when old differs from current;
  // the PropertiesPanel snapshots the settings before a graph-field edit and calls it afterward.
  void getGraphSettings(GraphSettings &out) const;
  void setGraphSettings(const GraphSettings &settings);
  void recordGraphSettingsChange(GraphSettings old_settings);

  // Pin-comment undo (the "Comment a pin" tool). setPinComment writes one pin's comment (used by
  // UndoPinComment restore/redo); pin comments are display-only, so it does not regenerate -- the
  // canvas re-reads graphData each frame. setPinCommentUndoable reads the current comment as the undo's
  // old value, applies new_comment, and records one "Edit pin comment" entry when it differs.
  void setPinComment(int node_id, int pin_index, const eastl::string &comment);
  void setPinCommentUndoable(int node_id, int pin_index, const eastl::string &new_comment);

  // Edge-mute undo (link double-click). setEdgeMuted writes one edge's flag (used by
  // UndoToggleEdgeMuted restore/redo); toggleEdgeMutedUndoable flips it and records one entry.
  // Unlike a pin comment this DOES regenerate -- a muted edge carries no data, so the compiler
  // drops it and prunes whatever it was the only source for.
  void setEdgeMuted(int edge_id, bool muted);
  void toggleEdgeMutedUndoable(int edge_id);

  // Block-resize undo. applyBlockSizes writes the given block sizes into graphData and queues a
  // ne::SetGroupSize push (ne stores the group bounds and ignores drawBlockNode's supplied size for an
  // existing group, so the size must be pushed explicitly -- the GraphPanel drains it in drawBlockNode).
  // Block size is display-only, so no regen. Used by UndoBlockResize restore/redo; the drag-end detector
  // records it via commitNodeTransforms (folded with any move of the same drag).
  void applyBlockSizes(const eastl::vector<BlockSize> &sizes);

  // Mark the graph dirty so the texgen worker thread runs `compile_graph_to_blks`
  // asynchronously and regenerates. Use for every mutation that doesn't change
  // the graph *source* (property edit, link add/remove, node spawn). Returns
  // instantly; coalesces with other pending marks before the worker picks them up.
  void markGraphDirtyAndRegen();

  // Use after a graph load (BLK / initial registration). Pushes heightmap
  // params, hands the GraphData pointer to the service (which resets pipeline
  // state -- preview-final, selected texture, etc.), then marks dirty so the
  // worker compiles the freshly-loaded graph. Do NOT call this from edit paths;
  // it wipes preview state.
  void notifyGraphSourceChanged();

  // Drains per-pin customTextureName values produced by the most recent worker-thread
  // compile (stashed by GraphCompilerImpl::compile under graphMutex) into
  // graphData.nodes[].pins[].customTextureName. Must run on the main thread: the
  // texture-preview lookup (graph_panel.cpp) reads that Pin field without taking
  // graphMutex, so the write itself must originate from the same thread to
  // preserve the "main is the only writer of nodes / pins" invariant. Called once
  // per tick from actObjects. Cheap no-op when no compile has finished since last
  // drain. Entries are keyed by node id so deletes between compile and apply just
  // drop their entry instead of corrupting a now-different node at that index.
  void applyPendingPinCustomTextureNames();

  // Hand-off used by GraphCompilerImpl::compile (worker thread, inside the
  // mutateGraphData critical section): replaces the pending-names buffer with the
  // most recent compile's output. The buffer is consumed on the main thread by
  // applyPendingPinCustomTextureNames -- see that method for the threading rationale.
  void setPendingPinCustomTextureNames(eastl::vector<eastl::pair<int, eastl::vector<eastl::string>>> names)
  {
    pendingPinCustomTextureNames = eastl::move(names);
  }

  // Accessors for sibling panels (in particular PropertiesPanel) that need to read shared
  // state without reaching into private members. GraphPanel may be null when the user has
  // closed it; texGenService may be null until the texgen service initialises.
  GraphPanel *getGraphPanel() const { return graphPanel.get(); }
  IGraphTexGenService *getTexGenService() const { return texGenService; }

  // Read-only access for main-thread callers (UI rendering, property-panel display,
  // findNodeById lookups). Main is the only writer; concurrent reads with the texgen
  // worker's compile() are safe because compile takes the graph mutex via the
  // mutateGraphData() path below. Do NOT mutate through this reference -- use
  // mutateGraphData() so the worker doesn't observe torn state mid-compile.
  const GraphData &getGraphData() const { return graphData; }

  // Take the graph mutex, hand a mutable reference to the lambda, release on return.
  // Use this for EVERY write to graphData.nodes / edges / propertyValues / heightmap*
  // / sourcePath / etc., and for the worker's compile read. The mutex is the only
  // thing preventing the texgen worker from reading a half-mutated graph and crashing
  // on a freed eastl::string buffer or a relocated vector slot.
  template <class Fn>
  void mutateGraphData(Fn &&fn)
  {
    WinAutoLock lock(graphMutex);
    fn(graphData);
    interlocked_increment(graphRevision);
  }

  // Bumped by every mutateGraphData call, so a main-thread cache derived from graphData
  // (GraphPanel's dead-path cache) can detect staleness by comparing one value instead of
  // threading a dirty flag through every mutation site. The texgen worker also reaches
  // mutateGraphData when it commits a compile, so it bumps this too; a redundant refresh is
  // harmless. Read outside graphMutex on purpose -- the cache it gates is display-only, so a
  // relaxed load losing a race costs a one-frame-late refresh and nothing more.
  uint64_t getGraphRevision() const { return interlocked_relaxed_load(graphRevision); }

  // Read the canonical graphData under graphMutex, for worker-thread readers (the dshl assembler
  // snapshot) that must not race a main-thread load. Read-only; use mutateGraphData to write.
  template <class Fn>
  void readGraphData(Fn &&fn)
  {
    WinAutoLock lock(graphMutex);
    fn(static_cast<const GraphData &>(graphData));
  }

  const char *getShaderIncludesDir() const { return resourcePaths.shaderIncludesDir; }
  const char *getMainGraphsDir() const { return resourcePaths.mainGraphsDir; }
  const char *getSubgraphsDir() const { return resourcePaths.subgraphsDir; }

  // Drops the cached base-nodes blk + uid index and re-runs the lazy load + synthesis pipeline,
  // then re-populates the BaseNodesPanel tree so newly added shader / subgraph files appear
  // without restarting the editor. Cheap: the load itself is bounded (~100 base nodes plus a
  // small fixed set of shader / subgraph files on disk). Triggered from the panel's reload button.
  void reloadBaseNodes();

  bool promptPinComment(eastl::string &inout_comment);

private:
  void initResourcePaths();
  void toggleShortcutsPanel();
  void newEmptyGraph();
  void promptAndLoadGraphBlk();
  void promptAndSaveGraphBlk();
  void promptAndSaveAsSubgraphBlk();
  bool loadBaseNodesBlkIfNeeded();

  void appendShaderTemplatesToBaseNodes();
  // Mirrors appendShaderTemplatesToBaseNodes for *.subgraph.blk files under
  // resourcePaths.subgraphsDir. Each graph file becomes a synthesized node{} descriptor in
  // baseNodesBlk with category "Subgraphs", plugin "subgraph", and one pin{} per
  // `subgraph in: TYPE` / `subgraph out` boundary node found inside the child. The pin's
  // interface name comes from the boundary's `name` property value (not its descriptor
  // pin name), so multiple boundaries in one child stay distinguishable.
  void appendSubgraphTemplatesToBaseNodes();

  bool isVisible = false;
  int toolBarId = -1;
  eastl::unique_ptr<GraphPanel> graphPanel;
  IGraphTexGenService *texGenService = nullptr;
  eastl::unique_ptr<TexturePreviewPanel> previewPanel;
  eastl::unique_ptr<HistogramPanel> histogramPanel;
  eastl::unique_ptr<LandscapePreviewPanel> landscapePanel;
  eastl::unique_ptr<BaseNodesPanel> baseNodesPanel;
  eastl::unique_ptr<PropertiesPanel> propertiesPanel;
  eastl::unique_ptr<ShortcutsPanel> shortcutsPanel;

  ResourcePaths resourcePaths;
  GraphData graphData;

  // Guards `graphData`'s source-of-truth fields (nodes / edges / propertyValues
  // / heightmap*) against concurrent read by the texgen worker
  // running compile_graph_to_blks. Also covers the compiled outputs
  // (mainGraphBlk / shaderListBlk) on the write side: the plugin's
  // IGraphCompiler::compile() commits them inside the same mutateGraphData
  // critical section so a main-thread loader can't wipe mainGraphBlk while the
  // worker is committing into it. Worker-side READS of mainGraphBlk (by
  // addPreviewFinalToBlk / startGenerateTex etc.) still happen under the
  // service's stateLock -- racing those against a main-thread load is a
  // pre-existing hazard scoped for a follow-up commit.
  WinCritSec graphMutex;

  // See getGraphRevision. Written under graphMutex by both threads, read lock-free.
  volatile uint64_t graphRevision = 0;

  // Adapter that forwards IGraphCompiler::compile() calls (issued from the
  // texgen worker) into compile_graph_to_blks(graphData) under graphMutex.
  // Constructed once the texgen service is resolved; cleared from the service
  // before the impl is destroyed at shutdown.
  eastl::unique_ptr<IGraphCompiler> graphCompiler;

  // Per-pin texgen register names produced by the most recent compile_graph_to_blks,
  // keyed by GraphData::Node::id (not array index, so a node-delete between compile
  // and drain just drops its entry rather than aliasing onto a different node).
  // Inner vector is parallel to that node's pins[] at compile time. Filled inside
  // the worker's mutateGraphData critical section; drained by
  // applyPendingPinCustomTextureNames on the main thread inside its own
  // mutateGraphData critical section -- so the actual Pin write happens on main.
  eastl::vector<eastl::pair<int, eastl::vector<eastl::string>>> pendingPinCustomTextureNames;

  DataBlock baseNodesBlk;
  // uid -> node{} block pointer into baseNodesBlk. Built once in
  // loadBaseNodesBlkIfNeeded() after a successful load; lifetime is tied to
  // baseNodesBlk (do NOT reload baseNodesBlk without rebuilding this map).
  eastl::hash_map<eastl::string, const DataBlock *> baseNodesByUid;
  bool baseNodesBlkLoaded = false;

  bool isTexturePreviewVisible = true;
  bool isHistogramVisible = true;
  bool isLandscapeVisible = true;
  bool isGraphVisible = true;
  bool isBaseNodesVisible = false;
  bool isPropertiesVisible = true;
  bool isShortcutsVisible = false;
};
