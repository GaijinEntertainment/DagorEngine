// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/panelWindow.h>

#include <generic/dag_span.h>

#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include <imgui/imgui.h>

class GraphDocument;
class GraphEditorPlg;
class GraphTab;
class SubgraphWindow;
struct IGraphTexGenService;

// The "Graph" dock window and the strip of open graphs inside it. Owns every tab; one of them is
// the active one, the only one drawn. Its active surface, the tab or the subgraph window over it
// focused last, is the graph the plugin works on. The window comes and goes with the user's
// toggle; the tabs outlive it.
class GraphPanel final : public PropPanel::ControlEventHandler
{
public:
  explicit GraphPanel(GraphEditorPlg &plugin);
  ~GraphPanel() override;

  void createWindow();
  void destroyWindow();
  PropPanel::PanelWindowPropertyControl *getPanelWindow() const { return panelWindow; }

  GraphTab &activeSurface();
  GraphDocument &activeDocument();
  int activeTabId();
  bool hasTabs() const { return !tabs.empty(); }
  // The one way the focus moves between the canvas of the active tab and a window over it (null: the canvas), never
  // to a minimized window.
  void focusSurface(GraphTab &root, SubgraphWindow *window);

  // A new untitled tab, appended and activated.
  GraphTab &openNewTab();
  // Activates the tab already showing that file, or loads it into a new one. Null if the load fails.
  GraphTab *openGraphFile(const char *path);
  void activateTab(GraphTab &tab);
  // Used by the undo entries: the tab or window holding that document comes to the front, on the next tick.
  void activateTabOf(const GraphDocument &doc);
  // In a window over the root of `from`; a file already shown brings up its tab or window instead.
  void openSubgraph(GraphTab &from, const char *stem);
  // On a tab, or in a window over any tab.
  bool isSubgraphOpen(const char *stem) const;
  // The window `from` was opened from, else the root canvas, on the next tick.
  void focusParentSurface(const GraphTab &from);

  // Points the texgen service at the active tab, for graph swaps only; edits go through markGraphDirtyAndRegen.
  void rebindTexGen();
  // The texgen service for the one document it compiles and previews, the active tab's own; null for
  // any other document, a window's included.
  IGraphTexGenService *texGenServiceFor(const GraphDocument &doc) const;
  GraphTab &texGenTab() { return activeTab(); }

  // Every entry recorded while this plugin is current is owned by the tab it was recorded in,
  // so one tab's history can be dropped on its own. Stamped per frame because the engine sets
  // its own token on a plugin switch, and ownsUndoToken answers its claim query so undoing a
  // graph entry from another plugin still brings the user back here.
  void stampUndoOwner();
  void dropUndoOps();
  bool ownsUndoToken(const void *token) const;

  void updateImgui();
  void actObjects(float dt);
  // Only the active tab and its windows tick, so only their queues can raise anything.
  bool hasPendingActions() const;
  // Asks about every graph that holds unsaved edits. False when one of them is to be kept: the
  // user cancelled, or said save and the save did not happen.
  bool confirmCloseAllTabs();
  // Drops every graph, asking nothing. The caller has already asked, or nobody is there to ask.
  void closeAllTabs();
  // Over every tab, windows included: the descriptors they read are shared, and a compile result
  // belongs to the document it was started for, which is not always the active one.
  void invalidateDerivedState();
  void applyPendingCompileResults();
  int getSelectedNodeId() const;
  // True while a node-library drag hovers a drawn canvas that would take the drop.
  bool isBaseNodeDropTargetHot() const;

  // Canvas state, so they build the canvas if it is not up yet; the plugin asks with the window open.
  ImVec2 getLastCanvasCenter();
  int allocateNodeId();

  // Dropped while the window is closed: they queue work for the next render pass.
  void requestSelectAll();
  void requestDeselectAll();
  void requestInvertSelection();
  void requestFrameSelected();

private:
  // The root the strip shows. Not for the plugin, which works on the active surface.
  GraphTab &activeTab();
  void stampUndoOwner(GraphTab &owner);
  void drawTabStrip();
  // imgui_tab_ids holds the ImGui id each tab was drawn with this frame, parallel to tabs.
  void syncTabOrderFromBar(dag::ConstSpan<ImGuiID> imgui_tab_ids);
  // Appends without activating.
  GraphTab &createTab();
  // The one place a switch happens.
  void setActiveIndex(int index);
  // Asks before a modified graph goes: true to close it, false to keep it open -- the user said
  // cancel, or said save and the save did not happen.
  bool confirmClose(GraphTab &tab, const char *title);
  // Its windows too: closing a tab closes them.
  bool confirmCloseWithWindows(GraphTab &root);
  void closeTabNow(int tab_id);
  // Before the root canvas, so a press on a window focuses it first. Returns the focused one.
  SubgraphWindow *drawSubgraphWindows(GraphTab &root, const ImVec2 &canvas_min, const ImVec2 &canvas_max);
  // The stack at the right end of the hint bar row; returns its width, 0 without one.
  float drawMinimizedSubgraphWindows(GraphTab &root, const ImVec2 &canvas_min, const ImVec2 &canvas_max);
  void minimizeSubgraphWindow(GraphTab &root, SubgraphWindow &window);
  // The window and the windows opened from it, which close with it.
  bool confirmCloseSubgraphWindow(GraphTab &root, SubgraphWindow &window);
  void closeSubgraphWindowNow(GraphTab &root, SubgraphWindow &window);
  int indexOfTab(const GraphTab &tab) const;
  int indexOfTabId(int tab_id) const;
  // The tab or window showing that file. One file is one document: of two copies, the one saved last
  // would drop the other's edits.
  GraphTab *findSurfaceByPath(const char *path) const;
  // Root or window tab; null once closed.
  GraphTab *findSurface(int tab_id) const;
  // Its root becomes the active tab, and it the focused surface, back from the stack if it was minimized.
  void activateSurface(GraphTab &surface);

  PropPanel::PanelWindowPropertyControl *panelWindow = nullptr;
  GraphEditorPlg &plugin;
  eastl::vector<eastl::unique_ptr<GraphTab>> tabs;
  int activeIndex = -1;
  int nextTabId = 1;
  // Both resolved between frames: closing raises a modal prompt, and a switch commits an edit and
  // can cancel a gesture, which record undo entries. Neither may run inside the tab bar, and a
  // switch may not run inside an undo apply.
  int pendingCloseTabId = -1;
  int pendingActivateTabId = -1;
  bool plusRequested = false;
  enum class WindowAction
  {
    Close,
    Save,
    SaveAs,
    Minimize,
  };
  struct PendingWindowAction
  {
    int tabId = -1; // none
    WindowAction action = WindowAction::Close;
  };
  // A window button whose action raises a modal or moves the focus, which the focus update after the windows would undo,
  // so it runs between frames too. One is enough: a frame takes one click, and the tick drains it before the next frame.
  PendingWindowAction pendingWindowAction;
  // A switch we made ourselves, pushed to the strip with ImGuiTabItemFlags_SetSelected.
  int selectTabIdNextFrame = -1;
  int focusStamp = 0;
  // ImGui's focus follows the root canvas on the next frame.
  bool rootFocusRequested = false;
  bool subgraphCardsShown = false;
};
