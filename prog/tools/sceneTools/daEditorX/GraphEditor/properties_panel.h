// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/util/undo.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/panelWindow.h>

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <EASTL/vector_set.h>

#include <graphEditor/graph_data.h>

class CurvePreviewControl;
class DataBlock;
class GraphDocument;
class GraphEditorPlg;

// Context-sensitive property grid. With one node selected it walks the node's descriptor in
// base_nodes.blk and creates a control per `property {}` block. Otherwise it shows the graph-level
// fields: the texture defaults, plus output dirs and heightmap parameters for the main graph or the
// inputs and outputs for a subgraph window. P toggles that graph view whatever is selected.
// Edits write back into GraphData and ask IGraphTexGenService to regenerate.
class PropertiesPanel final : public PropPanel::ControlEventHandler
{
public:
  explicit PropertiesPanel(GraphEditorPlg &plugin);
  ~PropertiesPanel() override;

  PropPanel::PanelWindowPropertyControl *getPanelWindow() { return panelWindow; }

  // Called every frame after GraphPanel updates. Diffs against lastRenderedNodeId /
  // lastRenderedTabId to decide whether a control rebuild is required; the no-change
  // path is just a few comparisons + panelWindow->updateImgui().
  void updateImgui();

  // Force a control rebuild on the next updateImgui. Called after an undo/redo changes a property
  // value out-of-band (UndoNodeProps) so the controls reflect the restored values.
  void invalidateControls() { forceRebuild = true; }

  void toggleGraphView() { graphView = !graphView; }

  // Lands a half-made edit in the document it was typed into, before the tab strip switches away.
  void commitPendingEdit();

  // The tab holding this document is closing: commit, drop the controls built for it and let go.
  void detachFromDocument(const GraphDocument &closing);

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChangeFinished(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

private:
  enum class Mode
  {
    None,
    Graph,
    Node,
    Subgraph,
  };

  void rebuildForGraph();
  void rebuildForSubgraph();
  void rebuildForNode(int node_id);
  void createTextureCombos(PropPanel::ContainerPropertyControl &container, bool in_group);

  void commitNodeProperty(int pcb_id, PropPanel::ContainerPropertyControl *panel, bool finished);

  bool shouldWarnOnce(int node_id, const char *prop_name);

  const GraphData::Node *findNodeById(int id) const;

  void dropControls();

  PropPanel::PanelWindowPropertyControl *panelWindow = nullptr;
  GraphEditorPlg &plugin;
  // The document the current controls were built for, refreshed on every rebuild.
  GraphDocument *doc = nullptr;

  Mode currentMode = Mode::None;
  int lastRenderedNodeId = -1;
  // Two untitled graphs share the empty source path, so the tab is what tells them apart.
  int lastRenderedTabId = -1;
  bool forceRebuild = false; // one-shot rebuild request from invalidateControls() (undo/redo refresh)
  bool graphView = false;
  int lastSeenSelectedId = -1;
  // The subgraph view lists boundary nodes by y, which the canvas edits while they are shown.
  uint64_t lastRenderedRevision = 0;
  uint64_t lastRenderedPositionsStamp = 0;
  // The view rebuilds on those edits, which would open the groups the user collapsed.
  bool inputsCollapsed = false;
  bool outputsCollapsed = false;
  bool texturesCollapsed = false;

  // One undo operation per edit, so a live property that commits per change undoes as a whole.
  UndoEditGesture editGesture;

  // Per-pid -> property name (node mode), wiped on every rebuild, keyed on the property's PID block
  // base.
  eastl::vector_map<int, eastl::string> pidToPropertyName;

  // gradient_preview strips, owned here because the holder control only borrows the pointer.
  eastl::vector<eastl::unique_ptr<CurvePreviewControl>> curvePreviews;

  // Malformed values already reported, so re-selecting the node does not repeat the warning.
  eastl::vector_set<eastl::string> warnedProperties;
};
