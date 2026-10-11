// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/shared_ptr.h>
#include <EASTL/weak_ptr.h>
#include <generic/dag_enumerate.h>
#include <generic/dag_span.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_lockSbuffer.h>
#include <render/daFrameGraph/daFG.h>
#include <render/world/bvh.h>
#include <math/dag_hlsl_floatx.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <shaders/dag_refinedBlock.h>
#include <frustumCulling/frustumPlanes.h>
#include <util/dag_convar.h>
#include <perfMon/dag_statDrv.h>
#include "../CSMShadows.h"
#include "../globalManager.h"
#include "../../shaders/dagdp_common.hlsli"
#include "../../shaders/dagdp_common_placer.hlsli"
#include "../../shaders/dagdp_volume.hlsli"
#include "../../shaders/dagdp_heightmap.hlsli"
#include "../block.h"
#include "volume.h"
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/ccExtra.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_cpuJobs.h>

CONSOLE_BOOL_VAL("dagdp", volume_early_riex_gather, true);
CONSOLE_BOOL_VAL("dagdp", volume_early_grid_gather, true);

namespace var
{
static ShaderVariableInfo draw_ranges("dagdp_volume__draw_ranges", true);
static ShaderVariableInfo placeables("dagdp_volume__placeables", true);
static ShaderVariableInfo placeable_weights("dagdp_volume__placeable_weights", true);
static ShaderVariableInfo renderable_indices("dagdp_volume__renderable_indices", true);
static ShaderVariableInfo variants("dagdp_volume__variants", true);
static ShaderVariableInfo volume_variants("dagdp_volume__volume_variants", true);

static ShaderVariableInfo debug_frustum_culling_bias("dagdp_volume__debug_frustum_culling_bias", true);
static ShaderVariableInfo max_placeable_bounding_radius("dagdp_volume__max_placeable_bounding_radius", true);
static ShaderVariableInfo num_renderables("dagdp_volume__num_renderables", true);
static ShaderVariableInfo num_placeables("dagdp_volume__num_placeables", true);
static ShaderVariableInfo tile_start_index("dagdp_volume__tile_start_index", true);

static ShaderVariableInfo prng_seed_placeable("dagdp_volume__prng_seed_placeable", true);
static ShaderVariableInfo prng_seed_slope("dagdp_volume__prng_seed_slope", true);
static ShaderVariableInfo prng_seed_occlusion("dagdp_volume__prng_seed_occlusion", true);
static ShaderVariableInfo prng_seed_scale("dagdp_volume__prng_seed_scale", true);
static ShaderVariableInfo prng_seed_yaw("dagdp_volume__prng_seed_yaw", true);
static ShaderVariableInfo prng_seed_pitch("dagdp_volume__prng_seed_pitch", true);
static ShaderVariableInfo prng_seed_roll("dagdp_volume__prng_seed_roll", true);
static ShaderVariableInfo prng_seed_triangle1("dagdp_volume__prng_seed_triangle1", true);
static ShaderVariableInfo prng_seed_triangle2("dagdp_volume__prng_seed_triangle2", true);
static ShaderVariableInfo prng_seed_jitter_x("dagdp_volume__prng_seed_jitter_x", true);
static ShaderVariableInfo prng_seed_jitter_z("dagdp_volume__prng_seed_jitter_z", true);

static ShaderVariableInfo viewport_pos("dagdp_volume__viewport_pos", true);
static ShaderVariableInfo viewport_max_distance("dagdp_volume__viewport_max_distance", true);
static ShaderVariableInfo viewport_index("dagdp_volume__viewport_index", true);

static ShaderVariableInfo dispatch_index("dagdp_volume__dispatch_index", true);

static ShaderVariableInfo mesh_params("dagdp_volume__mesh_params", true);
static ShaderVariableInfo areas_start_offset("dagdp_volume__areas_start_offset", true);
static ShaderVariableInfo areas_bottom_offset("dagdp_volume__areas_bottom_offset", true);
static ShaderVariableInfo areas_top_offset("dagdp_volume__areas_top_offset", true);
static ShaderVariableInfo areas_count("dagdp_volume__areas_count", true);
static ShaderVariableInfo areas("dagdp_volume__areas", true);

static ShaderVariableInfo volumes("dagdp_volume__volumes", true);
static ShaderVariableInfo meshes("dagdp_volume__meshes", true);
static ShaderVariableInfo dispatches("dagdp_volume__dispatches", true);
static ShaderVariableInfo tiles("dagdp_volume__tiles", true);
} // namespace var

using TmpName = eastl::fixed_string<char, 128>;

namespace dagdp
{

bool is_volume_early_riex_gather_enabled() { return volume_early_riex_gather.get(); }
bool is_volume_early_grid_gather_enabled() { return volume_early_grid_gather.get(); }

struct VolumePersistentData;
static dag::Vector<eastl::weak_ptr<VolumePersistentData>> g_volume_views;

static constexpr uint32_t ESTIMATED_PREFIX_SUM_LEVELS = 4;
static inline uint32_t divUp(uint32_t size, uint32_t stride) { return (size + stride - 1) / stride; }

struct VolumeConstants
{
  ViewInfo viewInfo;
  eastl::fixed_string<char, 32> viewResourceName;
  float maxPlaceableBoundingRadius;
  uint32_t numRenderables;
  uint32_t numPlaceables;
  uint32_t prngSeed;
  uint32_t totalCounters;
  uint32_t maxObjects;
  uint32_t maxTriangles;
  uint32_t maxMeshes;
  uint32_t maxDispatches;
  uint32_t maxTiles;
  uint32_t maxVolumes;
  int targetMeshLod;
  VolumeMapping mapping;
  refined_block::PassBlockHandle passBlock;
};

// Note: not safe if FG starts using MT execution.
struct VolumeMutables
{
  uint32_t areasUsed = 0;
  RiexProcessor riexProcessor;
  DagdpRiexGatherJob riexGatherJob;
};

struct VolumePersistentData
{
  CommonPlacerBuffers commonBuffers;

  UniqueBuf allocsBuffer;
  UniqueBuf areasBuffer;
  UniqueBuf volumeVariantsBuffer;
  UniqueBuf volumesBuffer;
  UniqueBuf meshesBuffer;
  UniqueBuf dispatchesBuffer;
  UniqueBuf tilesBuffer;

  VolumeConstants constants{};
  VolumeMutables mutables{};
};

struct GatheredInfo
{
  struct PerViewport
  {
    uint32_t tileStartIndex;
    uint32_t tileEndIndex;
    uint32_t dispatchStartIndex;
    uint32_t dispatchEndIndex;
    uint32_t meshEndIndex;
    uint32_t volumeEndIndex;
  };

  dag::RelocatableFixedVector<PerViewport, DAGDP_MAX_VIEWPORTS> perViewport;
  // Dispatch mesh ranges are consecutive, so N dispatches are stored as N+1 boundary
  // offsets: dispatch i covers meshes [dispatches[i], dispatches[i+1]).
  dag::RelocatableFixedVector<uint32_t, MAX_VB_INDICES> dispatches;
  static uint32_t numDispatchBoundaries(uint32_t num_dispatches) { return num_dispatches + 1; }
  uint32_t numDispatches() const { return dispatches.size() - 1; }
  void truncateDispatches(uint32_t num) { dispatches.resize(numDispatchBoundaries(num)); }
  RelevantMeshes relevantMeshes;
  RelevantTiles relevantTiles;
  RelevantVolumes relevantVolumes;
  bool isValid = false;

  dag::RelocatableFixedVector<MeshToProcess, ESTIMATED_PROCESSED_MESHES_PER_FRAME> meshesToProcess;
};

template <typename T>
using PerLevel = dag::RelocatableFixedVector<T, ESTIMATED_PREFIX_SUM_LEVELS, true, framemem_allocator>;

template <typename T>
using PerProcessedMesh = dag::RelocatableFixedVector<T, ESTIMATED_PROCESSED_MESHES_PER_FRAME, true, framemem_allocator>;

void DagdpRiexGatherJob::reset()
{
  entryCount = 0;
  localVolumeCount = 0;
  volumeCount = 0;
  worldVolumeCount = 0;
  meshPool.clear();
  riGenSourcePool.clear();
  for (size_t i = 0; i < viewportCount; ++i)
  {
    viewportDataPool[i].onRi.clear();
    viewportDataPool[i].aroundRi.clear();
    viewportDataPool[i].valid = false;
  }
  viewportCount = 0;
}

DagdpRiexGatherJob::PerViewportData &DagdpRiexGatherJob::addViewport()
{
  if (viewportCount >= viewportDataPool.size())
    viewportDataPool.emplace_back();
  auto &v = viewportDataPool[viewportCount++];
  v.onRi.clear();
  v.aroundRi.clear();
  v.valid = false;
  return v;
}

size_t DagdpRiexGatherJob::addEntry(int res_idx, bbox3f_cref box)
{
  if (entryCount >= entryPool.size())
    entryPool.emplace_back();
  auto &e = entryPool[entryCount];
  e.resIdx = res_idx;
  e.box = box;
  return entryCount++;
}

void DagdpRiexGatherJob::addEntries(PlacerData &rec, const ecs::IntList &resource_ids, bbox3f_cref box)
{
  rec.firstEntry = entryCount;
  for (auto resIdx : resource_ids)
    addEntry(resIdx, box);
  rec.entryCount = entryCount - rec.firstEntry;
}

DagdpRiexGatherJob::LocalVolumeSnapshot &DagdpRiexGatherJob::addLocalVolume()
{
  if (localVolumeCount >= localVolumePool.size())
    localVolumePool.emplace_back();
  return localVolumePool[localVolumeCount++];
}

DagdpRiexGatherJob::VolumeEntry &DagdpRiexGatherJob::addVolumeEntry(const VolumeEntry &e)
{
  if (volumeCount >= volumePool.size())
    volumePool.emplace_back();
  auto &slot = volumePool[volumeCount++];
  slot = e;
  return slot;
}

void init_volume_entry(DagdpRiexGatherJob::VolumeEntry &e,
  const VolumeMappingItem *variant,
  const TMatrix &tm,
  mat44f_cref tm44,
  float scale,
  int volume_type,
  float max_bounding_radius)
{
  e.variant = variant;
  e.tm = tm;
  e.scale = scale;
  e.volumeType = volume_type;
  e.box = volume_local_box(tm44, scale);
  e.center = tm44.col3;
  e.extent2 = volume_cull_extent2(tm44, scale, max_bounding_radius);
}

void prepare_volume_entry(DagdpRiexGatherJob::VolumeEntry &e, mat44f_cref tm44)
{
  // Reference: volumePlacerES.cpp.inl, VolumePlacer::updateVisibility
  mat44f itm44;
  v_mat44_inverse43(itm44, tm44);

  mat43f itm43;
  v_mat44_transpose_to_mat43(itm43, itm44);

  G_ASSERT(e.scale > FLT_EPSILON);
  const vec4f invScale = v_splats(1.0f / e.scale);
  e.itmRow0 = v_mul(itm43.row0, invScale);
  e.itmRow1 = v_mul(itm43.row1, invScale);
  e.itmRow2 = v_mul(itm43.row2, invScale);

  Point3 axis = e.variant->axis;
  if (e.variant->axisLocal)
    axis = e.tm * axis;
  e.axis = normalize(axis);

  const float instancePosDelta = 1.0f / sqrtf(e.variant->density);
  const float tileWorldSize = TILE_INSTANCE_COUNT_1D * instancePosDelta;
  e.instancePosDelta = instancePosDelta;
  e.tileMin.x = static_cast<int>(floorf(v_extract_x(e.box.bmin) / tileWorldSize));
  e.tileMin.y = static_cast<int>(floorf(v_extract_z(e.box.bmin) / tileWorldSize));
  e.tileMax.x = static_cast<int>(ceilf(v_extract_x(e.box.bmax) / tileWorldSize));
  e.tileMax.y = static_cast<int>(ceilf(v_extract_z(e.box.bmax) / tileWorldSize));
}

void DagdpRiexGatherJob::gatherVolume(VolumeEntry &e)
{
  e.firstMesh = meshPool.size();
  e.firstUnprocessedRes = -1;
  if (meshesEnabled)
  {
    handleScratch.clear();
    rendinst::gatherRIGenExtraCollidableMin(handleScratch, e.box, MIN_GEOMETRY_SIZE);
    e.firstUnprocessedRes = filter_volume_meshes(make_span_const(handleScratch), resolve_mesh_lod(*e.variant, targetMeshLod), &e.box,
      *riexProcessor, meshPool);
  }
  e.meshCount = meshPool.size() - e.firstMesh;
}

// The record's instances concatenated, so the node emits them as one volume outside any loop.
void DagdpRiexGatherJob::filterRecord(PlacerData &rec)
{
  handleScratch.clear();
  for (size_t i = 0; i < rec.entryCount; ++i)
  {
    const auto &h = entryPool[rec.firstEntry + i].handles;
    handleScratch.insert(handleScratch.end(), h.begin(), h.end());
  }
  rec.handleCount = handleScratch.size();

  rec.firstMesh = meshPool.size();
  rec.firstUnprocessedRes = -1;
  if (meshesEnabled)
    rec.firstUnprocessedRes = filter_volume_meshes(make_span_const(handleScratch), rec.meshLod, nullptr, *riexProcessor, meshPool);
  rec.meshCount = meshPool.size() - rec.firstMesh;
}

void DagdpRiexGatherJob::initProfileMarkers(const char *view_name)
{
  TmpName name(TmpName::CtorSprintf(), "dagdp_volume_riex_gather@%s", view_name);
  jobNameProfDesc = DA_PROFILE_ADD_LOCAL_DESCRIPTION(0, name.c_str());
  name.sprintf("dagdp_volume_gather_start@%s", view_name);
  startDesc = DA_PROFILE_ADD_LOCAL_DESCRIPTION(0, name.c_str());
  name.sprintf("wait_dagdp_volume_riex_gather@%s", view_name);
  waitDesc = DA_PROFILE_ADD_LOCAL_DESCRIPTION(0, name.c_str());
}

void DagdpRiexGatherJob::start(GatherMode mode)
{
  launched = true;
  scheduled = false;
  if (entryCount == 0 && worldVolumeCount == 0)
    return;
  if (mode == GatherMode::Synchronous)
    doJob();
  else
  {
    threadpool::add(this, threadpool::PRIO_NORMAL, /*wake_up*/ mode == GatherMode::Async);
    scheduled = true;
  }
}

void DagdpRiexGatherJob::doJob()
{
  {
    TIME_PROFILE(dagdp_volume_gather_world_volumes);
    for (size_t i = 0; i < worldVolumeCount; ++i)
      gatherVolume(volumePool[i]);
  }

  {
    TIME_PROFILE(dagdp_volume_gather_riex_instances);
    for (size_t i = 0; i < entryCount; ++i)
    {
      auto &e = entryPool[i];
      rendinst::getRiGenExtraInstances(e.handles, e.resIdx, e.box);
    }
    for (size_t v = 0; v < viewportCount; ++v)
      for (auto &rec : viewportDataPool[v].onRi)
        filterRecord(rec);
  }

  {
    TIME_PROFILE(dagdp_volume_gather_local_volumes);
    for (size_t v = 0; v < viewportCount; ++v)
      for (auto &rec : viewportDataPool[v].aroundRi)
        gatherLocalVolumes(rec, v);
  }

  DA_PROFILE_TAG_DESC(jobNameProfDesc, "world=%d riex_entries=%d local=%d meshes=%d", (int)worldVolumeCount, (int)entryCount,
    (int)(volumeCount - worldVolumeCount), (int)meshPool.size());
}

void DagdpRiexGatherJob::gatherLocalVolumes(AroundRiPlacerData &rec, size_t viewport_index)
{
  size_t riexSourceCount = 0;
  for (size_t i = 0; i < rec.entryCount; ++i)
    riexSourceCount += entryPool[rec.firstEntry + i].handles.size();
  if (rec.localVolumeCount == 0 || riexSourceCount + rec.riGenSourceCount == 0)
    return;

  if (sourceTmScratch.size() < riexSourceCount)
    sourceTmScratch.resize(riexSourceCount);
  {
    TIME_PROFILE(dagdp_volume_source_tms);
    rendinst::ScopedRIExtraReadLock rd;
    size_t sourceIndex = 0;
    for (size_t i = 0; i < rec.entryCount; ++i)
      for (auto handle : entryPool[rec.firstEntry + i].handles)
      {
        mat44f sourceTm44;
        v_mat43_transpose_to_mat44(sourceTm44, rendinst::getRIGenExtra43(handle));
        v_mat_43cu_from_mat44(sourceTmScratch[sourceIndex++].array, sourceTm44);
      }
  }

  const vec4f viewPos = viewportDataPool[viewport_index].worldPos;
  const Frustum &frustum = viewportDataPool[viewport_index].frustum;
  const auto riGenSources = make_span_const(riGenSourcePool.data() + rec.firstRiGenSource, rec.riGenSourceCount);
  const auto riexSources = make_span_const(sourceTmScratch.data(), riexSourceCount);
  for (size_t localIndex = 0; localIndex < rec.localVolumeCount; ++localIndex)
  {
    LocalVolumeSnapshot &local = localVolumePool[rec.firstLocalVolume + localIndex];
    local.firstVolume = volumeCount;

    const auto gatherPairs = [&](dag::ConstSpan<TMatrix> sources) {
      for (const TMatrix &sourceTm : sources)
      {
        DECL_ALIGN16(TMatrix, tm) = sourceTm * local.transform;

        mat44f tm44;
        v_mat44_make_from_43cu_unsafe(tm44, tm.array);
        VolumeEntry e{};
        init_volume_entry(e, rec.variant, tm, tm44, local.scale, local.volumeType, maxBoundingRadius);
        if (!volume_in_draw_range(e.center, e.extent2, viewPos, rec.drawRadius) ||
            !frustum.testBoxExtentB(v_add(e.center, e.center), e.extent2))
          continue;

        prepare_volume_entry(e, tm44);
        gatherVolume(addVolumeEntry(e));
      }
    };
    // RIGen sources first: the order the synchronous path emits in.
    gatherPairs(riGenSources);
    gatherPairs(riexSources);

    local.volumeCount = volumeCount - local.firstVolume;
  }
}

void DagdpRiexGatherJob::waitDone()
{
  if (scheduled)
  {
    DA_PROFILE_EVENT_DESC(waitDesc);
    threadpool::wait(this);
    scheduled = false;
  }
  launched = false;
}

static void truncate_gathered_info(GatheredInfo &gathered_info, const VolumeConstants &constants)
{
  const uint32_t requiredMeshes = gathered_info.relevantMeshes.size();
  const uint32_t requiredDispatches = gathered_info.numDispatches();
  const uint32_t requiredTiles = gathered_info.relevantTiles.size();
  const uint32_t requiredVolumes = gathered_info.relevantVolumes.size();

  if (requiredMeshes <= constants.maxMeshes && requiredDispatches <= constants.maxDispatches && requiredTiles <= constants.maxTiles &&
      requiredVolumes <= constants.maxVolumes)
    return;

  uint32_t fittingViewportsEnd = 0;
  for (const auto &item : gathered_info.perViewport)
  {
    if (item.meshEndIndex > constants.maxMeshes || item.dispatchEndIndex > constants.maxDispatches ||
        item.tileEndIndex > constants.maxTiles || item.volumeEndIndex > constants.maxVolumes)
      break;
    ++fittingViewportsEnd;
  }

  const bool any = fittingViewportsEnd > 0;
  const uint32_t meshEnd = any ? gathered_info.perViewport[fittingViewportsEnd - 1].meshEndIndex : 0;
  const uint32_t dispatchEnd = any ? gathered_info.perViewport[fittingViewportsEnd - 1].dispatchEndIndex : 0;
  const uint32_t tileEnd = any ? gathered_info.perViewport[fittingViewportsEnd - 1].tileEndIndex : 0;
  const uint32_t volumeEnd = any ? gathered_info.perViewport[fittingViewportsEnd - 1].volumeEndIndex : 0;

  gathered_info.relevantMeshes.resize(meshEnd);
  gathered_info.truncateDispatches(dispatchEnd);
  gathered_info.relevantTiles.resize(tileEnd);
  gathered_info.relevantVolumes.resize(volumeEnd);

  // make the rest of the viewports that don't fit no-op
  for (uint32_t i = fittingViewportsEnd; i < gathered_info.perViewport.size(); ++i)
  {
    auto &item = gathered_info.perViewport[i];
    item.tileStartIndex = item.tileEndIndex = tileEnd;
    item.dispatchStartIndex = item.dispatchEndIndex = dispatchEnd;
    item.meshEndIndex = meshEnd;
    item.volumeEndIndex = volumeEnd;
  }

  RequiredLimits required;
  required.maxMeshes = requiredMeshes;
  required.maxTiles = requiredTiles;
  required.maxVolumes = requiredVolumes;
  GlobalManager::updateRequiredLimits(required);
}

void create_volume_nodes(const ViewInfo &view_info,
  const ViewBuilder &view_builder,
  const VolumeManager &volume_manager,
  NodeInserter node_inserter,
  const RulesBuilder &rules_builder)
{
  FRAMEMEM_REGION;

  auto &builder = volume_manager.currentBuilder;
  CommonPlacerBufferInit commonBufferInit;

  for (const auto &variant : builder.variants)
    add_variant(commonBufferInit, variant.objectGroups, variant.density, 0.0f);

  if (commonBufferInit.numPlaceables == 0)
    return;

  TmpName bufferNamePrefix(TmpName::CtorSprintf(), "dagdp_%s_volume", view_info.uniqueName.c_str());
  auto persistentData = eastl::make_shared<VolumePersistentData>();
  g_volume_views.push_back(persistentData);
  VolumeConstants &constants = persistentData->constants;
  constants.mapping = eastl::move(builder.mapping);
  constants.viewInfo = view_info;
  persistentData->mutables.riexGatherJob.initProfileMarkers(view_info.uniqueName.c_str());
  constants.viewResourceName.append_sprintf("view@%s", constants.viewInfo.uniqueName.c_str());
  constants.numRenderables = view_builder.numRenderables;
  constants.numPlaceables = commonBufferInit.numPlaceables;
  constants.prngSeed = 0; // Not implemented for now.
  constants.totalCounters = constants.viewInfo.maxViewports * constants.numRenderables;
  constants.maxObjects = rules_builder.maxObjects;
  constants.maxTriangles = rules_builder.maxTriangles;
  constants.maxMeshes = rules_builder.maxMeshes;
  constants.maxDispatches =
    ((rules_builder.maxMeshes + MAX_DISPATCH_MESHES - 1) / MAX_DISPATCH_MESHES + MAX_VB_INDICES) * view_info.maxViewports;
  constants.maxTiles = rules_builder.maxTiles;
  constants.maxVolumes = rules_builder.maxVolumes;
  constants.targetMeshLod = rules_builder.targetMeshLod;

  constants.maxPlaceableBoundingRadius = 0.0f;
  for (const auto &variant : builder.variants)
    for (const auto &objectGroup : variant.objectGroups)
      constants.maxPlaceableBoundingRadius = max(constants.maxPlaceableBoundingRadius, objectGroup.info->maxPlaceableBoundingRadius);

  bool commonSuccess = init_common_placer_buffers(commonBufferInit, bufferNamePrefix, persistentData->commonBuffers);
  if (!commonSuccess)
    return;

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_areas", bufferNamePrefix.c_str());
    persistentData->areasBuffer =
      dag::buffers::create_ua_sr_structured(sizeof(float), constants.maxTriangles, bufferName.c_str(), d3d::buffers::Init::Zero);
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_volume_variants", bufferNamePrefix.c_str());
    persistentData->volumeVariantsBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(VolumeVariantGpuData), builder.variants.size(), bufferName.c_str());

    dag::Vector<VolumeVariantGpuData, framemem_allocator> variantsGpuDataFmem(builder.variants.size());

    for (uint32_t i = 0; i < builder.variants.size(); ++i)
    {
      const auto &variant = builder.variants[i];
      auto &gpuData = variantsGpuDataFmem[i];
      gpuData.density = variant.density;
      gpuData.minTriangleArea2 = 2.0f * variant.minTriangleArea;
      gpuData.distBasedScale = variant.distBasedScale;
      gpuData.distBasedCenter = variant.distBasedCenter;
      gpuData.distBasedInvRange = 1.0f / max(variant.distBasedRange, 1e-6f);
      gpuData.sampleRange = variant.sampleRange;
      gpuData.flags = (variant.axisAbs ? VOLUME_AXIS_ABS_BIT : 0);
    }

    bool updated = persistentData->volumeVariantsBuffer->updateData(0, data_size(variantsGpuDataFmem), variantsGpuDataFmem.data(),
      VBLOCK_WRITEONLY);

    if (!updated)
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return;
    }
  }

  debug("[FRAMEMEM] Allocated buffers in daGDP:");

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_volumes", bufferNamePrefix.c_str());
    persistentData->volumesBuffer =
      dag::buffers::create_one_frame_sr_structured(sizeof(VolumeGpuData), constants.maxVolumes, bufferName.c_str());
    debug("[FRAMEMEM] - For volumes '%s': %d", bufferName.c_str(), constants.maxVolumes);
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_meshes", bufferNamePrefix.c_str());
    persistentData->meshesBuffer =
      dag::buffers::create_one_frame_sr_structured(sizeof(MeshIntersection), constants.maxMeshes, bufferName.c_str());
    debug("[FRAMEMEM] - For meshes '%s': %d", bufferName.c_str(), constants.maxMeshes);
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_dispatches", bufferNamePrefix.c_str());
    const uint32_t maxDispatchBoundaries = GatheredInfo::numDispatchBoundaries(constants.maxDispatches);
    persistentData->dispatchesBuffer =
      dag::buffers::create_one_frame_sr_structured(sizeof(uint32_t), maxDispatchBoundaries, bufferName.c_str());
    debug("[FRAMEMEM] - For dispatches '%s': %d", bufferName.c_str(), maxDispatchBoundaries);
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_tiles", bufferNamePrefix.c_str());
    persistentData->tilesBuffer =
      dag::buffers::create_one_frame_sr_structured(sizeof(VolumeTerrainTile), constants.maxTiles, bufferName.c_str());
    debug("[FRAMEMEM] - For tiles '%s': %d", bufferName.c_str(), constants.maxTiles);
  }

  constants.passBlock =
    get_dagdp_view_block(constants.viewInfo.uniqueName.c_str())
      .refineBlock(TmpName(TmpName::CtorSprintf(), "dagdp_volume_pass@%s", constants.viewInfo.uniqueName.c_str()).c_str());

  const dafg::NameSpace ns = dafg::root() / "dagdp" / view_info.uniqueName.c_str() / "volume";

  dafg::NodeHandle gatherNode = ns.registerNode("gather", DAFG_PP_NODE_SRC, [persistentData](dafg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    registry.registerBuffer("areas", [persistentData](auto) { return ManagedBufView(persistentData->areasBuffer); })
      .atStage(dafg::Stage::COMPUTE)
      .useAs(dafg::Usage::SHADER_RESOURCE);

    const auto viewHandle = registry.readBlob<ViewPerFrameData>(constants.viewResourceName.c_str()).handle();
    const auto gatheredInfoHandle = registry.createBlob<GatheredInfo>("gatheredInfo").handle();
    return [persistentData, viewHandle, gatheredInfoHandle] {
      const auto &constants = persistentData->constants;
      auto &gatheredInfo = gatheredInfoHandle.ref();
      const auto &view = viewHandle.ref();
      G_ASSERT(view.viewports.size() <= constants.viewInfo.maxViewports);

      if (is_volume_early_riex_gather_enabled())
        gather_start(persistentData->mutables.riexGatherJob, constants.mapping, constants.viewInfo, view,
          constants.maxPlaceableBoundingRadius, constants.targetMeshLod, persistentData->mutables.riexProcessor,
          GatherMode::Synchronous);

      persistentData->mutables.riexProcessor.resetCurrent();

      gatheredInfo.dispatches.push_back(0);
      RelevantMeshes meshes;
      for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
      {
        const uint32_t meshStartIndex = gatheredInfo.relevantMeshes.size();
        const uint32_t tileStartIndex = gatheredInfo.relevantTiles.size();
        const uint32_t dispatchStartIndex = gatheredInfo.numDispatches();
        const auto &viewport = view.viewports[viewportIndex];
        gather_process(persistentData->mutables.riexGatherJob, persistentData->constants.mapping, constants.viewInfo, viewportIndex,
          viewport, constants.maxPlaceableBoundingRadius, constants.targetMeshLod, persistentData->mutables.riexProcessor, meshes,
          gatheredInfo.relevantTiles, gatheredInfo.relevantVolumes);

        auto &item = gatheredInfo.perViewport.push_back();
        item.tileStartIndex = tileStartIndex;
        item.tileEndIndex = gatheredInfo.relevantTiles.size();

        // sort meshes by vbIndex
        {
          TIME_PROFILE(dagdp_volume_sort_meshes);
          const uint32_t numMeshes = meshes.size();

          uint32_t buckets[MAX_VB_INDICES + 1];
          memset(buckets, 0, sizeof(buckets));

          // count items per bucket
          for (uint32_t meshIndex = 0; meshIndex < numMeshes; meshIndex++)
          {
            const uint32_t vbIndex = meshes[meshIndex].vbIndex;
            G_ASSERT(vbIndex < MAX_VB_INDICES);
            buckets[vbIndex + 1]++;
          }

          // prefix-sum counts to array offsets
          for (uint32_t i = 0, s = meshStartIndex; i <= MAX_VB_INDICES; i++)
          {
            uint32_t n = buckets[i];
            uint32_t s0 = s;
            s += n;
            buckets[i] = s;
            while (s0 < s)
            {
              s0 += min<uint32_t>(s - s0, MAX_DISPATCH_MESHES);
              gatheredInfo.dispatches.push_back(s0);
            }
          }
          G_ASSERT(buckets[MAX_VB_INDICES] == meshStartIndex + numMeshes);

          // put meshes in the sorted order
          gatheredInfo.relevantMeshes.resize(meshStartIndex + numMeshes);
          for (uint32_t meshIndex = 0; meshIndex < numMeshes; meshIndex++)
          {
            const auto &mesh = meshes[meshIndex];
            const uint32_t newIndex = buckets[mesh.vbIndex]++;
            gatheredInfo.relevantMeshes[newIndex] = mesh;
          }
        }

        item.dispatchStartIndex = dispatchStartIndex;
        item.dispatchEndIndex = gatheredInfo.numDispatches();
        item.meshEndIndex = gatheredInfo.relevantMeshes.size();
        item.volumeEndIndex = gatheredInfo.relevantVolumes.size();

        meshes.clear();
      }
      meshes.shrink_to_fit();

      truncate_gathered_info(gatheredInfo, constants);

      if (const auto *nextToProcess = persistentData->mutables.riexProcessor.current(); nextToProcess != nullptr)
      {
        const dag::ConstSpan<ShaderMesh::RElem> elems =
          nextToProcess->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque);

        for (const auto &elem : elems)
        {
          auto &item = gatheredInfo.meshesToProcess.push_back();
          item.startIndex = elem.si;
          item.numFaces = elem.numf;
          item.baseVertex = elem.baseVertex;
          item.stride = elem.vertexData->getStride();
          item.vbIndex = elem.vertexData->getVbIdx();
        }
      }

      TIME_PROFILE(dagdp_volume_upload);

      if (!update_frame_mem(make_span_const(gatheredInfo.relevantMeshes), persistentData->meshesBuffer.getBuf(),
            persistentData->constants.maxMeshes, "volume meshes"))
        return;

      const uint32_t maxDispatchBoundaries = GatheredInfo::numDispatchBoundaries(constants.maxDispatches);
      if (!update_frame_mem(make_span_const(gatheredInfo.dispatches), persistentData->dispatchesBuffer.getBuf(), maxDispatchBoundaries,
            "volume dispatches"))
        return;

      if (!update_frame_mem(make_span_const(gatheredInfo.relevantTiles), persistentData->tilesBuffer.getBuf(),
            persistentData->constants.maxTiles, "volume tiles"))
        return;

      if (!update_frame_mem(make_span_const(gatheredInfo.relevantVolumes), persistentData->volumesBuffer.getBuf(),
            persistentData->constants.maxVolumes, "volumes"))
        return;

      gatheredInfo.isValid = true;
    };
  });

  dafg::NodeHandle processNode = ns.registerNode("process", DAFG_PP_NODE_SRC, [persistentData](dafg::Registry registry) {
    const auto gatheredInfoHandle = registry.readBlob<GatheredInfo>("gatheredInfo").handle();
    registry.modify("areas").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__areas");

    return [persistentData, gatheredInfoHandle, shaderTri = ComputeShader("dagdp_volume_mesh_process_tri"),
             shaderUp = ComputeShader("dagdp_volume_mesh_process_up"), shaderDown = ComputeShader("dagdp_volume_mesh_process_down")] {
      FRAMEMEM_REGION;

      const auto &gatheredInfo = gatheredInfoHandle.ref();
      AreasIndices areasIndices;
      bool outOfMem = false;

      const auto bumpAllocate = [&used = persistentData->mutables.areasUsed, &outOfMem,
                                  maxTriangles = persistentData->constants.maxTriangles](uint32_t count) {
        uint32_t offset = used;
        used += count;
        if (used > maxTriangles)
          outOfMem = true;
        return offset;
      };

      struct DispatchTri
      {
        MeshToProcess mesh;
        uint32_t areasStartOffset;
      };

      struct DispatchUpDown
      {
        uint32_t areasBottomCount;
        uint32_t areasBottomOffset;
        uint32_t areasTopOffset;
        uint32_t areasTopCount;
      };

      PerProcessedMesh<DispatchTri> dispatchTris;
      PerLevel<PerProcessedMesh<DispatchUpDown>> dispatchUpDowns; // Up forwards <=> down backwards

      for (const auto &mesh : gatheredInfo.meshesToProcess)
      {
        uint32_t count = mesh.numFaces;
        uint32_t prevOffset = ~0u;
        uint32_t prevCount = 0;

        uint32_t level = 0;
        do
        {
          if (level == 0)
          {
            auto &item = dispatchTris.push_back();
            item.mesh = mesh;
            item.areasStartOffset = bumpAllocate(count);
            prevOffset = item.areasStartOffset;
            prevCount = count;
            areasIndices.push_back(item.areasStartOffset);
          }
          else
          {
            if (dispatchUpDowns.size() < level)
              dispatchUpDowns.push_back();

            auto &item = dispatchUpDowns[level - 1].push_back();
            item.areasBottomOffset = prevOffset;
            item.areasBottomCount = prevCount;
            item.areasTopOffset = bumpAllocate(count);
            item.areasTopCount = count;
            prevOffset = item.areasTopOffset;
            prevCount = item.areasTopCount;
          }

          ++level;
          count = divUp(count, DAGDP_PREFIX_SUM_GROUP_SIZE) - 1;
        } while (count > 0);
      }

      if (outOfMem)
      {
        RequiredLimits required;
        required.maxTriangles = persistentData->mutables.areasUsed;
        GlobalManager::updateRequiredLimits(required);
        return;
      }

      STATE_GUARD(ShaderGlobal::set_buffer(var::meshes, VALUE), persistentData->meshesBuffer.getBufId(), BAD_D3DRESID);

      Ibuffer *ib = unitedvdata::riUnitedVdata.getIB();
      d3d::set_buffer(STAGE_CS, 0, ib);

      bool dispatchSuccess = true;
      for (const auto &dispatch : dispatchTris)
      {
        const auto &mesh = dispatch.mesh;
        Vbuffer *vb = unitedvdata::riUnitedVdata.getVB(mesh.vbIndex);
        d3d::set_buffer(STAGE_CS, 1, vb);
        ShaderGlobal::set_int4(var::mesh_params, IPoint4(mesh.startIndex, mesh.numFaces, mesh.baseVertex, mesh.stride));
        ShaderGlobal::set_int(var::areas_start_offset, dispatch.areasStartOffset);
        dispatchSuccess &= shaderTri.dispatchThreads(mesh.numFaces, 1, 1);
      }

      d3d::set_buffer(STAGE_CS, 0, nullptr);
      d3d::set_buffer(STAGE_CS, 1, nullptr);

      d3d::resource_barrier({persistentData->areasBuffer.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE});

      for (const auto &perLevel : dispatchUpDowns)
      {
        for (const auto &dispatch : perLevel)
        {
          ShaderGlobal::set_int(var::areas_bottom_offset, dispatch.areasBottomOffset);
          ShaderGlobal::set_int(var::areas_top_offset, dispatch.areasTopOffset);
          ShaderGlobal::set_int(var::areas_count, dispatch.areasTopCount);
          dispatchSuccess &= shaderUp.dispatchThreads(dispatch.areasTopCount, 1, 1);
        }

        d3d::resource_barrier({persistentData->areasBuffer.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE});
      }

      // Backwards.
      for (auto it = dispatchUpDowns.end(); it != dispatchUpDowns.begin();)
      {
        const auto &level = *--it;
        for (const auto &dispatch : level)
        {
          ShaderGlobal::set_int(var::areas_bottom_offset, dispatch.areasBottomOffset);
          ShaderGlobal::set_int(var::areas_top_offset, dispatch.areasTopOffset);
          ShaderGlobal::set_int(var::areas_count, dispatch.areasBottomCount);
          dispatchSuccess &= shaderDown.dispatchThreads(dispatch.areasBottomCount, 1, 1);
        }

        if (it != dispatchUpDowns.begin())
        {
          // Can skip this barrier on the last level.
          d3d::resource_barrier({persistentData->areasBuffer.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE});
        }
      }

      if (!dispatchSuccess)
      {
        logerr("daGdp: volume mesh processing dispatch failed!");
        return;
      }

      persistentData->mutables.riexProcessor.markCurrentAsProcessed(eastl::move(areasIndices));
    };
  });

  dafg::NodeHandle setArgsNode = ns.registerNode("set_args", DAFG_PP_NODE_SRC, [persistentData](dafg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    registry.create("dispatch_args")
      .buffer({sizeof(uint32_t), DISPATCH_INDIRECT_NUM_ARGS * constants.maxDispatches, SBCF_UA_SR_BYTE_ADDRESS | SBCF_UA_INDIRECT, 0})
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("dagdp_volume__dispatch_args");
    registry.create("mesh_objects")
      .buffer({sizeof(uint32_t), constants.maxMeshes * 2, SBCF_UA_SR_STRUCTURED, 0})
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("dagdp_volume__mesh_objects");

    registry.read("areas").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__areas");

    const auto gatheredInfoHandle = registry.readBlob<GatheredInfo>("gatheredInfo").handle();

    return [persistentData, gatheredInfoHandle, shader = ComputeShader("dagdp_volume_set_args")] {
      const auto &gatheredInfo = gatheredInfoHandle.ref();

      if (!gatheredInfo.isValid || gatheredInfo.numDispatches() == 0)
        return;

      STATE_GUARD(ShaderGlobal::set_buffer(var::meshes, VALUE), persistentData->meshesBuffer.getBufId(), BAD_D3DRESID);
      STATE_GUARD(ShaderGlobal::set_buffer(var::dispatches, VALUE), persistentData->dispatchesBuffer.getBufId(), BAD_D3DRESID);
      STATE_GUARD(ShaderGlobal::set_buffer(var::volumes, VALUE), persistentData->volumesBuffer.getBufId(), BAD_D3DRESID);
      STATE_GUARD(ShaderGlobal::set_buffer(var::variants, VALUE), persistentData->commonBuffers.variantsBuffer.getBufId(),
        BAD_D3DRESID);
      STATE_GUARD(ShaderGlobal::set_buffer(var::volume_variants, VALUE), persistentData->volumeVariantsBuffer.getBufId(),
        BAD_D3DRESID);

      const bool dispatchSuccess = shader.dispatchGroups(gatheredInfo.numDispatches(), 1, 1);

      if (!dispatchSuccess)
        logerr("daGdp: volume set args dispatch failed!");
    };
  });

  const auto createPlaceNode = [&](bool isOptimistic) {
    return ns.registerNode(isOptimistic ? "place_stage0" : "place_stage1", DAFG_PP_NODE_SRC,
      [isOptimistic, persistentData](dafg::Registry registry) {
        const auto &constants = persistentData->constants;
        view_multiplex(registry, constants.viewInfo.kind);

        registry.read("areas").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__areas");
        registry.read("mesh_objects").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("dagdp_volume__mesh_objects");
        const auto dispatchArgsHandle =
          registry.read("dispatch_args").buffer().atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::INDIRECTION_BUFFER).handle();
        const auto gatheredInfoHandle = registry.readBlob<GatheredInfo>("gatheredInfo").handle();
        const auto viewHandle = registry.readBlob<ViewPerFrameData>(constants.viewResourceName.c_str()).handle();

        const auto ns = registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str();

        ns.read(isOptimistic ? "dyn_allocs_stage0" : "dyn_allocs_stage1")
          .buffer()
          .atStage(dafg::Stage::COMPUTE)
          .bindToShaderVar("dagdp__dyn_allocs");

        ns.modify(isOptimistic ? "dyn_counters_stage0" : "dyn_counters_stage1")
          .buffer()
          .atStage(dafg::Stage::COMPUTE)
          .bindToShaderVar("dagdp__dyn_counters");

        ns.modify("instance_data").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("dagdp__instance_data");

        return [persistentData, viewHandle, gatheredInfoHandle, dispatchArgsHandle, isOptimistic,
                 shader = ComputeShader(isOptimistic ? "dagdp_volume_place_stage0" : "dagdp_volume_place_stage1")] {
          const auto &constants = persistentData->constants;
          const auto &view = viewHandle.ref();
          const auto &gatheredInfo = gatheredInfoHandle.ref();
          auto *dispatchArgsBuffer = const_cast<Sbuffer *>(dispatchArgsHandle.get());

          if (!gatheredInfo.isValid)
            return;

          bool dispatchSuccess = true;
          TIME_D3D_PROFILE(dagdp_mesh_placers);

          STATE_GUARD(ShaderGlobal::set_buffer(var::volumes, VALUE), persistentData->volumesBuffer.getBufId(), BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::meshes, VALUE), persistentData->meshesBuffer.getBufId(), BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::dispatches, VALUE), persistentData->dispatchesBuffer.getBufId(), BAD_D3DRESID);

          STATE_GUARD(ShaderGlobal::set_buffer(var::draw_ranges, VALUE), persistentData->commonBuffers.drawRangesBuffer.getBufId(),
            BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::placeables, VALUE), persistentData->commonBuffers.placeablesBuffer.getBufId(),
            BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::placeable_weights, VALUE),
            persistentData->commonBuffers.placeableWeightsBuffer.getBufId(), BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::renderable_indices, VALUE),
            persistentData->commonBuffers.renderableIndicesBuffer.getBufId(), BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::variants, VALUE), persistentData->commonBuffers.variantsBuffer.getBufId(),
            BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::volume_variants, VALUE), persistentData->volumeVariantsBuffer.getBufId(),
            BAD_D3DRESID);

          ShaderGlobal::set_float(var::debug_frustum_culling_bias, get_frustum_culling_bias());
          ShaderGlobal::set_float(var::max_placeable_bounding_radius, constants.maxPlaceableBoundingRadius);
          ShaderGlobal::set_int(var::num_renderables, constants.numRenderables);
          ShaderGlobal::set_int(var::num_placeables, constants.numPlaceables);
          ShaderGlobal::set_int(var::prng_seed_placeable, constants.prngSeed + 0x08C2592Cu);
          ShaderGlobal::set_int(var::prng_seed_scale, constants.prngSeed + 0xDF3069FFu);
          ShaderGlobal::set_int(var::prng_seed_slope, constants.prngSeed + 0x3C1385DBu);
          ShaderGlobal::set_int(var::prng_seed_occlusion, constants.prngSeed + 0x93C0FB91u);
          ShaderGlobal::set_int(var::prng_seed_yaw, constants.prngSeed + 0x71F23960u);
          ShaderGlobal::set_int(var::prng_seed_pitch, constants.prngSeed + 0xDEB40CF0u);
          ShaderGlobal::set_int(var::prng_seed_roll, constants.prngSeed + 0xF6A38A81u);
          ShaderGlobal::set_int(var::prng_seed_triangle1, constants.prngSeed + 0x54F2A367u);
          ShaderGlobal::set_int(var::prng_seed_triangle2, constants.prngSeed + 0x45F9668Eu);

          constants.passBlock.setState();

          Ibuffer *ib = unitedvdata::riUnitedVdata.getIB();
          d3d::set_buffer(STAGE_CS, 0, ib);

          if (d3d::get_driver_code().is(d3d::vulkan))
            d3d::driver_command(Drv3dCommand::DELAY_SYNC);

          for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
          {
            const auto &viewport = view.viewports[viewportIndex];
            ScopeFrustumPlanesShaderVars scopedFrustumVars(viewport.frustum);

            ShaderGlobal::set_float4(var::viewport_pos, viewport.worldPos);
            ShaderGlobal::set_float(var::viewport_max_distance,
              min(viewport.maxDrawDistance, constants.viewInfo.maxDrawDistance) * get_global_range_scale());
            ShaderGlobal::set_int(var::viewport_index, viewportIndex);

            const auto &info = gatheredInfo.perViewport[viewportIndex];
            for (uint32_t dispatchIndex = info.dispatchStartIndex, endIndex = info.dispatchEndIndex; dispatchIndex < endIndex;
                 dispatchIndex++)
            {
              uint32_t meshIndex = gatheredInfo.dispatches[dispatchIndex];
              const auto &mesh = gatheredInfo.relevantMeshes[meshIndex];
              Vbuffer *vb = unitedvdata::riUnitedVdata.getVB(mesh.vbIndex);
              d3d::set_buffer(STAGE_CS, 1, vb);
              ShaderGlobal::set_int(var::dispatch_index, dispatchIndex);

              // TODO: Performance: ideally need to merge all dispatches (regardless of viewport / vbIndex / meshIndex).
              // TODO: Performance: in the pessimistic phase, should set dispatch args to zero, based on success/failure of the
              // optimistic phase. Should be easy to do, since they are already indirect.
              int indirectOffset = DISPATCH_INDIRECT_NUM_ARGS * sizeof(uint32_t) * dispatchIndex;
              dispatchSuccess &= shader.dispatchIndirect(dispatchArgsBuffer, indirectOffset);
            }
          }

          if (d3d::get_driver_code().is(d3d::vulkan))
            d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);

          d3d::set_buffer(STAGE_CS, 0, nullptr);
          d3d::set_buffer(STAGE_CS, 1, nullptr);

          if (!dispatchSuccess)
            logerr("daGdp: volume place dispatch failed! (optimistic = %d)", isOptimistic);
        };
      });
  };

  const auto createTerrainPlaceNode = [&](bool isOptimistic) {
    return ns.registerNode(isOptimistic ? "terrain_place_stage0" : "terrain_place_stage1", DAFG_PP_NODE_SRC,
      [isOptimistic, persistentData](dafg::Registry registry) {
        const auto &constants = persistentData->constants;
        view_multiplex(registry, constants.viewInfo.kind);

        const auto viewHandle = registry.readBlob<ViewPerFrameData>(constants.viewResourceName.c_str()).handle();
        const auto gatheredInfoHandle = registry.readBlob<GatheredInfo>("gatheredInfo").handle();

        const auto ns = registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str();

        ns.read(isOptimistic ? "dyn_allocs_stage0" : "dyn_allocs_stage1")
          .buffer()
          .atStage(dafg::Stage::COMPUTE)
          .bindToShaderVar("dagdp__dyn_allocs");

        ns.modify(isOptimistic ? "dyn_counters_stage0" : "dyn_counters_stage1")
          .buffer()
          .atStage(dafg::Stage::COMPUTE)
          .bindToShaderVar("dagdp__dyn_counters");

        ns.modify("instance_data").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("dagdp__instance_data");

        return [persistentData, viewHandle, gatheredInfoHandle, isOptimistic,
                 shader = ComputeShader(isOptimistic ? "dagdp_volume_terrain_place_stage0" : "dagdp_volume_terrain_place_stage1")] {
          const auto &constants = persistentData->constants;
          const auto &view = viewHandle.ref();
          const auto &gatheredInfo = gatheredInfoHandle.ref();

          if (!gatheredInfo.isValid)
            return;

          STATE_GUARD(ShaderGlobal::set_buffer(var::volumes, VALUE), persistentData->volumesBuffer.getBufId(), BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::tiles, VALUE), persistentData->tilesBuffer.getBufId(), BAD_D3DRESID);

          STATE_GUARD(ShaderGlobal::set_buffer(var::draw_ranges, VALUE), persistentData->commonBuffers.drawRangesBuffer.getBufId(),
            BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::placeables, VALUE), persistentData->commonBuffers.placeablesBuffer.getBufId(),
            BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::placeable_weights, VALUE),
            persistentData->commonBuffers.placeableWeightsBuffer.getBufId(), BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::renderable_indices, VALUE),
            persistentData->commonBuffers.renderableIndicesBuffer.getBufId(), BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::variants, VALUE), persistentData->commonBuffers.variantsBuffer.getBufId(),
            BAD_D3DRESID);
          STATE_GUARD(ShaderGlobal::set_buffer(var::volume_variants, VALUE), persistentData->volumeVariantsBuffer.getBufId(),
            BAD_D3DRESID);

          ShaderGlobal::set_float(var::debug_frustum_culling_bias, get_frustum_culling_bias());
          ShaderGlobal::set_float(var::max_placeable_bounding_radius, constants.maxPlaceableBoundingRadius);
          ShaderGlobal::set_int(var::num_renderables, constants.numRenderables);
          ShaderGlobal::set_int(var::num_placeables, constants.numPlaceables);

          ShaderGlobal::set_int(var::prng_seed_placeable, constants.prngSeed + 0x08C2592Cu);
          ShaderGlobal::set_int(var::prng_seed_scale, constants.prngSeed + 0xDF3069FFu);
          ShaderGlobal::set_int(var::prng_seed_slope, constants.prngSeed + 0x3C1385DBu);
          ShaderGlobal::set_int(var::prng_seed_yaw, constants.prngSeed + 0x71F23960u);
          ShaderGlobal::set_int(var::prng_seed_pitch, constants.prngSeed + 0xDEB40CF0u);
          ShaderGlobal::set_int(var::prng_seed_roll, constants.prngSeed + 0xF6A38A81u);
          ShaderGlobal::set_int(var::prng_seed_triangle1, constants.prngSeed + 0x54F2A367u);
          ShaderGlobal::set_int(var::prng_seed_triangle2, constants.prngSeed + 0x45F9668Eu);
          ShaderGlobal::set_int(var::prng_seed_jitter_x, constants.prngSeed + 0xDE437762u);
          ShaderGlobal::set_int(var::prng_seed_jitter_z, constants.prngSeed + 0xA2EA427Du);

          constants.passBlock.setState();

          bool dispatchSuccess = true;
          for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
          {
            const auto &viewport = view.viewports[viewportIndex];
            const auto &info = gatheredInfo.perViewport[viewportIndex];
            ScopeFrustumPlanesShaderVars scopedFrustumVars(viewport.frustum);

            ShaderGlobal::set_float4(var::viewport_pos, viewport.worldPos);
            ShaderGlobal::set_float(var::viewport_max_distance,
              min(viewport.maxDrawDistance, constants.viewInfo.maxDrawDistance) * get_global_range_scale());
            ShaderGlobal::set_int(var::viewport_index, viewportIndex);
            ShaderGlobal::set_int(var::tile_start_index, info.tileStartIndex);

            // TODO: Performance: ideally need to merge all dispatches.
            int size = (gatheredInfo.perViewport[viewportIndex].tileEndIndex - gatheredInfo.perViewport[viewportIndex].tileStartIndex);
            if (size > 0)
              dispatchSuccess &= shader.dispatchThreads(size, 1, 1);
          }

          if (!dispatchSuccess)
            logerr("daGdp: volume place dispatch failed! (optimistic = %d)", isOptimistic);
        };
      });
  };

  node_inserter(eastl::move(gatherNode));
  node_inserter(eastl::move(processNode));

  node_inserter(eastl::move(setArgsNode));
  node_inserter(createPlaceNode(true));
  node_inserter(createPlaceNode(false));

  node_inserter(createTerrainPlaceNode(true));
  node_inserter(createTerrainPlaceNode(false));
}

template <typename Populate>
static void start_gather_views(GatherMode mode, const Populate &populate)
{
  if (!is_volume_early_riex_gather_enabled())
    return;

  for (auto it = g_volume_views.begin(); it != g_volume_views.end();)
  {
    auto pd = it->lock(); // it is valid as long as the node that registered it is alive
    if (!pd)
    {
      it = g_volume_views.erase(it);
      continue;
    }
    ++it;

    ViewPerFrameData viewData;
    if (populate(pd->constants.viewInfo, viewData))
    {
      pd->mutables.riexGatherJob.waitDone(); // device lost leaves its start undrained
      gather_start(pd->mutables.riexGatherJob, pd->constants.mapping, pd->constants.viewInfo, viewData,
        pd->constants.maxPlaceableBoundingRadius, pd->constants.targetMeshLod, pd->mutables.riexProcessor, mode);
    }
  }
}

void start_gather_camera_volumes(const Point3 &cam_pos, const Frustum &cam_frustum)
{
  start_gather_views(GatherMode::AsyncNoWake, [&](const ViewInfo &info, ViewPerFrameData &out) {
    if (info.kind != ViewKind::MAIN_CAMERA && info.kind != ViewKind::BVH)
      return false;
    auto &vp = out.viewports.emplace_back();
    vp.worldPos = DPoint3(cam_pos.x, cam_pos.y, cam_pos.z);
    vp.maxDrawDistance = FLT_MAX;
    vp.frustum = info.kind == ViewKind::BVH ? Frustum(get_bvh_culling_matrix(cam_pos)) : cam_frustum;
    return true;
  });
}

void start_gather_csm_volumes(ecs::EntityManager &manager)
{
  start_gather_views(GatherMode::Async, [&](const ViewInfo &info, ViewPerFrameData &out) {
    if (info.kind != ViewKind::CSM_SHADOWS)
      return false;
    populate_csm_viewports(out, manager, info.maxViewports);
    return true;
  });
}

} // namespace dagdp
