// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/type_traits.h>
#include "levelProfilerInterface.h"

namespace levelprofiler
{

using FilterDrawCallback = eastl::function<void()>;

class RIModule;
enum TableColumn;
struct TextureData;


// === Base Filters ===


// Generic filter for discrete values, allowing items to be included/excluded.
template <typename Key>
class CheckFilter
{
public:
  CheckFilter() {}

  // Returns true if the value is not in the exclusion set.
  bool pass(const Key &value) const { return offSet.find(value) == offSet.end(); }


  void reset() { offSet.clear(); }


  void toggle(const Key &value)
  {
    auto iterator = offSet.find(value);
    if (iterator == offSet.end())
    {
      offSet.insert(value);
    }
    else
    {
      offSet.erase(iterator);
    }
  }


  void invert(const eastl::vector<Key> &all_values)
  {
    eastl::hash_set<Key> tempSet;
    for (auto &value : all_values)
    {
      if (offSet.find(value) == offSet.end())
      {
        tempSet.insert(value);
      }
    }
    offSet.swap(tempSet);
  }

  const eastl::hash_set<Key> &getOffList() const { return offSet; }
  eastl::hash_set<Key> &getOffList() { return offSet; }

private:
  eastl::hash_set<Key> offSet; // Stores values that are currently excluded by the filter.
};

// === Text Filters ===

// Represents a text mask for filtering, specifying whether the mask is for inclusion or exclusion.
struct NameMask
{
  ProfilerString text;
  bool isInclude; // True if this mask is for including items, false for excluding.

  NameMask() = default;


  NameMask(const ProfilerString &mask_string, bool is_include) : text(mask_string), isInclude(is_include) {}
};


// Provides UI and logic for filtering based on text search and include/exclude masks.
class LpIncludeExcludeFilter
{
public:
  LpIncludeExcludeFilter();
  ~LpIncludeExcludeFilter();

  // Returns true if the filter settings were changed.
  bool drawInline();

  void reset();

  bool pass(const ProfilerString &value) const;

  void setSearchText(const char *search_text);
  const char *getSearchText() const;


  void addMask(const ProfilerString &mask, bool is_include);


  void removeMask(int index);
  void clearMasks();
  const eastl::vector<NameMask> &getMasks() const;

  void drawPopup(const char *popup_id);

  bool isActive() const;

private:
  char searchBuffer[64] = "";
  eastl::vector<NameMask> masks;

  bool newIncludeState = true;
  ProfilerString newMaskBuffer;
  ProfilerString oldSearchBuffer;

  bool matchesSearch(const char *text) const;
  bool matchesMasks(const char *text) const;

  static bool containsIgnoreCase(const char *haystack, const char *needle);
};


// Range Filters


// Generic filter for a numerical range (min/max).
template <typename T>
class LpRangeFilter
{
public:
  // Ensures that T is an arithmetic type (e.g., int, float).
  static_assert(eastl::is_arithmetic<T>::value, "LpRangeFilter requires arithmetic type");

  LpRangeFilter() = default;

  // Initializes with default and absolute min/max values.
  // If absolute_min_value or absolute_max_value are 0, they default to default_min_value and default_max_value respectively.
  // This is a common pattern for when the absolute bounds are not known at construction or are the same as the defaults.
  LpRangeFilter(T default_min_value, T default_max_value, T absolute_min_value = 0, T absolute_max_value = 0) :
    useMin(false),
    useMax(false),
    minValue(default_min_value),
    maxValue(default_max_value),
    defaultMin(default_min_value),
    defaultMax(default_max_value),
    absoluteMin(absolute_min_value == 0 ? default_min_value : absolute_min_value),
    absoluteMax(absolute_max_value == 0 ? default_max_value : absolute_max_value)
  {}

  bool pass(T value) const
  {
    if (useMin && value < minValue)
      return false;
    if (useMax && value > maxValue)
      return false;
    return true;
  }

  void reset()
  {
    useMin = useMax = false;
    minValue = defaultMin;
    maxValue = defaultMax;
  }

  // Resets the filter with new default values and updates absolute bounds if they were not previously set (were 0).
  void reset(T default_min_value, T default_max_value)
  {
    useMin = useMax = false;
    minValue = defaultMin = default_min_value;
    maxValue = defaultMax = default_max_value;

    if (absoluteMin == 0)
      absoluteMin = default_min_value;
    if (absoluteMax == 0)
      absoluteMax = default_max_value;
  }

  void setRange(T new_min_value, T new_max_value)
  {
    minValue = new_min_value;
    maxValue = new_max_value;
  }

  void setAbsoluteBounds(T new_absolute_min, T new_absolute_max)
  {
    absoluteMin = new_absolute_min;
    absoluteMax = new_absolute_max;
  }

  void setUseMin(bool should_use_min) { useMin = should_use_min; }

  void setUseMax(bool should_use_max) { useMax = should_use_max; }

  T getMin() const { return minValue; }

  T getMax() const { return maxValue; }

  T getAbsoluteMin() const { return absoluteMin; }

  T getAbsoluteMax() const { return absoluteMax; }

  bool isUsingMin() const { return useMin; }

  bool isUsingMax() const { return useMax; }

  T getDefaultMin() const { return defaultMin; }

  T getDefaultMax() const { return defaultMax; }

  // Returns true if the range values were changed by the user.
  // The 'speed' parameter controls the increment/decrement speed of the drag.
  // 'flags' can be used to customize ImGui slider behavior.
  bool drawRangeSlider(const char *label, const char *format_min = "Min: %.3f", const char *format_max = "Max: %.3f",
    float speed = 1.0f, ImGuiSliderFlags flags = 0)
  {
    bool changed = false;

    T currentMin = minValue;
    T currentMax = maxValue;

    if constexpr (eastl::is_integral<T>::value)
    {
      changed = ImGui::DragIntRange2(label, reinterpret_cast<int *>(&currentMin), reinterpret_cast<int *>(&currentMax), speed,
        static_cast<int>(absoluteMin), static_cast<int>(absoluteMax), format_min, format_max, flags);
    }
    else if constexpr (eastl::is_floating_point<T>::value)
    {
      changed = ImGui::DragFloatRange2(label, reinterpret_cast<float *>(&currentMin), reinterpret_cast<float *>(&currentMax), speed,
        static_cast<float>(absoluteMin), static_cast<float>(absoluteMax), format_min, format_max, flags);
    }

    if (changed)
    {
      minValue = currentMin;
      maxValue = currentMax;

      useMin = (minValue > absoluteMin);
      useMax = (maxValue < absoluteMax);
    }

    return changed;
  }

  // Returns true if the input values were changed.
  bool drawInputs(const char *min_label, const char *max_label, const char *format = "%.3f")
  {
    bool changed = false;
    ImGui::PushID("RangeInputs"); // Ensures unique IDs for ImGui elements within this section.

    bool tempUseMin = useMin;
    bool tempUseMax = useMax;
    T currentMin = minValue;
    T currentMax = maxValue;

    ImGui::Columns(2, nullptr, false);            // Use 2 columns for a compact layout.
    if (ImGui::Checkbox("##UseMin", &tempUseMin)) // Checkbox to enable/disable the min value.
    {
      useMin = tempUseMin;
      changed = true;
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!tempUseMin);

    if constexpr (eastl::is_integral<T>::value)
    {
      int tempMin = static_cast<int>(currentMin);
      // ImGuiInputTextFlags_EnterReturnsTrue makes the input apply on Enter key press.
      if (ImGui::InputInt(min_label, &tempMin, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
      {
        minValue = static_cast<T>(tempMin);
        useMin = true; // Automatically enable if value is changed.
        changed = true;
      }
    }
    else
    {
      float tempMin = static_cast<float>(currentMin);
      // ImGuiInputTextFlags_EnterReturnsTrue makes the input apply on Enter key press.
      if (ImGui::InputFloat(min_label, &tempMin, 0.1f, 1.0f, format, ImGuiInputTextFlags_EnterReturnsTrue))
      {
        minValue = static_cast<T>(tempMin);
        useMin = true; // Automatically enable if value is changed.
        changed = true;
      }
    }
    ImGui::EndDisabled();

    ImGui::NextColumn();
    if (ImGui::Checkbox("##UseMax", &tempUseMax))
    {
      useMax = tempUseMax;
      changed = true;
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!tempUseMax);

    if constexpr (eastl::is_integral<T>::value)
    {
      int tempMax = static_cast<int>(currentMax);
      // ImGuiInputTextFlags_EnterReturnsTrue makes the input apply on Enter key press.
      if (ImGui::InputInt(max_label, &tempMax, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
      {
        maxValue = static_cast<T>(tempMax);
        useMax = true; // Automatically enable if value is changed.
        changed = true;
      }
    }
    else
    {
      float tempMax = static_cast<float>(currentMax);
      // ImGuiInputTextFlags_EnterReturnsTrue makes the input apply on Enter key press.
      if (ImGui::InputFloat(max_label, &tempMax, 0.1f, 1.0f, format, ImGuiInputTextFlags_EnterReturnsTrue))
      {
        maxValue = static_cast<T>(tempMax);
        useMax = true; // Automatically enable if value is changed.
        changed = true;
      }
    }
    ImGui::EndDisabled();

    ImGui::Columns(1);
    ImGui::PopID();

    return changed;
  }

  // Returns true if any part of the range filter UI changed its state.
  bool drawRangeFilterUI(const char *label, const char *format_min = "Min: %.3f", const char *format_max = "Max: %.3f",
    float speed = 1.0f, ImGuiSliderFlags flags = 0)
  {
    bool changed = false;

    if (ImGui::Button("Clear")) // Button to reset the filter to its default state.
    {
      reset();
      changed = true;
    }

    ImGui::Separator();

    bool inputsChanged = drawInputs(format_min, format_max);
    changed |= inputsChanged;

    ImGui::Separator();

    bool sliderChanged = drawRangeSlider(label, format_min, format_max, speed, flags);

    return changed || sliderChanged;
  }

protected:
  bool useMin = false, useMax = false;
  T minValue = static_cast<T>(0), maxValue = static_cast<T>(0);
  T defaultMin = static_cast<T>(0), defaultMax = static_cast<T>(0);
  T absoluteMin = static_cast<T>(0), absoluteMax = static_cast<T>(0);
};


// === Filter UI Components ===


// A button-like UI element that indicates if a filter is active and can be clicked to open a filter popup.
class LpFilterIndicator
{
public:
  // \'identifier\' must be unique to avoid ImGui ID collisions.
  explicit LpFilterIndicator(const char *identifier) : id(identifier), isClicked(false) {}


  // Draws the indicator. Should be called within an ImGui table context.
  // \'is_active\' determines the indicator\'s color.
  void Draw(bool is_active)
  {
    isClicked = false;
    ImGui::PushID(id);
    const char *iconText = "F"; // Simple text icon for the filter.
    ImVec2 textSize = ImGui::CalcTextSize(iconText);
    float columnWidth = ImGui::GetColumnWidth();
    float paddingX = ImGui::GetStyle().FramePadding.x;
    // Calculate position to right-align the button within the current table cell.
    float cursorX = ImGui::GetCursorPosX() + (columnWidth - (textSize.x + paddingX * 2));
    ImGui::SetCursorPosX(cursorX);

    // Use different colors for active and inactive states.
    ImVec4 backgroundColor = is_active ? ImVec4(0.2f, 0.6f, 1.0f, 0.3f) : ImVec4(0.5f, 0.5f, 0.5f, 0.1f);
    ImVec4 backgroundColorHover = is_active ? ImVec4(0.2f, 0.6f, 1.0f, 0.5f) : ImVec4(0.6f, 0.6f, 0.6f, 0.2f);
    ImGui::PushStyleColor(ImGuiCol_Button, backgroundColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, backgroundColorHover);
    if (ImGui::Button(iconText))
    {
      isClicked = true;
    }
    ImGui::PopStyleColor(2);
    ImGui::PopID();
  }

  bool IsClicked() const { return isClicked; }

private:
  const char *id;
  bool isClicked;
};


// Defines an entry for a filter popup, associating a table column with a drawing function.
struct PopupEntry
{
  TableColumn column = COLUMN_NONE;
  const char *popupId = nullptr;
  FilterDrawCallback drawFunction;
};

class LpFilterPopupManager
{
public:
  void registerPopup(TableColumn table_column, const char *popup_identifier, FilterDrawCallback draw_func);

  void open(TableColumn table_column);

  void drawAll();

private:
  eastl::vector<PopupEntry> entries;
  TableColumn columnToOpen{COLUMN_NONE};
};


// === Master Filter Manager ===


class TextureProfilerUI;

// Helper structures for multiselect
struct LpSelectionExternalStorage;

class TextureFilterManager
{
public:
  TextureFilterManager();
  ~TextureFilterManager();

  bool passesAllFilters(const ProfilerString &texture_name, const TextureData &texture_data, const TextureUsage *texture_usage_ptr);

  void resetAllFilters();
  void applyFilters();
  bool isColumnFilterActive(TableColumn table_column) const;


  // Call in UI right after filter creation
  void setRIModule(RIModule *module_instance);

  CheckFilter<ProfilerString> &getFormatFilter() { return formatFilter; }
  CheckFilter<int> &getWidthFilter() { return widthFilter; }
  CheckFilter<int> &getHeightFilter() { return heightFilter; }

  LpRangeFilter<float> &getSizeRangeFilter() { return sizeRangeFilter; }

  LpRangeFilter<int> &getMipRangeFilter() { return mipRangeFilter; }
  LpRangeFilter<int> &getLodsRangeFilter() { return lodsRangeFilter; }
  LpRangeFilter<int> &getResolutionRangeFilter() { return resolutionRangeFilter; }
  LpRangeFilter<int> &getTextureUsageRangeFilter() { return textureUsageRangeFilter; }

  void setTextureUsageFilters(bool non_ri_flag, bool unique_flag, bool shared_flag);
  bool getTextureUsageFilterNonRI() const { return isTextureUsageFilterNonRI; }
  bool getTextureUsageFilterUnique() const { return isTextureUsageFilterUnique; }
  bool getTextureUsageFilterShared() const { return isTextureUsageFilterShared; }


  eastl::vector<ProfilerString> getUniqueFormats() const;


  eastl::vector<int> getUniqueWidths() const;


  eastl::vector<int> getUniqueHeights() const;

  // Helper structure for string filter multi-select context
  struct StringFilterContext
  {
    const eastl::vector<ProfilerString> *values = nullptr;
    CheckFilter<ProfilerString> *filter = nullptr;
  };

  // Helper structure for integer filter multi-select context
  struct IntFilterContext
  {
    const eastl::vector<int> *values = nullptr;
    CheckFilter<int> *filter = nullptr;
  };

  // Adapter for updating string filter selection state
  static void updateStringFilterSelection(LpSelectionExternalStorage *storage, int index, bool selected);

  // Adapter for updating integer filter selection state
  static void updateIntFilterSelection(LpSelectionExternalStorage *storage, int index, bool selected);

  void drawMultiSelectFilter(const char *popup_name, eastl::vector<ProfilerString> &unique_values,
    CheckFilter<ProfilerString> &filter_object, bool *changed);

  void drawMultiSelectFilter(const char *popup_name, eastl::vector<int> &unique_values, CheckFilter<int> &filter_object, bool *changed,
    bool use_custom_labels = false);

  void setNameSearch(const char *search_text);
  const char *getNameSearch() const;


  void addNameMask(const ProfilerString &mask, bool is_include);


  void removeNameMask(int index);
  void clearNameMasks();
  const eastl::vector<NameMask> &getNameMasks() const;
  bool isNameFilterActive() const;

  void setTextureUsageSearch(const char *search_text);
  const char *getTextureUsageSearch() const;


  void addTextureUsageMask(const ProfilerString &mask, bool is_include);


  void removeTextureUsageMask(int index);
  void clearTextureUsageMasks();
  const eastl::vector<NameMask> &getTextureUsageMasks() const;
  bool isTextureUsageFilterActive() const;

  LpIncludeExcludeFilter &getNameFilterComponent() { return nameFilter; }

  LpIncludeExcludeFilter &getTextureUsageFilterComponent() { return textureUsageFilter; }

private:
  RIModule *riModule = nullptr;
  LpIncludeExcludeFilter nameFilter;
  LpIncludeExcludeFilter textureUsageFilter;

  CheckFilter<ProfilerString> formatFilter;
  CheckFilter<int> widthFilter, heightFilter;

  LpRangeFilter<float> sizeRangeFilter;

  LpRangeFilter<int> lodsRangeFilter;
  LpRangeFilter<int> resolutionRangeFilter;
  LpRangeFilter<int> mipRangeFilter;
  LpRangeFilter<int> textureUsageRangeFilter;

  bool isTextureUsageFilterNonRI = true;
  bool isTextureUsageFilterUnique = true;
  bool isTextureUsageFilterShared = true;
};

} // namespace levelprofiler