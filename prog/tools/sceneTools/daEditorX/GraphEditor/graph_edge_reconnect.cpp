// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_edge_reconnect.h"

#include "graph_edge_render.h" // draw_dangling_link
#include "graph_validation.h"  // validate_new_edge

#include <imgui/imgui.h>


bool GraphEdgeReconnect::beginForEdge(const GraphData &gd, int edge_id, int detach_node, int detach_pin)
{
  if (active || detach_node < 0 || detach_pin < 0)
  {
    return false;
  }

  const GraphData::Edge *const edge = find_edge_by_id(gd, edge_id);
  if (!edge)
  {
    return false;
  }

  int oppositeNode = -1;
  int oppositePin = -1;
  if (!edge_opposite_end(*edge, detach_node, detach_pin, oppositeNode, oppositePin))
  {
    return false;
  }

  // The opposite end becomes the anchor; its role orients the eventual reconnection.
  const GraphData::Node *anchor = find_node_by_id(gd, oppositeNode);
  if (!anchor || !is_pin_reachable(*anchor, oppositePin))
  {
    return false;
  }
  active = true;
  anchorNodeId = oppositeNode;
  anchorPinIndex = oppositePin;
  anchorIsOutput = (anchor->pins[oppositePin].role == PinRole::Out);
  haveAnchorScreenPos = false;
  return true;
}

void GraphEdgeReconnect::drawPreview(ImDrawList *draw_list, const ImVec2 &cursor, uint32_t color, float thickness) const
{
  if (!active || !haveAnchorScreenPos || !draw_list)
  {
    return;
  }

  draw_dangling_link(draw_list, ImVec2(anchorScreenX, anchorScreenY), cursor, anchorIsOutput, color, thickness);
}

bool GraphEdgeReconnect::tryComplete(const GraphData &gd, int node_id, int pin_index, GraphData::Edge &out_edge) const
{
  if (!active || node_id < 0 || pin_index < 0)
  {
    return false;
  }

  // validate_new_edge expects (output, input) order, so try the orientation implied by the anchor's
  // role first, then the other way (covers Any/Ctrl anchor pins). It checks role / type / cycle as
  // if a single-connect pin's existing edge were already gone; the document is what removes it.
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
