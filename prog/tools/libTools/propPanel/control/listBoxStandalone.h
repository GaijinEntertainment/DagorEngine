// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/listBoxInterface.h>
#include <propPanel/c_window_event_handler.h>
#include <propPanel/constants.h>
#include <propPanel/imguiHelper.h>
#include "../contextMenuInternal.h"
#include "../scopedImguiBeginDisabled.h"
#include "../tooltipHelper.h"

namespace PropPanel
{

class ListBoxControlStandalone : public IListBoxInterface
{
public:
  ListBoxControlStandalone(const Tab<String> &vals, int index) :
    values(vals), selectedIndex((index >= 0 && index < vals.size()) ? index : -1), ensureVisibleRequestedIndex(selectedIndex)
  {}

  void setEventHandler(WindowControlEventHandler *event_handler) { eventHandler = event_handler; }

  // The provided window_base is passed to the event handler events (set with setEventHandler). This class does not use
  // it otherwise.
  void setWindowBaseForEventHandler(WindowBase *window_base) { windowBaseForEventHandler = window_base; }

  int getSelectedIndex() const { return selectedIndex; }

  const String *getSelectedText() const
  {
    if (selectedIndex < 0)
      return nullptr;

    return &values[selectedIndex];
  }

  void setSelectedIndex(int index)
  {
    if (index >= -1 && index < values.size())
    {
      selectedIndex = index;
      ensureVisibleRequestedIndex = selectedIndex;
    }
  }

  void setValues(const Tab<String> &vals)
  {
    selectedIndex = -1;
    ensureVisibleRequestedIndex = -1;
    values = vals;
  }

  void setSelectedValue(const char value[])
  {
    if (selectedIndex >= 0 && selectedIndex < values.size())
      values[selectedIndex] = value;
  }

  int addValue(const char *value)
  {
    values.emplace_back(String(value));
    return values.size() - 1;
  }

  void removeValueByIndex(int index)
  {
    if (index < 0 || index >= values.size())
      return;

    values.erase(values.begin() + index);

    if (values.size() == 0)
      selectedIndex = -1;
    else if (values.size() == index) // if we delete last item
      --selectedIndex;

    if (ensureVisibleRequestedIndex >= 0)
      ensureVisibleRequestedIndex = selectedIndex;
  }

  bool isEnabled() const { return controlEnabled; }

  void setEnabled(bool enabled) { controlEnabled = enabled; }

  virtual IMenu &createContextMenu() override
  {
    contextMenu.reset(new ContextMenu());
    return *contextMenu;
  }

  void updateImgui(float controlWidth = 0.0f, float controlHeight = (float)Constants::LISTBOX_DEFAULT_HEIGHT)
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    // Use full width by default. Use height that can fit ~7 items by default.
    const float width = controlWidth > 0.0f ? controlWidth : -FLT_MIN;
    const float height =
      (controlHeight == (float)Constants::LISTBOX_DEFAULT_HEIGHT || controlHeight > (float)Constants::LISTBOX_FULL_HEIGHT)
        ? controlHeight
        : -FLT_MIN;
    const ImVec2 size(width, height);

    if (ImguiHelper::beginListBoxWithWindowFlags("##lb", size, ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_HorizontalScrollbar))
    {
      const float listBoxInnerWidth = ImGui::GetContentRegionAvail().x;

      ImGui::PushStyleColor(ImGuiCol_Header, Constants::LISTBOX_SELECTION_BACKGROUND_COLOR);
      ImGui::PushStyleColor(ImGuiCol_HeaderActive, Constants::LISTBOX_HIGHLIGHT_BACKGROUND_COLOR);
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Constants::LISTBOX_HIGHLIGHT_BACKGROUND_COLOR);

      // The only reason we use ImGui::BeginMultiSelect here is because with that the keyboard arrows keys work as
      // expected: they move the selection instead of just the focus.
      const int oldSelectedIndex = selectedIndex;
      ImGuiMultiSelectIO *multiSelectIo =
        ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_SingleSelect, oldSelectedIndex >= 0 ? 1 : 0, values.size());
      if (multiSelectIo)
        applySelectionRequests(*multiSelectIo);

      bool doubleClicked = false;
      bool rightClicked = false;

      for (int i = 0; i < values.size(); ++i)
      {
        const bool wasSelected = i == oldSelectedIndex;
        if (wasSelected)
          ImGui::PushStyleColor(ImGuiCol_Text, Constants::LISTBOX_SELECTION_TEXT_COLOR);

        ImGui::SetNextItemSelectionUserData(i);

        const char *label = values[i].empty() ? "<empty>" : values[i].c_str();
        if (ImGui::Selectable(label, wasSelected, ImGuiSelectableFlags_AllowDoubleClick))
        {
          if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            doubleClicked = true;
        }
        // Imitate Windows where the context menu comes up on mouse button release. (BeginPopupContextItem also does this.)
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsItemHovered())
        {
          rightClicked = true;
        }

        if (wasSelected)
          ImGui::PopStyleColor();

        if (i == ensureVisibleRequestedIndex)
        {
          ensureVisibleRequestedIndex = -1;
          ImGui::SetScrollHereY();
        }

        // Show tooltip for overly long items.
        if (ImGui::IsItemVisible())
        {
          const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);
          if (labelSize.x > listBoxInnerWidth)
            tooltip_helper.setPreviousImguiControlTooltip((const void *)ImGui::GetItemID(), label);
        }
      }

      ImGui::EndMultiSelect();
      if (multiSelectIo)
        applySelectionRequests(*multiSelectIo);

      ImGui::PopStyleColor(3);
      ImGui::EndListBox();

      if (selectedIndex != oldSelectedIndex)
      {
        if (eventHandler)
          eventHandler->onWcChange(windowBaseForEventHandler);
      }

      if (doubleClicked && eventHandler)
        eventHandler->onWcDoubleClick(windowBaseForEventHandler);
      else if (rightClicked && eventHandler)
        eventHandler->onWcRightClick(windowBaseForEventHandler);
    }

    if (contextMenu)
    {
      const bool open = contextMenu->updateImgui();
      if (!open)
        contextMenu.reset();
    }
  }

private:
  void applySelectionRequests(const ImGuiMultiSelectIO &multi_select_io)
  {
    for (const ImGuiSelectionRequest &request : multi_select_io.Requests)
    {
      if (request.Type == ImGuiSelectionRequestType_SetAll)
      {
        G_ASSERT(!request.Selected);
        selectedIndex = -1;
      }
      else if (request.Type == ImGuiSelectionRequestType_SetRange)
      {
        G_ASSERT(request.RangeFirstItem == request.RangeLastItem);
        G_ASSERT(request.RangeFirstItem >= 0 && request.RangeFirstItem < values.size());
        selectedIndex = request.Selected ? request.RangeFirstItem : -1;
      }
    }
  }

  WindowControlEventHandler *eventHandler = nullptr;
  WindowBase *windowBaseForEventHandler = nullptr;
  bool controlEnabled = true;
  Tab<String> values;
  int selectedIndex;
  int ensureVisibleRequestedIndex;
  eastl::unique_ptr<ContextMenu> contextMenu;
};

} // namespace PropPanel