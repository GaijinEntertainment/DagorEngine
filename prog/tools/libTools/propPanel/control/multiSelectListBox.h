// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/constants.h>
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class MultiSelectListBoxPropertyControl : public PropertyControlBase
{
public:
  MultiSelectListBoxPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y,
    hdpi::Px w, hdpi::Px h, const Tab<String> &vals) :
    PropertyControlBase(id, event_handler, parent, x, y, w, h), values(vals)
  {
    selected.resize(vals.size(), false);
  }

  virtual unsigned getTypeMaskForSet() const override
  {
    return CONTROL_DATA_STRINGS | CONTROL_DATA_SELECTION | CONTROL_DATA_TYPE_STRING;
  }

  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_STRINGS | CONTROL_DATA_SELECTION | CONTROL_DATA_TYPE_INT; }

  virtual void setTextValue(const char value[]) override
  {
    Tab<int> filtered(tmpmem);

    if (strlen(value) > 0)
    {
      String filter(value);
      filter.toLower();

      String tempString;
      for (int i = 0; i < values.size(); ++i)
      {
        tempString = values[i];
        tempString.toLower();
        if (strstr(tempString.c_str(), filter.c_str()))
          filtered.push_back(i);
      }
    }

    setSelectionValue(filtered);
  }

  virtual void setStringsValue(const Tab<String> &vals) override
  {
    values = vals;

    selected.resize_noinit(vals.size());
    setAllSelected(false);
    ensureVisibleRequestedIndex = -1;
  }

  virtual void setSelectionValue(const Tab<int> &sels) override
  {
    setAllSelected(false);
    ensureVisibleRequestedIndex = -1;

    for (int indexToSelect : sels)
    {
      if (indexToSelect >= 0 && indexToSelect < selected.size())
      {
        selected[indexToSelect] = true;

        if (ensureVisibleRequestedIndex < 0)
          ensureVisibleRequestedIndex = indexToSelect;
      }
    }
  }

  virtual void reset() override
  {
    Tab<int> vals(midmem);
    setSelectionValue(vals);

    PropertyControlBase::reset();
  }

  virtual int getIntValue() const override { return values.size(); }

  virtual int getStringsValue(Tab<String> &vals) override
  {
    vals = values;
    return values.size();
  }

  virtual int getSelectionValue(Tab<int> &sels) override
  {
    for (int i = 0; i < selected.size(); ++i)
      if (selected[i])
        sels.push_back(i);

    return 0;
  }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    setFocusToNextImGuiControlIfRequested();

    // Use full width by default. Use height that can fit ~7 items by default.
    const float width = mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : -FLT_MIN;
    const float height =
      (mH == (float)Constants::LISTBOX_DEFAULT_HEIGHT || mH > (float)Constants::LISTBOX_FULL_HEIGHT) ? mH : -FLT_MIN;
    const ImVec2 size(width, height);

    if (ImGui::BeginListBox("##lb", size))
    {
      ImGui::PushStyleColor(ImGuiCol_Header, Constants::LISTBOX_SELECTION_BACKGROUND_COLOR);
      ImGui::PushStyleColor(ImGuiCol_HeaderActive, Constants::LISTBOX_HIGHLIGHT_BACKGROUND_COLOR);
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Constants::LISTBOX_HIGHLIGHT_BACKGROUND_COLOR);

      bool selectionChanged = false;
      bool doubleClicked = false;

      ImGuiMultiSelectIO *multiSelectIo = ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_None, -1, values.size());
      if (multiSelectIo)
        selectionChanged |= applySelectionRequests(*multiSelectIo);

      for (int i = 0; i < values.size(); ++i)
      {
        const bool wasSelected = selected[i];
        if (wasSelected)
          ImGui::PushStyleColor(ImGuiCol_Text, Constants::LISTBOX_SELECTION_TEXT_COLOR);

        ImGui::SetNextItemSelectionUserData(i);

        const char *label = values[i].empty() ? "<empty>" : values[i].c_str();
        if (ImGui::Selectable(label, wasSelected, ImGuiSelectableFlags_AllowDoubleClick))
        {
          if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            doubleClicked = true;
        }

        if (wasSelected)
          ImGui::PopStyleColor();

        if (i == ensureVisibleRequestedIndex)
        {
          ensureVisibleRequestedIndex = -1;
          ImGui::SetScrollHereY();
        }
      }

      ImGui::EndMultiSelect();
      if (multiSelectIo)
        selectionChanged |= applySelectionRequests(*multiSelectIo);

      ImGui::PopStyleColor(3);
      ImGui::EndListBox();

      if (selectionChanged)
        onWcChange(nullptr);

      if (doubleClicked)
        onWcDoubleClick(nullptr);
    }
  }

private:
  void setAllSelected(bool new_value)
  {
    for (int i = 0; i < selected.size(); ++i)
      selected[i] = new_value;
  }

  bool applySelectionRequests(const ImGuiMultiSelectIO &multi_select_io)
  {
    bool selectionChanged = false;

    for (const ImGuiSelectionRequest &request : multi_select_io.Requests)
    {
      if (request.Type == ImGuiSelectionRequestType_SetAll)
      {
        for (int i = 0; i < selected.size(); ++i)
        {
          if (selected[i] != request.Selected)
          {
            selected[i] = request.Selected;
            selectionChanged = true;
          }
        }
      }
      else if (request.Type == ImGuiSelectionRequestType_SetRange)
      {
        G_ASSERT(request.RangeFirstItem >= 0 && request.RangeLastItem < selected.size());
        for (int i = request.RangeFirstItem; i <= request.RangeLastItem; ++i)
        {
          if (selected[i] != request.Selected)
          {
            selected[i] = request.Selected;
            selectionChanged = true;
          }
        }
      }
    }

    return selectionChanged;
  }

  bool controlEnabled = true;
  Tab<String> values;
  Tab<bool> selected;
  int ensureVisibleRequestedIndex = -1;
};

} // namespace PropPanel