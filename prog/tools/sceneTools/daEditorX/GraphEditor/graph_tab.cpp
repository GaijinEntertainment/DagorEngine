// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_tab.h"

#include "command_definitions.h"
#include "graph_document.h"
#include "graph_edge_render.h"
#include "graph_node_render.h"
#include "graph_panel.h"
#include "graph_pin_colors.h"
#include "graph_subgraph_window.h"
#include "graph_theme.h"
#include "graph_validation.h"
#include "node_drag_payload.h"
#include "plugin.h"
#include "pluginService/graph_tex_gen_service.h"

#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_interface.h>
#include <libTools/util/undo.h>
#include <oldEditor/de_cm.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <EASTL/algorithm.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/sort.h>
#include <EASTL/utility.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h> // ImGui::IsMouseDragPastThreshold
#include <imgui_node_editor.h>

namespace ne = ax::NodeEditor;

namespace
{
// Node level-of-detail (GraphTab::updateCanvas): at or below this canvas zoom the on-screen nodes are
// too small to read, so they render "reduced" (text and pin-square decoration dropped; boxes + links
// kept -- pins stay bound so links still land)
constexpr float LOD_SCALE = 0.1f;

// A node's live editor position must differ from the one it had at the press by more than this
// (canvas units) for the release to record a move -- guards against sub-pixel noise.
constexpr float NODE_MOVE_EPSILON = 0.1f;

// Canvas units a Duplicate offsets its copy by, so it does not hide on top of the originals.
constexpr float DUPLICATE_OFFSET = 24.0f;

// Canvas-unit offset to where Space drops the node: from the pin (FORWARD is along the flow, so an
// input pin mirrors it), or from the cursor on the edge (UP).
constexpr float ADD_NODE_DROP_FORWARD = 80.0f;
constexpr float ADD_NODE_DROP_DOWN = 140.0f;
constexpr float ADD_NODE_DROP_UP = 140.0f;

constexpr const char *TRANSIT_ARMED_HINT = "Hover over a pin / edge to activate Transit mode.";
constexpr int TRANSIT_GHOST_EDGE_WIDTH = 2;
constexpr ImU32 TRANSIT_GHOST_EDGE_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 0x80);

constexpr float GRAPH_EDITOR_ZOOM_LEVELS[] = {
  0.01f,
  0.025f,
  0.05f,
  0.075f,
  0.1f,
  0.15f,
  0.20f,
  0.25f,
  0.33f,
  0.5f,
  0.75f,
  1.0f,
  1.25f,
  1.50f,
  2.0f,
  2.5f,
  3.0f,
  4.0f,
  5.0f,
  6.0f,
  7.0f,
  8.0f,
};

void initEditorConfig(ne::Config &cfg)
{
  cfg.SettingsFile = nullptr; // do not persist anything to disk
  cfg.ContainGroupedNodesByCenter = true;
  cfg.DrawSelectionRectOnTop = true;
  cfg.NavigateButtonIndex = ImGuiMouseButton_Middle;

  for (float z : GRAPH_EDITOR_ZOOM_LEVELS)
  {
    cfg.CustomZoomLevels.push_back(z);
  }
}

void apply_editor_defaults(ne::EditorContext *ed)
{
  ne::SetCurrentEditor(ed);
  ne::EnableShortcuts(false);

  ne::Style &style = ne::GetStyle();
  style.SelectedNodeBorderWidth = 0.0f;
  style.HoveredNodeBorderWidth = 0.0f;
  // ne's default backdrop is translucent, so the themed window fill beneath it tinted the whole
  // canvas. Opaque here, at the colour that default composited to on the dark theme.
  style.Colors[ne::StyleColor_Bg] = ImColor(GRAPH_CANVAS_BG_COLOR);
  // Each of ne's three decorative link passes repaints the whole curve in one global colour, which would
  // throw away the per-type edge colour; hover and selection are conveyed by weight instead. A zero-alpha
  // stroke is skipped outright, so switching them off costs nothing. Highlight is inert while
  // StyleVar_HighlightConnectedLinks stays 0, but zeroing it keeps enabling that from resurfacing this.
  style.Colors[ne::StyleColor_HovLinkBorder] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  style.Colors[ne::StyleColor_SelLinkBorder] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  style.Colors[ne::StyleColor_HighlightLinkBorder] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  ne::SetCurrentEditor(nullptr);
}

bool shortcut_fired(const char *command_id)
{
  IEditorCommandSystem *commandSystem = EDITORCORE->queryEditorInterface<IEditorCommandSystem>();
  if (!commandSystem)
  {
    return false;
  }
  const int hotkeyCount = commandSystem->getCommandHotkeyCount(command_id);
  for (int i = 0; i < hotkeyCount; ++i)
  {
    if (ImGui::Shortcut(commandSystem->getCommandKeyChord(command_id, i),
          ImGuiInputFlags_RouteFocused | ImGuiInputFlags_RouteOverActive))
    {
      return true;
    }
  }
  return false;
}

// Takes the editor's undo and redo keys for the next frame: over its global accelerator, under an active text field.
void hold_undo_redo_keys()
{
  IEditorCommandSystem *commandSystem = EDITORCORE->queryEditorInterface<IEditorCommandSystem>();
  if (!commandSystem)
  {
    return;
  }
  for (const char *commandId : {EditorCommandIds::UNDO, EditorCommandIds::REDO})
  {
    const int hotkeyCount = commandSystem->getCommandHotkeyCount(commandId);
    for (int i = 0; i < hotkeyCount; ++i)
    {
      ImGui::Shortcut(commandSystem->getCommandKeyChord(commandId, i), ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused);
    }
  }
}

// Queues every currently-selected node and link into imgui-node-editor's deletion queue.
// The existing ne::BeginDelete loop later in the frame picks them up and runs the
// deferred / block-prompt path. Shared by the Delete and Cut shortcut dispatches.
// Requires ne::SetCurrentEditor active.
void queue_selected_for_delete()
{
  const int objCount = ne::GetSelectedObjectCount();
  if (objCount == 0)
  {
    return;
  }
  eastl::vector<ne::NodeId> selNodes;
  eastl::vector<ne::LinkId> selLinks;
  selNodes.resize(objCount);
  selLinks.resize(objCount);
  const int nodeCount = ne::GetSelectedNodes(selNodes.data(), objCount);
  const int linkCount = ne::GetSelectedLinks(selLinks.data(), objCount);
  selNodes.resize(nodeCount);
  selLinks.resize(linkCount);
  for (ne::NodeId id : selNodes)
  {
    ne::DeleteNode(id);
  }
  for (ne::LinkId id : selLinks)
  {
    ne::DeleteLink(id);
  }
}

void select_nodes_with_no_connected_outputs(const GraphData &gd)
{
  // Pin slots (node id + pin index) touched by an edge, on either endpoint, keyed by the same
  // make_pin_id used for rendering. Both endpoints are inserted so the result is independent of
  // stored edge orientation (matches the JS reference, which inspects both endpoints' roles).
  eastl::hash_set<uint64_t> connectedPins;
  connectedPins.reserve(gd.edges.size() * 2);
  for (const GraphData::Edge &e : gd.edges)
  {
    connectedPins.insert(make_pin_id(e.elemA, e.pinA));
    connectedPins.insert(make_pin_id(e.elemB, e.pinB));
  }

  ne::ClearSelection();
  for (const GraphData::Node &n : gd.nodes)
  {
    bool hasOutputPin = false;
    bool hasConnectedOutput = false;
    for (int j = 0; j < static_cast<int>(n.pins.size()); ++j)
    {
      if (n.pins[j].role != PinRole::Out)
      {
        continue;
      }
      hasOutputPin = true;
      if (connectedPins.find(make_pin_id(n.id, j)) != connectedPins.end())
      {
        hasConnectedOutput = true;
        break;
      }
    }
    if (hasOutputPin && !hasConnectedOutput)
    {
      ne::SelectNode(ne::NodeId(make_node_id(n.id)), /*append=*/true);
    }
  }
}

// Landing the destination pin under the cursor is what makes a second Tab walk back along the edge just
// followed, so screen_target is normally the mouse.
void scroll_pin_to_screen(int node_id, int pin_index, const ImVec2 &screen_target)
{
  const ImVec2 pinCanvas = ne::GetPinPosition(ne::PinId(make_pin_id(node_id, pin_index)));
  ne::ScrollCanvasPointToScreen(pinCanvas, screen_target);
}

// Pin under the cursor. The out params are always written, so a caller may ignore the return.
bool hovered_pin(int &out_node, int &out_pin)
{
  out_node = -1;
  out_pin = -1;
  const ne::PinId hoveredPin = ne::GetHoveredPin();
  if (!hoveredPin)
  {
    return false;
  }
  decode_pin_id(hoveredPin.Get(), out_node, out_pin);
  return out_node >= 0 && out_pin >= 0;
}

// Where a node added at this pin drops: forward along the flow, and below it. The picker's ghost and
// the node that follows it both place from here, so the offsets stay in one body.
ImVec2 drop_point_at_pin(const GraphData &gd, int node_id, int pin_index, bool &out_is_output)
{
  const GraphData::Pin *const pin = find_pin(gd, node_id, pin_index);
  out_is_output = pin && pin->role == PinRole::Out;
  const ImVec2 pinCanvas = ne::GetPinPosition(ne::PinId(make_pin_id(node_id, pin_index)));
  const float dx = out_is_output ? ADD_NODE_DROP_FORWARD : -ADD_NODE_DROP_FORWARD;
  return ImVec2(pinCanvas.x + dx, pinCanvas.y + ADD_NODE_DROP_DOWN);
}

// Node top-left for a drop point already clamped into view. Screen space, because that is where the
// ghost sized the body: the node lands on the ghost at any zoom, and every path that places one goes
// through here so they cannot drift apart.
ImVec2 spawn_pos_for_clamped_drop(const ImVec2 &drop_canvas, bool source_is_output)
{
  ImVec2 ghostOffset, ghostBodyMax;
  add_node_ghost_body_box(source_is_output, ghostOffset, ghostBodyMax);
  const ImVec2 dropScreen = ne::CanvasToScreen(drop_canvas);
  return ne::ScreenToCanvas(ImVec2(dropScreen.x + ghostOffset.x, dropScreen.y + ghostOffset.y));
}

// ne finds links and ImGui takes drops by the rect alone, even under a window drawn over the canvas.
bool cursor_on_canvas_window()
{
  return ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
}

// Source pin of the edge under the cursor; returns the edge, or -1 when there is none to act on.
int hovered_edge_source_pin(const GraphData &gd, int &out_node, int &out_pin)
{
  out_node = -1;
  out_pin = -1;
  const ne::LinkId hoveredLink = cursor_on_canvas_window() ? ne::GetHoveredLink() : ne::LinkId();
  if (!hoveredLink)
  {
    return -1;
  }
  const int edgeId = decode_link_id(hoveredLink.Get());
  const GraphData::Edge *const edge = find_edge_by_id(gd, edgeId);
  return edge && edge_source_pin(gd, *edge, out_node, out_pin) ? edgeId : -1;
}

// Pin nearest a canvas point, within a dot's reach; both out params stay -1 when there is none.
// Own hit test: ne publishes no hot object while another item holds ActiveId, which is the library
// drag, nor while an action of its own runs, which a press on a pin becomes after a pixel of drift.
void pin_at_canvas_point(const GraphData &gd, const GraphCanvasCull &cull, const ImVec2 &mouse, int &out_node, int &out_pin)
{
  out_node = -1;
  out_pin = -1;
  const float reach = pin_hit_radius();

  // One node first, off the cull's cached rects, so only one node's pins reach ne::GetPinPosition.
  // Overlapping rects take the last match, which is the node the draw pass submits last.
  int nodeIndex = -1;
  for (int ni = 0; ni < static_cast<int>(gd.nodes.size()); ++ni)
  {
    // By id, not by position: this runs before the cull rebuilds, so a node removed since then has
    // shifted every index after it and the rect would belong to another node than the pins below.
    const int culled = cull.indexOf(gd.nodes[ni].id);
    ImVec2 rectMin, rectMax;
    if (culled < 0 || !cull.nodeRect(culled, rectMin, rectMax))
    {
      continue;
    }
    if (mouse.x >= rectMin.x - reach && mouse.x <= rectMax.x + reach && mouse.y >= rectMin.y - reach && mouse.y <= rectMax.y + reach)
    {
      nodeIndex = ni;
    }
  }
  if (nodeIndex < 0)
  {
    return;
  }

  const GraphData::Node &node = gd.nodes[nodeIndex];
  float bestDistSq = reach * reach;
  for (int i = 0; i < static_cast<int>(node.pins.size()); ++i)
  {
    if (!is_pin_reachable(node, i))
    {
      continue;
    }
    const ImVec2 center = ne::GetPinPosition(ne::PinId(make_pin_id(node.id, i)));
    const float dx = center.x - mouse.x;
    const float dy = center.y - mouse.y;
    if (dx * dx + dy * dy <= bestDistSq)
    {
      bestDistSq = dx * dx + dy * dy;
      out_node = node.id;
      out_pin = i;
    }
  }
}

enum class BlockDeleteChoice
{
  DELETE_WITH_CHILDREN,
  KEEP_CHILDREN,
  CANCEL,
};

BlockDeleteChoice promptBlockDelete()
{
  const int ret = wingw::message_box(wingw::MBS_YESNOCANCEL, "Delete block",
    "One or more selected group nodes have children.\n\n"
    "Do you want to delete child nodes too?");

  switch (ret)
  {
    case wingw::MB_ID_YES: return BlockDeleteChoice::DELETE_WITH_CHILDREN;
    case wingw::MB_ID_NO: return BlockDeleteChoice::KEEP_CHILDREN;
    default: return BlockDeleteChoice::CANCEL;
  }
}

// An undo index past the end is a stale one, so it appends instead of failing.
template <class T>
void insert_or_append(eastl::vector<T> &list, int index, T item)
{
  const bool inside = index >= 0 && index < static_cast<int>(list.size());
  list.insert(inside ? list.begin() + index : list.end(), eastl::move(item));
}
} // namespace

GraphTab::GraphTab(GraphEditorPlg &plg, int tab_id, GraphTab *root, SubgraphWindow *host) :
  plugin(plg),
  doc(plg, *this),
  graphData(doc.getGraphData()),
  compiler(plg.createGraphCompiler(doc)),
  tabId(tab_id),
  rootTab(root ? root : this),
  hostWindow(host)
{
  seenDescriptorGeneration = plugin.getSubgraphDescriptorGeneration();
  resetEditor();
}

GraphTab::~GraphTab()
{
  if (editor)
  {
    ne::DestroyEditor(editor);
    editor = nullptr;
  }
}

IGraphTexGenService *GraphTab::texGen() const { return plugin.getTexGenService(); }

// The cull cache holds screen bounds from the last frame this canvas drew, which is not this one.
void GraphTab::onActivated()
{
  canvasCull.markDirty();
  // A press taken before the tab was hidden had its release unseen.
  pressPositions.clear();
}

void GraphTab::onDeactivated()
{
  // Popups live in the shared ImGui id space and gestures track the mouse: none may outlive the switch.
  addNodePopup.close();
  pinJumpMenu.close();
  contextMenu.close();
  if (edgeReconnect.isActive())
  {
    doc.recordReconnectEdge(reconnectRemovedEdge, reconnectRemovedEdgeIndex, nullptr); // the Escape outcome: the edge stays dropped
    edgeReconnect.cancel();
  }
  linkDrop = LinkDropOnCanvas();
  transitArmed = false;
  pendingRerouteEdge = -1;
  dropTargetPinNode = -1;
  // pressPositions stays: focus can leave a surface mid-drag while it is still drawn and sees the release.
  blockResizeOld.clear();
  // The service drops its preview final on setGraphData, so the return has to push it again.
  lastSelectedNodeName.clear();
}

eastl::string GraphTab::getName() const
{
  const eastl::string &path = graphData.sourcePath;
  if (path.empty())
  {
    // Tab ids are handed out in order and never reused, so they number the untitled graphs too.
    return eastl::string(eastl::string::CtorSprintf(), "untitled_%02d", tabId);
  }
  bool isSubgraph = false;
  const eastl::string stem = graph_file_stem(path.c_str(), &isSubgraph);
  // The design names a subgraph apart from a graph.
  return isSubgraph ? eastl::string(eastl::string::CtorSprintf(), "Sub-graph: %s", stem.c_str()) : stem;
}

eastl::string GraphTab::getPathOrName() const
{
  const eastl::string &path = graphData.sourcePath;
  return path.empty() ? getName() : path;
}

bool GraphTab::isModified() const
{
  // The tab is its own undo owner token, which the undo system compares and never dereferences.
  return !EDITORCORE->getUndoSystem()->is_at_save_point(const_cast<GraphTab *>(this));
}

void GraphTab::markSaved() { EDITORCORE->getUndoSystem()->set_save_point(this); }

void GraphTab::resetEditor()
{
  if (editor)
  {
    ne::DestroyEditor(editor);
  }
  ne::Config cfg;
  initEditorConfig(cfg);
  editor = ne::CreateEditor(&cfg);
  apply_editor_defaults(editor);
}

void GraphTab::onGraphDataChanged()
{
  // Recreate editor so node-state caches don't leak across graphs.
  resetEditor();

  navigationFramesLeft = 5;
  canvasCull.markDirty();

  // Clear any stale selection -- the previous graph's selected node id is meaningless
  // against the new node set (most often: previous was non-empty, new is empty).
  selectedNodeId = -1;
  previewNodeId = -1;
  // The popup is aimed at a pin id from the old graph, which the new one may well reuse.
  addNodePopup.close();
  linkDrop = LinkDropOnCanvas();
  // Its target is a pin of the old graph; the per-frame resolve clears the rest.
  transitArmed = false;
  pendingRerouteEdge = -1; // an edge id the new graph may well reuse
  pressPositions.clear();  // keyed by node ids the new graph may well reuse

  // Reset selection-undo tracking: the new graph starts with an empty selection, and the swap itself
  // must not record a "Select nodes" entry (the next settled frame resyncs the baseline).
  lastSelection = GraphSelection();
  pendingSelection = GraphSelection();
  hasPendingSelection = false;
  suppressSelectionRecord = true;

  // Every loaded node needs its position pushed to ne::SetNodePosition once on first render.
  pendingPositionIds.clear();
  // ne is recreated with the graph, so this only drops ids the new graph does not use.
  appliedBlockGroupSizes.clear();
  for (const GraphData::Node &n : graphData.nodes)
  {
    pendingPositionIds.insert(n.id);
  }

  if (texGen() && !lastSelectedNodeName.empty())
  {
    texGen()->setPreviewFinal(nullptr);
  }
  lastSelectedNodeName.clear();
}

void GraphTab::addNode(GraphData::Node node, int index)
{
  pendingPositionIds.insert(node.id);
  doc.mutateGraphData([&](GraphData &gd) { insert_or_append(gd.nodes, index, eastl::move(node)); });
  canvasCull.markDirty();
}

void GraphTab::markPositionsPending(const eastl::vector<int> &node_ids)
{
  for (int id : node_ids)
  {
    pendingPositionIds.insert(id);
  }
  canvasCull.markDirty();
}

void GraphTab::markBlockSizesChanged() { canvasCull.markDirty(); }

int GraphTab::allocateNodeId() const
{
  int maxId = -1;
  for (const GraphData::Node &n : graphData.nodes)
  {
    maxId = eastl::max(maxId, n.id);
  }
  return maxId + 1;
}

bool GraphTab::isBaseNodeDropTargetHot() const { return dropTargetHotFrame == ImGui::GetFrameCount(); }

// Reads graphData, the structure revision, the compiled stamp and the seed map without graphMutex.
// Safe only because main is the only writer of all four: the texgen worker holds the mutex but
// writes the compiled BLKs and the pending results (see GraphCompilerImpl), and the pending results
// reach the stamp and the map on main, through applyPendingCompileResults and replaceGraphData. A
// worker that started writing any of the four would race this silently.
void GraphTab::refreshDerivedState()
{
  const uint64_t rev = doc.getStructureRevision();
  const uint64_t compiledStamp = doc.getCompiledResolutionsStamp();
  if (rev == derivedStateRevision && compiledStamp == compiledSeedStamp)
  {
    return;
  }
  rebuildDerivedState();
  derivedStateRevision = rev;
  compiledSeedStamp = compiledStamp;
}

void GraphTab::rebuildDerivedState()
{
  eastl::hash_map<int, int> idToIndex;
  build_node_index(graphData, idToIndex);
  compute_dead_paths(graphData, idToIndex, deadPaths);

  linkedPins.clear();
  livePins.clear();
  for (int i = 0; i < static_cast<int>(graphData.edges.size()); ++i)
  {
    const GraphData::Edge &e = graphData.edges[i];
    const uint64_t a = make_pin_id(e.elemA, e.pinA);
    const uint64_t b = make_pin_id(e.elemB, e.pinB);
    linkedPins.insert(a);
    linkedPins.insert(b);
    if (!deadPaths.isDeadEdge(i))
    {
      livePins.insert(a);
      livePins.insert(b);
    }
  }

  eastl::vector<const DataBlock *> nodeDescriptors(graphData.nodes.size(), nullptr);
  for (int i = 0; i < static_cast<int>(graphData.nodes.size()); ++i)
  {
    nodeDescriptors[i] = plugin.findBaseNodeBlockByUid(graphData.nodes[i].templateUid.c_str());
  }
  PinConnectivity connectivity;
  build_pin_connectivity(graphData, deadPaths, idToIndex, connectivity);
  // Seeds the subgraph instances, whose bodies this pass never sees.
  compute_graph_resolutions(graphData, nodeDescriptors, idToIndex, deadPaths, connectivity, &doc.getCompiledNodeResolutions(),
    resolutions);
}

void GraphTab::addEdge(GraphData::Edge edge, int index)
{
  doc.mutateGraphData([&](GraphData &gd) { insert_or_append(gd.edges, index, eastl::move(edge)); });
  plugin.markGraphDirtyAndRegen(doc);
  canvasCull.markDirty();
}

bool GraphTab::removeEdgeById(int edge_id)
{
  bool erased = false;
  doc.mutateGraphData([&](GraphData &gd) { erased = erase_edge(gd, edge_id); });
  if (erased)
  {
    plugin.markGraphDirtyAndRegen(doc);
    canvasCull.markDirty();
  }
  return erased;
}

bool GraphTab::removeNodeById(int node_id)
{
  bool erasedAny = false;
  doc.mutateGraphData([&](GraphData &gd) { erasedAny = erase_node_and_incident_edges(gd, node_id); });
  if (erasedAny)
  {
    if (previewNodeId == node_id)
    {
      previewNodeId = -1;
    }
    plugin.markGraphDirtyAndRegen(doc);
    canvasCull.markDirty();
  }
  return erasedAny;
}

void GraphTab::collectNodesInsideBlock(int block_node_id, eastl::vector<int> &out_child_ids) const
{
  out_child_ids.clear();

  const ImVec2 blockMin = ne::GetNodePosition(ne::NodeId(make_node_id(block_node_id)));
  const ImVec2 blockSize = ne::GetNodeSize(ne::NodeId(make_node_id(block_node_id)));
  if (blockSize.x <= 0.0f || blockSize.y <= 0.0f)
  {
    return;
  }
  const ImVec2 blockMax(blockMin.x + blockSize.x, blockMin.y + blockSize.y);

  for (const GraphData::Node &n : graphData.nodes)
  {
    if (n.id == block_node_id)
    {
      continue;
    }
    const ImVec2 pos = ne::GetNodePosition(ne::NodeId(make_node_id(n.id)));
    const ImVec2 size = ne::GetNodeSize(ne::NodeId(make_node_id(n.id)));
    if (size.x <= 0.0f || size.y <= 0.0f)
    {
      continue;
    }
    const ImVec2 centre(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
    if (centre.x > blockMin.x && centre.x < blockMax.x && centre.y > blockMin.y && centre.y < blockMax.y)
    {
      out_child_ids.push_back(n.id);
    }
  }
}

void GraphTab::actObjects([[maybe_unused]] float dt)
{
  // After stampUndoOwner, so the entry is this tab's; waits out a reroute, whose edge may name an instance.
  const uint32_t descriptorGeneration = plugin.getSubgraphDescriptorGeneration();
  if (seenDescriptorGeneration != descriptorGeneration && !edgeReconnect.isActive())
  {
    doc.resyncSubgraphInstances(seenDescriptorGeneration);
    seenDescriptorGeneration = descriptorGeneration;
  }

  if (!pendingCreateSubgraphIds.empty())
  {
    eastl::vector<int> nodeIds;
    nodeIds.swap(pendingCreateSubgraphIds);
    plugin.createSubgraphFromSelection(*this, nodeIds);
  }

  if (pendingCommentNodeId >= 0)
  {
    const int nodeId = pendingCommentNodeId;
    const int pinIndex = pendingCommentPinIndex;
    pendingCommentNodeId = -1;
    pendingCommentPinIndex = -1;

    eastl::string comment;
    const GraphData::Node *const node = find_node_by_id(graphData, nodeId);
    if (node && pinIndex >= 0 && pinIndex < static_cast<int>(node->pins.size()))
    {
      comment = node->pins[pinIndex].comment;
    }

    if (plugin.promptPinComment(comment))
    {
      doc.setPinComment(nodeId, pinIndex, comment);
    }
  }

  resolvePendingDeletes();

  // Last: a switch stamps the other tab as the undo owner, and the edits above belong to this one.
  if (!pendingOpenSubgraphStem.empty())
  {
    eastl::string stem;
    stem.swap(pendingOpenSubgraphStem);
    plugin.getGraphPanel().openSubgraph(*this, stem.c_str());
  }
}

void GraphTab::resolvePendingDeletes()
{
  if (pendingNodeDeletes.empty())
  {
    return;
  }

  eastl::vector<PendingNodeDelete> pending;
  pending.swap(pendingNodeDeletes);

  eastl::hash_set<int> explicitlySelected;
  for (const PendingNodeDelete &p : pending)
  {
    explicitlySelected.insert(p.nodeId);
  }

  eastl::hash_set<int> implicitChildren;
  for (const PendingNodeDelete &p : pending)
  {
    for (int childId : p.childIds)
    {
      if (explicitlySelected.find(childId) == explicitlySelected.end())
      {
        implicitChildren.insert(childId);
      }
    }
  }

  // Prompt only when there's actually an implicit child to ask about. Cancel rolls back the
  // entire batch -- nothing gets deleted -- so a mixed selection with one block-with-children
  // is consistent.
  bool deleteImplicit = false;
  if (!implicitChildren.empty())
  {
    const BlockDeleteChoice choice = promptBlockDelete();
    if (choice == BlockDeleteChoice::CANCEL)
    {
      return;
    }
    deleteImplicit = (choice == BlockDeleteChoice::DELETE_WITH_CHILDREN);
  }

  eastl::vector<int> idsToDelete;
  idsToDelete.reserve(pending.size() + (deleteImplicit ? implicitChildren.size() : 0));
  for (const PendingNodeDelete &p : pending)
  {
    idsToDelete.push_back(p.nodeId);
  }
  if (deleteImplicit)
  {
    for (int childId : implicitChildren)
    {
      idsToDelete.push_back(childId);
    }
  }
  // One undo step for the whole Delete action; the multi-selection / block-plus-children set
  // resolves atomically.
  doc.deleteNodes(idsToDelete);
}

void GraphTab::syncBlockSizes()
{
  // After ne::End(), the editor's SizeAction may have mutated m_Bounds / m_GroupBounds for a
  // group node the user dragged a border on. draw_block_node stacks a header dummy, the ne::Group
  // dummy and a 1px sentinel with ItemSpacing zeroed, so GetNodeSize returns
  // (widthClamp, heightClamp + BLOCK_CONTAINMENT_GAP). We mirror any change back into GraphData so
  // the next save/reload preserves the user's resize. Skip zero-sized reads -- ne reports 0,0 for
  // nodes that haven't been laid out yet (the frame a new block is spawned).
  //
  // Clamp to MIN_BLOCK_SIZE so even if the user drags below the floor mid-session the
  // persisted value lands >= MIN_BLOCK_SIZE (JS parity, graphEditor.js:2756). draw_block_node
  // applies the same floor -- widened to the caption box for a block authored below it -- so once
  // a write lands the next frame's clamped size matches and the diff check skips: no ping-pong.
  const int nodeCount = static_cast<int>(graphData.nodes.size());
  for (int ni = 0; ni < nodeCount; ++ni)
  {
    const GraphData::Node &n = graphData.nodes[ni];
    if (n.descName != "block")
    {
      continue;
    }
    // A culled block was not submitted, so ne still reports the bounds it had when it last drew;
    // reading those back would clobber a size a resize undo changed since.
    if (!canvasCull.needed(ni))
    {
      continue;
    }
    const ImVec2 sz = ne::GetNodeSize(ne::NodeId(make_node_id(n.id)));
    if (sz.x <= 0.0f || sz.y <= 0.0f)
    {
      continue;
    }
    // sz.y includes the 1px BLOCK_CONTAINMENT_GAP sentinel Dummy; subtract it so n.blockHeight
    // mirrors the user-perceived group height (matches what was passed to ne::Group on the
    // first frame and what the GroupBorder outlines visually).
    const float newWidth = eastl::max(MIN_BLOCK_SIZE, sz.x);
    const float newHeight = eastl::max(MIN_BLOCK_SIZE, sz.y - BLOCK_CONTAINMENT_GAP);
    if (fabsf(newWidth - n.blockWidth) < 0.5f && fabsf(newHeight - n.blockHeight) < 0.5f)
    {
      continue;
    }
    // First size change of an active resize drag: remember the pre-change size for the resize undo.
    // Gate on a held mouse button: a size delta with the mouse up is ne re-flooring a size we just set
    // out of frame (load / resize undo / spawn), not a user resize. Recording that would leave a stale
    // blockResizeOld entry -- nothing clears it until the next release, where it would be folded into an
    // unrelated block's resize undo and silently resize this block on that undo.
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
      bool capturedOld = false;
      for (const BlockSize &b : blockResizeOld)
      {
        if (b.nodeId == n.id)
        {
          capturedOld = true;
          break;
        }
      }
      if (!capturedOld)
      {
        blockResizeOld.push_back(BlockSize{n.id, n.blockWidth, n.blockHeight});
      }
    }
    const int nodeId = n.id;
    doc.mutateWithoutInvalidating([&](GraphData &gd) {
      for (GraphData::Node &node : gd.nodes)
      {
        if (node.id != nodeId)
        {
          continue;
        }
        node.blockWidth = newWidth;
        node.blockHeight = newHeight;
        break;
      }
    });
  }
}

void GraphTab::showNextSelectedNode()
{
  eastl::fixed_vector<int, 32, true> selected;
  for (const GraphData::Node &n : graphData.nodes)
  {
    if (ne::IsNodeSelected(ne::NodeId(make_node_id(n.id))))
    {
      selected.push_back(n.id);
    }
  }
  if (selected.empty())
  {
    lastShownSelectedNodeId = -1;
    return;
  }

  // Advance to the node after the one shown last, wrapping. If the last-shown id is no longer in
  // the selection (selection changed since), idx stays 0 so we restart from the first selected node.
  int idx = 0;
  for (int i = 0; i < static_cast<int>(selected.size()); ++i)
  {
    if (selected[i] == lastShownSelectedNodeId)
    {
      idx = (i + 1) % static_cast<int>(selected.size());
      break;
    }
  }

  lastShownSelectedNodeId = selected[idx];

  ne::SelectNode(ne::NodeId(make_node_id(lastShownSelectedNodeId)), /*append=*/false);
  ne::NavigateToSelection(/*zoomIn=*/true);

  ne::ClearSelection();
  for (int id : selected)
  {
    ne::SelectNode(ne::NodeId(make_node_id(id)), /*append=*/true);
  }
}

void GraphTab::removeSelectedKeepingConnections()
{
  eastl::hash_set<int> selected;
  for (const GraphData::Node &n : graphData.nodes)
  {
    if (ne::IsNodeSelected(ne::NodeId(make_node_id(n.id))))
    {
      selected.insert(n.id);
    }
  }
  if (selected.empty())
  {
    return;
  }

  // node id -> node, to resolve an edge endpoint's pin role. Stored edges are not guaranteed to be
  // oriented out->in (graphs loaded from the JS editor may store either order), so the role is read
  // from the pin rather than assumed from the A/B endpoint.
  eastl::hash_map<int, const GraphData::Node *> byId;
  byId.reserve(graphData.nodes.size());
  for (const GraphData::Node &n : graphData.nodes)
  {
    byId[n.id] = &n;
  }
  auto pinRoleOf = [&byId](int node_id, int pin_idx) -> PinRole {
    const auto it = byId.find(node_id);
    if (it == byId.end() || pin_idx < 0 || pin_idx >= static_cast<int>(it->second->pins.size()))
    {
      return PinRole::Any; // unknown -> neither in nor out, so ignored below
    }
    return it->second->pins[pin_idx].role;
  };

  // Walk edges crossing the selection boundary: the external pin feeding the lowest-indexed
  // selected input becomes the single upstream source; every external pin a selected output feeds
  // becomes a downstream consumer to reconnect.
  struct BridgeConsumer
  {
    int nodeId;
    int pinIdx;
    bool muted;
  };
  int srcNode = -1;
  int srcPin = -1;
  int bestInputPin = 0;
  bool srcMuted = false;
  eastl::vector<BridgeConsumer> consumers;
  for (const GraphData::Edge &e : graphData.edges)
  {
    const bool aSel = selected.find(e.elemA) != selected.end();
    const bool bSel = selected.find(e.elemB) != selected.end();
    if (aSel == bSel)
    {
      continue; // both selected (internal edge) or neither (untouched) -- nothing to bridge
    }

    const int selNode = aSel ? e.elemA : e.elemB;
    const int selPin = aSel ? e.pinA : e.pinB;
    const int extNode = aSel ? e.elemB : e.elemA;
    const int extPin = aSel ? e.pinB : e.pinA;

    const PinRole role = pinRoleOf(selNode, selPin);
    if (role == PinRole::In)
    {
      if (srcNode < 0 || selPin < bestInputPin)
      {
        srcNode = extNode;
        srcPin = extPin;
        bestInputPin = selPin;
        srcMuted = e.muted;
      }
    }
    else if (role == PinRole::Out)
    {
      consumers.push_back(BridgeConsumer{extNode, extPin, e.muted});
    }
  }

  eastl::vector<GraphData::Node> removedNodes;
  eastl::vector<int> removedNodeIndices;
  eastl::vector<GraphData::Edge> removedEdges;
  eastl::vector<int> removedEdgeIndices;
  doc.snapshotRemoval(selected, removedNodes, removedNodeIndices, removedEdges, removedEdgeIndices);

  // One pass: drop every edge touching the selection, drop the selected nodes, then add the bridge
  // edges. Validation runs against the spliced graph (matching the JS, which reconnects after the
  // deletions), so a consumer's input pin reads as free once the removed node's edges are gone.
  eastl::vector<GraphData::Edge> bridgeEdges; // reconnect edges this op adds, with final ids -- for undo
  doc.mutateGraphData([&](GraphData &gd) {
    gd.edges.erase(eastl::remove_if(gd.edges.begin(), gd.edges.end(),
                     [&selected](const GraphData::Edge &e) {
                       return selected.find(e.elemA) != selected.end() || selected.find(e.elemB) != selected.end();
                     }),
      gd.edges.end());

    gd.nodes.erase(eastl::remove_if(gd.nodes.begin(), gd.nodes.end(),
                     [&selected](const GraphData::Node &n) { return selected.find(n.id) != selected.end(); }),
      gd.nodes.end());

    if (srcNode < 0)
    {
      return;
    }
    int nextEdgeId = 0;
    for (const GraphData::Edge &e : gd.edges)
    {
      nextEdgeId = eastl::max(nextEdgeId, e.id + 1);
    }
    for (const BridgeConsumer &c : consumers)
    {
      if (validate_new_edge(gd, srcNode, srcPin, c.nodeId, c.pinIdx))
      {
        GraphData::Edge edge;
        edge.id = nextEdgeId++;
        edge.elemA = srcNode;
        edge.pinA = srcPin;
        edge.elemB = c.nodeId;
        edge.pinB = c.pinIdx;
        // The bridge replaces source -> removed node -> consumer, so either hop's mute carries:
        // removing a node must not switch a muted path back on.
        edge.muted = srcMuted || c.muted;
        bridgeEdges.push_back(edge); // record before the move so undo can erase it by id
        gd.edges.push_back(eastl::move(edge));
      }
    }
  });
  plugin.markGraphDirtyAndRegen(doc);

  doc.recordRemoveKeepingConnections(eastl::move(removedNodes), eastl::move(removedNodeIndices), eastl::move(removedEdges),
    eastl::move(removedEdgeIndices), eastl::move(bridgeEdges));

  ne::ClearSelection();
}

void GraphTab::readSelection(GraphSelection &out) const
{
  out.nodes.clear();
  out.links.clear();
  const int objCount = ne::GetSelectedObjectCount();
  if (objCount == 0)
  {
    return;
  }
  eastl::vector<ne::NodeId> selNodes;
  selNodes.resize(objCount);
  const int nodeCount = ne::GetSelectedNodes(selNodes.data(), objCount);
  out.nodes.reserve(nodeCount);
  for (int i = 0; i < nodeCount; ++i)
  {
    out.nodes.push_back(static_cast<int>(selNodes[i].Get()) - 1);
  }
  eastl::sort(out.nodes.begin(), out.nodes.end());

  eastl::vector<ne::LinkId> selLinks;
  selLinks.resize(objCount);
  const int linkCount = ne::GetSelectedLinks(selLinks.data(), objCount);
  out.links.reserve(linkCount);
  for (int i = 0; i < linkCount; ++i)
  {
    out.links.push_back(static_cast<int>(selLinks[i].Get()) - 1);
  }
  eastl::sort(out.links.begin(), out.links.end());
}

void GraphTab::setPendingSelection(const GraphSelection &selection)
{
  pendingSelection = selection;
  hasPendingSelection = true;
}

void GraphTab::foldLivePositions(const eastl::vector<int> &node_ids)
{
  eastl::vector<NodePos> live;
  live.reserve(node_ids.size());
  for (int id : node_ids)
  {
    // A position push still pending means graphData holds the real position and ne does not yet.
    if (pendingPositionIds.find(id) == pendingPositionIds.end())
    {
      const ImVec2 pos = ne::GetNodePosition(ne::NodeId(make_node_id(id)));
      live.push_back(NodePos{id, pos.x, pos.y});
    }
  }
  if (live.empty())
  {
    return;
  }
  doc.writeNodePositions(live);
}

void GraphTab::requestExplodeSelected()
{
  GraphSelection sel;
  readSelection(sel);
  if (sel.nodes.size() == 1)
  {
    requestExplode(sel.nodes[0]);
  }
}

void GraphTab::requestExplode(int node_id)
{
  // A reroute records its undo entry when it resolves, and that entry may name this node.
  if (edgeReconnect.isActive())
  {
    return;
  }
  const GraphData::Node *const node = find_node_by_id(graphData, node_id);
  if (!node || !is_subgraph_instance(*node))
  {
    return;
  }
  foldLivePositions({node_id}); // the body lands where the instance is now
  plugin.explodeSubgraphInstance(*this, node_id);
}

void GraphTab::requestCreateSubgraph()
{
  // A reroute resolves on the release the dialog would take, and its undo entry may name these nodes.
  if (edgeReconnect.isActive())
  {
    return;
  }
  GraphSelection sel;
  readSelection(sel);
  eastl::vector<int> nodeIds;
  for (int id : sel.nodes)
  {
    if (find_node_by_id(graphData, id))
    {
      nodeIds.push_back(id);
    }
  }
  if (nodeIds.size() < 2)
  {
    return;
  }
  foldLivePositions(nodeIds); // the child keeps the positions the user sees
  pendingCreateSubgraphIds = eastl::move(nodeIds);
}

GraphTab::SubgraphContext GraphTab::resolveSubgraphContext(const GraphSelection &sel) const
{
  SubgraphContext ctx;
  ctx.selectedNodeCount = static_cast<int>(sel.nodes.size());
  if (sel.nodes.size() == 1)
  {
    const GraphData::Node *const node = find_node_by_id(graphData, sel.nodes[0]);
    if (node && is_subgraph_instance(*node))
    {
      ctx.selectedSubgraph = node;
      ctx.selectedSubgraphOpen = plugin.getGraphPanel().isSubgraphOpen(node->descName.c_str());
    }
  }
  return ctx;
}

void GraphTab::removeEdgesUnderCursor()
{
  int nodeId = -1;
  int pinIndex = -1;
  if (!hovered_pin(nodeId, pinIndex))
  {
    return;
  }

  eastl::vector<PinEdge> pinEdges;
  collect_pin_edges(graphData, nodeId, pinIndex, pinEdges);
  eastl::vector<int> edgeIds;
  edgeIds.reserve(pinEdges.size());
  for (const PinEdge &pinEdge : pinEdges)
  {
    edgeIds.push_back(pinEdge.edgeId);
  }
  // Snapshots, erases, and records one "Delete edges" entry (no-op if the pin had no edges).
  doc.deleteEdges(edgeIds);
}

void GraphTab::openAddNodePopupAtPin(int node_id, int pin_index, const ImVec2 &canvas_min, const ImVec2 &canvas_max, bool splice)
{
  // openAt refuses a pin it cannot resolve, so there is nothing to check for first.
  bool isOutput = false;
  const ImVec2 drop = drop_point_at_pin(graphData, node_id, pin_index, isOutput);
  addNodePopup.openAt(graphData, plugin, node_id, pin_index, drop, canvas_min, canvas_max, splice, /*anchor_edge=*/-1);
}

ImVec2 GraphTab::spawnPosForDrop(const ImVec2 &drop_canvas, bool source_is_output, const ImVec2 &canvas_min,
  const ImVec2 &canvas_max) const
{
  return spawn_pos_for_clamped_drop(clamp_add_node_drop_point(source_is_output, drop_canvas, canvas_min, canvas_max),
    source_is_output);
}

ImVec2 GraphTab::spawnPosAtPin(int node_id, int pin_index, const ImVec2 &canvas_min, const ImVec2 &canvas_max) const
{
  bool isOutput = false;
  const ImVec2 drop = drop_point_at_pin(graphData, node_id, pin_index, isOutput);
  return spawnPosForDrop(drop, isOutput, canvas_min, canvas_max);
}

bool GraphTab::canSpliceDrop(const char *template_uid, int node_id, int pin_index) const
{
  GraphData::Node candidate;
  if (!plugin.makeNodeFromBaseBlk(template_uid, 0.0f, 0.0f, candidate))
  {
    return false;
  }
  SpliceEnds ends;
  splice_ends(graphData, node_id, pin_index, /*anchor_edge_id=*/-1, ends);
  return !ends.sinks.empty() && can_node_splice(graphData, ends, candidate);
}

void GraphTab::resolveBaseNodeDropTarget()
{
  dropTargetPinNode = -1;
  dropTargetPinIndex = -1;

  const ImGuiPayload *const payload = ImGui::GetDragDropPayload();
  if (!payload || !payload->IsDataType(BASE_NODE_DRAG_PAYLOAD) || !cursor_on_canvas_window())
  {
    return;
  }
  pin_at_canvas_point(graphData, canvasCull, ne::ScreenToCanvas(ImGui::GetMousePos()), dropTargetPinNode, dropTargetPinIndex);
}

void GraphTab::resolveArmedTransitTarget()
{
  transitTargetNode = -1;
  transitTargetPin = -1;
  transitTargetEdge = -1;
  if (!transitArmed || !cursor_on_canvas_window())
  {
    return;
  }

  // The same two targets the hotkey takes directly, under the same rule: there has to be a wire. The
  // pin needs the own hit test: by the release frame a press on one is ne's create-link, which
  // publishes no hover. A press on a link keeps it as long as the cursor stays on the link.
  int node = -1;
  int pin = -1;
  int edgeId = -1;
  pin_at_canvas_point(graphData, canvasCull, ne::ScreenToCanvas(ImGui::GetMousePos()), node, pin);
  if (node < 0)
  {
    edgeId = hovered_edge_source_pin(graphData, node, pin);
    if (edgeId < 0)
    {
      return;
    }
  }
  if (!has_splice_target(graphData, node, pin, edgeId))
  {
    return;
  }

  transitTargetNode = node;
  transitTargetPin = pin;
  transitTargetEdge = edgeId;
}

void GraphTab::updateArmedTransit(const ImVec2 &canvas_min, const ImVec2 &canvas_max)
{
  if (!transitArmed)
  {
    return;
  }
  // A reroute takes the pointer once it starts, and both would act on the same release. The context
  // menu hides the preview, so an arm that outlived it would be a mode with no visible state.
  if (ImGui::IsKeyPressed(ImGuiKey_Escape) || addNodePopup.isOpen() || edgeReconnect.isActive() || contextMenu.isOpen())
  {
    transitArmed = false;
    return;
  }
  if (!ImGui::IsMouseHoveringRect(canvas_min, canvas_max))
  {
    return; // over another panel, so neither the ghost nor the cursor is ours to set
  }
  if (pinJumpMenu.isOpen())
  {
    // It takes Space itself, so the key is dead while it stands. The arm waits rather than ending:
    // the menu is a detour, not an answer to the gesture.
    return;
  }

  // On release, as the design has it: the press lands on the pin ne is already tracking, so taking
  // the gesture on the way up leaves that interaction alone.
  if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left))
  {
    return;
  }
  transitArmed = false;
  // Only a click commits: a library drop, a link drag and a rubber band all end over the canvas too,
  // the last through ne, which reads a release just outside a dot as a drop on the background. The
  // slack follows the dot on screen, so what counts as a click does not narrow as the view zooms in.
  const float invZoom = ne::GetCurrentZoom(); // canvas units per screen pixel
  const float clickSlack = eastl::max(ImGui::GetIO().MouseDragThreshold, invZoom > 0.0f ? pin_hit_radius() / invZoom : 0.0f);
  if (transitTargetNode < 0 || ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left, clickSlack) || linkDrop.overEmptyCanvas)
  {
    return;
  }
  if (transitTargetEdge >= 0)
  {
    openAddNodePopupAtEdge(transitTargetNode, transitTargetPin, transitTargetEdge, ne::ScreenToCanvas(ImGui::GetMousePos()),
      canvas_min, canvas_max, /*splice=*/true);
  }
  else
  {
    openAddNodePopupAtPin(transitTargetNode, transitTargetPin, canvas_min, canvas_max, /*splice=*/true);
  }
}

void GraphTab::drawArmedTransitPreview(const ImVec2 &canvas_min, const ImVec2 &canvas_max)
{
  // A held button means another gesture owns the pointer and the ghost would ride over ne's own
  // preview. Both menus open after the commit half ran, so the arm still stands on the frame one of
  // them does; the canvas rect cannot stand in for them, as it tests the point, not what is on top.
  if (!transitArmed || pinJumpMenu.isOpen() || contextMenu.isOpen() || ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
      !ImGui::IsMouseHoveringRect(canvas_min, canvas_max))
  {
    return;
  }

  ImDrawList *const drawList = ImGui::GetWindowDrawList();
  const ImVec2 mouse = ImGui::GetMousePos();
  ImVec2 boxMin, boxMax, ghostIn, ghostOut;

  // The window list is not the canvas one, so nothing here is clipped for us.
  drawList->PushClipRect(canvas_min, canvas_max, true);
  if (transitTargetNode < 0)
  {
    ImGui::SetMouseCursor(ImGuiMouseCursor_NotAllowed);
    draw_ghost_frame(drawList, mouse, true, true, canvas_min, canvas_max, boxMin, boxMax, ghostIn, ghostOut);
    // The pin centres, not the body: the gap has to clear the dots the wires would leave from.
    draw_canvas_hint(drawList, ImVec2(ghostIn.x, boxMin.y), ImVec2(ghostOut.x, boxMax.y), TRANSIT_ARMED_HINT, canvas_min, canvas_max);
    drawList->PopClipRect();
    return;
  }

  // Anchored where the node will actually land, which is not the same rule for both targets: a pin
  // places it off the pin, an edge above the cursor on it.
  const GraphData::Pin *const anchorPin = find_pin(graphData, transitTargetNode, transitTargetPin);
  const bool anchorIsOut = anchorPin && anchorPin->role == PinRole::Out;
  const ImVec2 mouseCanvas = ne::ScreenToCanvas(mouse);
  const ImVec2 spawnCanvas = transitTargetEdge >= 0 ? spawnPosForDrop(ImVec2(mouseCanvas.x, mouseCanvas.y - ADD_NODE_DROP_UP),
                                                        anchorIsOut, canvas_min, canvas_max)
                                                    : spawnPosAtPin(transitTargetNode, transitTargetPin, canvas_min, canvas_max);
  draw_ghost_frame(drawList, ne::CanvasToScreen(spawnCanvas), true, true, canvas_min, canvas_max, boxMin, boxMax, ghostIn, ghostOut);

  SpliceEnds ends;
  splice_ends(graphData, transitTargetNode, transitTargetPin, transitTargetEdge, ends);
  const float thickness = static_cast<float>(eastl::max(1, hdpi::_pxS(TRANSIT_GHOST_EDGE_WIDTH)));
  const ImVec2 feedScreen = ne::CanvasToScreen(ne::GetPinPosition(ne::PinId(make_pin_id(ends.feed.node, ends.feed.pin))));
  draw_dangling_link(drawList, feedScreen, ghostIn, /*from_is_output=*/true, TRANSIT_GHOST_EDGE_COLOR, thickness);
  // One per consumer: an Out anchor hands every one of them on.
  for (const SplicePin &sink : ends.sinks)
  {
    const ImVec2 sinkScreen = ne::CanvasToScreen(ne::GetPinPosition(ne::PinId(make_pin_id(sink.node, sink.pin))));
    draw_dangling_link(drawList, ghostOut, sinkScreen, /*from_is_output=*/true, TRANSIT_GHOST_EDGE_COLOR, thickness);
  }
  drawList->PopClipRect();
}

void GraphTab::openAddNodePopupAtEdge(int source_node, int source_pin, int edge_id, const ImVec2 &cursor_canvas,
  const ImVec2 &canvas_min, const ImVec2 &canvas_max, bool splice)
{
  // Without splice the hovered edge survives, unless the source pin is single-connect and the spawn
  // replaces it like any other new connection there.
  addNodePopup.openAt(graphData, plugin, source_node, source_pin, ImVec2(cursor_canvas.x, cursor_canvas.y - ADD_NODE_DROP_UP),
    canvas_min, canvas_max, splice, edge_id);
}

void GraphTab::runPinEdgeCommand(GraphPinJumpMenu::Purpose purpose)
{
  int node = -1;
  int pin = -1;
  if (!hovered_pin(node, pin))
  {
    return;
  }

  // Anything but one edge is the chooser's business: it is now up, and reports the pick through draw().
  GraphPinJumpMenu::Choice choice;
  if (pinJumpMenu.resolve(graphData, purpose, node, pin, doc.getStructureRevision(), choice) != GraphPinJumpMenu::Pick::Single)
  {
    return;
  }
  if (purpose == GraphPinJumpMenu::Purpose::Jump)
  {
    scroll_pin_to_screen(choice.node, choice.pin, ImGui::GetMousePos());
  }
  else
  {
    beginEdgeReroute(choice.edgeId, node, pin);
  }
}

void GraphTab::applyPendingReroute()
{
  if (pendingRerouteEdge < 0)
  {
    return;
  }
  // The gesture ends on a left release, so it cannot begin while the click that chose the row is
  // still going: that click's own release would end it before the user aimed anywhere.
  if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left))
  {
    return;
  }
  const int edgeId = pendingRerouteEdge;
  pendingRerouteEdge = -1;
  beginEdgeReroute(edgeId, pendingRerouteNode, pendingReroutePin);
}

void GraphTab::beginEdgeReroute(int edge_id, int detach_node, int detach_pin)
{
  if (edge_id < 0 || !edgeReconnect.beginForEdge(graphData, edge_id, detach_node, detach_pin))
  {
    return;
  }
  // Snapshot before dropping it so the reconnect resolves as one undo step (recordReconnectEdge).
  if (const GraphData::Edge *const edge = find_edge_by_id(graphData, edge_id))
  {
    reconnectRemovedEdge = *edge;
    reconnectRemovedEdgeIndex = static_cast<int>(edge - graphData.edges.begin());
  }
  removeEdgeById(edge_id);
}

void GraphTab::duplicateSelection()
{
  CanvasClipboard scratch;
  scratch.captureSelection(*this, graphData, /*with_input_edges=*/true);
  if (scratch.empty())
  {
    return;
  }

  eastl::vector<GraphData::Node> nodes;
  eastl::vector<GraphData::Edge> edges;
  scratch.pasteShifted(*this, graphData, ImVec2(DUPLICATE_OFFSET, DUPLICATE_OFFSET), nodes, edges);
  doc.recordPaste(eastl::move(nodes), eastl::move(edges), "Duplicate");
}

void GraphTab::selectAllNodes()
{
  // Nodes only: deleting one cascade-removes its edges anyway, and a selection full of links would
  // make every edge-aware action hit the whole graph.
  ne::ClearSelection();
  for (const GraphData::Node &n : graphData.nodes)
  {
    ne::SelectNode(ne::NodeId(make_node_id(n.id)), /*append=*/true);
  }
}

void GraphTab::invertNodeSelection()
{
  GraphSelection cur;
  readSelection(cur);
  ne::ClearSelection();
  for (const GraphData::Node &n : graphData.nodes)
  {
    // readSelection sorts, so the membership test stays cheap on a big graph.
    if (!eastl::binary_search(cur.nodes.begin(), cur.nodes.end(), n.id))
    {
      ne::SelectNode(ne::NodeId(make_node_id(n.id)), /*append=*/true);
    }
  }
}

void GraphTab::applySelectionRequests()
{
  if (selectAllRequested)
  {
    selectAllRequested = false;
    selectAllNodes();
  }
  if (deselectAllRequested)
  {
    deselectAllRequested = false;
    ne::ClearSelection();
  }
  if (invertSelectionRequested)
  {
    invertSelectionRequested = false;
    invertNodeSelection();
  }
}

void GraphTab::updateContextMenu()
{
  // Suspended, so the mouse is back in screen space -- which is what the popup anchors to. Only
  // stored when a menu opens: this runs on every frame one stands, and a Paste has to land on the
  // right-click, not on the row the cursor travelled to.
  const ImVec2 clickCanvasPos = ne::ScreenToCanvas(ImGui::GetMousePos());

  ne::NodeId ctxNode;
  ne::PinId ctxPin;
  ne::LinkId ctxLink;
  if (ne::ShowNodeContextMenu(&ctxNode))
  {
    // Right-clicking outside the selection selects that node, as a left click would; inside it, the
    // selection stands so the menu covers the whole group.
    if (!ne::IsNodeSelected(ctxNode))
    {
      ne::ClearSelection();
      ne::SelectNode(ctxNode, /*append=*/false);
    }
    GraphSelection selection;
    readSelection(selection);
    contextMenuCanvasPos = clickCanvasPos;
    const int nodeId = decode_node_id(ctxNode.Get());
    const GraphData::Node *const node = find_node_by_id(graphData, nodeId);
    const bool isInstance = node && is_subgraph_instance(*node);
    contextMenu.openForNode(nodeId, static_cast<int>(selection.nodes.size()), !canvasClipboard.empty(), isInstance,
      isInstance && plugin.getGraphPanel().isSubgraphOpen(node->descName.c_str()));
  }
  else if (ne::ShowPinContextMenu(&ctxPin))
  {
    int node = -1;
    int pin = -1;
    decode_pin_id(ctxPin.Get(), node, pin);
    contextMenuCanvasPos = clickCanvasPos;
    contextMenu.openForPin(graphData, node, pin);
  }
  else if (ne::ShowLinkContextMenu(&ctxLink))
  {
    if (!ne::IsLinkSelected(ctxLink))
    {
      ne::ClearSelection();
      ne::SelectLink(ctxLink, /*append=*/false);
    }
    GraphSelection selection;
    readSelection(selection);
    eastl::vector<int> edges = eastl::move(selection.links);
    if (edges.empty())
    {
      edges.push_back(decode_link_id(ctxLink.Get()));
    }
    contextMenuCanvasPos = clickCanvasPos;
    contextMenu.openForEdges(graphData, edges);
  }
  else if (ne::ShowBackgroundContextMenu())
  {
    contextMenuCanvasPos = clickCanvasPos;
    contextMenu.openForBackground(texGen() && texGen()->getAutoupdate());
  }

  contextMenu.updateImgui();
}

void GraphTab::applyPendingMenuAction(const ImVec2 &canvas_min, const ImVec2 &canvas_max)
{
  const GraphContextMenu::Result act = contextMenu.takePicked();

  switch (act.action)
  {
    case GraphContextMenu::Action::None: break;

    // Both targets carry the source pin in targetNode/targetPin and differ only in the drop rule,
    // but they need their own actions: a pin menu fills act.edges too, so that cannot tell them
    // apart. The background copy of the row still draws disabled.
    case GraphContextMenu::Action::AddNode:
      openAddNodePopupAtPin(act.targetNode, act.targetPin, canvas_min, canvas_max, /*splice=*/false);
      break;

    case GraphContextMenu::Action::AddNodeAtEdge:
      // No edge id: this row fans the new node out of the source pin rather than entering a wire.
      openAddNodePopupAtEdge(act.targetNode, act.targetPin, /*edge_id=*/-1, contextMenuCanvasPos, canvas_min, canvas_max,
        /*splice=*/false);
      break;

    case GraphContextMenu::Action::AddTransitNode:
      openAddNodePopupAtPin(act.targetNode, act.targetPin, canvas_min, canvas_max, /*splice=*/true);
      break;

    case GraphContextMenu::Action::AddTransitNodeAtEdge:
      // The edge id keeps the splice to the wire the menu was opened on; its source may feed several.
      openAddNodePopupAtEdge(act.targetNode, act.targetPin, act.edges.empty() ? -1 : act.edges[0], contextMenuCanvasPos, canvas_min,
        canvas_max, /*splice=*/true);
      break;

    case GraphContextMenu::Action::OpenSubgraph:
      if (const GraphData::Node *const node = find_node_by_id(graphData, act.targetNode))
      {
        pendingOpenSubgraphStem = node->descName;
      }
      break;
    case GraphContextMenu::Action::CreateSubgraph: requestCreateSubgraph(); break;
    case GraphContextMenu::Action::ExplodeSubgraph: requestExplode(act.targetNode); break;

    case GraphContextMenu::Action::Remove: queue_selected_for_delete(); break;
    case GraphContextMenu::Action::RemoveKeepConnections: removeSelectedKeepingConnections(); break;
    case GraphContextMenu::Action::Duplicate: duplicateSelection(); break;
    case GraphContextMenu::Action::Copy: canvasClipboard.captureSelection(*this, graphData, /*with_input_edges=*/false); break;

    case GraphContextMenu::Action::Cut:
      canvasClipboard.captureSelection(*this, graphData, /*with_input_edges=*/false);
      queue_selected_for_delete();
      break;

    case GraphContextMenu::Action::Paste:
    {
      eastl::vector<GraphData::Node> pastedNodes;
      eastl::vector<GraphData::Edge> pastedEdges;
      canvasClipboard.paste(*this, graphData, contextMenuCanvasPos, pastedNodes, pastedEdges);
      doc.recordPaste(eastl::move(pastedNodes), eastl::move(pastedEdges));
      break;
    }

    case GraphContextMenu::Action::ToggleAutoUpdate:
      if (texGen())
      {
        texGen()->toggleAutoupdate();
      }
      break;

    case GraphContextMenu::Action::ForceRebuild:
      if (texGen())
      {
        texGen()->requestForceRebuild();
      }
      break;

    case GraphContextMenu::Action::SelectAll: selectAllNodes(); break;
    case GraphContextMenu::Action::RemoveEdges: doc.deleteEdges(act.edges); break;
    case GraphContextMenu::Action::MuteEdges: doc.setEdgesMuted(act.edges, /*muted=*/true); break;
    case GraphContextMenu::Action::UnmuteEdges: doc.setEdgesMuted(act.edges, /*muted=*/false); break;

    case GraphContextMenu::Action::RerouteEdge:
      if (!act.edges.empty())
      {
        beginEdgeReroute(act.edges[0], act.detachNode, act.detachPin);
      }
      break;

    case GraphContextMenu::Action::JumpToOppositePin:
      // A graph write while the menu stood can have retired the destination.
      if (is_pin_reachable(graphData, act.jumpNode, act.jumpPin))
      {
        scroll_pin_to_screen(act.jumpNode, act.jumpPin, ImGui::GetMousePos());
      }
      break;
  }
}

void GraphTab::updateCanvas(const ImVec2 &canvas_min, const ImVec2 &canvas_max)
{
  ne::SetCurrentEditor(editor);

  // Framing requests are captured here but the actual ne::NavigateTo* runs at end-of-frame (after
  // the node loop). ne's GetContentBounds / GetSelectionBounds only union nodes drawn (live) this
  // frame (internal.h GetBounds filters by m_IsLive), and off-screen culling skips the rest -- so
  // framing on load (view not yet on the graph) or framing an off-screen target would measure empty
  // bounds and no-op. Deferring + forcing a full render this frame (forceAllVisible) fixes that.
  const bool fitSelectionReq = frameSelectedRequested;
  frameSelectedRequested = false;
  bool fitContentReq = false;
  bool showNextReq = false;

  // Before the shortcut dispatches, so a menu-driven delete reaches this frame's BeginDelete queue.
  applyPendingMenuAction(canvas_min, canvas_max);
  applyPendingReroute();

  applySelectionRequests();

  // No canvas command may fire while the add-node field owns the keyboard: several of them are bare
  // letters (A modifies an edge, C comments a pin) and the rest include Backspace and Delete.
  // A reroute owns the pin until it resolves: steal focus first and its release handler reads no pin
  // and files the edge as a deletion. A held button means another gesture owns the pin too.
  const bool pinCommandsLive = !edgeReconnect.isActive() && !ImGui::IsMouseDown(ImGuiMouseButton_Left);
  // A reroute's undo entry may name a node a removal takes. The click on a menu row resolves it first, a key does not.
  const bool removalsLive = !edgeReconnect.isActive();

  if (!addNodePopup.isOpen())
  {
    fitContentReq = shortcut_fired(CANVAS_ZOOM_AND_CENTER);
    showNextReq = shortcut_fired(CANVAS_SHOW_NEXT_SELECTED);

    if (shortcut_fired(CANVAS_COPY))
    {
      canvasClipboard.captureSelection(*this, graphData, /*with_input_edges=*/false);
    }
    if (shortcut_fired(CANVAS_CUT) && removalsLive)
    {
      canvasClipboard.captureSelection(*this, graphData, /*with_input_edges=*/false);
      queue_selected_for_delete();
    }
    if (shortcut_fired(CANVAS_PASTE))
    {
      const ImVec2 mouseCanvas = ne::ScreenToCanvas(ImGui::GetMousePos());
      eastl::vector<GraphData::Node> pastedNodes;
      eastl::vector<GraphData::Edge> pastedEdges;
      canvasClipboard.paste(*this, graphData, mouseCanvas, pastedNodes, pastedEdges);
      doc.recordPaste(eastl::move(pastedNodes), eastl::move(pastedEdges));
    }
    if (shortcut_fired(CANVAS_DELETE_SELECTED) && removalsLive)
    {
      queue_selected_for_delete();
    }
    if (shortcut_fired(CANVAS_DUPLICATE))
    {
      duplicateSelection();
    }
    if (shortcut_fired(CANVAS_EXPLODE_SUBGRAPH))
    {
      requestExplodeSelected();
    }
    if (shortcut_fired(CANVAS_CREATE_SUBGRAPH))
    {
      requestCreateSubgraph();
    }
    if (shortcut_fired(CANVAS_TOGGLE_AUTOUPDATE) && texGen())
    {
      texGen()->toggleAutoupdate();
    }
    if (shortcut_fired(CANVAS_SELECT_NODES_NO_OUTPUTS))
    {
      select_nodes_with_no_connected_outputs(graphData);
    }
    if (shortcut_fired(CANVAS_REMOVE_KEEP_CONNECTIONS) && removalsLive)
    {
      removeSelectedKeepingConnections();
    }
    if (shortcut_fired(CANVAS_REMOVE_EDGES_AT_PIN) && pinCommandsLive)
    {
      removeEdgesUnderCursor();
    }
    if (shortcut_fired(CANVAS_MODIFY_EDGE_AT_PIN) && pinCommandsLive && !pinJumpMenu.isOpen())
    {
      runPinEdgeCommand(GraphPinJumpMenu::Purpose::Reroute);
    }

    // One Tab press fires all three; the first branch that applies takes it, as the hotkeys bar shows.
    const bool jumpFired = shortcut_fired(CANVAS_JUMP_OPPOSITE_PIN);
    const bool openFired = shortcut_fired(CANVAS_OPEN_SUBGRAPH);
    const bool parentFired = shortcut_fired(CANVAS_GO_TO_PARENT_GRAPH);
    if ((jumpFired || openFired || parentFired) && pinCommandsLive && !pinJumpMenu.isOpen())
    {
      int hoverNode = -1;
      int hoverPin = -1;
      hovered_pin(hoverNode, hoverPin);
      GraphSelection sel;
      readSelection(sel);
      const SubgraphContext ctx = resolveSubgraphContext(sel);
      if (jumpFired && has_reachable_destination(graphData, hoverNode, hoverPin))
      {
        runPinEdgeCommand(GraphPinJumpMenu::Purpose::Jump);
      }
      else if (openFired && ctx.selectedSubgraph)
      {
        pendingOpenSubgraphStem = ctx.selectedSubgraph->descName;
      }
      else if (parentFired && hostWindow)
      {
        plugin.getGraphPanel().focusParentSurface(*this);
      }
    }
    // Space puts the node alongside what the pin feeds, Shift+Space puts it in between. Same targets,
    // so one dispatch; both are polled first because shortcut_fired consumes the route.
    const bool addNodeFired = shortcut_fired(CANVAS_ADD_NODE_AT_PIN);
    const bool addTransitFired = shortcut_fired(CANVAS_ADD_TRANSIT_NODE);
    // The jump menu confirms on Space with KeyOwner_Any, so one press would otherwise do both.
    if ((addNodeFired || addTransitFired) && pinCommandsLive && !pinJumpMenu.isOpen())
    {
      const bool splice = addTransitFired;
      int node = -1;
      int pin = -1;
      int edgeId = -1;
      const bool overPin = hovered_pin(node, pin);
      if (!overPin)
      {
        edgeId = hovered_edge_source_pin(graphData, node, pin);
      }
      // A splice needs a wire to take over, the one rule the hint bar and the menu rows draw too.
      const bool target = (overPin || edgeId >= 0) && (!splice || has_splice_target(graphData, node, pin, edgeId));
      if (splice && transitArmed)
      {
        transitArmed = false; // pressed again, which is the way out the hint bar keeps offering
      }
      else if (target && overPin)
      {
        openAddNodePopupAtPin(node, pin, canvas_min, canvas_max, splice);
      }
      else if (target)
      {
        // The edge id scopes a splice to one wire; a plain add fans out of the source pin and names none.
        openAddNodePopupAtEdge(node, pin, splice ? edgeId : -1, ne::ScreenToCanvas(ImGui::GetMousePos()), canvas_min, canvas_max,
          splice);
      }
      else if (splice)
      {
        // Nothing here to splice into, so the gesture waits for a target instead of dying.
        transitArmed = true;
      }
    }
    if (shortcut_fired(CANVAS_COMMENT_PIN) && pinCommandsLive)
    {
      // Record the pin under the cursor; the modal edit dialog runs later, in actObjects.
      int node = -1;
      int pin = -1;
      if (hovered_pin(node, pin))
      {
        pendingCommentNodeId = node;
        pendingCommentPinIndex = pin;
      }
    }
  }

  if (const ne::LinkId dblLink = ne::GetDoubleClickedLink())
  {
    doc.toggleEdgeMuted(decode_link_id(dblLink.Get()));
  }

  refreshDerivedState();

  resolveBaseNodeDropTarget();
  resolveArmedTransitTarget();
  // Before ne::Begin for two reasons: the aim the ghost block reads is captured inside the canvas
  // pass, so a picker opened after it would leave the ghost on the previous drop point; and the edge
  // variant needs the mouse in screen space, which the canvas rewrites once it is entered.
  updateArmedTransit(canvas_min, canvas_max);

  // ne::End draws the canvas frame straight from ImGuiCol_Border / BorderShadow, and anything the
  // canvas raises (tooltips, the menus below) inherits the popup colours. Lock them for the whole
  // canvas pass rather than patching the vendored editor.
  ImGui::PushStyleColor(ImGuiCol_Border, GRAPH_POPUP_BORDER_COLOR);
  ImGui::PushStyleColor(ImGuiCol_BorderShadow, IM_COL32_BLACK_TRANS);
  ImGui::PushStyleColor(ImGuiCol_PopupBg, GRAPH_POPUP_BG_COLOR);
  ImGui::PushStyleColor(ImGuiCol_Text, GRAPH_TEXT_COLOR);

  ImGui::SetCursorScreenPos(canvas_min);
  ne::Begin("perf_graph", ImVec2(canvas_max.x - canvas_min.x, canvas_max.y - canvas_min.y));

  lastCanvasCenter = ne::ScreenToCanvas(ImVec2((canvas_min.x + canvas_max.x) * 0.5f, (canvas_min.y + canvas_max.y) * 0.5f));

  // Suspend culling while a framing operation is in flight (load fit over navigationFramesLeft, or a
  // deferred fit / show-next this frame): the fit measures only nodes drawn live this frame, so every
  // node must render. A few full-render frames during framing is a negligible one-shot cost.
  const bool forceAllVisible = navigationFramesLeft > 0 || fitContentReq || fitSelectionReq || showNextReq;

  canvasCull.update(graphData, canvas_min, canvas_max, forceAllVisible, pendingPositionIds);

  dragCollision.update(graphData, canvasCull);

  const float invZoom = ne::GetCurrentZoom();
  const bool zoomLod = invZoom > 0.0f && (1.0f / invZoom) <= LOD_SCALE;

  GraphSelection drawnSelection;
  readSelection(drawnSelection);

  // Drop target first: both resolve the same way, and a library drag ends in the drop that acts.
  const uint64_t highlightPinId = dropTargetPinNode >= 0   ? make_pin_id(dropTargetPinNode, dropTargetPinIndex)
                                  : transitTargetNode >= 0 ? make_pin_id(transitTargetNode, transitTargetPin)
                                                           : 0;

  eastl::string hoverTooltip;
  GraphNodeFrame nodeFrame(graphData, linkedPins, livePins, resolutions, pinJumpMenu, edgeReconnect, hoverTooltip,
    ne::GetHoveredNode().Get(), previewNodeId, highlightPinId);

  const int nodeCount = static_cast<int>(graphData.nodes.size());
  for (int ni = 0; ni < nodeCount; ++ni)
  {
    const GraphData::Node &n = graphData.nodes[ni];

    if (!canvasCull.needed(ni))
    {
      continue; // off-screen and not attached to any on-screen link -> skip entirely
    }

    const bool selected = eastl::binary_search(drawnSelection.nodes.begin(), drawnSelection.nodes.end(), n.id);

    if (n.descName == "comment")
    {
      draw_comment_node(nodeFrame, n, selected);
      continue;
    }
    if (n.descName == "block")
    {
      draw_block_node(nodeFrame, n, selected, appliedBlockGroupSizes);
      continue;
    }

    draw_graph_node(nodeFrame, n, ni, selected, /*reduced=*/!canvasCull.visible(ni) || zoomLod);
  }
  if (nodeFrame.clickedSubgraphNodeId >= 0)
  {
    if (const GraphData::Node *const node = find_node_by_id(graphData, nodeFrame.clickedSubgraphNodeId))
    {
      pendingOpenSubgraphStem = node->descName;
    }
  }

  // Kept past the scope below for the reconnect preview, which draws at the same weight.
  float edgeThickness = 0.0f;
  {
    const GraphEdgeFrame edgeFrame(graphData, canvasCull, deadPaths, drawnSelection, pinJumpMenu);
    edgeThickness = edgeFrame.thickness;
    draw_graph_edges(edgeFrame);
    handle_link_create(edgeFrame, doc, edgeReconnect, linkDrop);
  }

  if (linkDrop.spawnRequested)
  {
    addNodePopup.openAt(graphData, plugin, linkDrop.sourceNode, linkDrop.sourcePin, linkDrop.dropCanvasPos, canvas_min, canvas_max,
      /*splice_into_connections=*/false, /*anchor_edge=*/-1);
    // Only this flag: handle_link_create clears overEmptyCanvas and createInFlight every frame, and
    // the aim fields are read under the first of those, which it sets only where it writes them.
    linkDrop.spawnRequested = false;
  }

  // Link / node deletion. Both propagate into graphData; node deletes cascade-remove the
  // edges that referenced them (see removeNodeById) before erasing the node itself. Items
  // reach the BeginDelete queue via the CANVAS_DELETE_SELECTED shortcut at the top of
  // updateCanvas (which calls ne::DeleteNode / ne::DeleteLink); the library's own Del-key
  // handler is suppressed via apply_editor_defaults so this is the only path.
  if (ne::BeginDelete())
  {
    handle_deleted_links(doc);

    ne::NodeId deletedNodeId;
    eastl::vector<int> liveIds;
    while (ne::QueryDeletedNode(&deletedNodeId))
    {
      const int node_id = decode_node_id(deletedNodeId.Get());
      const GraphData::Node *const node = find_node_by_id(graphData, node_id);
      if (!node)
      {
        ne::AcceptDeletedItem();
        continue;
      }

      // Reject and queue every node deletion -- the actual removal happens in actObjects so
      // (a) the prompt for blocks-with-implicit-children can fire outside the ImGui frame,
      // and (b) one batch can resolve a mixed selection atomically: Cancel rolls back the
      // whole multi-selection, not just the block whose prompt was visible.
      ne::RejectDeletedItem();
      PendingNodeDelete pending;
      pending.nodeId = node_id;
      liveIds.push_back(node_id);
      if (node->descName == "block")
      {
        // childIds must be captured now -- ne::GetNodePosition / GetNodeSize is only valid
        // while we're inside the SetCurrentEditor scope of the ne::Begin/End that wraps us.
        collectNodesInsideBlock(node_id, pending.childIds);
        liveIds.insert(liveIds.end(), pending.childIds.begin(), pending.childIds.end());
      }
      pendingNodeDeletes.push_back(eastl::move(pending));
    }

    // The delete runs in actObjects, out of frame, where ne positions cannot be read; undo restores
    // what it snapshots.
    foldLivePositions(liveIds);
  }
  ne::EndDelete();

  // Deferred framing (requests captured at the top of updateCanvas). Runs here, after the node loop,
  // so the nodes the fit measures were drawn live this frame -- forceAllVisible guaranteed a full
  // render on any frame one of these is set.
  if (showNextReq)
  {
    showNextSelectedNode();
  }
  if (fitSelectionReq)
  {
    ne::NavigateToSelection(/*zoomIn=*/true);
  }
  if (fitContentReq)
  {
    ne::NavigateToContent();
  }

  // Re-fit for several frames after load. One-shot doesn't survive: the canvas widget can resize
  // during initial layout, and on resize the editor overwrites our pending nav with the previous
  // view rect (imgui_node_editor.cpp:1212). Retrying a few frames lets the fit settle.
  if (navigationFramesLeft > 0)
  {
    ne::NavigateToContent(0.0f);
    --navigationFramesLeft;
  }

  // "Modify edge" (A) rubber-band preview. Drawn here, INSIDE ne::Begin/End, so the anchor pin
  // centre (captured from GetCursorScreenPos during the pin pass) and ImGui::GetMousePos() are both
  // in the canvas's local space and the canvas transform maps them to the screen together. Drawing
  // it after ne::End would mix local anchor coords with screen-space mouse coords, sending the
  // anchor end off-screen under any pan / zoom.
  if (edgeReconnect.isActive())
  {
    const PinType anchorType = pin_type_at(graphData, canvasCull.indexOf(edgeReconnect.anchorNode()), edgeReconnect.anchorPin());
    edgeReconnect.drawPreview(ImGui::GetWindowDrawList(), ImGui::GetMousePos(), pin_color_for_type(anchorType), edgeThickness);
  }

  // Where the pending node is aimed: the live drag owns it until the popup takes over. Resolved once
  // so the preview and the ghost cannot read different halves. Whether it is showing stays a live test
  // at each site, because the popup can close later this frame.
  const bool aimFromPopup = addNodePopup.isOpen();
  const int aimNode = aimFromPopup ? addNodePopup.sourceNodeId() : linkDrop.sourceNode;
  const int aimPin = aimFromPopup ? addNodePopup.sourcePinIndex() : linkDrop.sourcePin;
  const bool aimIsOutput = aimFromPopup ? addNodePopup.sourceIsOutput() : linkDrop.sourceIsOutput;
  const ImVec2 aimDropCanvas = aimFromPopup ? addNodePopup.dropCanvasPos() : linkDrop.dropCanvasPos;

  // Canvas-local for the reason the reconnect preview above gives. It has to take over the moment the
  // button comes up: ne stops drawing its white link then, and the popup opens a frame later.
  if (aimFromPopup || (linkDrop.overEmptyCanvas && !ImGui::IsMouseDown(ImGuiMouseButton_Left)))
  {
    const ImVec2 pinCanvas = ne::GetPinPosition(ne::PinId(make_pin_id(aimNode, aimPin)));
    draw_dangling_link(ImGui::GetWindowDrawList(), pinCanvas, aimDropCanvas, aimIsOutput, GRAPH_PENDING_LINK_COLOR, edgeThickness);
  }

  if (!hoverTooltip.empty())
  {
    ne::Suspend();
    ImGui::SetTooltip("%s", hoverTooltip.c_str());
    ne::Resume();
  }

  if (pinJumpMenu.isOpen())
  {
    // Re-read every frame: a navigate animation or a canvas resize moves the pin while the menu stands
    // (the sibling overlay does the same, see edgeReconnect.setAnchorScreenPos).
    const int menuNode = pinJumpMenu.sourceNodeId();
    const int menuPin = pinJumpMenu.sourcePinIndex();
    const GraphPinJumpMenu::Purpose menuPurpose = pinJumpMenu.openedFor();
    const ImVec2 sourcePinScreen = ne::CanvasToScreen(ne::GetPinPosition(ne::PinId(make_pin_id(menuNode, menuPin))));
    GraphPinJumpMenu::Choice choice;
    // Everything here stays inside the suspend: the canvas rewrites io.MousePos into its own local
    // space and only restores screen coordinates while suspended, and the jump reads the cursor.
    ne::Suspend();
    const bool picked = pinJumpMenu.draw(graphData, doc.getStructureRevision(), canvas_min, canvas_max, sourcePinScreen, choice);
    if (picked && menuPurpose == GraphPinJumpMenu::Purpose::Jump)
    {
      // A cursor off the canvas would pan the destination out of sight, so land it on the source pin
      // instead and let the view slide along the edge.
      const ImVec2 mouse = ImGui::GetMousePos();
      const bool cursorOnCanvas =
        mouse.x >= canvas_min.x && mouse.x < canvas_max.x && mouse.y >= canvas_min.y && mouse.y < canvas_max.y;
      scroll_pin_to_screen(choice.node, choice.pin, cursorOnCanvas ? mouse : sourcePinScreen);
    }
    else if (picked)
    {
      // Queued, not started here: applyPendingReroute waits for the click that picked the row.
      pendingRerouteEdge = choice.edgeId;
      pendingRerouteNode = menuNode;
      pendingReroutePin = menuPin;
    }
    ne::Resume();
  }

  if (addNodePopup.isOpen())
  {
    const int srcNode = addNodePopup.sourceNodeId();
    const int srcPin = addNodePopup.sourcePinIndex();
    const ImVec2 dropCanvas = addNodePopup.dropCanvasPos();
    const bool splice = addNodePopup.splicesIntoConnections();
    const int anchorEdge = addNodePopup.anchorEdgeId();
    eastl::string pickedUid;
    ne::Suspend();
    const bool picked = addNodePopup.draw(graphData, canvas_min, canvas_max, pickedUid);
    ne::Resume();
    if (picked)
    {
      // openAt clamped dropCanvas already, so this is the offset half only.
      const ImVec2 spawnPos = spawn_pos_for_clamped_drop(dropCanvas, addNodePopup.sourceIsOutput());
      // The spawn goes to the focused surface, and the right-click that opened this popup did not move the focus.
      plugin.getGraphPanel().focusSurface(getRootTab(), getHostWindow());
      const int newId = plugin.spawnBaseNodeWired(pickedUid.c_str(), spawnPos.x, spawnPos.y, srcNode, srcPin, splice, anchorEdge);
      if (newId >= 0)
      {
        // ne has never seen this id and SelectNode no-ops on an unknown one, so this is what makes
        // the node exist; the cull's own push next frame then finds the bounds already right.
        ne::SetNodePosition(ne::NodeId(make_node_id(newId)), spawnPos);
        GraphSelection justAdded;
        justAdded.nodes.push_back(newId);
        setPendingSelection(justAdded);
      }
    }
  }

  // After the node and link passes: the right-click target is the editor's hot object, which those
  // passes establish.
  ne::Suspend();
  updateContextMenu();
  ne::Resume();

  // ne::End frames the canvas from ImGuiCol_Border, pushed above for the whole pass; a window's canvas
  // has no frame in the design.
  if (hostWindow)
  {
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_BLACK_TRANS);
  }
  ne::End();
  if (hostWindow)
  {
    ImGui::PopStyleColor();
  }

  ImGui::PopStyleColor(4);

  // Pull post-frame block sizes back into GraphData. ne's built-in SizeAction handles the
  // resize-by-border interaction for group nodes; we just need to mirror the new size into
  // our persisted fields so save/reload round-trips it.
  syncBlockSizes();

  // On release, fold a finished drag into one undo entry: nodes whose position changed (ne owns the
  // live position and never writes it back) and blocks whose size changed (captured by syncBlockSizes).
  // A corner resize changes both for the same block, so one entry lets a single Ctrl+Z restore position
  // and size together. Must run while ne is current.
  // constraint: ne truncates the positions it stores, so a move is measured from its press positions, never graphData.
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
  {
    pressPositions.clear();
    if (cursor_on_canvas_window())
    {
      pressPositions.reserve(graphData.nodes.size());
      for (const GraphData::Node &n : graphData.nodes)
      {
        // A node still awaiting its first SetNodePosition has no meaningful live position yet.
        if (pendingPositionIds.find(n.id) == pendingPositionIds.end())
        {
          pressPositions[n.id] = ne::GetNodePosition(ne::NodeId(make_node_id(n.id)));
        }
      }
    }
  }
  // constraint: ne starts a node drag at 1 px, below ImGui's default drag threshold.
  const bool dragged = !pressPositions.empty() && ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left, 1.0f);
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && (dragged || !blockResizeOld.empty()))
  {
    eastl::vector<NodePos> oldPositions;
    eastl::vector<NodePos> newPositions;
    for (const GraphData::Node &n : graphData.nodes)
    {
      const auto pressed = pressPositions.find(n.id);
      if (pressed == pressPositions.end() || pendingPositionIds.find(n.id) != pendingPositionIds.end())
      {
        continue;
      }
      const ImVec2 live = ne::GetNodePosition(ne::NodeId(make_node_id(n.id)));
      const ImVec2 &atPress = pressed->second;
      const bool moved = ImFabs(live.x - atPress.x) > NODE_MOVE_EPSILON || ImFabs(live.y - atPress.y) > NODE_MOVE_EPSILON;
      // An undo with the button held moves a node to where graphData has it, truncated as ne stores it.
      const bool changed = ImFabs(live.x - IM_TRUNC(n.x)) > NODE_MOVE_EPSILON || ImFabs(live.y - IM_TRUNC(n.y)) > NODE_MOVE_EPSILON;
      if (moved && changed)
      {
        oldPositions.push_back(NodePos{n.id, n.x, n.y});
        newPositions.push_back(NodePos{n.id, live.x, live.y});
      }
    }
    pressPositions.clear();

    // blockResizeOld holds each block's pre-drag size (captured by syncBlockSizes); pair it with the
    // now-committed graphData size, skipping any that netted back to the original.
    eastl::vector<BlockSize> oldSizes;
    eastl::vector<BlockSize> newSizes;
    for (const BlockSize &before : blockResizeOld)
    {
      const GraphData::Node *const n = find_node_by_id(graphData, before.nodeId);
      if (n && (n->blockWidth != before.width || n->blockHeight != before.height))
      {
        oldSizes.push_back(before);
        newSizes.push_back(BlockSize{n->id, n->blockWidth, n->blockHeight});
      }
    }
    blockResizeOld.clear();

    if (!newPositions.empty() || !newSizes.empty())
    {
      doc.commitNodeTransforms(eastl::move(oldPositions), eastl::move(newPositions), eastl::move(oldSizes), eastl::move(newSizes));
    }
  }

  // Drop target for drag-drop from NodeLibraryPanel. Must be issued while the editor is still
  // current so ne::ScreenToCanvas works on the stored mouse pos. AcceptBeforeDelivery is what lets
  // the target answer the overlay with the button still down.
  {
    const ImGuiID dropId = ImGui::GetID("graph_drop_target");
    const ImRect dropRect(canvas_min, canvas_max);
    if (cursor_on_canvas_window() && ImGui::BeginDragDropTargetCustom(dropRect, dropId))
    {
      const ImGuiDragDropFlags acceptFlags = ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect;
      if (const ImGuiPayload *p = ImGui::AcceptDragDropPayload(BASE_NODE_DRAG_PAYLOAD, acceptFlags))
      {
        if (p->IsDelivery())
        {
          BaseNodeDragUid templateUid;
          read_base_node_drag_payload(*p, templateUid);
          // Spawns go to the active surface.
          plugin.getGraphPanel().focusSurface(getRootTab(), getHostWindow());
          if (dropTargetPinNode >= 0)
          {
            const ImVec2 spawnPos = spawnPosAtPin(dropTargetPinNode, dropTargetPinIndex, canvas_min, canvas_max);
            // The same admission test the picker filters with: a drag carries whatever the user
            // grabbed, so a node that cannot pass the wire on is wired alongside instead.
            const int newId = plugin.spawnBaseNodeWired(templateUid, spawnPos.x, spawnPos.y, dropTargetPinNode, dropTargetPinIndex,
              canSpliceDrop(templateUid, dropTargetPinNode, dropTargetPinIndex), /*anchor_edge=*/-1);
            if (newId >= 0)
            {
              // The pending selection below runs this same frame, and SelectNode no-ops on an id ne
              // has never seen; this is what makes the node exist by then.
              ne::SetNodePosition(ne::NodeId(make_node_id(newId)), spawnPos);
              GraphSelection justAdded;
              justAdded.nodes.push_back(newId);
              setPendingSelection(justAdded);
            }
          }
          else
          {
            const ImVec2 canvasPos = ne::ScreenToCanvas(ImGui::GetMousePos());
            plugin.spawnBaseNode(templateUid, canvasPos.x, canvasPos.y);
          }
        }

        // IsPreview, not plain acceptance: ImGui delivers only to a target that accepted on the
        // previous frame as well, so on the first hovered frame a release would still create nothing.
        if (p->IsPreview())
        {
          dropTargetHotFrame = ImGui::GetFrameCount();
        }
      }
      ImGui::EndDragDropTarget();
    }
  }

  // Apply a selection that an undo/redo queued (UndoSelectNodes). Done here, after the render pass, so
  // any nodes a sibling entry just re-added already exist in ne and can be selected. Marked suppressed
  // so the detector below resyncs rather than recording it as a fresh change.
  if (hasPendingSelection)
  {
    ne::ClearSelection();
    for (int id : pendingSelection.nodes)
    {
      ne::SelectNode(ne::NodeId(make_node_id(id)), /*append=*/true);
    }
    for (int id : pendingSelection.links)
    {
      ne::SelectLink(ne::LinkId(make_link_id(id)), /*append=*/true);
    }
    hasPendingSelection = false;
    suppressSelectionRecord = true;
  }

  // Selection extraction. Done unconditionally (not just when texGenService is present) so
  // PropertiesPanel and other observers can read getSelectedNodeId() each frame. ne returns
  // a count via the second arg even though we only request one slot; we treat "exactly one"
  // as the selection signal -- multi-select and empty both map to -1.
  ne::NodeId selectedId;
  const int selCount = ne::GetSelectedNodes(&selectedId, 1);
  selectedNodeId = (selCount == 1) ? (static_cast<int>(selectedId.Get()) - 1) : -1;
  // Total selection size (nodes + links) for the status bar's "Selected:" counter; must be
  // read here, while the editor is still current.
  lastSelectedObjectCount = ne::GetSelectedObjectCount();

  // Selection-undo: a deliberate selection change (click, box-select, the select / show commands)
  // becomes its own "Select nodes" entry. A change folded into an edit (delete / paste / splice) or
  // applied by an undo sets suppressSelectionRecord instead -- it rides that entry. Recording is
  // coalesced to mouse-up so a rubber-band drag is one entry, not one per frame.
  GraphSelection curSelection;
  readSelection(curSelection);
  // A link gesture deselects on its way and resolves a frame after release, so hold what was recorded
  // until the edit it produces can fold the change in; otherwise the deselect files its own entry and
  // one undo restores the edge without its selection.
  const bool linkGestureResolving = linkDrop.createInFlight || edgeReconnect.isActive();
  if (suppressSelectionRecord)
  {
    lastSelection = eastl::move(curSelection);
    suppressSelectionRecord = false;
  }
  else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !linkGestureResolving && curSelection != lastSelection)
  {
    doc.recordSelectionChange(lastSelection, curSelection);
    lastSelection = eastl::move(curSelection);
  }

  if (const ne::NodeId dblClickedId = ne::GetDoubleClickedNode())
  {
    previewNodeId = static_cast<int>(dblClickedId.Get()) - 1;
  }
  else if (ne::IsBackgroundDoubleClicked())
  {
    previewNodeId = -1;
  }

  if (IGraphTexGenService *texGenService = plugin.getGraphPanel().texGenServiceFor(doc))
  {
    // The preview key is the texgen register name written onto the node's first output pin
    // (customTextureName, e.g. "_t_45_0") -- not the desc name, which many nodes share.
    const char *selectedName = nullptr;
    if (previewNodeId >= 0)
    {
      if (const GraphData::Node *const previewNode = find_node_by_id(graphData, previewNodeId))
      {
        for (const GraphData::Pin &p : previewNode->pins)
        {
          if (!p.customTextureName.empty())
          {
            selectedName = p.customTextureName.c_str();
            break;
          }
        }
      }
    }

    // Selection change is the only thing we need to push: the service repopulates selectedTexState
    // synchronously at the end of finalizeTexGen, so the preview/histogram panels just read it.
    const eastl::string_view newName(selectedName ? selectedName : "");
    if (newName != eastl::string_view(lastSelectedNodeName.data(), lastSelectedNodeName.size()))
    {
      lastSelectedNodeName.assign(newName.begin(), newName.end());
      texGenService->setPreviewFinal(selectedName);
    }
  }

  if (edgeReconnect.isActive())
  {
    bool resolved = false;
    GraphData::Edge newEdge;
    const GraphData::Edge *addedEdge = nullptr;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
      resolved = true; // cancelled: the picked edge stays removed, recorded as a plain deletion below
    }
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
      resolved = true;
      GraphData::Edge bridged;
      int targetNode = -1;
      int targetPin = -1;
      if (hovered_pin(targetNode, targetPin))
      {
        if (edgeReconnect.tryComplete(graphData, targetNode, targetPin, bridged))
        {
          bridged.id = next_edge_id(graphData);
          bridged.muted = reconnectRemovedEdge.muted; // moving a wire must not switch it back on
          newEdge = bridged;
          addEdge(eastl::move(bridged));
          addedEdge = &newEdge;
        }
      }
    }
    if (resolved)
    {
      doc.recordReconnectEdge(reconnectRemovedEdge, reconnectRemovedEdgeIndex, addedEdge);
      edgeReconnect.cancel();
    }
  }

  // Read while the editor is still current: the bar draws after SetCurrentEditor(nullptr).
  int hintNode = -1;
  int hintPinIndex = -1;
  // Mirrors the dispatch conditions above, so the bar cannot advertise a key that is gated off.
  const bool pinHovered = hovered_pin(hintNode, hintPinIndex);
  // A standing menu answers for itself: it is Tab's own, or it is the re-route chooser, which the bar
  // does not advertise and which takes Tab for its own selection. The hover test cannot decide that,
  // as the menu sits beside the pin and leaves the cursor on it.
  const bool pinJumpLive = pinJumpMenu.isOpen() ? pinJumpMenu.openedFor() == GraphPinJumpMenu::Purpose::Jump
                                                : has_reachable_destination(graphData, hintNode, hintPinIndex);
  // Pin and edge hover are mutually exclusive, so the same pair carries whichever one resolved.
  int hintEdgeId = -1;
  if (!pinHovered)
  {
    hintEdgeId = hovered_edge_source_pin(graphData, hintNode, hintPinIndex);
  }
  const bool edgeSourceHovered = hintEdgeId >= 0;
  GraphHotkeyContext hotkeyContext;
  hotkeyContext.pinJumpAvailable = pinCommandsLive && pinJumpLive;
  hotkeyContext.addNodeAvailable = pinCommandsLive && !pinJumpMenu.isOpen() && (pinHovered || edgeSourceHovered);
  // While armed the key is what ends the gesture, so the bar keeps offering it wherever the cursor
  // is -- but only where the dispatch would see it. A held button or the jump menu blocks both, and
  // that menu takes Space itself.
  hotkeyContext.addTransitNodeAvailable =
    pinCommandsLive && !pinJumpMenu.isOpen() &&
    (transitArmed || (hotkeyContext.addNodeAvailable && has_splice_target(graphData, hintNode, hintPinIndex, hintEdgeId)));
  // The drawn selection, not a second read: the bar trails a click by one frame.
  const SubgraphContext subgraph = resolveSubgraphContext(drawnSelection);
  const bool canvasKeysLive = !addNodePopup.isOpen();
  const bool tabFree = canvasKeysLive && pinCommandsLive && !pinJumpMenu.isOpen() && !hotkeyContext.pinJumpAvailable;
  const bool oneInstance = subgraph.selectedSubgraph != nullptr;
  hotkeyContext.createSubgraphAvailable = canvasKeysLive && subgraph.selectedNodeCount >= 2 && !edgeReconnect.isActive();
  hotkeyContext.openSubgraphAvailable = tabFree && oneInstance && !subgraph.selectedSubgraphOpen;
  hotkeyContext.focusSubgraphAvailable = tabFree && oneInstance && subgraph.selectedSubgraphOpen;
  hotkeyContext.explodeSubgraphAvailable = canvasKeysLive && oneInstance && !edgeReconnect.isActive();
  hotkeyContext.parentGraphAvailable = tabFree && !oneInstance && hostWindow != nullptr;
  lastHotkeyContext = hotkeyContext;

  drawArmedTransitPreview(canvas_min, canvas_max);

  // Resolved here because CanvasToScreen needs the editor current; drawn below in screen space, so it
  // holds its size under zoom like the popup above it.
  bool ghostVisible = false;
  ImVec2 ghostAnchor(0.0f, 0.0f);
  ImU32 ghostPinColor = 0;
  bool ghostSourceIsOutput = false;
  if (addNodePopup.isOpen() || linkDrop.overEmptyCanvas)
  {
    ghostSourceIsOutput = aimIsOutput;
    // The cursor while the drag is live, the settled drop point once the popup owns it.
    ghostAnchor = addNodePopup.isOpen() ? ne::CanvasToScreen(aimDropCanvas) : ImGui::GetMousePos();
    ghostPinColor = pin_color_for_type(pin_type_at(graphData, canvasCull.indexOf(aimNode), aimPin));
    ghostVisible = true;
  }

  ne::SetCurrentEditor(nullptr);

  if (ghostVisible)
  {
    draw_add_node_ghost(ImGui::GetWindowDrawList(), ghostAnchor, ghostPinColor, ghostSourceIsOutput, canvas_min, canvas_max);
  }

  if (edgeReconnect.isActive() || pendingRerouteEdge >= 0 || transitArmed || pinJumpMenu.isOpen() || contextMenu.isOpen() ||
      addNodePopup.isOpen())
  {
    hold_undo_redo_keys();
  }
}
