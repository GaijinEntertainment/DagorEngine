// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <cctype>
#include <cstring>
#include <EASTL/algorithm.h>
#include "levelProfilerUI.h"
#include "levelProfilerFilter.h"
#include "textureModule.h"
#include "riModule.h"

namespace levelprofiler
{

static constexpr int TEXTURE_PROFILER_TAB_INDEX = 0;

static TextureModule *getTextureModule()
{
  auto profilerUI = static_cast<TextureProfilerUI *>(ILevelProfiler::getInstance()->getTab(TEXTURE_PROFILER_TAB_INDEX)->module);
  return profilerUI->getTextureModule();
}

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

const char *LpIncludeExcludeFilter::getSearchText() const { return searchBuffer; }

void LpIncludeExcludeFilter::addMask(const ProfilerString &mask, bool is_include) { masks.emplace_back(mask, is_include); }

void LpIncludeExcludeFilter::removeMask(int index)
{
  if (index >= 0 && static_cast<size_t>(index) < masks.size())
    masks.erase(masks.begin() + index);
}

void LpIncludeExcludeFilter::clearMasks() { masks.clear(); }

const eastl::vector<NameMask> &LpIncludeExcludeFilter::getMasks() const { return masks; }

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

void LpIncludeExcludeFilter::drawPopup(const char *popup_id)
{
  if (!ImGui::BeginPopup(popup_id))
    return;

  drawInline();
  ImGui::EndPopup();
}

// === Filter UI Components ===

void LpFilterPopupManager::registerPopup(TableColumn table_column, const char *popup_identifier, FilterDrawCallback draw_func)
{
  entries.push_back({table_column, popup_identifier, draw_func});
}

void LpFilterPopupManager::open(TableColumn table_column) { columnToOpen = table_column; }

void LpFilterPopupManager::drawAll()
{
  for (auto &entry : entries)
  {
    if (entry.column == columnToOpen)
    {
      ImGui::OpenPopup(entry.popupId);
      columnToOpen = COLUMN_NONE; // Reset after opening
    }

    if (ImGui::BeginPopup(entry.popupId))
    {
      entry.drawFunction();
      ImGui::EndPopup();
    }
  }
}

// === Master Filter Manager ===

void TextureFilterManager::setNameSearch(const char *search_text)
{
  nameFilter.setSearchText(search_text);
  applyFilters();
}

const char *TextureFilterManager::getNameSearch() const { return nameFilter.getSearchText(); }

void TextureFilterManager::addNameMask(const ProfilerString &mask, bool is_include)
{
  nameFilter.addMask(mask, is_include);
  applyFilters();
}

void TextureFilterManager::removeNameMask(int index)
{
  nameFilter.removeMask(index);
  applyFilters();
}

void TextureFilterManager::clearNameMasks()
{
  nameFilter.clearMasks();
  applyFilters();
}

const eastl::vector<NameMask> &TextureFilterManager::getNameMasks() const { return nameFilter.getMasks(); }

bool TextureFilterManager::isNameFilterActive() const { return nameFilter.isActive(); }


void TextureFilterManager::setTextureUsageSearch(const char *search_text)
{
  textureUsageFilter.setSearchText(search_text);
  applyFilters();
}

const char *TextureFilterManager::getTextureUsageSearch() const { return textureUsageFilter.getSearchText(); }

void TextureFilterManager::addTextureUsageMask(const ProfilerString &mask, bool is_include)
{
  textureUsageFilter.addMask(mask, is_include);
  applyFilters();
}

void TextureFilterManager::removeTextureUsageMask(int index)
{
  textureUsageFilter.removeMask(index);
  applyFilters();
}

void TextureFilterManager::clearTextureUsageMasks()
{
  textureUsageFilter.clearMasks();
  applyFilters();
}

const eastl::vector<NameMask> &TextureFilterManager::getTextureUsageMasks() const { return textureUsageFilter.getMasks(); }

bool TextureFilterManager::isTextureUsageFilterActive() const { return textureUsageFilter.isActive(); }


TextureFilterManager::TextureFilterManager() :
  // Initialize with reasonable defaults for filter ranges
  mipRangeFilter(1, 12),           // Typical mip level range
  sizeRangeFilter(0.0f, 100.0f),   // Size range in MB
  lodsRangeFilter(1, 6),           // LOD levels range
  resolutionRangeFilter(32, 4096), // Typical resolution range
  textureUsageRangeFilter(2, 10)   // Texture Usage count range, min 2 for shared
{
  isTextureUsageFilterNonRI = true;
  isTextureUsageFilterUnique = true;
  isTextureUsageFilterShared = true;

  mipRangeFilter.setAbsoluteBounds(0, 16);
  sizeRangeFilter.setAbsoluteBounds(0, 128.0f);
  textureUsageRangeFilter.setAbsoluteBounds(2, 100);
}

TextureFilterManager::~TextureFilterManager() {}

bool TextureFilterManager::passesAllFilters(const ProfilerString &texture_name, const TextureData &texture_data,
  const TextureUsage *texture_usage_ptr)
{
  if (!nameFilter.pass(texture_name))
    return false;

  auto formatName = TextureModule::getFormatName(texture_data.info.cflg);
  if (!formatFilter.pass(formatName))
    return false;

  // Width filter (use -1 for "Other" values)
  {
    auto textureModule = getTextureModule();
    auto uniqueWidths = textureModule->getUniqueWidths();
    int widthKey = texture_data.info.w;
    if (eastl::find(uniqueWidths.begin(), uniqueWidths.end(), widthKey) == uniqueWidths.end())
      widthKey = -1;

    if (!widthFilter.pass(widthKey))
      return false;
  }

  {
    auto textureModule = getTextureModule();
    auto uniqueHeights = textureModule->getUniqueHeights();
    int heightKey = texture_data.info.h;
    if (eastl::find(uniqueHeights.begin(), uniqueHeights.end(), heightKey) == uniqueHeights.end())
      heightKey = -1;

    if (!heightFilter.pass(heightKey))
      return false;
  }

  {
    float mipLevel = static_cast<float>(texture_data.info.mipLevels);
    if (!mipRangeFilter.pass(mipLevel))
      return false;
  }

  {
    auto textureModule = getTextureModule();
    float sizeInMB = textureModule->getTextureMemorySize(texture_data);
    if (!sizeRangeFilter.pass(sizeInMB))
      return false;
  }

  // LOD filter (proxy for texture complexity)
  {
    int lodCount = texture_data.info.mipLevels;
    if (!lodsRangeFilter.pass(lodCount))
      return false;
  }

  // Resolution is determined by the larger of the texture's width or height.
  {
    int maxResolution = eastl::max(texture_data.info.w, texture_data.info.h);
    if (!resolutionRangeFilter.pass(maxResolution))
      return false;
  }

  TextureUsage defaultTextureUsageInfo{}; // Zero-initialized for cases where usage_ptr is null.
  const TextureUsage &textureUsage = texture_usage_ptr ? *texture_usage_ptr : defaultTextureUsageInfo;

  bool isNonRi = (textureUsage.unique == 0);
  bool isUnique = (textureUsage.unique == 1);
  bool isShared = (textureUsage.unique >= 2);

  if (isNonRi && !isTextureUsageFilterNonRI)
    return false;
  if (isUnique && !isTextureUsageFilterUnique)
    return false;
  if (isShared && !isTextureUsageFilterShared)
    return false;

  // Shared textures have an additional filter based on their usage count.
  if (isShared && !textureUsageRangeFilter.pass(textureUsage.unique))
    return false;

  return true;
}

bool TextureFilterManager::isColumnFilterActive(TableColumn table_column) const
{
  switch (table_column)
  {
    case COL_NAME: return nameFilter.isActive();
    case COL_FORMAT: return !formatFilter.getOffList().empty();
    case COL_WIDTH: return !widthFilter.getOffList().empty();
    case COL_HEIGHT: return !heightFilter.getOffList().empty();
    case COL_MIPS: return mipRangeFilter.isUsingMin() || mipRangeFilter.isUsingMax();
    case COL_MEM_SIZE: return sizeRangeFilter.isUsingMin() || sizeRangeFilter.isUsingMax();
    case COL_TEX_USAGE:
      return !isTextureUsageFilterNonRI || !isTextureUsageFilterUnique || !isTextureUsageFilterShared ||
             textureUsageRangeFilter.isUsingMin() || textureUsageRangeFilter.isUsingMax() || textureUsageFilter.isActive();
    default: return false;
  }
}

void TextureFilterManager::resetAllFilters()
{
  nameFilter.reset();
  formatFilter.reset();
  widthFilter.reset();
  heightFilter.reset();

  auto profilerUI = static_cast<TextureProfilerUI *>(ILevelProfiler::getInstance()->getTab(TEXTURE_PROFILER_TAB_INDEX)->module);
  auto textureModule = profilerUI->getTextureModule();
  auto riModuleInstance = profilerUI->getRIModule();

  int minMip = textureModule->getMipMinDefault();
  int maxMip = textureModule->getMipMaxDefault();
  float minSize = textureModule->getMemorySizeMinDefault();
  float maxSize = textureModule->getMemorySizeMaxDefault();
  int minTextureUsage = 2; // Minimum for shared textures
  int maxTextureUsageCount = riModuleInstance->getMaxUniqueTextureUsageCount();

  mipRangeFilter.reset(minMip, maxMip);
  mipRangeFilter.setAbsoluteBounds(minMip, maxMip);

  sizeRangeFilter.reset(minSize, maxSize);
  sizeRangeFilter.setAbsoluteBounds(minSize, maxSize);

  textureUsageRangeFilter.reset(minTextureUsage, maxTextureUsageCount);
  textureUsageRangeFilter.setAbsoluteBounds(minTextureUsage, maxTextureUsageCount);

  lodsRangeFilter.reset(1, 6);
  resolutionRangeFilter.reset(32, 4096);

  isTextureUsageFilterNonRI = true;
  isTextureUsageFilterUnique = true;
  isTextureUsageFilterShared = true;
}

void TextureFilterManager::applyFilters()
{
  auto profilerUI = static_cast<TextureProfilerUI *>(ILevelProfiler::getInstance()->getTab(TEXTURE_PROFILER_TAB_INDEX)->module);
  auto textureModule = profilerUI->getTextureModule();
  auto riModuleInstance = profilerUI->getRIModule();

  const auto &allTextures = textureModule->getTextures();
  const auto &textureUsageMap = riModuleInstance->getTextureUsage();

  auto &currentFilteredTextures = const_cast<eastl::vector<ProfilerString> &>(textureModule->getFilteredTextures());
  currentFilteredTextures.clear();

  for (const auto &[textureName, textureData] : allTextures)
  {
    // If the texture usage filter (by asset name) is active, check if any asset using this texture passes.
    if (textureUsageFilter.isActive())
    {
      const auto &textureToAssetMap = riModuleInstance->getTextureToAssetsMap();
      auto assetIterator = textureToAssetMap.find(textureName);

      if (assetIterator == textureToAssetMap.end())
        continue; // Texture is not used by any asset, so it cannot pass the asset name filter.

      bool anyAssetMatch = false;
      for (const auto &assetName : assetIterator->second)
      {
        if (textureUsageFilter.pass(assetName.c_str()))
        {
          anyAssetMatch = true;
          break;
        }
      }

      if (!anyAssetMatch)
        continue;
    }

    const TextureUsage *textureUsage = nullptr;
    auto textureUsageIterator = textureUsageMap.find(textureName);
    if (textureUsageIterator != textureUsageMap.end())
      textureUsage = &textureUsageIterator->second;

    if (passesAllFilters(textureName, textureData, textureUsage))
      currentFilteredTextures.push_back(textureName);
  }

  textureModule->rebuildFilteredList();
}

void TextureFilterManager::setTextureUsageFilters(bool non_ri_flag, bool unique_flag, bool shared_flag)
{
  isTextureUsageFilterNonRI = non_ri_flag;
  isTextureUsageFilterUnique = unique_flag;
  isTextureUsageFilterShared = shared_flag;
  applyFilters();
}

void TextureFilterManager::setRIModule(RIModule *module) { riModule = module; }

eastl::vector<ProfilerString> TextureFilterManager::getUniqueFormats() const { return getTextureModule()->getUniqueFormats(); }

eastl::vector<int> TextureFilterManager::getUniqueWidths() const { return getTextureModule()->getUniqueWidths(); }

eastl::vector<int> TextureFilterManager::getUniqueHeights() const { return getTextureModule()->getUniqueHeights(); }


void TextureFilterManager::updateStringFilterSelection(LpSelectionExternalStorage *storage, int index, bool is_selected)
{
  auto context = static_cast<StringFilterContext *>(storage->userData);
  if (!context || !context->values || !context->filter)
    return;

  const ProfilerString &value = (*context->values)[index];
  if (is_selected)
  {
    auto &offList = const_cast<eastl::hash_set<ProfilerString> &>(context->filter->getOffList());
    offList.erase(value);
  }
  else
    context->filter->toggle(value);
}


void TextureFilterManager::updateIntFilterSelection(LpSelectionExternalStorage *storage, int index, bool is_selected)
{
  auto context = static_cast<IntFilterContext *>(storage->userData);
  if (!context || !context->values || !context->filter)
    return;

  int value = (*context->values)[index];
  if (is_selected)
  {
    auto &offList = const_cast<eastl::hash_set<int> &>(context->filter->getOffList());
    offList.erase(value);
  }
  else
    context->filter->toggle(value);
}

} // namespace levelprofiler