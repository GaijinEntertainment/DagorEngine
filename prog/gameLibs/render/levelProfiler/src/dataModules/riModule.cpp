// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texIdSet.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <gameRes/dag_collisionResource.h>
#include <math/dag_Point3.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/hash_set.h>
#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <math/dag_mathBase.h>
#include <riGen/riGenData.h>
#include <riGen/riGenExtra.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstGenRtTools.h>
#include <rendInst/ccExtra.h>
#include <generic/dag_enumerate.h>
#include "riModule.h"

namespace rendinst
{
// Defined in rendInstGenGlobals.cpp, declared in no rendInst header. It selects what precomputeCell
// does with riExtra-substituted pools: on, it creates their pregen instances; off, it counts them.
extern bool persistentRiExtraInstances;
} // namespace rendinst

namespace levelprofiler
{

auto &uvd = ::unitedvdata::riUnitedVdata;

static RIModule *g_current_ri_module_instance = nullptr;

RIModule::RIModule() {}

RIModule::~RIModule() { shutdownImpl(); }

void RIModule::shutdown() { shutdownImpl(); }

void RIModule::shutdownImpl() { clearImpl(); }

void RIModule::clear() { clearImpl(); }

void RIModule::clearImpl()
{
  resetCollectionState();
  assets.clear();
  textureToAssetsMap.clear();
  textureUsage.clear();
  allRenderableInstances.clear();
  riData.clear();
  riDataLookup.clear();
  riInstanceCounts.clear();
  maxUniqueTextureUsageCount = 0;
  maxAssetInstanceCount = 0;
  riDataProvisional = false;
  collectCancelled = false;
  riDataGeneration++;
}

void RIModule::drawUI() {}

// A riGen load fills the layer tables with no lock, so a collect or walk then would read them half-built.
bool RIModule::canCollect() const { return !RendInstGenData::isLoading; }

void RIModule::collect()
{
  if (collectPhase != CollectPhase::Idle)
    return;

  clear();
  collectRenderableInstances();
  assets = getUniqueAssets();
  CollisionByRes collisionByRes;
  scanRiExtraPools(collisionByRes);
  addRiGenCollision(collisionByRes);
  collectRiDataForProfiling(collisionByRes);
  collectRiGenInstanceCounts();
  buildTextureToAssetMap();
  computeTextureUsageStatistics();
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

void RIModule::addRiGenCollision(CollisionByRes &collision_by_res)
{
  for (const RendInstGenData *rgl : rendinst::rgLayer)
  {
    if (!rgl || !rgl->rtData)
      continue;
    const RendInstGenData::RtData &rtData = *rgl->rtData;
    for (int i = 0; i < rtData.riRes.size(); ++i)
      if (rtData.riCollRes[i].collRes)
        collision_by_res.emplace(rtData.riRes[i], rtData.riCollRes[i].collRes);
  }
}

void RIModule::scanRiExtraPools(CollisionByRes &collision_by_res)
{
  for (const auto &asset : assets)
    riInstanceCounts[asset.name] = 0;

  rendinst::ScopedRIExtraReadLock riExtraLock; // another thread can add pools, and an add can reallocate the pool vector
  rendinst::iterateRIExtra([&](int poolIndex, const rendinst::RiExtraPool &pool) {
    if (pool.collRes)
      collision_by_res.emplace(pool.res, pool.collRes);
    const char *riName = pool.res ? rendinst::riExtraMap.getName(poolIndex) : nullptr;
    if (!riName)
      return;
    int &storedCount = riInstanceCounts[ProfilerString(riName)];
    storedCount += pool.getEntitiesCount();
    maxAssetInstanceCount = eastl::max(maxAssetInstanceCount, storedCount);
  });
}

int RIModule::countRiGenCell(LayerCollectState &state, RendInstGenData *layer, int cell_x, int cell_y)
{
  RendInstGenData::RtData *rtData = layer->rtData;
  if (!rtData)
    return 0;

  RendInstGenData::CellRtData *crt = new RendInstGenData::CellRtData(rtData->riRes.size(), rtData);

  // With mask-generated off precomputeCell skips land class placement, so force it on to count those RI.
  const bool prevMaskEnabled = RendInstGenData::maskGeneratedEnabled;
  if (!prevMaskEnabled)
    rendinst::enable_rigen_mask_generated(true);

  // The engine calls precomputeCell once per cell and keeps the result; this walk repeats it on
  // every collect, so with persistent riExtra on it would add a duplicate of the level to the
  // world every run. The instances it would create are already counted by the riExtra pass.
  const bool prevPersistentRiExtra = rendinst::persistentRiExtraInstances;
  rendinst::persistentRiExtraInstances = false;

  layer->precomputeCell(*crt, cell_x, cell_y);

  rendinst::persistentRiExtraInstances = prevPersistentRiExtra;
  if (!prevMaskEnabled)
    rendinst::enable_rigen_mask_generated(false);

  int cellInstances = 0;
  const int poolCount = eastl::min(static_cast<int>(crt->pools.size()), static_cast<int>(state.poolSlots.size()));
  for (int poolIndex = 0; poolIndex < poolCount; ++poolIndex)
  {
    const auto &poolData = crt->pools[poolIndex];
    if (poolData.avail < 0 || poolData.total <= 0) // avail < 0: substituted to riExtra, counted there
      continue;

    cellInstances += poolData.total;

    const LayerCollectState::PoolCountSlot &slot = state.poolSlots[poolIndex];
    if (!slot.count)
      continue;

    *slot.count += poolData.total;
    if (*slot.count > maxAssetInstanceCount)
      maxAssetInstanceCount = *slot.count;
    if (slot.data)
    {
      slot.data->countOnMap = *slot.count;
      riCountsGeneration++;
    }
  }

  delete crt;
  return cellInstances;
}

void RIModule::collectRiGenInstanceCounts() { prepareLayerCollection(); }

void RIModule::prepareLayerCollection()
{
  layerCollectStates.clear();
  totalCellsToProcess = 0;
  processedCells = 0;
  currentLayerIndex = 0;

  for (int layerIndex = 0; layerIndex < rendinst::rgLayer.size(); layerIndex++)
  {
    RendInstGenData *rgl = rendinst::getRgLayer(layerIndex);
    if (!rgl || !rgl->rtData || rgl->cellNumW <= 0 || rgl->cellNumH <= 0)
      continue;

    LayerCollectState state;
    state.layerIndex = layerIndex;
    state.expectedLayer = rgl;
    state.expectedRtData = rgl->rtData;
    state.cellCountW = rgl->cellNumW;
    state.cellCountH = rgl->cellNumH;

    RendInstGenData::RtData *rtData = rgl->rtData;
    state.poolSlots.resize(rtData->riResName.size());
    for (int i = 0; i < static_cast<int>(rtData->riResName.size()); ++i)
    {
      const char *riName = rtData->riResName[i];
      if (!riName)
        continue;
      ProfilerString assetName(riName);
      state.poolSlots[i].count = &riInstanceCounts[assetName];
      auto it = riDataLookup.find(assetName);
      state.poolSlots[i].data = it != riDataLookup.end() ? it->second : nullptr;
    }

    layerCollectStates.push_back(eastl::move(state));

    totalCellsToProcess += static_cast<size_t>(rgl->cellNumW) * static_cast<size_t>(rgl->cellNumH);
  }

  if (totalCellsToProcess == 0)
  {
    collectPhase = CollectPhase::Finalizing;
    finalizeCollection();
  }
  else
  {
    collectPhase = CollectPhase::Collecting;
  }
}

RendInstGenData *RIModule::resolveCollectLayer(const LayerCollectState &state) const
{
  RendInstGenData *rgl = rendinst::getRgLayer(state.layerIndex);
  if (!rgl || rgl != state.expectedLayer || rgl->rtData != state.expectedRtData)
    return nullptr;

  // Unload plus load can hand the allocator back both addresses, so the grid is compared as
  // well: walking a new level with the old geometry indexes outside its cell array. Same
  // geometry and recycled addresses still pass, which needs a load counter rendInst has not.
  if (rgl->cellNumW != state.cellCountW || rgl->cellNumH != state.cellCountH)
    return nullptr;

  return rgl;
}

bool RIModule::processLayerCell(LayerCollectState &state, RendInstGenData *layer, int &out_instances)
{
  out_instances = 0;

  const int totalCells = state.cellCountW * state.cellCountH;
  if (state.nextCellIndex >= totalCells)
    return false;

  const int cellIndex = state.nextCellIndex;
  const int cellX = cellIndex % state.cellCountW;
  const int cellY = cellIndex / state.cellCountW;
  state.nextCellIndex++;

  out_instances = countRiGenCell(state, layer, cellX, cellY);

  processedCells++;
  return true;
}

void RIModule::finalizeCollection()
{
  if (collectPhase == CollectPhase::Idle)
    return;

  // Counts are published live during the walk, so finalizing only drops the provisional flag.
  riDataProvisional = false;
  riDataGeneration++;

  layerCollectStates.clear();
  currentLayerIndex = 0;
  totalCellsToProcess = 0;
  processedCells = 0;
  collectPhase = CollectPhase::Idle;
}

void RIModule::cancelCollection()
{
  layerCollectStates.clear();
  currentLayerIndex = 0;
  totalCellsToProcess = 0;
  processedCells = 0;
  collectPhase = CollectPhase::Idle;

  // The walk cannot be resumed against a level that is gone, so the partial counts stay flagged.
  riDataProvisional = true;
  collectCancelled = true;
  riDataGeneration++;
}

void RIModule::resetCollectionState()
{
  layerCollectStates.clear();
  currentLayerIndex = 0;
  totalCellsToProcess = 0;
  processedCells = 0;
  collectPhase = CollectPhase::Idle;

  riDataLookup.clear();
  riDataProvisional = false;
}

// A dense riGen cell holds many instances, so cell count alone does not bound per-frame work.
// The budget is checked before a cell, so one cell always runs and a huge cell cannot stall the walk.
static constexpr int RIGEN_INSTANCE_BUDGET_PER_FRAME = 30000;
static constexpr int CELL_BUDGET_PER_FRAME = 8;

void RIModule::continueCollect()
{
  if ((collectPhase == CollectPhase::Collecting || collectPhase == CollectPhase::Paused) && !canCollect())
  {
    cancelCollection();
    return;
  }

  if (collectPhase == CollectPhase::Paused)
  {
    // Nothing advances while paused, but a level unload still has to be noticed here: the UI
    // would otherwise keep reporting a paused walk over layers that no longer exist.
    if (currentLayerIndex < static_cast<int>(layerCollectStates.size()) && !resolveCollectLayer(layerCollectStates[currentLayerIndex]))
      cancelCollection();
    return;
  }

  if (collectPhase != CollectPhase::Collecting && collectPhase != CollectPhase::Finalizing)
    return;

  // countRiGenCell forces two process-global riGen flags around precomputeCell, and the riGen
  // streaming job generates cells under those same flags. isRIGenPrepareFinished only reports
  // that the job manager was idle when it ran, and the act thread queues cell jobs every frame,
  // so it is checked per cell rather than once per slice. A job queued while a cell is being
  // counted can still overlap it; closing that needs a way to serialize with cell generation.
  if (collectPhase == CollectPhase::Collecting)
  {
    int remainingCells = CELL_BUDGET_PER_FRAME;
    int remainingInstances = RIGEN_INSTANCE_BUDGET_PER_FRAME;
    while (remainingCells > 0 && remainingInstances > 0 && currentLayerIndex < static_cast<int>(layerCollectStates.size()) &&
           rendinst::isRIGenPrepareFinished())
    {
      LayerCollectState &state = layerCollectStates[currentLayerIndex];

      RendInstGenData *layer = resolveCollectLayer(state);
      if (!layer)
      {
        // The riGen layers were dropped or rebuilt between two pumps (clearRIGen), so the walk is void.
        cancelCollection();
        return;
      }

      int cellInstances = 0;
      if (!processLayerCell(state, layer, cellInstances))
      {
        currentLayerIndex++;
        continue;
      }

      remainingCells--;
      remainingInstances -= cellInstances;
    }

    if (currentLayerIndex >= static_cast<int>(layerCollectStates.size()))
      collectPhase = CollectPhase::Finalizing;
  }

  if (collectPhase == CollectPhase::Finalizing)
    finalizeCollection();
}

bool RIModule::isCollecting() const { return collectPhase == CollectPhase::Collecting || collectPhase == CollectPhase::Finalizing; }

float RIModule::getCollectProgress() const
{
  if (totalCellsToProcess == 0)
    return 0.0f;

  return static_cast<float>(processedCells) / static_cast<float>(totalCellsToProcess);
}

void RIModule::pauseCollection()
{
  if (collectPhase == CollectPhase::Idle || collectPhase == CollectPhase::Paused)
    return;

  if (collectPhase == CollectPhase::Finalizing)
  {
    finalizeCollection();
    return;
  }

  collectPhase = CollectPhase::Paused;
}

void RIModule::resumeCollection()
{
  if (collectPhase != CollectPhase::Paused)
    return;

  if (currentLayerIndex >= static_cast<int>(layerCollectStates.size()))
  {
    collectPhase = CollectPhase::Finalizing;
    finalizeCollection();
    return;
  }

  collectPhase = CollectPhase::Collecting;
}

void RIModule::collectRiDataForProfiling(const CollisionByRes &collision_by_res)
{
  riData.clear();
  riDataLookup.clear();
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
    riDataItem.lodCount = static_cast<int>(lods.size());
    const int collectedLodCount = eastl::min<int>(lods.size(), rendinst::RiExtraPool::MAX_LODS);
    riDataItem.lods.reserve(collectedLodCount);
    for (int lodIndex = 0; lodIndex < collectedLodCount; ++lodIndex)
    {
      LodInfo lodInfo = analyzeLodData(lods[lodIndex], riDataItem.bSphereRadius, riDataItem.bBoxRadius);
      riDataItem.lods.push_back(lodInfo);
    }

    auto collisionIter = collision_by_res.find(resource);
    riDataItem.collision = collisionIter != collision_by_res.end() ? analyzeCollision(collisionIter->second) : CollisionInfo{};

    riData.push_back(eastl::move(riDataItem));
    RiData *storedPtr = &riData.back();
    riDataLookup[storedPtr->name] = storedPtr;
  }
  riDataProvisional = true;
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
    lodInfo.maxEdgePerDistance = 2.0f * bbox_radius / lodInfo.lodDistance;

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
      eastl::sort(heavyShadersList.begin(), heavyShadersList.end(),
        [](const LodInfo::HeavyShaderEntry &a, const LodInfo::HeavyShaderEntry &b) { return a.name < b.name; });
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