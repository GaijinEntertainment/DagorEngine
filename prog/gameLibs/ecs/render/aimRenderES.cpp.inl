// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/render/aimRender.h>

#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <ecs/anim/anim.h>
#include <math/dag_mathUtils.h>
#include <render/cameraParams.h>
#include <render/daFrameGraph/daFG.h>
#include <render/viewportEllipse_api.h>
#include <gameRes/dag_collisionResource.h>
#include <shaders/dag_dynSceneRes.h>

template <typename Callable>
inline static void query_aim_data_ecs_query(ecs::EntityManager &manager, Callable c);

template <typename Callable>
static inline void get_scope_animchar_ecs_query(ecs::EntityManager &manager, ecs::EntityId, Callable);

static bool prepare_aim_render(AimRenderingData &aimData)
{
  bool gathered = false;
  query_aim_data_ecs_query(*g_entity_mgr,
    [&](ECS_REQUIRE(eastl::true_type camera__active) int aim_data__lensNodeId, int aim_data__lensCollisionNodeId,
      float aim_data__lensBoundingSphereRadius, bool aim_data__farDofEnabled, bool aim_data__lensRenderEnabled,
      const ecs::EntityId aim_data__entityWithScopeLensEid, bool aim_data__isAiming, float aim_data__scopeLength = 0.0f,
      float aim_data__aimingTime = 0.0f) {
      aimData.farDofEnabled = aim_data__farDofEnabled;
      aimData.lensRenderEnabled = aim_data__lensRenderEnabled && aim_data__lensNodeId >= 0;
      aimData.isAiming = aim_data__isAiming;
      aimData.entityWithScopeLensEid = aim_data__entityWithScopeLensEid;
      aimData.lensNodeId = aim_data__lensNodeId;
      aimData.lensCollisionNodeId = aim_data__lensCollisionNodeId;
      aimData.lensBoundingSphereRadius = aim_data__lensBoundingSphereRadius;
      aimData.scopeLength = aim_data__scopeLength;
      aimData.aimingTime = saturate(aim_data__aimingTime);
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

const DynamicRenderableSceneInstance *get_scope_lens(const AimRenderingData &aim_data)
{
  if (!aim_data.entityWithScopeLensEid)
    return nullptr;

  const AnimV20::AnimcharRendComponent *lensAnimcharRender = nullptr;

  get_scope_animchar_ecs_query(*g_entity_mgr, aim_data.entityWithScopeLensEid,
    [&](const AnimV20::AnimcharRendComponent &animchar_render) { lensAnimcharRender = &animchar_render; });

  if (!lensAnimcharRender)
    return nullptr;

  return lensAnimcharRender->getSceneInstance();
}

float get_scope_lens_bs_radius(const CollisionResource *collres, const int node_id)
{
  if (collres && node_id >= 0)
  {
    const Point3 w = collres->getNodeBBox(node_id).width();
    return 0.5f * max(w.x, max(w.y, w.z)) / collres->getNodeMaxTmScale(node_id);
  }
  return 1.0f;
}

float get_scope_length(const CollisionResource *collres)
{
  if (!collres || collres->boundingBox.isempty())
    return 0.0f;
  const Point3 w = collres->boundingBox.width();
  return max(w.x, max(w.y, w.z));
}

dafg::NodeHandle makeScopeLensEllipseNode()
{
  return dafg::register_node("scope_lens_ellipse_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto cockpitCameraHndl = registry.readBlob<CameraParams>("current_cockpit_camera").handle();
    auto ellipseHndl = registry.createBlob<ViewportEllipse>(SCOPE_LENS_ELLIPSE_BLOB).withHistory().handle();
    return [cockpitCameraHndl, ellipseHndl]() {
      const AimRenderingData aimData = get_aim_rendering_data();
      const DynamicRenderableSceneInstance *scene = aimData.lensNodeId >= 0 ? get_scope_lens(aimData) : nullptr;
      if (!scene)
      {
        ellipseHndl.ref() = ViewportEllipse{};
        return;
      }
      const CameraParams &cockpitCamera = cockpitCameraHndl.ref();
      const float viewportScale = cockpitCamera.noJitterPersp.wk / cockpitCamera.noJitterPersp.hk;
      ellipseHndl.ref() = calc_viewport_ellipse(aimData.lensBoundingSphereRadius, scene->getNodeWtm(aimData.lensNodeId),
        cockpitCamera.viewRotJitterProjTm, viewportScale);
    };
  });
}
