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
#include <libTools/util/hdpiUtil.h>
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
constexpr int PIN_DOT_SIZE = 10;
constexpr int PIN_DOT_SIZE_HOVER = 9;
constexpr int PIN_HOVER_HALO_SIZE = 16;    // design Dot_highlight
constexpr float PIN_COMMENT_MARGIN = 6.0f; // gap between a pin comment caption and the node edge it sits outside of
constexpr ImU32 PIN_OUTLINE_COLOR = IM_COL32(0x6A, 0x6A, 0x6A, 0xFF);
constexpr ImU32 PIN_HOVER_HALO_COLOR = IM_COL32(0x64, 0xFF, 0x64, 128); // #64FF64 at 0.50

constexpr int NODE_CORNER_RADIUS = 3;
constexpr int NODE_BORDER_WIDTH = 1; // drawn inside the body; hover does not change it, it only adds a shadow
constexpr int NODE_PADDING = 8;      // inner padding of all three bands, and the header's horizontal one

constexpr int NODE_BODY_WIDTH = 165;
constexpr int NODE_OUTPUT_COL_WIDTH = 45;

constexpr int NODE_HEADER_ICON_SIZE = 16; // Placeholder
constexpr int NODE_HEADER_ICON_GAP = 4;   // Placeholder

constexpr int NODE_HEADER_HEIGHT = 36;
constexpr int NODE_FOOTER_HEIGHT = 36;
constexpr int NODE_ROW_PITCH = 20; // one pin row
constexpr int NODE_TITLE_FONT_SIZE = 14;
constexpr int NODE_ROW_FONT_SIZE = 12;
constexpr int NODE_FOOTER_FONT_SIZE = 12;
constexpr int NODE_INPUT_COL_PAD_R = 4; // input column -> divider
constexpr int NODE_DIVIDER_WIDTH = 1;
constexpr int NODE_OUTPUT_COL_PAD_L = 8; // divider -> output column
constexpr int NODE_FOOTER_MIN_GAP = 8;   // space-between floor for the two footer texts

constexpr int NODE_SEPARATOR_GAP = 4;
constexpr int NODE_SEPARATOR_MIN_RULE = 8;

constexpr int NODE_PLATE_WIDTH = 8;
constexpr int NODE_PLATE_OUTLINE_WIDTH = 2;

constexpr int NODE_SHADOW_REACH = 22;
constexpr int NODE_SHADOW_LAYER_ALPHA = 5;
constexpr int NODE_SHADOW_STEPS = 16;

constexpr ImU32 NODE_BORDER_COLOR = IM_COL32(0x6A, 0x6A, 0x6A, 0xFF);
constexpr ImU32 NODE_HEADER_COLOR = IM_COL32(0x47, 0x47, 0x47, 0xFF); // header and footer band
constexpr ImU32 NODE_CONTENT_COLOR = IM_COL32(0x5A, 0x5A, 0x5A, 0xFF);
constexpr ImU32 NODE_CONTENT_FOCUSED_COLOR = IM_COL32(0x21, 0x21, 0x21, 0xFF);
constexpr ImU32 NODE_BAND_MINIMIZED_COLOR = IM_COL32(0x3D, 0x62, 0x99, 0xFF);
constexpr ImU32 NODE_PLATE_FILL_COLOR = IM_COL32(0x59, 0x7D, 0xB3, 0xFF); // == dark.blk Color_FrameBgHovered
constexpr ImU32 NODE_PLATE_STROKE_COLOR = IM_COL32(0x4A, 0x6D, 0xA4, 0xFF);
constexpr ImU32 NODE_TITLE_TEXT_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
constexpr ImU32 NODE_ROW_TEXT_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
constexpr ImU32 NODE_FOOTER_TEXT_COLOR = IM_COL32(0xD5, 0xD5, 0xD5, 0xFF);
constexpr ImU32 NODE_DIVIDER_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 26); // white at 0.10

constexpr const char *NODE_UNNAMED_TITLE = "<unnamed>";
constexpr const char *NODE_ELLIPSIS = "...";

constexpr int EDGE_THICKNESS = 2;
constexpr int EDGE_THICKNESS_ACTIVE = 4; // hovered, selected, or attached to a selected node

constexpr float EDGE_DEAD_ALPHA = 0.5f;
constexpr int EDGE_MUTED_DASH = 4; // dash and gap length, passed to ne::SetNextLinkDashSize

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

// Off-screen-cull memoization (GraphPanel::updateImgui): the cull pre-pass result is cached and rebuilt
// only when an input changes. When a pointer interaction (node drag / block resize) ends, keep reculling
// for this many frames so the final committed node bounds are captured.
constexpr int CULL_INTERACTION_SETTLE_FRAMES = 2;
// Below this much viewport movement (canvas units) the cached cull is treated as still valid; guards
// against floating-point noise rebuilding the pass while the view is static.
constexpr float CULL_VIEW_MOVE_EPS = 0.01f;

// Node level-of-detail (GraphPanel::updateImgui): at or below this canvas zoom the on-screen nodes are
// too small to read, so they render "reduced" (text and pin-square decoration dropped; boxes + links
// kept -- pins stay bound so links still land)
constexpr float LOD_SCALE = 0.1f;

// A node's live editor position must differ from its committed graphData position by more than this
// (canvas units) for a finished drag to be recorded as a move -- guards against sub-pixel noise.
constexpr float NODE_MOVE_EPSILON = 0.1f;

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

void apply_editor_defaults(ne::EditorContext *ed)
{
  ne::SetCurrentEditor(ed);
  ne::EnableShortcuts(false);

  ne::Style &style = ne::GetStyle();
  style.SelectedNodeBorderWidth = 0.0f;
  style.HoveredNodeBorderWidth = 0.0f;
  // Each of ne's three decorative link passes repaints the whole curve in one global colour, which would
  // throw away the per-type edge colour; hover and selection are conveyed by weight instead. A zero-alpha
  // stroke is skipped outright, so switching them off costs nothing. Highlight is inert while
  // StyleVar_HighlightConnectedLinks stays 0, but zeroing it keeps enabling that from resurfacing this.
  style.Colors[ne::StyleColor_HovLinkBorder] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  style.Colors[ne::StyleColor_SelLinkBorder] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  style.Colors[ne::StyleColor_HighlightLinkBorder] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  ne::SetCurrentEditor(nullptr);
}

class NeStyleScope
{
public:
  NeStyleScope() = default;
  ~NeStyleScope()
  {
    if (varCount > 0)
    {
      ne::PopStyleVar(varCount);
    }
    if (colorCount > 0)
    {
      ne::PopStyleColor(colorCount);
    }
  }
  NeStyleScope(const NeStyleScope &) = delete;
  NeStyleScope &operator=(const NeStyleScope &) = delete;

  void color(ne::StyleColor idx, ImU32 value)
  {
    ne::PushStyleColor(idx, ImColor(value));
    ++colorCount;
  }
  void var(ne::StyleVar idx, float value)
  {
    ne::PushStyleVar(idx, value);
    ++varCount;
  }
  void var(ne::StyleVar idx, const ImVec4 &value)
  {
    ne::PushStyleVar(idx, value);
    ++varCount;
  }

private:
  int varCount = 0;
  int colorCount = 0;
};

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

ImU32 deadColorForType(PinType t)
{
  const ImU32 packed = pinColorForType(t);
  const ImU32 alpha = static_cast<ImU32>(((packed >> IM_COL32_A_SHIFT) & 0xFF) * EDGE_DEAD_ALPHA);
  return (packed & ~IM_COL32_A_MASK) | (alpha << IM_COL32_A_SHIFT);
}

ImVec2 drawPinDot(ne::PinId pin_id, ne::PinKind kind, PinType type, bool has_link, bool has_live_link, bool draw_decoration = true)
{
  ne::BeginPin(pin_id, kind);

  const float box = static_cast<float>(hdpi::_pxS(PIN_DOT_SIZE));
  const ImVec2 cursor = ImGui::GetCursorScreenPos();
  const float lineH = ImGui::GetTextLineHeight();
  const ImVec2 a(cursor.x, cursor.y + (lineH - box) * 0.5f);
  const ImVec2 b(a.x + box, a.y + box);
  const ImVec2 center((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
  ImGui::Dummy(ImVec2(box, lineH));

  if (draw_decoration)
  {
    ImDrawList *dl = ImGui::GetWindowDrawList();

    const bool hovered = ne::GetHoveredPin() == pin_id;
    if (hovered)
    {
      dl->AddCircleFilled(center, static_cast<float>(hdpi::_pxS(PIN_HOVER_HALO_SIZE)) * 0.5f, PIN_HOVER_HALO_COLOR);
    }

    const float radius = static_cast<float>(hdpi::_pxS(hovered ? PIN_DOT_SIZE_HOVER : PIN_DOT_SIZE)) * 0.5f;
    const float outline = static_cast<float>(eastl::max(1, hdpi::_pxS(1)));

    const ImU32 dotColor = (has_link && !has_live_link) ? deadColorForType(type) : pinColorForType(type);
    if (has_link)
    {
      dl->AddCircleFilled(center, radius + outline, dotColor);
    }
    dl->AddCircleFilled(center, radius, PIN_OUTLINE_COLOR);
    dl->AddCircleFilled(center, radius - outline, dotColor);
  }

  ne::PinRect(a, b);

  ne::EndPin();

  return center;
}

void draw_pin_comment_outside(const ImVec2 &node_min, const ImVec2 &node_max, const ImVec2 &pin_center, bool is_input,
  eastl::string_view comment)
{
  const char *textBegin = comment.data();
  const char *textEnd = comment.data() + comment.size();
  const float y = pin_center.y - ImGui::GetTextLineHeight() * 0.5f;
  const float edgeGap = static_cast<float>(hdpi::_pxS(PIN_DOT_SIZE)) * 0.5f + PIN_COMMENT_MARGIN;
  const float x = is_input ? (node_min.x - edgeGap - ImGui::CalcTextSize(textBegin, textEnd).x) : (node_max.x + edgeGap);
  ImGui::GetWindowDrawList()->AddText(ImVec2(x, y), ImGui::GetColorU32(ImGuiCol_Text), textBegin, textEnd);
}

bool ellipsize_text(ImFont *font, float size, const char *text, float avail_w, eastl::string &out, float ellipsis_w,
  float *out_width = nullptr)
{
  const float fullW = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text).x;
  if (fullW <= avail_w)
  {
    if (out_width)
    {
      *out_width = fullW;
    }
    return false;
  }
  const char *cut = text;
  const float keptW = font->CalcTextSizeA(size, eastl::max(0.0f, avail_w - ellipsis_w), 0.0f, text, nullptr, &cut).x;
  out.assign(text, cut - text);
  out += NODE_ELLIPSIS;
  if (out_width)
  {
    *out_width = keptW + ellipsis_w;
  }
  return true;
}

struct NodeVisual
{
  ImU32 header = NODE_HEADER_COLOR;
  ImU32 content = NODE_CONTENT_COLOR;
  ImU32 footer = NODE_HEADER_COLOR;
  bool band = false;   // selected
  bool shadow = false; // hovered
};

// One content row: either a pin row (an input and / or an output, paired by position) or a full-width
// section divider. Separators split the pin list into sections and pairing restarts in each section,
// so a divider never leaves an input paired with an output from the other side of it.
struct NodeRow
{
  int inputPin = -1;
  int outputPin = -1;
  int separatorPin = -1; // >= 0 -> divider row; label is that pin's name
};

struct NodeLayout
{
  float h = 0.0f;
  float headerH = 0.0f;
  float contentH = 0.0f;
  float footerH = 0.0f;
  float rowPitch = 0.0f;
  int rowCount = 0;
  bool hasDivider = false;
  bool hasFooter = false;
};

NodeVisual resolve_node_visual(bool selected, bool focused, bool hovered, bool minimized)
{
  NodeVisual v;
  const bool tintBands = minimized && selected;
  v.header = tintBands ? NODE_BAND_MINIMIZED_COLOR : NODE_HEADER_COLOR;
  v.content = focused ? NODE_CONTENT_FOCUSED_COLOR : NODE_CONTENT_COLOR;
  v.footer = tintBands ? NODE_BAND_MINIMIZED_COLOR : (focused ? NODE_CONTENT_FOCUSED_COLOR : NODE_HEADER_COLOR);
  v.band = selected;
  v.shadow = hovered;
  return v;
}

void draw_soft_shadow(ImDrawList *dl, const ImVec2 &a, const ImVec2 &b, float rounding)
{
  const float reach = static_cast<float>(hdpi::_pxS(NODE_SHADOW_REACH));
  const ImU32 col = IM_COL32(0, 0, 0, NODE_SHADOW_LAYER_ALPHA);
  for (int i = 0; i < NODE_SHADOW_STEPS; ++i)
  {
    const float t = static_cast<float>(NODE_SHADOW_STEPS - i) / static_cast<float>(NODE_SHADOW_STEPS);
    const float off = reach * t * t;
    dl->AddRectFilled(ImVec2(a.x - off, a.y - off), ImVec2(b.x + off, b.y + off), col, rounding + off);
  }
}

float node_band_margin() { return static_cast<float>(hdpi::_pxS(NODE_PLATE_OUTLINE_WIDTH)); }

float node_band_reach() { return node_band_margin() + static_cast<float>(hdpi::_pxS(NODE_PLATE_WIDTH)); }

void stroke_inside(ImDrawList *dl, const ImVec2 &node_min, const ImVec2 &node_max, float at, float thickness, float radius,
  ImU32 color)
{
  const float inset = at + thickness * 0.5f - 0.5f;
  dl->AddRect(ImVec2(node_min.x + inset, node_min.y + inset), ImVec2(node_max.x - inset, node_max.y - inset), color,
    eastl::max(0.0f, radius - inset - 0.5f), ImDrawFlags_RoundCornersAll, thickness);
}

void draw_node_select_band(ImDrawList *dl, const ImVec2 &node_min, const ImVec2 &node_max)
{
  const float outline = node_band_margin();
  const float plate = static_cast<float>(hdpi::_pxS(NODE_PLATE_WIDTH));
  const float radius = static_cast<float>(hdpi::_pxS(NODE_CORNER_RADIUS)) + outline;

  stroke_inside(dl, node_min, node_max, outline, plate, radius, NODE_PLATE_FILL_COLOR);
  stroke_inside(dl, node_min, node_max, 0.0f, outline, radius, NODE_PLATE_STROKE_COLOR);
}

bool node_has_texture_output(const GraphData::Node &n, bool &out_is_particles)
{
  out_is_particles = false;
  for (const GraphData::Pin &p : n.pins)
  {
    if (p.hidden || p.role != PinRole::Out)
    {
      continue;
    }
    if (p.type == PinType::Particles)
    {
      out_is_particles = true;
      return true;
    }
    if (p.type == PinType::Texture1D || p.type == PinType::Texture2D || p.type == PinType::Texture3D)
    {
      return true;
    }
  }
  return false;
}

void build_footer_texts(const GraphData &gd, const GraphData::Node &n, bool is_particles, char *out_left, size_t left_size,
  char *out_right, size_t right_size)
{
  auto prop = [&n](const char *name, const char *fallback) -> const char * {
    const eastl::string *v = find_property_value(n, name);
    return (v && !v->empty()) ? v->c_str() : fallback;
  };
  const char *w = prop("texture width", is_particles ? "= 512" : "parent size");
  const char *h = prop("texture height", is_particles ? "= 1" : "width");
  const char *t = prop("texture type", "parent type");
  const char *a = prop("texture wrap", "parent wrap");

  int width = gd.graphTextureWidth;
  int height = gd.graphTextureHeight;
  if (w[0] == '=')
  {
    width = parse_graph_size(w, width);
  }
  if (h[0] == '=')
  {
    height = parse_graph_size(h, height);
  }
  else if (strcmp(h, "width") == 0)
  {
    height = width;
  }
  auto clampDim = [](int v) { return eastl::max(eastl::min(v, 8192), 1); };
  snprintf(out_left, left_size, "%dx%d", clampDim(width), clampDim(height));

  const char *type = (strcmp(t, "parent type") == 0 || strcmp(t, "graph type") == 0) ? gd.graphTextureType.c_str() : t;
  const char *wrapSrc = (strcmp(a, "parent wrap") == 0 || strcmp(a, "graph wrap") == 0) ? gd.graphTextureWrap.c_str() : a;
  snprintf(out_right, right_size, "%s %s", type, strcmp(wrapSrc, "clamp") == 0 ? "CL" : "WR");
}
} // namespace

GraphPanel::GraphPanel(GraphEditorPlg &plg, IGraphTexGenService *tex_gen_service, GraphData &graph_data) :
  plugin(plg), texGenService(tex_gen_service), graphData(graph_data)
{
  panelWindow = IEditorCoreEngine::get()->createPropPanel(this, "Graph");

  ne::Config cfg;
  initEditorConfig(cfg);
  editor = ne::CreateEditor(&cfg);
  apply_editor_defaults(editor);
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
  apply_editor_defaults(editor);

  navigationFramesLeft = 5;
  cullDirty = true;

  // Clear any stale selection -- the previous graph's selected node id is meaningless
  // against the new node set (most often: previous was non-empty, new is empty).
  selectedNodeId = -1;
  previewNodeId = -1;

  // Reset selection-undo tracking: the new graph starts with an empty selection, and the swap itself
  // must not record a "Select nodes" entry (the next settled frame resyncs the baseline).
  lastSelection = GraphSelection();
  pendingSelection = GraphSelection();
  hasPendingSelection = false;
  suppressSelectionRecord = true;

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
  cullDirty = true;
}

void GraphPanel::markPositionsPending(const eastl::vector<int> &node_ids)
{
  for (int id : node_ids)
  {
    pendingPositionIds.insert(id);
  }
  cullDirty = true;
}

void GraphPanel::markBlockSizesPending(const eastl::vector<int> &node_ids)
{
  for (int id : node_ids)
  {
    pendingBlockSizeIds.insert(id);
  }
  cullDirty = true;
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

// Reads graphData and the revision without graphMutex. Safe only because main is the only writer of
// nodes and edges -- the texgen worker holds the mutex but writes just the compiled BLKs (see
// GraphCompilerImpl). A worker that started writing either would race this silently.
void GraphPanel::refreshDeadPaths()
{
  const uint64_t rev = plugin.getGraphRevision();
  if (rev == deadPathsRevision)
  {
    return;
  }
  compute_dead_paths(graphData, deadPaths);

  linkedPins.clear();
  livePins.clear();
  for (int i = 0; i < static_cast<int>(graphData.edges.size()); ++i)
  {
    const GraphData::Edge &e = graphData.edges[i];
    const uint64_t a = makePinId(e.elemA, e.pinA);
    const uint64_t b = makePinId(e.elemB, e.pinB);
    linkedPins.insert(a);
    linkedPins.insert(b);
    if (!deadPaths.isDeadEdge(i))
    {
      livePins.insert(a);
      livePins.insert(b);
    }
  }

  deadPathsRevision = rev;
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
  cullDirty = true;
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
    cullDirty = true;
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
    if (previewNodeId == node_id)
    {
      previewNodeId = -1;
    }
    plugin.markGraphDirtyAndRegen();
    cullDirty = true;
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
      plugin.setPinCommentUndoable(nodeId, pinIndex, comment);
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
  plugin.deleteNodesUndoable(idsToDelete);
}

void GraphPanel::drawCommentNode(const GraphData::Node &n, bool selected)
{
  // Pull current property values; fall back to the descriptor's defaults (which makeNodeFromBaseBlk
  // already copied into propertyValues for fresh nodes) only on a missing entry.
  const eastl::string *textStr = find_property_value(n, "comment string");
  const eastl::string *fontStr = find_property_value(n, "font size");
  const char *text = (textStr && !textStr->empty()) ? textStr->c_str() : "//";
  const float fontSize = fontStr ? (float)atoi(fontStr->c_str()) : COMMENT_FONT_SIZE_DEFAULT;
  const float commentPad = eastl::max(COMMENT_PADDING, node_band_reach());

  NeStyleScope commentStyle;
  commentStyle.color(ne::StyleColor_NodeBg, COMMENT_BG_COLOR);
  commentStyle.color(ne::StyleColor_NodeBorder, COMMENT_BORDER_COLOR);
  commentStyle.var(ne::StyleVar_NodePadding, ImVec4(commentPad, commentPad, commentPad, commentPad));
  commentStyle.var(ne::StyleVar_NodeRounding, 4.0f);

  ne::BeginNode(ne::NodeId(makeNodeId(n.id)));
  if (selected)
  {
    const ne::NodeId nid = ne::NodeId(makeNodeId(n.id));
    const ImVec2 pos = ne::GetNodePosition(nid);
    const ImVec2 size = ne::GetNodeSize(nid);
    if (size.x > 0.0f && size.y > 0.0f)
    {
      draw_node_select_band(ImGui::GetWindowDrawList(), pos, ImVec2(pos.x + size.x, pos.y + size.y));
    }
  }
  // No title bar -- the big text IS the body, matching JS's "comment string" rendering. ImGui auto-
  // sizes the surrounding node frame to fit the text plus padding (JS uses box.width + 20 in
  // graphEditor.js:706-707; the same +20 lives in our COMMENT_PADDING * 2).
  draw_scaled_text(text, fontSize > 0.0f ? fontSize : COMMENT_FONT_SIZE_DEFAULT);
  ne::EndNode();
}

void GraphPanel::drawBlockNode(const GraphData::Node &n, bool selected)
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

  NeStyleScope blockStyle;
  blockStyle.color(ne::StyleColor_NodeBg, IM_COL32(0, 0, 0, 0));
  blockStyle.color(ne::StyleColor_NodeBorder, IM_COL32(0, 0, 0, 0));
  blockStyle.color(ne::StyleColor_GroupBg, bgColor);
  blockStyle.color(ne::StyleColor_GroupBorder, IM_COL32(0, 0, 0, 0));
  blockStyle.var(ne::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
  blockStyle.var(ne::StyleVar_NodeBorderWidth, 0.0f);
  blockStyle.var(ne::StyleVar_GroupBorderWidth, 2.0f);
  blockStyle.var(ne::StyleVar_NodeRounding, 4.0f);
  blockStyle.var(ne::StyleVar_GroupRounding, 4.0f);

  const float effectiveFontSize = fontSize > 0.0f ? fontSize : BLOCK_FONT_SIZE_DEFAULT;
  const float widthClamp = eastl::max(MIN_BLOCK_SIZE, n.blockWidth);
  const float heightClamp = eastl::max(MIN_BLOCK_SIZE, n.blockHeight);
  const float blockPad = eastl::max(COMMENT_PADDING, node_band_reach());
  const float headerH = effectiveFontSize + blockPad * 2.0f;
  // eastl::max is purely defensive: today MIN_BLOCK_SIZE=200 vs max headerH ~140 keeps groupH >= 60,
  // but a future MIN_BLOCK_SIZE / max-font-size combo could invert it.
  const float groupH = eastl::max(1.0f, heightClamp - headerH);

  // A resize undo/redo queued a new size for this block. ne stores the group bounds and ne::Group
  // reuses them for an existing group (ignoring the supplied size), so writing graphData alone would
  // not resize it -- push the size explicitly, before BeginNode so ne::Group reads the new bounds.
  // Mirrors SetNodePosition for move undo.
  if (auto it = pendingBlockSizeIds.find(n.id); it != pendingBlockSizeIds.end())
  {
    ne::SetGroupSize(ne::NodeId(makeNodeId(n.id)), ImVec2(widthClamp, groupH));
    pendingBlockSizeIds.erase(it);
  }

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
  constexpr float HEADER_INSET = 1.0f; // keeps the gray fill clear of the 2px outline corners.

  // Tint behind the gray header keeps the 1px HEADER_INSET strip showing the body's
  // translucent color, matching the look from when the group's GroupBg covered the full
  // block.
  dl->AddRectFilled(nodeOriginScreen, ImVec2(nodeOriginScreen.x + widthClamp, nodeOriginScreen.y + headerH), bgColor, 4.0f,
    ImDrawFlags_RoundCornersTop);

  dl->AddRectFilled(ImVec2(nodeOriginScreen.x + HEADER_INSET, nodeOriginScreen.y + HEADER_INSET),
    ImVec2(nodeOriginScreen.x + widthClamp - HEADER_INSET, nodeOriginScreen.y + headerH), COMMENT_BG_COLOR, 4.0f,
    ImDrawFlags_RoundCornersTop);

  // Outline the full block (header + body). GroupBorder is set transparent in the style push above so
  // this is the only visible outline; drawing on the content channel keeps it under any child node
  // channels, matching where GroupBorder used to render relative to children.
  const ImVec2 blockMax(nodeOriginScreen.x + widthClamp, nodeOriginScreen.y + heightClamp);
  stroke_inside(dl, nodeOriginScreen, blockMax, 0.0f, 2.0f, 4.0f, BLOCK_BORDER_COLOR);

  // After the header fills and the outline, both of which would paint over the band where it crosses
  // them, and before the header caption, which keeps its own pixels. A block reserves no margin for the
  // band -- it is sized by the user -- so the band lands on the outline and on blockPad of the header.
  if (selected)
  {
    draw_node_select_band(dl, nodeOriginScreen, blockMax);
  }

  // PushFont(NULL, sz) applies FontScaleMain * FontScaleDpi to the requested size; using
  // GetFontSize() in AddText then matches what draw_scaled_text produces for comment nodes,
  // so block headers stay proportional on hi-DPI viewports.
  ImGui::PushFont(nullptr, effectiveFontSize);
  dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(nodeOriginScreen.x + blockPad, nodeOriginScreen.y + blockPad),
    IM_COL32_BLACK, text);
  ImGui::PopFont();

  // Reserve header space so m_GroupBounds.Min.y lands at the bottom of the visible header
  // (see the long comment above ne::BeginNode for why this matters for the Header region).
  ImGui::Dummy(ImVec2(widthClamp, headerH));

  ne::Group(ImVec2(widthClamp, groupH));

  // 1x1 sentinel Dummy. Width is INTENTIONALLY 1px (not widthClamp): a wider Dummy would
  // pin m_Bounds.Max.x to widthClamp even after the user shrinks the group via SizeAction,
  // so user-driven width-shrink resizes would never make it back into GraphData. Max.y
  // extends 1px past m_GroupBounds.Max.y to break Node::GetGroupedNodes self-containment
  // recursion (imgui_node_editor.cpp:745). syncBlockSizes subtracts BLOCK_CONTAINMENT_GAP
  // from the reported m_Bounds height so n.blockHeight tracks the total block height
  // (header + body).
  ImGui::Dummy(ImVec2(BLOCK_CONTAINMENT_GAP, BLOCK_CONTAINMENT_GAP));

  ne::EndNode();
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
    // A resize undo/redo queued a SetGroupSize for this block but drawBlockNode has not applied it yet
    // (e.g. the block is culled): ne still reports the stale size, so skip -- reading it back here would
    // clobber the just-restored graphData size, silently reverting the undo. drawBlockNode drains the
    // push when the block renders, and the next pass syncs cleanly.
    if (pendingBlockSizeIds.find(n.id) != pendingBlockSizeIds.end())
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
  eastl::vector<GraphData::Edge> removedEdges;
  for (const GraphData::Node &n : graphData.nodes)
  {
    if (selected.find(n.id) != selected.end())
    {
      removedNodes.push_back(n);
    }
  }
  for (const GraphData::Edge &e : graphData.edges)
  {
    if (selected.find(e.elemA) != selected.end() || selected.find(e.elemB) != selected.end())
    {
      removedEdges.push_back(e);
    }
  }

  // One pass: drop every edge touching the selection, drop the selected nodes, then add the bridge
  // edges. Validation runs against the spliced graph (matching the JS, which reconnects after the
  // deletions), so a consumer's input pin reads as free once the removed node's edges are gone.
  eastl::vector<GraphData::Edge> bridgeEdges; // reconnect edges this op adds, with final ids -- for undo
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
  plugin.markGraphDirtyAndRegen();

  plugin.recordRemoveKeepingConnections(eastl::move(removedNodes), eastl::move(removedEdges), eastl::move(bridgeEdges));

  ne::ClearSelection();
}

void GraphPanel::readSelection(GraphSelection &out) const
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

void GraphPanel::setPendingSelection(const GraphSelection &selection)
{
  pendingSelection = selection;
  hasPendingSelection = true;
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

  eastl::vector<int> edgeIds;
  for (const GraphData::Edge &e : graphData.edges)
  {
    if ((e.elemA == nodeId && e.pinA == pinIndex) || (e.elemB == nodeId && e.pinB == pinIndex))
    {
      edgeIds.push_back(e.id);
    }
  }
  // Snapshots, erases, and records one "Delete edges" entry (no-op if the pin had no edges).
  plugin.deleteEdgesUndoable(edgeIds);
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
    eastl::vector<GraphData::Node> pastedNodes;
    eastl::vector<GraphData::Edge> pastedEdges;
    canvasClipboard.paste(*this, mouseCanvas, pastedNodes, pastedEdges);
    plugin.recordPaste(eastl::move(pastedNodes), eastl::move(pastedEdges));
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
        // Snapshot the picked edge before dropping it so the reconnect resolves as one undo step
        // (see recordReconnectEdge). begin() guarantees the edge exists when it returns an id.
        const auto it = eastl::find_if(graphData.edges.begin(), graphData.edges.end(),
          [pickedEdgeId](const GraphData::Edge &e) { return e.id == pickedEdgeId; });
        if (it != graphData.edges.end())
        {
          reconnectRemovedEdge = *it;
        }
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

  if (const ne::LinkId dblLink = ne::GetDoubleClickedLink())
  {
    plugin.toggleEdgeMutedUndoable(extractEdgeIdFromLinkId(dblLink.Get()));
  }

  refreshDeadPaths();

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

  // Cull viewport in canvas space. Cheap to compute every frame (two ScreenToCanvas + an inflate), and it
  // doubles as the change signal for pan / zoom / navigate-animation, which all move this rect.
  ImRect cullRect(ne::ScreenToCanvas(canvasMin), ne::ScreenToCanvas(canvasMax));
  {
    const ImVec2 sz = cullRect.GetSize();
    cullRect.Expand(ImVec2(sz.x * CULL_VIEWPORT_MARGIN_FRAC, sz.y * CULL_VIEWPORT_MARGIN_FRAC));
  }

  // Suspend culling while a framing operation is in flight (load fit over navigationFramesLeft, or a
  // deferred fit / show-next this frame): the fit measures only nodes drawn live this frame, so every
  // node must render. A few full-render frames during framing is a negligible one-shot cost.
  const bool forceAllVisible = navigationFramesLeft > 0 || fitContentReq || fitSelectionReq || fitSelectionMarginReq || showNextReq;

  // The cull pass is ~O(N^2) (ne::GetNodePosition / GetNodeSize are linear lookups), so its result
  // (cullNodes / cullNodeOrder) is cached and rebuilt only when an input changed. A held mouse button --
  // and a few frames after release -- forces a rebuild every frame: a node drag or block resize moves ne
  // bounds with no view or graph-data change, so neither cullRect nor cullDirty would notice.
  if (ImGui::IsAnyMouseDown())
  {
    cullSettleFrames = CULL_INTERACTION_SETTLE_FRAMES;
  }
  else if (cullSettleFrames > 0)
  {
    --cullSettleFrames;
  }

  const bool viewMoved =
    ImFabs(cullRect.Min.x - cullViewMin.x) > CULL_VIEW_MOVE_EPS || ImFabs(cullRect.Min.y - cullViewMin.y) > CULL_VIEW_MOVE_EPS ||
    ImFabs(cullRect.Max.x - cullViewMax.x) > CULL_VIEW_MOVE_EPS || ImFabs(cullRect.Max.y - cullViewMax.y) > CULL_VIEW_MOVE_EPS;

  // pendingPositionIds is drained inside the pass (each id's position pushed to ne once), so a non-empty
  // set must let it run; the size mismatch is a safety net for a node add / remove that bypassed cullDirty.
  const bool recull = cullDirty || forceAllVisible || viewMoved || cullSettleFrames > 0 || !pendingPositionIds.empty() ||
                      static_cast<int>(cullNodes.size()) != nodeCount;

  if (recull)
  {
    cullNodes.resize(nodeCount);
    cullNodeOrder.clear();
    cullNodeOrder.reserve(nodeCount);

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

    cullViewMin = cullRect.Min;
    cullViewMax = cullRect.Max;
    // A forceAllVisible pass is an all-visible snapshot, not the steady state -- rebuild once framing ends.
    cullDirty = forceAllVisible;
  }

  const float invZoom = ne::GetCurrentZoom();
  const bool zoomLod = invZoom > 0.0f && (1.0f / invZoom) <= LOD_SCALE;

  ImFont *const nodeFont = ImGui::GetFont();
  const float titleFontSize = static_cast<float>(hdpi::_pxS(NODE_TITLE_FONT_SIZE));
  const float rowFontSize = static_cast<float>(hdpi::_pxS(NODE_ROW_FONT_SIZE));
  const float footerFontSize = static_cast<float>(hdpi::_pxS(NODE_FOOTER_FONT_SIZE));
  const float nodePad = static_cast<float>(hdpi::_pxS(NODE_PADDING));
  const float contentPadX = node_band_reach() + nodePad;
  const float titleDx =
    contentPadX + static_cast<float>(hdpi::_pxS(NODE_HEADER_ICON_SIZE)) + static_cast<float>(hdpi::_pxS(NODE_HEADER_ICON_GAP));
  const float nodeRounding = static_cast<float>(hdpi::_pxS(NODE_CORNER_RADIUS));
  const float dividerWidth = static_cast<float>(hdpi::_pxS(NODE_DIVIDER_WIDTH));
  const float inputColPadR = static_cast<float>(hdpi::_pxS(NODE_INPUT_COL_PAD_R));
  const float separatorGap = static_cast<float>(hdpi::_pxS(NODE_SEPARATOR_GAP));
  const float separatorMinRule = static_cast<float>(hdpi::_pxS(NODE_SEPARATOR_MIN_RULE));
  const float pinDotSize = static_cast<float>(hdpi::_pxS(PIN_DOT_SIZE));
  const float nodeBorderWidth = static_cast<float>(hdpi::_pxS(NODE_BORDER_WIDTH));
  const float pinBoxHeight = ImGui::GetTextLineHeight(); // drawPinDot reserves this per pin
  const float headerHeight =
    eastl::max(static_cast<float>(hdpi::_pxS(NODE_HEADER_HEIGHT)), 2.0f * nodePad + eastl::max(titleFontSize, pinBoxHeight));
  const float footerHeight = eastl::max(static_cast<float>(hdpi::_pxS(NODE_FOOTER_HEIGHT)), 2.0f * nodePad + footerFontSize);
  const float rowPitch = eastl::max(eastl::max(static_cast<float>(hdpi::_pxS(NODE_ROW_PITCH)), rowFontSize), pinBoxHeight);
  const float bodyWidth = static_cast<float>(hdpi::_pxS(NODE_BODY_WIDTH));
  const float outputColPadL = static_cast<float>(hdpi::_pxS(NODE_OUTPUT_COL_PAD_L));
  // Width is fixed, so the divider is placed from the right: the output column keeps a constant width
  // and the input column takes what is left, which is how the design splits both of its states.
  const float dividerDx = bodyWidth - contentPadX - static_cast<float>(hdpi::_pxS(NODE_OUTPUT_COL_WIDTH)) - 0.5f * dividerWidth;
  auto textWidth = [nodeFont](const char *s, float size) { return nodeFont->CalcTextSizeA(size, FLT_MAX, 0.0f, s).x; };
  // One measurement per font size per frame, for every ellipsize_text call below.
  const float titleEllipsisW = textWidth(NODE_ELLIPSIS, titleFontSize);
  const float rowEllipsisW = textWidth(NODE_ELLIPSIS, rowFontSize);
  const float footerEllipsisW = textWidth(NODE_ELLIPSIS, footerFontSize);

  const ne::NodeId hoveredNodeId = ne::GetHoveredNode();
  GraphSelection drawnSelection;
  readSelection(drawnSelection);

  eastl::string truncatedTooltip;

  for (int ni = 0; ni < nodeCount; ++ni)
  {
    const GraphData::Node &n = graphData.nodes[ni];

    if (!cullNodes[ni].needed)
    {
      continue; // off-screen and not attached to any on-screen link -> skip entirely
    }

    const bool selected = eastl::binary_search(drawnSelection.nodes.begin(), drawnSelection.nodes.end(), n.id);

    // Comment / block annotation nodes (descName-driven, matches JS at graphEditor.js:552)
    // get their own renderers: no title bar, no pins, large text, block also gets z-pushed
    // behind and a corner resize handle. Continue past the generic title+pins block below.
    if (n.descName == "comment")
    {
      drawCommentNode(n, selected);
      continue;
    }
    if (n.descName == "block")
    {
      drawBlockNode(n, selected);
      continue;
    }

    // Drop the (illegible) text when the node is off-screen but kept live for a link, or when zoomed
    // too far out to read it; the pin-square decoration is dropped in the same two cases. BeginPin /
    // EndPin and PinRect still run for every pin (inside drawPinDot), so links stay bound and land
    // on the right pin -- only the square's AddRectFilled / AddRect draw is skipped.
    const bool reduced = !cullNodes[ni].visible || zoomLod;

    // Pin shape was resolved against the descriptor at load / drag-drop-insert time
    // (see resolve_node_pins in graph_data.cpp). Renderer just reads the cached fields.
    eastl::fixed_vector<int, 16, true> inputIdx;
    eastl::fixed_vector<int, 16, true> outputIdx;
    // Left-column stream: inputs and separators interleaved in descriptor order. Outputs are their
    // own stream and are NOT affected by separators -- a divider belongs to the input list only, so
    // the two columns stay independently indexed from the top exactly as they were before.
    eastl::fixed_vector<int, 24, true> leftItems;
    for (int i = 0; i < (int)n.pins.size(); ++i)
    {
      const GraphData::Pin &p = n.pins[i];
      if (p.hidden)
      {
        continue;
      }
      if (p.separator)
      {
        leftItems.push_back(i);
      }
      else if (p.isInput)
      {
        inputIdx.push_back(i);
        leftItems.push_back(i);
      }
      else
      {
        outputIdx.push_back(i);
      }
    }

    eastl::fixed_vector<NodeRow, 24, true> rows;
    const int rowTotal = static_cast<int>(eastl::max(leftItems.size(), outputIdx.size()));
    rows.resize(rowTotal);
    for (int r = 0; r < rowTotal; ++r)
    {
      if (r < static_cast<int>(leftItems.size()))
      {
        const int i = leftItems[r];
        if (n.pins[i].separator)
        {
          rows[r].separatorPin = i;
        }
        else
        {
          rows[r].inputPin = i;
        }
      }
      if (r < static_cast<int>(outputIdx.size()))
      {
        rows[r].outputPin = outputIdx[r];
      }
    }

    const char *const titleStr = n.descName.empty() ? NODE_UNNAMED_TITLE : n.descName.c_str();

    NodeLayout lay;
    bool isParticles = false;
    char footerLeft[32] = {0};
    char footerRight[32] = {0};
    lay.hasFooter = node_has_texture_output(n, isParticles);
    if (lay.hasFooter)
    {
      build_footer_texts(graphData, n, isParticles, footerLeft, sizeof(footerLeft), footerRight, sizeof(footerRight));
    }

    lay.rowCount = static_cast<int>(rows.size());
    lay.rowPitch = rowPitch;
    lay.headerH = headerHeight;
    lay.footerH = lay.hasFooter ? footerHeight : 0.0f;
    lay.contentH = lay.rowCount > 0 ? 2.0f * nodePad + lay.rowCount * lay.rowPitch : 0.0f;
    lay.hasDivider = !inputIdx.empty() && !outputIdx.empty();

    lay.h = ImCeil(lay.headerH + lay.contentH + lay.footerH);

    const ne::NodeId nid = ne::NodeId(makeNodeId(n.id));
    const NodeVisual vis = resolve_node_visual(selected, previewNodeId == n.id, hoveredNodeId == nid, reduced);

    NeStyleScope nodeStyle;
    nodeStyle.color(ne::StyleColor_NodeBg, IM_COL32(0, 0, 0, 0));
    nodeStyle.color(ne::StyleColor_NodeBorder, IM_COL32(0, 0, 0, 0));
    nodeStyle.var(ne::StyleVar_NodePadding, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    nodeStyle.var(ne::StyleVar_NodeBorderWidth, 0.0f);
    nodeStyle.var(ne::StyleVar_NodeRounding, nodeRounding);

    ne::BeginNode(nid);

    const ImVec2 bodyMin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(bodyWidth, lay.h));
    const ImVec2 bodyMax(bodyMin.x + bodyWidth, bodyMin.y + lay.h);

    ImDrawList *dl = ImGui::GetWindowDrawList();
    if (vis.shadow)
    {
      draw_soft_shadow(dl, bodyMin, bodyMax, nodeRounding);
    }

    const float contentTop = bodyMin.y + lay.headerH;
    const float footerTop = contentTop + lay.contentH;
    dl->AddRectFilled(bodyMin, bodyMax, vis.content, nodeRounding);
    dl->AddRectFilled(bodyMin, ImVec2(bodyMax.x, contentTop), vis.header, nodeRounding, ImDrawFlags_RoundCornersTop);
    if (lay.hasFooter)
    {
      dl->AddRectFilled(ImVec2(bodyMin.x, footerTop), bodyMax, vis.footer, nodeRounding, ImDrawFlags_RoundCornersBottom);
    }

    stroke_inside(dl, bodyMin, bodyMax, 0.0f, nodeBorderWidth, nodeRounding, NODE_BORDER_COLOR);

    if (vis.band)
    {
      draw_node_select_band(dl, bodyMin, bodyMax);
    }

    if (lay.hasDivider && lay.contentH > 0.0f)
    {
      const float dividerX = bodyMin.x + dividerDx;
      dl->AddLine(ImVec2(dividerX, contentTop + nodePad), ImVec2(dividerX, footerTop - nodePad), NODE_DIVIDER_COLOR, dividerWidth);
    }

    // Only the hovered node can own the tooltip, so the rect tests below never run for the rest.
    const bool nodeHovered = hoveredNodeId == nid;

    auto offerFullText = [&](bool cut, const ImVec2 &rect_min, const ImVec2 &rect_max, const char *full) {
      if (cut && nodeHovered && ImGui::IsMouseHoveringRect(rect_min, rect_max, false))
      {
        truncatedTooltip = full;
      }
    };

    const float rowsTop = contentTop + nodePad;
    const float inputColX = bodyMin.x + contentPadX;
    const float contentRightX = bodyMax.x - contentPadX;
    // Both columns stop at the divider when there is one; without one the single column owns the width.
    const float inputColRightX = lay.hasDivider ? (bodyMin.x + dividerDx - dividerWidth * 0.5f - inputColPadR) : contentRightX;
    const float outputColLeftX = lay.hasDivider ? (bodyMin.x + dividerDx + dividerWidth * 0.5f + outputColPadL) : inputColX;

    if (!reduced)
    {
      const float titleX = bodyMin.x + titleDx;
      eastl::string shortTitle;
      const bool titleCut = ellipsize_text(nodeFont, titleFontSize, titleStr, contentRightX - titleX, shortTitle, titleEllipsisW);
      dl->AddText(nodeFont, titleFontSize, ImVec2(titleX, bodyMin.y + (lay.headerH - titleFontSize) * 0.5f), NODE_TITLE_TEXT_COLOR,
        titleCut ? shortTitle.c_str() : titleStr);
      offerFullText(titleCut, ImVec2(titleX, bodyMin.y), ImVec2(contentRightX, contentTop), titleStr);
      if (lay.hasFooter)
      {
        const float footerTextY = footerTop + (lay.footerH - footerFontSize) * 0.5f;
        const float footerLeftX = bodyMin.x + contentPadX;
        eastl::string shortRight;
        float rightW = 0.0f;
        const bool rightCut =
          ellipsize_text(nodeFont, footerFontSize, footerRight, contentRightX - footerLeftX, shortRight, footerEllipsisW, &rightW);
        const char *const rightStr = rightCut ? shortRight.c_str() : footerRight;
        const float footerRightX = contentRightX - rightW;
        const float footerSplitX = eastl::max(footerLeftX, footerRightX - static_cast<float>(hdpi::_pxS(NODE_FOOTER_MIN_GAP)));
        const float footerBottom = footerTop + lay.footerH;
        const float leftBudget = footerSplitX - footerLeftX;
        eastl::string shortLeft;
        float leftW = 0.0f;
        const bool leftCut = ellipsize_text(nodeFont, footerFontSize, footerLeft, leftBudget, shortLeft, footerEllipsisW, &leftW);
        const char *const leftStr = leftCut ? shortLeft.c_str() : footerLeft;
        // Ellipsizing bottoms out at the width of "..." itself, so a budget narrower than that still
        // yields something too wide. Clip rather than let it run under the right text. Gated: a clip
        // rect splits the draw batch, and in the normal case the ellipsis already fits.
        const bool clipLeft = leftW > leftBudget;
        if (clipLeft)
        {
          dl->PushClipRect(ImVec2(footerLeftX, footerTop), ImVec2(footerSplitX, footerBottom), true);
        }
        dl->AddText(nodeFont, footerFontSize, ImVec2(footerLeftX, footerTextY), NODE_FOOTER_TEXT_COLOR, leftStr);
        if (clipLeft)
        {
          dl->PopClipRect();
        }
        dl->AddText(nodeFont, footerFontSize, ImVec2(footerRightX, footerTextY), NODE_FOOTER_TEXT_COLOR, rightStr);
        // Each footer text is hit-tested over its own half.
        offerFullText(leftCut, ImVec2(footerLeftX, footerTop), ImVec2(footerSplitX, footerBottom), footerLeft);
        offerFullText(rightCut, ImVec2(footerRightX, footerTop), ImVec2(contentRightX, footerBottom), footerRight);
      }
    }

    const float inputPinCursorX = bodyMin.x - pinDotSize * 0.5f;
    const float outputPinCursorX = bodyMax.x - pinDotSize * 0.5f;

    for (int row = 0; row < lay.rowCount; ++row)
    {
      const float rowTop = rowsTop + row * lay.rowPitch;
      const float pinBoxY = rowTop + (lay.rowPitch - pinBoxHeight) * 0.5f;
      const float labelY = rowTop + (lay.rowPitch - rowFontSize) * 0.5f;

      if (rows[row].separatorPin >= 0)
      {
        // Section divider: a rule across the content width, broken around a centred label. No pin is
        // declared for it, so link drag and hover cannot reach it -- the JS guards the same way.
        const float ruleY = rowTop + lay.rowPitch * 0.5f;
        const float ruleLeft = bodyMin.x + contentPadX;
        // A separator divides the input list, so its rule spans the input column and stops short of
        // the vertical divider by the same gap the column itself keeps -- it never reaches into the
        // output side. With no output pins there is no divider, so it runs the full content width.
        const float ruleRight = inputColRightX;
        const eastl::string &label = n.pins[rows[row].separatorPin].name;
        // The rule keeps its stubs whatever the label is: a label too wide for the gap is ellipsized,
        // it does not eat the rule (before the width was fixed it widened the column instead).
        const float labelMaxW = eastl::max(0.0f, ruleRight - ruleLeft - 2.0f * (separatorMinRule + separatorGap));
        eastl::string shortLabel;
        // Ellipsizing reports the width of the string it wants drawn, so the stub geometry below needs
        // no measurement of its own. An empty label short-circuits and leaves the width at 0.
        float shownW = 0.0f;
        const bool cut =
          !label.empty() && ellipsize_text(nodeFont, rowFontSize, label.c_str(), labelMaxW, shortLabel, rowEllipsisW, &shownW);
        const char *const shown = cut ? shortLabel.c_str() : label.c_str();
        if (shownW <= 0.0f)
        {
          dl->AddLine(ImVec2(ruleLeft, ruleY), ImVec2(ruleRight, ruleY), NODE_DIVIDER_COLOR, dividerWidth);
        }
        else
        {
          const float centerX = (ruleLeft + ruleRight) * 0.5f;
          const float half = shownW * 0.5f + separatorGap;
          dl->AddLine(ImVec2(ruleLeft, ruleY), ImVec2(centerX - half, ruleY), NODE_DIVIDER_COLOR, dividerWidth);
          dl->AddLine(ImVec2(centerX + half, ruleY), ImVec2(ruleRight, ruleY), NODE_DIVIDER_COLOR, dividerWidth);
          if (!reduced)
          {
            dl->AddText(nodeFont, rowFontSize, ImVec2(centerX - (half - separatorGap), labelY), NODE_ROW_TEXT_COLOR, shown);
            offerFullText(cut, ImVec2(ruleLeft, rowTop), ImVec2(ruleRight, rowTop + lay.rowPitch), label.c_str());
          }
        }
      }
      else if (rows[row].inputPin >= 0)
      {
        const int i = rows[row].inputPin;
        const GraphData::Pin &p = n.pins[i];
        ImGui::SetCursorScreenPos(ImVec2(inputPinCursorX, pinBoxY));
        const uint64_t pinKey = makePinId(n.id, i);
        const ImVec2 pinCenter = drawPinDot(ne::PinId(pinKey), ne::PinKind::Input, p.type, linkedPins.find(pinKey) != linkedPins.end(),
          livePins.find(pinKey) != livePins.end(), !reduced);
        if (edgeReconnect.isActive() && edgeReconnect.anchorNode() == n.id && edgeReconnect.anchorPin() == i)
        {
          edgeReconnect.setAnchorScreenPos(pinCenter.x, pinCenter.y);
        }
        if (!reduced)
        {
          if (!p.name.empty())
          {
            eastl::string shortName;
            const bool cut =
              ellipsize_text(nodeFont, rowFontSize, p.name.c_str(), inputColRightX - inputColX, shortName, rowEllipsisW);
            dl->AddText(nodeFont, rowFontSize, ImVec2(inputColX, labelY), NODE_ROW_TEXT_COLOR,
              cut ? shortName.c_str() : p.name.c_str());
            offerFullText(cut, ImVec2(inputColX, rowTop), ImVec2(inputColRightX, rowTop + lay.rowPitch), p.name.c_str());
          }
          if (!p.comment.empty())
          {
            draw_pin_comment_outside(bodyMin, bodyMax, pinCenter, /*is_input=*/true, p.comment);
          }
        }
      }
      if (rows[row].outputPin >= 0)
      {
        const int i = rows[row].outputPin;
        const GraphData::Pin &p = n.pins[i];
        ImGui::SetCursorScreenPos(ImVec2(outputPinCursorX, pinBoxY));
        const uint64_t pinKey = makePinId(n.id, i);
        const ImVec2 pinCenter = drawPinDot(ne::PinId(pinKey), ne::PinKind::Output, p.type,
          linkedPins.find(pinKey) != linkedPins.end(), livePins.find(pinKey) != livePins.end(), !reduced);
        if (edgeReconnect.isActive() && edgeReconnect.anchorNode() == n.id && edgeReconnect.anchorPin() == i)
        {
          edgeReconnect.setAnchorScreenPos(pinCenter.x, pinCenter.y);
        }
        if (!reduced)
        {
          if (!p.name.empty())
          {
            eastl::string shortName;
            float shownW = 0.0f;
            const bool cut =
              ellipsize_text(nodeFont, rowFontSize, p.name.c_str(), contentRightX - outputColLeftX, shortName, rowEllipsisW, &shownW);
            const char *const shown = cut ? shortName.c_str() : p.name.c_str();
            // Right-aligned, and ellipsizing already knows how wide the drawn string is.
            const float labelX = contentRightX - shownW;
            dl->AddText(nodeFont, rowFontSize, ImVec2(labelX, labelY), NODE_ROW_TEXT_COLOR, shown);
            offerFullText(cut, ImVec2(outputColLeftX, rowTop), ImVec2(contentRightX, rowTop + lay.rowPitch), p.name.c_str());
          }
          if (!p.comment.empty())
          {
            draw_pin_comment_outside(bodyMin, bodyMax, pinCenter, /*is_input=*/false, p.comment);
          }
        }
      }
    }

    ne::EndNode();
  }

  const float edgeThickness = static_cast<float>(hdpi::_pxS(EDGE_THICKNESS));
  const float edgeThicknessActive = static_cast<float>(hdpi::_pxS(EDGE_THICKNESS_ACTIVE));
  const ne::LinkId hoveredLink = ne::GetHoveredLink();
  // Type of the pin an edge endpoint lands on, or Unknown when the index pair does not resolve (a cull
  // cache built against a different node list, a stale pin cache after a descriptor reload); every
  // bound is checked because the caller's indices come from that cache. Unknown maps to plain white.
  auto endpointPinType = [this](int node_index, int pin_index) {
    if (node_index < 0 || node_index >= static_cast<int>(graphData.nodes.size()) || pin_index < 0 ||
        pin_index >= static_cast<int>(graphData.nodes[node_index].pins.size()))
    {
      return PinType::Unknown;
    }
    return graphData.nodes[node_index].pins[pin_index].type;
  };

  const float edgeMutedDash = static_cast<float>(hdpi::_pxS(EDGE_MUTED_DASH));

  for (int ei = 0; ei < static_cast<int>(graphData.edges.size()); ++ei)
  {
    const GraphData::Edge &e = graphData.edges[ei];
    // Skip a link whose endpoint node was culled this frame: ne::Link would no-op anyway (the pin
    // isn't live) and this avoids the per-edge FindPin lookups. Endpoints kept "reduced" still
    // declared their pins, so links reaching into the visible area survive.
    const int ia = cullNodeIndex(e.elemA);
    const int ib = cullNodeIndex(e.elemB);
    if ((ia >= 0 && !cullNodes[ia].needed) || (ib >= 0 && !cullNodes[ib].needed))
    {
      continue;
    }

    // Colour follows the data the edge carries, so it reads as one piece with the pin dots it joins.
    // elemA/pinA is the source (out) pin by the stored-edge convention.
    PinType type = endpointPinType(ia, e.pinA);
    if (type == PinType::Unknown)
    {
      type = endpointPinType(ib, e.pinB);
    }

    const bool active = hoveredLink == ne::LinkId(makeLinkId(e.id)) ||
                        eastl::binary_search(drawnSelection.links.begin(), drawnSelection.links.end(), e.id) ||
                        eastl::binary_search(drawnSelection.nodes.begin(), drawnSelection.nodes.end(), e.elemA) ||
                        eastl::binary_search(drawnSelection.nodes.begin(), drawnSelection.nodes.end(), e.elemB);
    const float thickness = active ? edgeThicknessActive : edgeThickness;

    const bool dead = deadPaths.isDeadEdge(ei);
    const ImU32 color = dead ? deadColorForType(type) : pinColorForType(type);

    if (e.muted)
    {
      ne::SetNextLinkDashSize(edgeMutedDash);
    }
    ne::Link(ne::LinkId(makeLinkId(e.id)), ne::PinId(makePinId(e.elemA, e.pinA)), ne::PinId(makePinId(e.elemB, e.pinB)),
      ImColor(color), thickness);
  }

  // Link creation. QueryNewLink hands us the pin where the user grabbed the drag and where they
  // dropped it; the user can grab either end first, so we re-orient to (output, input) before
  // storing -- mirrors the JS editor's convention that elemA/pinA is the source.
  // The dangling drag has no target pin yet, so it keeps the neutral colour and only matches the
  // edge weight; once a valid target is under the cursor, AcceptNewItem repaints it in its type colour.
  ne::BeginCreate(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), edgeThickness);
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
          const PinType srcType = endpointPinType(cullNodeIndex(srcNode), srcPin);
          if (ne::AcceptNewItem(ImColor(pinColorForType(srcType)), edgeThicknessActive))
          {
            // The validator's hideSameConnection pass treats an edge already on this pin pair as
            // replaced by the candidate, but nothing here ever removed it -- so the drag used to add
            // a second copy. Target the existing edge instead: reviving a muted one is what the
            // gesture means, and a live one already is what the user asked for. Stored orientation
            // is not guaranteed, so match both.
            const auto existing = eastl::find_if(graphData.edges.begin(), graphData.edges.end(), [&](const GraphData::Edge &e) {
              return (e.elemA == srcNode && e.pinA == srcPin && e.elemB == dstNode && e.pinB == dstPin) ||
                     (e.elemA == dstNode && e.pinA == dstPin && e.elemB == srcNode && e.pinB == srcPin);
            });
            if (existing != graphData.edges.end())
            {
              if (existing->muted)
              {
                plugin.toggleEdgeMutedUndoable(existing->id);
              }
            }
            else
            {
              GraphData::Edge edge;
              edge.id = allocateEdgeId();
              edge.elemA = srcNode;
              edge.pinA = srcPin;
              edge.elemB = dstNode;
              edge.pinB = dstPin;
              plugin.addEdgeUndoable(eastl::move(edge));
            }
          }
        }
        else
        {
          ne::RejectNewItem(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), edgeThickness);
        }
      }
    }
  }
  ne::EndCreate();

  // Link / node deletion. Both propagate into graphData; node deletes cascade-remove the
  // edges that referenced them (see removeNodeById) before erasing the node itself. Items
  // reach the BeginDelete queue via the CANVAS_DELETE_SELECTED shortcut at the top of
  // updateImgui (which calls ne::DeleteNode / ne::DeleteLink); the library's own Del-key
  // handler is suppressed via apply_editor_defaults so this is the only path.
  if (ne::BeginDelete())
  {
    ne::LinkId deletedLinkId;
    eastl::vector<int> deletedEdgeIds;
    while (ne::QueryDeletedLink(&deletedLinkId))
    {
      if (ne::AcceptDeletedItem())
      {
        deletedEdgeIds.push_back(extractEdgeIdFromLinkId(deletedLinkId.Get()));
      }
    }
    // One undo step for the links removed in this Delete; the actual erase happens here. Node deletes
    // are recorded separately (deferred to actObjects); undoing replays nodes-first, then these edges.
    if (!deletedEdgeIds.empty())
    {
      plugin.deleteEdgesUndoable(deletedEdgeIds);
    }
    ne::NodeId deletedNodeId;
    eastl::vector<eastl::pair<int, ImVec2>> livePositions; // (node id, live canvas pos) captured in-frame
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
      livePositions.push_back(eastl::pair<int, ImVec2>(node_id, ne::GetNodePosition(deletedNodeId)));
      if (nodeIt->descName == "block")
      {
        // childIds must be captured now -- ne::GetNodePosition / GetNodeSize is only valid
        // while we're inside the SetCurrentEditor scope of the ne::Begin/End that wraps us.
        collectNodesInsideBlock(node_id, pending.childIds);
        for (int childId : pending.childIds)
        {
          livePositions.push_back(eastl::pair<int, ImVec2>(childId, ne::GetNodePosition(ne::NodeId(makeNodeId(childId)))));
        }
      }
      pendingNodeDeletes.push_back(eastl::move(pending));
    }

    // Fold the just-captured live canvas positions into graphData. ne::GetNodePosition is only
    // valid here (inside the current-editor scope); the delete itself runs in actObjects, out of
    // frame. Live drags are not otherwise written back to Node.x/y, so without this a moved node
    // would be snapshotted -- and thus restored on undo -- at its stale load/spawn position.
    if (!livePositions.empty())
    {
      plugin.mutateGraphData([&livePositions](GraphData &gd) {
        eastl::hash_map<int, ImVec2> posById;
        posById.reserve(livePositions.size());
        for (const eastl::pair<int, ImVec2> &idPos : livePositions)
        {
          posById[idPos.first] = idPos.second;
        }
        for (GraphData::Node &n : gd.nodes)
        {
          auto it = posById.find(n.id);
          if (it != posById.end())
          {
            n.x = it->second.x;
            n.y = it->second.y;
          }
        }
      });
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
    const PinType anchorType = endpointPinType(cullNodeIndex(edgeReconnect.anchorNode()), edgeReconnect.anchorPin());
    edgeReconnect.drawPreview(ImGui::GetWindowDrawList(), ImGui::GetMousePos(), pinColorForType(anchorType), edgeThickness);
  }

  if (!truncatedTooltip.empty())
  {
    ne::Suspend();
    ImGui::SetTooltip("%s", truncatedTooltip.c_str());
    ne::Resume();
  }

  ne::End();

  // Pull post-frame block sizes back into GraphData. ne's built-in SizeAction handles the
  // resize-by-border interaction for group nodes; we just need to mirror the new size into
  // our persisted fields so save/reload round-trips it.
  syncBlockSizes();

  // On release, fold a finished drag into one undo entry: nodes whose position changed (ne owns the
  // live position and never writes it back) and blocks whose size changed (captured by syncBlockSizes).
  // A corner resize changes both for the same block, so one entry lets a single Ctrl+Z restore position
  // and size together. Runs on a real drag, or whenever a resize was captured -- a border drag can
  // resize below ImGui's drag threshold without setting the move flag. Must run while ne is current.
  if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
  {
    nodeDragInProgress = true;
  }
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && (nodeDragInProgress || !blockResizeOld.empty()))
  {
    nodeDragInProgress = false;

    eastl::vector<NodePos> oldPositions;
    eastl::vector<NodePos> newPositions;
    for (const GraphData::Node &n : graphData.nodes)
    {
      // A node still awaiting its first SetNodePosition has no meaningful live position yet.
      if (pendingPositionIds.find(n.id) != pendingPositionIds.end())
      {
        continue;
      }
      const ImVec2 live = ne::GetNodePosition(ne::NodeId(makeNodeId(n.id)));
      if (ImFabs(live.x - n.x) > NODE_MOVE_EPSILON || ImFabs(live.y - n.y) > NODE_MOVE_EPSILON)
      {
        oldPositions.push_back(NodePos{n.id, n.x, n.y});
        newPositions.push_back(NodePos{n.id, live.x, live.y});
      }
    }

    // blockResizeOld holds each block's pre-drag size (captured by syncBlockSizes); pair it with the
    // now-committed graphData size, skipping any that netted back to the original.
    eastl::vector<BlockSize> oldSizes;
    eastl::vector<BlockSize> newSizes;
    for (const BlockSize &before : blockResizeOld)
    {
      for (const GraphData::Node &n : graphData.nodes)
      {
        if (n.id == before.nodeId)
        {
          if (n.blockWidth != before.width || n.blockHeight != before.height)
          {
            oldSizes.push_back(before);
            newSizes.push_back(BlockSize{n.id, n.blockWidth, n.blockHeight});
          }
          break;
        }
      }
    }
    blockResizeOld.clear();

    if (!newPositions.empty() || !newSizes.empty())
    {
      plugin.commitNodeTransforms(eastl::move(oldPositions), eastl::move(newPositions), eastl::move(oldSizes), eastl::move(newSizes));
    }
  }

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

  // Apply a selection that an undo/redo queued (UndoSelectNodes). Done here, after the render pass, so
  // any nodes a sibling entry just re-added already exist in ne and can be selected. Marked suppressed
  // so the detector below resyncs rather than recording it as a fresh change.
  if (hasPendingSelection)
  {
    ne::ClearSelection();
    for (int id : pendingSelection.nodes)
    {
      ne::SelectNode(ne::NodeId(makeNodeId(id)), /*append=*/true);
    }
    for (int id : pendingSelection.links)
    {
      ne::SelectLink(ne::LinkId(makeLinkId(id)), /*append=*/true);
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
  // read here, while the editor is still current. Handed to draw_graph_status_bar below.
  const int selectedObjectCount = ne::GetSelectedObjectCount();

  // Selection-undo: a deliberate selection change (click, box-select, the select / show commands)
  // becomes its own "Select nodes" entry. A change folded into an edit (delete / paste / splice) or
  // applied by an undo sets suppressSelectionRecord instead -- it rides that entry. Recording is
  // coalesced to mouse-up so a rubber-band drag is one entry, not one per frame.
  GraphSelection curSelection;
  readSelection(curSelection);
  if (suppressSelectionRecord)
  {
    lastSelection = eastl::move(curSelection);
    suppressSelectionRecord = false;
  }
  else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && curSelection != lastSelection)
  {
    plugin.recordSelectionChange(lastSelection, curSelection);
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
      if (const ne::PinId hoveredPin = ne::GetHoveredPin())
      {
        int targetNode = -1;
        int targetPin = -1;
        extractPinFromId(hoveredPin.Get(), targetNode, targetPin);
        if (edgeReconnect.tryComplete(graphData, targetNode, targetPin, bridged))
        {
          bridged.id = allocateEdgeId();
          newEdge = bridged;
          addEdge(eastl::move(bridged));
          addedEdge = &newEdge;
        }
      }
    }
    if (resolved)
    {
      plugin.recordReconnectEdge(reconnectRemovedEdge, addedEdge);
      edgeReconnect.cancel();
    }
  }

  ne::SetCurrentEditor(nullptr);

  draw_graph_hotkeys_bar(canvasMax);

  ImGui::SetCursorScreenPos(ImVec2(canvasMin.x, canvasMin.y + canvasHeight));
  draw_graph_status_bar(graphData, texGenService, selectedObjectCount, statusBarHeight);
}
