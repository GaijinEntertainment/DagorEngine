// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "levelProfilerInterface.h"

extern bool resolve_game_resource_name(String &out_name, const RenderableInstanceLodsResource *res);

namespace levelprofiler
{

struct AssetInfo
{
  ProfilerString name;
  eastl::vector<ProfilerString> textureNames;
};

// RI module - collects and manages LOD asset data
class RIModule : public IDataCollector, public IProfilerModule
{
public:
  RIModule();
  virtual ~RIModule();

  void collect() override;
  void clear() override;

  void init() override;
  void shutdown() override;
  void drawUI() override;

  const eastl::vector<AssetInfo> &getAssets() const { return assets; }
  const eastl::hash_map<ProfilerString, eastl::vector<ProfilerString>> &getTextureToAssetsMap() const { return textureToAssetsMap; }
  const eastl::hash_map<ProfilerString, TextureUsage> &getTextureUsage() const { return textureUsage; }

  int getMaxUniqueTextureUsageCount() const { return maxUniqueTextureUsageCount; }

  void buildTextureToAssetMap();
  void computeTextureUsageStatistics();

  eastl::vector<const RenderableInstanceLodsResource *> allRenderableInstances;

protected:
  void shutdownImpl();
  void clearImpl();

private:
  eastl::vector<AssetInfo> assets;
  eastl::hash_map<ProfilerString, eastl::vector<ProfilerString>> textureToAssetsMap;
  eastl::hash_map<ProfilerString, TextureUsage> textureUsage;

  int maxUniqueTextureUsageCount = 0; // Maximum unique texture references, used for UI scaling

  void collectRenderableInstances();
  eastl::vector<AssetInfo> getUniqueAssets();
};

} // namespace levelprofiler