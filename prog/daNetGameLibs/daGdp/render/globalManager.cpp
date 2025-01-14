// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/shared_ptr.h>
#include <generic/dag_enumerate.h>
#include <debug/dag_log.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_hlsl_floatx.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <shaders/dag_computeShaders.h>
#include <render/daBfg/bfg.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <drv/3d/dag_rwResource.h>
#include <ecs/rendInst/riExtra.h>
#include "../shaders/dagdp_common.hlsli"
#include "../shaders/dagdp_dynamic.hlsli"
#include "globalManager.h"

namespace var
{
static ShaderVariableInfo dyn_region("dagdp__dyn_region");
static ShaderVariableInfo dyn_counters_num("dagdp__dyn_counters_num");
}; // namespace var

using TmpName = eastl::fixed_string<char, 256>;

namespace dagdp
{

struct ViewConstants
{
  dagdp::ViewInfo viewInfo;
  uint32_t totalMaxInstances; // static for all viewports, and dynamic.
  uint32_t maxStaticInstancesPerViewport;
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

void GlobalManager::reconfigure(const GlobalConfig &new_config)
{
  config = new_config;
  destroyViews();
}

void GlobalManager::invalidateRules()
{
  rulesAreValid = false;
  invalidateViews();
}

void GlobalManager::destroyViews()
{
  viewsAreCreated = false;
  views.clear();
  viewIndependentNodes.clear();
  invalidateRules();
}

void GlobalManager::recreateViews()
{
  G_ASSERT(!viewsAreCreated);
  G_ASSERT(views.empty());

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

  g_entity_mgr->broadcastEventImmediate(EventInvalidateViews());

  viewsAreBuilt = false;
}

void GlobalManager::update()
{
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
    rebuildViews();
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

void GlobalManager::rebuildViews()
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

  dag::Vector<ViewInfo> dynShadowsViews;
  dynShadowsViews.reserve(dynamic_shadow_render::ESTIMATED_MAX_SHADOWS_TO_UPDATE_PER_FRAME);

  for (auto &view : views)
  {
    ViewBuilder viewBuilder;
    viewBuilder.numRenderables = numRenderables;
    viewBuilder.renderablesMaxInstances.resize(numRenderables);

    // While processing, each placer will add to the static limits.
    g_entity_mgr->broadcastEventImmediate(EventViewProcess(rulesBuilder, view.info, &viewBuilder));

    // At this point, all static limits are known, and we can calculate the instance regions.
    viewBuilder.renderablesInstanceRegions.reserve(numRenderables);

    for (const uint32_t maxInstances : viewBuilder.renderablesMaxInstances)
    {
      InstanceRegion region;
      region.baseIndex = viewBuilder.maxStaticInstancesPerViewport;
      region.maxCount = maxInstances;
      viewBuilder.renderablesInstanceRegions.push_back(region);
      viewBuilder.maxStaticInstancesPerViewport += maxInstances;
    }

    viewBuilder.totalMaxInstances = viewBuilder.maxStaticInstancesPerViewport * view.info.maxViewports;
    if (viewBuilder.hasDynamicPlacers && rulesBuilder.maxObjects > 0) // -V560
    {
      // Dynamic region starts after all static regions.
      viewBuilder.dynamicInstanceRegion.baseIndex = viewBuilder.totalMaxInstances;
      viewBuilder.dynamicInstanceRegion.maxCount = rulesBuilder.maxObjects;
      viewBuilder.totalMaxInstances += rulesBuilder.maxObjects;
    }

    if (viewBuilder.totalMaxInstances > 0)
    {
      dabfg::NodeHandle viewProviderNode;

      switch (view.info.kind)
      {
        case ViewKind::MAIN_CAMERA:
        {
          const auto declare = [&view](dabfg::Registry registry) {
            view_multiplex(registry, view.info.kind);
            auto viewDataHandle = registry.createBlob<ViewPerFrameData>("view@main_camera", dabfg::History::No).handle();
            auto cameraHandle = registry.readBlob<CameraParamsUnified>("current_camera_unified").handle();
            return [viewDataHandle, cameraHandle] {
              const auto &camera = cameraHandle.ref();
              auto &viewData = viewDataHandle.ref();
              auto &viewport = viewData.viewports.emplace_back();
              viewport.frustum = camera.unionFrustum;
              viewport.maxDrawDistance = FLT_MAX;
              viewport.worldPos = camera.cameraWorldPos;
            };
          };

          const auto ns = dabfg::root() / "dagdp";
          view.nodes.push_back(ns.registerNode("main_camera_provider", DABFG_PP_NODE_SRC, declare));
          break;
        }
        case ViewKind::DYN_SHADOWS:
        {
          // We want to create one node for all views of this kind, so just accumulate the needed data.
          dynShadowsViews.push_back(view.info);
          break;
        }
        default: G_ASSERTF(false, "Unknown view kind");
      }

      if (viewProviderNode)
        view.nodes.push_back(eastl::move(viewProviderNode));

      auto persistentData = eastl::make_shared<ViewPersistentData>();
      persistentData->constants.viewInfo = view.info;
      persistentData->constants.totalMaxInstances = viewBuilder.totalMaxInstances;
      persistentData->constants.maxStaticInstancesPerViewport = viewBuilder.maxStaticInstancesPerViewport;
      persistentData->constants.totalCounters = view.info.maxViewports * numRenderables;
      persistentData->constants.numRenderables = numRenderables;
      persistentData->constants.dynamicInstanceRegion = viewBuilder.dynamicInstanceRegion;

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

      const dabfg::NameSpace ns = dabfg::root() / "dagdp" / view.info.uniqueName.c_str();

      dabfg::NodeHandle setupNode = ns.registerNode("setup", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
        const auto &constants = persistentData->constants;
        view_multiplex(registry, constants.viewInfo.kind);

        const size_t perInstanceFormatElementStride = get_tex_format_desc(perInstanceFormat).bytesPerElement;
        G_ASSERT(sizeof(PerInstanceData) % perInstanceFormatElementStride == 0);
        const size_t perInstanceElements = sizeof(PerInstanceData) / perInstanceFormatElementStride;

        registry.create("instance_data", dabfg::History::No)
          .buffer({static_cast<uint32_t>(perInstanceFormatElementStride),
            static_cast<uint32_t>(constants.totalMaxInstances * perInstanceElements), SBCF_BIND_UNORDERED | SBCF_BIND_SHADER_RES,
            perInstanceFormat});

        registry.registerBuffer("per_draw_gathered_data", [](auto) { return ManagedBufView(get_per_draw_gathered_data()); })
          .atStage(dabfg::Stage::TRANSFER);
      });

      dabfg::NodeHandle staticSetupNode =
        ns.registerNode("setup_static", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
          const auto &constants = persistentData->constants;
          view_multiplex(registry, constants.viewInfo.kind);

          const auto countersHandle = registry.create("counters", dabfg::History::No)
                                        .structuredBufferUaSr<uint32_t>(constants.totalCounters)
                                        .atStage(dabfg::Stage::UNKNOWN) // TODO: d3d::clear_rwbufi semantics is not defined.
                                        .useAs(dabfg::Usage::UNKNOWN)
                                        .handle();

          return [countersHandle] {
            uint32_t zeros[4] = {};
            d3d::clear_rwbufi(countersHandle.get(), zeros);
          };
        });

      dabfg::NodeHandle dynamicSetupNode =
        ns.registerNode("setup_dynamic", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
          const auto &constants = persistentData->constants;
          view_multiplex(registry, constants.viewInfo.kind);

          const auto dynCountersHandle = registry.create("dyn_counters_stage0", dabfg::History::No)
                                           .structuredBufferUaSr<uint32_t>(constants.totalCounters + DYN_COUNTERS_PREFIX)
                                           .atStage(dabfg::Stage::UNKNOWN) // TODO: d3d::clear_rwbufi semantics is not defined.
                                           .useAs(dabfg::Usage::UNKNOWN)
                                           .handle();

          registry
            .registerBuffer("dyn_allocs_stage0", [persistentData](auto) { return ManagedBufView(persistentData->dynAllocsBuffer); })
            .atStage(dabfg::Stage::COMPUTE)
            .useAs(dabfg::Usage::SHADER_RESOURCE);

          return [dynCountersHandle] {
            uint32_t zeros[4] = {};
            d3d::clear_rwbufi(dynCountersHandle.get(), zeros);
          };
        });

      dabfg::NodeHandle dynamicRecountNode = ns.registerNode("recount", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
        const auto &constants = persistentData->constants;
        view_multiplex(registry, constants.viewInfo.kind);

        registry.rename("dyn_allocs_stage0", "dyn_allocs_stage1", dabfg::History::No)
          .buffer()
          .atStage(dabfg::Stage::COMPUTE)
          .bindToShaderVar("dagdp__dyn_allocs");

        registry.rename("dyn_counters_stage0", "dyn_counters_stage1", dabfg::History::No)
          .buffer()
          .atStage(dabfg::Stage::COMPUTE)
          .bindToShaderVar("dagdp__dyn_counters");

        return [persistentData, shader = ComputeShader("dagdp_dynamic_recount")] {
          const auto &constants = persistentData->constants;

          // TODO: to support more, need a multi-stage prefix sum here. Not done for simplicity.
          G_ASSERT(constants.totalCounters <= DAGDP_DYNAMIC_RECOUNT_GROUP_SIZE);

          ShaderGlobal::set_int(var::dyn_counters_num, constants.totalCounters);
          ShaderGlobal::set_int4(var::dyn_region, constants.dynamicInstanceRegion.baseIndex, constants.dynamicInstanceRegion.maxCount,
            0, 0);
          const bool res = shader.dispatchThreads(constants.totalCounters, 1, 1);
          if (!res)
            logerr("daGdp: dynamic recount dispatch failed!");
        };
      });

      dabfg::NodeHandle dynamicReadbackNode =
        ns.registerNode("readback", DABFG_PP_NODE_SRC, [persistentData, &view](dabfg::Registry registry) {
          view_multiplex(registry, persistentData->constants.viewInfo.kind);
          registry.executionHas(dabfg::SideEffects::External);

          const auto dynCountersHandle =
            registry.read("dyn_counters_stage1").buffer().atStage(dabfg::Stage::TRANSFER).useAs(dabfg::Usage::COPY).handle();

          return [persistentData, dynCountersHandle, &view] {
            auto &dynCounters = const_cast<Sbuffer &>(dynCountersHandle.ref());

            int ignoredStride;
            uint32_t ignoredFrameNum;
            if (const auto *data = (uint32_t *)persistentData->readback.lock(ignoredStride, ignoredFrameNum, false))
            {
              const uint32_t totalPlaced = data[DYN_COUNTERS_INDEX_TOTAL_PLACED];
              const uint32_t totalCapacity = data[DYN_COUNTERS_INDEX_TOTAL_CAPACITY];

#if DAGDP_DEBUG
              view.dynamicInstanceCounter = totalPlaced;
#endif

              if (totalPlaced > totalCapacity * DYNAMIC_THRESHOLD_MULTIPLIER)
                LOGERR_ONCE("daGdp: dynamic placement overflow detected! %" PRIu32 " > %" PRIu32 " * %f", totalPlaced, totalCapacity,
                  DYNAMIC_THRESHOLD_MULTIPLIER);

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

      if (viewBuilder.maxStaticInstancesPerViewport > 0)
        view.nodes.push_back(eastl::move(staticSetupNode));

      if (viewBuilder.dynamicInstanceRegion.maxCount > 0)
      {
        view.nodes.push_back(eastl::move(dynamicSetupNode));
        view.nodes.push_back(eastl::move(dynamicRecountNode));
        view.nodes.push_back(eastl::move(dynamicReadbackNode));
      }

      const auto nodeInserter = [&nodes = view.nodes](dabfg::NodeHandle node) { nodes.push_back(eastl::move(node)); };
      g_entity_mgr->broadcastEventImmediate(EventViewFinalize(view.info, viewBuilder, nodeInserter));
    }

#if DAGDP_DEBUG
    debug.builders.emplace_back(eastl::move(viewBuilder));
#endif
  }

  if (dynShadowsViews.size() > 0)
  {
    const auto declare = [views = eastl::move(dynShadowsViews)](dabfg::Registry registry) {
      view_multiplex(registry, ViewKind::DYN_SHADOWS);

      auto updatesHandle = registry.readBlob<dynamic_shadow_render::FrameUpdates>("scene_shadow_updates").handle();
      auto mappingHandle =
        registry.createBlob<dynamic_shadow_render::FrameVector<int>>("scene_shadow_updates_mapping", dabfg::History::No).handle();

      dag::Vector<dabfg::VirtualResourceHandle<ViewPerFrameData, false, false>> viewDataHandles;
      viewDataHandles.reserve(views.size());

      for (const auto &info : views)
      {
        // Workaround for the fact that dabfg::NameSpaceRequest does not allow to create resources.
        // But in fact we want this node to create resources for all of the views!
        eastl::fixed_string<char, 32> resourceName;
        resourceName.append_sprintf("view@%s", info.uniqueName.c_str());
        viewDataHandles.push_back(registry.createBlob<ViewPerFrameData>(resourceName.c_str(), dabfg::History::No).handle());
      }

      return [viewDataHandles = eastl::move(viewDataHandles), views, updatesHandle, mappingHandle] {
        const auto &updates = updatesHandle.ref();
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

    const auto ns = dabfg::root() / "dagdp";
    viewIndependentNodes.push_back(ns.registerNode("dyn_shadows_provider", DABFG_PP_NODE_SRC, declare));

    const auto nodeInserter = [&nodes = viewIndependentNodes](dabfg::NodeHandle node) { nodes.push_back(eastl::move(node)); };
    g_entity_mgr->broadcastEventImmediate(EventFinalize(nodeInserter));
  }

  viewsAreBuilt = true;
}

} // namespace dagdp