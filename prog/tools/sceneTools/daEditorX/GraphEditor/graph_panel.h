// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/panelWindow.h>

#include <EASTL/hash_set.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <graphEditor/graph_data.h>

namespace ax
{
namespace NodeEditor
{
struct EditorContext;
}
} // namespace ax

class GraphEditorPlg;
struct IGraphTexGenService;

class GraphPanel final : public PropPanel::ControlEventHandler
{
public:
  GraphPanel(GraphEditorPlg &plugin, IGraphTexGenService *tex_gen_service, GraphData &graph_data);
  ~GraphPanel() override;

  PropPanel::PanelWindowPropertyControl *getPanelWindow() { return panelWindow; }

  void updateImgui();

  // Called by the plugin after it (re)populates the shared GraphData. Resets the NodeEditor
  // context (positions, fit, selection) so the panel doesn't carry stale display state across
  // graph swaps.
  void onGraphDataChanged();

  // Universal node-insertion entry point. Trusts the caller's id; pushes onto graphData.nodes
  // and marks the id as pending so the next render pushes node.x/node.y to ne::SetNodePosition.
  void addNode(GraphData::Node node);

  // Returns max(existing.id) + 1, or 0 if empty. Stable allocator -- callers that need a
  // unique id call this then pass the result to addNode.
  int allocateNodeId() const;

  // Edge counterpart of allocateNodeId / addNode. addEdge trusts the caller's id and just
  // appends; removeEdgeById is a linear scan + erase (returns false if no edge with that id).
  // Used by the link-create / link-delete plumbing in updateImgui.
  int allocateEdgeId() const;
  void addEdge(GraphData::Edge edge);
  bool removeEdgeById(int edge_id);
  bool removeNodeById(int node_id);

  // Per-tick hook driven by the plugin's actObjects. Runs between ImGui frames -- the right
  // place for anything that needs to be done outside WithinFrameScope (modal dialogs in
  // particular: PropPanel::DialogWindow::showDialog aborts inside an ImGui frame).
  void actObjects(float dt);

  // Original node id (GraphData::Node::id) of the single selected node, or -1 when none /
  // multiple are selected. Refreshed each updateImgui frame after the ne::End call. Stable
  // across the rest of the frame so other panels (e.g. PropertiesPanel) can read it.
  int getSelectedNodeId() const { return selectedNodeId; }

private:
  static uint64_t makePinId(int node_id, int pin_index)
  {
    return ((uint64_t(uint32_t(node_id)) + 1) << 20) | (uint64_t(uint32_t(pin_index)) + 1);
  }
  static uint64_t makeNodeId(int node_id) { return uint64_t(uint32_t(node_id)) + 1; }
  static uint64_t makeLinkId(int edge_id) { return uint64_t(uint32_t(edge_id)) + 1; }

  PropPanel::PanelWindowPropertyControl *panelWindow = nullptr;
  GraphEditorPlg &plugin;
  IGraphTexGenService *texGenService = nullptr;
  GraphData &graphData;
  ax::NodeEditor::EditorContext *editor = nullptr;

  // Node ids that need ne::SetNodePosition pushed this frame (using node.x/node.y, which is
  // already canvas-space). Populated by onGraphDataChanged (every loaded id) and addNode
  // (each drag-drop insert). Drained per-id during the render loop; cleared on graph reload.
  eastl::hash_set<int> pendingPositionIds;

  eastl::string lastSelectedNodeName;
  int selectedNodeId = -1;
  int navigationFramesLeft = 5;

  void drawCommentNode(const GraphData::Node &n);
  void drawBlockNode(const GraphData::Node &n);

  // Fills out_child_ids with the ids of every node (other than the block itself) whose
  // on-screen rect centre lies inside the block's on-screen rect. Uses ne::GetNodePosition
  // / ne::GetNodeSize so the layout reflects the live view (post-resize, post-pan). The
  // BLOCK_CONTAINMENT_GAP sentinel ensures the block is never seen as its own child.
  // Recursion is implicit: a nested block's centre is inside the outer rect, so the nested
  // block and everything inside it land in the output too -- one pass picks up the whole
  // transitive set.
  void collectNodesInsideBlock(int block_node_id, eastl::vector<int> &out_child_ids) const;

  // Node deletes captured during the deletion loop but waiting for the next actObjects tick
  // to resolve as a batch (so one prompt covers the whole selection and Cancel can roll back
  // all-or-nothing). For block nodes, childIds is the spatial-containment snapshot taken at
  // queue time -- ne::GetNodePosition / GetNodeSize would not be valid by actObjects time.
  // For non-block nodes childIds stays empty.
  struct PendingNodeDelete
  {
    int nodeId;
    eastl::vector<int> childIds;
  };
  eastl::vector<PendingNodeDelete> pendingNodeDeletes;

  // After-frame pass: mirror each block's current ne::GetNodeSize back into GraphData so a
  // user-driven resize (handled internally by ne::SizeAction for group nodes) round-trips
  // through save / reload. Must run while the imgui-node-editor is still current (between
  // ne::End and ne::SetCurrentEditor(nullptr)).
  void syncBlockSizes();

  // Perf overlay.
  float lastFrameMs = 0.0f;
  float emaFrameMs = 0.0f;
};
