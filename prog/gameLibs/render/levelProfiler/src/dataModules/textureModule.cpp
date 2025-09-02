// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/sort.h>
#include <EASTL/algorithm.h>
#include <3d/dag_texIdSet.h>
#include <drv/3d/dag_rwResource.h>
#include <shaders/dag_shaderResUnitedData.h>
#include "levelProfilerUI.h"
#include "textureModule.h"

namespace levelprofiler
{

// Conversion constant: 1 MB = 1024 * 1024 bytes
static const float BYTES_TO_MEGABYTES = 1048576.0f;

// Texture dimension threshold for filtering significant sizes
static const int TEXTURE_SIZE_THRESHOLD = 256;

// Number of RGBA channels in texture
static constexpr int RGBA_CHANNEL_COUNT = 4;

// Bit mask for all RGBA channels enabled (RGBA = 1+2+4+8 = 15)
static constexpr int RGBA_CHANNELS_MASK = (1 << RGBA_CHANNEL_COUNT) - 1;

// Number of style colors pushed for channel buttons (Button, ButtonHovered, ButtonActive)
static const int CHANNEL_BUTTON_STYLE_COUNT = 3;

// Number of frames to process one texture during data collection
static constexpr int FRAMES_PER_TEXTURE = 3; // 3-5 frames is enough to download texture in memory

TextureModule::TextureModule()
{
  for (int i = 0; i < RGBA_CHANNEL_COUNT; ++i)
    channelMask[i] = true; // Default state - all channels visible
}

TextureModule::~TextureModule() { shutdownImpl(); }

void TextureModule::init()
{
  imguiChannelMaskVariableId = get_shader_variable_id("imgui_channel_mask", true);
  imguiMipLevelVariableId = get_shader_variable_id("imgui_mip_level", true);
  imguiUseCustomSamplerVariableId = get_shader_variable_id("imgui_use_custom_sampler", true);

  // Create point sampler for texture preview (nearest-neighbor filtering)
  d3d::SamplerInfo samplerInfo;
  samplerInfo.filter_mode = d3d::FilterMode::Point;
  pointSamplerHandle = d3d::request_sampler(samplerInfo);

  // Making the initial data collection for plugs
  collect();
}

void TextureModule::shutdown() { shutdownImpl(); }

void TextureModule::shutdownImpl() { clearImpl(); }

void TextureModule::clear() { clearImpl(); }

void TextureModule::clearImpl()
{
  textures.clear();
  filteredTextures.clear();
  selectedTextureName.clear();

  totalCount = filteredCount = 0;
  totalMemorySize = filteredMemorySize = 0;

  isCollectingData = false;
  texturesToCollect.clear();
  currentCollectionIndex = 0;
  collectionFrameCounter = 0;
}

void TextureModule::collect()
{
  clear();

  // Primary collection - get plug data
  for (TEXTUREID id = first_managed_texture(1); id != BAD_TEXTUREID; id = next_managed_texture(id, 1))
  {
    if (auto texturePtr = acquire_managed_tex(id))
    {
      TextureInfo textureInfo;
      texturePtr->getinfo(textureInfo);

      // Only track 2D textures with valid dimensions
      if (textureInfo.w > 0 && textureInfo.h > 0)
      {
        uint64_t sizeBytes = texturePtr->getSize();
        TextureData textureData(textureInfo, sizeBytes);
        textures.insert({ProfilerString(texturePtr->getTexName()), textureData});
      }

      release_managed_tex(id);
    }
  }

  mipMinDefault = INT_MAX;
  mipMaxDefault = INT_MIN;
  sizeMinDefault = FLT_MAX;
  sizeMaxDefault = 0.0f;

  for (const auto &[textureName, textureData] : textures)
  {
    mipMinDefault = eastl::min(mipMinDefault, (int)textureData.info.mipLevels);
    mipMaxDefault = eastl::max(mipMaxDefault, (int)textureData.info.mipLevels);

    float currentSizeMB = getTextureMemorySize(textureData);
    sizeMinDefault = eastl::min(sizeMinDefault, currentSizeMB);
    sizeMaxDefault = eastl::max(sizeMaxDefault, currentSizeMB);
  }

  // Handle edge cases for empty data
  if (mipMinDefault > mipMaxDefault)
    mipMinDefault = mipMaxDefault = 0;

  if (sizeMinDefault > sizeMaxDefault)
    sizeMinDefault = sizeMaxDefault = 0.0f;

  filteredTextures.clear();
  filteredTextures.reserve(textures.size());
  for (const auto &[textureName, textureData] : textures)
    filteredTextures.push_back(textureName);

  recalculateStatistics();
}

void TextureModule::startDataCollection()
{
  isCollectingData = true;
  currentCollectionIndex = 0;
  collectionFrameCounter = 0;

  texturesToCollect.clear();
  texturesToCollect.reserve(textures.size());
  for (const auto &[textureName, textureData] : textures)
    texturesToCollect.push_back(textureName);

  debug("TextureModule: Starting data collection for %zu textures", texturesToCollect.size());

  if (texturesToCollect.empty())
  {
    isCollectingData = false;
    debug("TextureModule: No textures to collect, aborting");
  }
}

const ProfilerString &TextureModule::getCurrentCollectionTexture() const
{
  static ProfilerString empty;
  if (!isCollectingData || texturesToCollect.empty() || currentCollectionIndex >= texturesToCollect.size())
    return empty;

  return selectedTextureName;
}

void TextureModule::updateDataCollection()
{
  if (!isCollectingData || texturesToCollect.empty())
    return;

  // process one texture for several frames
  if (currentCollectionIndex < texturesToCollect.size())
  {
    const ProfilerString &currentTexture = texturesToCollect[currentCollectionIndex];

    if (collectionFrameCounter == 0)
    {
      // First frame -select the texture for the preview
      selectTexture(currentTexture.c_str());

      // Update the choice in the table through Ilevelprofler
      if (ILevelProfiler *profiler = ILevelProfiler::getInstance())
      {
        if (ProfilerTab *tab = profiler->getTab(0); tab && tab->module)
        {
          TextureProfilerUI *ui = static_cast<TextureProfilerUI *>(tab->module);
          if (ui && ui->getTextureTable())
            ui->getTextureTable()->setSelectedTexture(currentTexture);
        }
      }

      debug("TextureModule: Selected texture %s for preview (%zu/%zu)", currentTexture.c_str(), currentCollectionIndex + 1,
        texturesToCollect.size());
    }
    else if (collectionFrameCounter >= 2 && collectionFrameCounter < FRAMES_PER_TEXTURE - 1)
    {
      // Medium frames - force the loading of texture
      if (TEXTUREID id = get_managed_texture_id(currentTexture.c_str()); id != BAD_TEXTUREID)
      {
        if (auto texturePtr = acquire_managed_tex(id))
        {
          texturePtr->texmiplevel(0, 0);

          TextureInfo tempInfo;
          texturePtr->getinfo(tempInfo, 0);

          release_managed_tex(id);
        }
      }
    }
    else if (collectionFrameCounter == FRAMES_PER_TEXTURE - 1)
    {
      // Last frame -collect data after drawing
      if (TEXTUREID id = get_managed_texture_id(currentTexture.c_str()); id != BAD_TEXTUREID)
      {
        if (auto texturePtr = acquire_managed_tex(id))
        {
          texturePtr->texmiplevel(0, 0);

          TextureInfo textureInfo;
          texturePtr->getinfo(textureInfo, 0);

          uint64_t sizeBytes = texturePtr->getSize();
          TextureData textureData(textureInfo, sizeBytes);

          auto oldData = textures[currentTexture];
          if (oldData.info.w != textureInfo.w || oldData.info.h != textureInfo.h || oldData.info.mipLevels != textureInfo.mipLevels ||
              oldData.sizeBytes != sizeBytes)
          {
            debug("TextureModule: Updated %s - old: %dx%d (%d mips, %llu bytes), new: %dx%d (%d mips, %llu bytes)",
              currentTexture.c_str(), oldData.info.w, oldData.info.h, oldData.info.mipLevels, oldData.sizeBytes, textureInfo.w,
              textureInfo.h, textureInfo.mipLevels, sizeBytes);
          }

          // update data with correct values of source
          textures[currentTexture] = textureData;

          release_managed_tex(id);
        }
      }
    }

    collectionFrameCounter++;

    // Go to the next texture
    if (collectionFrameCounter >= FRAMES_PER_TEXTURE)
    {
      collectionFrameCounter = 0;
      currentCollectionIndex++;
    }
  }
  else
  {
    isCollectingData = false;
    selectedTextureName.clear();
    collectionFrameCounter = 0;

    debug("TextureModule: Data collection complete for %zu textures", texturesToCollect.size());

    mipMinDefault = INT_MAX;
    mipMaxDefault = INT_MIN;
    sizeMinDefault = FLT_MAX;
    sizeMaxDefault = 0.0f;

    for (const auto &[textureName, textureData] : textures)
    {
      mipMinDefault = eastl::min(mipMinDefault, (int)textureData.info.mipLevels);
      mipMaxDefault = eastl::max(mipMaxDefault, (int)textureData.info.mipLevels);

      float currentSizeMB = getTextureMemorySize(textureData);
      sizeMinDefault = eastl::min(sizeMinDefault, currentSizeMB);
      sizeMaxDefault = eastl::max(sizeMaxDefault, currentSizeMB);
    }

    if (mipMinDefault > mipMaxDefault)
      mipMinDefault = mipMaxDefault = 0;

    if (sizeMinDefault > sizeMaxDefault)
      sizeMinDefault = sizeMaxDefault = 0.0f;

    recalculateStatistics();

    texturesToCollect.clear();

    if (ILevelProfiler *profiler = ILevelProfiler::getInstance())
    {
      if (ProfilerTab *tab = profiler->getTab(0); tab && tab->module)
      {
        TextureProfilerUI *ui = static_cast<TextureProfilerUI *>(tab->module);
        if (ui && ui->getTextureTable())
          ui->getTextureTable()->setSelectedTexture("");
      }
    }
  }
}

float TextureModule::getCollectionProgress() const
{
  if (!isCollectingData || texturesToCollect.empty())
    return 1.0f; // if the collection does not go, we think that it is completed

  return (float)currentCollectionIndex / (float)texturesToCollect.size();
}

void TextureModule::drawUI()
{
  // UI is processed in TextureProfilerui
  // Updatedatacollection is now called from LevelProfailerui :: DRAWUI
}

const eastl::hash_map<ProfilerString, TextureData> &TextureModule::getTextures() const { return textures; }

const eastl::vector<ProfilerString> &TextureModule::getFilteredTextures() const { return filteredTextures; }

float TextureModule::getTextureMemorySize(const TextureData &texture_data) const
{
  if (texture_data.sizeBytes == 0)
    return 0.0f;

  // Convert from bytes to megabytes
  return float(texture_data.sizeBytes) / BYTES_TO_MEGABYTES;
}

void TextureModule::selectTexture(const char *texture_name)
{
  selectedTextureName = texture_name ? texture_name : "";
  selectedMipLevel = 0; // Reset to base mip level
}

void TextureModule::drawTextureView(const char *texture_name)
{
  // Use provided name or currently selected texture
  ProfilerString currentTextureName = texture_name ? texture_name : selectedTextureName;
  if (currentTextureName.empty())
    return;

  // Look up texture info
  auto iterator = textures.find(currentTextureName);
  if (iterator == textures.end())
    return;

  // Get engine texture ID
  TEXTUREID textureId = get_managed_texture_id(currentTextureName.c_str());
  if (textureId == BAD_TEXTUREID)
    return;

  // If the data is collected, force the loading of the basic mine
  if (isCollectingData)
  {
    if (auto texturePtr = acquire_managed_tex(textureId))
    {
      texturePtr->texmiplevel(0, 0);
      release_managed_tex(textureId);
    }
  }

  // Get texture info for UI
  auto &currentTextureData = iterator->second;
  auto &currentTextureInfo = currentTextureData.info;

  // Channel selector buttons (R/G/B/A) with color coding
  {
    static constexpr const char *labels[RGBA_CHANNEL_COUNT] = {"R", "G", "B", "A"};
    // Color schemes for each channel button
    const ImVec4 activeCol[RGBA_CHANNEL_COUNT] = {
      ImVec4(0.90f, 0.20f, 0.20f, 0.85f), // Red channel
      ImVec4(0.20f, 0.90f, 0.20f, 0.85f), // Green channel
      ImVec4(0.20f, 0.20f, 0.90f, 0.85f), // Blue channel
      ImVec4(0.85f, 0.85f, 0.85f, 0.85f), // Alpha channel
    };
    const ImVec4 hoverCol[RGBA_CHANNEL_COUNT] = {
      ImVec4(1.00f, 0.30f, 0.30f, 1.00f),
      ImVec4(0.30f, 1.00f, 0.30f, 1.00f),
      ImVec4(0.30f, 0.30f, 1.00f, 1.00f),
      ImVec4(1.00f, 1.00f, 1.00f, 1.00f),
    };
    const ImVec4 disabledCol[RGBA_CHANNEL_COUNT] = {
      ImVec4(0.50f, 0.10f, 0.10f, 0.20f),
      ImVec4(0.10f, 0.50f, 0.10f, 0.20f),
      ImVec4(0.10f, 0.10f, 0.50f, 0.20f),
      ImVec4(0.50f, 0.50f, 0.50f, 0.20f),
    };

    ImGuiStyle &style = ImGui::GetStyle();
    float oldRounding = style.FrameRounding;
    style.FrameRounding = 6.0f;

    for (int channelIndex = 0; channelIndex < RGBA_CHANNEL_COUNT; ++channelIndex)
    {
      ImGui::PushID(channelIndex);
      if (channelIndex > 0)
        ImGui::SameLine();

      bool isChannelVisible = channelMask[channelIndex];

      ImVec4 buttonColor = isChannelVisible ? activeCol[channelIndex] : disabledCol[channelIndex];
      ImVec4 hoverColor = isChannelVisible ? hoverCol[channelIndex] : disabledCol[channelIndex];
      ImVec4 activeColor = activeCol[channelIndex];

      ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);

      if (ImGui::Button(labels[channelIndex], ImVec2(30, 0)))
        isChannelVisible = !isChannelVisible; // Left click - toggle this channel

      // Right click - solo this channel (turn off others)
      if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
      {
        for (int j = 0; j < RGBA_CHANNEL_COUNT; ++j)
          channelMask[j] = false;

        isChannelVisible = true;
      }

      channelMask[channelIndex] = isChannelVisible;

      ImGui::PopStyleColor(CHANNEL_BUTTON_STYLE_COUNT);
      ImGui::PopID();
    }

    // Restore original style
    style.FrameRounding = oldRounding;
  }

  // Mip level slider
  {
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    ImGui::SliderInt("##mip_level", &selectedMipLevel, 0, currentTextureInfo.mipLevels - 1, "Mip Level: %d");
    ImGui::PopItemWidth();
  }

  // Setup custom texture preview rendering
  ImGui::GetWindowDrawList()->AddCallback(
    [](const ImDrawList *parent_list, const ImDrawCmd *cmd) {
      TextureModule *self = static_cast<TextureModule *>(cmd->UserCallbackData);
      self->setImagePreviewMode(parent_list, cmd);
    },
    this);

  // Calculate display size preserving aspect ratio
  ImVec2 availableRegion = ImGui::GetContentRegionAvail();
  float textureAspectRatio = float(currentTextureInfo.w) / currentTextureInfo.h;
  float displayWidth = availableRegion.x, displayHeight = availableRegion.y;

  if (displayWidth / displayHeight > textureAspectRatio)
    displayWidth = displayHeight * textureAspectRatio; // Constrained by height
  else
    displayHeight = displayWidth / textureAspectRatio; // Constrained by width

  ImGui::Image(reinterpret_cast<ImTextureID>(unsigned(textureId)), ImVec2(displayWidth, displayHeight));

  if (ImGui::IsItemHovered())
  {
    float sizeMB = getTextureMemorySize(currentTextureData);
    ImGui::SetTooltip("%s\n%s\n%dx%d\n%.2f MB", currentTextureName.c_str(), getFormatName(currentTextureInfo.cflg),
      currentTextureInfo.w, currentTextureInfo.h, sizeMB);
  }

  // Restore rendering state
  ImGui::GetWindowDrawList()->AddCallback(
    [](const ImDrawList *parent_list, const ImDrawCmd *cmd) {
      TextureModule *self = static_cast<TextureModule *>(cmd->UserCallbackData);
      self->resetImagePreviewMode(parent_list, cmd);
    },
    this);
}

void TextureModule::setImagePreviewMode(const ImDrawList * /* draw_list */, const ImDrawCmd * /* command */)
{
  // Encode channel mask as bit field
  int maskValue = 0;
  for (int i = 0; i < RGBA_CHANNEL_COUNT; ++i)
  {
    if (channelMask[i])
      maskValue |= 1 << i;
  }

  ShaderGlobal::set_int(imguiChannelMaskVariableId, maskValue);
  ShaderGlobal::set_int(imguiMipLevelVariableId, selectedMipLevel);
  ShaderGlobal::set_int(imguiUseCustomSamplerVariableId, 1);

  static int samplerSlot = ShaderGlobal::get_slot_by_name("custom_sampler_const_no");
  d3d::set_sampler(STAGE_PS, samplerSlot, pointSamplerHandle);
}

void TextureModule::resetImagePreviewMode(const ImDrawList * /* draw_list */, const ImDrawCmd * /* command */)
{
  ShaderGlobal::set_int(imguiChannelMaskVariableId, RGBA_CHANNELS_MASK); // Enable all RGBA channels
  ShaderGlobal::set_int(imguiUseCustomSamplerVariableId, 0);             // Use default sampler
}

void TextureModule::rebuildFilteredList()
{
  totalCount = 0;
  totalMemorySize = 0.0;

  for (const auto &[textureName, textureData] : textures)
  {
    ++totalCount;
    totalMemorySize += getTextureMemorySize(textureData);
  }

  filteredCount = 0;
  filteredMemorySize = 0.0;

  for (auto &name : filteredTextures)
  {
    ++filteredCount;
    filteredMemorySize += getTextureMemorySize(textures[name]);
  }
}

void TextureModule::initializeFilteredTextures()
{
  filteredTextures.clear();

  for (const auto &[textureName, textureData] : textures)
    filteredTextures.push_back(textureName);

  rebuildFilteredList();
}

void TextureModule::recalculateStatistics()
{
  totalCount = 0;
  totalMemorySize = 0.0;

  for (const auto &[textureName, textureData] : textures)
  {
    ++totalCount;
    totalMemorySize += getTextureMemorySize(textureData);
  }

  filteredCount = 0;
  filteredMemorySize = 0.0;

  for (auto &name : filteredTextures)
  {
    ++filteredCount;
    filteredMemorySize += getTextureMemorySize(textures[name]);
  }
}

int TextureModule::getFilteredTextureCount() const { return filteredCount; }

float TextureModule::getFilteredTotalMemorySize() const { return filteredMemorySize; }

int TextureModule::getTotalTextureCount() const { return totalCount; }

float TextureModule::getTotalMemorySize() const { return totalMemorySize; }

void TextureModule::sortFilteredTextures(ImGuiTableSortSpecs *sort_specs)
{
  if (!sort_specs || sort_specs->SpecsCount == 0)
    return;

  auto compareFunction = [this, sort_specs](const ProfilerString &textureNameA, const ProfilerString &textureNameB) -> bool {
    const TextureData &dataA = textures[textureNameA];
    const TextureData &dataB = textures[textureNameB];

    // Check each sort spec in order
    for (int i = 0; i < sort_specs->SpecsCount; i++)
    {
      const ImGuiTableColumnSortSpecs *currentSortSpec = &sort_specs->Specs[i];
      int comparisonResult = 0;

      // Compare based on column type
      switch (currentSortSpec->ColumnUserID)
      {
        case COL_NAME: comparisonResult = textureNameA.compare(textureNameB); break;
        case COL_FORMAT: comparisonResult = strcmp(getFormatName(dataA.info.cflg), getFormatName(dataB.info.cflg)); break;
        case COL_WIDTH: comparisonResult = dataA.info.w - dataB.info.w; break;
        case COL_HEIGHT: comparisonResult = dataA.info.h - dataB.info.h; break;
        case COL_MIPS: comparisonResult = dataA.info.mipLevels - dataB.info.mipLevels; break;
        case COL_MEM_SIZE:
        {
          float sizeDifference = getTextureMemorySize(dataA) - getTextureMemorySize(dataB);
          comparisonResult = (sizeDifference < 0) ? -1 : (sizeDifference > 0 ? 1 : 0);
          break;
        }
          // Texture Usage handled separately
      }

      // If values differ, apply sort direction
      if (comparisonResult != 0)
        return (currentSortSpec->SortDirection == ImGuiSortDirection_Ascending) ? (comparisonResult < 0) : (comparisonResult > 0);
    }

    // Equal
    return false;
  };

  eastl::sort(filteredTextures.begin(), filteredTextures.end(), compareFunction);
}

const char *TextureModule::getFormatName(uint32_t format) { return get_tex_format_name(format); }

eastl::vector<ProfilerString> TextureModule::getUniqueFormats()
{
  eastl::vector<ProfilerString> result;
  eastl::hash_set<ProfilerString> uniqueFormats;

  // Collect unique formats from this instance's data
  for (const auto &[textureName, textureData] : textures)
  {
    ProfilerString formatName = getFormatName(textureData.info.cflg);
    if (uniqueFormats.find(formatName) == uniqueFormats.end())
    {
      uniqueFormats.insert(formatName);
      result.push_back(formatName);
    }
  }

  return result;
}

eastl::vector<int> TextureModule::getUniqueWidths()
{
  eastl::vector<int> result;
  eastl::hash_set<int> uniqueWidths;

  // Collect unique widths from this instance's data
  for (const auto &[textureName, textureData] : textures)
  {
    int width = textureData.info.w;
    if (uniqueWidths.find(width) == uniqueWidths.end())
    {
      uniqueWidths.insert(width);
      result.push_back(width);
    }
  }

  // Sort in descending order (larger sizes first)
  eastl::sort(result.begin(), result.end(), eastl::greater<int>());

  // Special handling for power-of-2 and multiple of threshold
  eastl::vector<int> filtered;
  int otherCount = 0;

  for (int width : result)
  {
    // Check if width is power-of-2 or multiple of threshold
    bool isPowerOf2 = (width & (width - 1)) == 0;
    bool isMultipleOfThreshold = (width % TEXTURE_SIZE_THRESHOLD) == 0;

    if (isPowerOf2 || isMultipleOfThreshold)
      filtered.push_back(width);
    else
      ++otherCount;
  }

  // Add "Other" category if needed (-1 represents "Other")
  if (otherCount > 0)
    filtered.push_back(-1);

  return filtered;
}

eastl::vector<int> TextureModule::getUniqueHeights()
{
  eastl::vector<int> result;
  eastl::hash_set<int> uniqueHeights;

  // Collect unique heights from this instance's data
  for (const auto &[textureName, textureData] : textures)
  {
    int height = textureData.info.h;
    if (uniqueHeights.find(height) == uniqueHeights.end())
    {
      uniqueHeights.insert(height);
      result.push_back(height);
    }
  }

  // Sort in descending order (larger sizes first)
  eastl::sort(result.begin(), result.end(), eastl::greater<int>());

  // Special handling for power-of-2 and multiple of threshold
  eastl::vector<int> filtered;
  int otherCount = 0;

  for (int height : result)
  {
    // Check if height is power-of-2 or multiple of threshold
    bool isPowerOf2 = (height & (height - 1)) == 0;
    bool isMultipleOfThreshold = (height % TEXTURE_SIZE_THRESHOLD) == 0;

    if (isPowerOf2 || isMultipleOfThreshold)
      filtered.push_back(height);
    else
      ++otherCount;
  }

  // Add "Other" category if needed (-1 represents "Other")
  if (otherCount > 0)
    filtered.push_back(-1);

  return filtered;
}

} // namespace levelprofiler