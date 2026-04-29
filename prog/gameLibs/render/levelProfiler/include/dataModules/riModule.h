// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "levelProfilerInterface.h"

class CollisionResource;

extern bool resolve_game_resource_name(String &out_name, const RenderableInstanceLodsResource *res);

namespace levelprofiler
{

struct AssetInfo
{
  ProfilerString name;
  eastl::vector<ProfilerString> textureNames;
};

struct LodInfo
{
  int drawCalls = 0;
  int totalFaces = 0;
  float lodDistance = 0.0f;
  float screenPercent = 0.0f;
  struct HeavyShaderEntry
  {
    ProfilerString name;
    int count = 1;
  };
  eastl::vector<HeavyShaderEntry> heavyShaders;
};

struct CollisionInfo
{
  int physTriangles = 0;
  int traceTriangles = 0;
};

struct RiData
{
  ProfilerString name;
  int countOnMap = 0;
  float bSphereRadius = 0.0f;
  float bBoxRadius = 0.0f;
  eastl::vector<LodInfo> lods;
  CollisionInfo collision;
  const RenderableInstanceLodsResource *resource = nullptr;

  RiData() = default;
  RiData(const ProfilerString &asset_name) : name(asset_name) {}
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

  const eastl::vector<RiData> &getRiData() const { return riData; }
  const RiData *getRiDataByName(const ProfilerString &name) const;
  const eastl::hash_map<ProfilerString, int> &getRiInstanceCounts() const { return riInstanceCounts; }

  int getMaxUniqueTextureUsageCount() const { return maxUniqueTextureUsageCount; }
  int getMaxAssetInstanceCount() const { return maxAssetInstanceCount; }

  void buildTextureToAssetMap();
  void computeTextureUsageStatistics();
  void collectRiDataForProfiling();

  eastl::vector<const RenderableInstanceLodsResource *> allRenderableInstances;

protected:
  void shutdownImpl();
  void clearImpl();

private:
  eastl::vector<AssetInfo> assets;
  eastl::hash_map<ProfilerString, eastl::vector<ProfilerString>> textureToAssetsMap;
  eastl::hash_map<ProfilerString, TextureUsage> textureUsage;

  eastl::vector<RiData> riData;
  eastl::hash_map<ProfilerString, int> riInstanceCounts;

  int maxUniqueTextureUsageCount = 0;
  int maxAssetInstanceCount = 0;

  void collectRenderableInstances();
  eastl::vector<AssetInfo> getUniqueAssets();

  void collectInstanceCounts();
  template <typename LodType>
  LodInfo analyzeLodData(const LodType &lod, float bsphere_radius, float bbox_radius) const;
  CollisionInfo analyzeCollision(const CollisionResource *collision_resource) const;
};

} // namespace levelprofiler