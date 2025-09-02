// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <ecs/render/updateStageRender.h>

#include <render/world/animCharRenderAdditionalData.h>

#include <render/world/portalRenderer.h>
#include <shaders/portal_render.hlsli>


ECS_REGISTER_BOXED_TYPE(PortalRenderer, nullptr)


template <typename Callable>
static void get_portal_renderer_manager_ecs_query(ecs::EntityId, Callable);


PortalRenderer *portal_renderer_mgr::query_portal_renderer()
{
  PortalRenderer *result = nullptr;
  get_portal_renderer_manager_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("portal_renderer_manager")),
    [&](PortalRenderer &portal_renderer) { result = &portal_renderer; });

  // if (!result)
  //   logerr("PortalRenderer not found");

  return result;
}


static uint32_t pack_portal_cube_index(int portal_cube_index, bool is_bidirectional)
{
  G_ASSERT(portal_cube_index >= -1);
  const bool isActive = portal_cube_index >= 0;
  uint32_t packedPortalIndex = isActive ? (portal_cube_index & PORTAL_INDEX_MASK) : 0;
  packedPortalIndex |= (isActive ? PORTAL_FLAG_ACTIVE_MASK : 0);
  packedPortalIndex |= (is_bidirectional ? PORTAL_FLAG_MASK_BIDIRECTIONAL : 0);
  return packedPortalIndex;
}

ECS_TAG(render)
static void update_portal_data_es(const UpdateStageInfoBeforeRender &, int dynamic_portal__index, ecs::Point4List &additional_data)
{
  TMatrix portalTm = TMatrix::IDENT;
  float portalLife = 0;
  bool isBidirectional = false;
  int renderedPortalCubeIndex =
    portal_renderer_mgr::get_rendered_portal_cube_index(dynamic_portal__index, portalTm, portalLife, isBidirectional);

  portalTm.setcol(3, Point3::ZERO);
  float scaleY = portalTm.getcol(1).length(); // for portal floor scaling
  portalTm.orthonormalize();
  portalTm = orthonormalized_inverse(portalTm);

  Point4 extraData{bitwise_cast<float>(pack_portal_cube_index(renderedPortalCubeIndex, isBidirectional)), scaleY, portalLife, 0};

  int dataPos = animchar_additional_data::request_space<AAD_PORTAL_INDEX_TM>(additional_data, 1 + 3);
  additional_data[dataPos + 0] = extraData;
  additional_data[dataPos + 1] = Point4(portalTm.getcol(0).x, portalTm.getcol(1).x, portalTm.getcol(2).x, 0);
  additional_data[dataPos + 2] = Point4(portalTm.getcol(0).y, portalTm.getcol(1).y, portalTm.getcol(2).y, 0);
  additional_data[dataPos + 3] = Point4(portalTm.getcol(0).z, portalTm.getcol(1).z, portalTm.getcol(2).z, 0);
}
