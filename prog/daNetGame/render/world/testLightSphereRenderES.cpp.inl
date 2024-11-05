// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <perfMon/dag_statDrv.h>
#include <ecs/render/updateStageRender.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_shaders.h>
#include <render/spheres_consts.hlsli>
#include "camera/sceneCam.h"
#include <math/dag_TMatrix.h>
#include <debug/dag_logSys.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <daECS/core/coreEvents.h>

static uint32_t spheresCounter = 0;
static DynamicShaderHelper spheresRenderer;
static int debugGbufferSpherePosId = -1;
static int debugGbufferSphereColorId = -1;
static int debugGbufferSphereReflectanceId = -1;
static int debugGbufferSphereMetallnessId = -1;
static int debugGbufferSphereSmoothnessId = -1;


ECS_REQUIRE(ecs::Tag IsDebugGbufferSphereTag)
ECS_TAG(dev, render)
static __forceinline void test_light_sphere_render_es(const UpdateStageInfoRender &,
  const TMatrix &transform,
  float size,
  float metallness,
  float reflectance,
  float smoothness,
  const Point3 &albedo)
{
  TIME_D3D_PROFILE(debug_gbuffer_sphere);
  spheresRenderer.shader->setStates(0, true);
  ShaderGlobal::set_color4(debugGbufferSpherePosId, transform[3][0], transform[3][1], transform[3][2], size);
  ShaderGlobal::set_color4(debugGbufferSphereColorId, albedo.x, albedo.y, albedo.z, 0);
  ShaderGlobal::set_real(debugGbufferSphereReflectanceId, reflectance);
  ShaderGlobal::set_real(debugGbufferSphereMetallnessId, metallness);
  ShaderGlobal::set_real(debugGbufferSphereSmoothnessId, smoothness);

  d3d::draw_instanced(PRIM_TRILIST, 0, SPHERES_INDICES_TO_DRAW, 1);
}

template <typename Callable>
void current_camera_ecs_query(ecs::EntityId, Callable);

ECS_REQUIRE(ecs::Tag IsDebugGbufferSphereTag)
ECS_REQUIRE(eastl::true_type isFixedToCamera)
ECS_TAG(dev, render)
static __forceinline void operate_camera_bounded_sphere_es(
  const UpdateStageInfoBeforeRender &, TMatrix &transform, Point3 cameraOffset)
{
  TMatrix &sphereTransform = transform;
  current_camera_ecs_query(get_cur_cam_entity(), [&](const TMatrix &transform) {
    Point3 transformedOffset = transform * cameraOffset;
    sphereTransform.setcol(3, transformedOffset);
  });
}

ECS_REQUIRE(ecs::Tag IsDebugGbufferSphereTag)
ECS_TAG(dev, render)
ECS_ON_EVENT(on_appear)
void new_sphere_created_es_event_handler(const ecs::Event &)
{
  if (spheresCounter == 0)
  {
    spheresRenderer.init("debug_gbuffer_sphere", nullptr, 0, "debug_gbuffer_sphere", true);

    debugGbufferSpherePosId = get_shader_variable_id("debug_gbuffer_sphere_pos_size");
    debugGbufferSphereColorId = get_shader_variable_id("debug_gbuffer_sphere_albedo");
    debugGbufferSphereReflectanceId = get_shader_variable_id("debug_gbuffer_sphere_reflectance", true);
    debugGbufferSphereMetallnessId = get_shader_variable_id("debug_gbuffer_sphere_metallness", true);
    debugGbufferSphereSmoothnessId = get_shader_variable_id("debug_gbuffer_sphere_smoothness", true);
  }
  spheresCounter++;
}

ECS_NO_ORDER
ECS_REQUIRE(ecs::Tag IsDebugGbufferSphereTag)
ECS_REQUIRE(eastl::true_type triggerDeleteSphere)
ECS_TAG(dev, render)
static __forceinline void delete_sphere_es(const ecs::UpdateStageInfoAct &, ecs::EntityId eid) { g_entity_mgr->destroyEntity(eid); }

ECS_REQUIRE(ecs::Tag IsDebugGbufferSphereTag)
ECS_TAG(dev, render)
ECS_ON_EVENT(on_disappear)
void sphere_deleted_es_event_handler(const ecs::Event &)
{
  spheresCounter--;
  if (spheresCounter == 0)
  {
    spheresRenderer.close();
  }
}