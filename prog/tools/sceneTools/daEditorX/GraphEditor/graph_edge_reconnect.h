// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <graphEditor/graph_data.h>

#include <stdint.h>

struct ImDrawList;
struct ImVec2;

// "Modify edge" (the A shortcut) interaction state machine, ported from graphEditor.js
// modifyEdgeUnderCursor. The caller picks the edge and deletes it; this anchors the edge's
// opposite end and lets the user re-route it to a new pin with a rubber-band preview.
class GraphEdgeReconnect
{
public:
  bool isActive() const { return active; }
  int anchorNode() const { return anchorNodeId; }
  int anchorPin() const { return anchorPinIndex; }

  // Anchors the opposite end of `edge_id` as seen from (detach_node, detach_pin) and starts the gesture;
  // the caller removes the edge. False -- and nothing started -- if any of that does not hold, which
  // includes an anchor pin the renderer never submits: no target could validate against it.
  bool beginForEdge(const GraphData &gd, int edge_id, int detach_node, int detach_pin);

  // Anchor pin's current on-screen centre, fed from the pin render pass (it moves with pan/zoom).
  void setAnchorScreenPos(float x, float y)
  {
    anchorScreenX = x;
    anchorScreenY = y;
    haveAnchorScreenPos = true;
  }

  // Rubber-band preview from the anchor pin to the cursor (no-op until the anchor position is known).
  void drawPreview(ImDrawList *draw_list, const ImVec2 &cursor, uint32_t color, float thickness) const;

  // If pin (node_id, pin_index) forms a valid edge with the anchor, fill out_edge (oriented
  // output -> input, id left unset for the caller to assign) and return true. Pure query: it does
  // not end the interaction -- the caller calls cancel() afterwards regardless of the result.
  bool tryComplete(const GraphData &gd, int node_id, int pin_index, GraphData::Edge &out_edge) const;

  void cancel() { reset(); }

private:
  void reset();

  bool active = false;
  int anchorNodeId = -1;
  int anchorPinIndex = -1;
  bool anchorIsOutput = false; // anchor pin's role, to orient the reconnection output -> input
  float anchorScreenX = 0.0f;
  float anchorScreenY = 0.0f;
  bool haveAnchorScreenPos = false;
};
