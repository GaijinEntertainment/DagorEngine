// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/aimRender.h>

#include <daECS/core/componentTypes.h>
#include <daECS/core/dataComponent.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <ecs/render/shaderVar.h>
#include <ecs/anim/anim.h>
#include <render/daFrameGraph/daFG.h>

template <typename Callable>
inline static void query_aim_data_ecs_query(ecs::EntityManager &manager, Callable c);

template <typename Callable>
inline static void check_if_lens_renderer_enabled_globally_ecs_query(ecs::EntityManager &manager, Callable c);

static bool prepare_aim_render(AimRenderingData &aimData)
{
  bool gathered = false;
  query_aim_data_ecs_query(*g_entity_mgr,
    [&](ECS_REQUIRE(eastl::true_type camera__active) int aim_data__lensNodeId, int aim_data__lensCollisionNodeId,
      float aim_data__lensBoundingSphereRadius, bool aim_data__farDofEnabled, bool aim_data__lensRenderEnabled,
      const ecs::EntityId aim_data__entityWithScopeLensEid, bool aim_data__isAiming, int aim_data__crosshairNodeId = -1) {
      aimData.farDofEnabled = aim_data__farDofEnabled;
      aimData.lensRenderEnabled = aim_data__lensRenderEnabled && aim_data__lensNodeId >= 0;
      aimData.isAiming = aim_data__isAiming;
      aimData.entityWithScopeLensEid = aim_data__entityWithScopeLensEid;
      aimData.lensNodeId = aim_data__lensNodeId;
      aimData.lensCrosshairNodeId = aim_data__crosshairNodeId;
      aimData.lensCollisionNodeId = aim_data__lensCollisionNodeId;
      aimData.lensBoundingSphereRadius = aim_data__lensBoundingSphereRadius;
      gathered = true;
    });
  return gathered;
}

AimRenderingData get_aim_rendering_data()
{
  AimRenderingData r;
  prepare_aim_render(r);
  return r;
}

bool lens_renderer_enabled_globally()
{
  bool enabled = false;
  check_if_lens_renderer_enabled_globally_ecs_query(*g_entity_mgr,
    [&enabled](bool lens_renderer_enabled_globally) { enabled = enabled || lens_renderer_enabled_globally; });
  return enabled;
}

dafg::NodeHandle makeGenAimRenderingDataNode()
{
  return dafg::register_node("setup_aim_rendering_data", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto aimRenderDataHandle = registry.createBlob<AimRenderingData>("aim_render_data").handle();
    registry.multiplex(dafg::multiplexing::Mode::None);
    return [aimRenderDataHandle]() {
      auto &ard = aimRenderDataHandle.ref();
      if (!prepare_aim_render(ard))
        ard = AimRenderingData();
    };
  });
}

template <typename Callable>
static inline void get_scope_animchar_ecs_query(ecs::EntityManager &manager, ecs::EntityId, Callable);

const DynamicRenderableSceneInstance *get_scope_lens(const AimRenderingData &aimData)
{
  if (!aimData.entityWithScopeLensEid)
    return nullptr;

  const AnimV20::AnimcharRendComponent *lensAnimcharRender = nullptr;

  get_scope_animchar_ecs_query(*g_entity_mgr, aimData.entityWithScopeLensEid,
    [&](const AnimV20::AnimcharRendComponent &animchar_render) { lensAnimcharRender = &animchar_render; });

  if (!lensAnimcharRender)
    return nullptr;

  return lensAnimcharRender->getSceneInstance();
}
