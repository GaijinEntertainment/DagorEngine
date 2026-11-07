// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <cctype>
#include <cstring>
#include <EASTL/algorithm.h>
#include "levelProfilerFilter.h"
#include "levelProfilerWidgets.h"

namespace levelprofiler
{

// === Base Filters ===

namespace ImGuiUtils
{
LpMultiSelectIO *begin_multi_select(LpMultiSelectFlags flags, int dirty_selection, int item_count)
{
  return LpMultiSelect::begin_multi_select(flags, dirty_selection, item_count);
}

LpMultiSelectIO *end_multi_select() { return LpMultiSelect::end_multi_select(); }
} // namespace ImGuiUtils

// === Text Filters ===

LpIncludeExcludeFilter::LpIncludeExcludeFilter() { searchBuffer[0] = '\0'; }

LpIncludeExcludeFilter::~LpIncludeExcludeFilter() = default;

void LpIncludeExcludeFilter::reset()
{
  searchBuffer[0] = '\0';
  masks.clear();

  newIncludeState = true;
  newMaskBuffer.clear();
  oldSearchBuffer.clear();
}

bool LpIncludeExcludeFilter::pass(const ProfilerString &value) const
{
  const char *text = value.c_str();

  // Check search text first (quick reject)
  if (searchBuffer[0] && !matchesSearch(text))
    return false;

  // Check include/exclude masks
  if (!matchesMasks(text))
    return false;

  return true;
}

void LpIncludeExcludeFilter::setSearchText(const char *text)
{
  if (text && strlen(text) < sizeof(searchBuffer))
    strcpy(searchBuffer, text);
  else
    searchBuffer[0] = '\0';
}

void LpIncludeExcludeFilter::addMask(const ProfilerString &mask, bool is_include) { masks.emplace_back(mask, is_include); }

void LpIncludeExcludeFilter::removeMask(int index)
{
  if (index >= 0 && static_cast<size_t>(index) < masks.size())
    masks.erase(masks.begin() + index);
}

void LpIncludeExcludeFilter::clearMasks() { masks.clear(); }

bool LpIncludeExcludeFilter::matchesSearch(const char *text) const
{
  size_t length = strlen(searchBuffer);
  if (length == 0)
    return true;

  size_t textLength = strlen(text);
  if (length > textLength)
    return false;

  for (size_t i = 0; i <= textLength - length; ++i)
  {
    bool isMatch = true;
    for (size_t j = 0; j < length; ++j)
    {
      if (tolower(static_cast<unsigned char>(text[i + j])) != tolower(static_cast<unsigned char>(searchBuffer[j])))
      {
        isMatch = false;
        break;
      }
    }
    if (isMatch)
      return true;
  }

  return false;
}

bool LpIncludeExcludeFilter::matchesMasks(const char *text) const
{
  bool hasIncludeMask = false;
  for (const auto &mask : masks)
  {
    if (mask.isInclude)
    {
      hasIncludeMask = true;
      break;
    }
  }

  if (hasIncludeMask)
  {
    bool passedInclude = false;
    for (const auto &mask : masks)
    {
      if (!mask.isInclude)
        continue;

      if (containsIgnoreCase(text, mask.text.c_str()))
      {
        passedInclude = true;
        break;
      }
    }

    if (!passedInclude)
      return false;
  }

  for (const auto &mask : masks)
    if (!mask.isInclude && containsIgnoreCase(text, mask.text.c_str()))
      return false;

  return true;
}

bool LpIncludeExcludeFilter::containsIgnoreCase(const char *haystack, const char *needle)
{
  size_t needleLength = strlen(needle);
  if (needleLength == 0)
    return true;

  size_t haystackLength = strlen(haystack);
  if (needleLength > haystackLength)
    return false;

  for (size_t i = 0; i <= haystackLength - needleLength; ++i)
  {
    bool isMatch = true;
    for (size_t j = 0; j < needleLength; ++j)
    {
      if (tolower(static_cast<unsigned char>(haystack[i + j])) != tolower(static_cast<unsigned char>(needle[j])))
      {
        isMatch = false;
        break;
      }
    }
    if (isMatch)
      return true;
  }

  return false;
}

bool LpIncludeExcludeFilter::isActive() const { return searchBuffer[0] != '\0' || !masks.empty(); }

bool LpIncludeExcludeFilter::drawInline()
{
  bool changed = false;

  // Include/Exclude toggle buttons
  ImGui::PushStyleColor(ImGuiCol_Button, newIncludeState ? IM_COL32(0, 160, 0, 200) : IM_COL32(100, 100, 100, 100));
  if (ImGui::Button("Include"))
  {
    if (!newIncludeState)
    {
      newIncludeState = true;
      changed = true;
    }
  }
  ImGui::PopStyleColor();

  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Button, newIncludeState ? IM_COL32(100, 100, 100, 100) : IM_COL32(160, 0, 0, 200));
  if (ImGui::Button("Exclude"))
  {
    if (newIncludeState)
    {
      newIncludeState = false;
      changed = true;
    }
  }
  ImGui::PopStyleColor();

  // New mask input field and button
  ImGui::SameLine();
  ImGui::SetNextItemWidth(120);

  static constexpr size_t maxMaskLength = 64;
  newMaskBuffer.resize(maxMaskLength);
  bool addRequested = false;
  if (ImGui::InputTextWithHint("##mask", "Mask...", newMaskBuffer.data(), newMaskBuffer.capacity(),
        ImGuiInputTextFlags_EnterReturnsTrue))
  {
    addRequested = true;
    newMaskBuffer.resize(strlen(newMaskBuffer.c_str()));
  }

  ImGui::SameLine();
  if (ImGui::Button("Add"))
    addRequested = true;

  if (addRequested && !newMaskBuffer.empty())
  {
    addMask(newMaskBuffer, newIncludeState);
    newMaskBuffer.clear();
    changed = true;
  }

  ImGui::Separator();

  // Display existing masks with remove buttons
  for (size_t i = 0; i < masks.size(); ++i)
  {
    const auto &mask = masks[i];

    // Color-code based on include/exclude
    ImGui::PushStyleColor(ImGuiCol_Button, mask.isInclude ? IM_COL32(0, 160, 0, 200) : IM_COL32(160, 0, 0, 200));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));

    ProfilerString tempBuffer;
    tempBuffer = ProfilerString(1, mask.isInclude ? '+' : '-') + " " + mask.text;
    ImGui::Button(tempBuffer.c_str());

    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    if (ProfilerString buttonBuffer; buttonBuffer.sprintf("x##%d", static_cast<int>(i)), ImGui::SmallButton(buttonBuffer.c_str()))
    {
      removeMask(static_cast<int>(i));
      changed = true;
      break; // Break to avoid issues with changed collection
    }
  }

  if (!masks.empty())
  {
    ImGui::NewLine();
    if (ImGui::Button("Clear All"))
    {
      clearMasks();
      changed = true;
    }
  }

  // Track search text changes
  if (ProfilerString currentSearchBuffer = searchBuffer; oldSearchBuffer != currentSearchBuffer)
  {
    oldSearchBuffer = currentSearchBuffer;
    changed = true;
  }

  return changed;
}

} // namespace levelprofiler