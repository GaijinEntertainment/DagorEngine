// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include <generic/dag_span.h>

#include <imgui/imgui.h>

class GraphEditorPlg;
class GraphTab;
class SubgraphWindow;
struct ImRect;

struct SubgraphWindowLayout
{
  ImVec2 offset = ImVec2(0.0f, 0.0f); // from the area's top-left
  ImVec2 size = ImVec2(0.0f, 0.0f);
  bool maximized = false; // offset and size keep the geometry for restore
  bool valid = false;     // placed on the first frame that knows the area
};

struct SubgraphWindowFrame
{
  ImVec2 areaMin = ImVec2(0.0f, 0.0f);
  ImVec2 areaMax = ImVec2(0.0f, 0.0f);
  ImVec2 canvasMax = ImVec2(0.0f, 0.0f);    // areaMax plus the hint bar strip, where a shadow may still reach
  bool active = false;                      // the active surface: framed
  bool failed = false;                      // on the chain the tab's compile failed to splice: red, when active
  eastl::vector<eastl::string> parentNames; // crumbs, the root first
};

struct SubgraphStackResult
{
  SubgraphWindow *shown = nullptr; // to bring back and focus
  SubgraphWindow *closed = nullptr;
};

// A subgraph file open in a window over a root tab's canvas, with a tab of its own.
class SubgraphWindow
{ // -V802 PVS sizes EASTL members as one byte; at their real sizes no member order is smaller
public:
  struct DrawResult
  {
    bool focused = false; // ImGui's, after the canvas pass
    bool closeClicked = false;
    bool minimizeClicked = false;
    bool saveClicked = false;
    bool saveAsClicked = false;
    int crumbClicked = -1; // into SubgraphWindowFrame::parentNames
  };

  // Builds the tab, which points back at this window. `last_layout` not valid: placed on the first draw.
  SubgraphWindow(GraphEditorPlg &plugin, int tab_id, GraphTab &root, SubgraphWindow *opened_from, const char *file_stem,
    const SubgraphWindowLayout &last_layout);
  ~SubgraphWindow();

  GraphTab &getTab() const { return *tab; }
  const eastl::string &getStem() const { return stem; }
  // Its graph went to another file: a Save as.
  void setStem(const char *file_stem) { stem = file_stem; }
  const SubgraphWindowLayout &getLayout() const { return layout; }
  // The window it was opened from; null: the root canvas.
  SubgraphWindow *getParent() const { return parent; }

  // On top, but a maximized window stays behind the others. ImGui's focus follows on the next draw.
  void raise(int focus_stamp)
  {
    focusStamp = focus_stamp;
    focusRequested = true;
  }
  int getFocusStamp() const { return focusStamp; }

  // Not drawn: a card in the stack stands for it, while its tab still ticks. The stamp, above 0, orders the cards.
  bool isMinimized() const { return minimizeStamp != 0; }
  int getMinimizeStamp() const { return minimizeStamp; }
  void minimize(int stamp) { minimizeStamp = stamp; }
  void unminimize();

  bool isPlaced() const { return layout.valid; }
  // Bottom-right of the area, stepped up-left past the windows already there.
  void placeDefault(const ImVec2 &area_size, const eastl::vector<eastl::unique_ptr<SubgraphWindow>> &windows);
  DrawResult draw(const SubgraphWindowFrame &frame);
  // Its card in the stack; restore and maximize set the layout, then ask to show it.
  void drawCard(ImDrawList &dl, const ImVec2 &min, bool hovered, SubgraphStackResult &result);

private:
  SubgraphWindowLayout fitted(const ImVec2 &area_size) const;
  void startGrab(int sides, const SubgraphWindowLayout &from);
  bool followGrab(const ImVec2 &delta, const ImVec2 &area_size);
  int drawCrumbs(ImDrawList &dl, const ImVec2 &min, float max_x, const SubgraphWindowFrame &frame, ImRect &name_rect) const;
  void drawFooter(ImDrawList &dl, const ImVec2 &min, const ImVec2 &max, DrawResult &result) const;
  bool drawCloseButton(ImDrawList &dl, const ImVec2 &min, float size) const;
  eastl::string shownName() const;
  void drawNameTooltip(const ImRect &name_rect, bool cut) const;

  SubgraphWindow *const parent;
  eastl::string stem; // of <stem>.subgraph.blk
  eastl::unique_ptr<GraphTab> tab;
  SubgraphWindowLayout layout;
  int focusStamp = 0;
  int minimizeStamp = 0;
  bool focusRequested = false;
  int grabSides = 0;
  ImGuiID grabId = 0;
  SubgraphWindowLayout grabStart;
};

// The minimized windows of a canvas, in the order given, wait in a stack in the hint bar row. While the pointer is
// on the stack, a card for each shows above it, in rows fitted to the canvas width. `cards_shown` keeps them up.
float subgraph_stack_width(int minimized_count);
SubgraphStackResult draw_subgraph_stack(const ImRect &stack, const ImVec2 &canvas_min, const ImVec2 &canvas_max,
  dag::ConstSpan<SubgraphWindow *> minimized, bool &cards_shown);

// The design's frame of the focused surface, a window or the canvas: red instead of yellow when `failed`.
void draw_focus_frame(ImDrawList &dl, const ImVec2 &min, const ImVec2 &max, float rounding, bool failed);
