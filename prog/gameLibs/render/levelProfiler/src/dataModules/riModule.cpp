// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texIdSet.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <gameRes/dag_collisionResource.h>
#include <math/dag_Point3.h>
#include <EASTL/algorithm.h>
#include <EASTL/hash_set.h>
#include <EASTL/array.h>
#include <math/dag_mathBase.h>
#include <riGen/riGenData.h>
#include <riGen/riGenExtra.h>
#include <generic/dag_enumerate.h>
#include "riModule.h"

namespace levelprofiler
{

auto &uvd = ::unitedvdata::riUnitedVdata;

static RIModule *g_current_ri_module_instance = nullptr;

RIModule::RIModule() {}

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
  riData.clear();
  riInstanceCounts.clear();
  maxUniqueTextureUsageCount = 0;
}

void RIModule::drawUI() {}

void RIModule::collect()
{
  clear();
  collectRenderableInstances();
  assets = getUniqueAssets();
  collectInstanceCounts();
  buildTextureToAssetMap();
  computeTextureUsageStatistics();
  collectRiDataForProfiling();
}

void RIModule::collectRenderableInstances()
{
  allRenderableInstances.clear();

  g_current_ri_module_instance = this;

  uvd.availableRElemsAccessor([](dag::Span<RenderableInstanceLodsResource *> resources) {
    for (auto renderableResource : resources)
    {
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
  result.reserve(allRenderableInstances.size());

  for (auto resource : allRenderableInstances)
  {
    AssetInfo assetInfoData;

    String resolvedResourceName;
    if (!resolve_game_resource_name(resolvedResourceName, resource))
      continue;

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
    eastl::hash_set<ProfilerString> uniqueAssetNames(assetNamesList.begin(), assetNamesList.end());

    int totalReferences = static_cast<int>(assetNamesList.size());
    int uniqueReferences = static_cast<int>(uniqueAssetNames.size());

    textureUsage[textureName] = TextureUsage(totalReferences, uniqueReferences);

    maxUniqueTextureUsageCount = eastl::max(maxUniqueTextureUsageCount, uniqueReferences);
  }
}

const RiData *RIModule::getRiDataByName(const ProfilerString &name) const
{
  for (const auto &riDataItem : riData)
  {
    if (riDataItem.name == name)
      return &riDataItem;
  }
  return nullptr;
}

void RIModule::collectInstanceCounts()
{
  riInstanceCounts.clear();
  maxAssetInstanceCount = 0;

  for (const auto &asset : assets)
    riInstanceCounts[asset.name] = 0;

  for (auto [poolIndex, pool] : enumerate(rendinst::riExtra))
  {
    if (!pool.res)
      continue;
    const char *riName = rendinst::riExtraMap.getName(poolIndex);
    if (!riName)
      continue;
    int instanceCount = static_cast<int>(pool.riTm.size());
    riInstanceCounts[ProfilerString(riName)] = instanceCount;

    if (instanceCount > maxAssetInstanceCount)
      maxAssetInstanceCount = instanceCount;
  }
}

void RIModule::collectRiDataForProfiling()
{
  riData.clear();
  riData.reserve(allRenderableInstances.size());

  for (auto resource : allRenderableInstances)
  {
    String resolvedResourceName;
    if (!resolve_game_resource_name(resolvedResourceName, resource))
      continue;

    RiData riDataItem(ProfilerString(resolvedResourceName.c_str()));
    riDataItem.resource = resource;

    auto instanceCountIter = riInstanceCounts.find(riDataItem.name);
    if (instanceCountIter != riInstanceCounts.end())
      riDataItem.countOnMap = instanceCountIter->second;

    Point3 bboxWidth = resource->bbox.width();
    float maxBoxEdge = eastl::max(eastl::max(bboxWidth.x, bboxWidth.y), bboxWidth.z) * 0.5f;
    riDataItem.bSphereRadius = resource->bsphRad;
    riDataItem.bBoxRadius = maxBoxEdge;

    const auto &lods = resource->lods;
    riDataItem.lods.reserve(lods.size());
    for (size_t lodIndex = 0; lodIndex < lods.size() && lodIndex < 4; ++lodIndex)
    {
      LodInfo lodInfo = analyzeLodData(lods[lodIndex], riDataItem.bSphereRadius, riDataItem.bBoxRadius);
      riDataItem.lods.push_back(lodInfo);
    }

    riDataItem.collision = CollisionInfo{};
    for (const auto &pool : rendinst::riExtra)
    {
      if (pool.res == resource)
      {
        riDataItem.collision = analyzeCollision(pool.collRes);
        break;
      }
    }

    riData.push_back(riDataItem);
  }
}

static constexpr size_t HEAVY_SHADERS_COUNT = 3;
static const eastl::array<const char *, HEAVY_SHADERS_COUNT> HEAVY_SHADERS = {
  "rendinst_perlin_layered", "rendinst_mask_layered", "rendinst_vcolor_layered"};

template <typename LodType>
LodInfo RIModule::analyzeLodData(const LodType &lod, float /* bsphere_radius */, float bbox_radius) const
{
  LodInfo lodInfo;

  lodInfo.lodDistance = lod.range;
  auto getUnderlyingMesh = [](const LodType &l) -> const auto * { return l.scene->getMesh()->getMesh()->getMesh(); };

  const auto *mesh = getUnderlyingMesh(lod);
  if (mesh)
  {
    lodInfo.totalFaces = mesh->calcTotalFaces();
    lodInfo.drawCalls = (int)mesh->getAllElems().size();
  }

  if (lodInfo.lodDistance > 0.0f)
  {
    constexpr float FOV_DEG = 90.0f;
    const float tg = tanf(FOV_DEG / 180.0f * PI * 0.5f);
    float sizeScale = 2.0f / (tg * lodInfo.lodDistance);
    float boxPartOfScreen = bbox_radius * sizeScale;
    lodInfo.screenPercent = boxPartOfScreen * 100.0f;
  }

  {
    eastl::vector<LodInfo::HeavyShaderEntry> heavyShadersList;
    const auto *mesh = getUnderlyingMesh(lod);
    if (mesh)
    {
      eastl::array<int, HEAVY_SHADERS_COUNT> counts = {0, 0, 0};
      for (const auto &elem : mesh->getAllElems())
      {
        const char *shaderName = elem.e->getShaderClassName();
        if (!shaderName)
          continue;
        for (size_t shaderIndex = 0; shaderIndex < HEAVY_SHADERS_COUNT; ++shaderIndex)
          if (strcmp(shaderName, HEAVY_SHADERS[shaderIndex]) == 0)
            counts[shaderIndex]++;
      }
      for (size_t shaderIndex = 0; shaderIndex < HEAVY_SHADERS_COUNT; ++shaderIndex)
        if (counts[shaderIndex] > 0)
          heavyShadersList.push_back({ProfilerString(HEAVY_SHADERS[shaderIndex]), counts[shaderIndex]});
    }
    lodInfo.heavyShaders = eastl::move(heavyShadersList);
  }
  return lodInfo;
}

CollisionInfo RIModule::analyzeCollision(const CollisionResource *collision_resource) const
{
  CollisionInfo info;
  if (!collision_resource)
    return info;

  dag::ConstSpan<CollisionNode> nodes = collision_resource->getAllNodes();
  for (const CollisionNode &cNode : nodes)
  {
    int tris = collision_resource->getNodeFaceCount(cNode.nodeIndex);
    if (cNode.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
      info.physTriangles += tris;
    if (cNode.checkBehaviorFlags(CollisionNode::TRACEABLE))
      info.traceTriangles += tris;
  }
  return info;
}

} // namespace levelprofiler