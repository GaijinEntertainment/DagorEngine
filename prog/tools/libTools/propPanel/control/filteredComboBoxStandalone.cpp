// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "filteredComboBoxStandalone.h"
#include <gui/dag_imguiUtil.h>

#include <imgui/imgui_internal.h>

namespace PropPanel
{

bool FilteredComboBoxStandalone::beginCombo(const char *label, const char *preview_text, int unfiltered_selected_index)
{
  const ImVec2 comboPreviewStartPos = ImGui::GetCursorScreenPos();

  if (ImGui::BeginCombo(label, preview_text))
  {
    ImGui::CloseCurrentPopup();
    ImGui::EndCombo();

    dropdownOpenState.reset(new DropdownOpenState(unfiltered_selected_index, ImGui::GetItemRectSize()));
    ImGui::OpenPopupEx(dropdownOpenState->getPopupId());
  }

  if (!dropdownOpenState)
    return false;

  if (!ImGui::IsPopupOpen(dropdownOpenState->getPopupId(), ImGuiPopupFlags_None))
  {
    dropdownOpenState.reset();
    return false;
  }

  dropdownOpenState->beginCombo(comboPreviewStartPos);

  return true;
}

int FilteredComboBoxStandalone::endCombo()
{
  DropdownOpenState *dropdownState = dropdownOpenState.get();
  G_ASSERT_RETURN(dropdownState, -1);

  const bool dropdownOpen = dropdownState->endCombo();
  if (dropdownOpen)
    return -1;

  const int selectedIndex = dropdownState->getSelectedUnfilteredIndex();
  dropdownOpenState.reset();
  return selectedIndex;
}

bool FilteredComboBoxStandalone::beginFiltering(int unfiltered_item_count)
{
  DropdownOpenState *dropdownState = dropdownOpenState.get();
  G_ASSERT(dropdownState);
  return dropdownState && dropdownState->beginFiltering(unfiltered_item_count);
}

void FilteredComboBoxStandalone::endFiltering()
{
  DropdownOpenState *dropdownState = dropdownOpenState.get();
  G_ASSERT_RETURN(dropdownState, );
  dropdownState->endFiltering();
}

void FilteredComboBoxStandalone::addItem(const char *label, int unfiltered_item_index, int match_position_in_label, bool exact_match)
{
  DropdownOpenState *dropdownState = dropdownOpenState.get();
  G_ASSERT_RETURN(dropdownState, );
  dropdownState->addItem(label, unfiltered_item_index, match_position_in_label, exact_match);
}

const char *FilteredComboBoxStandalone::getFilter() const
{
  G_ASSERT(dropdownOpenState);
  return dropdownOpenState ? dropdownOpenState->getFilter() : "";
}

FilteredComboBoxStandalone::DropdownOpenState::DropdownOpenState(int unfiltered_selected_index, const ImVec2 &combo_preview_size) :
  dropdownFilteredIndex(unfiltered_selected_index), // Filter is empty at this point, so this is safe.
  comboPreviewSize(combo_preview_size),
  popupId(ImGui::GetID(this))
{}

void FilteredComboBoxStandalone::DropdownOpenState::beginCombo(const ImVec2 &combo_preview_start_pos)
{
  comboPreviewStartPos = combo_preview_start_pos;
}

bool FilteredComboBoxStandalone::DropdownOpenState::beginFiltering(int unfiltered_item_count)
{
  if (filterChanged)
    unfilteredItemCount = unfiltered_item_count;

  return filterChanged;
}

void FilteredComboBoxStandalone::DropdownOpenState::endFiltering()
{
  G_ASSERT(filterChanged);

  if (!dropdownJustOpened)
    dropdownFilteredIndex = pickBestMatch();

  filterChanged = false;
}

void FilteredComboBoxStandalone::DropdownOpenState::addItem(const char *label, int unfiltered_item_index, int match_position_in_label,
  bool exact_match)
{
  G_ASSERT(filterChanged);

  const int filteredItemIndex = filteredItems.size();
  FilteredItem &filteredItem = filteredItems.push_back_noinit();
  filteredItem.label = label;
  filteredItem.unfilteredIndex = unfiltered_item_index;
  filteredItem.filterMatchPositionInLabel = match_position_in_label;
  filteredItem.exactMatch = exact_match;
}

int FilteredComboBoxStandalone::DropdownOpenState::getSelectedUnfilteredIndex() const
{
  if (dropdownFilteredIndex >= 0 && dropdownFilteredIndex < filteredItems.size())
    return filteredItems[dropdownFilteredIndex].unfilteredIndex;
  else
    return -1;
}

int FilteredComboBoxStandalone::DropdownOpenState::pickBestMatch() const
{
  G_ASSERT(filterChanged);
  G_ASSERT(dropdownFilteredIndex < 0);

  if (filteredItems.empty())
    return -1;

  int prefixMatchIndex = -1;
  for (int i = 0; i < filteredItems.size(); ++i)
  {
    const FilteredItem &item = filteredItems[i];
    if (item.filterMatchPositionInLabel != 0)
      continue;

    if (item.exactMatch)
      return i;
    if (prefixMatchIndex < 0)
      prefixMatchIndex = i;
  }

  return prefixMatchIndex >= 0 ? prefixMatchIndex : 0;
}

bool FilteredComboBoxStandalone::DropdownOpenState::openPopup(float text_input_height, float vertical_space_between_controls)
{
  const ImVec2 popupWindowPadding(0.0f, 0.0f);
  const ImVec2 popupLeftTopPos(comboPreviewStartPos - popupWindowPadding);
  const ImVec2 popupRightBottomPos(comboPreviewStartPos + comboPreviewSize + popupWindowPadding);
  const int itemCountToSizeFor = clamp((int)filteredItems.size(), 1, 20); // Show max. 20 items like ImGuiComboFlags_HeightLarge.
  const float dropdownItemsHeight = calcMaxPopupHeightFromItemCount(itemCountToSizeFor);
  const ImVec2 popupSize(popupRightBottomPos.x - popupLeftTopPos.x,
    dropdownItemsHeight + text_input_height + vertical_space_between_controls);

  // At the first positioning (when there is no filter) downward direction is allowed. If there was not enough space
  // then continue opening upward regardless the filtered size.
  const bool allowDownward = dropdownOpensDownward;
  const ImRect popupRect =
    getPopupPositionAndSize(ImRect(popupLeftTopPos, popupRightBottomPos), popupSize, allowDownward, dropdownOpensDownward);

  ImGui::SetNextWindowPos(popupRect.Min);
  ImGui::SetNextWindowSize(popupRect.GetSize());

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, popupWindowPadding);
  const bool dropdownOpen =
    ImGui::BeginPopupEx(popupId, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
  ImGui::PopStyleVar();

  return dropdownOpen;
}

void FilteredComboBoxStandalone::DropdownOpenState::updateImguiFilterInput(bool &dropdown_open, float vertical_space_after_filter_inpt)
{
  bool focusFilterInput = dropdownJustOpened;

  // Ensure that the focus will be returned to the filter input (for example after the user clicked on an
  // ImGui::Selectable but released the button on another Selectable thus not selecting it). The mouse down and
  // released checks are needed to keep the Selectable clickable.
  const bool itemsWindowFocused = !ImGui::IsWindowFocused() && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
  if (itemsWindowFocused && !ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    focusFilterInput = true;

  const float arrowSize = ImGui::GetFrameHeight();
  const float availableWidth = ImGui::GetContentRegionAvail().x;
  const float inputWidth = availableWidth - arrowSize;
  const ImVec2 inputStartPos = ImGui::GetCursorScreenPos();

  if (inputWidth > 0.0f)
    ImGui::SetNextItemWidth(inputWidth);

  if (focusFilterInput)
    ImGui::SetKeyboardFocusHere();

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, vertical_space_after_filter_inpt));

  // We cannot have a border here because we need a single border around the InputText and the arrow.
  ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
  filterChanged = ImGuiDagor::InputText("##filter", &filter);
  ImGui::PopStyleColor();

  ImGui::PopStyleVar();

  const ImGuiID filterInputId = ImGui::GetItemID();

  if (ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_None, filterInputId))
  {
    dropdown_open = false;
  }
  else if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_None, filterInputId))
  {
    dropdown_open = false;
    dropdownFilteredIndex = -1;
  }
  else if (ImGui::Shortcut(ImGuiKey_UpArrow, ImGuiInputFlags_Repeat, filterInputId))
  {
    if (dropdownFilteredIndex > 0)
      --dropdownFilteredIndex;
  }
  else if (ImGui::Shortcut(ImGuiKey_DownArrow, ImGuiInputFlags_Repeat, filterInputId))
  {
    if ((dropdownFilteredIndex + 1) < filteredItems.size())
      ++dropdownFilteredIndex;
  }
  else if (ImGui::Shortcut(ImGuiKey_PageUp, ImGuiInputFlags_Repeat, filterInputId))
  {
    if (dropdownFilteredIndex > 0 && dropdownFirstVisibleFilteredIndex >= 0)
    {
      if (dropdownFirstVisibleFilteredIndex == dropdownFilteredIndex)
      {
        const int itemsPerPage = (int)floorf(
          (lastItemsWindowInnerHeight + ImGui::GetStyle().ItemSpacing.y) / (ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y));
        dropdownFilteredIndex = max(0, dropdownFilteredIndex - itemsPerPage);
      }
      else
      {
        dropdownFilteredIndex = dropdownFirstVisibleFilteredIndex;
      }
    }
  }
  else if (ImGui::Shortcut(ImGuiKey_PageDown, ImGuiInputFlags_Repeat, filterInputId))
  {
    if ((dropdownFilteredIndex + 1) < filteredItems.size() && dropdownLastVisibleFilteredIndex >= 0)
    {
      if (dropdownLastVisibleFilteredIndex == dropdownFilteredIndex)
      {
        const int itemsPerPage = (int)floorf(
          (lastItemsWindowInnerHeight + ImGui::GetStyle().ItemSpacing.y) / (ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y));
        dropdownFilteredIndex = min((int)(filteredItems.size() - 1), dropdownFilteredIndex + itemsPerPage);
      }
      else
      {
        dropdownFilteredIndex = dropdownLastVisibleFilteredIndex;
      }
    }
  }

  if (inputWidth > 0.0f)
  {
    const ImU32 bgCol = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    const ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);
    const ImVec2 framePadding = ImGui::GetStyle().FramePadding;
    const ImRect rectRect(inputStartPos + ImVec2(inputWidth, 0.0f),
      inputStartPos + ImVec2(comboPreviewSize.x, ImGui::GetCurrentContext()->FontSize + framePadding.y * 2.0f));

    ImGui::GetCurrentWindow()->DrawList->AddRectFilled(rectRect.Min, rectRect.Max, bgCol, ImGui::GetStyle().FrameRounding,
      ImDrawFlags_RoundCornersRight);
    ImGui::RenderArrow(ImGui::GetCurrentWindow()->DrawList, inputStartPos + ImVec2(inputWidth + framePadding.y, framePadding.y),
      textCol, ImGuiDir_Down, 1.0f);

    if (rectRect.Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
      dropdown_open = false;
  }

  ImGui::RenderFrameBorder(inputStartPos, inputStartPos + comboPreviewSize, ImGui::GetStyle().FrameRounding);
}

void FilteredComboBoxStandalone::DropdownOpenState::updateImguiComboBoxItems(int old_dropdown_filtered_index, bool &dropdown_open)
{
  const int filteredItemCount = filteredItems.size();
  if (filteredItemCount > 0)
  {
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);

    dropdownFirstVisibleFilteredIndex = -1;
    dropdownLastVisibleFilteredIndex = -1;

    for (int filteredItemIndex = 0; filteredItemIndex < filteredItemCount; ++filteredItemIndex)
    {
      // ScrollToItem is only a request, it will only apply in the next frame. To avoid flicker we render the old
      // selection now, and only show the new selection in the next frame to match the scrolling.

      const bool wasSelected = filteredItemIndex == old_dropdown_filtered_index;
      if (ImGui::Selectable(filteredItems[filteredItemIndex].label, wasSelected, ImGuiSelectableFlags_NoAutoClosePopups))
      {
        dropdown_open = false;
        dropdownFilteredIndex = filteredItemIndex;
      }

      if (filteredItemIndex == dropdownFilteredIndex && scrollToSelectedItem)
        ImGui::ScrollToItem();

      if (ImGui::IsItemVisible())
      {
        // ImGui::Selectable adds padding in both directions.
        const float halfSpaceY = ImGui::GetStyle().ItemSpacing.y * 0.5f;

        if ((ImGui::GetCurrentContext()->LastItemData.Rect.Min.y + halfSpaceY) >= ImGui::GetCurrentWindow()->ClipRect.Min.y &&
            (ImGui::GetCurrentContext()->LastItemData.Rect.Max.y - halfSpaceY) <= ImGui::GetCurrentWindow()->ClipRect.Max.y)
        {
          if (dropdownFirstVisibleFilteredIndex < 0)
            dropdownFirstVisibleFilteredIndex = filteredItemIndex;

          dropdownLastVisibleFilteredIndex = filteredItemIndex;
        }
      }
    }

    ImGui::PopItemFlag();
  }
  else
  {
    const char *message = unfilteredItemCount > 0 ? "No results found" : "Empty";
    const ImVec2 textSize = ImGui::CalcTextSize(message, nullptr);
    ImVec2 posMin = ImGui::GetCursorScreenPos();
    ImVec2 posMax = posMin + ImGui::GetContentRegionAvail();
    posMin.x = max(posMin.x, posMin.x + (((posMax.x - posMin.x) - textSize.x) * 0.5f));
    posMin.y = max(posMin.y, posMin.y + (((posMax.y - posMin.y) - textSize.y) * 0.5f));
    posMax.x = min(posMax.x, posMin.x + textSize.x);
    posMax.y = min(posMax.y, posMin.y + textSize.y);
    ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), posMin, posMax, posMax.x, posMax.x, message, nullptr, &textSize);
  }
}

bool FilteredComboBoxStandalone::DropdownOpenState::endCombo()
{
  G_ASSERT(!filterChanged);

  const int oldDropDownFilteredIndex = dropdownFilteredIndex;
  const float textInputHeight = ImGui::GetCurrentContext()->FontSize + (ImGui::GetStyle().FramePadding.y * 2.0f);
  const float verticalSpaceBetweenControls = 0.0f; // We already have WindowPadding.y, there is no need for more.

  bool dropdownOpen = openPopup(textInputHeight, verticalSpaceBetweenControls);
  if (!dropdownOpen)
    return false;

  const ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
  const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
  const ImVec2 itemsSize(0.0f, contentRegionAvail.y - textInputHeight - verticalSpaceBetweenControls);

  if (!dropdownOpensDownward)
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + contentRegionAvail.y - textInputHeight));

  updateImguiFilterInput(dropdownOpen, verticalSpaceBetweenControls);

  if (!dropdownOpensDownward)
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y));

  // Horizontally align with the framed text.
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ImGui::GetStyle().FramePadding.x, ImGui::GetStyle().WindowPadding.y));
  const bool childVisible = ImGui::BeginChild("##items", itemsSize, ImGuiChildFlags_AlwaysUseWindowPadding,
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
  ImGui::PopStyleVar();

  if (childVisible)
  {
    lastItemsWindowInnerHeight = ImGui::GetCurrentWindow()->InnerRect.GetHeight();
    scrollToSelectedItem |= dropdownFilteredIndex != oldDropDownFilteredIndex;
    updateImguiComboBoxItems(oldDropDownFilteredIndex, dropdownOpen);
  }
  ImGui::EndChild();

  if (!dropdownOpen)
    ImGui::CloseCurrentPopup();

  ImGui::EndPopup();

  dropdownJustOpened = false;
  scrollToSelectedItem = filterChanged;

  if (filterChanged)
  {
    filteredItems.clear();
    dropdownFilteredIndex = -1;
    dropdownFirstVisibleFilteredIndex = -1;
    dropdownLastVisibleFilteredIndex = -1;
  }

  return dropdownOpen;
}

// Based on ImGui's CalcMaxPopupHeightFromItemCount.
float FilteredComboBoxStandalone::calcMaxPopupHeightFromItemCount(int item_count)
{
  G_ASSERT(item_count > 0);
  ImGuiContext &g = *GImGui;
  return (g.FontSize + g.Style.ItemSpacing.y) * item_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

ImRect FilteredComboBoxStandalone::getPopupPositionAndSize(const ImRect &combo_preview_rect, const ImVec2 &requested_size,
  bool allow_downward, bool &downward)
{
  // Position the pop-up's top (when opening downward) or bottom (when opening upward) to cover the combobox's preview
  // block, so the filter text input and the arrow will be placed right over the original ones.

  const ImRect allowedRect = ImGui::GetPopupAllowedExtentRect(ImGui::GetCurrentWindow());
  const ImRect &combo = combo_preview_rect;

  if (allow_downward && (combo.Min.y + requested_size.y) <= allowedRect.Max.y)
  {
    downward = true;
    return ImRect(combo.Min, combo.Min + requested_size);
  }

  if ((combo.Max.y - requested_size.y) >= allowedRect.Min.y)
  {
    downward = false;
    return ImRect(combo.Min.x, combo.Max.y - requested_size.y, combo.Max.x, combo.Max.y);
  }

  const float yRoomDown = allowedRect.Max.y - combo.Min.y;
  const float yRoomUp = combo.Max.y - allowedRect.Min.y;
  if (allow_downward && yRoomDown >= yRoomUp)
  {
    downward = true;
    return ImRect(combo.Min.x, combo.Min.y, combo.Max.x, combo.Min.y + yRoomDown);
  }
  else
  {
    downward = false;
    return ImRect(combo.Min.x, combo.Max.y - yRoomUp, combo.Max.x, combo.Max.y);
  }
}

} // namespace PropPanel