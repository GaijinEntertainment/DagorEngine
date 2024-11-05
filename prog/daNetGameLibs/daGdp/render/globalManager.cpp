// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_enumerate.h>
#include <debug/dag_log.h>
#include <math/dag_vecMathCompatibility.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <render/daBfg/bfg.h>
#include "globalManager.h"
#include <drv/3d/dag_rwResource.h>
#include <ecs/rendInst/riExtra.h>

namespace dagdp
{

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

#if DAGOR_DBGLEVEL != 0
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

    // While processing, each placer will add to the total limits.
    g_entity_mgr->broadcastEventImmediate(EventViewProcess(rulesBuilder, view.info, &viewBuilder));

    // At this point, all view-specific limits are known, and we can calculate the instance regions.
    viewBuilder.renderablesInstanceRegions.reserve(numRenderables);

    for (const uint32_t maxInstances : viewBuilder.renderablesMaxInstances)
    {
      InstanceRegion region;
      region.baseIndex = viewBuilder.maxInstancesPerViewport;
      region.maxCount = maxInstances;
      viewBuilder.renderablesInstanceRegions.push_back(region);
      viewBuilder.maxInstancesPerViewport += maxInstances;
    }

    if (viewBuilder.maxInstancesPerViewport != 0)
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

      // Create the setup node.
      dabfg::NodeHandle setupNode =
        (dabfg::root() / "dagdp" / view.info.uniqueName.c_str())
          .registerNode("setup", DABFG_PP_NODE_SRC,
            [numRenderables, maxInstancesPerViewport = viewBuilder.maxInstancesPerViewport, info = view.info](
              dabfg::Registry registry) {
              view_multiplex(registry, info.kind);
              const uint32_t totalCounters = info.maxViewports * numRenderables;

              auto countersHandle = registry.create("counters", dabfg::History::No)
                                      .structuredBufferUaSr<uint32_t>(totalCounters)
                                      .atStage(dabfg::Stage::UNKNOWN) // TODO: d3d::clear_rwbufi semantics is not defined.
                                      .useAs(dabfg::Usage::UNKNOWN)
                                      .handle();

              const size_t perInstanceFormatElementStride = get_tex_format_desc(perInstanceFormat).bytesPerElement;
              G_ASSERT(sizeof(PerInstanceData) % perInstanceFormatElementStride == 0);
              const size_t perInstanceElements = sizeof(PerInstanceData) / perInstanceFormatElementStride;

              registry.create("instance_data", dabfg::History::No)
                .buffer({static_cast<uint32_t>(perInstanceFormatElementStride),
                  static_cast<uint32_t>(info.maxViewports * maxInstancesPerViewport * perInstanceElements),
                  SBCF_BIND_UNORDERED | SBCF_BIND_SHADER_RES, perInstanceFormat});

              registry.registerBuffer("per_draw_gathered_data", [](auto) { return ManagedBufView(get_per_draw_gathered_data()); })
                .atStage(dabfg::Stage::TRANSFER);

              return [countersHandle] {
                uint32_t zeros[4] = {};
                d3d::clear_rwbufi(countersHandle.get(), zeros);
              };
            });
      view.nodes.push_back(eastl::move(setupNode));

      const auto nodeInserter = [&nodes = view.nodes](dabfg::NodeHandle node) { nodes.push_back(eastl::move(node)); };
      g_entity_mgr->broadcastEventImmediate(EventViewFinalize(view.info, viewBuilder, nodeInserter));
    }

#if DAGOR_DBGLEVEL != 0
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