// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/shared_ptr.h>
#include <generic/dag_enumerate.h>
#include <debug/dag_log.h>
#include <util/dag_convar.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_hlsl_floatx.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_refinedBlock.h>
#include <render/daFrameGraph/daFG.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <drv/3d/dag_rwResource.h>
#include <ecs/rendInst/riExtra.h>
#include <render/noiseTex.h>
#include <math/dag_Point3.h>
#include "../shaders/dagdp_common.hlsli"
#include "../shaders/dagdp_dynamic.hlsli"
#include "block.h"
#include "globalManager.h"
#include <render/world/bvh.h>
#include <render/world/dynamicShadowRenderExtender.h>
#include "CSMShadows.h"

namespace var
{
static ShaderVariableInfo dyn_region("dagdp__dyn_region", true);
static ShaderVariableInfo dyn_counters_num("dagdp__dyn_counters_num", true);
}; // namespace var

using TmpName = eastl::fixed_string<char, 256>;

namespace dagdp
{

struct ViewConstants
{
  dagdp::ViewInfo viewInfo;
  uint32_t totalMaxInstances;
  uint32_t totalCounters; // Of each kind.
  uint32_t numRenderables;
  InstanceRegion dynamicInstanceRegion;
};

struct ViewPersistentData
{
  UniqueBuf dynAllocsBuffer;
  RingCPUBufferLock readback;
  ViewConstants constants;
};

float GlobalManager::globalDensityMul = 1;

CONSOLE_FLOAT_VAL_MINMAX("dagdp", lod_range_mul, 1.0f, 0.1f, 1.0f);

float GlobalManager::lodRangeMul() { return lod_range_mul.get(); }
void GlobalManager::setLodRangeMul(float mul) { lod_range_mul.set(mul); }

// Caps for grow_limit, so that content with excessive demands cannot grow the
// GPU buffers without bound. Each cap must stay above the template default of
// its dagdp__max_* setting, else growth cannot engage for default-configured
// levels. The volumes cap is also a hard bound: grid3d packs volume start/end
// indices into 16 bits each (see packVolumeRange), and a 1 << 16 limit would
// overflow that packing.
static constexpr uint32_t GROWN_LIMIT_CAP_MESHES = 1 << 16;
static constexpr uint32_t GROWN_LIMIT_CAP_TILES = 1 << 16;
static constexpr uint32_t GROWN_LIMIT_CAP_3D_TILES = 1 << 16;
static constexpr uint32_t GROWN_LIMIT_CAP_VOLUMES = 0xFFFF;
static constexpr uint32_t GROWN_LIMIT_CAP_TRIANGLES = 1 << 22;
static constexpr uint32_t GROWN_LIMIT_CAP_OBJECTS = 1 << 16;

RequiredLimits GlobalManager::requiredLimits;

void GlobalManager::updateRequiredLimits(const RequiredLimits &required)
{
  requiredLimits.maxMeshes = max(requiredLimits.maxMeshes, required.maxMeshes);
  requiredLimits.maxTiles = max(requiredLimits.maxTiles, required.maxTiles);
  requiredLimits.max3dTiles = max(requiredLimits.max3dTiles, required.max3dTiles);
  requiredLimits.maxVolumes = max(requiredLimits.maxVolumes, required.maxVolumes);
  requiredLimits.maxObjects = max(requiredLimits.maxObjects, required.maxObjects);
  requiredLimits.maxTriangles = max(requiredLimits.maxTriangles, required.maxTriangles);
}

// Grows limit to 1.5x the required count, up to cap. Returns true if the limit changed. grown_limit records the new floor.
static bool grow_limit(uint32_t required, uint32_t &limit, uint32_t &grown_limit, uint32_t cap, const char *what)
{
  if (required <= limit)
    return false;

  const uint32_t newLimit = min(cap, required + required / 2);
  if (newLimit <= limit)
  {
    LOGERR_ONCE("daGdp: %u %s needed, but the limit (%u) cannot grow past the cap (%u). Placement will be incomplete. "
                "Reduce the amount of content covered by daGdp placers.",
      required, what, limit, cap);
    return false;
  }

  logwarn("daGdp: growing max. number of %s: %u -> %u (%u needed). Views will be rebuilt.", what, limit, newLimit, required);
  limit = newLimit;
  grown_limit = newLimit;
  return true;
}

void GlobalManager::applyRequiredLimits()
{
  const RequiredLimits required = requiredLimits;
  requiredLimits = {};

  // Note: only the max* scalars of rulesBuilder are mutated; they are consumed
  // at view build time and no pointers into rulesBuilder are affected.
  bool grown = false;
  grown |= grow_limit(required.maxMeshes, rulesBuilder.maxMeshes, grownLimits.maxMeshes, GROWN_LIMIT_CAP_MESHES, "volume meshes");
  grown |= grow_limit(required.maxTiles, rulesBuilder.maxTiles, grownLimits.maxTiles, GROWN_LIMIT_CAP_TILES, "volume tiles");
  grown |= grow_limit(required.max3dTiles, rulesBuilder.max3dTiles, grownLimits.max3dTiles, GROWN_LIMIT_CAP_3D_TILES, "3d tiles");
  grown |= grow_limit(required.maxVolumes, rulesBuilder.maxVolumes, grownLimits.maxVolumes, GROWN_LIMIT_CAP_VOLUMES, "volumes");

  grown |=
    grow_limit(required.maxObjects, rulesBuilder.maxObjects, grownLimits.maxObjects, GROWN_LIMIT_CAP_OBJECTS, "dynamic objects");
  grown |= grow_limit(required.maxTriangles, rulesBuilder.maxTriangles, grownLimits.maxTriangles, GROWN_LIMIT_CAP_TRIANGLES,
    "volume triangles");

  if (grown)
    invalidateViews();
}

void GlobalManager::reconfigure(const GlobalConfig &new_config)
{
  config = new_config;
  if (!config.enabled)
  {
    grownLimits = {};
    requiredLimits = {};
  }
  destroyViews();
}

void GlobalManager::invalidateRules()
{
  rulesAreValid = false;
  invalidateViews();
}

GlobalManager::~GlobalManager() { destroyViews(); }

void GlobalManager::destroyViews()
{
  if (viewsAreCreated)
  {
    release_blue_noise();
    release_perline_noise_3d();
  }
  viewsAreCreated = false;
  views.clear();
  viewIndependentNodes.clear();
  invalidateRules();
}

void GlobalManager::recreateViews()
{
  G_ASSERT(!viewsAreCreated);
  G_ASSERT(views.empty());

  init_and_get_blue_noise();
  Point3 perlinNoiseMin, perlinNoiseMax;
  init_and_get_perlin_noise_3d(perlinNoiseMin, perlinNoiseMax);

  if (!config.enabled)
  {
    viewsAreCreated = true;
    return;
  }

  // Main view is always present.
  ViewInfo &mainCamera = views.push_back().info;
  mainCamera.uniqueName = "main_camera";
  mainCamera.kind = ViewKind::MAIN_CAMERA;
  mainCamera.maxDrawDistance = GLOBAL_MAX_DRAW_DISTANCE;
  mainCamera.maxViewports = 1;

  const auto viewInserter = [this]() -> ViewInfo & { return this->views.push_back().info; };
  g_entity_mgr->broadcastEventImmediate(EventRecreateViews(&config, viewInserter));

  viewsAreCreated = true;
}

void GlobalManager::invalidateViews()
{
  if (!viewsAreBuilt)
    return;

  for (auto &view : views)
    view.nodes.clear();

  viewIndependentNodes.clear();

#if DAGDP_DEBUG
  debug.builders.clear();
#endif

  if (viewsAreCreated)
    g_entity_mgr->broadcastEventImmediate(EventInvalidateViews());

  viewsAreBuilt = false;
}

void GlobalManager::update(ecs::EntityManager &manager)
{
  if (lod_range_mul.pullValueChange())
    invalidateRules();

  applyRequiredLimits();

  if (!viewsAreCreated)
  {
    const auto startRef = ref_time_ticks();
    recreateViews();
    ::debug("daGdp: recreateViews took %d us", get_time_usec(startRef));
  }

  if (!rulesAreValid)
  {
    const auto startRef = ref_time_ticks();
    rebuildRules();
    ::debug("daGdp: rebuildRules took %d us", get_time_usec(startRef));
  }

  if (!viewsAreBuilt)
  {
    const auto startRef = ref_time_ticks();
    rebuildViews(manager);
    ::debug("daGdp: rebuildViews took %d us", get_time_usec(startRef));
  }
}

void GlobalManager::rebuildRules()
{
  rulesBuilder = RulesBuilder{};

  if (!config.enabled)
  {
    rulesAreValid = true;
    return;
  }

  queryLevelSettings(rulesBuilder);

  // Grown limits are floors over the level settings, so that a rules rebuild
  // does not shrink the buffers back and later re-hitch on the same overflow.
  rulesBuilder.maxMeshes = max(rulesBuilder.maxMeshes, grownLimits.maxMeshes);
  rulesBuilder.maxTiles = max(rulesBuilder.maxTiles, grownLimits.maxTiles);
  rulesBuilder.max3dTiles = max(rulesBuilder.max3dTiles, grownLimits.max3dTiles);
  rulesBuilder.maxVolumes = max(rulesBuilder.maxVolumes, grownLimits.maxVolumes);
  rulesBuilder.maxObjects = max(rulesBuilder.maxObjects, grownLimits.maxObjects);
  rulesBuilder.maxTriangles = max(rulesBuilder.maxTriangles, grownLimits.maxTriangles);

  accumulateObjectGroups(rulesBuilder);
  accumulatePlacers(rulesBuilder);

  if (rulesBuilder.objectGroups.empty() || rulesBuilder.placers.empty())
  {
    rulesAreValid = true;
    return;
  }

  g_entity_mgr->broadcastEventImmediate(EventObjectGroupProcess(&rulesBuilder));
  g_entity_mgr->broadcastEventImmediate(EventInitialize());

  // Note: rulesBuilder and anything recursively contained in it should be considered immutable from this point, up to the point of
  // rules invalidation.
  rulesAreValid = true;
}

void GlobalManager::rebuildViews(ecs::EntityManager &manager)
{
  FRAMEMEM_REGION;
  G_ASSERT(rulesAreValid);
  // Since rules are already built, we know about all placeables/renderables at this point.
  uint32_t numRenderables = rulesBuilder.nextRenderableId;

  if (numRenderables == 0)
  {
    // Nothing to render...
    viewsAreBuilt = true;
    return;
  }

  viewIndependentNodes.push_back(
    (dafg::root() / "dagdp").registerNode("per_draw_data_setup", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.registerBuffer("per_draw_gathered_data", [](auto) { return ManagedBufView(get_per_draw_gathered_data()); })
        .atStage(dafg::Stage::TRANSFER);
    }));

  dag::Vector<ViewInfo> dynShadowsViews;
  dynShadowsViews.reserve(dynamic_shadow_render::ESTIMATED_MAX_SHADOWS_TO_UPDATE_PER_FRAME);

  for (auto &view : views)
  {
    ViewBuilder viewBuilder;
    viewBuilder.numRenderables = numRenderables;

    // While processing, each placer will add to the static limits.
    g_entity_mgr->broadcastEventImmediate(EventViewProcess(rulesBuilder, view.info, &viewBuilder));

    viewBuilder.dynamicInstanceRegion.baseIndex = viewBuilder.totalMaxInstances;
    viewBuilder.dynamicInstanceRegion.maxCount = rulesBuilder.maxObjects;
    viewBuilder.totalMaxInstances += rulesBuilder.maxObjects;

    if (viewBuilder.totalMaxInstances > 0)
    {
      dafg::NodeHandle viewProviderNode;

      switch (view.info.kind)
      {
        case ViewKind::MAIN_CAMERA:
        {
          const auto declare = [&view](dafg::Registry registry) {
            view_multiplex(registry, view.info.kind);
            auto viewDataHandle = registry.createBlob<ViewPerFrameData>("view@main_camera").handle();
            auto cameraHandle = registry.readBlob<CameraParamsUnified>("current_camera_unified").handle();
            return [viewDataHandle, cameraHandle] {
              const auto &camera = cameraHandle.ref();
              auto &viewData = viewDataHandle.ref();
              auto &viewport = viewData.viewports.emplace_back();
              viewport.frustum = camera.unionFrustum;
              viewport.maxDrawDistance = FLT_MAX;
              viewport.worldPos = camera.cameraWorldPos;
              set_global_range_scale(camera.rangeScale);
            };
          };

          const auto ns = dafg::root() / "dagdp";
          view.nodes.push_back(ns.registerNode("main_camera_provider", DAFG_PP_NODE_SRC, declare));
          break;
        }
        case ViewKind::DYN_SHADOWS:
        {
          // We want to create one node for all views of this kind, so just accumulate the needed data.
          dynShadowsViews.push_back(view.info);
          break;
        }
        case ViewKind::BVH:
        {
          const auto declare = [&view](dafg::Registry registry) {
            view_multiplex(registry, view.info.kind);
            auto viewDataHandle = registry.createBlob<ViewPerFrameData>("view@bvh").handle();
            auto cameraHandle = registry.readBlob<CameraParamsUnified>("current_camera_unified").handle();
            return [viewDataHandle, cameraHandle] {
              const auto &camera = cameraHandle.ref();
              auto &viewData = viewDataHandle.ref();
              auto &viewport = viewData.viewports.emplace_back();
              viewport.frustum = get_bvh_culling_matrix(Point3::xyz(camera.cameraWorldPos));
              viewport.maxDrawDistance = FLT_MAX;
              viewport.worldPos = camera.cameraWorldPos;
            };
          };

          const auto ns = dafg::root() / "dagdp";
          view.nodes.push_back(ns.registerNode("bvh_provider", DAFG_PP_NODE_SRC, declare));
          break;
        }
        case ViewKind::CSM_SHADOWS:
        {
          const auto ns = dafg::root() / "dagdp";
          view.nodes.push_back(create_csm_shadows_provider(ns, manager, view.info.maxViewports));
          break;
        }
        default: G_ASSERTF(false, "Unknown view kind");
      }

      if (viewProviderNode)
        view.nodes.push_back(eastl::move(viewProviderNode));

      auto persistentData = eastl::make_shared<ViewPersistentData>();
      persistentData->constants.viewInfo = view.info;
      persistentData->constants.totalMaxInstances = viewBuilder.totalMaxInstances;
      persistentData->constants.totalCounters = view.info.maxViewports * numRenderables;
      persistentData->constants.numRenderables = numRenderables;
      persistentData->constants.dynamicInstanceRegion = viewBuilder.dynamicInstanceRegion;

      auto viewBlock = get_dagdp_view_block(view.info.uniqueName.c_str());

      if (viewBuilder.dynamicInstanceRegion.maxCount > 0)
      {
        {
          TmpName bufferName(TmpName::CtorSprintf(), "dagdp_%s_dyn_allocs", view.info.uniqueName.c_str());
          persistentData->dynAllocsBuffer = dag::buffers::create_ua_sr_structured(sizeof(DynAlloc),
            view.info.maxViewports * numRenderables, bufferName.c_str(), d3d::buffers::Init::Zero);

          // Match last FG usage.
          d3d::resource_barrier({persistentData->dynAllocsBuffer.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
        }

        {
          TmpName bufferName(TmpName::CtorSprintf(), "dagdp_%s_readback", view.info.uniqueName.c_str());
          persistentData->readback.init(sizeof(uint32_t), DYN_COUNTERS_PREFIX, 4, bufferName.c_str(), SBCF_UA_STRUCTURED_READBACK, 0,
            false);
        }
      }

      const dafg::NameSpace ns = dafg::root() / "dagdp" / view.info.uniqueName.c_str();

      dafg::NodeHandle setupNode = ns.registerNode("setup", DAFG_PP_NODE_SRC, [persistentData](dafg::Registry registry) {
        const auto &constants = persistentData->constants;
        view_multiplex(registry, constants.viewInfo.kind);

        const size_t perInstanceFormatElementStride = get_tex_format_desc(perInstanceFormat).bytesPerElement;
        G_ASSERT(sizeof(PerInstanceData) % perInstanceFormatElementStride == 0);
        const size_t perInstanceElements = sizeof(PerInstanceData) / perInstanceFormatElementStride;

        registry.create("instance_data")
          .buffer({static_cast<uint32_t>(perInstanceFormatElementStride),
            static_cast<uint32_t>(constants.totalMaxInstances * perInstanceElements), SBCF_BIND_UNORDERED | SBCF_BIND_SHADER_RES,
            perInstanceFormat})
          .atStage(dafg::Stage::COMPUTE)
          .useAs(dafg::Usage::SHADER_RESOURCE);
      });

      dafg::NodeHandle dynamicSetupNode =
        ns.registerNode("setup_dynamic", DAFG_PP_NODE_SRC, [persistentData](dafg::Registry registry) {
          const auto &constants = persistentData->constants;
          view_multiplex(registry, constants.viewInfo.kind);

          const auto dynCountersHandle = registry.create("dyn_counters_stage0")
                                           .structuredBuffer<uint32_t>(constants.totalCounters + DYN_COUNTERS_PREFIX)
                                           .atStage(dafg::Stage::CS)
                                           .useAs(dafg::Usage::SHADER_RESOURCE) // TODO: Make a new usage "Clear" that will be platform
                                                                                // specific
                                           .handle();

          registry
            .registerBuffer("dyn_allocs_stage0", [persistentData](auto) { return ManagedBufView(persistentData->dynAllocsBuffer); })
            .atStage(dafg::Stage::COMPUTE)
            .useAs(dafg::Usage::SHADER_RESOURCE);

          return [dynCountersHandle] { d3d::zero_rwbufi(dynCountersHandle.get()); };
        });

      dafg::NodeHandle dynamicRecountNode = ns.registerNode("recount", DAFG_PP_NODE_SRC,
        [persistentData, passBlock = viewBlock.refineBlock("dynamic_recount")](dafg::Registry registry) {
          const auto &constants = persistentData->constants;
          view_multiplex(registry, constants.viewInfo.kind);

          registry.rename("dyn_allocs_stage0", "dyn_allocs_stage1")
            .buffer()
            .atStage(dafg::Stage::COMPUTE)
            .bindToShaderVar("dagdp__dyn_allocs");

          registry.rename("dyn_counters_stage0", "dyn_counters_stage1")
            .buffer()
            .atStage(dafg::Stage::COMPUTE)
            .bindToShaderVar("dagdp__dyn_counters");

          return [persistentData, shader = ComputeShader("dagdp_dynamic_recount"), passBlock] {
            const auto &constants = persistentData->constants;

            // TODO: to support more, need a multi-stage prefix sum here. Not done for simplicity.
            G_ASSERT(constants.totalCounters <= DAGDP_DYNAMIC_RECOUNT_GROUP_SIZE);
            passBlock.setState();

            ShaderGlobal::set_int(var::dyn_counters_num, constants.totalCounters);
            ShaderGlobal::set_int4(var::dyn_region, constants.dynamicInstanceRegion.baseIndex,
              constants.dynamicInstanceRegion.maxCount, 0, 0);

            const bool res = shader.dispatchThreads(constants.totalCounters, 1, 1);
            if (!res)
              logerr("daGdp: dynamic recount dispatch failed!");
          };
        });

      dafg::NodeHandle dynamicReadbackNode =
        ns.registerNode("readback", DAFG_PP_NODE_SRC, [persistentData, &view](dafg::Registry registry) {
          view_multiplex(registry, persistentData->constants.viewInfo.kind);
          registry.executionHas(dafg::SideEffects::External);

          const auto dynCountersHandle =
            registry.read("dyn_counters_stage1").buffer().atStage(dafg::Stage::TRANSFER).useAs(dafg::Usage::COPY).handle();

          return [persistentData, dynCountersHandle, &view] {
            auto &dynCounters = const_cast<Sbuffer &>(dynCountersHandle.ref());

            int ignoredStride;
            uint32_t ignoredFrameNum;
            if (const auto *data = (uint32_t *)persistentData->readback.lock(ignoredStride, ignoredFrameNum, false))
            {
              const uint32_t totalPlaced = data[DYN_COUNTERS_INDEX_TOTAL_PLACED];
              const uint32_t totalCapacity = persistentData->constants.dynamicInstanceRegion.maxCount;
#if DAGDP_DEBUG
              view.dynamicInstanceCounter = totalPlaced;
#else
              G_UNUSED(view);
#endif
              if (totalCapacity > 0 && totalPlaced > totalCapacity * DYNAMIC_GROW_THRESHOLD)
              {
                RequiredLimits required;
                required.maxObjects = static_cast<uint32_t>(totalPlaced / DYNAMIC_GROW_THRESHOLD) + 1;
                GlobalManager::updateRequiredLimits(required);
              }

              persistentData->readback.unlock();
            }

            auto *target = (Sbuffer *)persistentData->readback.getNewTarget(ignoredFrameNum);
            if (target)
            {
              dynCounters.copyTo(target, 0, 0, sizeof(uint32_t) * DYN_COUNTERS_PREFIX);
              persistentData->readback.startCPUCopy();
            }
          };
        });

      view.nodes.push_back(eastl::move(setupNode));

      if (viewBuilder.dynamicInstanceRegion.maxCount > 0)
      {
        view.nodes.push_back(eastl::move(dynamicSetupNode));
        view.nodes.push_back(eastl::move(dynamicRecountNode));
        view.nodes.push_back(eastl::move(dynamicReadbackNode));
      }

      const auto nodeInserter = [&nodes = view.nodes](dafg::NodeHandle node) { nodes.push_back(eastl::move(node)); };
      g_entity_mgr->broadcastEventImmediate(
        EventViewFinalize(view.info, viewBuilder, nodeInserter, rulesBuilder, triangleSizeDebugEnabled));
    }

#if DAGDP_DEBUG
    debug.builders.emplace_back(eastl::move(viewBuilder));
#endif
  }

  if (dynShadowsViews.size() > 0)
  {
    const auto declare = [views = eastl::move(dynShadowsViews)](dafg::Registry registry) {
      view_multiplex(registry, ViewKind::DYN_SHADOWS);

      auto updatesHandle = registry.readBlob<SceneShadowRenderData>("scene_shadow_render_data").handle();
      auto mappingHandle = registry.createBlob<dynamic_shadow_render::FrameVector<int>>("scene_shadow_updates_mapping").handle();

      dag::Vector<dafg::VirtualResourceHandle<ViewPerFrameData, false, false>> viewDataHandles;
      viewDataHandles.reserve(views.size());

      for (const auto &info : views)
      {
        // Workaround for the fact that dafg::NameSpaceRequest does not allow to create resources.
        // But in fact we want this node to create resources for all of the views!
        eastl::fixed_string<char, 32> resourceName;
        resourceName.append_sprintf("view@%s", info.uniqueName.c_str());
        viewDataHandles.push_back(registry.createBlob<ViewPerFrameData>(resourceName.c_str()).handle());
      }

      return [viewDataHandles = eastl::move(viewDataHandles), views, updatesHandle, mappingHandle] {
        const auto &updates = updatesHandle.ref().volumeData.staticUpdates;
        auto &mapping = mappingHandle.ref();
        mapping.assign(updates.size(), -1);

        for (const auto [updateIndex, update] : enumerate(updates))
        {
          if (update.renderGPUObjects == DynamicShadowRenderGPUObjects::NO)
            continue;

          int bestViewIndex = -1;
          float bestViewDistance = FLT_MAX;

          for (int i = 0; i < views.size(); ++i)
          {
            const auto &info = views[i];
            if (viewDataHandles[i].ref().viewports.empty() && info.maxViewports == update.numViews &&
                info.maxDrawDistance >= update.maxDrawDistance && info.maxDrawDistance < bestViewDistance)
            {
              bestViewDistance = info.maxDrawDistance;
              bestViewIndex = i;
            }
          }

          if (bestViewIndex == -1)
            logerr("daGdp: could not find a suitable view for a shadow update. This should not happen.");
          else
          {
            mapping[updateIndex] = bestViewIndex;

            // Note: shadow render views correspond to daGDP viewports.
            G_ASSERT(update.views.size() >= update.numViews);
            for (int i = 0; i < update.numViews; ++i)
            {
              auto &viewport = viewDataHandles[bestViewIndex].ref().viewports.push_back();
              mat44f viewProj;
              v_mat44_mul(viewProj, update.proj, update.views[i].view);
              Point3_vec4 p3;
              v_stu_p3(&p3.x, update.views[i].invView.col3);
              viewport.worldPos = p3;
              viewport.maxDrawDistance = update.maxDrawDistance;
              viewport.frustum = Frustum(viewProj);
            }
          }
        }
      };
    };

    const auto ns = dafg::root() / "dagdp";
    viewIndependentNodes.push_back(ns.registerNode("dyn_shadows_provider", DAFG_PP_NODE_SRC, declare));

    const auto nodeInserter = [&nodes = viewIndependentNodes](dafg::NodeHandle node) { nodes.push_back(eastl::move(node)); };
    g_entity_mgr->broadcastEventImmediate(EventFinalize(nodeInserter));
  }

  viewsAreBuilt = true;
}

} // namespace dagdp