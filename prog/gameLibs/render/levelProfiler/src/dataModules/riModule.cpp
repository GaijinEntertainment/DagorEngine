// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texIdSet.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <EASTL/algorithm.h>
#include "riModule.h"

namespace levelprofiler
{

auto &uvd = ::unitedvdata::riUnitedVdata;

// Static pointer for callback access
static RIModule *g_current_ri_module_instance = nullptr;

RIModule::RIModule()
{
  // VOID
}

RIModule::~RIModule() { shutdownImpl(); }

void RIModule::init() { collect(); }

void RIModule::shutdown() { shutdownImpl(); }

void RIModule::shutdownImpl() { clearImpl(); }

void RIModule::clear() { clearImpl(); }

void RIModule::clearImpl()
{
  assets.clear();
  textureToAssetsMap.clear();
  textureUsage.clear();
  allRenderableInstances.clear();
  maxUniqueTextureUsageCount = 0;
}

void RIModule::drawUI()
{
  // UI handled by RIProfilerUI
}

void RIModule::collect()
{
  clear();
  collectRenderableInstances();
  assets = getUniqueAssets();
  buildTextureToAssetMap();
  computeTextureUsageStatistics();
}

void RIModule::collectRenderableInstances()
{
  allRenderableInstances.clear();

  // Set global pointer for callback access
  g_current_ri_module_instance = this;

  uvd.availableRElemsAccessor([](dag::Span<RenderableInstanceLodsResource *> resources) {
    for (auto renderableResource : resources)
    {
      // Only collect original instances (not duplicates/copies)
      if (renderableResource->getFirstOriginal() == renderableResource)
      {
        if (g_current_ri_module_instance)
          g_current_ri_module_instance->allRenderableInstances.push_back(renderableResource);
      }
    }
  });

  g_current_ri_module_instance = nullptr;
}

eastl::vector<AssetInfo> RIModule::getUniqueAssets()
{
  eastl::vector<AssetInfo> result;
  result.reserve(allRenderableInstances.size()); // Pre-allocate for efficiency

  for (auto resource : allRenderableInstances)
  {
    AssetInfo assetInfoData;

    String resolvedResourceName;
    if (!resolve_game_resource_name(resolvedResourceName, resource))
      continue; // If we can't resolve the resource name, skip this asset

    assetInfoData.name = ProfilerString(resolvedResourceName.c_str());

    TextureIdSet collectedTextureIds;
    resource->gatherUsedTex(collectedTextureIds);

    for (auto textureId : collectedTextureIds)
    {
      if (auto managedTextureName = get_managed_texture_name(textureId))
        assetInfoData.textureNames.push_back(ProfilerString(managedTextureName));
    }

    result.push_back(assetInfoData);
  }

  return result;
}

void RIModule::buildTextureToAssetMap()
{
  textureToAssetsMap.clear();

  for (auto &asset : assets)
  {
    for (auto &nameOfTexture : asset.textureNames)
      textureToAssetsMap[nameOfTexture].push_back(asset.name);
  }
}

void RIModule::computeTextureUsageStatistics()
{
  textureUsage.clear();
  maxUniqueTextureUsageCount = 0;

  for (auto &[textureName, assetNamesList] : textureToAssetsMap)
  {
    // Using a hash_set to efficiently count unique asset names.
    eastl::hash_set<ProfilerString> uniqueAssetNames(assetNamesList.begin(), assetNamesList.end());

    int totalReferences = static_cast<int>(assetNamesList.size());
    int uniqueReferences = static_cast<int>(uniqueAssetNames.size());

    textureUsage[textureName] = TextureUsage(totalReferences, uniqueReferences);

    maxUniqueTextureUsageCount = eastl::max(maxUniqueTextureUsageCount, uniqueReferences);
  }
}

} // namespace levelprofiler