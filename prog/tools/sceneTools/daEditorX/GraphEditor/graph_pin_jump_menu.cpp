// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_pin_jump_menu.h"

#include "graph_row_list.h"
#include "graph_theme.h"

#include <libTools/util/hdpiUtil.h>

#include <EASTL/algorithm.h>

#include <graphEditor/graph_data.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace
{
constexpr int MENU_WIDTH = 323;
constexpr int MENU_ROW_HEIGHT = 28;
constexpr int MENU_ROW_ROUNDING = 3;
constexpr int MENU_ROW_PAD_X = 8;
constexpr int MENU_COMMENT_DX = 126; // row-relative start of the comment column
constexpr int MENU_LABEL_FONT_SIZE = 16;
constexpr int MENU_COMMENT_FONT_SIZE = 12;
constexpr int MENU_MAX_VISIBLE_ROWS = 10;
// Placement offsets from the source pin centre.
constexpr int MENU_ANCHOR_DX = 21;
constexpr int MENU_ANCHOR_DY = 36;

constexpr ImU32 MENU_COMMENT_SELECTED_COLOR = IM_COL32(0xE1, 0xE1, 0xE1, 0xFF);

constexpr const char *MENU_POPUP_ID = "##graph_pin_jump";

void draw_row_text(ImDrawList *dl, const ImVec2 &row_min, float row_h, float x_from, float x_to, float font_size_base, ImU32 color,
  const eastl::string &text)
{
  // RenderTextEllipsis measures and draws with the current font, so the size has to be pushed, not
  // just used to place the baseline. PushFont takes an unscaled size and ImGui applies the global
  // factors on top, so the row centring reads the height back rather than reusing the argument.
  ImGui::PushFont(nullptr, font_size_base);
  const float y = row_min.y + (row_h - ImGui::GetFontSize()) * 0.5f;
  const ImVec2 textMin(row_min.x + x_from, y);
  // Clipped to the row, not to the font size, so a descender is not cut off.
  const ImVec2 textMax(row_min.x + x_to, row_min.y + row_h);
  ImGui::PushStyleColor(ImGuiCol_Text, color);
  ImGui::RenderTextEllipsis(dl, textMin, textMax, textMax.x, text.c_str(), text.c_str() + text.size(), nullptr);
  ImGui::PopStyleColor();
  ImGui::PopFont();
}
} // namespace

void GraphPinJumpMenu::collectRows(const GraphData &gd, int node_id, int pin_index)
{
  rows.clear();

  eastl::vector<PinEdge> pinEdges;
  collect_pin_edges(gd, node_id, pin_index, pinEdges);
  for (const PinEdge &pinEdge : pinEdges)
  {
    if (!pinEdge.oppositePinReachable)
    {
      continue;
    }
    Row &row = rows.push_back();
    row.choice.edgeId = pinEdge.edgeId;
    row.choice.node = pinEdge.oppositeNode;
    row.choice.pin = pinEdge.oppositePin;
    row.label = node_display_name(gd, pinEdge.oppositeNode);
    row.comment = pin_comment(gd, pinEdge.oppositeNode, pinEdge.oppositePin);
  }
}

GraphPinJumpMenu::Pick GraphPinJumpMenu::resolve(const GraphData &gd, Purpose menu_purpose, int node_id, int pin_index,
  uint64_t graph_revision, Choice &out_choice)
{
  close();
  purpose = menu_purpose;
  collectRows(gd, node_id, pin_index);
  if (rows.empty())
  {
    return Pick::None;
  }
  if (rows.size() == 1)
  {
    out_choice = rows[0].choice;
    rows.clear();
    return Pick::Single;
  }

  const GraphData::Node *const src = find_node_by_id(gd, node_id);
  sourceIsOutput = src && pin_index >= 0 && pin_index < static_cast<int>(src->pins.size()) && !src->pins[pin_index].isInput;
  sourceNode = node_id;
  sourcePin = pin_index;
  selected = 0;
  menuOpen = true;
  pendingPopupOpen = true;
  // The rows child is the same ImGui window every session and keeps its scroll, so ask for the
  // selection to be scrolled into view.
  scrollToSelected = true;
  revision = graph_revision;
  return Pick::MenuOpened;
}

void GraphPinJumpMenu::close()
{
  rows.clear();
  selected = 0;
  menuOpen = false;
  pendingPopupOpen = false;
  scrollToSelected = false;
  sourceNode = -1;
  sourcePin = -1;
}

int GraphPinJumpMenu::highlightedEdgeId() const
{
  if (!menuOpen || selected < 0 || selected >= static_cast<int>(rows.size()))
  {
    return -1;
  }
  return rows[selected].choice.edgeId;
}

bool GraphPinJumpMenu::draw(const GraphData &gd, uint64_t graph_revision, const ImVec2 &canvas_min, const ImVec2 &canvas_max,
  const ImVec2 &pin_screen_pos, Choice &out_choice)
{
  if (!menuOpen)
  {
    return false;
  }
  // Rows come from edges and pin comments, and every write to those bumps the structure revision,
  // so re-read the fan-out rather than close on it.
  if (graph_revision != revision)
  {
    const int keepEdgeId = highlightedEdgeId();
    revision = graph_revision;
    collectRows(gd, sourceNode, sourcePin);
    // One edge left is no longer a choice; the plain command takes over on the next press.
    if (rows.size() < 2)
    {
      close();
      return false;
    }
    selected = 0;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i)
    {
      if (rows[i].choice.edgeId == keepEdgeId)
      {
        selected = i;
        break;
      }
    }
    scrollToSelected = true; // the row it landed on may sit outside the scrolled window
  }

  const bool justOpened = pendingPopupOpen;
  if (pendingPopupOpen)
  {
    ImGui::OpenPopup(MENU_POPUP_ID);
    pendingPopupOpen = false;
  }

  const int rowCount = static_cast<int>(rows.size());
  const float rowH = static_cast<float>(hdpi::_pxS(MENU_ROW_HEIGHT));
  const float width = static_cast<float>(hdpi::_pxS(MENU_WIDTH));
  const float listH = rowH * eastl::min(rowCount, MENU_MAX_VISIBLE_ROWS);
  const ImVec2 pad = ImGui::GetStyle().WindowPadding;
  const ImVec2 size(width, listH + pad.y * 2.0f);

  const float anchorDx = static_cast<float>(hdpi::_pxS(MENU_ANCHOR_DX));
  const float anchorDy = static_cast<float>(hdpi::_pxS(MENU_ANCHOR_DY));
  ImVec2 pos(sourceIsOutput ? (pin_screen_pos.x + anchorDx - size.x) : (pin_screen_pos.x - anchorDx), pin_screen_pos.y + anchorDy);
  // Flip above the pin when it does not fit below, the way a dropdown does.
  if (pos.y + size.y > canvas_max.y)
  {
    pos.y = pin_screen_pos.y - anchorDy - size.y;
  }
  // ImGui does not clamp a position set through SetNextWindowPos, so keep the menu on the canvas.
  pos.x = ImClamp(pos.x, canvas_min.x, eastl::max(canvas_min.x, canvas_max.x - size.x));
  pos.y = ImClamp(pos.y, canvas_min.y, eastl::max(canvas_min.y, canvas_max.y - size.y));

  ImGui::SetNextWindowPos(pos);
  ImGui::SetNextWindowSize(size);
  // NoNavInputs: this menu reads the arrows and Tab itself.
  constexpr ImGuiWindowFlags POPUP_FLAGS = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavInputs;
  ImGui::PushStyleColor(ImGuiCol_PopupBg, GRAPH_POPUP_BG_COLOR);
  ImGui::PushStyleColor(ImGuiCol_Border, GRAPH_POPUP_BORDER_COLOR);
  const bool visible = ImGui::BeginPopup(MENU_POPUP_ID, POPUP_FLAGS);
  ImGui::PopStyleColor(2);
  if (!visible)
  {
    close(); // ImGui owns the open state; ours follows whatever closed it
    return false;
  }

  bool confirmed = false;
  bool cancelled = false;
  if (!justOpened)
  {
    const ImGuiIO &io = ImGui::GetIO();
    cancelled = ImGui::IsKeyPressed(ImGuiKey_Escape, ImGuiInputFlags_None, ImGuiKeyOwner_Any);
    if (!io.KeyCtrl && !io.KeyAlt && !io.KeySuper)
    {
      // Only the arrows repeat while held.
      const bool tab = ImGui::IsKeyPressed(ImGuiKey_Tab, ImGuiInputFlags_None, ImGuiKeyOwner_Any);
      if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) || (tab && !io.KeyShift))
      {
        selected = (selected + 1) % rowCount;
        scrollToSelected = true;
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || (tab && io.KeyShift))
      {
        selected = (selected + rowCount - 1) % rowCount;
        scrollToSelected = true;
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_Home, ImGuiInputFlags_None, ImGuiKeyOwner_Any))
      {
        selected = 0;
        scrollToSelected = true;
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_End, ImGuiInputFlags_None, ImGuiKeyOwner_Any))
      {
        selected = rowCount - 1;
        scrollToSelected = true;
      }
      confirmed = ImGui::IsKeyPressed(ImGuiKey_Enter, ImGuiInputFlags_None, ImGuiKeyOwner_Any) ||
                  ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, ImGuiInputFlags_None, ImGuiKeyOwner_Any) ||
                  ImGui::IsKeyPressed(ImGuiKey_Space, ImGuiInputFlags_None, ImGuiKeyOwner_Any);
    }
  }

  const float contentW = size.x - pad.x * 2.0f;
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f)); // design rows are contiguous
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  // The themed child fill is opaque and would hide the popup background.
  ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32_BLACK_TRANS);
  ImGui::BeginChild("rows", ImVec2(contentW, listH), ImGuiChildFlags_None, ImGuiWindowFlags_NoNavInputs);
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();

  // Rows are contiguous here, so the grid pitch is the row height.
  const CanvasRowList rowList = update_canvas_row_list(rowCount, rowH, justOpened, selected);
  const float rowW = rowList.rowWidth;
  const int hoveredRow = rowList.hoveredRow;
  confirmed = confirmed || rowList.clicked;

  ImDrawList *const dl = ImGui::GetWindowDrawList();
  // Unscaled, unlike the geometry below: ImGui multiplies a pushed font size by FontScaleMain and
  // FontScaleDpi, so scaling here would apply the factor twice.
  const float labelFontSize = static_cast<float>(MENU_LABEL_FONT_SIZE);
  const float commentFontSize = static_cast<float>(MENU_COMMENT_FONT_SIZE);
  const float padX = static_cast<float>(hdpi::_pxS(MENU_ROW_PAD_X));
  const float commentDx = static_cast<float>(hdpi::_pxS(MENU_COMMENT_DX));
  const float rounding = static_cast<float>(hdpi::_pxS(MENU_ROW_ROUNDING));

  for (int i = 0; i < rowCount; ++i)
  {
    const Row &row = rows[i];
    const ImVec2 rowMin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(contentW, rowH));
    if (scrollToSelected && i == selected)
    {
      ImGui::SetScrollHereY(0.5f);
    }

    const bool isSelected = i == selected;
    if (isSelected)
    {
      const ImU32 fill = (i == hoveredRow) ? GRAPH_MENU_ROW_HOVERED_COLOR : GRAPH_MENU_ROW_SELECTED_COLOR;
      dl->AddRectFilled(rowMin, ImVec2(rowMin.x + rowW, rowMin.y + rowH), fill, rounding);
    }
    draw_row_text(dl, rowMin, rowH, padX, commentDx, labelFontSize, GRAPH_MENU_LABEL_COLOR, row.label);
    if (!row.comment.empty())
    {
      draw_row_text(dl, rowMin, rowH, commentDx, rowW - padX, commentFontSize,
        isSelected ? MENU_COMMENT_SELECTED_COLOR : GRAPH_MENU_SECONDARY_COLOR, row.comment);
    }
  }
  scrollToSelected = false;

  ImGui::EndChild();
  ImGui::PopStyleVar();

  if (confirmed || cancelled)
  {
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();

  if (confirmed)
  {
    out_choice = rows[selected].choice;
  }
  if (confirmed || cancelled)
  {
    close();
  }
  return confirmed;
}
