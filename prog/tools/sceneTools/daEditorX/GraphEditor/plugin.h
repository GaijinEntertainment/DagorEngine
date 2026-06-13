// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_wndPublic.h>
#include <oldEditor/de_interface.h>
#include <propPanel/c_control_event_handler.h>

#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_string.h>

#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>

#include <graphEditor/graph_data.h>

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
  // is null or template_uid is unknown.
  void spawnBaseNode(const char *template_uid, float x, float y);

  // Mark the graph dirty so the texgen worker thread runs `compile_graph_to_blks`
  // asynchronously and regenerates. Use for every mutation that doesn't change
  // the graph *source* (property edit, link add/remove, node spawn). Returns
  // instantly; coalesces with other pending marks before the worker picks them up.
  void markGraphDirtyAndRegen();

  // Like markGraphDirtyAndRegen, but forces a full texture-pipeline rebuild (closes texGenReg /
  // clears regcache) so source textures reload from scratch. Use when a change invalidates
  // already-registered textures that a plain regen would reuse -- e.g. a textureRootDir edit
  // (textures are keyed by their root-independent reference name, so the new root wouldn't take
  // effect otherwise).
  void markGraphForceRebuild();

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
  // / textureRootDir / heightmap*) against concurrent read by the texgen worker
  // running compile_graph_to_blks. Also covers the compiled outputs
  // (mainGraphBlk / shaderListBlk) on the write side: the plugin's
  // IGraphCompiler::compile() commits them inside the same mutateGraphData
  // critical section so a main-thread loader can't wipe mainGraphBlk while the
  // worker is committing into it. Worker-side READS of mainGraphBlk (by
  // addPreviewFinalToBlk / startGenerateTex etc.) still happen under the
  // service's stateLock -- racing those against a main-thread load is a
  // pre-existing hazard scoped for a follow-up commit.
  WinCritSec graphMutex;

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
