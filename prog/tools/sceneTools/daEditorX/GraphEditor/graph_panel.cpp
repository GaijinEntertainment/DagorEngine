// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_panel.h"

#include "graph_document.h"
#include "graph_hotkeys_bar.h"
#include "graph_status_bar.h"
#include "graph_subgraph_window.h"
#include "graph_tab.h"
#include "plugin.h"

#include "pluginService/graph_tex_gen_service.h"

#include <de3_interface.h>
#include <EditorCore/ec_interface.h>
#include <libTools/util/undo.h>
#include <osApiWrappers/dag_direct.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <imgui/imgui_internal.h>

#include <EASTL/algorithm.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/sort.h>
#include <EASTL/string.h>
#include <EASTL/utility.h>

namespace
{
constexpr const char *TAB_STRIP_ID = "graph_tabs";

// The window of the root focused last, else the root's own canvas.
GraphTab &focused_surface(GraphTab &root)
{
  SubgraphWindow *window = root.getFocusedWindow();
  return window ? window->getTab() : root;
}

// The root, then its windows. By index: a tick can open a window, which appends to the list.
template <class Fn>
void for_each_surface_of(GraphTab &root, Fn &&fn)
{
  fn(root);
  const eastl::vector<eastl::unique_ptr<SubgraphWindow>> &windows = root.getSubgraphWindows();
  for (int i = 0; i < static_cast<int>(windows.size()); ++i)
  {
    fn(windows[i]->getTab());
  }
}

template <class Fn>
void for_each_surface(const eastl::vector<eastl::unique_ptr<GraphTab>> &tabs, Fn &&fn)
{
  for (const eastl::unique_ptr<GraphTab> &t : tabs)
  {
    for_each_surface_of(*t, fn);
  }
}

// The first surface of the root that `pred` accepts, or null.
template <class Pred>
GraphTab *find_surface_of(GraphTab &root, Pred &&pred)
{
  GraphTab *found = nullptr;
  for_each_surface_of(root, [&](GraphTab &t) {
    if (!found && pred(t))
    {
      found = &t;
    }
  });
  return found;
}

template <class Pred>
GraphTab *find_surface(const eastl::vector<eastl::unique_ptr<GraphTab>> &tabs, Pred &&pred)
{
  for (const eastl::unique_ptr<GraphTab> &t : tabs)
  {
    if (GraphTab *found = find_surface_of(*t, pred))
    {
      return found;
    }
  }
  return nullptr;
}

// A file keeps the pins its instances had when it was saved. Rebuilt before the document takes the
// graph, so it opens unmodified and the file catches up on its next save.
void upgrade_subgraph_instances(GraphData &gd, const DataBlock &base_nodes_blk, const char *path)
{
  for (GraphData::Node &n : gd.nodes)
  {
    GraphData::Node fresh;
    eastl::vector<GraphData::Edge> kept;
    eastl::vector<DroppedInstanceEdge> dropped;
    if (!is_subgraph_instance(n) || !resync_subgraph_instance(gd, n.id, base_nodes_blk, fresh, kept, dropped))
    {
      continue;
    }
    for (const DroppedInstanceEdge &d : dropped)
    {
      DAEDITOR3.conWarning("GraphEditor: %s: instance '%s' id=%d: edge %d dropped, pin '%s' no longer exists", path,
        n.descName.c_str(), n.id, d.edgeId, d.pinName.c_str());
    }
    // In place, one instance at a time: the next one reads these edges, and the file keeps its order.
    const int id = n.id;
    n = eastl::move(fresh);
    auto keptEdge = [&kept](int edge_id) -> const GraphData::Edge * {
      for (const GraphData::Edge &k : kept)
      {
        if (k.id == edge_id)
        {
          return &k;
        }
      }
      return nullptr;
    };
    gd.edges.erase(eastl::remove_if(gd.edges.begin(), gd.edges.end(),
                     [&](const GraphData::Edge &e) { return (e.elemA == id || e.elemB == id) && !keptEdge(e.id); }),
      gd.edges.end());
    for (GraphData::Edge &e : gd.edges)
    {
      if (e.elemA == id || e.elemB == id)
      {
        e = *keptEdge(e.id);
      }
    }
  }
}

// A graph file as a document takes it: its instances upgraded on the way.
bool load_graph_file(GraphEditorPlg &plugin, const char *path, GraphData &out)
{
  if (!load_graph_data_blk(out, path, plugin.getShaderIncludesDir(), &plugin.getBaseNodesBlk()))
  {
    return false;
  }
  upgrade_subgraph_instances(out, plugin.getBaseNodesBlk(), path);
  return true;
}

SubgraphWindow *nearest_shown_window(SubgraphWindow *window)
{
  while (window && window->isMinimized())
  {
    window = window->getParent();
  }
  return window;
}

// `window` and the windows opened from it, at any depth, each before the one it was opened from:
// the list holds them in open order, so it is read backwards.
eastl::fixed_vector<SubgraphWindow *, 8> subgraph_window_tree(GraphTab &root, SubgraphWindow &window)
{
  eastl::fixed_vector<SubgraphWindow *, 8> tree;
  const eastl::vector<eastl::unique_ptr<SubgraphWindow>> &windows = root.getSubgraphWindows();
  for (int i = static_cast<int>(windows.size()) - 1; i >= 0; --i)
  {
    for (const SubgraphWindow *w = windows[i].get(); w; w = w->getParent())
    {
      if (w == &window)
      {
        tree.push_back(windows[i].get());
        break;
      }
    }
  }
  return tree;
}
} // namespace

GraphPanel::GraphPanel(GraphEditorPlg &plg) : plugin(plg) {}

GraphPanel::~GraphPanel()
{
  // No undo entry may outlive the document it points into.
  dropUndoOps();
  tabs.clear();
  destroyWindow();
}

void GraphPanel::createWindow()
{
  G_ASSERT(!panelWindow);
  panelWindow = IEditorCoreEngine::get()->createPropPanel(this, "Graph");
}

void GraphPanel::destroyWindow()
{
  if (!panelWindow)
  {
    return;
  }
  IEditorCoreEngine::get()->deleteCustomPanel(panelWindow);
  panelWindow = nullptr;
}

// The first tab is built on first use, the way the strip opens the rest.
GraphTab &GraphPanel::activeTab()
{
  if (tabs.empty())
  {
    createTab();
    setActiveIndex(0);
  }
  return *tabs[activeIndex];
}

GraphTab &GraphPanel::activeSurface() { return focused_surface(activeTab()); }

GraphDocument &GraphPanel::activeDocument() { return activeSurface().getDocument(); }

int GraphPanel::activeTabId() { return activeSurface().getId(); }

GraphTab &GraphPanel::openNewTab()
{
  GraphTab &tab = createTab();
  activateTab(tab);
  return tab;
}

GraphTab *GraphPanel::openGraphFile(const char *path)
{
  if (GraphTab *existing = findSurfaceByPath(path))
  {
    activateSurface(*existing);
    return existing;
  }

  // Loaded before the tab exists: a file that fails to parse must not leave an empty tab behind.
  GraphData loaded;
  if (!load_graph_file(plugin, path, loaded))
  {
    return nullptr;
  }
  GraphTab &tab = createTab();
  tab.getDocument().replaceGraphData([&loaded](GraphData &gd) { gd = eastl::move(loaded); });
  tab.onGraphDataChanged();
  activateTab(tab);
  return &tab;
}

void GraphPanel::activateTab(GraphTab &tab) { setActiveIndex(indexOfTab(tab)); }

// Called from an undo entry: undo() is walking its stack, and switching would record into it.
// The apply itself needs no canvas, so only the switch waits for the next tick.
void GraphPanel::activateTabOf(const GraphDocument &doc)
{
  if (GraphTab *surface = find_surface(tabs, [&doc](GraphTab &t) { return &t.getDocument() == &doc; }))
  {
    pendingActivateTabId = surface->getId();
  }
}

void GraphPanel::focusParentSurface(const GraphTab &from)
{
  if (const SubgraphWindow *window = from.getHostWindow())
  {
    pendingActivateTabId = window->getParent() ? window->getParent()->getTab().getId() : from.getRootTab().getId();
  }
}

void GraphPanel::openSubgraph(GraphTab &from, const char *stem)
{
  GraphTab &root = from.getRootTab();
  const String path = plugin.getSubgraphPath(stem);
  // A graph open above this instance would open inside itself.
  const eastl::string &rootPath = root.getDocument().getGraphData().sourcePath;
  bool cycle = !rootPath.empty() && dd_fname_equal(rootPath.c_str(), path.str());
  for (const SubgraphWindow *w = from.getHostWindow(); w && !cycle; w = w->getParent())
  {
    cycle = w->getStem() == stem;
  }
  if (cycle)
  {
    DAEDITOR3.conError("GraphEditor: %s does not open inside itself: it contains this instance", path.str());
    return;
  }
  if (GraphTab *open = findSurfaceByPath(path.str()))
  {
    pendingActivateTabId = open->getId(); // on the next tick: this runs inside the tick of a surface
    return;
  }

  GraphData loaded;
  if (!load_graph_file(plugin, path.str(), loaded))
  {
    DAEDITOR3.conError("GraphEditor: failed to load subgraph %s", path.str());
    return;
  }
  auto window =
    eastl::make_unique<SubgraphWindow>(plugin, nextTabId++, root, from.getHostWindow(), stem, root.getSubgraphWindowLayout(stem));
  window->getTab().getDocument().replaceGraphData([&loaded](GraphData &gd) { gd = eastl::move(loaded); });
  window->getTab().onGraphDataChanged();
  SubgraphWindow &opened = *window;
  root.getSubgraphWindows().push_back(eastl::move(window));
  focusSurface(root, &opened);
}

bool GraphPanel::isSubgraphOpen(const char *stem) const { return findSurfaceByPath(plugin.getSubgraphPath(stem)) != nullptr; }

void GraphPanel::rebindTexGen()
{
  IGraphTexGenService *svc = plugin.getTexGenService();
  if (!svc)
  {
    return;
  }
  GraphTab &active = activeTab();
  // The compiler and the graph data are two writes under two locks, so the worker must not be able
  // to run between them: it would compile the new graph and commit the result into the old one.
  // Clearing the compiler first drains a compile in flight, as the close path does.
  svc->setGraphCompiler(nullptr);
  active.getDocument().pushHeightmapParams();
  svc->setGraphData(active.getDocument().dataForTexGenService());
  svc->setGraphCompiler(active.getCompiler());
  svc->markGraphDirty();
}

IGraphTexGenService *GraphPanel::texGenServiceFor(const GraphDocument &doc) const
{
  if (activeIndex < 0 || &tabs[activeIndex]->getDocument() != &doc)
  {
    return nullptr;
  }
  return plugin.getTexGenService();
}

void GraphPanel::stampUndoOwner()
{
  if (activeIndex >= 0)
  {
    stampUndoOwner(focused_surface(*tabs[activeIndex]));
  }
}

void GraphPanel::stampUndoOwner(GraphTab &owner)
{
  // A switch reaches this while another plugin can be current: never take that plugin's owner.
  if (EDITORCORE->curPlugin() != static_cast<IGenEditorPlugin *>(&plugin))
  {
    return;
  }
  EDITORCORE->getUndoSystem()->set_op_owner(&owner);
}

bool GraphPanel::ownsUndoToken(const void *token) const
{
  return find_surface(tabs, [token](GraphTab &t) { return token == static_cast<const void *>(&t); }) != nullptr;
}

void GraphPanel::dropUndoOps()
{
  UndoSystem *undo = EDITORCORE->getUndoSystem();
  for_each_surface(tabs, [undo](GraphTab &t) { undo->remove_ops_by_owner(&t); });
  // The engine re-stamps its own token on each plugin switch, so an entry accepted between that
  // and our next frame carries it instead of the tab.
  undo->remove_ops_by_owner(static_cast<IGenEditorPlugin *>(&plugin));
}

// The ### suffix keeps each tab id stable while its label follows the file name.
void GraphPanel::drawTabStrip()
{
  const ImGuiTabBarFlags barFlags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll |
                                    ImGuiTabBarFlags_NoTooltip | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton |
                                    ImGuiTabBarFlags_Reorderable;
  if (!ImGui::BeginTabBar(TAB_STRIP_ID, barFlags))
  {
    return;
  }
  // ImGui lays the bar out from the previous frame, so a selection we push (or a tab we just
  // added) only shows next frame. Reading the still-visible old tab as a click would switch
  // back and forth every frame.
  const bool pushingSelection = selectTabIdNextFrame >= 0;
  int clickedTabId = -1;
  eastl::fixed_vector<ImGuiID, 16> imguiTabIds;
  for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
  {
    GraphTab &tab = *tabs[i];
    // Asking walks the undo history, and the label, the close and the tooltip all want the answer.
    const bool modified = tab.isModified();
    // ImGui caps the tab width and ends a long label with "...", so the name goes in whole. The
    // design marks a graph that holds edits its file does not.
    const eastl::string label(eastl::string::CtorSprintf(), "%s%s###tab%d", modified ? "*" : "", tab.getName().c_str(), tab.getId());
    // NoAssumedClosure: a close is a request here, and the prompt that can refuse it runs between
    // frames. Without it ImGui drops the tab on the press and re-adds it when we keep submitting.
    ImGuiTabItemFlags flags = ImGuiTabItemFlags_NoTooltip | ImGuiTabItemFlags_NoAssumedClosure;
    if (selectTabIdNextFrame == tab.getId())
    {
      flags |= ImGuiTabItemFlags_SetSelected;
    }
    bool open = true;
    if (ImGui::BeginTabItem(label.c_str(), &open, flags))
    {
      ImGui::EndTabItem();
      if (!pushingSelection && i != activeIndex)
      {
        clickedTabId = tab.getId();
      }
    }
    imguiTabIds.push_back(ImGui::GetItemID());
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
    {
      // The design gives the modified mark a tooltip of its own, so the star is hit-tested apart from
      // the rest of the tab. It is the first glyph, drawn where the label starts.
      const ImVec2 itemMin = ImGui::GetItemRectMin();
      const ImVec2 pad = ImGui::GetStyle().FramePadding;
      const ImVec2 starMin(itemMin.x + pad.x, itemMin.y + pad.y);
      const ImVec2 starMax(starMin.x + ImGui::CalcTextSize("*").x, starMin.y + ImGui::GetTextLineHeight());
      const bool onStar = modified && ImGui::IsMouseHoveringRect(starMin, starMax);
      ImGui::SetTooltip("%s", onStar ? "Unsaved changes" : tab.getPathOrName().c_str());
    }
    if (!open)
    {
      pendingCloseTabId = tab.getId();
    }
  }
  selectTabIdNextFrame = -1;
  // Trailing keeps it after the last tab whatever the order is: a reorder cannot cross sections, and
  // the section starts where the tabs end. A button tab can never be selected. The design draws a
  // bare + on the strip: nothing at rest, and the hovered fill alone once the cursor is on it. The
  // theme gives every tab a 1px border, which TabItemBackground strokes whatever the fill is.
  ImGui::PushStyleColor(ImGuiCol_Tab, IM_COL32(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_TabDimmed, IM_COL32(0, 0, 0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_TabBorderSize, 0.0f);
  if (ImGui::TabItemButton("+", ImGuiTabItemFlags_NoTooltip | ImGuiTabItemFlags_Trailing))
  {
    plusRequested = true;
  }
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(2);
  syncTabOrderFromBar(make_span_const(imguiTabIds));
  ImGui::EndTabBar();
  // Outside the bar, so the switch never runs between BeginTabBar and EndTabBar. By id, because
  // the sync above may have moved the tab the click landed on.
  if (clickedTabId >= 0)
  {
    setActiveIndex(indexOfTabId(clickedTabId));
    // ImGui has that tab selected already; pushing it back would blind the next frame's click.
    selectTabIdNextFrame = -1;
  }
}

// Following ImGui's order is what leaves closeTabNow picking the neighbour the user sees.
void GraphPanel::syncTabOrderFromBar(dag::ConstSpan<ImGuiID> imgui_tab_ids)
{
  const GraphTab *active = activeIndex >= 0 ? tabs[activeIndex].get() : nullptr;
  eastl::vector<eastl::unique_ptr<GraphTab>> reordered;
  reordered.reserve(tabs.size());
  // The bar is never null here: this runs between BeginTabBar and EndTabBar, nowhere else.
  for (const ImGuiTabItem &item : ImGui::GetCurrentTabBar()->Tabs)
  {
    if (item.Flags & ImGuiTabItemFlags_Button)
    {
      continue; // the + is the one item with no tab behind it
    }
    for (int i = 0; i < static_cast<int>(imgui_tab_ids.size()); ++i)
    {
      if (tabs[i] && imgui_tab_ids[i] == item.ID)
      {
        reordered.push_back(eastl::move(tabs[i]));
        break;
      }
    }
  }
  // A tab the bar does not know, which submission rules out. Kept rather than dropped, because
  // dropping one here frees a document the undo entries and the texgen service point into.
  for (eastl::unique_ptr<GraphTab> &tab : tabs)
  {
    if (tab)
    {
      reordered.push_back(eastl::move(tab));
    }
  }
  tabs = eastl::move(reordered);
  activeIndex = active ? indexOfTab(*active) : -1;
}

void GraphPanel::updateImgui()
{
  drawTabStrip();
  GraphTab &root = activeTab();
  if (rootFocusRequested)
  {
    ImGui::SetWindowFocus();
    rootFocusRequested = false;
  }
  // A status-bar strip is reserved below the canvas; the 4px floor keeps the canvas size positive on
  // a tiny panel (ImGuiEx::Canvas treats a non-positive size as "use all available").
  const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
  const ImVec2 avail = ImGui::GetContentRegionAvail();
  const float statusBarHeight = graph_status_bar_height();
  const float canvasHeight = eastl::max(4.0f, avail.y - statusBarHeight);
  const ImVec2 canvasMax(canvasMin.x + avail.x, canvasMin.y + canvasHeight);
  SubgraphWindow *const focusedWindow = drawSubgraphWindows(root, canvasMin, canvasMax);
  const float stackWidth = drawMinimizedSubgraphWindows(root, canvasMin, canvasMax);
  stampUndoOwner(root);
  root.updateCanvas(canvasMin, canvasMax);
  // The panels drawn after the canvases edit the focused surface's document.
  stampUndoOwner();
  // Focus leaving for another panel keeps the surface.
  if (ImGui::IsWindowFocused())
  {
    focusSurface(root, nullptr);
  }
  else if (focusedWindow)
  {
    focusSurface(root, focusedWindow);
  }
  TexGenPipelineStatus pipeline;
  if (IGraphTexGenService *svc = plugin.getTexGenService())
  {
    pipeline = svc->getPipelineStatus();
  }
  // constraint: the design frames the focused surface only while windows exist to tell it from.
  if (!root.getFocusedWindow() && !root.getSubgraphWindows().empty())
  {
    draw_focus_frame(*ImGui::GetWindowDrawList(), canvasMin, canvasMax, 0.0f, pipeline.graphCompileFailed || pipeline.hasErrors);
  }
  // The bars describe the surface the keys go to: a window over this canvas, when one is focused.
  GraphTab &surface = activeSurface();
  draw_graph_hotkeys_bar(canvasMin, canvasMax, surface.getHotkeyContext(), stackWidth);
  ImGui::SetCursorScreenPos(ImVec2(canvasMin.x, canvasMax.y));
  draw_graph_status_bar(surface.getDocument().getGraphData(), pipeline, surface.getSelectedObjectCount(), statusBarHeight);
}

bool GraphPanel::hasPendingActions() const
{
  bool pending = pendingCloseTabId >= 0 || pendingWindowAction.tabId >= 0 || pendingActivateTabId >= 0 || plusRequested;
  if (activeIndex >= 0)
  {
    pending = pending || find_surface_of(*tabs[activeIndex], [](GraphTab &t) { return t.hasPendingActions(); }) != nullptr;
  }
  return pending;
}

void GraphPanel::actObjects(float dt)
{
  stampUndoOwner(); // this runs before the first frame after a switch, where the engine owns the stamp
  if (pendingWindowAction.tabId >= 0)
  {
    const PendingWindowAction request = pendingWindowAction;
    pendingWindowAction = PendingWindowAction();
    // Before the switches below: the window belongs to the active tab.
    GraphTab *const surface = findSurface(request.tabId);
    SubgraphWindow *const window = surface ? surface->getHostWindow() : nullptr;
    if (surface && window)
    {
      switch (request.action)
      {
        case WindowAction::Close:
          if (confirmCloseSubgraphWindow(surface->getRootTab(), *window))
          {
            closeSubgraphWindowNow(surface->getRootTab(), *window);
          }
          break;
        case WindowAction::Save: plugin.saveSubgraph(*surface); break;
        case WindowAction::SaveAs: plugin.saveAsSubgraphBlk(*surface); break;
        case WindowAction::Minimize: minimizeSubgraphWindow(surface->getRootTab(), *window); break;
      }
    }
  }
  if (pendingCloseTabId >= 0)
  {
    const int id = pendingCloseTabId;
    pendingCloseTabId = -1;
    const int idx = indexOfTabId(id);
    if (idx >= 0 && confirmCloseWithWindows(*tabs[idx]))
    {
      closeTabNow(id);
    }
  }
  if (pendingActivateTabId >= 0)
  {
    const int id = pendingActivateTabId;
    pendingActivateTabId = -1;
    if (GraphTab *const target = findSurface(id)) // null if closed since
    {
      activateSurface(*target);
    }
  }
  if (plusRequested)
  {
    plusRequested = false;
    // A PropPanel dialog aborts inside an ImGui frame, so the click only queued the request.
    // Last, because it opens a tab of its own and a switch queued earlier must not override it.
    plugin.promptOpenGraph();
  }
  if (!tabs.empty())
  {
    // Only the shown tab and its windows tick: this is where a queued delete raises its modal prompt.
    // Each one owns the entries its own tick records.
    for_each_surface_of(activeTab(), [this, dt](GraphTab &t) {
      stampUndoOwner(t);
      t.actObjects(dt);
    });
    stampUndoOwner();
  }
}

void GraphPanel::requestSelectAll()
{
  if (panelWindow)
  {
    activeSurface().requestSelectAll();
  }
}

void GraphPanel::requestDeselectAll()
{
  if (panelWindow)
  {
    activeSurface().requestDeselectAll();
  }
}

void GraphPanel::requestInvertSelection()
{
  if (panelWindow)
  {
    activeSurface().requestInvertSelection();
  }
}

void GraphPanel::requestFrameSelected()
{
  if (panelWindow)
  {
    activeSurface().requestFrameSelected();
  }
}

void GraphPanel::invalidateDerivedState()
{
  for_each_surface(tabs, [](GraphTab &t) { t.invalidateDerivedState(); });
}

void GraphPanel::applyPendingCompileResults()
{
  for_each_surface(tabs, [](GraphTab &t) { t.getDocument().applyPendingCompileResults(); });
}

int GraphPanel::getSelectedNodeId() const { return activeIndex >= 0 ? focused_surface(*tabs[activeIndex]).getSelectedNodeId() : -1; }

bool GraphPanel::isBaseNodeDropTargetHot() const
{
  return activeIndex >= 0 && find_surface_of(*tabs[activeIndex], [](GraphTab &t) { return t.isBaseNodeDropTargetHot(); }) != nullptr;
}

ImVec2 GraphPanel::getLastCanvasCenter() { return activeSurface().getLastCanvasCenter(); }

int GraphPanel::allocateNodeId() { return activeSurface().allocateNodeId(); }

GraphTab &GraphPanel::createTab()
{
  tabs.push_back(eastl::make_unique<GraphTab>(plugin, nextTabId++));
  return *tabs.back();
}

void GraphPanel::setActiveIndex(int index)
{
  if (index < 0 || index == activeIndex)
  {
    return;
  }
  // The properties panel commits its open edit while the graph it was typed into is still the
  // active one, so the entry carries that tab and no gesture stays open across the switch.
  plugin.beforeActiveTabChanges();
  if (activeIndex >= 0)
  {
    // Every surface, mirroring the activation below; a second call on one already deactivated does nothing.
    for_each_surface_of(*tabs[activeIndex], [](GraphTab &t) { t.onDeactivated(); });
  }
  activeIndex = index;
  GraphTab &tab = *tabs[activeIndex];
  selectTabIdNextFrame = tab.getId();
  // Every canvas of the tab is drawn again, and the surface it had focused comes back with it.
  for_each_surface_of(tab, [](GraphTab &t) { t.onActivated(); });
  // A tab click focuses the dock window, which would move the surface to the canvas.
  if (SubgraphWindow *window = tab.getFocusedWindow())
  {
    window->raise(++focusStamp);
  }
  rebindTexGen();
  stampUndoOwner();
}

// Only the active tab's canvases are drawn, so no other tab can move its focus.
void GraphPanel::focusSurface(GraphTab &root, SubgraphWindow *window)
{
  if (activeIndex < 0 || tabs[activeIndex].get() != &root || root.getFocusedWindow() == window)
  {
    return;
  }
  G_ASSERT(!window || !window->isMinimized());
  // A pending edit belongs to the surface being left, whichever canvas pass this runs in.
  stampUndoOwner();
  plugin.beforeActiveTabChanges();
  focused_surface(root).onDeactivated();
  // No onActivated: the canvas it moves to was drawn all along.
  root.setFocusedWindow(window);
  if (window)
  {
    window->raise(++focusStamp);
  }
  else
  {
    rootFocusRequested = true;
  }
  stampUndoOwner();
}

bool GraphPanel::confirmCloseAllTabs()
{
  for (const eastl::unique_ptr<GraphTab> &tab : tabs)
  {
    if (!confirmCloseWithWindows(*tab))
    {
      return false;
    }
  }
  return true;
}

// Each window opened from the canvas, with the windows nested in it, then the canvas itself.
bool GraphPanel::confirmCloseWithWindows(GraphTab &root)
{
  const eastl::vector<eastl::unique_ptr<SubgraphWindow>> &windows = root.getSubgraphWindows();
  for (int i = static_cast<int>(windows.size()) - 1; i >= 0; --i)
  {
    if (!windows[i]->getParent() && !confirmCloseSubgraphWindow(root, *windows[i]))
    {
      return false;
    }
  }
  return confirmClose(root, "Close graph");
}

// The editor asks this way about a changed project, and the design asks for the same about a graph.
bool GraphPanel::confirmClose(GraphTab &tab, const char *title)
{
  if (!tab.isModified())
  {
    return true;
  }
  const eastl::string name = tab.getPathOrName();
  const int ret = wingw::message_box(wingw::MBS_YESNOCANCEL, title, "Graph changed:\n\"%s\"\n\nSave changes?", name.c_str());
  if (ret == wingw::MB_ID_YES)
  {
    // A save that did not happen keeps the graph open: a cancelled path dialog must not lose it.
    return tab.getHostWindow() ? plugin.saveSubgraph(tab) : plugin.saveGraph(tab);
  }
  return ret == wingw::MB_ID_NO;
}

// The strip is left empty: activeTab() makes a fresh tab for whoever asks first, so a teardown
// that asks for none does not pay for one.
void GraphPanel::closeAllTabs()
{
  if (IGraphTexGenService *svc = plugin.getTexGenService())
  {
    // Drains a compile in flight, so none commits into a document that is about to go.
    svc->setGraphCompiler(nullptr);
    svc->setGraphData(nullptr);
  }
  // The properties panel may hold controls built for any of these documents, and an edit still open.
  for_each_surface(tabs, [this](GraphTab &t) { plugin.beforeTabCloses(t.getDocument()); });
  G_ASSERT(!EDITORCORE->getUndoSystem()->is_holding());
  dropUndoOps();
  tabs.clear();
  activeIndex = -1;
  pendingCloseTabId = -1;
  pendingActivateTabId = -1;
  pendingWindowAction = PendingWindowAction();
  plusRequested = false;
  selectTabIdNextFrame = -1;
}

void GraphPanel::closeTabNow(int tab_id)
{
  const int idx = indexOfTabId(tab_id);
  if (idx < 0)
  {
    return;
  }

  GraphTab &tab = *tabs[idx];
  // The properties panel may hold controls built for this document, or for one of its windows.
  for_each_surface_of(tab, [this](GraphTab &t) { plugin.beforeTabCloses(t.getDocument()); });
  // Before its entries go: a gesture it ends records its outcome.
  for_each_surface_of(tab, [](GraphTab &t) { t.onDeactivated(); });
  UndoSystem *undo = EDITORCORE->getUndoSystem();
  G_ASSERT(!undo->is_holding());
  for_each_surface_of(tab, [undo](GraphTab &t) { undo->remove_ops_by_owner(&t); });

  // The strip is never empty: the last tab closing leaves a fresh untitled one.
  if (tabs.size() == 1)
  {
    createTab();
  }
  const bool wasActive = idx == activeIndex;
  const int nextIdx = idx + 1 < static_cast<int>(tabs.size()) ? idx + 1 : idx - 1;

  IGraphTexGenService *svc = plugin.getTexGenService();
  if (svc)
  {
    // Drains a compile of any tab that is still in flight, so none commits into a dead document.
    svc->setGraphCompiler(nullptr);
  }
  if (wasActive)
  {
    if (svc)
    {
      // Re-point before the document dies: the service holds the pointer, not a copy.
      svc->setGraphData(tabs[nextIdx]->getDocument().dataForTexGenService());
    }
    activeIndex = -1;
  }

  tabs.erase(tabs.begin() + idx);

  if (wasActive)
  {
    setActiveIndex(nextIdx > idx ? nextIdx - 1 : nextIdx);
  }
  else
  {
    if (activeIndex > idx)
    {
      --activeIndex;
    }
    if (svc)
    {
      svc->setGraphCompiler(activeTab().getCompiler());
    }
  }
}

SubgraphWindow *GraphPanel::drawSubgraphWindows(GraphTab &root, const ImVec2 &canvas_min, const ImVec2 &canvas_max)
{
  // The design keeps windows off the hotkeys bar.
  const ImVec2 areaMax(canvas_max.x, canvas_max.y - graph_hotkeys_bar_height());
  eastl::vector<eastl::unique_ptr<SubgraphWindow>> &windows = root.getSubgraphWindows();
  eastl::fixed_vector<SubgraphWindow *, 8> order;
  for (const eastl::unique_ptr<SubgraphWindow> &w : windows)
  {
    if (!w->isPlaced())
    {
      w->placeDefault(ImVec2(areaMax.x - canvas_min.x, areaMax.y - canvas_min.y), windows);
    }
    if (!w->isMinimized())
    {
      order.push_back(w.get());
    }
  }
  // ImGui puts the child begun last on top.
  // constraint: the design keeps a maximized window behind the others, which it would hide.
  eastl::sort(order.begin(), order.end(), [](const SubgraphWindow *a, const SubgraphWindow *b) {
    if (a->getLayout().maximized != b->getLayout().maximized)
    {
      return a->getLayout().maximized;
    }
    return a->getFocusStamp() < b->getFocusStamp();
  });
  SubgraphWindowFrame frame;
  frame.areaMin = canvas_min;
  frame.areaMax = areaMax;
  frame.canvasMax = canvas_max;
  const eastl::vector<eastl::string> &failedSubgraphs = root.getDocument().getFailedSubgraphs();
  SubgraphWindow *focused = nullptr;
  for (SubgraphWindow *w : order)
  {
    frame.active = root.getFocusedWindow() == w;
    frame.failed = eastl::find(failedSubgraphs.begin(), failedSubgraphs.end(), w->getStem()) != failedSubgraphs.end();
    // The windows above w, the outermost first; the crumbs put the root before them.
    eastl::fixed_vector<const SubgraphWindow *, 8> chain;
    for (const SubgraphWindow *p = w->getParent(); p; p = p->getParent())
    {
      chain.push_back(p);
    }
    eastl::reverse(chain.begin(), chain.end());
    frame.parentNames.clear();
    frame.parentNames.push_back(root.getName());
    for (const SubgraphWindow *p : chain)
    {
      frame.parentNames.push_back(p->getStem());
    }
    // A canvas records into its own document, so it owns those entries whichever surface is focused.
    stampUndoOwner(w->getTab());
    const SubgraphWindow::DrawResult result = w->draw(frame);
    if (result.focused)
    {
      focused = w;
    }
    if (result.closeClicked)
    {
      pendingWindowAction = {w->getTab().getId(), WindowAction::Close};
    }
    if (result.minimizeClicked)
    {
      pendingWindowAction = {w->getTab().getId(), WindowAction::Minimize};
    }
    if (result.saveClicked)
    {
      pendingWindowAction = {w->getTab().getId(), WindowAction::Save};
    }
    if (result.saveAsClicked)
    {
      pendingWindowAction = {w->getTab().getId(), WindowAction::SaveAs};
    }
    if (result.crumbClicked >= 0)
    {
      // On the next tick: a switch made now would lose to the focus the click gave w.
      pendingActivateTabId = result.crumbClicked == 0 ? root.getId() : chain[result.crumbClicked - 1]->getTab().getId();
    }
  }
  return focused;
}

float GraphPanel::drawMinimizedSubgraphWindows(GraphTab &root, const ImVec2 &canvas_min, const ImVec2 &canvas_max)
{
  eastl::fixed_vector<SubgraphWindow *, 8> minimized;
  for (const eastl::unique_ptr<SubgraphWindow> &w : root.getSubgraphWindows())
  {
    if (w->isMinimized())
    {
      minimized.push_back(w.get());
    }
  }
  if (minimized.empty())
  {
    subgraphCardsShown = false;
    return 0.0f;
  }
  eastl::sort(minimized.begin(), minimized.end(),
    [](const SubgraphWindow *a, const SubgraphWindow *b) { return a->getMinimizeStamp() < b->getMinimizeStamp(); });
  const float width = subgraph_stack_width(static_cast<int>(minimized.size()));
  const SubgraphStackResult result = draw_subgraph_stack(graph_hotkeys_bar_end(canvas_max, width), canvas_min, canvas_max,
    make_span_const(minimized), subgraphCardsShown);
  if (result.shown)
  {
    pendingActivateTabId = result.shown->getTab().getId();
  }
  if (result.closed)
  {
    pendingWindowAction = {result.closed->getTab().getId(), WindowAction::Close};
  }
  return width;
}

void GraphPanel::minimizeSubgraphWindow(GraphTab &root, SubgraphWindow &window)
{
  if (root.getFocusedWindow() == &window)
  {
    focusSurface(root, nearest_shown_window(window.getParent()));
  }
  // focusSurface moves nothing on a root that is not shown.
  if (root.getFocusedWindow() != &window)
  {
    window.minimize(++focusStamp);
  }
}

// A Cancel on any of them keeps them all open.
bool GraphPanel::confirmCloseSubgraphWindow(GraphTab &root, SubgraphWindow &window)
{
  for (SubgraphWindow *w : subgraph_window_tree(root, window))
  {
    if (!confirmClose(w->getTab(), "Close subgraph"))
    {
      return false;
    }
  }
  return true;
}

// The windows opened from it go too: their crumbs and Tab lead through it.
void GraphPanel::closeSubgraphWindowNow(GraphTab &root, SubgraphWindow &window)
{
  const eastl::fixed_vector<SubgraphWindow *, 8> closing = subgraph_window_tree(root, window);
  if (eastl::find(closing.begin(), closing.end(), root.getFocusedWindow()) != closing.end())
  {
    focusSurface(root, nearest_shown_window(window.getParent()));
  }
  eastl::vector<eastl::unique_ptr<SubgraphWindow>> &windows = root.getSubgraphWindows();
  UndoSystem *undo = EDITORCORE->getUndoSystem();
  for (SubgraphWindow *w : closing)
  {
    root.setSubgraphWindowLayout(w->getStem().c_str(), w->getLayout());
    plugin.beforeTabCloses(w->getTab().getDocument());
    G_ASSERT(!undo->is_holding());
    undo->remove_ops_by_owner(&w->getTab());
    windows.erase(
      eastl::find_if(windows.begin(), windows.end(), [w](const eastl::unique_ptr<SubgraphWindow> &uw) { return uw.get() == w; }));
  }
}

GraphTab *GraphPanel::findSurface(int tab_id) const
{
  return find_surface(tabs, [tab_id](GraphTab &t) { return t.getId() == tab_id; });
}

GraphTab *GraphPanel::findSurfaceByPath(const char *path) const
{
  return find_surface(tabs, [path](GraphTab &t) {
    const eastl::string &openPath = t.getDocument().getGraphData().sourcePath;
    return !openPath.empty() && dd_fname_equal(openPath.c_str(), path);
  });
}

void GraphPanel::activateSurface(GraphTab &surface)
{
  GraphTab &root = surface.getRootTab();
  activateTab(root);
  SubgraphWindow *window = surface.getHostWindow();
  if (window)
  {
    window->unminimize();
  }
  focusSurface(root, window);
}

int GraphPanel::indexOfTab(const GraphTab &tab) const
{
  for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
  {
    if (tabs[i].get() == &tab)
    {
      return i;
    }
  }
  return -1;
}

int GraphPanel::indexOfTabId(int tab_id) const
{
  for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
  {
    if (tabs[i]->getId() == tab_id)
    {
      return i;
    }
  }
  return -1;
}