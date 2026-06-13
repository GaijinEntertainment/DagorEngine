// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_edge_reconnect.h"

#include "graph_validation.h" // validate_new_edge

#include <imgui/imgui.h>

#include <math.h>

namespace
{
// TODO do not forget to replace this with color theme
constexpr ImU32 RECONNECT_LINE_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 0xFF);
constexpr float RECONNECT_LINE_THICKNESS = 2.0f;

const GraphData::Node *find_node(const GraphData &gd, int node_id)
{
  for (const GraphData::Node &n : gd.nodes)
  {
    if (n.id == node_id)
    {
      return &n;
    }
  }
  return nullptr;
}
} // namespace

int GraphEdgeReconnect::begin(const GraphData &gd, int node_id, int pin_index)
{
  if (active || node_id < 0 || pin_index < 0)
  {
    return -1;
  }

  for (int i = static_cast<int>(gd.edges.size()) - 1; i >= 0; --i)
  {
    const GraphData::Edge &e = gd.edges[i];
    int oppNode = -1;
    int oppPin = -1;
    if (e.elemA == node_id && e.pinA == pin_index)
    {
      oppNode = e.elemB;
      oppPin = e.pinB;
    }
    else if (e.elemB == node_id && e.pinB == pin_index)
    {
      oppNode = e.elemA;
      oppPin = e.pinA;
    }
    else
    {
      continue;
    }

    // The far end becomes the anchor; its role orients the eventual reconnection.
    const GraphData::Node *anchor = find_node(gd, oppNode);
    if (!anchor || oppPin < 0 || oppPin >= static_cast<int>(anchor->pins.size()))
    {
      return -1;
    }
    active = true;
    anchorNodeId = oppNode;
    anchorPinIndex = oppPin;
    anchorIsOutput = (anchor->pins[oppPin].role == PinRole::Out);
    haveAnchorScreenPos = false;
    return e.id;
  }

  return -1;
}

void GraphEdgeReconnect::drawPreview(ImDrawList *draw_list, const ImVec2 &cursor) const
{
  if (!active || !haveAnchorScreenPos || !draw_list)
  {
    return;
  }

  const ImVec2 from(anchorScreenX, anchorScreenY);
  // Horizontal tangents like an imgui-node-editor link: an output leaves to the right, an input
  // to the left, so the curve reads naturally regardless of where the cursor is.
  const float dirX = anchorIsOutput ? 1.0f : -1.0f;
  const float strength = fabsf(cursor.x - from.x) * 0.5f + 25.0f;
  const ImVec2 c1(from.x + dirX * strength, from.y);
  const ImVec2 c2(cursor.x - dirX * strength, cursor.y);
  draw_list->AddBezierCubic(from, c1, c2, cursor, RECONNECT_LINE_COLOR, RECONNECT_LINE_THICKNESS);
}

bool GraphEdgeReconnect::tryComplete(const GraphData &gd, int node_id, int pin_index, GraphData::Edge &out_edge) const
{
  if (!active || node_id < 0 || pin_index < 0)
  {
    return false;
  }

  // validate_new_edge expects (output, input) order, so try the orientation implied by the anchor's
  // role first, then the other way (covers Any/Ctrl anchor pins). validate_new_edge enforces the
  // role / type / cycle / single-connect rules.
  auto tryDir = [&](int out_node, int out_pin, int in_node, int in_pin) -> bool {
    if (!validate_new_edge(gd, out_node, out_pin, in_node, in_pin))
    {
      return false;
    }
    out_edge.elemA = out_node;
    out_edge.pinA = out_pin;
    out_edge.elemB = in_node;
    out_edge.pinB = in_pin;
    return true;
  };

  if (anchorIsOutput)
  {
    return tryDir(anchorNodeId, anchorPinIndex, node_id, pin_index) || tryDir(node_id, pin_index, anchorNodeId, anchorPinIndex);
  }
  return tryDir(node_id, pin_index, anchorNodeId, anchorPinIndex) || tryDir(anchorNodeId, anchorPinIndex, node_id, pin_index);
}

void GraphEdgeReconnect::reset()
{
  active = false;
  anchorNodeId = -1;
  anchorPinIndex = -1;
  anchorIsOutput = false;
  haveAnchorScreenPos = false;
}
