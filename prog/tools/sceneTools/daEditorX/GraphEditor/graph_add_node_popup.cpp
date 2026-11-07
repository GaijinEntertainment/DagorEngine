// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_add_node_popup.h"

#include "base_node_descriptors.h"
#include "graph_node_render.h" // add_node_ghost_body_box
#include "graph_row_list.h"
#include "graph_theme.h"
#include "graph_validation.h"
#include "plugin.h"

#include <de3_interface.h>
#include <libTools/util/hdpiUtil.h>

#include <propPanel/imguiWidgetService.h>

#include <EASTL/algorithm.h>

#include <graphEditor/graph_data.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui_node_editor.h>

namespace ne = ax::NodeEditor;

namespace
{
constexpr int POPUP_WIDTH = 270;
constexpr int POPUP_PADDING = 8;
constexpr int POPUP_ROUNDING = 3;

constexpr int FIELD_HEIGHT = 28;
constexpr int FIELD_PAD_X = 12;
constexpr int FIELD_ROUNDING = 3;
constexpr int FIELD_FOCUS_RING = 2;
constexpr int FIELD_FONT_SIZE = 16;

constexpr int ROW_HEIGHT = 20;
constexpr int ROW_ROUNDING = 3;
constexpr int ROW_PAD_X = 4;
constexpr int ROW_SPACING = 2;
constexpr int ROW_FONT_SIZE = 12;
constexpr int MAX_VISIBLE_ROWS = 8;

constexpr const char *POPUP_ID = "##graph_add_node";
constexpr const char *SEARCH_HINT = "Search for node";
} // namespace

void GraphAddNodePopup::openAt(const GraphData &gd, GraphEditorPlg &plugin, int source_node, int source_pin,
  const ImVec2 &drop_canvas_pos, const ImVec2 &canvas_min, const ImVec2 &canvas_max, bool splice_into_connections, int anchor_edge)
{
  close();

  if (!iconsLoaded)
  {
    PropPanel::IPropPanelService *service = DAEDITOR3.getPropPanelService();
    searchIcon = service->loadIcon("search");
    clearIcon = service->loadIcon("close_editor");
    iconsLoaded = true;
  }

  const GraphData::Pin *const sourcePinDesc = source_node >= 0 ? find_pin(gd, source_node, source_pin) : nullptr;
  if (source_node >= 0 && !sourcePinDesc)
  {
    return;
  }

  sourceNode = source_node;
  sourcePin = source_pin;
  // With no pin the ghost hangs right of the drop point, as an output's does.
  srcIsOutput = !sourcePinDesc || sourcePinDesc->role == PinRole::Out;
  splice = splice_into_connections;
  anchorEdge = anchor_edge;

  // A splice off an In anchor is fed by that pin's driver, so that is what the offer list must match.
  SpliceEnds ends;
  ends.feed = SplicePin{source_node, source_pin};
  if (splice)
  {
    splice_ends(gd, source_node, source_pin, anchor_edge, ends);
  }

  dropCanvas = clamp_add_node_drop_point(srcIsOutput, drop_canvas_pos, canvas_min, canvas_max);

  for_each_node_descriptor(plugin.getBaseNodesBlk(), [&](const DataBlock &desc) {
    if (!node_desc_is_offerable(desc))
    {
      return;
    }
    const char *const uid = desc.getStr("templateUid", "");
    if (sourcePinDesc)
    {
      // A node made and never inserted: making it resolves the pin types from the descriptor, which can_node_splice reads.
      GraphData::Node candidate;
      if (!plugin.makeNodeFromBaseBlk(uid, 0.0f, 0.0f, candidate))
      {
        return;
      }
      // A transit has to pass the wire on, which is what keeps a one-sided node like `subgraph out`
      // off the list.
      if (!can_node_splice(gd, ends, candidate))
      {
        return;
      }
    }

    Row &row = offerable.push_back();
    row.uid = uid;
    row.name = desc.getStr("name", "");
    row.synonyms = desc.getStr("synonyms", "");
  });

  menuOpen = true;
  pendingPopupOpen = true;
  focusRequested = true;
}

void GraphAddNodePopup::close()
{
  offerable.clear();
  rows.clear();
  searchText = "";
  searchInputFocused = false;
  focusRequested = false;
  scrollToSelected = false;
  selected = 0;
  menuOpen = false;
  pendingPopupOpen = false;
  sourceNode = -1;
  sourcePin = -1;
  srcIsOutput = false;
  splice = false;
  anchorEdge = -1;
}

void GraphAddNodePopup::rebuildRows()
{
  rows.clear();
  selected = 0;
  scrollToSelected = true;

  if (searchText.empty())
  {
    return;
  }

  for (int i = 0; i < static_cast<int>(offerable.size()); ++i)
  {
    const Row &row = offerable[i];
    if (node_name_matches_search(row.name.c_str(), row.synonyms.c_str(), searchText.str()))
    {
      rows.push_back(i);
    }
  }
}

bool GraphAddNodePopup::draw(const GraphData &gd, const ImVec2 &canvas_min, const ImVec2 &canvas_max, eastl::string &out_template_uid)
{
  if (!menuOpen)
  {
    return false;
  }

  if (sourceNode >= 0 && !is_pin_reachable(gd, sourceNode, sourcePin))
  {
    close();
    return false;
  }

  const bool justOpened = pendingPopupOpen;
  if (pendingPopupOpen)
  {
    ImGui::OpenPopup(POPUP_ID);
    pendingPopupOpen = false;
  }

  const float width = static_cast<float>(hdpi::_pxS(POPUP_WIDTH));
  const float pad = static_cast<float>(hdpi::_pxS(POPUP_PADDING));
  const float fieldH = static_cast<float>(hdpi::_pxS(FIELD_HEIGHT));
  const float rowH = static_cast<float>(hdpi::_pxS(ROW_HEIGHT));
  const float rowSpacing = static_cast<float>(hdpi::_pxS(ROW_SPACING));

  // Sized from the rows the previous frame settled on: the window has to be committed before the
  // search field runs, and that field is what changes the row set. So the box lags one frame while
  // everything below reads the live count.
  const int visibleRows = eastl::min(static_cast<int>(rows.size()), MAX_VISIBLE_ROWS);
  const float listH = visibleRows > 0 ? (visibleRows * rowH + (visibleRows - 1) * rowSpacing + pad * 2.0f) : 0.0f;
  const float listGap = visibleRows > 0 ? ImGui::GetStyle().ItemSpacing.y : 0.0f;
  const ImVec2 size(width, pad * 2.0f + fieldH + listGap + listH);

  // Hung off the ghost's outward edge so it never covers the preview. An input-source ghost sits
  // left of the drop point, so the box grows leftward from that edge rather than back across it.
  ImVec2 ghostMin, ghostMax;
  add_node_ghost_body_box(srcIsOutput, ghostMin, ghostMax);
  const ImVec2 dropScreen = ne::CanvasToScreen(dropCanvas);
  const float outwardX = srcIsOutput ? ghostMax.x : ghostMin.x - size.x;
  ImVec2 pos(dropScreen.x + outwardX, dropScreen.y + ghostMax.y);
  if (pos.y + size.y > canvas_max.y)
  {
    pos.y -= size.y;
  }
  pos.x = ImClamp(pos.x, canvas_min.x, eastl::max(canvas_min.x, canvas_max.x - size.x));
  pos.y = ImClamp(pos.y, canvas_min.y, eastl::max(canvas_min.y, canvas_max.y - size.y));

  ImGui::SetNextWindowPos(pos);
  ImGui::SetNextWindowSize(size);

  constexpr ImGuiWindowFlags POPUP_FLAGS = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoNavInputs;
  // The canvas pass pushed the menu popup colours for its whole span; this panel differs in both.
  ImGui::PushStyleColor(ImGuiCol_PopupBg, GRAPH_ADD_NODE_POPUP_BG_COLOR);
  ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_BLACK_TRANS);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad, pad));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, static_cast<float>(hdpi::_pxS(POPUP_ROUNDING)));
  const bool visible = ImGui::BeginPopup(POPUP_ID, POPUP_FLAGS);
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(2);
  if (!visible)
  {
    close(); // ImGui owns the open state; ours follows whatever closed it
    return false;
  }

  // Once, on the frame the popup opens: a nav-activated field select-alls, so asking again would let
  // the next keystroke wipe the query.
  if (focusRequested)
  {
    ImGui::SetKeyboardFocusHere();
  }
  focusRequested = false;

  PropPanel::IImguiWidgetService &widgets = DAEDITOR3.getPropPanelService()->getImguiWidgets();
  const float contentW = size.x - pad * 2.0f;

  // searchInput adds ItemInnerSpacing.x plus the icon width to FramePadding.x for the magnifier,
  // which is what puts the icon at the design's 12 and the text after it.
  ImGui::PushStyleColor(ImGuiCol_FrameBg, GRAPH_SEARCH_FIELD_BG_COLOR);
  ImGui::PushStyleColor(ImGuiCol_Border, GRAPH_SEARCH_FIELD_BORDER_COLOR);
  ImGui::PushStyleColor(ImGuiCol_Text, GRAPH_MENU_LABEL_COLOR);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, static_cast<float>(hdpi::_pxS(FIELD_ROUNDING)));
  ImGui::PushFont(nullptr, static_cast<float>(FIELD_FONT_SIZE));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
    ImVec2(static_cast<float>(hdpi::_pxS(FIELD_PAD_X)), (fieldH - ImGui::GetFontSize()) * 0.5f));
  ImGui::SetNextItemWidth(contentW);
  ImRect fieldRect;
  const bool textChanged = widgets.searchInput(&searchText, "##addNodeSearch", SEARCH_HINT, searchText, searchIcon, clearIcon,
    &searchInputFocused, /*input_id=*/nullptr, &fieldRect);
  ImGui::PopStyleVar(1);
  ImGui::PopFont();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(3);

  // searchInput reports the clear button as a change too, so this covers every path into the text.
  if (textChanged)
  {
    rebuildRows();
  }

  // Read after the rebuild, so the count the arrows wrap on and the row Enter takes are the live
  // ones. KeyOwner_Any because the active field owns the keyboard otherwise, and Escape would take
  // two presses -- one to revert the field, one to reach us.
  const int rowCount = static_cast<int>(rows.size());
  bool confirmed = false;
  bool cancelled = false;
  if (!justOpened)
  {
    cancelled = ImGui::IsKeyPressed(ImGuiKey_Escape, ImGuiInputFlags_None, ImGuiKeyOwner_Any);
    if (rowCount > 0)
    {
      if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, ImGuiInputFlags_Repeat, ImGuiKeyOwner_Any))
      {
        selected = (selected + 1) % rowCount;
        scrollToSelected = true;
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, ImGuiInputFlags_Repeat, ImGuiKeyOwner_Any))
      {
        selected = (selected + rowCount - 1) % rowCount;
        scrollToSelected = true;
      }
      // Space is not a confirm here, unlike the pin jump menu: it is a character being typed.
      confirmed = ImGui::IsKeyPressed(ImGuiKey_Enter, ImGuiInputFlags_None, ImGuiKeyOwner_Any) ||
                  ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, ImGuiInputFlags_None, ImGuiKeyOwner_Any);
    }
  }

  ImDrawList *const dl = ImGui::GetWindowDrawList();

  // An active InputText renders no nav highlight, so the ring has no widget state to come from.
  if (searchInputFocused)
  {
    const float ring = static_cast<float>(hdpi::_pxS(FIELD_FOCUS_RING));
    dl->AddRect(ImVec2(fieldRect.Min.x - ring, fieldRect.Min.y - ring), ImVec2(fieldRect.Max.x + ring, fieldRect.Max.y + ring),
      GRAPH_SEARCH_FIELD_FOCUS_COLOR, static_cast<float>(hdpi::_pxS(FIELD_ROUNDING)) + ring, ImDrawFlags_None, ring);
  }

  if (rowCount > 0)
  {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, rowSpacing));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad, pad));
    // The themed child fill is opaque and would hide the popup background.
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32_BLACK_TRANS);
    // ItemSpacing stays pushed until after EndChild: popping it here lays the rows out on the
    // editor's default 3px while the hover grid and listH both assume ROW_SPACING.
    // AlwaysUseWindowPadding: a borderless child otherwise has its WindowPadding forced to zero, and
    // listH has already reserved it.
    // Floored at a row: on the frame a query first matches, listH still carries the lagged zero, and
    // a zero height would make the child take "all available" -- which is nothing.
    const float childH = eastl::max(listH, rowH + pad * 2.0f);
    ImGui::BeginChild("rows", ImVec2(contentW, childH), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoNavInputs);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(); // WindowPadding; ItemSpacing is popped after EndChild

    const CanvasRowList rowList = update_canvas_row_list(rowCount, rowH + rowSpacing, justOpened, selected);
    const float rowW = rowList.rowWidth;
    confirmed = confirmed || rowList.clicked;

    const float padX = static_cast<float>(hdpi::_pxS(ROW_PAD_X));
    const float rounding = static_cast<float>(hdpi::_pxS(ROW_ROUNDING));
    ImGui::PushFont(nullptr, static_cast<float>(ROW_FONT_SIZE));
    ImGui::PushStyleColor(ImGuiCol_Text, GRAPH_MENU_LABEL_COLOR);
    for (int i = 0; i < rowCount; ++i)
    {
      const ImVec2 rowMin = ImGui::GetCursorScreenPos();
      ImGui::Dummy(ImVec2(rowW, rowH));
      if (i == selected)
      {
        if (scrollToSelected)
        {
          ImGui::SetScrollHereY(0.5f);
        }
        dl->AddRectFilled(rowMin, ImVec2(rowMin.x + rowW, rowMin.y + rowH), NODE_PLATE_FILL_COLOR, rounding);
      }

      const ImVec2 textMin(rowMin.x + padX, rowMin.y + (rowH - ImGui::GetFontSize()) * 0.5f);
      // Clipped to the row, not the font size, so a descender survives.
      const ImVec2 textMax(rowMin.x + rowW - padX, rowMin.y + rowH);
      const eastl::string &label = offerable[rows[i]].name;
      ImGui::RenderTextEllipsis(dl, textMin, textMax, textMax.x, label.c_str(), label.c_str() + label.size(), nullptr);
    }
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::EndChild();
    ImGui::PopStyleVar(); // ItemSpacing
  }
  scrollToSelected = false;

  if (confirmed || cancelled)
  {
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();

  if (confirmed)
  {
    out_template_uid = offerable[rows[selected]].uid;
  }
  if (confirmed || cancelled)
  {
    close();
  }
  return confirmed;
}
