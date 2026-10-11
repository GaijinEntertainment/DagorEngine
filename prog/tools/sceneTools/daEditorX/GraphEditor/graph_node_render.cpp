// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_node_render.h"

#include "canvas_text.h"
#include "graph_edge_reconnect.h"
#include "graph_pin_colors.h"
#include "graph_pin_jump_menu.h"
#include "graph_theme.h"

#include <EditorCore/ec_imguiInitialization.h>
#include <de3_interface.h>
#include <libTools/util/hdpiUtil.h>
#include <propPanel/propPanelService.h>

#include <EASTL/algorithm.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/string_view.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui_node_editor.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace ne = ax::NodeEditor;

namespace
{
constexpr int PIN_DOT_SIZE = 10;
constexpr int PIN_DOT_SIZE_HOVER = 9;
constexpr int PIN_HOVER_HALO_SIZE = 16;    // design Dot_highlight
constexpr float PIN_COMMENT_MARGIN = 6.0f; // gap between a pin comment caption and the node edge it sits outside of
constexpr ImU32 PIN_OUTLINE_COLOR = IM_COL32(0x6A, 0x6A, 0x6A, 0xFF);
constexpr ImU32 PIN_HOVER_HALO_COLOR = IM_COL32(0x64, 0xFF, 0x64, 128);

constexpr int NODE_CORNER_RADIUS = 3;
constexpr int NODE_BORDER_WIDTH = 1; // drawn inside the body; hover does not change it, it only adds a shadow
constexpr int NODE_PADDING = 8;      // inner padding of all three bands, and the header's horizontal one

constexpr int NODE_BODY_WIDTH = 165;
constexpr int NODE_OUTPUT_COL_WIDTH = 45;

constexpr int NODE_HEADER_ICON_SIZE = 16; // Placeholder
constexpr int NODE_HEADER_ICON_GAP = 4;   // Placeholder

constexpr int NODE_HEADER_HEIGHT = 36;
constexpr int NODE_FOOTER_HEIGHT = 36;
constexpr int NODE_ROW_PITCH = 20;
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
constexpr ImU32 NODE_PLATE_STROKE_COLOR = IM_COL32(0x4A, 0x6D, 0xA4, 0xFF);
constexpr ImU32 NODE_TITLE_TEXT_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
constexpr ImU32 NODE_ROW_TEXT_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
constexpr ImU32 NODE_FOOTER_TEXT_COLOR = IM_COL32(0xD5, 0xD5, 0xD5, 0xFF);
constexpr ImU32 NODE_DIVIDER_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 26); // white at 0.10
constexpr ImU32 NODE_HEADER_ICON_HOVER_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 26);

// The canvas never goes light, so the dark theme's icon whatever the active theme is.
constexpr const char *NODE_SUBGRAPH_ICON = "dark/subgraph";

constexpr const char *NODE_UNNAMED_TITLE = "<unnamed>";
constexpr const char *NODE_ELLIPSIS = "...";

constexpr int GHOST_WIDTH = 61;
constexpr int GHOST_HEIGHT = 47;
constexpr int GHOST_CORNER_RADIUS = 5;
constexpr int GHOST_TEXT_PAD = 3;                                   // all four sides
constexpr int GHOST_NOTCH_DEPTH = 3;                                // the slot cut into each side edge
constexpr int GHOST_NOTCH_INSET = 6;                                // and its clearance from the top and bottom
constexpr int GHOST_PIN_DX = 12;                                    // body edge -> pin dot centre
constexpr ImU32 GHOST_PIN_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF); // no pin type yet

constexpr int CANVAS_HINT_PAD_X = 12;
constexpr int CANVAS_HINT_PAD_Y = 8;
constexpr int CANVAS_HINT_GAP = 12; // clear of whatever it is anchored beside

constexpr float COMMENT_FONT_SIZE_DEFAULT = 35.0f;
constexpr float BLOCK_FONT_SIZE_DEFAULT = 80.0f;
constexpr float COMMENT_PADDING = 10.0f;

constexpr ImU32 COMMENT_BG_COLOR = IM_COL32(0xCC, 0xCC, 0xCC, 0xFF);
// Shared by the comment body and the block caption -- both sit on the light COMMENT_BG_COLOR fill.
constexpr ImU32 COMMENT_TEXT_COLOR = IM_COL32(0x00, 0x00, 0x00, 0xFF);
constexpr ImU32 COMMENT_BORDER_COLOR = IM_COL32(0x00, 0x00, 0x00, 0xFF);
constexpr ImU32 BLOCK_BORDER_COLOR = IM_COL32(0x00, 0x00, 0x00, 0xFF);
// 0.2 fill-opacity in the JS reference.
constexpr uint8_t BLOCK_BG_ALPHA = 51;
constexpr ImU32 BLOCK_BG_FALLBACK_COLOR = IM_COL32(0x33, 0x33, 0x33, BLOCK_BG_ALPHA);

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

// Comma-separated "r,g,b,a" in [0,1] -- the format base_nodes.blk val:p4 properties are stored in.
// Malformed input yields opaque black and false.
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

// The one face whose advances do not depend on the rasterizer density, which is what lets the canvas
// bake one size at several densities without the layout moving.
ImFont *canvas_font()
{
  ImFont *font = DAEDITOR3.getPropPanelService()->getCustomFont(LINEAR_METRICS_FONT_NAME);
  // A non-null answer is not proof the face loaded: the host substitutes the default font when the
  // file is missing, and that one carries the hinted advance.
  if (!font || font == ImGui::GetIO().FontDefault)
  {
    static bool warned = false;
    if (!warned)
    {
      warned = true;
      DAEDITOR3.conWarning("GraphEditor: no '%s' font, canvas labels change width with the zoom", LINEAR_METRICS_FONT_NAME);
    }
    return ImGui::GetFont();
  }
  return font;
}

// PushFont retargets the size of the face it is given; the global FontScaleMain / FontScaleDpi
// factors apply on top, so a property-driven size still scales with DPI.
void draw_scaled_text(ImFont *font, const char *text, float font_size_px)
{
  ImGui::PushFont(font, font_size_px);
  {
    // PushFont already made it the current font, so this is the same one ImGui will draw with.
    const RasterizerDensityScope textDensity(*ImGui::GetFont(), ImGui::GetFontSize());
    ImGui::TextUnformatted(text);
  }
  ImGui::PopFont();
}

ImVec2 draw_pin_dot(const GraphNodeFrame &frame, ne::PinId pin_id, ne::PinKind kind, PinType type, bool has_link, bool has_live_link,
  bool draw_decoration = true, bool force_hovered = false)
{
  ne::BeginPin(pin_id, kind);

  const float box = frame.pinDotSize;
  const ImVec2 cursor = ImGui::GetCursorScreenPos();
  const float lineH = frame.pinBoxHeight;
  const ImVec2 a(cursor.x, cursor.y + (lineH - box) * 0.5f);
  const ImVec2 b(a.x + box, a.y + box);
  const ImVec2 center((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
  ImGui::Dummy(ImVec2(box, lineH));

  if (draw_decoration)
  {
    ImDrawList *dl = ImGui::GetWindowDrawList();

    const bool hovered = force_hovered || ne::GetHoveredPin() == pin_id;
    if (hovered)
    {
      dl->AddCircleFilled(center, frame.pinHoverHaloSize * 0.5f, PIN_HOVER_HALO_COLOR);
    }

    const float radius = (hovered ? frame.pinDotSizeHover : frame.pinDotSize) * 0.5f;
    const float outline = frame.pinOutline;

    const ImU32 dotColor = (has_link && !has_live_link) ? dead_color_for_type(type) : pin_color_for_type(type);
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

void draw_pin_comment_outside(const GraphNodeFrame &frame, const ImVec2 &node_min, const ImVec2 &node_max, const ImVec2 &pin_center,
  bool is_input, eastl::string_view comment)
{
  const char *textBegin = comment.data();
  const char *textEnd = comment.data() + comment.size();
  const float y = pin_center.y - frame.pinBoxHeight * 0.5f;
  const float edgeGap = frame.pinCommentEdgeGap;
  ImFont *const font = frame.font;
  const float fontSize = ImGui::GetFontSize();
  const float x =
    is_input ? (node_min.x - edgeGap - calc_canvas_text_size(*font, fontSize, textBegin, textEnd).x) : (node_max.x + edgeGap);
  add_canvas_text(ImGui::GetWindowDrawList(), *font, fontSize, ImVec2(x, y), GRAPH_TEXT_COLOR, textBegin, textEnd);
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

// ellipsize_text itself stays raw: the screen-space drag ghost shares it and must not take the
// canvas density.
bool ellipsize_canvas_text(ImFont *font, float size, const char *text, float avail_w, eastl::string &out, float ellipsis_w,
  float *out_width = nullptr)
{
  const RasterizerDensityScope density(*font, size);
  return ellipsize_text(font, size, text, avail_w, out, ellipsis_w, out_width);
}

using GhostTextLines = eastl::fixed_vector<eastl::string, 4, true>;

// Overflow past max_lines is folded back into the last line and ellipsized there, so the name loses
// its tail rather than ending at a word break.
void wrap_text_lines(ImFont *font, float size, const char *text, float wrap_w, int max_lines, float ellipsis_w, GhostTextLines &out)
{
  const char *const end = text + strlen(text);
  const char *cur = text;
  const char *lastLineStart = text;
  while (cur < end && static_cast<int>(out.size()) < max_lines)
  {
    lastLineStart = cur;
    const char *const lineEnd = font->CalcWordWrapPosition(size, cur, end, wrap_w);
    out.push_back(eastl::string(cur, lineEnd - cur));
    // What guarantees progress: the wrap position stalls on a newline, and this is what steps over
    // it (and over the blanks the break swallowed).
    cur = ImTextCalcWordWrapNextLineStart(lineEnd, end);
  }

  eastl::string elided;
  if (cur < end && ellipsize_text(font, size, eastl::string(lastLineStart, end - lastLineStart).c_str(), wrap_w, elided, ellipsis_w))
  {
    out.back() = elided;
  }
}

// One concave fill, not stacked rects: abutting bands each antialias against the background and
// would leave a seam.
void fill_ghost_body(ImDrawList *draw_list, const ImVec2 &box_min, const ImVec2 &box_max, float rounding, float notch_depth,
  float notch_inset)
{
  draw_list->PathArcToFast(ImVec2(box_min.x + rounding, box_min.y + rounding), rounding, 6, 9);
  draw_list->PathArcToFast(ImVec2(box_max.x - rounding, box_min.y + rounding), rounding, 9, 12);
  draw_list->PathLineTo(ImVec2(box_max.x, box_min.y + notch_inset));
  draw_list->PathLineTo(ImVec2(box_max.x - notch_depth, box_min.y + notch_inset));
  draw_list->PathLineTo(ImVec2(box_max.x - notch_depth, box_max.y - notch_inset));
  draw_list->PathLineTo(ImVec2(box_max.x, box_max.y - notch_inset));
  draw_list->PathArcToFast(ImVec2(box_max.x - rounding, box_max.y - rounding), rounding, 0, 3);
  draw_list->PathArcToFast(ImVec2(box_min.x + rounding, box_max.y - rounding), rounding, 3, 6);
  draw_list->PathLineTo(ImVec2(box_min.x, box_max.y - notch_inset));
  draw_list->PathLineTo(ImVec2(box_min.x + notch_depth, box_max.y - notch_inset));
  draw_list->PathLineTo(ImVec2(box_min.x + notch_depth, box_min.y + notch_inset));
  draw_list->PathLineTo(ImVec2(box_min.x, box_min.y + notch_inset));
  draw_list->PathFillConcave(NODE_CONTENT_COLOR);
}

struct NodeVisual
{
  ImU32 header = NODE_HEADER_COLOR;
  ImU32 content = NODE_CONTENT_COLOR;
  ImU32 footer = NODE_HEADER_COLOR;
  bool band = false;   // selected
  bool shadow = false; // hovered
};

// One content row: an input and / or an output paired by position, or a full-width section divider.
// Pairing restarts at each divider, so an input is never paired across one.
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

NodeVisual resolve_node_visual(bool selected, bool focused, bool hovered, bool reduced)
{
  NodeVisual v;
  const bool tintBands = reduced && selected;
  v.header = tintBands ? NODE_BAND_MINIMIZED_COLOR : NODE_HEADER_COLOR;
  v.content = focused ? NODE_CONTENT_FOCUSED_COLOR : NODE_CONTENT_COLOR;
  v.footer = tintBands ? NODE_BAND_MINIMIZED_COLOR : (focused ? NODE_CONTENT_FOCUSED_COLOR : NODE_HEADER_COLOR);
  v.band = selected;
  v.shadow = hovered;
  return v;
}

void stroke_inside(ImDrawList *dl, const ImVec2 &node_min, const ImVec2 &node_max, float at, float thickness, float radius,
  ImU32 color)
{
  const float inset = at + thickness * 0.5f - 0.5f;
  dl->AddRect(ImVec2(node_min.x + inset, node_min.y + inset), ImVec2(node_max.x - inset, node_max.y - inset), color,
    eastl::max(0.0f, radius - inset - 0.5f), ImDrawFlags_RoundCornersAll, thickness);
}

void draw_node_select_band(const GraphNodeFrame &frame, ImDrawList *dl, const ImVec2 &node_min, const ImVec2 &node_max)
{
  stroke_inside(dl, node_min, node_max, frame.bandMargin, frame.plateWidth, frame.bandRadius, NODE_PLATE_FILL_COLOR);
  stroke_inside(dl, node_min, node_max, 0.0f, frame.bandMargin, frame.bandRadius, NODE_PLATE_STROKE_COLOR);
}


// An unresolved node shows the graph defaults, as the compile does when it emits one.
void build_footer_texts(const GraphData &gd, const NodeResolution *res, int dimension, char *out_left, size_t left_size,
  char *out_right, size_t right_size)
{
  const int width = res ? res->width : gd.graphTextureWidth;
  const int height = res ? res->height : gd.graphTextureHeight;
  const int depth = res ? res->depth : gd.graphTextureDepth;
  if (dimension >= 3)
  {
    snprintf(out_left, left_size, "%dx%dx%d", width, height, depth);
  }
  else if (dimension == 2)
  {
    snprintf(out_left, left_size, "%dx%d", width, height);
  }
  else
  {
    snprintf(out_left, left_size, "%d", width);
  }

  const char *const type = res ? res->type.c_str() : gd.graphTextureType.c_str();
  const char *const wrap = res ? res->wrap.c_str() : gd.graphTextureWrap.c_str();
  snprintf(out_right, right_size, "%s %s", type, strcmp(wrap, "clamp") == 0 ? "CL" : "WR");
}
} // namespace

GraphNodeFrame::GraphNodeFrame(const GraphData &graph_data, const eastl::hash_set<uint64_t> &linked_pins,
  const eastl::hash_set<uint64_t> &live_pins, const GraphResolutions &node_resolutions, const GraphPinJumpMenu &pin_jump_menu,
  GraphEdgeReconnect &edge_reconnect, eastl::string &hover_tooltip, uint64_t hovered_ne_node_id, int preview_node_id,
  uint64_t highlight_pin_id) :
  graph(graph_data),
  linkedPins(linked_pins),
  livePins(live_pins),
  resolutions(node_resolutions),
  pinJumpMenu(pin_jump_menu),
  edgeReconnect(edge_reconnect),
  hoverTooltip(hover_tooltip),
  hoveredNeNodeId(hovered_ne_node_id),
  previewNodeId(preview_node_id),
  highlightPinId(highlight_pin_id)
{
  font = canvas_font();
  titleFontSize = static_cast<float>(hdpi::_pxS(NODE_TITLE_FONT_SIZE));
  rowFontSize = static_cast<float>(hdpi::_pxS(NODE_ROW_FONT_SIZE));
  footerFontSize = static_cast<float>(hdpi::_pxS(NODE_FOOTER_FONT_SIZE));
  nodePad = static_cast<float>(hdpi::_pxS(NODE_PADDING));
  bandMargin = static_cast<float>(hdpi::_pxS(NODE_PLATE_OUTLINE_WIDTH));
  plateWidth = static_cast<float>(hdpi::_pxS(NODE_PLATE_WIDTH));
  bandReach = bandMargin + plateWidth;
  contentPadX = bandReach + nodePad;
  headerIconSize = static_cast<float>(hdpi::_pxS(NODE_HEADER_ICON_SIZE));
  headerIconGap = static_cast<float>(hdpi::_pxS(NODE_HEADER_ICON_GAP));
  titleDx = contentPadX + headerIconSize + headerIconGap;
  nodeRounding = static_cast<float>(hdpi::_pxS(NODE_CORNER_RADIUS));
  dividerWidth = static_cast<float>(hdpi::_pxS(NODE_DIVIDER_WIDTH));
  inputColPadR = static_cast<float>(hdpi::_pxS(NODE_INPUT_COL_PAD_R));
  outputColPadL = static_cast<float>(hdpi::_pxS(NODE_OUTPUT_COL_PAD_L));
  separatorGap = static_cast<float>(hdpi::_pxS(NODE_SEPARATOR_GAP));
  separatorMinRule = static_cast<float>(hdpi::_pxS(NODE_SEPARATOR_MIN_RULE));
  pinDotSize = static_cast<float>(hdpi::_pxS(PIN_DOT_SIZE));
  nodeBorderWidth = static_cast<float>(hdpi::_pxS(NODE_BORDER_WIDTH));
  bodyWidth = static_cast<float>(hdpi::_pxS(NODE_BODY_WIDTH));
  pinBoxHeight = ImGui::GetTextLineHeight(); // draw_pin_dot reserves this per pin
  headerHeight =
    eastl::max(static_cast<float>(hdpi::_pxS(NODE_HEADER_HEIGHT)), 2.0f * nodePad + eastl::max(titleFontSize, pinBoxHeight));
  footerHeight = eastl::max(static_cast<float>(hdpi::_pxS(NODE_FOOTER_HEIGHT)), 2.0f * nodePad + footerFontSize);
  rowPitch = eastl::max(eastl::max(static_cast<float>(hdpi::_pxS(NODE_ROW_PITCH)), rowFontSize), pinBoxHeight);
  // Placed from the right so the output column keeps a constant width and the input column takes the
  // rest, which is how the design splits both of its states.
  dividerDx = bodyWidth - contentPadX - static_cast<float>(hdpi::_pxS(NODE_OUTPUT_COL_WIDTH)) - 0.5f * dividerWidth;
  bandRadius = nodeRounding + bandMargin;
  footerMinGap = static_cast<float>(hdpi::_pxS(NODE_FOOTER_MIN_GAP));
  pinDotSizeHover = static_cast<float>(hdpi::_pxS(PIN_DOT_SIZE_HOVER));
  pinHoverHaloSize = static_cast<float>(hdpi::_pxS(PIN_HOVER_HALO_SIZE));
  pinOutline = static_cast<float>(eastl::max(1, hdpi::_pxS(1)));
  pinCommentEdgeGap = pinDotSize * 0.5f + PIN_COMMENT_MARGIN;
  annotationPad = eastl::max(COMMENT_PADDING, bandReach);
  const int iconTexturePx = static_cast<int>(headerIconSize * canvas_raster_density(headerIconSize));
  subgraphIcon = DAEDITOR3.getPropPanelService()->getIconTextureId(NODE_SUBGRAPH_ICON, iconTexturePx);

  auto ellipsisWidth = [this](float size) { return calc_canvas_text_size(*font, size, NODE_ELLIPSIS).x; };
  titleEllipsisW = ellipsisWidth(titleFontSize);
  rowEllipsisW = ellipsisWidth(rowFontSize);
  footerEllipsisW = ellipsisWidth(footerFontSize);
}

void draw_soft_shadow(ImDrawList *dl, const ImVec2 &min, const ImVec2 &max, float rounding)
{
  const float reach = static_cast<float>(hdpi::_pxS(NODE_SHADOW_REACH));
  const ImU32 col = IM_COL32(0, 0, 0, NODE_SHADOW_LAYER_ALPHA);
  for (int i = 0; i < NODE_SHADOW_STEPS; ++i)
  {
    const float t = static_cast<float>(NODE_SHADOW_STEPS - i) / static_cast<float>(NODE_SHADOW_STEPS);
    const float off = reach * t * t;
    dl->AddRectFilled(ImVec2(min.x - off, min.y - off), ImVec2(max.x + off, max.y + off), col, rounding + off);
  }
}

void draw_comment_node(const GraphNodeFrame &frame, const GraphData::Node &node, bool selected)
{
  const eastl::string *textStr = find_property_value(node, "comment string");
  const eastl::string *fontStr = find_property_value(node, "font size");
  const char *text = (textStr && !textStr->empty()) ? textStr->c_str() : "//";
  const float fontSize = fontStr ? static_cast<float>(atoi(fontStr->c_str())) : COMMENT_FONT_SIZE_DEFAULT;
  const float commentPad = frame.annotationPad;

  NeStyleScope commentStyle;
  commentStyle.color(ne::StyleColor_NodeBg, COMMENT_BG_COLOR);
  commentStyle.color(ne::StyleColor_NodeBorder, COMMENT_BORDER_COLOR);
  commentStyle.var(ne::StyleVar_NodePadding, ImVec4(commentPad, commentPad, commentPad, commentPad));
  commentStyle.var(ne::StyleVar_NodeRounding, 4.0f);

  ne::BeginNode(ne::NodeId(make_node_id(node.id)));
  if (selected)
  {
    const ne::NodeId nid = ne::NodeId(make_node_id(node.id));
    const ImVec2 pos = ne::GetNodePosition(nid);
    const ImVec2 size = ne::GetNodeSize(nid);
    if (size.x > 0.0f && size.y > 0.0f)
    {
      draw_node_select_band(frame, ImGui::GetWindowDrawList(), pos, ImVec2(pos.x + size.x, pos.y + size.y));
    }
  }
  // No title bar: the text is the body, and ImGui sizes the node frame around it plus the padding.
  ImGui::PushStyleColor(ImGuiCol_Text, COMMENT_TEXT_COLOR);
  draw_scaled_text(frame.font, text, fontSize > 0.0f ? fontSize : COMMENT_FONT_SIZE_DEFAULT);
  ImGui::PopStyleColor();
  ne::EndNode();
}

void draw_block_node(const GraphNodeFrame &frame, const GraphData::Node &node, bool selected,
  eastl::hash_map<int, ImVec2> &applied_group_sizes)
{
  const eastl::string *textStr = find_property_value(node, "comment string");
  const eastl::string *fontStr = find_property_value(node, "font size");
  const eastl::string *colorStr = find_property_value(node, "color");
  const char *text = (textStr && !textStr->empty()) ? textStr->c_str() : "Block";
  const float fontSize = fontStr ? static_cast<float>(atoi(fontStr->c_str())) : BLOCK_FONT_SIZE_DEFAULT;

  // Block bg is the descriptor's color, tinted.
  ImU32 bgColor = BLOCK_BG_FALLBACK_COLOR;
  if (colorStr)
  {
    float r = 0.0f, g = 0.0f, b = 0.0f;
    if (parse_block_color(*colorStr, r, g, b))
    {
      bgColor = IM_COL32(static_cast<int>(r * 255.0f + 0.5f), static_cast<int>(g * 255.0f + 0.5f), static_cast<int>(b * 255.0f + 0.5f),
        BLOCK_BG_ALPHA);
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
  const float blockPad = frame.annotationPad;
  // PushFont scales by FontScaleMain and clamps to IMGUI_FONT_SIZE_MAX: the property value is not
  // the size the glyphs come out at.
  ImGui::PushFont(frame.font, effectiveFontSize);
  const float captionFontSize = ImGui::GetFontSize();
  ImGui::PopFont();
  const ImVec2 captionSize = calc_canvas_text_size(*frame.font, captionFontSize, text);

  const float headerH = captionSize.y + blockPad * 2.0f;
  // A block authored below the floor (the 1x1 ones the JS graphs converted with) takes the caption
  // box once: syncBlockSizes persists that as its authored size, so it is a migration, not a live fit.
  const float widthClamp =
    node.blockWidth < MIN_BLOCK_SIZE ? eastl::max(MIN_BLOCK_SIZE, captionSize.x + blockPad * 2.0f) : node.blockWidth;
  const float heightClamp = node.blockHeight < MIN_BLOCK_SIZE ? eastl::max(MIN_BLOCK_SIZE, headerH) : node.blockHeight;
  const float groupH = eastl::max(1.0f, heightClamp - headerH);

  // ne keeps the group bounds and ignores the size ne::Group is given for a group that already
  // exists, so every change -- a resize undo, a header that changed height -- has to be pushed here,
  // before BeginNode. Mid-drag it fires every frame, harmlessly: the push runs before ne applies this
  // frame's drag delta, so it writes back the size ne already holds.
  const ImVec2 groupSize(widthClamp, groupH);
  ImVec2 &appliedGroupSize = applied_group_sizes[node.id];
  if (fabsf(appliedGroupSize.x - groupSize.x) >= 0.5f || fabsf(appliedGroupSize.y - groupSize.y) >= 0.5f)
  {
    appliedGroupSize = groupSize;
    ne::SetGroupSize(ne::NodeId(make_node_id(node.id)), groupSize);
  }

  // The dummies below must add up to exactly heightClamp: ne zeroes ItemSpacing only on its
  // NodePadding path, and syncBlockSizes would mirror whatever is left into blockHeight.
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

  ne::BeginNode(ne::NodeId(make_node_id(node.id)));
  const ImVec2 nodeOriginScreen = ImGui::GetCursorScreenPos();

  // The header is draw-list only, so a Dummy of the same height is submitted below as a real widget:
  // without it ne's Header region collapses to a sliver at the top and only that sliver is
  // hit-testable, leaving the header undraggable.
  ImDrawList *dl = ImGui::GetWindowDrawList();
  constexpr float HEADER_INSET = 1.0f; // keeps the gray fill clear of the 2px outline corners.

  // Tint behind the gray fill so the HEADER_INSET strip shows the body color.
  dl->AddRectFilled(nodeOriginScreen, ImVec2(nodeOriginScreen.x + widthClamp, nodeOriginScreen.y + headerH), bgColor, 4.0f,
    ImDrawFlags_RoundCornersTop);

  dl->AddRectFilled(ImVec2(nodeOriginScreen.x + HEADER_INSET, nodeOriginScreen.y + HEADER_INSET),
    ImVec2(nodeOriginScreen.x + widthClamp - HEADER_INSET, nodeOriginScreen.y + headerH), COMMENT_BG_COLOR, 4.0f,
    ImDrawFlags_RoundCornersTop);

  // GroupBorder is pushed transparent above, so this is the only outline; drawing it on the content
  // channel keeps it under any child node.
  const ImVec2 blockMax(nodeOriginScreen.x + widthClamp, nodeOriginScreen.y + heightClamp);
  stroke_inside(dl, nodeOriginScreen, blockMax, 0.0f, 2.0f, 4.0f, BLOCK_BORDER_COLOR);

  // After the fills and outline, which would paint over the band, and before the caption, which keeps
  // its pixels. A block reserves no margin, so the band lands on the outline and the header padding.
  if (selected)
  {
    draw_node_select_band(frame, dl, nodeOriginScreen, blockMax);
  }

  add_canvas_text(dl, *frame.font, captionFontSize, ImVec2(nodeOriginScreen.x + blockPad, nodeOriginScreen.y + blockPad),
    COMMENT_TEXT_COLOR, text);

  ImGui::Dummy(ImVec2(widthClamp, headerH));

  ne::Group(groupSize);

  // The 1px width is deliberate: a wider Dummy would pin m_Bounds.Max.x, so a user width-shrink
  // would never reach GraphData. The 1px height puts m_Bounds past m_GroupBounds, which stops ne
  // treating the node as its own child and recursing until the stack overflows.
  ImGui::Dummy(ImVec2(BLOCK_CONTAINMENT_GAP, BLOCK_CONTAINMENT_GAP));

  ne::EndNode();

  ImGui::PopStyleVar();
}

void draw_graph_node(GraphNodeFrame &frame, const GraphData::Node &node, int node_index, bool selected, bool reduced)
{
  // Pin roles and types were resolved against the descriptor at load time (resolve_node_pins).
  eastl::fixed_vector<int, 16, true> inputIdx;
  eastl::fixed_vector<int, 16, true> outputIdx;
  // Inputs and separators interleaved in descriptor order. Outputs are a separate stream that
  // separators do not touch, so the two columns stay independently indexed from the top.
  eastl::fixed_vector<int, 24, true> leftItems;
  for (int i = 0; i < static_cast<int>(node.pins.size()); ++i)
  {
    const GraphData::Pin &p = node.pins[i];
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
      if (node.pins[i].separator)
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

  const char *const titleStr = node.descName.empty() ? NODE_UNNAMED_TITLE : node.descName.c_str();

  NodeLayout lay;
  // Only the inherited path is clamped, so a hand-edited absolute size can be far wider than 8192.
  char footerLeft[48] = {0};
  char footerRight[48] = {0};
  // constraint: layout-only, so it stays a pure function of the node - a missing derived cache may
  // not change node height.
  const int footerDimension = node_out_texture_dimension(node);
  lay.hasFooter = footerDimension != 0;

  lay.rowCount = static_cast<int>(rows.size());
  lay.rowPitch = frame.rowPitch;
  lay.headerH = frame.headerHeight;
  lay.footerH = lay.hasFooter ? frame.footerHeight : 0.0f;
  lay.contentH = lay.rowCount > 0 ? 2.0f * frame.nodePad + lay.rowCount * lay.rowPitch : 0.0f;
  lay.hasDivider = !inputIdx.empty() && !outputIdx.empty();

  lay.h = ImCeil(lay.headerH + lay.contentH + lay.footerH);

  const uint64_t neNodeId = make_node_id(node.id);
  const NodeVisual vis = resolve_node_visual(selected, frame.previewNodeId == node.id, frame.hoveredNeNodeId == neNodeId, reduced);

  NeStyleScope nodeStyle;
  nodeStyle.color(ne::StyleColor_NodeBg, IM_COL32(0, 0, 0, 0));
  nodeStyle.color(ne::StyleColor_NodeBorder, IM_COL32(0, 0, 0, 0));
  nodeStyle.var(ne::StyleVar_NodePadding, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
  nodeStyle.var(ne::StyleVar_NodeBorderWidth, 0.0f);
  nodeStyle.var(ne::StyleVar_NodeRounding, frame.nodeRounding);

  ne::BeginNode(ne::NodeId(neNodeId));

  const ImVec2 bodyMin = ImGui::GetCursorScreenPos();
  ImGui::Dummy(ImVec2(frame.bodyWidth, lay.h));
  const ImVec2 bodyMax(bodyMin.x + frame.bodyWidth, bodyMin.y + lay.h);

  ImDrawList *dl = ImGui::GetWindowDrawList();
  if (vis.shadow)
  {
    draw_soft_shadow(dl, bodyMin, bodyMax, frame.nodeRounding);
  }

  const float contentTop = bodyMin.y + lay.headerH;
  const float footerTop = contentTop + lay.contentH;
  dl->AddRectFilled(bodyMin, bodyMax, vis.content, frame.nodeRounding);
  dl->AddRectFilled(bodyMin, ImVec2(bodyMax.x, contentTop), vis.header, frame.nodeRounding, ImDrawFlags_RoundCornersTop);
  if (lay.hasFooter)
  {
    dl->AddRectFilled(ImVec2(bodyMin.x, footerTop), bodyMax, vis.footer, frame.nodeRounding, ImDrawFlags_RoundCornersBottom);
  }

  stroke_inside(dl, bodyMin, bodyMax, 0.0f, frame.nodeBorderWidth, frame.nodeRounding, NODE_BORDER_COLOR);

  if (vis.band)
  {
    draw_node_select_band(frame, dl, bodyMin, bodyMax);
  }

  if (lay.hasDivider && lay.contentH > 0.0f)
  {
    const float dividerX = bodyMin.x + frame.dividerDx;
    dl->AddLine(ImVec2(dividerX, contentTop + frame.nodePad), ImVec2(dividerX, footerTop - frame.nodePad), NODE_DIVIDER_COLOR,
      frame.dividerWidth);
  }

  // Only the hovered node can own the tooltip, so the rect tests below never run for the rest.
  const bool nodeHovered = frame.hoveredNeNodeId == neNodeId;

  auto offerFullText = [&](bool cut, const ImVec2 &rect_min, const ImVec2 &rect_max, const char *full) {
    if (cut && nodeHovered && ImGui::IsMouseHoveringRect(rect_min, rect_max, false))
    {
      frame.hoverTooltip = full;
    }
  };

  const float rowsTop = contentTop + frame.nodePad;
  const float inputColX = bodyMin.x + frame.contentPadX;
  const float contentRightX = bodyMax.x - frame.contentPadX;
  const float inputColRightX =
    lay.hasDivider ? (bodyMin.x + frame.dividerDx - frame.dividerWidth * 0.5f - frame.inputColPadR) : contentRightX;
  const float outputColLeftX =
    lay.hasDivider ? (bodyMin.x + frame.dividerDx + frame.dividerWidth * 0.5f + frame.outputColPadL) : inputColX;

  if (!reduced)
  {
    float titleRightX = contentRightX;
    if (is_subgraph_instance(node))
    {
      const float iconSize = frame.headerIconSize;
      const ImVec2 iconMin(contentRightX - iconSize, bodyMin.y + (lay.headerH - iconSize) * 0.5f);
      const ImVec2 iconMax(iconMin.x + iconSize, iconMin.y + iconSize);
      ImGui::PushID(node.id);
      const bool held = ImGui::GetActiveID() == ImGui::GetID("subgraph_open"); // ne reports no hover while it is active
      // Only on the node ne found under the cursor, or one below would take the click through the one on
      // top; and not during a reroute, which ends on the release this click would share.
      if ((nodeHovered || held) && !frame.edgeReconnect.isActive())
      {
        // Submitted before ne's own hit items, so it takes the press and the node neither drags nor reselects.
        ImGui::SetCursorScreenPos(iconMin);
        if (ImGui::InvisibleButton("subgraph_open", ImVec2(iconSize, iconSize)))
        {
          frame.clickedSubgraphNodeId = node.id;
        }
        if (ImGui::IsItemHovered())
        {
          dl->AddRectFilled(iconMin, iconMax, NODE_HEADER_ICON_HOVER_COLOR, frame.nodeRounding);
          frame.hoverTooltip = "Click to open subgraph";
        }
      }
      ImGui::PopID();
      if (frame.subgraphIcon)
      {
        dl->AddImage(frame.subgraphIcon, iconMin, iconMax);
      }
      titleRightX = iconMin.x - frame.headerIconGap;
    }

    const float titleX = bodyMin.x + frame.titleDx;
    eastl::string shortTitle;
    const bool titleCut =
      ellipsize_canvas_text(frame.font, frame.titleFontSize, titleStr, titleRightX - titleX, shortTitle, frame.titleEllipsisW);
    add_canvas_text(dl, *frame.font, frame.titleFontSize, ImVec2(titleX, bodyMin.y + (lay.headerH - frame.titleFontSize) * 0.5f),
      NODE_TITLE_TEXT_COLOR, titleCut ? shortTitle.c_str() : titleStr);
    offerFullText(titleCut, ImVec2(titleX, bodyMin.y), ImVec2(titleRightX, contentTop), titleStr);
    if (lay.hasFooter)
    {
      // constraint: the band is laid out from this node's own pins, so the format comes from
      // footerDimension and not from whatever produced the resolution.
      build_footer_texts(frame.graph, frame.resolutions.byIndex(node_index), footerDimension, footerLeft, sizeof(footerLeft),
        footerRight, sizeof(footerRight));
      const float footerTextY = footerTop + (lay.footerH - frame.footerFontSize) * 0.5f;
      const float footerLeftX = bodyMin.x + frame.contentPadX;
      eastl::string shortRight;
      float rightW = 0.0f;
      const bool rightCut = ellipsize_canvas_text(frame.font, frame.footerFontSize, footerRight, contentRightX - footerLeftX,
        shortRight, frame.footerEllipsisW, &rightW);
      const char *const rightStr = rightCut ? shortRight.c_str() : footerRight;
      const float footerRightX = contentRightX - rightW;
      const float footerSplitX = eastl::max(footerLeftX, footerRightX - frame.footerMinGap);
      const float footerBottom = footerTop + lay.footerH;
      const float leftBudget = footerSplitX - footerLeftX;
      eastl::string shortLeft;
      float leftW = 0.0f;
      const bool leftCut =
        ellipsize_canvas_text(frame.font, frame.footerFontSize, footerLeft, leftBudget, shortLeft, frame.footerEllipsisW, &leftW);
      const char *const leftStr = leftCut ? shortLeft.c_str() : footerLeft;
      // Ellipsizing bottoms out at the width of "..." itself, so a narrower budget still overflows.
      // Gated because a clip rect splits the draw batch and the ellipsis normally fits.
      const bool clipLeft = leftW > leftBudget;
      if (clipLeft)
      {
        dl->PushClipRect(ImVec2(footerLeftX, footerTop), ImVec2(footerSplitX, footerBottom), true);
      }
      add_canvas_text(dl, *frame.font, frame.footerFontSize, ImVec2(footerLeftX, footerTextY), NODE_FOOTER_TEXT_COLOR, leftStr);
      if (clipLeft)
      {
        dl->PopClipRect();
      }
      add_canvas_text(dl, *frame.font, frame.footerFontSize, ImVec2(footerRightX, footerTextY), NODE_FOOTER_TEXT_COLOR, rightStr);
      // Each footer text is hit-tested over its own half.
      offerFullText(leftCut, ImVec2(footerLeftX, footerTop), ImVec2(footerSplitX, footerBottom), footerLeft);
      offerFullText(rightCut, ImVec2(footerRightX, footerTop), ImVec2(contentRightX, footerBottom), footerRight);
    }
  }

  const float inputPinCursorX = bodyMin.x - frame.pinDotSize * 0.5f;
  const float outputPinCursorX = bodyMax.x - frame.pinDotSize * 0.5f;

  for (int row = 0; row < lay.rowCount; ++row)
  {
    const float rowTop = rowsTop + row * lay.rowPitch;
    const float pinBoxY = rowTop + (lay.rowPitch - frame.pinBoxHeight) * 0.5f;
    const float labelY = rowTop + (lay.rowPitch - frame.rowFontSize) * 0.5f;

    if (rows[row].separatorPin >= 0)
    {
      // No pin is declared for a divider, so link drag and hover cannot reach it.
      const float ruleY = rowTop + lay.rowPitch * 0.5f;
      const float ruleLeft = bodyMin.x + frame.contentPadX;
      // A separator divides the input list only, so its rule stops at the input column and never
      // reaches the output side.
      const float ruleRight = inputColRightX;
      const eastl::string &label = node.pins[rows[row].separatorPin].name;
      // The rule keeps its stubs whatever the label is: a too-wide label is ellipsized, not honoured.
      const float labelMaxW = eastl::max(0.0f, ruleRight - ruleLeft - 2.0f * (frame.separatorMinRule + frame.separatorGap));
      eastl::string shortLabel;
      // An empty label short-circuits and leaves the width at 0, which the branch below reads.
      float shownW = 0.0f;
      const bool cut = !label.empty() && ellipsize_canvas_text(frame.font, frame.rowFontSize, label.c_str(), labelMaxW, shortLabel,
                                           frame.rowEllipsisW, &shownW);
      const char *const shown = cut ? shortLabel.c_str() : label.c_str();
      if (shownW <= 0.0f)
      {
        dl->AddLine(ImVec2(ruleLeft, ruleY), ImVec2(ruleRight, ruleY), NODE_DIVIDER_COLOR, frame.dividerWidth);
      }
      else
      {
        const float centerX = (ruleLeft + ruleRight) * 0.5f;
        const float half = shownW * 0.5f + frame.separatorGap;
        dl->AddLine(ImVec2(ruleLeft, ruleY), ImVec2(centerX - half, ruleY), NODE_DIVIDER_COLOR, frame.dividerWidth);
        dl->AddLine(ImVec2(centerX + half, ruleY), ImVec2(ruleRight, ruleY), NODE_DIVIDER_COLOR, frame.dividerWidth);
        if (!reduced)
        {
          add_canvas_text(dl, *frame.font, frame.rowFontSize, ImVec2(centerX - (half - frame.separatorGap), labelY),
            NODE_ROW_TEXT_COLOR, shown);
          offerFullText(cut, ImVec2(ruleLeft, rowTop), ImVec2(ruleRight, rowTop + lay.rowPitch), label.c_str());
        }
      }
    }
    else if (rows[row].inputPin >= 0)
    {
      const int i = rows[row].inputPin;
      const GraphData::Pin &p = node.pins[i];
      ImGui::SetCursorScreenPos(ImVec2(inputPinCursorX, pinBoxY));
      const uint64_t pinKey = make_pin_id(node.id, i);
      const ImVec2 pinCenter = draw_pin_dot(frame, ne::PinId(pinKey), ne::PinKind::Input, p.type,
        frame.linkedPins.find(pinKey) != frame.linkedPins.end(), frame.livePins.find(pinKey) != frame.livePins.end(), !reduced,
        frame.pinJumpMenu.isSourcePin(node.id, i) || frame.highlightPinId == pinKey);
      if (frame.edgeReconnect.isActive() && frame.edgeReconnect.anchorNode() == node.id && frame.edgeReconnect.anchorPin() == i)
      {
        frame.edgeReconnect.setAnchorScreenPos(pinCenter.x, pinCenter.y);
      }
      if (!reduced)
      {
        if (!p.name.empty())
        {
          eastl::string shortName;
          const bool cut = ellipsize_canvas_text(frame.font, frame.rowFontSize, p.name.c_str(), inputColRightX - inputColX, shortName,
            frame.rowEllipsisW);
          add_canvas_text(dl, *frame.font, frame.rowFontSize, ImVec2(inputColX, labelY), NODE_ROW_TEXT_COLOR,
            cut ? shortName.c_str() : p.name.c_str());
          offerFullText(cut, ImVec2(inputColX, rowTop), ImVec2(inputColRightX, rowTop + lay.rowPitch), p.name.c_str());
        }
        if (!p.comment.empty())
        {
          draw_pin_comment_outside(frame, bodyMin, bodyMax, pinCenter, /*is_input=*/true, p.comment);
        }
      }
    }
    if (rows[row].outputPin >= 0)
    {
      const int i = rows[row].outputPin;
      const GraphData::Pin &p = node.pins[i];
      ImGui::SetCursorScreenPos(ImVec2(outputPinCursorX, pinBoxY));
      const uint64_t pinKey = make_pin_id(node.id, i);
      const ImVec2 pinCenter = draw_pin_dot(frame, ne::PinId(pinKey), ne::PinKind::Output, p.type,
        frame.linkedPins.find(pinKey) != frame.linkedPins.end(), frame.livePins.find(pinKey) != frame.livePins.end(), !reduced,
        frame.pinJumpMenu.isSourcePin(node.id, i) || frame.highlightPinId == pinKey);
      if (frame.edgeReconnect.isActive() && frame.edgeReconnect.anchorNode() == node.id && frame.edgeReconnect.anchorPin() == i)
      {
        frame.edgeReconnect.setAnchorScreenPos(pinCenter.x, pinCenter.y);
      }
      if (!reduced)
      {
        if (!p.name.empty())
        {
          eastl::string shortName;
          float shownW = 0.0f;
          const bool cut = ellipsize_canvas_text(frame.font, frame.rowFontSize, p.name.c_str(), contentRightX - outputColLeftX,
            shortName, frame.rowEllipsisW, &shownW);
          const char *const shown = cut ? shortName.c_str() : p.name.c_str();
          // Right-aligned, and ellipsizing already knows how wide the drawn string is.
          const float labelX = contentRightX - shownW;
          add_canvas_text(dl, *frame.font, frame.rowFontSize, ImVec2(labelX, labelY), NODE_ROW_TEXT_COLOR, shown);
          offerFullText(cut, ImVec2(outputColLeftX, rowTop), ImVec2(contentRightX, rowTop + lay.rowPitch), p.name.c_str());
        }
        if (!p.comment.empty())
        {
          draw_pin_comment_outside(frame, bodyMin, bodyMax, pinCenter, /*is_input=*/false, p.comment);
        }
      }
    }
  }

  ne::EndNode();
}

void draw_ghost_frame(ImDrawList *draw_list, const ImVec2 &anchor, bool has_input_pins, bool has_output_pins, const ImVec2 &bounds_min,
  const ImVec2 &bounds_max, ImVec2 &out_box_min, ImVec2 &out_box_max, ImVec2 &out_in_pin, ImVec2 &out_out_pin)
{
  const float rounding = static_cast<float>(hdpi::_pxS(GHOST_CORNER_RADIUS));
  const float notchDepth = static_cast<float>(hdpi::_pxS(GHOST_NOTCH_DEPTH));
  const float notchInset = static_cast<float>(hdpi::_pxS(GHOST_NOTCH_INSET));
  const float pinRadius = static_cast<float>(hdpi::_pxS(PIN_DOT_SIZE)) * 0.5f;
  const float pinDx = static_cast<float>(hdpi::_pxS(GHOST_PIN_DX));
  const float pinOutline = static_cast<float>(eastl::max(1, hdpi::_pxS(1)));
  const float width = static_cast<float>(hdpi::_pxS(GHOST_WIDTH));
  const float height = static_cast<float>(hdpi::_pxS(GHOST_HEIGHT));

  // Only a side that carries a pin reserves the dot's room.
  const float pinReach = pinDx + pinRadius;
  const float leftReach = has_input_pins ? pinReach : 0.0f;
  const float rightReach = has_output_pins ? pinReach : 0.0f;
  out_box_min = ImVec2(eastl::max(eastl::min(anchor.x, bounds_max.x - width - rightReach), bounds_min.x + leftReach),
    eastl::max(eastl::min(anchor.y, bounds_max.y - height), bounds_min.y));
  out_box_max = ImVec2(out_box_min.x + width, out_box_min.y + height);

  fill_ghost_body(draw_list, out_box_min, out_box_max, rounding, notchDepth, notchInset);

  const float pinY = (out_box_min.y + out_box_max.y) * 0.5f;
  out_in_pin = ImVec2(out_box_min.x - pinDx, pinY);
  out_out_pin = ImVec2(out_box_max.x + pinDx, pinY);
  auto drawPin = [draw_list, pinRadius, pinOutline](const ImVec2 &center) {
    draw_list->AddCircleFilled(center, pinRadius, PIN_OUTLINE_COLOR);
    draw_list->AddCircleFilled(center, pinRadius - pinOutline, GHOST_PIN_COLOR);
  };
  if (has_input_pins)
  {
    drawPin(out_in_pin);
  }
  if (has_output_pins)
  {
    drawPin(out_out_pin);
  }
}

void draw_node_drag_ghost(ImDrawList *draw_list, const char *node_name, bool has_input_pins, bool has_output_pins,
  const ImVec2 &anchor, const ImVec2 &bounds_min, const ImVec2 &bounds_max)
{
  ImVec2 boxMin, boxMax, inPin, outPin;
  draw_ghost_frame(draw_list, anchor, has_input_pins, has_output_pins, bounds_min, bounds_max, boxMin, boxMax, inPin, outPin);

  ImFont *const font = ImGui::GetFont();
  const float fontSize = static_cast<float>(hdpi::_pxS(NODE_TITLE_FONT_SIZE));
  const float pad = static_cast<float>(hdpi::_pxS(GHOST_TEXT_PAD));
  const float width = boxMax.x - boxMin.x;
  const float height = boxMax.y - boxMin.y;

  const char *const title = (node_name && node_name[0]) ? node_name : NODE_UNNAMED_TITLE;
  const int maxLines = eastl::max(1, static_cast<int>((height - 2.0f * pad) / fontSize));
  GhostTextLines lines;
  wrap_text_lines(font, fontSize, title, width - 2.0f * pad, maxLines, font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, NODE_ELLIPSIS).x,
    lines);

  float lineY = boxMin.y + (height - fontSize * static_cast<float>(lines.size())) * 0.5f;
  for (const eastl::string &line : lines)
  {
    const float lineWidth = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, line.c_str()).x;
    draw_list->AddText(font, fontSize, ImVec2(boxMin.x + (width - lineWidth) * 0.5f, lineY), NODE_TITLE_TEXT_COLOR, line.c_str());
    lineY += fontSize;
  }
}

float pin_hit_radius() { return static_cast<float>(hdpi::_pxS(PIN_DOT_SIZE)); }

void add_node_ghost_body_box(bool source_is_output, ImVec2 &out_min, ImVec2 &out_max)
{
  const int dx = source_is_output ? GHOST_PIN_DX : -(GHOST_PIN_DX + GHOST_WIDTH);
  out_min = ImVec2(static_cast<float>(hdpi::_pxS(dx)), static_cast<float>(hdpi::_pxS(-GHOST_HEIGHT / 2)));
  out_max = ImVec2(out_min.x + static_cast<float>(hdpi::_pxS(GHOST_WIDTH)), out_min.y + static_cast<float>(hdpi::_pxS(GHOST_HEIGHT)));
}

ImVec2 clamp_add_node_drop_point(bool source_is_output, const ImVec2 &drop_canvas, const ImVec2 &canvas_min, const ImVec2 &canvas_max)
{
  // The body hangs to one side of the drop, so each bound leaves only the room that side needs.
  ImVec2 ghostMin, ghostMax;
  add_node_ghost_body_box(source_is_output, ghostMin, ghostMax);
  const float roomLeft = eastl::max(0.0f, -ghostMin.x);
  const float roomRight = eastl::max(0.0f, ghostMax.x);
  const float roomUp = eastl::max(0.0f, -ghostMin.y);
  const float roomDown = eastl::max(0.0f, ghostMax.y);

  ImVec2 dropScreen = ne::CanvasToScreen(drop_canvas);
  dropScreen.x = ImClamp(dropScreen.x, canvas_min.x + roomLeft, eastl::max(canvas_min.x + roomLeft, canvas_max.x - roomRight));
  dropScreen.y = ImClamp(dropScreen.y, canvas_min.y + roomUp, eastl::max(canvas_min.y + roomUp, canvas_max.y - roomDown));
  return ne::ScreenToCanvas(dropScreen);
}

void draw_canvas_hint(ImDrawList *draw_list, const ImVec2 &avoid_min, const ImVec2 &avoid_max, const char *text,
  const ImVec2 &bounds_min, const ImVec2 &bounds_max)
{
  const float padX = static_cast<float>(hdpi::_pxS(CANVAS_HINT_PAD_X));
  const float padY = static_cast<float>(hdpi::_pxS(CANVAS_HINT_PAD_Y));
  const float rounding = static_cast<float>(hdpi::_pxS(GHOST_CORNER_RADIUS));
  const float gap = static_cast<float>(hdpi::_pxS(CANVAS_HINT_GAP));
  const float border = static_cast<float>(eastl::max(1, hdpi::_pxS(1)));
  const ImVec2 textSize = ImGui::CalcTextSize(text);
  const ImVec2 size(textSize.x + padX * 2.0f, textSize.y + padY * 2.0f);

  // Centred on the box and outside it on whichever side has the room; the clamps below are only for
  // a canvas with room on no side at all.
  ImVec2 boxMin(avoid_max.x + gap, (avoid_min.y + avoid_max.y - size.y) * 0.5f);
  if (boxMin.x + size.x > bounds_max.x)
  {
    boxMin.x = avoid_min.x - gap - size.x;
  }
  if (boxMin.x < bounds_min.x)
  {
    // Neither side fits, so take the one direction a single line always has room in.
    boxMin.x = avoid_min.x;
    boxMin.y = avoid_max.y + gap + size.y <= bounds_max.y ? avoid_max.y + gap : avoid_min.y - gap - size.y;
  }
  boxMin.x = eastl::max(bounds_min.x, eastl::min(boxMin.x, bounds_max.x - size.x));
  boxMin.y = eastl::max(bounds_min.y, eastl::min(boxMin.y, bounds_max.y - size.y));
  const ImVec2 boxMax(boxMin.x + size.x, boxMin.y + size.y);

  draw_list->AddRectFilled(boxMin, boxMax, GRAPH_POPUP_BG_COLOR, rounding);
  draw_list->AddRect(boxMin, boxMax, GRAPH_POPUP_BORDER_COLOR, rounding, ImDrawFlags_None, border);
  draw_list->AddText(ImVec2(boxMin.x + padX, boxMin.y + padY), GRAPH_TEXT_COLOR, text);
}

void draw_add_node_ghost(ImDrawList *draw_list, const ImVec2 &anchor, ImU32 pin_color, bool source_is_output, const ImVec2 &bounds_min,
  const ImVec2 &bounds_max)
{
  const float rounding = static_cast<float>(hdpi::_pxS(GHOST_CORNER_RADIUS));
  const float notchDepth = static_cast<float>(hdpi::_pxS(GHOST_NOTCH_DEPTH));
  const float notchInset = static_cast<float>(hdpi::_pxS(GHOST_NOTCH_INSET));
  const float pinRadius = static_cast<float>(hdpi::_pxS(PIN_DOT_SIZE)) * 0.5f;
  const float pinOutline = static_cast<float>(eastl::max(1, hdpi::_pxS(1)));
  // Clipped, not clamped: the body has to stay attached to the anchor dot.
  draw_list->PushClipRect(bounds_min, bounds_max, true);

  ImVec2 bodyMin, bodyMax;
  add_node_ghost_body_box(source_is_output, bodyMin, bodyMax);
  fill_ghost_body(draw_list, ImVec2(anchor.x + bodyMin.x, anchor.y + bodyMin.y), ImVec2(anchor.x + bodyMax.x, anchor.y + bodyMax.y),
    rounding, notchDepth, notchInset);

  draw_list->AddCircleFilled(anchor, pinRadius, PIN_OUTLINE_COLOR);
  draw_list->AddCircleFilled(anchor, pinRadius - pinOutline, pin_color);

  draw_list->PopClipRect();
}
