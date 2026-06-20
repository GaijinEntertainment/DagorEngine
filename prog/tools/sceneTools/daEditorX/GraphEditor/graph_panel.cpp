// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_panel.h"

#include "command_definitions.h"
#include "graph_hotkeys_bar.h"
#include "graph_status_bar.h"
#include "graph_validation.h"
#include "plugin.h"
#include "pluginService/graph_tex_gen_service.h"

#include <de3_interface.h>
#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_interface.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <ioSys/dag_dataBlock.h>

#include <EASTL/algorithm.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/sort.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui_node_editor.h>

namespace ne = ax::NodeEditor;

namespace
{
constexpr float PIN_SQUARE_SIZE = 9.0f;
constexpr float PIN_HALO_RADIUS = 12.0f;
constexpr float COLUMNS_GAP = 24.0f;
constexpr float PIN_COMMENT_MARGIN = 6.0f; // gap between a pin comment caption and the node edge it sits outside of
// #8f8 at ~0.7 opacity, mirroring graphEditor.js pinSelectSvg (#pinSelectSvg fill="#8f8" fill-opacity=0.7).
constexpr ImU32 PIN_HOVER_HALO_COLOR = IM_COL32(0x88, 0xFF, 0x88, 178);

constexpr float COMMENT_FONT_SIZE_DEFAULT = 35.0f;
constexpr float BLOCK_FONT_SIZE_DEFAULT = 80.0f;
constexpr float COMMENT_PADDING = 10.0f;

constexpr ImU32 COMMENT_BG_COLOR = IM_COL32(0xCC, 0xCC, 0xCC, 0xFF);
constexpr ImU32 COMMENT_BORDER_COLOR = IM_COL32(0x00, 0x00, 0x00, 0xFF);
constexpr ImU32 BLOCK_BORDER_COLOR = IM_COL32(0x00, 0x00, 0x00, 0xFF);
// Block bg alpha == 0.2 in JS (graphEditor.js:821 fill-opacity = 0.2). 0.2 * 255 ~= 51.
constexpr uint8_t BLOCK_BG_ALPHA = 51;
constexpr ImU32 BLOCK_BG_FALLBACK_COLOR = IM_COL32(0x33, 0x33, 0x33, BLOCK_BG_ALPHA);
// JS clamps blockWidth / blockHeight to 200 on user resize (graphEditor.js:2756, :2764).
// imgui-node-editor's SizeAction has no minimum-size hook, so we enforce the floor at the
// supplied-size site (drawBlockNode) and again on the persisted value (syncBlockSizes).
constexpr float MIN_BLOCK_SIZE = 200.0f;
// 1px sentinel dummy after ne::Group so m_Bounds.Max.y > m_GroupBounds.Max.y. ne's
// Node::GetGroupedNodes calls FindNodesInRect(m_GroupBounds, ..., includeIntersecting=false)
// (imgui_node_editor.cpp:745) which uses rect.Contains(bounds). Without the gap m_Bounds ==
// m_GroupBounds, the node's own bounds are contained in its own group rect, so the node
// becomes its own child and GetGroupedNodes recurses until the stack overflows.
constexpr float BLOCK_CONTAINMENT_GAP = 1.0f;

// Off-screen node culling (GraphPanel::updateImgui): a node whose canvas-space rect is entirely
// outside the viewport, inflated by this fraction of the viewport size on each side, is not drawn
// this frame. The margin keeps nodes / links from popping at the edge during a pan and covers link
// bezier control-point bulge beyond the endpoint node rects.
constexpr float CULL_VIEWPORT_MARGIN_FRAC = 0.5f;

// Node level-of-detail (GraphPanel::updateImgui): at or below this canvas zoom the on-screen nodes are
// too small to read, so they render "reduced" (text and pin-square decoration dropped; boxes + links
// kept -- pins stay bound so links still land)
constexpr float LOD_SCALE = 0.1f;

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
  cfg.NavigateButtonIndex = ImGuiMouseButton_Middle;
  for (float z : GRAPH_EDITOR_ZOOM_LEVELS)
  {
    cfg.CustomZoomLevels.push_back(z);
  }
}

void disable_node_editor_shortcuts(ne::EditorContext *ed)
{
  ne::SetCurrentEditor(ed);
  ne::EnableShortcuts(false);
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
  // makePinId used for rendering. Both endpoints are inserted so the result is independent of
  // stored edge orientation (matches the JS reference, which inspects both endpoints' roles).
  eastl::hash_set<uint64_t> connectedPins;
  connectedPins.reserve(gd.edges.size() * 2);
  for (const GraphData::Edge &e : gd.edges)
  {
    connectedPins.insert(GraphPanel::makePinId(e.elemA, e.pinA));
    connectedPins.insert(GraphPanel::makePinId(e.elemB, e.pinB));
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
      if (connectedPins.find(GraphPanel::makePinId(n.id, j)) != connectedPins.end())
      {
        hasConnectedOutput = true;
        break;
      }
    }
    if (hasOutputPin && !hasConnectedOutput)
    {
      ne::SelectNode(ne::NodeId(GraphPanel::makeNodeId(n.id)), /*append=*/true);
    }
  }
}

const eastl::string *find_property_value(const GraphData::Node &n, const char *prop_name)
{
  for (const auto &pv : n.propertyValues)
  {
    if (pv.first == prop_name)
    {
      return &pv.second;
    }
  }
  return nullptr;
}

// Returns true on parse success. Comma-separated "r,g,b,a" with components in [0,1]; same format
// the descriptor's color property uses (base_nodes.blk's val:p4) and the format makeNodeFromBaseBlk
// writes into propertyValues. Falls back to opaque black on malformed input -- matches the
// degenerate path of properties_panel.cpp:115's parse_color helper.
bool parse_block_color(const eastl::string &s, float &r, float &g, float &b)
{
  if (s.empty())
  {
    return false;
  }
  float a = 1.0f;
  const int got = sscanf(s.c_str(), "%f,%f,%f,%f", &r, &g, &b, &a);
  if (got < 3)
  {
    r = g = b = 0.0f;
    return false;
  }
  auto clamp01 = [](float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); };
  r = clamp01(r);
  g = clamp01(g);
  b = clamp01(b);
  return true;
}

// Draw text at a property-driven base size by pushing the current font with an unscaled
// size of font_size_px. ImGui's 1.92+ PushFont(NULL, size) keeps the current font face and
// retargets size; global scale factors (FontScaleMain / FontScaleDpi) apply on top, so a
// "font size 35" property still renders proportionally bigger on a hi-DPI viewport.
void draw_scaled_text(const char *text, float font_size_px)
{
  ImGui::PushFont(nullptr, font_size_px);
  ImGui::TextUnformatted(text);
  ImGui::PopFont();
}

// Inverses of GraphPanel::makePinId / makeLinkId. Used by the link-create / link-delete
// plumbing to recover GraphData node-id / pin-index / edge-id values from the
// imgui-node-editor identifiers passed back via QueryNewLink / QueryDeletedLink.
void extractPinFromId(uint64_t pin_id, int &out_node_id, int &out_pin_index)
{
  out_node_id = static_cast<int>((pin_id >> 20) & 0xFFFFF) - 1;
  out_pin_index = static_cast<int>(pin_id & 0xFFFFF) - 1;
}

int extractEdgeIdFromLinkId(uint64_t link_id) { return static_cast<int>(link_id) - 1; }

int extractNodeIdFromNeNodeId(uint64_t ne_node_id) { return static_cast<int>(ne_node_id) - 1; }

ImU32 pinColorForType(PinType t)
{
  switch (t)
  {
    case PinType::Bool: return IM_COL32(0x77, 0x77, 0x77, 0xFF);
    case PinType::Int: return IM_COL32(0x11, 0x88, 0xFF, 0xFF);
    case PinType::Uint: return IM_COL32(0x00, 0x00, 0xAA, 0xFF);
    case PinType::Float: return IM_COL32(0x00, 0xAA, 0x00, 0xFF);
    case PinType::Float2: return IM_COL32(0xFF, 0xFF, 0x00, 0xFF);
    case PinType::Float3: return IM_COL32(0x00, 0xFF, 0xFF, 0xFF);
    case PinType::Float4: return IM_COL32(0xFF, 0x00, 0xFF, 0xFF);
    case PinType::Texture1D:
    case PinType::Texture2D:
    case PinType::Texture3D:
    case PinType::Texture2DArray:
    case PinType::Texture2DShdArray: return IM_COL32(0xFF, 0x88, 0x11, 0xFF);
    case PinType::Particles: return IM_COL32(0x88, 0x55, 0xFF, 0xFF);
    case PinType::BiomeData: return IM_COL32(0xFF, 0x99, 0x00, 0xFF);
    case PinType::NBSGbuffer: return IM_COL32(0xAA, 0x11, 0xAA, 0xFF);
    case PinType::MaterialT: return IM_COL32(0x88, 0x66, 0x44, 0xFF);
    case PinType::LayerT: return IM_COL32(0xFF, 0x44, 0x00, 0xFF);
    case PinType::MaskT: return IM_COL32(0x44, 0xFF, 0x00, 0xFF);
    case PinType::CtrlT: return IM_COL32(0x86, 0x00, 0x00, 0xFF);
    case PinType::Unknown: break;
  }
  return IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
}

ImVec2 drawPinSquare(ne::PinId pin_id, ne::PinKind kind, PinType type, bool draw_decoration = true)
{
  ne::BeginPin(pin_id, kind);

  const ImVec2 cursor = ImGui::GetCursorScreenPos();
  const float lineH = ImGui::GetTextLineHeight();
  const ImVec2 a(cursor.x, cursor.y + (lineH - PIN_SQUARE_SIZE) * 0.5f);
  const ImVec2 b(a.x + PIN_SQUARE_SIZE, a.y + PIN_SQUARE_SIZE);
  const ImVec2 center((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
  ImGui::Dummy(ImVec2(PIN_SQUARE_SIZE, lineH));

  if (draw_decoration)
  {
    ImDrawList *dl = ImGui::GetWindowDrawList();

    if (ne::GetHoveredPin() == pin_id)
    {
      dl->AddCircleFilled(center, PIN_HALO_RADIUS, PIN_HOVER_HALO_COLOR);
    }

    dl->AddRectFilled(a, b, pinColorForType(type));
    dl->AddRect(a, b, IM_COL32_BLACK);
  }

  ne::PinRect(a, b);

  ne::EndPin();

  return center;
}

void draw_pin_comment_outside(int node_id, const ImVec2 &pin_center, bool is_input, eastl::string_view comment)
{
  const char *textBegin = comment.data();
  const char *textEnd = comment.data() + comment.size();
  const ne::NodeId nid = ne::NodeId(GraphPanel::makeNodeId(node_id));
  const ImVec2 nodePos = ne::GetNodePosition(nid);
  const ImVec2 nodeSize = ne::GetNodeSize(nid);
  const float y = pin_center.y - ImGui::GetTextLineHeight() * 0.5f;
  const float x = is_input ? (nodePos.x - PIN_COMMENT_MARGIN - ImGui::CalcTextSize(textBegin, textEnd).x)
                           : (nodePos.x + nodeSize.x + PIN_COMMENT_MARGIN);
  ImGui::GetWindowDrawList()->AddText(ImVec2(x, y), ImGui::GetColorU32(ImGuiCol_Text), textBegin, textEnd);
}
} // namespace

GraphPanel::GraphPanel(GraphEditorPlg &plg, IGraphTexGenService *tex_gen_service, GraphData &graph_data) :
  plugin(plg), texGenService(tex_gen_service), graphData(graph_data)
{
  panelWindow = IEditorCoreEngine::get()->createPropPanel(this, "Graph");

  ne::Config cfg;
  initEditorConfig(cfg);
  editor = ne::CreateEditor(&cfg);
  disable_node_editor_shortcuts(editor);
}

GraphPanel::~GraphPanel()
{
  if (editor)
  {
    ne::DestroyEditor(editor);
    editor = nullptr;
  }
  IEditorCoreEngine::get()->deleteCustomPanel(panelWindow);
}

void GraphPanel::onGraphDataChanged()
{
  // Recreate editor so node-state caches don't leak across graphs.
  if (editor)
  {
    ne::DestroyEditor(editor);
    editor = nullptr;
  }
  ne::Config cfg;
  initEditorConfig(cfg);
  editor = ne::CreateEditor(&cfg);
  disable_node_editor_shortcuts(editor);

  navigationFramesLeft = 5;

  // Clear any stale selection -- the previous graph's selected node id is meaningless
  // against the new node set (most often: previous was non-empty, new is empty).
  selectedNodeId = -1;
  previewNodeId = -1;

  // Every loaded node needs its position pushed to ne::SetNodePosition once on first render.
  pendingPositionIds.clear();
  for (const GraphData::Node &n : graphData.nodes)
  {
    pendingPositionIds.insert(n.id);
  }

  if (texGenService && !lastSelectedNodeName.empty())
  {
    texGenService->setPreviewFinal(nullptr);
  }
  lastSelectedNodeName.clear();
}

void GraphPanel::addNode(GraphData::Node node)
{
  pendingPositionIds.insert(node.id);
  plugin.mutateGraphData([&](GraphData &gd) { gd.nodes.emplace_back(eastl::move(node)); });
}

int GraphPanel::allocateNodeId() const
{
  int maxId = -1;
  for (const GraphData::Node &n : graphData.nodes)
  {
    maxId = eastl::max(maxId, n.id);
  }
  return maxId + 1;
}

int GraphPanel::allocateEdgeId() const
{
  int maxId = -1;
  for (const GraphData::Edge &e : graphData.edges)
  {
    maxId = eastl::max(maxId, e.id);
  }
  return maxId + 1;
}

void GraphPanel::addEdge(GraphData::Edge edge)
{
  plugin.mutateGraphData([&](GraphData &gd) { gd.edges.emplace_back(eastl::move(edge)); });
  plugin.markGraphDirtyAndRegen();
}

bool GraphPanel::removeEdgeById(int edge_id)
{
  bool erased = false;
  plugin.mutateGraphData([&](GraphData &gd) {
    auto it = eastl::find_if(gd.edges.begin(), gd.edges.end(), [edge_id](const GraphData::Edge &e) { return e.id == edge_id; });
    if (it != gd.edges.end())
    {
      gd.edges.erase(it);
      erased = true;
    }
  });
  if (erased)
  {
    plugin.markGraphDirtyAndRegen();
  }
  return erased;
}

bool GraphPanel::removeNodeById(int node_id)
{
  bool erasedAny = false;
  plugin.mutateGraphData([&](GraphData &gd) {
    auto edgeNewEnd = eastl::remove_if(gd.edges.begin(), gd.edges.end(),
      [node_id](const GraphData::Edge &e) { return e.elemA == node_id || e.elemB == node_id; });
    if (edgeNewEnd != gd.edges.end())
    {
      gd.edges.erase(edgeNewEnd, gd.edges.end());
      erasedAny = true;
    }
    auto nodeIt = eastl::find_if(gd.nodes.begin(), gd.nodes.end(), [node_id](const GraphData::Node &n) { return n.id == node_id; });
    if (nodeIt != gd.nodes.end())
    {
      gd.nodes.erase(nodeIt);
      erasedAny = true;
    }
  });
  if (erasedAny)
  {
    plugin.markGraphDirtyAndRegen();
  }
  return erasedAny;
}

void GraphPanel::collectNodesInsideBlock(int block_node_id, eastl::vector<int> &out_child_ids) const
{
  out_child_ids.clear();

  const ImVec2 blockMin = ne::GetNodePosition(ne::NodeId(makeNodeId(block_node_id)));
  const ImVec2 blockSize = ne::GetNodeSize(ne::NodeId(makeNodeId(block_node_id)));
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
    const ImVec2 pos = ne::GetNodePosition(ne::NodeId(makeNodeId(n.id)));
    const ImVec2 size = ne::GetNodeSize(ne::NodeId(makeNodeId(n.id)));
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

namespace
{
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
} // namespace

void GraphPanel::actObjects([[maybe_unused]] float dt)
{
  if (pendingCommentNodeId >= 0)
  {
    const int nodeId = pendingCommentNodeId;
    const int pinIndex = pendingCommentPinIndex;
    pendingCommentNodeId = -1;
    pendingCommentPinIndex = -1;

    eastl::string comment;
    for (const GraphData::Node &n : graphData.nodes)
    {
      if (n.id == nodeId && pinIndex >= 0 && pinIndex < static_cast<int>(n.pins.size()))
      {
        comment = n.pins[pinIndex].comment;
        break;
      }
    }

    if (plugin.promptPinComment(comment))
    {
      bool changed = false;
      plugin.mutateGraphData([&](GraphData &gd) {
        for (GraphData::Node &n : gd.nodes)
        {
          if (n.id == nodeId && pinIndex >= 0 && pinIndex < static_cast<int>(n.pins.size()))
          {
            if (n.pins[pinIndex].comment != comment)
            {
              n.pins[pinIndex].comment = comment;
              changed = true;
            }
            break;
          }
        }
      });
    }
  }

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

  for (const PendingNodeDelete &p : pending)
  {
    removeNodeById(p.nodeId);
  }
  if (deleteImplicit)
  {
    for (int childId : implicitChildren)
    {
      removeNodeById(childId);
    }
  }
}

void GraphPanel::drawCommentNode(const GraphData::Node &n)
{
  // Pull current property values; fall back to the descriptor's defaults (which makeNodeFromBaseBlk
  // already copied into propertyValues for fresh nodes) only on a missing entry.
  const eastl::string *textStr = find_property_value(n, "comment string");
  const eastl::string *fontStr = find_property_value(n, "font size");
  const char *text = (textStr && !textStr->empty()) ? textStr->c_str() : "//";
  const float fontSize = fontStr ? (float)atoi(fontStr->c_str()) : COMMENT_FONT_SIZE_DEFAULT;

  ne::PushStyleColor(ne::StyleColor_NodeBg, ImColor(COMMENT_BG_COLOR));
  ne::PushStyleColor(ne::StyleColor_NodeBorder, ImColor(COMMENT_BORDER_COLOR));
  ne::PushStyleVar(ne::StyleVar_NodePadding, ImVec4(COMMENT_PADDING, COMMENT_PADDING, COMMENT_PADDING, COMMENT_PADDING));
  ne::PushStyleVar(ne::StyleVar_NodeRounding, 4.0f);

  ne::BeginNode(ne::NodeId(makeNodeId(n.id)));
  // No title bar -- the big text IS the body, matching JS's "comment string" rendering. ImGui auto-
  // sizes the surrounding node frame to fit the text plus padding (JS uses box.width + 20 in
  // graphEditor.js:706-707; the same +20 lives in our COMMENT_PADDING * 2).
  draw_scaled_text(text, fontSize > 0.0f ? fontSize : COMMENT_FONT_SIZE_DEFAULT);
  ne::EndNode();

  ne::PopStyleVar(2);
  ne::PopStyleColor(2);
}

void GraphPanel::drawBlockNode(const GraphData::Node &n)
{
  const eastl::string *textStr = find_property_value(n, "comment string");
  const eastl::string *fontStr = find_property_value(n, "font size");
  const eastl::string *colorStr = find_property_value(n, "color");
  const char *text = (textStr && !textStr->empty()) ? textStr->c_str() : "Block";
  const float fontSize = fontStr ? (float)atoi(fontStr->c_str()) : BLOCK_FONT_SIZE_DEFAULT;

  // Block bg is the descriptor's color tinted to alpha 0.2 (graphEditor.js:821).
  ImU32 bgColor = BLOCK_BG_FALLBACK_COLOR;
  if (colorStr)
  {
    float r = 0.0f, g = 0.0f, b = 0.0f;
    if (parse_block_color(*colorStr, r, g, b))
    {
      bgColor = IM_COL32((int)(r * 255.0f + 0.5f), (int)(g * 255.0f + 0.5f), (int)(b * 255.0f + 0.5f), BLOCK_BG_ALPHA);
    }
  }

  ne::PushStyleColor(ne::StyleColor_NodeBg, ImColor(IM_COL32(0, 0, 0, 0)));
  ne::PushStyleColor(ne::StyleColor_NodeBorder, ImColor(IM_COL32(0, 0, 0, 0)));
  ne::PushStyleColor(ne::StyleColor_GroupBg, ImColor(bgColor));
  ne::PushStyleColor(ne::StyleColor_GroupBorder, ImColor(IM_COL32(0, 0, 0, 0)));
  ne::PushStyleVar(ne::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
  ne::PushStyleVar(ne::StyleVar_NodeBorderWidth, 0.0f);
  ne::PushStyleVar(ne::StyleVar_GroupBorderWidth, 2.0f);
  ne::PushStyleVar(ne::StyleVar_NodeRounding, 4.0f);
  ne::PushStyleVar(ne::StyleVar_GroupRounding, 4.0f);

  const float effectiveFontSize = fontSize > 0.0f ? fontSize : BLOCK_FONT_SIZE_DEFAULT;
  const float widthClamp = eastl::max(MIN_BLOCK_SIZE, n.blockWidth);
  const float heightClamp = eastl::max(MIN_BLOCK_SIZE, n.blockHeight);

  ne::BeginNode(ne::NodeId(makeNodeId(n.id)));
  const ImVec2 nodeOriginScreen = ImGui::GetCursorScreenPos();

  // The visible header is draw-list-only (AddRectFilled / AddText do not contribute to
  // m_Bounds), so a Dummy of the same height is submitted below as a real layout widget.
  // Without that Dummy, m_GroupBounds.Min.y == m_Bounds.Min.y and ne's Header region
  // (imgui_node_editor.cpp:823-829) collapses to a 5px strip at the very top, leaving
  // only that sliver hit-testable. Pushing the group down by headerH makes the Header
  // region span the full visible band, so a click anywhere on the gray header selects
  // (or starts dragging) the block.
  ImDrawList *dl = ImGui::GetWindowDrawList();
  const float headerH = effectiveFontSize + COMMENT_PADDING * 2.0f;
  constexpr float HEADER_INSET = 1.0f; // keeps the gray fill clear of the 2px outline corners.

  // Tint behind the gray header keeps the 1px HEADER_INSET strip showing the body's
  // translucent color, matching the look from when the group's GroupBg covered the full
  // block.
  dl->AddRectFilled(nodeOriginScreen, ImVec2(nodeOriginScreen.x + widthClamp, nodeOriginScreen.y + headerH), bgColor, 4.0f,
    ImDrawFlags_RoundCornersTop);

  dl->AddRectFilled(ImVec2(nodeOriginScreen.x + HEADER_INSET, nodeOriginScreen.y + HEADER_INSET),
    ImVec2(nodeOriginScreen.x + widthClamp - HEADER_INSET, nodeOriginScreen.y + headerH), COMMENT_BG_COLOR, 4.0f,
    ImDrawFlags_RoundCornersTop);

  // PushFont(NULL, sz) applies FontScaleMain * FontScaleDpi to the requested size; using
  // GetFontSize() in AddText then matches what draw_scaled_text produces for comment nodes,
  // so block headers stay proportional on hi-DPI viewports.
  ImGui::PushFont(nullptr, effectiveFontSize);
  dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
    ImVec2(nodeOriginScreen.x + COMMENT_PADDING, nodeOriginScreen.y + COMMENT_PADDING), IM_COL32_BLACK, text);
  ImGui::PopFont();

  // Reserve header space so m_GroupBounds.Min.y lands at the bottom of the visible header
  // (see the long comment above ne::BeginNode for why this matters for the Header region).
  ImGui::Dummy(ImVec2(widthClamp, headerH));

  // eastl::max is purely defensive: today MIN_BLOCK_SIZE=200 vs max headerH ~140 keeps
  // groupH >= 60, but a future MIN_BLOCK_SIZE / max-font-size combo could invert it.
  const float groupH = eastl::max(1.0f, heightClamp - headerH);
  ne::Group(ImVec2(widthClamp, groupH));

  // 1x1 sentinel Dummy. Width is INTENTIONALLY 1px (not widthClamp): a wider Dummy would
  // pin m_Bounds.Max.x to widthClamp even after the user shrinks the group via SizeAction,
  // so user-driven width-shrink resizes would never make it back into GraphData. Max.y
  // extends 1px past m_GroupBounds.Max.y to break Node::GetGroupedNodes self-containment
  // recursion (imgui_node_editor.cpp:745). syncBlockSizes subtracts BLOCK_CONTAINMENT_GAP
  // from the reported m_Bounds height so n.blockHeight tracks the total block height
  // (header + body).
  ImGui::Dummy(ImVec2(BLOCK_CONTAINMENT_GAP, BLOCK_CONTAINMENT_GAP));

  // Outline the full block (header + body). GroupBorder is set transparent in the style
  // push above so this is the only visible outline; drawing on the content channel keeps
  // it under any child node channels, matching where GroupBorder used to render relative
  // to children.
  dl->AddRect(nodeOriginScreen, ImVec2(nodeOriginScreen.x + widthClamp, nodeOriginScreen.y + heightClamp), BLOCK_BORDER_COLOR, 4.0f, 0,
    2.0f);

  ne::EndNode();

  ne::PopStyleVar(5);
  ne::PopStyleColor(4);
}

void GraphPanel::syncBlockSizes()
{
  // After ne::End(), the editor's SizeAction may have mutated m_Bounds / m_GroupBounds for a
  // group node the user dragged a border on. drawBlockNode submits a single ne::Group dummy
  // of (widthClamp, heightClamp - BLOCK_CONTAINMENT_GAP) plus a 1px sentinel Dummy, so
  // GetNodeSize returns m_Bounds.GetSize() == (widthClamp, heightClamp); the font-size-dependent
  // header is pure draw-list rendering and does not participate. We mirror any change back into
  // GraphData so the next save/reload preserves the user's resize. Skip zero-sized reads -- ne
  // reports 0,0 for nodes that haven't been laid out yet (the frame a new block is spawned).
  //
  // Clamp to MIN_BLOCK_SIZE so even if the user drags below the floor mid-session the
  // persisted value lands >= MIN_BLOCK_SIZE (JS parity, graphEditor.js:2756). The same clamp
  // is applied on the read side in drawBlockNode, so once a write lands the next frame's
  // sz / clamped delta is zero and the diff check skips -- no ping-pong against drawBlockNode's
  // clamped supplied size.
  for (const GraphData::Node &n : graphData.nodes)
  {
    if (n.descName != "block")
    {
      continue;
    }
    const ImVec2 sz = ne::GetNodeSize(ne::NodeId(makeNodeId(n.id)));
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
    const int nodeId = n.id;
    plugin.mutateGraphData([&](GraphData &gd) {
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

void GraphPanel::showNextSelectedNode()
{
  eastl::fixed_vector<int, 32, true> selected;
  for (const GraphData::Node &n : graphData.nodes)
  {
    if (ne::IsNodeSelected(ne::NodeId(makeNodeId(n.id))))
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

  ne::SelectNode(ne::NodeId(makeNodeId(lastShownSelectedNodeId)), /*append=*/false);
  ne::NavigateToSelection(/*zoomIn=*/true);

  ne::ClearSelection();
  for (int id : selected)
  {
    ne::SelectNode(ne::NodeId(makeNodeId(id)), /*append=*/true);
  }
}

void GraphPanel::removeSelectedKeepingConnections()
{
  eastl::hash_set<int> selected;
  for (const GraphData::Node &n : graphData.nodes)
  {
    if (ne::IsNodeSelected(ne::NodeId(makeNodeId(n.id))))
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
  int srcNode = -1;
  int srcPin = -1;
  int bestInputPin = 0;
  eastl::vector<eastl::pair<int, int>> consumers; // (node id, input pin index)
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
      }
    }
    else if (role == PinRole::Out)
    {
      consumers.push_back(eastl::pair<int, int>(extNode, extPin));
    }
  }

  // One pass: drop every edge touching the selection, drop the selected nodes, then add the bridge
  // edges. Validation runs against the spliced graph (matching the JS, which reconnects after the
  // deletions), so a consumer's input pin reads as free once the removed node's edges are gone.
  plugin.mutateGraphData([&](GraphData &gd) {
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
    for (const eastl::pair<int, int> &c : consumers)
    {
      if (validate_new_edge(gd, srcNode, srcPin, c.first, c.second))
      {
        GraphData::Edge edge;
        edge.id = nextEdgeId++;
        edge.elemA = srcNode;
        edge.pinA = srcPin;
        edge.elemB = c.first;
        edge.pinB = c.second;
        gd.edges.push_back(eastl::move(edge));
      }
    }
  });
  plugin.markGraphDirtyAndRegen();

  ne::ClearSelection();
}

void GraphPanel::removeEdgesUnderCursor()
{
  const ne::PinId hoveredPin = ne::GetHoveredPin();
  if (!hoveredPin)
  {
    return;
  }

  int nodeId = -1;
  int pinIndex = -1;
  extractPinFromId(hoveredPin.Get(), nodeId, pinIndex);
  if (nodeId < 0 || pinIndex < 0)
  {
    return;
  }

  bool removedAny = false;
  plugin.mutateGraphData([&](GraphData &gd) {
    const auto newEnd = eastl::remove_if(gd.edges.begin(), gd.edges.end(), [nodeId, pinIndex](const GraphData::Edge &e) {
      return (e.elemA == nodeId && e.pinA == pinIndex) || (e.elemB == nodeId && e.pinB == pinIndex);
    });
    removedAny = newEnd != gd.edges.end();
    gd.edges.erase(newEnd, gd.edges.end());
  });
  if (removedAny)
  {
    plugin.markGraphDirtyAndRegen();
  }
}

void GraphPanel::jumpToOppositePin()
{
  const ne::PinId hoveredPin = ne::GetHoveredPin();
  if (!hoveredPin)
  {
    return;
  }
  int node = -1;
  int pin = -1;
  extractPinFromId(hoveredPin.Get(), node, pin);
  if (node < 0 || pin < 0)
  {
    return;
  }

  int oppNode = -1;
  int oppPin = -1;
  for (const GraphData::Edge &e : graphData.edges)
  {
    if (e.elemA == node && e.pinA == pin)
    {
      oppNode = e.elemB;
      oppPin = e.pinB;
      break;
    }
    if (e.elemB == node && e.pinB == pin)
    {
      oppNode = e.elemA;
      oppPin = e.pinA;
      break;
    }
  }
  if (oppNode < 0)
  {
    return;
  }

  const ImVec2 oppPinCanvas = ne::GetPinPosition(ne::PinId(makePinId(oppNode, oppPin)));
  ne::ScrollCanvasPointToScreen(oppPinCanvas, ImGui::GetMousePos());
}

int GraphPanel::cullNodeIndex(int node_id) const
{
  const auto it = eastl::lower_bound(cullNodeOrder.begin(), cullNodeOrder.end(), node_id,
    [](const eastl::pair<int, int> &entry, int id) { return entry.first < id; });
  return (it != cullNodeOrder.end() && it->first == node_id) ? it->second : -1;
}

void GraphPanel::updateImgui()
{
  ImGui::TextDisabled("(graph: %s)", graphData.sourcePath.empty() ? "<none>" : graphData.sourcePath.c_str());

  // Capture the canvas rect (used as drop target after ne::End). Done before ne::Begin
  // because the node-editor child window swallows the cursor area. A status-bar strip is
  // reserved below the canvas; the 4px floor keeps the canvas size positive on a tiny panel
  // (ImGuiEx::Canvas treats non-positive sizes as "use all available", which would put the
  // canvas underneath the bar).
  const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
  const ImVec2 canvasAvail = ImGui::GetContentRegionAvail();
  const float statusBarHeight = graph_status_bar_height();
  const float canvasHeight = eastl::max(4.0f, canvasAvail.y - statusBarHeight);
  const ImVec2 canvasMax(canvasMin.x + canvasAvail.x, canvasMin.y + canvasHeight);

  ne::SetCurrentEditor(editor);

  // Framing requests are captured here but the actual ne::NavigateTo* runs at end-of-frame (after
  // the node loop). ne's GetContentBounds / GetSelectionBounds only union nodes drawn (live) this
  // frame (internal.h GetBounds filters by m_IsLive), and off-screen culling skips the rest -- so
  // framing on load (view not yet on the graph) or framing an off-screen target would measure empty
  // bounds and no-op. Deferring + forcing a full render this frame (forceAllVisible) fixes that.
  const bool fitSelectionReq = shortcut_fired(CANVAS_FRAME_SELECTED);
  const bool fitSelectionMarginReq = shortcut_fired(CANVAS_FRAME_SELECTED_WITH_MARGIN);
  const bool fitContentReq = shortcut_fired(CANVAS_ZOOM_AND_CENTER);
  const bool showNextReq = shortcut_fired(CANVAS_SHOW_NEXT_SELECTED);

  if (shortcut_fired(CANVAS_COPY))
  {
    canvasClipboard.captureSelection(*this, graphData);
  }
  if (shortcut_fired(CANVAS_CUT))
  {
    canvasClipboard.captureSelection(*this, graphData);
    queue_selected_for_delete();
  }
  if (shortcut_fired(CANVAS_PASTE))
  {
    const ImVec2 mouseCanvas = ne::ScreenToCanvas(ImGui::GetMousePos());
    canvasClipboard.paste(*this, mouseCanvas);
  }
  if (shortcut_fired(CANVAS_DELETE_SELECTED))
  {
    queue_selected_for_delete();
  }
  if (shortcut_fired(CANVAS_SELECT_NODES_NO_OUTPUTS))
  {
    select_nodes_with_no_connected_outputs(graphData);
  }
  if (shortcut_fired(CANVAS_REMOVE_KEEP_CONNECTIONS))
  {
    removeSelectedKeepingConnections();
  }
  if (shortcut_fired(CANVAS_REMOVE_EDGES_AT_PIN))
  {
    removeEdgesUnderCursor();
  }
  if (shortcut_fired(CANVAS_MODIFY_EDGE_AT_PIN))
  {
    if (const ne::PinId hoveredPin = ne::GetHoveredPin())
    {
      int node = -1;
      int pin = -1;
      extractPinFromId(hoveredPin.Get(), node, pin);
      const int pickedEdgeId = edgeReconnect.begin(graphData, node, pin);
      if (pickedEdgeId >= 0)
      {
        removeEdgeById(pickedEdgeId);
      }
    }
  }
  if (shortcut_fired(CANVAS_JUMP_OPPOSITE_PIN))
  {
    jumpToOppositePin();
  }
  if (shortcut_fired(CANVAS_COMMENT_PIN))
  {
    // Record the pin under the cursor; the modal edit dialog runs later, in actObjects.
    if (const ne::PinId hoveredPin = ne::GetHoveredPin())
    {
      int node = -1;
      int pin = -1;
      extractPinFromId(hoveredPin.Get(), node, pin);
      if (node >= 0 && pin >= 0)
      {
        pendingCommentNodeId = node;
        pendingCommentPinIndex = pin;
      }
    }
  }

  ne::Begin("perf_graph", ImVec2(0.0f, canvasHeight));

  // Off-screen node culling. Build per-node visibility against the viewport (in canvas space)
  // before drawing: a node fully outside it is skipped entirely (no BeginNode -> none of ne's
  // per-node channel / layout / draw-list work), which is the bulk of the saving on a large graph
  // panned while zoomed in. A node that is off-screen but holds an endpoint of a link whose span
  // meets the viewport is drawn "reduced" rather than skipped, so the still-visible link resolves
  // (a link draws only if BOTH its pins were declared live this frame -- ne's DoLink).
  //
  // Position application (ids in pendingPositionIds get their canvas-space coords pushed to ne once;
  // framing is handled by NavigateToContent over navigationFramesLeft) runs in the pass below for
  // EVERY node, so a newly-added off-screen node still gets positioned and acquires bounds.
  const int nodeCount = static_cast<int>(graphData.nodes.size());
  cullNodes.resize(nodeCount);
  cullNodeOrder.clear();
  cullNodeOrder.reserve(nodeCount);
  {
    ImRect cullRect(ne::ScreenToCanvas(canvasMin), ne::ScreenToCanvas(canvasMax));
    {
      const ImVec2 sz = cullRect.GetSize();
      cullRect.Expand(ImVec2(sz.x * CULL_VIEWPORT_MARGIN_FRAC, sz.y * CULL_VIEWPORT_MARGIN_FRAC));
    }

    // Suspend culling while a framing operation is in flight (load fit over navigationFramesLeft, or
    // a deferred fit / show-next this frame): the fit measures only nodes drawn live this frame, so
    // every node must render. A few full-render frames during framing is a negligible one-shot cost.
    const bool forceAllVisible = navigationFramesLeft > 0 || fitContentReq || fitSelectionReq || fitSelectionMarginReq || showNextReq;

    for (int ni = 0; ni < nodeCount; ++ni)
    {
      const GraphData::Node &n = graphData.nodes[ni];
      cullNodeOrder.push_back({n.id, ni});

      if (auto pit = pendingPositionIds.find(n.id); pit != pendingPositionIds.end())
      {
        ne::SetNodePosition(ne::NodeId(makeNodeId(n.id)), ImVec2(n.x, n.y));
        pendingPositionIds.erase(pit);
      }

      const ne::NodeId nid = ne::NodeId(makeNodeId(n.id));
      const ImVec2 pos = ne::GetNodePosition(nid);
      const ImVec2 size = ne::GetNodeSize(nid);
      NodeCull &cull = cullNodes[ni];
      cull.rectMin = pos;
      cull.rectMax = ImVec2(pos.x + size.x, pos.y + size.y);
      // size (0,0) == never laid out (cold load / just added): treat visible so it lays out now.
      const bool boundsKnown = (size.x > 0.0f && size.y > 0.0f);
      cull.visible = forceAllVisible || !boundsKnown || cullRect.Overlaps(ImRect(cull.rectMin, cull.rectMax));
      cull.needed = cull.visible;
    }

    eastl::sort(cullNodeOrder.begin(), cullNodeOrder.end());

    // A link can cross the viewport even when neither endpoint node is inside it (two nodes on
    // opposite off-screen sides). Mark both endpoints needed when the union of their rects -- a
    // superset of the link's bounding box -- meets the viewport.
    for (const GraphData::Edge &e : graphData.edges)
    {
      const int ia = cullNodeIndex(e.elemA);
      const int ib = cullNodeIndex(e.elemB);
      if (ia < 0 || ib < 0)
      {
        continue;
      }
      if (cullNodes[ia].needed && cullNodes[ib].needed)
      {
        continue;
      }
      ImRect span(cullNodes[ia].rectMin, cullNodes[ia].rectMax);
      span.Add(ImRect(cullNodes[ib].rectMin, cullNodes[ib].rectMax));
      if (cullRect.Overlaps(span))
      {
        cullNodes[ia].needed = true;
        cullNodes[ib].needed = true;
      }
    }
  }

  const float invZoom = ne::GetCurrentZoom();
  const bool zoomLod = invZoom > 0.0f && (1.0f / invZoom) <= LOD_SCALE;

  for (int ni = 0; ni < nodeCount; ++ni)
  {
    const GraphData::Node &n = graphData.nodes[ni];

    if (!cullNodes[ni].needed)
    {
      continue; // off-screen and not attached to any on-screen link -> skip entirely
    }

    // Comment / block annotation nodes (descName-driven, matches JS at graphEditor.js:552)
    // get their own renderers: no title bar, no pins, large text, block also gets z-pushed
    // behind and a corner resize handle. Continue past the generic title+pins block below.
    if (n.descName == "comment")
    {
      drawCommentNode(n);
      continue;
    }
    if (n.descName == "block")
    {
      drawBlockNode(n);
      continue;
    }

    // Drop the (illegible) text when the node is off-screen but kept live for a link, or when zoomed
    // too far out to read it; the pin-square decoration is dropped in the same two cases. BeginPin /
    // EndPin and PinRect still run for every pin (inside drawPinSquare), so links stay bound and land
    // on the right pin -- only the square's AddRectFilled / AddRect draw is skipped.
    const bool reduced = !cullNodes[ni].visible || zoomLod;

    ne::BeginNode(ne::NodeId(makeNodeId(n.id)));

    // In reduced mode reserve each label's exact footprint with a Dummy instead of emitting
    // glyphs. Layout (node size, pin column widths, pin centres) stays identical -- CalcTextSize
    // is what TextUnformatted itself measures, and the pre-measure passes below already call it --
    // so a link to a reduced node still lands on the right pin. Identical only because
    // CurrLineTextBaseOffset == 0 at every label site here (title is first-on-line; each pin name
    // follows a baseline Dummy + SameLine); revisit if a taller item is mixed onto a name row.
    auto drawLabel = [reduced](const char *text) {
      if (reduced)
      {
        ImGui::Dummy(ImGui::CalcTextSize(text));
      }
      else
      {
        ImGui::TextUnformatted(text);
      }
    };

    drawLabel(n.descName.empty() ? "<unnamed>" : n.descName.c_str());

    // Pin shape was resolved against the descriptor at load / drag-drop-insert time
    // (see resolve_node_pins in graph_data.cpp). Renderer just reads the cached fields.
    eastl::fixed_vector<int, 16, true> inputIdx;
    eastl::fixed_vector<int, 16, true> outputIdx;
    for (int i = 0; i < (int)n.pins.size(); ++i)
    {
      const GraphData::Pin &p = n.pins[i];
      if (p.hidden)
      {
        continue;
      }
      if (p.isInput)
      {
        inputIdx.push_back(i);
      }
      else
      {
        outputIdx.push_back(i);
      }
    }
    const int rowCount = (int)eastl::max(inputIdx.size(), outputIdx.size());

    // Pre-measure column widths so output pins can be right-aligned within the node's
    // right column (input column stays left-aligned).
    const ImGuiStyle &style = ImGui::GetStyle();
    auto pinRowWidth = [&](int i) -> float {
      const GraphData::Pin &p = n.pins[i];
      float w = PIN_SQUARE_SIZE;
      if (!p.name.empty())
      {
        w += style.ItemSpacing.x + ImGui::CalcTextSize(p.name.c_str()).x;
      }
      return w;
    };
    float inputColMaxW = 0.0f;
    float outputColMaxW = 0.0f;
    for (int idx : inputIdx)
    {
      inputColMaxW = eastl::max(inputColMaxW, pinRowWidth(idx));
    }
    for (int idx : outputIdx)
    {
      outputColMaxW = eastl::max(outputColMaxW, pinRowWidth(idx));
    }

    const float rowStartX = ImGui::GetCursorPosX();
    const float gap = (inputColMaxW > 0.0f && outputColMaxW > 0.0f) ? COLUMNS_GAP : 0.0f;
    const float bodyW = inputColMaxW + gap + outputColMaxW;
    const char *titleStr = n.descName.empty() ? "<unnamed>" : n.descName.c_str();
    const float titleW = ImGui::CalcTextSize(titleStr).x;
    const float outputColRightX = rowStartX + eastl::max(titleW, bodyW);

    for (int row = 0; row < rowCount; ++row)
    {
      const bool hasIn = row < (int)inputIdx.size();
      const bool hasOut = row < (int)outputIdx.size();
      if (hasIn)
      {
        const int i = inputIdx[row];
        const GraphData::Pin &p = n.pins[i];
        const ImVec2 pinCenter = drawPinSquare(ne::PinId(makePinId(n.id, i)), ne::PinKind::Input, p.type, !reduced);
        if (edgeReconnect.isActive() && edgeReconnect.anchorNode() == n.id && edgeReconnect.anchorPin() == i)
        {
          edgeReconnect.setAnchorScreenPos(pinCenter.x, pinCenter.y);
        }
        ImGui::SameLine();
        drawLabel(p.name.empty() ? "" : p.name.c_str());
        if (!reduced && !p.comment.empty())
        {
          draw_pin_comment_outside(n.id, pinCenter, /*is_input=*/true, p.comment);
        }
      }
      if (hasOut)
      {
        const int i = outputIdx[row];
        const GraphData::Pin &p = n.pins[i];
        const float thisRowW = pinRowWidth(i);
        const float outX = outputColRightX - thisRowW;
        if (hasIn)
        {
          ImGui::SameLine(0.0f, 0.0f);
        }
        ImGui::SetCursorPosX(outX);
        drawLabel(p.name.empty() ? "" : p.name.c_str());
        ImGui::SameLine();
        const ImVec2 pinCenter = drawPinSquare(ne::PinId(makePinId(n.id, i)), ne::PinKind::Output, p.type, !reduced);
        if (edgeReconnect.isActive() && edgeReconnect.anchorNode() == n.id && edgeReconnect.anchorPin() == i)
        {
          edgeReconnect.setAnchorScreenPos(pinCenter.x, pinCenter.y);
        }
        if (!reduced && !p.comment.empty())
        {
          draw_pin_comment_outside(n.id, pinCenter, /*is_input=*/false, p.comment);
        }
      }
    }

    ne::EndNode();
  }


  for (const GraphData::Edge &e : graphData.edges)
  {
    // Skip a link whose endpoint node was culled this frame: ne::Link would no-op anyway (the pin
    // isn't live) and this avoids the per-edge FindPin lookups. Endpoints kept "reduced" still
    // declared their pins, so links reaching into the visible area survive.
    const int ia = cullNodeIndex(e.elemA);
    const int ib = cullNodeIndex(e.elemB);
    if ((ia >= 0 && !cullNodes[ia].needed) || (ib >= 0 && !cullNodes[ib].needed))
    {
      continue;
    }
    ne::Link(ne::LinkId(makeLinkId(e.id)), ne::PinId(makePinId(e.elemA, e.pinA)), ne::PinId(makePinId(e.elemB, e.pinB)));
  }

  // Link creation. QueryNewLink hands us the pin where the user grabbed the drag and where they
  // dropped it; the user can grab either end first, so we re-orient to (output, input) before
  // storing -- mirrors the JS editor's convention that elemA/pinA is the source.
  ne::BeginCreate();
  {
    ne::PinId startId, endId;
    if (ne::QueryNewLink(&startId, &endId) && startId && endId)
    {
      if (edgeReconnect.isActive())
      {
        // A "modify edge" reconnect (A) owns the pin interaction; swallow ne's own link creation so
        // the two don't both fire and leave a stray edge to whichever pin ne's drag started from.
        ne::RejectNewItem();
      }
      else
      {
        int aNode = -1, aPin = -1, bNode = -1, bPin = -1;
        extractPinFromId(startId.Get(), aNode, aPin);
        extractPinFromId(endId.Get(), bNode, bPin);

        // Re-orient so the stored edge always goes (Out -> In). Validator accepts either order
        // but persisted edges match the convention used elsewhere (load path, JS editor).
        bool aIsOut = false;
        if (aNode >= 0 && bNode >= 0)
        {
          for (const GraphData::Node &n : graphData.nodes)
          {
            if (n.id == aNode && aPin >= 0 && aPin < (int)n.pins.size())
            {
              aIsOut = (n.pins[aPin].role == PinRole::Out);
              break;
            }
          }
        }
        const int srcNode = aIsOut ? aNode : bNode;
        const int srcPin = aIsOut ? aPin : bPin;
        const int dstNode = aIsOut ? bNode : aNode;
        const int dstPin = aIsOut ? bPin : aPin;

        if (validate_new_edge(graphData, srcNode, srcPin, dstNode, dstPin))
        {
          if (ne::AcceptNewItem())
          {
            GraphData::Edge edge;
            edge.id = allocateEdgeId();
            edge.elemA = srcNode;
            edge.pinA = srcPin;
            edge.elemB = dstNode;
            edge.pinB = dstPin;
            addEdge(eastl::move(edge));
          }
        }
        else
        {
          ne::RejectNewItem(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);
        }
      }
    }
  }
  ne::EndCreate();

  // Link / node deletion. Both propagate into graphData; node deletes cascade-remove the
  // edges that referenced them (see removeNodeById) before erasing the node itself. Items
  // reach the BeginDelete queue via the CANVAS_DELETE_SELECTED shortcut at the top of
  // updateImgui (which calls ne::DeleteNode / ne::DeleteLink); the library's own Del-key
  // handler is suppressed via disable_node_editor_shortcuts so this is the only path.
  if (ne::BeginDelete())
  {
    ne::LinkId deletedLinkId;
    while (ne::QueryDeletedLink(&deletedLinkId))
    {
      if (ne::AcceptDeletedItem())
      {
        removeEdgeById(extractEdgeIdFromLinkId(deletedLinkId.Get()));
      }
    }
    ne::NodeId deletedNodeId;
    while (ne::QueryDeletedNode(&deletedNodeId))
    {
      const int node_id = extractNodeIdFromNeNodeId(deletedNodeId.Get());
      auto nodeIt = eastl::find_if(graphData.nodes.begin(), graphData.nodes.end(),
        [node_id](const GraphData::Node &n) { return n.id == node_id; });
      if (nodeIt == graphData.nodes.end())
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
      if (nodeIt->descName == "block")
      {
        // childIds must be captured now -- ne::GetNodePosition / GetNodeSize is only valid
        // while we're inside the SetCurrentEditor scope of the ne::Begin/End that wraps us.
        collectNodesInsideBlock(node_id, pending.childIds);
      }
      pendingNodeDeletes.push_back(eastl::move(pending));
    }
  }
  ne::EndDelete();

  // Deferred framing (requests captured at the top of updateImgui). Runs here, after the node loop,
  // so the nodes the fit measures were drawn live this frame -- forceAllVisible guaranteed a full
  // render on any frame one of these is set.
  if (showNextReq)
  {
    showNextSelectedNode();
  }
  if (fitSelectionMarginReq)
  {
    ne::NavigateToSelection(/*zoomIn=*/true);
  }
  else if (fitSelectionReq)
  {
    ne::NavigateToSelection(/*zoomIn=*/false);
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
    edgeReconnect.drawPreview(ImGui::GetWindowDrawList(), ImGui::GetMousePos());
  }

  ne::End();

  // Pull post-frame block sizes back into GraphData. ne's built-in SizeAction handles the
  // resize-by-border interaction for group nodes; we just need to mirror the new size into
  // our persisted fields so save/reload round-trips it.
  syncBlockSizes();

  // Drop target for drag-drop from BaseNodesPanel. Must be issued while the editor is still
  // current so ne::ScreenToCanvas works on the stored mouse pos. Drops onto an empty graph
  // are explicitly disallowed (a "new graph" UI for spawning the first node is a future
  // change) -- accept-before-delivery + NotAllowed cursor signals "drop here is rejected".
  {
    const ImGuiID dropId = ImGui::GetID("graph_drop_target");
    const ImRect dropRect(canvasMin, canvasMax);
    if (ImGui::BeginDragDropTargetCustom(dropRect, dropId))
    {
      if (const ImGuiPayload *p = ImGui::AcceptDragDropPayload("DAGOR_BASE_NODE"))
      {
        if (p->IsDelivery())
        {
          // Payload is the descriptor's templateUid (see base_nodes_panel.cpp::onBeginDrag).
          char templateUid[128] = {};
          memcpy(templateUid, p->Data, eastl::min<int>(p->DataSize, (int)sizeof(templateUid) - 1));
          const ImVec2 canvasPos = ne::ScreenToCanvas(ImGui::GetMousePos());
          plugin.spawnBaseNode(templateUid, canvasPos.x, canvasPos.y);
        }
      }
      ImGui::EndDragDropTarget();
    }
  }

  // Selection extraction. Done unconditionally (not just when texGenService is present) so
  // PropertiesPanel and other observers can read getSelectedNodeId() each frame. ne returns
  // a count via the second arg even though we only request one slot; we treat "exactly one"
  // as the selection signal -- multi-select and empty both map to -1.
  ne::NodeId selectedId;
  const int selCount = ne::GetSelectedNodes(&selectedId, 1);
  selectedNodeId = (selCount == 1) ? (static_cast<int>(selectedId.Get()) - 1) : -1;
  // Total selection size (nodes + links) for the status bar's "Selected:" counter; must be
  // read here, while the editor is still current. Handed to draw_graph_status_bar below.
  const int selectedObjectCount = ne::GetSelectedObjectCount();

  if (const ne::NodeId dblClickedId = ne::GetDoubleClickedNode())
  {
    previewNodeId = static_cast<int>(dblClickedId.Get()) - 1;
  }
  else if (ne::IsBackgroundDoubleClicked())
  {
    previewNodeId = -1;
  }

  if (texGenService)
  {
    // The preview key is the texgen register name written onto the node's first output pin
    // (customTextureName, e.g. "_t_45_0") -- not the desc name, which many nodes share.
    const char *selectedName = nullptr;
    if (previewNodeId >= 0)
    {
      for (const GraphData::Node &n : graphData.nodes)
      {
        if (n.id != previewNodeId)
        {
          continue;
        }
        for (const GraphData::Pin &p : n.pins)
        {
          if (!p.customTextureName.empty())
          {
            selectedName = p.customTextureName.c_str();
            break;
          }
        }
        break;
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
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
      edgeReconnect.cancel();
    }
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
      GraphData::Edge bridged;
      if (const ne::PinId hoveredPin = ne::GetHoveredPin())
      {
        int targetNode = -1;
        int targetPin = -1;
        extractPinFromId(hoveredPin.Get(), targetNode, targetPin);
        if (edgeReconnect.tryComplete(graphData, targetNode, targetPin, bridged))
        {
          bridged.id = allocateEdgeId();
          addEdge(eastl::move(bridged));
        }
      }
      edgeReconnect.cancel();
    }
  }

  ne::SetCurrentEditor(nullptr);

  draw_graph_hotkeys_bar(canvasMax);

  ImGui::SetCursorScreenPos(ImVec2(canvasMin.x, canvasMin.y + canvasHeight));
  draw_graph_status_bar(graphData, texGenService, selectedObjectCount, statusBarHeight);
}
