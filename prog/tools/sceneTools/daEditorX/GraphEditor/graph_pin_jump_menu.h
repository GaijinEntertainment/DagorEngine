// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <imgui/imgui.h>

#include <stdint.h>

struct GraphData;

class GraphPinJumpMenu
{
public:
  enum class Purpose
  {
    Jump,
    Reroute,
  };

  enum class Pick
  {
    None,       // no edge the view can reach from this pin
    Single,     // one, reported through out_choice; no menu opened
    MenuOpened, // several; the chooser is up and reports the pick through draw()
  };

  struct Choice
  {
    int edgeId = -1;
    int node = -1; // opposite end of the edge
    int pin = -1;
  };

  // Destinations the view cannot reach -- a dangling edge, or one landing on a pin the renderer never
  // submits -- are dropped before the count, so every outcome honours the same filter.
  Pick resolve(const GraphData &gd, Purpose menu_purpose, int node_id, int pin_index, uint64_t graph_revision, Choice &out_choice);

  bool isOpen() const { return menuOpen; }

  // The menu closes itself: on Escape, a click outside, and a fan-out that lost its choice. The
  // panel also closes it when the tab it belongs to stops being shown.
  void close();

  Purpose openedFor() const { return purpose; }

  // Pin the menu is choosing at, for the caller that has to re-read its position.
  int sourceNodeId() const { return sourceNode; }
  int sourcePinIndex() const { return sourcePin; }

  // Keeps the pin being chosen at decorated as hovered while the cursor sits over the menu.
  bool isSourcePin(int node_id, int pin_index) const { return menuOpen && node_id == sourceNode && pin_index == sourcePin; }

  // Edge to draw highlighted, or -1. It trails the keyboard and the mouse by one frame: links are
  // submitted before the menu draws.
  int highlightedEdgeId() const;

  // Call between ne::Suspend() and ne::Resume(). Returns true once a row is confirmed. pin_screen_pos is
  // re-read every frame: a navigate animation or a canvas resize moves the pin under a standing menu.
  bool draw(const GraphData &gd, uint64_t graph_revision, const ImVec2 &canvas_min, const ImVec2 &canvas_max,
    const ImVec2 &pin_screen_pos, Choice &out_choice);

private:
  struct Row
  {
    Choice choice;
    eastl::string label;
    eastl::string comment;
  };

  // Fills rows with the edges touching a pin whose opposite end the view can actually go to.
  void collectRows(const GraphData &gd, int node_id, int pin_index);

  eastl::vector<Row> rows;
  Purpose purpose = Purpose::Jump;
  int selected = 0;
  bool menuOpen = false;
  bool pendingPopupOpen = false;
  bool scrollToSelected = false;
  int sourceNode = -1;
  int sourcePin = -1;
  bool sourceIsOutput = false; // the menu grows away from that pin's edges so they stay visible
  uint64_t revision = 0;
};
