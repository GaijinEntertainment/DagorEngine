// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/propPanelService.h> // IconId

#include <util/dag_string.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <imgui/imgui.h>

struct GraphData;
class GraphEditorPlg;

// The node picker: type, press Enter, get that node wired to the pin it was raised from. Raised by a
// link dropped on empty canvas, by Space over a pin or an edge, and by those two context menus; the
// floating toolbar's Add node and Space over nothing raise it with no pin, and the node is added unconnected.
class GraphAddNodePopup
{
public:
  // Resolves the offerable set, so this is the one expensive pass over the library. The drop point
  // is clamped into the canvas, so the ghost anchored on it cannot land off-screen. Mind the two
  // spaces: drop_canvas_pos is a graph point, canvas_min/canvas_max the widget rect in screen px.
  // splice_into_connections puts the node between the pin and what it feeds rather than alongside;
  // anchor_edge narrows that to one wire, -1 takes the pin's lot. A source_node of -1 offers every node.
  void openAt(const GraphData &gd, GraphEditorPlg &plugin, int source_node, int source_pin, const ImVec2 &drop_canvas_pos,
    const ImVec2 &canvas_min, const ImVec2 &canvas_max, bool splice_into_connections, int anchor_edge);

  bool isOpen() const { return menuOpen; }
  int sourceNodeId() const { return sourceNode; }
  int sourcePinIndex() const { return sourcePin; }
  bool sourceIsOutput() const { return srcIsOutput; }
  bool splicesIntoConnections() const { return splice; }
  int anchorEdgeId() const { return anchorEdge; }
  const ImVec2 &dropCanvasPos() const { return dropCanvas; }

  void close();

  // Call between ne::Suspend() and ne::Resume(). True once a row is confirmed. Re-anchors from
  // dropCanvasPos every frame: ImGui only closes a popup on the left and right buttons, so a
  // middle-mouse pan or a wheel zoom can happen under it.
  bool draw(const GraphData &gd, const ImVec2 &canvas_min, const ImVec2 &canvas_max, eastl::string &out_template_uid);

private:
  struct Row
  {
    eastl::string uid;
    eastl::string name;
    eastl::string synonyms;
  };

  void rebuildRows();

  // Resolved once on open, so per-keystroke work is only the name match. Uids, never DataBlock
  // pointers: a reloadBaseNodes() while the popup stands would dangle them.
  eastl::vector<Row> offerable;
  eastl::vector<int> rows; // indices into offerable, so a keystroke re-filters without copying
  String searchText;
  bool searchInputFocused = false;
  bool focusRequested = false;
  bool scrollToSelected = false; // the rows child keeps its scroll, so a keyboard move has to ask
  int selected = 0;
  bool splice = false;
  int anchorEdge = -1;
  bool menuOpen = false;
  bool pendingPopupOpen = false;
  int sourceNode = -1;
  int sourcePin = -1;
  bool srcIsOutput = false;
  ImVec2 dropCanvas = ImVec2(0.0f, 0.0f);
  // Resolved once: a theme switch keeps the IconId and swaps the texture under it.
  PropPanel::IconId searchIcon{};
  PropPanel::IconId clearIcon{};
  bool iconsLoaded = false;
};
