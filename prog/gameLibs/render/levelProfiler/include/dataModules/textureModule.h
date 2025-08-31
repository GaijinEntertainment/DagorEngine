// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>
#include <drv/3d/dag_rwResource.h>
#include "levelProfilerInterface.h"

namespace levelprofiler
{

struct TextureData
{
  TextureInfo info;
  uint64_t sizeBytes = 0;

  TextureData() = default;
  TextureData(const TextureInfo &texInfo, uint64_t size) : info(texInfo), sizeBytes(size) {}
};

class TextureModule : public IDataCollector, public IProfilerModule
{
public:
  TextureModule();
  virtual ~TextureModule();
  void collect() override;
  void clear() override;
  void init() override;
  void shutdown() override;
  void drawUI() override;

  const eastl::hash_map<ProfilerString, TextureData> &getTextures() const;
  const eastl::vector<ProfilerString> &getFilteredTextures() const;
  void initializeFilteredTextures();
  float getTextureMemorySize(const TextureData &texture_data) const;
  void selectTexture(const char *texture_name);
  void drawTextureView(const char *texture_name);
  void rebuildFilteredList();

  int getFilteredTextureCount() const;
  int getTotalTextureCount() const;
  float getFilteredTotalMemorySize() const;
  float getTotalMemorySize() const;

  void sortFilteredTextures(ImGuiTableSortSpecs *sort_specs);

  int getMipMinDefault() const { return mipMinDefault; }
  int getMipMaxDefault() const { return mipMaxDefault; }
  float getMemorySizeMinDefault() const { return sizeMinDefault; }
  float getMemorySizeMaxDefault() const { return sizeMaxDefault; }

  void startDataCollection();
  void updateDataCollection();
  bool isCollecting() const { return isCollectingData; }
  float getCollectionProgress() const;
  const ProfilerString &getCurrentCollectionTexture() const;

  static const char *getFormatName(uint32_t format);
  eastl::vector<ProfilerString> getUniqueFormats();
  eastl::vector<int> getUniqueWidths();
  eastl::vector<int> getUniqueHeights();

protected:
  void shutdownImpl();
  void clearImpl();

private:
  eastl::hash_map<ProfilerString, TextureData> textures; // All textures data
  eastl::vector<ProfilerString> filteredTextures;
  ProfilerString selectedTextureName;

  int selectedMipLevel = 0;
  bool channelMask[4] = {true, true, true, true};                      // RGBA channel toggles
  d3d::SamplerHandle pointSamplerHandle = d3d::INVALID_SAMPLER_HANDLE; // For texture preview
  uint32_t totalCount = 0, filteredCount = 0;                          // Texture counts
  float totalMemorySize = 0, filteredMemorySize = 0;                   // Texture Memory usage

  int mipMinDefault = 0, mipMaxDefault = 0;
  float sizeMinDefault = 0.f, sizeMaxDefault = 0.f;

  bool isCollectingData = false;
  eastl::vector<ProfilerString> texturesToCollect;
  size_t currentCollectionIndex = 0;
  int collectionFrameCounter = 0;

  void recalculateStatistics();
  void setImagePreviewMode(const ImDrawList *draw_list, const ImDrawCmd *command);
  void resetImagePreviewMode(const ImDrawList *draw_list, const ImDrawCmd *command);

  int imguiChannelMaskVariableId = -1;
  int imguiMipLevelVariableId = -1;
  int imguiUseCustomSamplerVariableId = -1;
};

} // namespace levelprofiler