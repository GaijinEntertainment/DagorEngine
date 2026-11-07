// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/type_traits.h>
#include "levelProfilerInterface.h"

namespace levelprofiler
{


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

  void retainExcludedIn(const eastl::vector<Key> &all_values)
  {
    eastl::hash_set<Key> tempSet;
    for (auto &value : all_values)
    {
      if (offSet.find(value) != offSet.end())
      {
        tempSet.insert(value);
      }
    }
    offSet.swap(tempSet);
  }

  void setPassed(const Key &value, bool passed)
  {
    if (passed)
      offSet.erase(value);
    else
      offSet.insert(value);
  }

  bool isActive() const { return !offSet.empty(); }

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


  void addMask(const ProfilerString &mask, bool is_include);


  void removeMask(int index);
  void clearMasks();

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

  // Moves the bounds onto freshly collected data. Each side the user did not set follows its new
  // bound, so no thumb is left off the slider track; a side they did set is kept, because pass()
  // works against a range wider than the data and clamping would collapse a partly overlapping
  // range with no way back. Inverted input means nothing in the data is selectable, so it does
  // nothing at all.
  void rebind(T new_min_value, T new_max_value)
  {
    if (new_min_value > new_max_value)
      return;

    defaultMin = absoluteMin = new_min_value;
    defaultMax = absoluteMax = new_max_value;

    if (!useMin)
      minValue = new_min_value;
    if (!useMax)
      maxValue = new_max_value;
  }

  // For a value whose unit changes by a positive factor: the filter then passes the same rows, up to float rounding.
  void scale(T factor)
  {
    minValue *= factor;
    maxValue *= factor;
    defaultMin *= factor;
    defaultMax *= factor;
    absoluteMin *= factor;
    absoluteMax *= factor;
  }

  void setRange(T new_min_value, T new_max_value)
  {
    minValue = new_min_value;
    maxValue = new_max_value;
  }

  void setUseMin(bool should_use_min) { useMin = should_use_min; }

  void setUseMax(bool should_use_max) { useMax = should_use_max; }

  T getMin() const { return minValue; }

  T getMax() const { return maxValue; }

  T getAbsoluteMin() const { return absoluteMin; }

  T getAbsoluteMax() const { return absoluteMax; }

  bool isActive() const { return useMin || useMax; }

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

} // namespace levelprofiler