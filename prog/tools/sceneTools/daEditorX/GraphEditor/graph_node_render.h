// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/string.h>

#include <imgui/imgui.h>

#include <graphEditor/graph_data.h>

#include "graph_resolutions.h"

#include <stdint.h>

class GraphEdgeReconnect;
class GraphPinJumpMenu;

// Every node metric derived from the hdpi scale and the canvas font, computed once per frame instead
// of once per node. Construct inside the ImGui frame and use it for that frame only.
struct GraphNodeFrame
{
  GraphNodeFrame(const GraphData &graph_data, const eastl::hash_set<uint64_t> &linked_pins, const eastl::hash_set<uint64_t> &live_pins,
    const GraphResolutions &node_resolutions, const GraphPinJumpMenu &pin_jump_menu, GraphEdgeReconnect &edge_reconnect,
    eastl::string &hover_tooltip, uint64_t hovered_ne_node_id, int preview_node_id, uint64_t highlight_pin_id);

  const GraphData &graph;
  // Pins touched by any edge, and the subset touched by a live one, as make_pin_id keys.
  const eastl::hash_set<uint64_t> &linkedPins;
  const eastl::hash_set<uint64_t> &livePins;
  // What each node produces, parallel to graph.nodes. GraphTab keeps it current.
  const GraphResolutions &resolutions;
  const GraphPinJumpMenu &pinJumpMenu;
  GraphEdgeReconnect &edgeReconnect; // fed the anchor pin's screen centre as the pins are laid out
  eastl::string &hoverTooltip;       // out: an ellipsized label's full text, or the icon hint under the cursor

  const uint64_t hoveredNeNodeId;
  const int previewNodeId;
  // Pin to draw hovered whatever ne thinks: a node-library drag holds ActiveId so ne reports no
  // hover of its own, and the armed transit gesture marks its target the same way. 0 when none.
  const uint64_t highlightPinId;

  ImTextureID subgraphIcon = ImTextureID{};
  int clickedSubgraphNodeId = -1; // out: the instance whose header icon was clicked

  // LINEAR_METRICS_FONT_NAME, not the current font: every canvas text call has to use it, or its
  // advances move with the zoom. Falls back to the current font when the host registered none.
  ImFont *font = nullptr;
  float titleFontSize = 0.0f;
  float rowFontSize = 0.0f;
  float footerFontSize = 0.0f;
  // Width of the ellipsis at each font size, for every ellipsize_text call the node pass makes.
  float titleEllipsisW = 0.0f;
  float rowEllipsisW = 0.0f;
  float footerEllipsisW = 0.0f;
  float nodePad = 0.0f;
  float contentPadX = 0.0f;
  float titleDx = 0.0f;
  float headerIconSize = 0.0f;
  float headerIconGap = 0.0f;
  float nodeRounding = 0.0f;
  float dividerWidth = 0.0f;
  float dividerDx = 0.0f;
  float inputColPadR = 0.0f;
  float outputColPadL = 0.0f;
  float separatorGap = 0.0f;
  float separatorMinRule = 0.0f;
  float pinDotSize = 0.0f;
  float nodeBorderWidth = 0.0f;
  float pinBoxHeight = 0.0f;
  float headerHeight = 0.0f;
  float footerHeight = 0.0f;
  float rowPitch = 0.0f;
  float bodyWidth = 0.0f;
  float footerMinGap = 0.0f;
  float pinDotSizeHover = 0.0f;
  float pinHoverHaloSize = 0.0f;
  float pinOutline = 0.0f;
  float pinCommentEdgeGap = 0.0f;
  // Selection-band geometry: bandMargin is the outer stroke, plateWidth the fill inside it, and
  // bandReach the total the node body has to keep clear.
  float bandMargin = 0.0f;
  float plateWidth = 0.0f;
  float bandReach = 0.0f;
  float bandRadius = 0.0f;
  // Annotation nodes reserve this instead of bandReach, so the band still lands inside the node.
  float annotationPad = 0.0f;
};

// GraphTab::syncBlockSizes reads the resized bounds back out, so it reverses both: the same floor,
// and the gap subtracted from the height.
inline constexpr float MIN_BLOCK_SIZE = 200.0f;
inline constexpr float BLOCK_CONTAINMENT_GAP = 1.0f;

// The design's floating drop shadow. It covers the rect too, so draw the fill after it; the clip rect must reach past the rect.
void draw_soft_shadow(ImDrawList *dl, const ImVec2 &min, const ImVec2 &max, float rounding);

// Pick by node.descName ("comment" / "block" are annotations); all must run between ne::Begin and
// ne::End. reduced drops text and pin decoration but still declares pins, so links stay bound.
void draw_graph_node(GraphNodeFrame &frame, const GraphData::Node &node, int node_index, bool selected, bool reduced);
void draw_comment_node(const GraphNodeFrame &frame, const GraphData::Node &node, bool selected);
// applied_group_sizes memoizes the group size pushed per block id; see draw_block_node.
void draw_block_node(const GraphNodeFrame &frame, const GraphData::Node &node, bool selected,
  eastl::hash_map<int, ImVec2> &applied_group_sizes);

// Preview of the node a node-library drag would drop, anchored at its top-left and kept inside
// bounds. The one entry point here that runs OUTSIDE ne::Begin / ne::End: it is a cursor-following
// overlay, so it is drawn on a foreground list over every panel rather than into the canvas.
void draw_node_drag_ghost(ImDrawList *draw_list, const char *node_name, bool has_input_pins, bool has_output_pins,
  const ImVec2 &anchor, const ImVec2 &bounds_min, const ImVec2 &bounds_max);

// Canvas-space reach of a pin dot for a hit test, wider than the dot: a drag aims less precisely
// than a click, and the caller takes the nearest pin inside it.
float pin_hit_radius();

// The add-node ghost body relative to the link's loose end, in dpi-scaled SCREEN pixels: the ghost
// is a screen overlay, and the spawn, the picker and the view clamp all place off it in that space.
// The body sits on the side the data flows towards, so the sign of out_min.x follows the pin role.
void add_node_ghost_body_box(bool source_is_output, ImVec2 &out_min, ImVec2 &out_max);

// Keeps the ghost body anchored on a drop point inside the canvas: off-screen the node would spawn
// where the user never sees it, since the ghost is clipped rather than moved. Canvas in, canvas out.
ImVec2 clamp_add_node_drop_point(bool source_is_output, const ImVec2 &drop_canvas, const ImVec2 &canvas_min, const ImVec2 &canvas_max);

// The ghost body and its pin dots, anchored top-left at `anchor` and kept inside the bounds. Returns
// the box and the pin centres, so a caller can put a title in it or hang wires off the pins.
void draw_ghost_frame(ImDrawList *draw_list, const ImVec2 &anchor, bool has_input_pins, bool has_output_pins, const ImVec2 &bounds_min,
  const ImVec2 &bounds_max, ImVec2 &out_box_min, ImVec2 &out_box_max, ImVec2 &out_in_pin, ImVec2 &out_out_pin);

// One-line hint floating on the canvas, beside the box it describes: right, else left, else under or
// over it. Clamping a hint this much wider than its subject back inside would land it on the subject.
void draw_canvas_hint(ImDrawList *draw_list, const ImVec2 &avoid_min, const ImVec2 &avoid_max, const char *text,
  const ImVec2 &bounds_min, const ImVec2 &bounds_max);

// Placement preview for the node a link dropped on empty canvas will create; nameless because the
// type is not chosen yet. anchor is the link's loose end, in screen space, as above.
void draw_add_node_ghost(ImDrawList *draw_list, const ImVec2 &anchor, ImU32 pin_color, bool source_is_output, const ImVec2 &bounds_min,
  const ImVec2 &bounds_max);
