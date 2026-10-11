// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_context_menu.h"

#include "command_definitions.h"
#include "graph_command_text.h"
#include "graph_theme.h"
#include "graph_validation.h"

#include <EditorCore/ec_interface.h>
#include <oldEditor/de_cm.h>

#include <libTools/util/hdpiUtil.h>

#include <EASTL/algorithm.h>

#include <graphEditor/graph_data.h>

#include <imgui/imgui.h>

namespace
{
constexpr int MENU_ROW_HEIGHT = 28;
constexpr int MENU_ROW_ROUNDING = 3;
constexpr int MENU_ROW_PAD_X = 8;
constexpr int MENU_COLUMN_GAP = 8;
constexpr int MENU_PANEL_PAD = 8;
constexpr int MENU_PANEL_ROUNDING = 3;
constexpr int MENU_LABEL_FONT_SIZE = 16;
constexpr int MENU_SHORTCUT_FONT_SIZE = 12;
constexpr int MENU_DIVIDER_BAND = 8; // height of a divider row, rule included

float scaled(int px) { return static_cast<float>(hdpi::_pxS(px)); }
} // namespace

void GraphContextMenu::beginMenu(const char *title, bool has_checkable_rows)
{
  entries.clear();
  targetEdges.clear();
  targetNode = -1;
  targetPin = -1;
  picked = Result();

  // The propPanel factory is host-only, so a plugin reaches it through the core interface -- the same
  // reason createDialog goes through there.
  menu = IEditorCoreEngine::get()->createContextMenu();
  menu->setEventHandler(this);

  PropPanel::MenuStyle style;
  style.checkmarkColumn = has_checkable_rows;
  style.rowPadX = scaled(MENU_ROW_PAD_X);
  style.secondaryFontSizeBase = static_cast<float>(MENU_SHORTCUT_FONT_SIZE); // unscaled: ImGui scales a font size
  style.rightAlignShortcut = true;
  style.submenuChevronArrow = true;
  style.separatorSpacingY = scaled(MENU_DIVIDER_BAND);
  menu->setStyle(style);

  menu->addLabel(PropPanel::ROOT_MENU_ITEM, title);
  menu->addSeparator(PropPanel::ROOT_MENU_ITEM);
}

unsigned GraphContextMenu::addRow(unsigned parent, const char *label, const char *shortcut, const Entry &entry, bool enabled)
{
  entries.push_back(entry);
  const unsigned id = static_cast<unsigned>(entries.size());

  String title(label);
  if (shortcut && *shortcut)
  {
    title.aprintf(0, "\t%s", shortcut);
  }
  menu->addItem(parent, id, title);
  if (!enabled)
  {
    menu->setEnabledById(id, false);
  }
  return id;
}

unsigned GraphContextMenu::addCommentRow(unsigned parent, const char *label, const char *comment, const Entry &entry)
{
  if (!comment || !*comment)
  {
    return addRow(parent, label, nullptr, entry);
  }
  entries.push_back(entry);
  const unsigned id = static_cast<unsigned>(entries.size());
  menu->addItemWithComment(parent, id, label, comment);
  return id;
}

unsigned GraphContextMenu::addSubMenuRow(unsigned parent, const char *label, const char *shortcut)
{
  // A container is never clicked, but its id must still be unique across the menu.
  entries.push_back(Entry());
  const unsigned id = static_cast<unsigned>(entries.size());

  String title(label);
  if (shortcut && *shortcut)
  {
    title.aprintf(0, "\t%s", shortcut);
  }
  menu->addSubMenu(parent, id, title);
  return id;
}

void GraphContextMenu::collectEdgeChoices(const GraphData &gd, const eastl::vector<PinEdge> &pin_edges, EdgeFilter filter,
  Action action, eastl::vector<EdgeChoice> &out) const
{
  out.clear();
  for (const PinEdge &pinEdge : pin_edges)
  {
    // Ahead of the filters: a removed destination leaves nothing to name, jump to or re-route.
    if (!pinEdge.oppositeNodeExists)
    {
      continue;
    }
    if (filter == EdgeFilter::Reachable && !pinEdge.oppositePinReachable)
    {
      continue;
    }

    EdgeChoice &choice = out.push_back();
    choice.label = node_display_name(gd, pinEdge.oppositeNode);
    choice.comment = pin_comment(gd, pinEdge.oppositeNode, pinEdge.oppositePin);
    choice.entry.action = action;
    choice.entry.edgeId = pinEdge.edgeId;
    choice.entry.detachNode = targetNode;
    choice.entry.detachPin = targetPin;
    if (pinEdge.oppositePinReachable)
    {
      choice.entry.jumpNode = pinEdge.oppositeNode;
      choice.entry.jumpPin = pinEdge.oppositePin;
    }
  }
}

void GraphContextMenu::addPinEdgeRow(const GraphData &gd, const eastl::vector<PinEdge> &pin_edges, const char *label,
  const char *shortcut, const char *all_label, Action action, EdgeFilter filter)
{
  eastl::vector<EdgeChoice> choices;
  collectEdgeChoices(gd, pin_edges, filter, action, choices);

  if (choices.empty())
  {
    Entry entry;
    entry.action = action;
    addRow(PropPanel::ROOT_MENU_ITEM, label, shortcut, entry, /*enabled=*/false);
    return;
  }
  // One destination is not a choice: the row stays a plain action on that single edge.
  if (choices.size() == 1)
  {
    addRow(PropPanel::ROOT_MENU_ITEM, label, shortcut, choices[0].entry);
    return;
  }

  const unsigned sub = addSubMenuRow(PropPanel::ROOT_MENU_ITEM, label, shortcut);
  if (all_label)
  {
    Entry all;
    all.action = action; // edgeId stays -1, i.e. every edge of the target
    addRow(sub, all_label, nullptr, all);
  }
  for (const EdgeChoice &choice : choices)
  {
    addCommentRow(sub, choice.label, choice.comment, choice.entry);
  }
}

void GraphContextMenu::openForNode(int node_id, int selected_node_count, bool can_paste, bool node_is_subgraph, bool subgraph_open)
{
  beginMenu(node_is_subgraph ? "Subgraph context menu" : "Node context menu");
  targetNode = node_id;

  if (node_is_subgraph)
  {
    addRow(PropPanel::ROOT_MENU_ITEM, subgraph_open ? "Focus" : "Open", command_hint(CANVAS_OPEN_SUBGRAPH),
      Entry{Action::OpenSubgraph});
    addRow(PropPanel::ROOT_MENU_ITEM, "Explode", command_hint(CANVAS_EXPLODE_SUBGRAPH), Entry{Action::ExplodeSubgraph});
    menu->addSeparator(PropPanel::ROOT_MENU_ITEM);
  }

  // A subgraph is folded out of a group of nodes, so the row only exists for a multi-node selection.
  if (selected_node_count > 1)
  {
    addRow(PropPanel::ROOT_MENU_ITEM, "Create a subgraph from the selection", command_hint(CANVAS_CREATE_SUBGRAPH),
      Entry{Action::CreateSubgraph});
    menu->addSeparator(PropPanel::ROOT_MENU_ITEM);
  }
  addRow(PropPanel::ROOT_MENU_ITEM, "Remove", command_hint(CANVAS_DELETE_SELECTED), Entry{Action::Remove});
  addRow(PropPanel::ROOT_MENU_ITEM, "Remove but keep connections", command_hint(CANVAS_REMOVE_KEEP_CONNECTIONS),
    Entry{Action::RemoveKeepConnections});
  menu->addSeparator(PropPanel::ROOT_MENU_ITEM);
  addRow(PropPanel::ROOT_MENU_ITEM, "Duplicate", command_hint(CANVAS_DUPLICATE), Entry{Action::Duplicate});
  addRow(PropPanel::ROOT_MENU_ITEM, "Copy", command_hint(CANVAS_COPY), Entry{Action::Copy});
  addRow(PropPanel::ROOT_MENU_ITEM, "Cut", command_hint(CANVAS_CUT), Entry{Action::Cut});
  addRow(PropPanel::ROOT_MENU_ITEM, "Paste", command_hint(CANVAS_PASTE), Entry{Action::Paste}, can_paste);
}

void GraphContextMenu::openForPin(const GraphData &gd, int node_id, int pin_index)
{
  beginMenu("Pin context menu");
  targetNode = node_id;
  targetPin = pin_index;

  // Collected once and handed to each row: the rows differ only in how they filter it.
  eastl::vector<PinEdge> pinEdges;
  collect_pin_edges(gd, node_id, pin_index, pinEdges);
  bool everyEdgeMuted = true;
  for (const PinEdge &pinEdge : pinEdges)
  {
    targetEdges.push_back(pinEdge.edgeId);
    everyEdgeMuted = everyEdgeMuted && pinEdge.muted;
  }
  const bool multiple = targetEdges.size() > 1;
  everyEdgeMuted = !targetEdges.empty() && everyEdgeMuted;

  addRow(PropPanel::ROOT_MENU_ITEM, "Add node", command_hint(CANVAS_ADD_NODE_AT_PIN), Entry{Action::AddNode});
  addRow(PropPanel::ROOT_MENU_ITEM, "Add transit node", command_hint(CANVAS_ADD_TRANSIT_NODE), Entry{Action::AddTransitNode},
    has_splice_target(gd, node_id, pin_index, /*anchor_edge_id=*/-1));
  menu->addSeparator(PropPanel::ROOT_MENU_ITEM);

  addPinEdgeRow(gd, pinEdges, multiple ? "Remove edges" : "Remove edge", command_hint(CANVAS_REMOVE_EDGES_AT_PIN), "Remove all",
    Action::RemoveEdges, EdgeFilter::Any);

  // The design shows only the mute row, which an all-muted pin could not act on.
  if (everyEdgeMuted)
  {
    addPinEdgeRow(gd, pinEdges, multiple ? "Unmute edges" : "Unmute edge", "Second DClick", "Unmute all", Action::UnmuteEdges,
      EdgeFilter::Any);
  }
  else
  {
    addPinEdgeRow(gd, pinEdges, multiple ? "Mute edges" : "Mute edge", "DClick", "Mute all", Action::MuteEdges, EdgeFilter::Any);
  }

  addPinEdgeRow(gd, pinEdges, multiple ? "Re-route edges" : "Re-route edge", command_hint(CANVAS_MODIFY_EDGE_AT_PIN), nullptr,
    Action::RerouteEdge, EdgeFilter::Reachable);
  menu->addSeparator(PropPanel::ROOT_MENU_ITEM);
  addPinEdgeRow(gd, pinEdges, "Jump to the opposite pin", command_hint(CANVAS_JUMP_OPPOSITE_PIN), nullptr, Action::JumpToOppositePin,
    EdgeFilter::Reachable);
}

void GraphContextMenu::openForEdges(const GraphData &gd, const eastl::vector<int> &edge_ids)
{
  bool anyMuted = false;
  bool anyEnabled = false;
  for (int edgeId : edge_ids)
  {
    if (const GraphData::Edge *const e = find_edge_by_id(gd, edgeId))
    {
      anyMuted = anyMuted || e->muted;
      anyEnabled = anyEnabled || !e->muted;
    }
  }

  if (edge_ids.size() == 1)
  {
    beginMenu("Edge context menu");
    targetEdges = edge_ids;
    const GraphData::Edge *const e = find_edge_by_id(gd, edge_ids[0]);
    // Add node hangs the new node off the source pin, so an edge with no resolvable one cannot act.
    const bool canAddNode = e && edge_source_pin(gd, *e, targetNode, targetPin);
    addRow(PropPanel::ROOT_MENU_ITEM, "Add node", command_hint(CANVAS_ADD_NODE_AT_PIN), Entry{Action::AddNodeAtEdge}, canAddNode);
    addRow(PropPanel::ROOT_MENU_ITEM, "Add transit node", command_hint(CANVAS_ADD_TRANSIT_NODE), Entry{Action::AddTransitNodeAtEdge},
      canAddNode && has_splice_target(gd, targetNode, targetPin, edge_ids[0]));
    menu->addSeparator(PropPanel::ROOT_MENU_ITEM);
    // Not the pin menu's X: that command needs a pin under the cursor and does nothing over an edge.
    // Right-clicking an edge selects it, so Delete is the key that really removes it.
    addRow(PropPanel::ROOT_MENU_ITEM, "Remove edge", command_hint(CANVAS_DELETE_SELECTED), Entry{Action::RemoveEdges});
    if (anyMuted)
    {
      addRow(PropPanel::ROOT_MENU_ITEM, "Unmute edge", "Second DClick", Entry{Action::UnmuteEdges});
    }
    else
    {
      addRow(PropPanel::ROOT_MENU_ITEM, "Mute edge", "DClick", Entry{Action::MuteEdges});
    }

    // The choice is which end to detach.
    if (!e)
    {
      addRow(PropPanel::ROOT_MENU_ITEM, "Re-route edge", "Click and move", Entry{Action::RerouteEdge}, /*enabled=*/false);
      return;
    }
    const unsigned sub = addSubMenuRow(PropPanel::ROOT_MENU_ITEM, "Re-route edge", "Click and move");
    const int endNodes[2] = {e->elemA, e->elemB};
    const int endPins[2] = {e->pinA, e->pinB};
    for (int i = 0; i < 2; ++i)
    {
      Entry entry;
      entry.action = Action::RerouteEdge;
      entry.edgeId = e->id;
      entry.detachNode = endNodes[i];
      entry.detachPin = endPins[i];
      addCommentRow(sub, node_display_name(gd, endNodes[i]), pin_comment(gd, endNodes[i], endPins[i]), entry);
    }
    return;
  }

  // A mixed selection offers both directions; either drives the whole selection to that state.
  beginMenu("Edges context menu");
  targetEdges = edge_ids;
  addRow(PropPanel::ROOT_MENU_ITEM, "Remove edges", command_hint(CANVAS_DELETE_SELECTED), Entry{Action::RemoveEdges});
  if (anyEnabled)
  {
    addRow(PropPanel::ROOT_MENU_ITEM, "Mute edges", "DClick", Entry{Action::MuteEdges});
  }
  if (anyMuted)
  {
    addRow(PropPanel::ROOT_MENU_ITEM, "Unmute edges", "Second DClick", Entry{Action::UnmuteEdges});
  }
}

void GraphContextMenu::openForBackground(bool auto_update_on)
{
  beginMenu("Graph context menu", /*has_checkable_rows=*/true);
  addRow(PropPanel::ROOT_MENU_ITEM, "Add node", "Space", Entry{Action::AddNode}, /*enabled=*/false);
  menu->addSeparator(PropPanel::ROOT_MENU_ITEM);
  // The design draws no mark, but a toggle that does not show its state leaves the user guessing.
  const unsigned autoUpdateRow =
    addRow(PropPanel::ROOT_MENU_ITEM, "Toggle auto-update", command_hint(CANVAS_TOGGLE_AUTOUPDATE), Entry{Action::ToggleAutoUpdate});
  menu->setCheckById(autoUpdateRow, auto_update_on);
  addRow(PropPanel::ROOT_MENU_ITEM, "Force rebuild", command_hint(FORCE_REBUILD), Entry{Action::ForceRebuild});
  menu->addSeparator(PropPanel::ROOT_MENU_ITEM);
  addRow(PropPanel::ROOT_MENU_ITEM, "Select all", command_hint(EditorCommandIds::SELECT_ALL), Entry{Action::SelectAll});
}

int GraphContextMenu::onMenuItemClick(unsigned id)
{
  if (id == 0 || id > entries.size())
  {
    return 0;
  }
  const Entry &entry = entries[id - 1];
  picked = Result();
  picked.action = entry.action;
  if (entry.edgeId >= 0)
  {
    picked.edges.push_back(entry.edgeId);
  }
  else
  {
    picked.edges = targetEdges;
  }
  picked.detachNode = entry.detachNode;
  picked.detachPin = entry.detachPin;
  picked.jumpNode = entry.jumpNode;
  picked.jumpPin = entry.jumpPin;
  picked.targetNode = targetNode;
  picked.targetPin = targetPin;
  return 1;
}

void GraphContextMenu::updateImgui()
{
  if (!menu)
  {
    return;
  }

  // ImGui::Selectable grows its rect by ItemSpacing.y, so text height plus spacing is the row height
  // and rows come out contiguous. The height is on-screen pixels, hence scaled().
  const float rowSpacingY = eastl::max(0.0f, scaled(MENU_ROW_HEIGHT) - scaled(MENU_LABEL_FONT_SIZE));

  // Unscaled: PushFont takes a base size and ImGui applies FontScaleMain / FontScaleDpi on top.
  ImGui::PushFont(nullptr, static_cast<float>(MENU_LABEL_FONT_SIZE));
  ImGui::PushStyleColor(ImGuiCol_PopupBg, GRAPH_POPUP_BG_COLOR);
  ImGui::PushStyleColor(ImGuiCol_Border, GRAPH_POPUP_BORDER_COLOR);
  ImGui::PushStyleColor(ImGuiCol_Separator, GRAPH_POPUP_BORDER_COLOR);
  ImGui::PushStyleColor(ImGuiCol_Text, GRAPH_MENU_LABEL_COLOR);
  ImGui::PushStyleColor(ImGuiCol_TextDisabled, GRAPH_MENU_SECONDARY_COLOR);
  // Header is only reached by a submenu parent whose submenu stands.
  ImGui::PushStyleColor(ImGuiCol_Header, GRAPH_MENU_ROW_SUBMENU_OPEN_COLOR);
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, GRAPH_MENU_ROW_HOVERED_COLOR);
  ImGui::PushStyleColor(ImGuiCol_HeaderActive, GRAPH_MENU_ROW_HOVERED_COLOR);
  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, scaled(MENU_PANEL_ROUNDING));
  ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, eastl::max(1.0f, scaled(1)));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(scaled(MENU_PANEL_PAD), scaled(MENU_PANEL_PAD)));
  // x is the label-to-shortcut gap and, halved, how far a row's fill bleeds past the panel padding.
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(scaled(MENU_COLUMN_GAP), rowSpacingY));
  ImGui::PushStyleVar(ImGuiStyleVar_SelectableRounding, scaled(MENU_ROW_ROUNDING));

  // renderContextMenu stays true until the menu has closed AND delivered its click.
  const bool stillOpen = IEditorCoreEngine::get()->renderContextMenu(*menu);

  ImGui::PopStyleVar(5);
  ImGui::PopStyleColor(8);
  ImGui::PopFont();

  if (!stillOpen)
  {
    menu.reset();
  }
}

GraphContextMenu::Result GraphContextMenu::takePicked()
{
  Result result = eastl::move(picked);
  picked = Result();
  return result;
}
