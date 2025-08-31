// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "../scopedImguiBeginDisabled.h"
#include "filteredComboBoxStandalone.h"
#include <EASTL/sort.h>

namespace PropPanel
{

class ComboBoxPropertyControl : public PropertyControlBase
{
public:
  ComboBoxPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], const Tab<String> &vals, int index, bool sorted) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)), controlCaption(caption), controlSorted(sorted)
  {
    setValues(vals);
    selectedDisplayIndex = getDisplayIndexFromIndex(index);
  }

  unsigned getTypeMaskForSet() const override
  {
    return CONTROL_DATA_TYPE_INT | CONTROL_CAPTION | CONTROL_DATA_TYPE_STRING | CONTROL_DATA_STRINGS;
  }

  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_INT | CONTROL_DATA_TYPE_STRING; }

  int getIntValue() const override { return selectedDisplayIndex >= 0 ? displayIndexToIndex[selectedDisplayIndex] : -1; }

  int getTextValue(char *buffer, int buflen) const override
  {
    if (selectedDisplayIndex < 0)
      return 0;

    const String &selectedText = values[displayIndexToIndex[selectedDisplayIndex]];
    return ImguiHelper::getTextValueForString(selectedText, buffer, buflen);
  }

  void setTextValue(const char value[]) override
  {
    const int index = ImguiHelper::getStringIndexInTab(values, value);
    selectedDisplayIndex = getDisplayIndexFromIndex(index);
  }

  void setIntValue(int index) override
  {
    if (index >= -1 && index < values.size())
      selectedDisplayIndex = getDisplayIndexFromIndex(index);
  }

  void setStringsValue(const Tab<String> &vals) override
  {
    selectedDisplayIndex = -1;
    setValues(vals);
  }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void reset() override
  {
    setIntValue(-1);

    PropertyControlBase::reset();
  }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    ImguiHelper::separateLineLabel(controlCaption);
    setFocusToNextImGuiControlIfRequested();

    // Use full width by default.
    ImGui::SetNextItemWidth(mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : -FLT_MIN);

    const char *selectedText = selectedDisplayIndex >= 0 ? values[displayIndexToIndex[selectedDisplayIndex]].c_str() : "";
    if (!filteredComboBox.beginCombo("##bc", selectedText, selectedDisplayIndex))
      return;

    if (filteredComboBox.beginFiltering(displayIndexToIndex.size()))
    {
      String lowerCaseFilter(filteredComboBox.getFilter());
      lowerCaseFilter.toLower();

      String lowerCaseLabel;

      for (int i = 0; i < displayIndexToIndex.size(); ++i)
      {
        const char *label = values[displayIndexToIndex[i]];
        lowerCaseLabel = label;
        lowerCaseLabel.toLower();

        const char *match = strstr(lowerCaseLabel, lowerCaseFilter);
        if (!match)
          continue;

        const int matchPosition = lowerCaseLabel.c_str() - match;
        const bool exactMatch = strcmp(lowerCaseLabel, lowerCaseFilter) == 0;
        filteredComboBox.addItem(label, i, matchPosition, exactMatch);
      }

      filteredComboBox.endFiltering();
    }

    const int newSelectedIndex = filteredComboBox.endCombo();
    if (newSelectedIndex != selectedDisplayIndex && newSelectedIndex >= 0)
    {
      selectedDisplayIndex = newSelectedIndex;
      onWcChange(nullptr);
    }
  }

private:
  void setValues(const Tab<String> &new_values)
  {
    values = new_values;

    displayIndexToIndex.resize_noinit(new_values.size());
    for (int i = 0; i < new_values.size(); ++i)
      displayIndexToIndex[i] = i;

    if (controlSorted)
    {
      eastl::sort(displayIndexToIndex.begin(), displayIndexToIndex.end(),
        [&new_values](int left, int right) { return stricmp(new_values[left], new_values[right]) < 0; });
    }
  }

  int getDisplayIndexFromIndex(int index) const
  {
    if (index >= 0 && index < displayIndexToIndex.size())
      for (int i = 0; i < displayIndexToIndex.size(); ++i)
        if (displayIndexToIndex[i] == index)
          return i;

    return -1;
  }

  String controlCaption;
  bool controlEnabled = true;
  const bool controlSorted;
  Tab<String> values;
  dag::Vector<int> displayIndexToIndex;
  int selectedDisplayIndex;
  FilteredComboBoxStandalone filteredComboBox;
};

} // namespace PropPanel