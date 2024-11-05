// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <util/dag_string.h>

#include <EASTL/unique_ptr.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h> // ImRect

namespace PropPanel
{

class FilteredComboBoxStandalone
{
public:
  bool beginCombo(const char *label, const char *preview_text, int unfiltered_selected_index);

  // Call only if beginCombo() returned true.
  // Returns with the selected unfiltered index. Returns with -1 while the selection is in progress or was cancelled.
  int endCombo();

  // Call after beginCombo().
  // Returns true if the items must be refiltered.
  bool beginFiltering(int unfiltered_item_count);

  // Call only if beginFiltering() returned true.
  void endFiltering();

  // Call between beginFiltering() and endFiltering().
  // Only add items that match the filter.
  // label: the pointer must remain valid until the end of endCombo().
  // match_position_in_label: where the filter is within the label
  // exact_match: whether the filter exactly matches the label
  void addItem(const char *label, int unfiltered_item_index, int match_position_in_label, bool exact_match);

  const char *getFilter() const;

private:
  class DropdownOpenState
  {
  public:
    DropdownOpenState(int unfiltered_selected_index, const ImVec2 &combo_preview_size);

    void beginCombo(const ImVec2 &combo_preview_start_pos);
    bool endCombo();
    bool beginFiltering(int unfiltered_item_count);
    void endFiltering();
    void addItem(const char *label, int unfiltered_item_index, int match_position_in_label, bool exact_match);

    const char *getFilter() const { return filter.c_str(); }
    int getSelectedUnfilteredIndex() const;
    ImGuiID getPopupId() const { return popupId; }

  private:
    struct FilteredItem
    {
      const char *label;
      int unfilteredIndex;
      int filterMatchPositionInLabel;
      bool exactMatch;
    };

    int pickBestMatch() const;
    void updateImguiFilterInput(bool &dropdown_open, float vertical_space_after_filter_inpt);
    void updateImguiComboBoxItems(int old_dropdown_filtered_index, bool &dropdown_open);
    bool openPopup(float text_input_height, float vertical_space_between_controls);

    dag::Vector<FilteredItem> filteredItems;
    String filter;
    int unfilteredItemCount = 0;
    int dropdownFilteredIndex = -1; // Index into filteredItems.
    int dropdownFirstVisibleFilteredIndex = -1;
    int dropdownLastVisibleFilteredIndex = -1;
    float lastItemsWindowInnerHeight = 0.0f;
    bool dropdownJustOpened = true;
    bool dropdownOpensDownward = true;
    bool filterChanged = true;
    bool scrollToSelectedItem = true;
    ImVec2 comboPreviewStartPos;
    ImVec2 comboPreviewSize;
    const ImGuiID popupId;
  };

  static float calcMaxPopupHeightFromItemCount(int item_count);
  static ImRect getPopupPositionAndSize(const ImRect &combo_preview_rect, const ImVec2 &requested_size, bool allow_downward,
    bool &downward);

  eastl::unique_ptr<DropdownOpenState> dropdownOpenState;
};

} // namespace PropPanel