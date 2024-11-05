// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_enumerate.h>
#include <daECS/core/coreEvents.h>
#include "globalManager.h"
#include "dynShadows.h"

static constexpr uint32_t SPOT_LIGHT_NUM_FRUSTUMS = 1;
static constexpr uint32_t OMNI_LIGHT_NUM_FRUSTUMS = 6;

ECS_REGISTER_RELOCATABLE_TYPE(dagdp::DynShadowsManager, nullptr);

namespace dagdp
{

template <typename Callable>
static inline void spot_lights_ecs_query(Callable);

template <typename Callable>
static inline void omni_lights_ecs_query(Callable);

ECS_NO_ORDER static inline void dyn_shadows_recreate_views_es(const EventRecreateViews &evt,
  DynShadowsManager &dagdp__dyn_shadows_manager)
{
  FRAMEMEM_REGION;
  G_UNUSED(dagdp__dyn_shadows_manager);

  const auto &config = *evt.get<0>();
  const auto &viewInserter = evt.get<1>();
  const float maxShadowDist = eastl::min(config.dynamicShadowQualityParams.maxShadowDist, GLOBAL_MAX_DRAW_DISTANCE);

  if (!config.enableDynamicShadows)
    return;

  dag::Vector<float, framemem_allocator> spotLightsFmem;
  dag::Vector<float, framemem_allocator> omniLightsFmem;

  spot_lights_ecs_query(
    [&](ECS_REQUIRE(eastl::true_type light__render_gpu_objects, eastl::true_type spot_light__shadows) float light__max_radius) {
      spotLightsFmem.push_back(light__max_radius);
    });

  omni_lights_ecs_query(
    [&](ECS_REQUIRE(eastl::true_type light__render_gpu_objects, eastl::true_type omni_light__shadows) float light__max_radius) {
      omniLightsFmem.push_back(light__max_radius);
    });

  stlsort::sort(spotLightsFmem.begin(), spotLightsFmem.end(), [](float a, float b) { return a >= b; });
  stlsort::sort(omniLightsFmem.begin(), omniLightsFmem.end(), [](float a, float b) { return a >= b; });

  const uint32_t maxPerFrame = static_cast<uint32_t>(config.dynamicShadowQualityParams.maxShadowsToUpdateOnFrame);
  spotLightsFmem.resize(eastl::min(maxPerFrame, spotLightsFmem.size()));
  omniLightsFmem.resize(eastl::min(maxPerFrame, omniLightsFmem.size()));

  // TODO: as the number of max. per-frame shadow updates is shared between all types of light sources,
  // we could in some cases reuse omni views as spot views. This is not yet done for simplicity.
  for (const auto [index, maxRadius] : enumerate(eastl::as_const(spotLightsFmem)))
  {
    ViewInfo &info = viewInserter();
    info.uniqueName.append_sprintf("spot_%d", index);
    info.kind = ViewKind::DYN_SHADOWS;
    info.maxDrawDistance = eastl::min(maxRadius, maxShadowDist);
    info.maxViewports = SPOT_LIGHT_NUM_FRUSTUMS;
  }

  for (const auto [index, maxRadius] : enumerate(eastl::as_const(omniLightsFmem)))
  {
    ViewInfo &info = viewInserter();
    info.uniqueName.append_sprintf("omni_%d", index);
    info.kind = ViewKind::DYN_SHADOWS;
    info.maxDrawDistance = eastl::min(maxRadius, maxShadowDist);
    info.maxViewports = OMNI_LIGHT_NUM_FRUSTUMS;
  }
}

template <typename Callable>
static inline void manager_ecs_query(Callable);

static void destroy_views()
{
  manager_ecs_query([](dagdp::GlobalManager &dagdp__global_manager) {
    // Note: this may be a relatively expensive operation.
    // The assumption is that the necessary lights are created at scene load, and then rarely, if ever, change.
    dagdp__global_manager.destroyViews();
  });
}

// TODO: *_lights_changed_es do not react to light__render_gpu_objects changes (ECS bug?).

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_ON_EVENT(on_disappear)
ECS_TRACK(light__render_gpu_objects, spot_light__shadows, light__max_radius)
ECS_REQUIRE(eastl::true_type light__render_gpu_objects, eastl::true_type spot_light__shadows, float light__max_radius)
static void spot_lights_changed_es(const ecs::Event &) { destroy_views(); }

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_ON_EVENT(on_disappear)
ECS_TRACK(light__render_gpu_objects, omni_light__shadows, light__max_radius)
ECS_REQUIRE(eastl::true_type light__render_gpu_objects, eastl::true_type omni_light__shadows, float light__max_radius)
static void omni_lights_changed_es(const ecs::Event &) { destroy_views(); }

} // namespace dagdp