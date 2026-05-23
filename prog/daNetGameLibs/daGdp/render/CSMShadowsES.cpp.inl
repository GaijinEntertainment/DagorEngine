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
#include <camera/cameraUtils.h>

ECS_REGISTER_RELOCATABLE_TYPE(dagdp::CSMShadowsManager, nullptr);

namespace dagdp
{

template <typename Callable>
static inline void level_settings_ecs_query(ecs::EntityManager &manager, Callable);

static inline int csm_max_cascade_count(ecs::EntityManager &manager, float fov)
{
  int maxCascade = 3;
  level_settings_ecs_query(manager, [&](ecs::FloatList dagdp__csm_max_fov ECS_REQUIRE(ecs::Tag dagdp_level_settings)) {
    maxCascade = dagdp__csm_max_fov.size();
    if (fov < 0)
      return;
    fov = cam::fov_to_deg(fov);

    for (int i = 0; i < dagdp__csm_max_fov.size(); ++i)
    {
      G_ASSERT(i == 0 || dagdp__csm_max_fov[i - 1] <= dagdp__csm_max_fov[i]);
      if (dagdp__csm_max_fov[i] > 0 && maxCascade == dagdp__csm_max_fov.size() && fov > dagdp__csm_max_fov[i])
        maxCascade = i;
    }
  });
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
  int maxCascadeCount = csm_max_cascade_count(manager, -1);
  if (!csm || shadowsManager.getSettingsQuality() < ShadowsQuality::SHADOWS_HIGH || maxCascadeCount == 0)
    return;

  ViewInfo &info = viewInserter();
  info.uniqueName = "csm";
  info.kind = ViewKind::CSM_SHADOWS;
  info.maxDrawDistance = maxShadowDist;
  info.maxViewports = eastl::min({CSM_MAX_CASCADES, DAGDP_MAX_VIEWPORTS, maxCascadeCount});
}

void populate_csm_viewports(ViewPerFrameData &view_data, ecs::EntityManager &manager, int max_viewports)
{
  ShadowsManager &shadowsManager = WRDispatcher::getShadowsManager();
  auto csm = shadowsManager.getCascadeShadows();
  if (!csm)
    return;

  int maxCascadeCount = csm_max_cascade_count(manager, csm->getCameraFov());
  int numCascades = csm->getNumCascadesToRender();
  numCascades = eastl::min({numCascades, max_viewports, maxCascadeCount});
  if (numCascades <= 0)
    return;

  view_data.viewports.resize(numCascades);
  for (int ci = 0; ci < numCascades; ci++)
  {
    auto &viewport = view_data.viewports[ci];
    viewport.worldPos = csm->getRenderCameraWorldViewPos(ci);
    viewport.maxDrawDistance = csm->getMaxShadowDistance();
    viewport.frustum = csm->getFrustum(ci);
    viewport.csmCascade = ci;
  }
}

dafg::NodeHandle create_csm_shadows_provider(const dafg::NameSpace &ns, ecs::EntityManager &manager, int max_viewports)
{
  const auto declare = [max_viewports, &manager](dafg::Registry registry) {
    view_multiplex(registry, ViewKind::CSM_SHADOWS);
    auto viewDataHandle = registry.createBlob<ViewPerFrameData>("view@csm").handle();
    return [viewDataHandle, &manager, max_viewports] { populate_csm_viewports(viewDataHandle.ref(), manager, max_viewports); };
  };

  return ns.registerNode("csm_camera_provider", DAFG_PP_NODE_SRC, declare);
};

template <typename Callable>
static void shadows_query_manager_ecs_query(ecs::EntityManager &manager, Callable c);

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__shadowsQuality)
ECS_REQUIRE(const ecs::string &render_settings__shadowsQuality)
ECS_AFTER(init_shadows_es)
static void recreate_views_on_shadow_settings_change_es(const ecs::Event &, ecs::EntityManager &manager)
{
  shadows_query_manager_ecs_query(manager, [](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.destroyViews(); });
}

} // namespace dagdp
