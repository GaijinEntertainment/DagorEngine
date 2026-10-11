// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/menu.h>

#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include <util/dag_string.h>

#include <graphEditor/graph_data.h>

struct GraphData;

// Right-click menus for the graph canvas: one per target (node, pin, edge, empty space), carrying the
// rows that target's state allows. Built on PropPanel's context menu, restyled to the design.
class GraphContextMenu : public PropPanel::IMenuEventHandler
{
public:
  enum class Action
  {
    None,
    AddNode,
    AddNodeAtEdge,
    AddTransitNode,
    AddTransitNodeAtEdge,
    CreateSubgraph,
    OpenSubgraph,
    ExplodeSubgraph,
    Remove,
    RemoveKeepConnections,
    Duplicate,
    Copy,
    Cut,
    Paste,
    ToggleAutoUpdate,
    ForceRebuild,
    SelectAll,
    RemoveEdges,
    MuteEdges,
    UnmuteEdges,
    RerouteEdge,
    JumpToOppositePin,
  };

  struct Result
  {
    Action action = Action::None;
    // Every edge of the target for a top-level row, the one chosen edge for a submenu pick.
    eastl::vector<int> edges;
    // Re-route: the endpoint to detach. The edge's opposite end anchors the rubber band.
    int detachNode = -1;
    int detachPin = -1;
    int jumpNode = -1; // Jump: the destination pin
    int jumpPin = -1;
    int targetNode = -1; // the node or pin the menu was opened for, or an edge target's source pin
    int targetPin = -1;
  };

  // The popup anchors itself to the cursor, so these must be called on the frame the right-click was
  // reported.
  void openForNode(int node_id, int selected_node_count, bool can_paste, bool node_is_subgraph, bool subgraph_open);
  void openForPin(const GraphData &gd, int node_id, int pin_index);
  void openForEdges(const GraphData &gd, const eastl::vector<int> &edge_ids);
  void openForBackground(bool auto_update_on);

  // Standing, or still delivering the click it was closed on: updateImgui drops the menu only once
  // renderContextMenu reports both.
  bool isOpen() const { return menu != nullptr; }

  // Dropped without delivering anything, for a tab that stops being shown.
  void close()
  {
    menu.reset();
    picked = Result();
  }

  // Call between ne::Suspend() and ne::Resume(): the popup anchors to ImGui's mouse position, which
  // is only in screen space while the canvas is suspended.
  void updateImgui();

  // The row picked since the last call, or Action::None. PropPanel delivers a click between frames,
  // which is also what keeps the release that picked the row from ending a re-route it just started.
  Result takePicked();

private:
  int onMenuItemClick(unsigned id) override;

  // Reachable drops edges whose opposite pin the renderer never submits, so it has no position: the
  // rows that navigate to that pin or anchor a re-route on it need one.
  enum class EdgeFilter
  {
    Any,
    Reachable,
  };

  // Menu item id is the row's index in `entries` plus one, so 0 stays free for the rows that carry
  // nothing (labels, separators, submenu containers).
  struct Entry
  {
    Action action = Action::None;
    int edgeId = -1; // -1 = every edge of the target
    int detachNode = -1;
    int detachPin = -1;
    int jumpNode = -1;
    int jumpPin = -1;
  };

  // One edge of the pin target, labelled with the node it leads to.
  struct EdgeChoice
  {
    String label;
    String comment;
    Entry entry;
  };

  // has_checkable_rows reserves the checkmark gutter, which indents every label in the menu.
  void beginMenu(const char *title, bool has_checkable_rows = false);
  unsigned addRow(unsigned parent, const char *label, const char *shortcut, const Entry &entry, bool enabled = true);
  // Falls back to a plain row when there is no comment to show.
  unsigned addCommentRow(unsigned parent, const char *label, const char *comment, const Entry &entry);
  unsigned addSubMenuRow(unsigned parent, const char *label, const char *shortcut);

  void collectEdgeChoices(const GraphData &gd, const eastl::vector<PinEdge> &pin_edges, EdgeFilter filter, Action action,
    eastl::vector<EdgeChoice> &out) const;

  // A plain item when the filter leaves one edge, disabled when it leaves none, a submenu when it
  // leaves several. all_label adds the leading "... all" row, where acting on every edge makes sense.
  void addPinEdgeRow(const GraphData &gd, const eastl::vector<PinEdge> &pin_edges, const char *label, const char *shortcut,
    const char *all_label, Action action, EdgeFilter filter);

  eastl::unique_ptr<PropPanel::IMenu> menu;
  eastl::vector<Entry> entries;
  eastl::vector<int> targetEdges; // every edge the target touches, in submenu order
  int targetNode = -1;            // node target: the node; pin target: the pin's node; edge target: the edge's source pin
  int targetPin = -1;
  Result picked;
};
