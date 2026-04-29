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
  bool isColumnFilterActive(ColumnIndex column_index) const;


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
  LpRangeFilter<int> &getAssetInstanceCountRangeFilter() { return assetInstanceCountRangeFilter; }

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
  LpRangeFilter<int> assetInstanceCountRangeFilter;

  bool isTextureUsageFilterNonRI = true;
  bool isTextureUsageFilterUnique = true;
  bool isTextureUsageFilterShared = true;
};

} // namespace levelprofiler