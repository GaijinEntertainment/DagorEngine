// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "render/world/wrDispatcher.h"
#include "render/world/shadowsManager.h"
#include "globalManager.h"
#include "CSMShadows.h"
#include <render/renderSettings.h>

ECS_REGISTER_RELOCATABLE_TYPE(dagdp::CSMShadowsManager, nullptr);

namespace dagdp
{

template <typename Callable>
static inline void level_settings_ecs_query(ecs::EntityManager &manager, Callable);

static inline int csm_max_cascade_count(ecs::EntityManager &manager)
{
  int maxCascade = 3;
  level_settings_ecs_query(manager,
    [&](int dagdp__csm_max_cascades ECS_REQUIRE(ecs::Tag dagdp_level_settings)) { maxCascade = dagdp__csm_max_cascades; });
  return maxCascade;
}

ECS_NO_ORDER static inline void csm_shadows_recreate_views_es(
  const dagdp::EventRecreateViews &evt, ecs::EntityManager &manager, dagdp::CSMShadowsManager &dagdp__csm_shadows_manager)
{
  FRAMEMEM_REGION;
  G_UNUSED(dagdp__csm_shadows_manager);

  const auto &config = *evt.get<0>();
  const auto &viewInserter = evt.get<1>();
  const float maxShadowDist = eastl::min(config.dynamicShadowQualityParams.maxShadowDist, GLOBAL_MAX_DRAW_DISTANCE);

  if (!config.enableDynamicShadows)
    return;

  ShadowsManager &shadowsManager = WRDispatcher::getShadowsManager();
  auto csm = shadowsManager.getCascadeShadows();
  int maxCascade = csm_max_cascade_count(manager);
  if (!csm || shadowsManager.getSettingsQuality() < ShadowsQuality::SHADOWS_HIGH || maxCascade == 0)
    return;

  ViewInfo &info = viewInserter();
  info.uniqueName = "csm";
  info.kind = ViewKind::CSM_SHADOWS;
  info.maxDrawDistance = maxShadowDist;
  info.maxViewports = eastl::min({CSM_MAX_CASCADES, DAGDP_MAX_VIEWPORTS, maxCascade});
}

dafg::NodeHandle create_csm_shadows_provider(const dafg::NameSpace &ns, int max_cascades)
{
  const auto declare = [max_cascades](dafg::Registry registry) {
    view_multiplex(registry, ViewKind::CSM_SHADOWS);
    auto viewDataHandle = registry.createBlob<ViewPerFrameData>("view@csm").handle();
    return [viewDataHandle, max_cascades] {
      auto &viewData = viewDataHandle.ref();

      ShadowsManager &shadowsManager = WRDispatcher::getShadowsManager();
      auto csm = shadowsManager.getCascadeShadows();
      if (!csm)
        return;

      int numCascades = csm->getNumCascadesToRender();
      numCascades = eastl::min({numCascades, DAGDP_MAX_VIEWPORTS, max_cascades});
      if (numCascades <= 0)
        return;

      viewData.viewports.resize(numCascades);
      for (int ci = 0; ci < numCascades; ci++)
      {
        auto &viewport = viewData.viewports[ci];
        viewport.worldPos = csm->getRenderCameraWorldViewPos(ci);
        viewport.maxDrawDistance = csm->getMaxShadowDistance();
        viewport.frustum = csm->getFrustum(ci);
        viewport.csmCascade = ci;
      }
    };
  };

  return ns.registerNode("csm_camera_provider", DAFG_PP_NODE_SRC, declare);
};

template <typename Callable>
static void shadows_query_manager_ecs_query(Callable c);

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__shadowsQuality)
ECS_REQUIRE(const ecs::string &render_settings__shadowsQuality)
ECS_AFTER(init_shadows_es)
static void recreate_views_on_shadow_settings_change_es(const ecs::Event &)
{
  shadows_query_manager_ecs_query([](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.destroyViews(); });
}

} // namespace dagdp
