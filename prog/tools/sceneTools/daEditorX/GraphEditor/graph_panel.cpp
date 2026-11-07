// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_panel.h"

#include "command_definitions.h"
#include "graph_document.h"
#include "graph_hotkeys_bar.h"
#include "graph_node_render.h"
#include "graph_open_subgraphs.h"
#include "graph_status_bar.h"
#include "graph_subgraph_window.h"
#include "graph_tab.h"
#include "graph_theme.h"
#include "graph_undo.h"
#include "plugin.h"

#include "pluginService/graph_tex_gen_service.h"

#include <de3_interface.h>
#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_interface.h>
#include <libTools/util/hdpiUtil.h>
#include <libTools/util/undo.h>
#include <osApiWrappers/dag_direct.h>
#include <propPanel/control/container.h>
#include <propPanel/imguiHelper.h>
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

enum
{
  TOOLBAR_ID = 1,
  TOOLBAR_NEW_GRAPH,
  TOOLBAR_LOAD_GRAPH,
  TOOLBAR_SAVE_GRAPH,
  TOOLBAR_SAVE_GRAPH_AS,
  TOOLBAR_CREATE_SUBGRAPH,
  TOOLBAR_SAVE_SUBGRAPH,
  TOOLBAR_SAVE_SUBGRAPH_AS,
  FLOATING_NODE_CONTROLS_ID,
  FLOATING_GRAPH_CONTROLS_ID,
};

constexpr int FLOATING_TOOLBAR_OFFSET = 8;
constexpr int FLOATING_TOOLBAR_SPACING = 4; // the design's padding and gap are one size
constexpr int FLOATING_TOOLBAR_ROUNDING = 3;
constexpr int FLOATING_TOOLBAR_DIVIDER = 1;
constexpr int MAX_LIVE_PIN_ROUNDS = 8; // deeper than any real nesting of subgraphs

float scaled(int px) { return static_cast<float>(hdpi::_pxS(px)); }

// Must match ToolbarContainerPropertyControl::updateImgui in propPanel.
float toolbar_font_size() { return ImGui::GetStyle().FontSizeBase * DAGORED2->getToolbarScalePercent() * 0.01f; }

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

// The tabs held for undo, of one root or of all (null). Collected before use: removing their entries can free a holder.
void append_held_tabs(const eastl::vector<HeldSurfaces *> &held, const GraphTab *only_root, eastl::vector<GraphTab *> &out)
{
  for (const HeldSurfaces *hs : held)
  {
    if (!only_root || &hs->root == only_root)
    {
      hs->appendTabs(out);
    }
  }
}

// Every surface, in the UI and held. A copy: dropping entries can bury held ones while it is walked.
eastl::vector<GraphTab *> collect_all_surfaces(const eastl::vector<eastl::unique_ptr<GraphTab>> &tabs,
  const eastl::vector<HeldSurfaces *> &held)
{
  eastl::vector<GraphTab *> out;
  for_each_surface(tabs, [&out](GraphTab &t) { out.push_back(&t); });
  append_held_tabs(held, nullptr, out);
  return out;
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
    warn_dropped_instance_edges(path, n, dropped);
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

// The node block a status message names; null for the compile issues, or no message.
const eastl::string *status_message_block(const TexGenPipelineStatus &pipeline, GraphStatusFocus focus)
{
  switch (focus)
  {
    case GraphStatusFocus::ErrorNode: return &pipeline.lastErrorNode;
    case GraphStatusFocus::WarningNode: return &pipeline.lastWarningNode;
    default: return nullptr;
  }
}

eastl::vector<int> status_message_nodes(const GraphDocument &doc, const TexGenPipelineStatus &pipeline, GraphStatusFocus focus)
{
  eastl::vector<int> ids;
  const eastl::string *block = status_message_block(pipeline, focus);
  if (focus == GraphStatusFocus::CompileIssues)
  {
    for (const CompileNodeIssue &issue : doc.getCompileNodeIssues())
    {
      ids.push_back(issue.nodeId);
    }
  }
  else if (block && !block->empty())
  {
    ids.push_back(doc.blockNodeId(block->c_str()));
  }
  eastl::sort(ids.begin(), ids.end());
  ids.erase(eastl::unique(ids.begin(), ids.end()), ids.end());
  const GraphData &gd = doc.getGraphData();
  ids.erase(eastl::remove_if(ids.begin(), ids.end(), [&gd](int id) { return !find_node_by_id(gd, id); }), ids.end());
  return ids;
}
} // namespace

GraphPanel::GraphPanel(GraphEditorPlg &plg) : plugin(plg) {}

GraphPanel::~GraphPanel()
{
  // No undo entry may outlive the document it points into.
  queuedSurfaceMoves.clear();
  dropUndoOps();
  flushGraveyard(); // before the roots its windows point at
  graveyard.clear();
  graveyardTabs.clear();
  G_ASSERT(heldSurfaces.empty());
  tabs.clear();
  destroyWindow();
}

void GraphPanel::createWindow()
{
  G_ASSERT(!panelWindow);
  panelWindow = IEditorCoreEngine::get()->createPropPanel(this, "Graph");
  createToolbar();
}

void GraphPanel::destroyWindow()
{
  if (!panelWindow)
  {
    return;
  }
  toolbar = nullptr;
  for (PropPanel::ContainerPropertyControl *&group : floatingControls)
  {
    group = nullptr;
  }
  IEditorCoreEngine::get()->deleteCustomPanel(panelWindow);
  panelWindow = nullptr;
}

void GraphPanel::createToolbar()
{
  IEditorCommandSystem *commandSystem = EDITORCORE->queryEditorInterface<IEditorCommandSystem>();
  G_ASSERT(commandSystem);
  toolbar = panelWindow->createToolbarPanel(TOOLBAR_ID);
  const auto addButton = [&](int id, const char *command_id, const char *icon, const char *tooltip) {
    commandSystem->createToolbarButton(*toolbar, id, command_id, tooltip);
    toolbar->setButtonPictures(id, icon);
  };
  addButton(TOOLBAR_NEW_GRAPH, NEW_GRAPH, "create_graph", "Create new graph");
  addButton(TOOLBAR_LOAD_GRAPH, LOAD_GRAPH_BLK, "load_graph", "Load graph");
  addButton(TOOLBAR_SAVE_GRAPH, SAVE_GRAPH, "save_graph", "Save graph");
  addButton(TOOLBAR_SAVE_GRAPH_AS, SAVE_GRAPH_BLK, "save_graph_as", "Save graph as");
  toolbar->createSeparator();
  addButton(TOOLBAR_CREATE_SUBGRAPH, CANVAS_CREATE_SUBGRAPH, "create_subgraph", "Create subgraph");
  addButton(TOOLBAR_SAVE_SUBGRAPH, SAVE_SUBGRAPH, "save_subgraph", "Save subgraph");
  addButton(TOOLBAR_SAVE_SUBGRAPH_AS, SAVE_AS_SUBGRAPH_BLK, "save_subgraph_as", "Save subgraph as");

  floatingControls[0] = panelWindow->createToolbarPanel(FLOATING_NODE_CONTROLS_ID);
  floatingControls[1] = panelWindow->createToolbarPanel(FLOATING_GRAPH_CONTROLS_ID);
  plugin.fillFloatingToolbar(*floatingControls[0], *floatingControls[1]);
}

void GraphPanel::drawFloatingToolbar(const ImVec2 &canvas_min, const ImVec2 &canvas_max)
{
  if (!floatingControls[0])
  {
    return;
  }
  const float spacing = scaled(FLOATING_TOOLBAR_SPACING);
  const float divider = scaled(FLOATING_TOOLBAR_DIVIDER);
  const float rounding = scaled(FLOATING_TOOLBAR_ROUNDING);
  ImGui::PushFont(nullptr, toolbar_font_size());
  const float iconSize = ImGui::GetTextLineHeight();
  const ImVec2 button = PropPanel::ImguiHelper::getImageButtonSize(ImVec2(iconSize, iconSize)); // as a toolbar button sizes itself
  float height = spacing + static_cast<float>(eastl::size(floatingControls) - 1) * (divider + spacing);
  for (PropPanel::ContainerPropertyControl *group : floatingControls)
  {
    height += static_cast<float>(group->getChildCount()) * (button.y + spacing);
  }
  const ImVec2 boxSize(button.x + 2.0f * spacing, height);
  const ImVec2 boxMin(canvas_min.x + scaled(FLOATING_TOOLBAR_OFFSET), canvas_min.y + scaled(FLOATING_TOOLBAR_OFFSET));
  const ImVec2 boxMax(boxMin.x + boxSize.x, boxMin.y + boxSize.y);
  floatingToolbarTopRight = ImVec2(boxMax.x, boxMin.y);

  // A child, so the canvas under it takes no hover. It paints its own fill: ImGui would put a child's
  // background into the parent's list, under the canvas drawn after it.
  // A child clips to the parent's clip at Begin, so a canvas shorter than the box keeps it off the bars below.
  ImGui::PushClipRect(canvas_min, canvas_max, true);
  ImGui::SetCursorScreenPos(boxMin);
  if (ImGui::BeginChild("graph_floating_toolbar", boxSize, ImGuiChildFlags_None,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
  {
    ImDrawList &dl = *ImGui::GetWindowDrawList();
    dl.PushClipRect(canvas_min, canvas_max, false); // the shadow reaches past the child's own rect
    draw_soft_shadow(&dl, boxMin, boxMax, rounding);
    dl.PopClipRect();
    dl.AddRectFilled(boxMin, boxMax, FLOATING_TOOLBAR_BG_COLOR, rounding);

    ImGui::PushStyleColor(ImGuiCol_Button, FLOATING_TOOLBAR_BUTTON_COLOR);
    // A toolbar button also hovers with ButtonActive.
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, FLOATING_TOOLBAR_BUTTON_ACTIVE_COLOR);
    // A press that focused this child would take the keys from the canvas.
    ImGui::PushItemFlag(ImGuiItemFlags_NoFocus, true);
    float y = boxMin.y + spacing;
    for (PropPanel::ContainerPropertyControl *group : floatingControls)
    {
      if (group != floatingControls[0])
      {
        dl.AddRectFilled(ImVec2(boxMin.x + spacing, y), ImVec2(boxMax.x - spacing, y + divider), FLOATING_TOOLBAR_DIVIDER_COLOR);
        y += divider + spacing;
      }
      for (unsigned i = 0; i < group->getChildCount(); ++i)
      {
        PropPanel::PropertyControlBase *control = group->getByIndex(i);
        ImGui::SetCursorScreenPos(ImVec2(boxMin.x + spacing, y));
        ImGui::PushID(control);
        control->updateImgui();
        ImGui::PopID();
        y += button.y + spacing;
      }
    }
    // Last, so the buttons take the hover first: a press between them stays here.
    ImGui::SetCursorScreenPos(boxMin);
    ImGui::InvisibleButton("box", boxSize,
      ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    // The box cannot take a press on a disabled button, which ImGui would turn into focusing this child.
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemActive())
    {
      ImGui::SetActiveID(ImGui::GetID("floating_toolbar_column"), ImGui::GetCurrentWindow());
    }
    ImGui::PopItemFlag();
    ImGui::PopStyleColor(2);
  }
  ImGui::EndChild();
  ImGui::PopClipRect();
  ImGui::PopFont();
}

void GraphPanel::drawToolbar()
{
  if (!toolbar)
  {
    return;
  }
  GraphTab &surface = activeSurface();
  const bool inWindow = surface.getHostWindow() != nullptr;
  toolbar->setEnabledById(TOOLBAR_CREATE_SUBGRAPH, surface.getHotkeyContext().createSubgraphAvailable);
  toolbar->setEnabledById(TOOLBAR_SAVE_SUBGRAPH, inWindow);
  toolbar->setEnabledById(TOOLBAR_SAVE_SUBGRAPH_AS, inWindow);
  // The editor refreshes the shortcut in the tooltips of its own toolbars only, so a rebind would leave these stale.
  EDITORCORE->queryEditorInterface<IEditorCommandSystem>()->updateToolbarButtons(*panelWindow);
  const ImVec2 rowMin = ImGui::GetCursorScreenPos();
  // A press that focused this window would move the focus from a subgraph window to the root canvas.
  ImGui::PushItemFlag(ImGuiItemFlags_NoFocus, true);
  ImGui::PushFont(nullptr, toolbar_font_size());
  toolbar->updateImgui();
  ImGui::PopFont();
  ImGui::PopItemFlag();
  const ImVec2 rowMax(rowMin.x + ImGui::GetContentRegionAvail().x, ImGui::GetItemRectMax().y);
  // ImGui focuses the window on a press no item takes, a disabled button's too; an active id stops it.
  if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(rowMin, rowMax) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
      !ImGui::IsAnyItemActive())
  {
    ImGui::SetActiveID(ImGui::GetID("toolbar_row"), ImGui::GetCurrentWindow());
  }
  // The design puts as much space under the toolbar as above it, where the window padding is.
  const ImGuiStyle &style = ImGui::GetStyle();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.WindowPadding.y - style.ItemSpacing.y);
}

// PropPanel delivers a toolbar click between frames, where a dialog may open.
void GraphPanel::onClick(int pcb_id, PropPanel::ContainerPropertyControl * /*panel*/) { runToolbarCommand(pcb_id); }

void GraphPanel::runToolbarCommand(int id)
{
  GraphTab &surface = activeSurface();
  switch (id)
  {
    case TOOLBAR_NEW_GRAPH: openNewTab(); break;
    case TOOLBAR_LOAD_GRAPH: plugin.promptAndLoadGraphBlk(); break;
    // The graph buttons save the tab, also when a subgraph window over it has the focus.
    case TOOLBAR_SAVE_GRAPH: saveActiveGraph(); break;
    case TOOLBAR_SAVE_GRAPH_AS: plugin.saveGraphAs(activeTab()); break;
    // The canvas reads its selection later in the frame, like Ctrl+G.
    case TOOLBAR_CREATE_SUBGRAPH: surface.requestSubgraphFromSelection(); break;
    case TOOLBAR_SAVE_SUBGRAPH: saveFocusedSubgraph(); break;
    case TOOLBAR_SAVE_SUBGRAPH_AS:
      if (surface.getHostWindow())
      {
        plugin.saveAsSubgraphBlk(surface);
      }
      break;
  }
}

void GraphPanel::saveActiveGraph() { plugin.saveGraph(activeTab()); }

void GraphPanel::saveFocusedSubgraph()
{
  GraphTab &surface = activeSurface();
  if (surface.getHostWindow())
  {
    plugin.saveSubgraph(surface);
  }
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
  recordTabOpened(tab);
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
  recordTabOpened(tab);
  return &tab;
}

void GraphPanel::recordTabOpened(GraphTab &tab)
{
  // The first tab of an empty strip stands for the bootstrap one: undoing its open would empty the strip.
  if (tabs.size() > 1)
  {
    recordSurfaceLifetime(eastl::make_shared<HeldSurfaces>(*this, tab, nullptr), false);
  }
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
  SubgraphWindow &opened = showSubgraphWindow(root,
    makeSubgraphWindow(root, from.getHostWindow(), stem, root.getSubgraphWindowLayout(stem), eastl::move(loaded)));
  recordSurfaceLifetime(eastl::make_shared<HeldSurfaces>(*this, root, &opened), false);
}

eastl::unique_ptr<SubgraphWindow> GraphPanel::makeSubgraphWindow(GraphTab &root, SubgraphWindow *opened_from, const char *stem,
  const SubgraphWindowLayout &layout, GraphData graph)
{
  auto window = eastl::make_unique<SubgraphWindow>(plugin, nextTabId++, root, opened_from, stem, layout);
  window->getTab().getDocument().replaceGraphData([&graph](GraphData &gd) { gd = eastl::move(graph); });
  window->getTab().onGraphDataChanged();
  return window;
}

SubgraphWindow &GraphPanel::showSubgraphWindow(GraphTab &root, eastl::unique_ptr<SubgraphWindow> window)
{
  SubgraphWindow &shown = *window;
  root.getSubgraphWindows().push_back(eastl::move(window));
  focusSurface(root, &shown);
  return shown;
}

bool GraphPanel::confirmCloseWindowsOpenedFrom(GraphTab &surface)
{
  SubgraphWindow *const window = surface.getHostWindow();
  G_ASSERT_RETURN(window, false);
  for (SubgraphWindow *w : subgraph_window_tree(surface.getRootTab(), *window))
  {
    if (w != window && !confirmClose(w->getTab(), "Close subgraph"))
    {
      return false;
    }
  }
  return true;
}

bool GraphPanel::isSubgraphOpen(const char *stem) const { return findSurfaceByPath(plugin.getSubgraphPath(stem)) != nullptr; }

void GraphPanel::reopenSubgraphWindowAs(GraphTab &surface, const char *path)
{
  SubgraphWindow *const old = surface.getHostWindow();
  G_ASSERT_RETURN(old, );
  GraphTab &root = surface.getRootTab();
  const eastl::string stem = graph_file_stem(path);
  // Not loaded back: a load takes nested instances to their files, so a wire to a pin that is only live would go.
  GraphData written = surface.getDocument().getGraphData();
  written.sourcePath = path;
  auto window = makeSubgraphWindow(root, old->getParent(), stem.c_str(), old->getLayout(), eastl::move(written));
  const String entryName(0, "Save subgraph %s as %s", old->getStem().c_str(), stem.c_str());
  eastl::shared_ptr<HeldSurfaces> held = eastl::make_shared<HeldSurfaces>(*this, root, old);
  holdWindows(*held);
  SubgraphWindow &opened = showSubgraphWindow(root, eastl::move(window));
  recordSurfaceLifetime(eastl::move(held), true, eastl::make_shared<HeldSurfaces>(*this, root, &opened), entryName.str());
}

void GraphPanel::moveTabToFile(GraphTab &tab, const char *path)
{
  // By hand: the tab need not be the focused surface, nor this plugin the current one (a close-all prompt).
  UndoSystem *undo = EDITORCORE->getUndoSystem();
  undo->set_op_owner(tab.getUndoToken());
  tab.getDocument().recordGraphPath(path);
  // A reused token holds the save point of its last time on this file; mark the file just written.
  tab.markSaved();
  if (EDITORCORE->curPlugin() == static_cast<IGenEditorPlugin *>(&plugin))
  {
    stampUndoOwner();
  }
  else
  {
    undo->set_op_owner(EDITORCORE->curPlugin()); // what the engine stamps on a plugin switch
  }
}

void GraphPanel::recordLivePinSync()
{
  // Until queued holds and restores apply, the windows in the UI are not the ones the history says.
  if (!queuedSurfaceMoves.empty())
  {
    return;
  }
  OpenSubgraphs &open = plugin.getOpenSubgraphs();
  // The rounds change instance pins only, never which windows are open.
  eastl::vector<OpenSubgraphDoc> docs;
  collectOpenSubgraphs(docs);
  for (int round = 0; round < MAX_LIVE_PIN_ROUNDS; ++round)
  {
    open.refreshInterfaces(make_span_const(docs));
    bool changed = false;
    for_each_surface(tabs, [&open, &changed](GraphTab &t) { changed = t.recordLivePins(open) || changed; });
    if (!changed)
    {
      return;
    }
  }
  DAEDITOR3.conWarning("GraphEditor: subgraph instance pins still change after %d rounds", MAX_LIVE_PIN_ROUNDS);
}

eastl::vector<eastl::string> GraphPanel::stemsLeavingUi(const HeldSurfaces &surfaces) const
{
  eastl::vector<OpenSubgraphDoc> stillOpen;
  collectOpenSubgraphs(stillOpen);
  eastl::vector<eastl::string> stems;
  auto add = [&](const SubgraphWindow &w) {
    auto sameStem = [&w](const OpenSubgraphDoc &d) { return d.stem == w.getStem(); };
    if (eastl::none_of(stillOpen.begin(), stillOpen.end(), sameStem))
    {
      stems.push_back(w.getStem());
    }
  };
  for (const HeldSurfaces::HeldWindow &h : surfaces.held)
  {
    add(*h.window);
  }
  if (surfaces.heldRoot)
  {
    for (const eastl::unique_ptr<SubgraphWindow> &w : surfaces.heldRoot->getSubgraphWindows())
    {
      add(*w);
    }
  }
  return stems;
}

void GraphPanel::collectOpenSubgraphs(eastl::vector<OpenSubgraphDoc> &out) const
{
  out.clear();
  for (const eastl::unique_ptr<GraphTab> &root : tabs)
  {
    for (const eastl::unique_ptr<SubgraphWindow> &window : root->getSubgraphWindows())
    {
      const String path = plugin.getSubgraphPath(window->getStem().c_str());
      const GraphDocument &doc = window->getTab().getDocument();
      auto samePath = [&path](const OpenSubgraphDoc &d) { return d.path == path.str(); };
      if (!dd_fname_equal(doc.getGraphData().sourcePath.c_str(), path.str()) || eastl::any_of(out.begin(), out.end(), samePath))
      {
        continue;
      }
      out.push_back(OpenSubgraphDoc{window->getTab().getId(), window->getStem(), eastl::string(path.str()), &doc});
    }
  }
}

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
  EDITORCORE->getUndoSystem()->set_op_owner(owner.getUndoToken());
}

bool GraphPanel::ownsUndoToken(const void *token) const
{
  return find_surface(tabs, [token](GraphTab &t) { return t.hasUndoToken(token); }) != nullptr ||
         eastl::any_of(heldSurfaces.begin(), heldSurfaces.end(), [token](const HeldSurfaces *hs) { return hs->holdsTab(token); });
}

void GraphPanel::dropUndoOps()
{
  UndoSystem *undo = EDITORCORE->getUndoSystem();
  for (GraphTab *t : collect_all_surfaces(tabs, heldSurfaces))
  {
    t->dropUndoOps();
  }
  // The engine re-stamps its own token on each plugin switch, so an entry accepted between that
  // and our next frame carries it instead of the tab.
  undo->remove_ops_by_owner(static_cast<IGenEditorPlugin *>(&plugin));
}

// The ### suffix keeps each tab id stable while its label follows the file name.
void GraphPanel::drawTabStrip()
{
  // One frame without Reorderable makes ImGui sort the bar by submission order, which a restored tab needs.
  const bool resyncOrder = stripOrderResync;
  const ImGuiTabBarFlags barFlags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll |
                                    ImGuiTabBarFlags_NoTooltip | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton |
                                    (resyncOrder ? ImGuiTabBarFlags_None : ImGuiTabBarFlags_Reorderable);
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
  if (resyncOrder)
  {
    stripOrderResync = false;
  }
  else
  {
    syncTabOrderFromBar(make_span_const(imguiTabIds));
  }
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
  drawToolbar();
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
  drawFloatingToolbar(canvasMin, canvasMax); // after the windows, so it stays on top of them
  TexGenPipelineStatus pipeline;
  if (IGraphTexGenService *svc = plugin.getTexGenService())
  {
    pipeline = svc->getPipelineStatus();
  }
  root.getDocument().setMessagesCompileStamp(pipeline.nodeMessagesStamp);
  stampUndoOwner(root);
  root.updateCanvas(canvasMin, canvasMax, &pipeline.nodeMessages);
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
  // the design frames the focused surface only while windows exist to tell it from.
  if (!root.getFocusedWindow() && !root.getSubgraphWindows().empty())
  {
    draw_focus_frame(*ImGui::GetWindowDrawList(), canvasMin, canvasMax, 0.0f, pipeline.graphCompileFailed || pipeline.hasErrors);
  }
  // The bars describe the surface the keys go to: a window over this canvas, when one is focused.
  GraphTab &surface = activeSurface();
  draw_graph_hotkeys_bar(canvasMin, canvasMax, surface.getHotkeyContext(), stackWidth);
  ImGui::SetCursorScreenPos(ImVec2(canvasMin.x, canvasMax.y));
  const GraphStatusFocus statusFocus =
    draw_graph_status_bar(surface.getDocument().getGraphData(), pipeline, surface.getSelectedObjectCount(), statusBarHeight);
  // The message is about the graph the texgen service runs: the root's, even with a window focused.
  const eastl::string *block = status_message_block(pipeline, statusFocus);
  const SubgraphNodeSource *spliced = block && !pipeline.compilePending ? root.getDocument().blockSource(block->c_str()) : nullptr;
  if (!spliced || !focusSubgraphNode(root, *spliced))
  {
    eastl::vector<int> focusIds = status_message_nodes(root.getDocument(), pipeline, statusFocus);
    if (!focusIds.empty())
    {
      focusSurface(root, nullptr);
      // Every window is drawn over the root canvas, so the framed node shows only with all of them minimized.
      for (const eastl::unique_ptr<SubgraphWindow> &window : root.getSubgraphWindows())
      {
        if (!window->isMinimized())
        {
          minimizeSubgraphWindow(root, *window);
        }
      }
      root.requestFocusNodes(eastl::move(focusIds));
    }
  }
}

bool GraphPanel::hasPendingActions() const
{
  bool pending = pendingCloseTabId >= 0 || pendingWindowAction.tabId >= 0 || pendingActivateTabId >= 0 || plusRequested ||
                 !queuedSurfaceMoves.empty();
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

void GraphPanel::requestAddNode()
{
  if (panelWindow)
  {
    // The toolbar covers the canvas corner itself.
    activeSurface().requestAddNodeAt(ImVec2(floatingToolbarTopRight.x + scaled(FLOATING_TOOLBAR_OFFSET), floatingToolbarTopRight.y));
  }
}

void GraphPanel::invalidateDerivedState()
{
  for (GraphTab *t : collect_all_surfaces(tabs, heldSurfaces))
  {
    t->invalidateDerivedState();
  }
}

void GraphPanel::applyPendingCompileResults()
{
  // Fresh, so the drain keeps the block map of the compile the messages name now, not a frame ago.
  if (IGraphTexGenService *svc = plugin.getTexGenService(); svc && activeIndex >= 0)
  {
    tabs[activeIndex]->getDocument().setMessagesCompileStamp(svc->getNodeMessagesStamp());
  }
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
  // A queued move keeps its surfaces held, so it would keep them out of the graveyard.
  queuedSurfaceMoves.clear();
  dropUndoOps();
  flushGraveyard();
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

  const bool wasActive = idx == activeIndex;
  eastl::shared_ptr<HeldSurfaces> held = eastl::make_shared<HeldSurfaces>(*this, *tabs[idx], nullptr);
  holdRootTab(*held);
  // The strip is never empty: the last tab closing leaves a fresh untitled one, which undo takes away again.
  eastl::shared_ptr<HeldSurfaces> fresh;
  if (tabs.empty())
  {
    fresh = eastl::make_shared<HeldSurfaces>(*this, createTab(), nullptr);
  }
  recordSurfaceLifetime(eastl::move(held), true, eastl::move(fresh));
  settleSurfaces(nullptr, wasActive ? idx : -1);
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
  // the design keeps a maximized window behind the others, which it would hide.
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
  eastl::shared_ptr<HeldSurfaces> held = eastl::make_shared<HeldSurfaces>(*this, root, &window);
  holdWindows(*held);
  recordSurfaceLifetime(eastl::move(held), true);
}

void GraphPanel::recordSurfaceLifetime(eastl::shared_ptr<HeldSurfaces> surfaces, bool closing,
  eastl::shared_ptr<HeldSurfaces> then_opened, const char *entry_name)
{
  UndoSystem *undo = EDITORCORE->getUndoSystem();
  G_ASSERT_RETURN(!undo->is_holding(), );
  GraphTab &owner = surfaces->ownerTab();
  const eastl::vector<eastl::string> leaving = closing ? stemsLeavingUi(*surfaces) : eastl::vector<eastl::string>();
  const String entryName = entry_name ? String(entry_name) : surfaces->entryName(closing);
  stampUndoOwner(owner);
  undo->begin();
  undo->put<UndoSurfaceLifetime>(eastl::move(surfaces), closing);
  if (then_opened)
  {
    undo->put<UndoSurfaceLifetime>(eastl::move(then_opened), false);
  }
  for (const eastl::string &stem : leaving)
  {
    if (!find_node_desc(&plugin.getBaseNodesBlk(), (eastl::string("subgraph::") + stem).c_str()) && isSubgraphInstanced(stem.c_str()))
    {
      DAEDITOR3.conWarning("GraphEditor: subgraph '%s' has no valid file, its instances keep their pins", stem.c_str());
    }
  }
  // After the hold, so an undo gives the parents their live pins back before the window returns.
  if (!leaving.empty())
  {
    for_each_surface(tabs, [&leaving](GraphTab &t) { t.getDocument().revertLiveSubgraphInstances(leaving); });
  }
  // A reverted instance can rename an unnamed boundary of the window it sits in.
  recordLivePinSync();
  undo->accept(entryName.str());
  stampUndoOwner();
}

void GraphPanel::holdWindows(HeldSurfaces &surfaces)
{
  GraphTab &root = surfaces.root;
  const eastl::fixed_vector<SubgraphWindow *, 8> tree = subgraph_window_tree(root, *surfaces.top);
  auto inTree = [&tree](const SubgraphWindow *w) { return eastl::find(tree.begin(), tree.end(), w) != tree.end(); };
  if (inTree(root.getFocusedWindow()))
  {
    SubgraphWindow *const parent = nearest_shown_window(surfaces.top->getParent());
    focusSurface(root, parent);
    // focusSurface moves nothing on a root that is not shown.
    root.setFocusedWindow(parent);
  }
  eastl::vector<eastl::unique_ptr<SubgraphWindow>> &windows = root.getSubgraphWindows();
  // All detached before any leaves the list: a detach can commit an edit, whose live pin pass walks the windows.
  for (SubgraphWindow *w : tree)
  {
    root.setSubgraphWindowLayout(w->getStem().c_str(), w->getLayout());
    detachForHold(w->getTab());
  }
  for (int i = 0; i < static_cast<int>(windows.size()); ++i)
  {
    if (inTree(windows[i].get()))
    {
      surfaces.held.push_back(HeldSurfaces::HeldWindow{eastl::move(windows[i]), i});
    }
  }
  stampUndoOwner();
  windows.erase(eastl::remove(windows.begin(), windows.end(), nullptr), windows.end());
}

void GraphPanel::restoreWindows(HeldSurfaces &surfaces)
{
  eastl::vector<eastl::unique_ptr<SubgraphWindow>> &windows = surfaces.root.getSubgraphWindows();
  // Ascending, so each one lands where it was.
  for (HeldSurfaces::HeldWindow &h : surfaces.held)
  {
    h.window->getTab().onActivated();
    windows.insert(windows.begin() + eastl::min(h.listIndex, static_cast<int>(windows.size())), eastl::move(h.window));
  }
  surfaces.held.clear();
}

void GraphPanel::queueSurfaceMove(eastl::shared_ptr<HeldSurfaces> surfaces, bool hold)
{
  queuedSurfaceMoves.push_back(QueuedSurfaceMove{eastl::move(surfaces), hold});
}

void GraphPanel::applyQueuedSurfaceMoves()
{
  // An open operation would swallow entries recorded under it.
  if (EDITORCORE->getUndoSystem()->is_holding())
  {
    return;
  }
  flushGraveyard();
  if (queuedSurfaceMoves.empty())
  {
    return;
  }
  eastl::vector<QueuedSurfaceMove> moves;
  moves.swap(queuedSurfaceMoves);
  applyingSurfaceMove = true;
  HeldSurfaces *shown = nullptr;
  int heldActiveIndex = -1;
  // No activeTab() in here: the strip can be empty between two moves of one entry.
  for (const QueuedSurfaceMove &m : moves)
  {
    HeldSurfaces &surfaces = *m.surfaces;
    if (m.hold == surfaces.isHolding())
    {
      continue;
    }
    if (!surfaces.top)
    {
      if (m.hold)
      {
        const bool wasActive = activeIndex >= 0 && tabs[activeIndex].get() == &surfaces.root;
        const int index = holdRootTab(surfaces);
        if (wasActive)
        {
          heldActiveIndex = index;
        }
        else if (index >= 0 && index < heldActiveIndex)
        {
          --heldActiveIndex;
        }
      }
      else
      {
        restoreRootTab(surfaces);
      }
      shown = m.hold ? nullptr : &surfaces;
      continue;
    }
    const int rootIndex = indexOfTab(surfaces.root);
    G_ASSERT_CONTINUE(rootIndex >= 0);
    // Undo shows what it changes: the graph comes to the front first.
    setActiveIndex(rootIndex);
    if (m.hold)
    {
      holdWindows(surfaces);
    }
    else
    {
      restoreWindows(surfaces);
    }
    shown = m.hold ? nullptr : &surfaces;
  }
  settleSurfaces(shown, heldActiveIndex);
  applyingSurfaceMove = false;
  stampUndoOwner();
}

void GraphPanel::detachForHold(GraphTab &surface)
{
  // Whatever these record is the surface's own.
  stampUndoOwner(surface);
  surface.onDeactivated();
  plugin.beforeTabCloses(surface.getDocument());
  surface.dropPendingActions();
}

int GraphPanel::holdRootTab(HeldSurfaces &surfaces)
{
  const int index = indexOfTab(surfaces.root);
  if (index < 0)
  {
    return -1;
  }
  for_each_surface_of(surfaces.root, [this](GraphTab &t) { detachForHold(t); });
  surfaces.stripIndex = index;
  surfaces.heldRoot = eastl::move(tabs[index]);
  tabs.erase(tabs.begin() + index);
  if (activeIndex == index)
  {
    activeIndex = -1;
  }
  else if (activeIndex > index)
  {
    --activeIndex;
  }
  stampUndoOwner();
  return index;
}

void GraphPanel::restoreRootTab(HeldSurfaces &surfaces)
{
  const int index = eastl::min(surfaces.stripIndex, static_cast<int>(tabs.size()));
  tabs.insert(tabs.begin() + index, eastl::move(surfaces.heldRoot));
  if (activeIndex >= index)
  {
    ++activeIndex;
  }
  stripOrderResync = true;
}

void GraphPanel::settleSurfaces(HeldSurfaces *shown, int held_active_index)
{
  // Unreachable through undo; keeps the strip non-empty if it happens.
  G_ASSERT(!tabs.empty());
  if (tabs.empty())
  {
    createTab();
  }
  if (shown)
  {
    // What came back wins over a switch queued since the last tick.
    pendingActivateTabId = -1;
    setActiveIndex(indexOfTab(shown->root));
    if (shown->top && !shown->top->isMinimized())
    {
      focusSurface(shown->root, shown->top);
    }
  }
  else if (activeIndex < 0)
  {
    setActiveIndex(eastl::clamp(held_active_index, 0, static_cast<int>(tabs.size()) - 1));
  }
  // A restored tab is new to ImGui, and AutoSelectNewTabs would select it over the active one.
  if (stripOrderResync)
  {
    selectTabIdNextFrame = tabs[activeIndex]->getId();
  }
}

void GraphPanel::registerHeld(HeldSurfaces &surfaces) { heldSurfaces.push_back(&surfaces); }

void GraphPanel::unregisterHeld(HeldSurfaces &surfaces)
{
  heldSurfaces.erase(eastl::remove(heldSurfaces.begin(), heldSurfaces.end(), &surfaces), heldSurfaces.end());
}

void GraphPanel::bury(eastl::unique_ptr<SubgraphWindow> window) { graveyard.push_back(eastl::move(window)); }

void GraphPanel::buryTab(eastl::unique_ptr<GraphTab> tab) { graveyardTabs.push_back(eastl::move(tab)); }

void GraphPanel::flushGraveyard()
{
  UndoSystem *undo = EDITORCORE->getUndoSystem();
  if (undo->is_holding())
  {
    return;
  }
  // An entry recorded on a held surface outlives its holder when the history is trimmed. A dead root buries the
  // windows held for it: repeat until empty.
  while (!graveyard.empty() || !graveyardTabs.empty())
  {
    eastl::vector<eastl::unique_ptr<GraphTab>> deadTabs;
    deadTabs.swap(graveyardTabs);
    for (const eastl::unique_ptr<GraphTab> &t : deadTabs)
    {
      dropHeldWindowsOf(*t);
      for_each_surface_of(*t, [](GraphTab &s) { s.dropUndoOps(); });
    }
    eastl::vector<eastl::unique_ptr<SubgraphWindow>> deadWindows;
    deadWindows.swap(graveyard);
    for (const eastl::unique_ptr<SubgraphWindow> &w : deadWindows)
    {
      w->getTab().dropUndoOps();
    }
  }
}

void GraphPanel::dropHeldWindowsOf(GraphTab &root)
{
  queuedSurfaceMoves.erase(eastl::remove_if(queuedSurfaceMoves.begin(), queuedSurfaceMoves.end(),
                             [&root](const QueuedSurfaceMove &m) { return &m.surfaces->root == &root; }),
    queuedSurfaceMoves.end());
  eastl::vector<GraphTab *> heldTabs;
  append_held_tabs(heldSurfaces, &root, heldTabs);
  for (GraphTab *t : heldTabs)
  {
    t->dropUndoOps();
  }
}

GraphTab *GraphPanel::findSurface(int tab_id) const
{
  return find_surface(tabs, [tab_id](GraphTab &t) { return t.getId() == tab_id; });
}

bool GraphPanel::isSubgraphInstanced(const char *stem) const
{
  const eastl::vector<GraphTab *> surfaces = collect_all_surfaces(tabs, heldSurfaces);
  return eastl::any_of(surfaces.begin(), surfaces.end(), [stem](GraphTab *t) {
    const eastl::vector<GraphData::Node> &nodes = t->getDocument().getGraphData().nodes;
    return eastl::any_of(nodes.begin(), nodes.end(),
      [stem](const GraphData::Node &n) { return is_subgraph_instance(n) && n.descName == stem; });
  });
}

bool GraphPanel::isFileShownElsewhere(const char *path, const GraphTab &except) const
{
  const GraphTab *shown = findSurfaceByPath(path);
  return shown && shown != &except;
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

bool GraphPanel::focusSubgraphNode(GraphTab &root, const SubgraphNodeSource &source)
{
  const int queuedActivateTabId = pendingActivateTabId;
  GraphTab *surface = &root;
  for (const eastl::string &stem : source.stems)
  {
    openSubgraph(*surface, stem.c_str());
    surface = findSurfaceByPath(plugin.getSubgraphPath(stem.c_str()));
    if (!surface)
    {
      // A window found open on the way queued itself, and would take the focus from the caller's fallback.
      pendingActivateTabId = queuedActivateTabId;
      return false;
    }
  }
  // openSubgraph queues the activation of a window found open; the last surface must win.
  pendingActivateTabId = surface->getId();
  if (find_node_by_id(surface->getDocument().getGraphData(), source.nodeId))
  {
    surface->requestFocusNodes({source.nodeId});
  }
  return true;
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