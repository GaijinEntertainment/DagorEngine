// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <propPanel/colors.h>
#include <util/dag_string.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

class AutocompletePopup
{
public:
  void beginAddingItems()
  {
    items.clear();
    selectedIndex = -1;
    firstVisibleIndex = -1;
    lastVisibleIndex = -1;
    requestedPopupOpening = false;
    requestedScrollToSelectedItem = false;
  }

  void addItem(const char *name, const char *description)
  {
    ComboBoxItem &item = items.push_back();
    item.name = name;
    if (description && *description)
      item.descriptionWithPrefix.printf(0, " -- %s", description);
  }

  void endAddingItems()
  {
    if (items.empty())
    {
      popupOpen = false;
    }
    else
    {
      // Unnecessarily calling ImGui::OpenPopupEx() would cause rare flickers because OpenPopupEx() sets ImGuiPopupData::Window to 0,
      // and that counts as window activation in ImGui::Begin() ("window_just_activated_by_user |= (window != popup_ref.Window)""),
      // and that causes the window to be hidden for a frame ("HiddenFramesCannotSkipItems = 1").
      if (!popupOpen)
        requestedPopupOpening = true;
    }
  }

  const char *getSelectedItemName() const
  {
    if (selectedIndex >= 0 && selectedIndex < items.size())
      return items[selectedIndex].name;
    else
      return nullptr;
  }

  bool isPopupOpen() const { return popupOpen; }

  // selection_changed: output parameter. Set to true if the user has changed the selection.
  // item_clicked: output parameter. Set to true if the user has made a selection by clicking with the mouse.
  void updateImgui(const ImRect &log_window_rect, const ImRect &edit_box_rect, bool &selection_changed, bool &item_clicked)
  {
    selection_changed = false;
    item_clicked = false;

    if (!popupOpen && !requestedPopupOpening)
      return;

    G_ASSERT(!items.empty());

    const ImGuiID popupId = ImGui::GetID(this);
    bool justOpened = false;
    if (requestedPopupOpening)
    {
      requestedPopupOpening = false;
      popupOpen = true;
      justOpened = true;
      ImGui::OpenPopupEx(popupId);
    }

    popupOpen = beginPopup(popupId, log_window_rect, edit_box_rect, justOpened);
    if (!popupOpen)
      return;

    const int oldSelectedIndex = selectedIndex;
    const ImVec2 itemsSize(0.0f, ImGui::GetContentRegionAvail().y);

    if (ImGui::BeginChild("##items", itemsSize, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_FrameStyle,
          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground))
    {
      lastItemsWindowInnerHeight = ImGui::GetCurrentWindow()->InnerRect.GetHeight();
      item_clicked = updateImguiComboBoxItems();
    }
    ImGui::EndChild();

    handleShortcuts();

    ImGui::EndPopup();

    if (selectedIndex != oldSelectedIndex)
    {
      selection_changed = true;
      requestedScrollToSelectedItem = true;
    }
  }

private:
  struct ComboBoxItem
  {
    String name;
    String descriptionWithPrefix; // " -- description"
  };

  void setSelectedIndexNoScroll(int index) { selectedIndex = index; }

  void setSelectedIndexScroll(int index)
  {
    if (index == selectedIndex)
      return;

    setSelectedIndexNoScroll(index);
    requestedScrollToSelectedItem = true;
  }

  void selectNextItem(bool next = true)
  {
    if (items.empty())
      return;

    if (next)
    {
      int newIndex = selectedIndex + 1;
      if (newIndex >= items.size())
        newIndex = 0;
      setSelectedIndexScroll(newIndex);
    }
    else
    {
      int newIndex = selectedIndex - 1;
      if (newIndex < 0)
        newIndex = items.size() - 1;
      setSelectedIndexScroll(newIndex);
    }
  }

  bool beginPopup(ImGuiID popup_id, const ImRect &log_window_rect, const ImRect &edit_box_rect, bool just_opened)
  {
    const int itemCountToSizeFor = clamp((int)items.size(), 1, 20); // Show max. 20 items like ImGuiComboFlags_HeightLarge.
    const float dropdownItemsHeight = calcMaxPopupHeightFromItemCount(itemCountToSizeFor);
    const ImVec2 popupPos(edit_box_rect.GetTL());
    // Compensate for the window padding vertically to avoid the scrollbar. (The popup is wide, no need to do it horizontally.)
    const ImVec2 popupSize(log_window_rect.GetWidth(), dropdownItemsHeight + ImGui::GetStyle().WindowPadding.y);
    const ImRect popupRect = getPopupPositionAndSize(popupPos, popupSize, edit_box_rect, popupDirection);

    ImGui::SetNextWindowPos(popupRect.GetTL());
    ImGui::SetNextWindowSize(popupRect.GetSize());

    const bool open = ImGui::BeginPopupEx(popup_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar |
                                                      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);

    if (open)
    {
      // These two hacks are needed because of using ImGuiWindowFlags_NoFocusOnAppearing.
      // 1) Without this the popup would get behind the console's window and would not show up again after switching to another window.
      if (just_opened)
        ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
      // 2) Without this the popup would not close when clicking into another window.
      if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        ImGui::CloseCurrentPopup();
    }

    return open;
  }

  bool updateImguiComboBoxItems()
  {
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);

    firstVisibleIndex = -1;
    lastVisibleIndex = -1;

    const int oldSelectedIndex = selectedIndex;
    bool pickedItem = false;

    for (int itemIndex = 0; itemIndex < items.size(); ++itemIndex)
    {
      // ScrollToItem is only a request, it will only apply in the next frame. To avoid flicker we render the old
      // selection now, and only show the new selection in the next frame to match the scrolling.
      const bool wasSelected = itemIndex == oldSelectedIndex;

      const ComboBoxItem &item = items[itemIndex];
      const ImVec2 selectStartCursorPos = ImGui::GetCursorScreenPos();
      if (ImGui::Selectable(item.name, wasSelected, ImGuiSelectableFlags_NoAutoClosePopups))
      {
        setSelectedIndexNoScroll(itemIndex);
        pickedItem = true;
        popupOpen = false;
      }

      if (!item.descriptionWithPrefix.empty())
      {
        ImGui::PushStyleColor(ImGuiCol_Text, PropPanel::getOverriddenColor(PropPanel::ColorOverride::CONSOLE_LOG_REMARK_TEXT));

        const ImVec2 nameSize = ImGui::CalcTextSize(item.name.c_str(), item.name.c_str() + item.name.length());
        const ImVec2 textPosMin(selectStartCursorPos.x + nameSize.x, selectStartCursorPos.y);
        const float clipPosX = ImGui::GetCurrentWindowRead()->ContentRegionRect.Max.x + ImGui::GetCurrentWindowRead()->Scroll.x;
        const ImVec2 textPosMax(clipPosX, textPosMin.y + ImGui::GetCurrentContext()->FontSize);
        ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), textPosMin, textPosMax, clipPosX, item.descriptionWithPrefix.c_str(),
          item.descriptionWithPrefix.c_str() + item.descriptionWithPrefix.length(), nullptr);

        ImGui::PopStyleColor();
      }

      if (itemIndex == oldSelectedIndex && requestedScrollToSelectedItem)
      {
        requestedScrollToSelectedItem = false;
        ImGui::ScrollToItem();
      }

      if (ImGui::IsItemVisible())
      {
        // ImGui::Selectable adds padding in both directions.
        const float halfSpaceY = ImGui::GetStyle().ItemSpacing.y * 0.5f;

        if ((ImGui::GetCurrentContext()->LastItemData.Rect.Min.y + halfSpaceY) >= ImGui::GetCurrentWindow()->ClipRect.Min.y &&
            (ImGui::GetCurrentContext()->LastItemData.Rect.Max.y - halfSpaceY) <= ImGui::GetCurrentWindow()->ClipRect.Max.y)
        {
          if (firstVisibleIndex < 0)
            firstVisibleIndex = itemIndex;

          lastVisibleIndex = itemIndex;
        }
      }
    }

    ImGui::PopItemFlag();

    return pickedItem;
  }

  void handleShortcuts()
  {
    // Hijack the handling of these keys from the edit box while the popup is open.
    const ImGuiID keyRobberId = ImGui::GetID("keyRobber");
    ImGui::SetKeyOwner(ImGuiKey_Escape, keyRobberId);
    ImGui::SetKeyOwner(ImGuiKey_Tab, keyRobberId);
    ImGui::SetKeyOwner(ImGuiKey_UpArrow, keyRobberId);
    ImGui::SetKeyOwner(ImGuiKey_DownArrow, keyRobberId);
    ImGui::SetKeyOwner(ImGuiKey_PageUp, keyRobberId);
    ImGui::SetKeyOwner(ImGuiKey_PageDown, keyRobberId);

    const ImGuiInputFlags shortcutFlags =
      ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused | ImGuiInputFlags_RouteOverActive;

    if (ImGui::Shortcut(ImGuiKey_Escape, shortcutFlags, keyRobberId))
    {
      popupOpen = false;
    }
    else if (ImGui::Shortcut(ImGuiKey_UpArrow, shortcutFlags | ImGuiInputFlags_Repeat, keyRobberId) ||
             ImGui::Shortcut(ImGuiMod_Shift | ImGuiKey_Tab, ImGuiInputFlags_RouteAlways | ImGuiInputFlags_Repeat, keyRobberId))
    {
      selectNextItem(false);
    }
    else if (ImGui::Shortcut(ImGuiKey_DownArrow, shortcutFlags | ImGuiInputFlags_Repeat, keyRobberId) ||
             ImGui::Shortcut(ImGuiKey_Tab, ImGuiInputFlags_RouteAlways | ImGuiInputFlags_Repeat, keyRobberId))
    {
      selectNextItem();
    }
    else if (ImGui::Shortcut(ImGuiKey_PageUp, shortcutFlags | ImGuiInputFlags_Repeat, keyRobberId))
    {
      if (selectedIndex > 0 && firstVisibleIndex >= 0)
      {
        if (firstVisibleIndex == selectedIndex)
        {
          const int itemsPerPage = (int)floorf(
            (lastItemsWindowInnerHeight + ImGui::GetStyle().ItemSpacing.y) / (ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y));
          setSelectedIndexScroll(max(0, selectedIndex - itemsPerPage));
        }
        else
        {
          setSelectedIndexScroll(firstVisibleIndex);
        }
      }
    }
    else if (ImGui::Shortcut(ImGuiKey_PageDown, shortcutFlags | ImGuiInputFlags_Repeat, keyRobberId))
    {
      if ((selectedIndex + 1) < items.size() && lastVisibleIndex >= 0)
      {
        if (lastVisibleIndex == selectedIndex)
        {
          const int itemsPerPage = (int)floorf(
            (lastItemsWindowInnerHeight + ImGui::GetStyle().ItemSpacing.y) / (ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y));
          setSelectedIndexScroll(min((int)(items.size() - 1), selectedIndex + itemsPerPage));
        }
        else
        {
          setSelectedIndexScroll(lastVisibleIndex);
        }
      }
    }
  }

  // Based on ImGui's CalcMaxPopupHeightFromItemCount.
  static float calcMaxPopupHeightFromItemCount(int item_count)
  {
    G_ASSERT(item_count > 0);
    const ImGuiContext &g = *GImGui;
    return (g.FontSize + g.Style.ItemSpacing.y) * item_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
  }

  static ImRect getPopupPositionAndSize(const ImVec2 &requested_position, const ImVec2 &requested_size, const ImRect &edit_box_rect,
    ImGuiDir &popup_direction)
  {
    const ImRect allowedRect = ImGui::GetPopupAllowedExtentRect(ImGui::GetCurrentWindow());
    const ImVec2 pos = ImGui::FindBestWindowPosForPopupEx(requested_position, requested_size, &popup_direction, allowedRect,
      edit_box_rect, ImGuiPopupPositionPolicy_ComboBox);
    return ImRect(pos.x, pos.y, min(pos.x + requested_size.x, allowedRect.Max.x), min(pos.y + requested_size.y, allowedRect.Max.y));
  }

  dag::Vector<ComboBoxItem> items;
  int selectedIndex = -1;
  int firstVisibleIndex = -1;
  int lastVisibleIndex = -1;
  float lastItemsWindowInnerHeight = 0.0f;
  ImGuiDir popupDirection = ImGuiDir_None;
  bool popupOpen = false;
  bool requestedPopupOpening = false;
  bool requestedScrollToSelectedItem = true;
};
