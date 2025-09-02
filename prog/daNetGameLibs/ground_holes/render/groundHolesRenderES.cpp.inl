// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/render/shaders.h>
#include <ecs/render/postfx_renderer.h>
#include <ecs/render/resPtr.h>
#include <render/renderEvent.h>
#include <EASTL/unique_ptr.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_resPtr.h>
#include <EASTL/vector.h>
#include <math/dag_Point4.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <render/dasModules/worldRenderer.h>
#include <heightmap/heightmapHandler.h>
#include <landMesh/lmeshManager.h>
#include <landMesh/heightmap_holes_zones.hlsli>
#include <util/dag_convar.h>
#include "render/boundsUtils.h"
#include "main/level.h"
#include <groundHolesCore/helpers.h>

#include <shaders/dag_computeShaders.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <ecs/render/compute_shader.h>

#include <groundHolesCore/render/groundHolesCoreRender.h>

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static inline void ground_holes_initialize_es(const ecs::Event &,
  int &hmap_holes_scale_step_offset_varId,
  int &hmap_holes_temp_ofs_size_varId,
  bool &should_render_ground_holes,
  ecs::Point4List &holes)
{
  ground_holes::holes_initialize(hmap_holes_scale_step_offset_varId, hmap_holes_temp_ofs_size_varId, should_render_ground_holes,
    holes);
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static inline void ground_holes_on_disappear_es(
  const ecs::Event &, int hmap_holes_scale_step_offset_varId, UniqueTexHolder &hmapHolesTex)
{
  ground_holes::on_disappear(hmap_holes_scale_step_offset_varId, hmapHolesTex);
}

ECS_TAG(render)
ECS_NO_ORDER
static void ground_hole_render_es(const UpdateStageInfoRender &,
  UniqueTexHolder &hmapHolesTex,
  UniqueTexHolder &hmapHolesTmpTex,
  UniqueBufHolder &hmapHolesBuf,
  PostFxRenderer &hmapHolesProcessRenderer,
  PostFxRenderer &hmapHolesMipmapRenderer,
  ShadersECS &hmapHolesPrepareRenderer,
  bool &should_render_ground_holes,
  int hmap_holes_scale_step_offset_varId,
  int hmap_holes_temp_ofs_size_varId,
  ecs::Point4List &holes,
  ecs::Point3List &invalidate_bboxes,
  const ComputeShader &heightmap_holes_process_cs)
{
  if (!should_render_ground_holes)
    return;
  if (!getLmeshMgr() || !getLmeshMgr()->getHolesManager() || getLmeshMgr()->getHolesManager()->getBbox().isempty())
    return;

  ground_holes::render(hmapHolesTex, hmapHolesTmpTex, hmapHolesBuf, hmapHolesProcessRenderer, hmapHolesMipmapRenderer,
    hmapHolesPrepareRenderer, should_render_ground_holes, hmap_holes_scale_step_offset_varId, hmap_holes_temp_ofs_size_varId, holes,
    invalidate_bboxes, heightmap_holes_process_cs);

  G_ASSERT_RETURN(invalidate_bboxes.size() % 2 == 0, );
  for (int i = 0; i < invalidate_bboxes.size() / 2; ++i)
    invalidate_after_heightmap_change(BBox3(invalidate_bboxes[i * 2], invalidate_bboxes[i * 2 + 1]));
}

template <typename Callable>
static void spawn_hole_ecs_query(Callable c);

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
void ground_holes_before_render_es(
  const UpdateStageInfoBeforeRender &, ecs::Point4List &holes, ecs::Point3List &invalidate_bboxes, bool should_render_ground_holes)
{
  // A `ground_hole` can initialize after `EventLevelLoaded` event.
  // TODO: Render hmapHolesTex only once.
  if (!should_render_ground_holes)
    return;

  holes.clear();
  invalidate_bboxes.clear();
  if (!ground_holes::get_debug_hide())
  {
    spawn_hole_ecs_query([&](const TMatrix &transform, bool ground_hole_sphere_shape, bool ground_hole_shape_intersection) {
      ground_holes::spawn_hole(holes, transform, ground_hole_sphere_shape, ground_hole_shape_intersection);
      ground_holes::get_invalidation_bbox(invalidate_bboxes, transform, ground_hole_shape_intersection);
    });
  }
}

ECS_TAG(render, dev)
ECS_BEFORE(ground_holes_before_render_es)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
void ground_holes_convar_helper_es(const UpdateStageInfoBeforeRender &, bool &should_render_ground_holes)
{
  ground_holes::convar_helper(should_render_ground_holes);
}

ECS_TAG(render)
ECS_ON_EVENT(EventLevelLoaded, AfterDeviceReset)
static void ground_holes_render_when_event_es(const ecs::Event &, bool &should_render_ground_holes)
{
  should_render_ground_holes = true;
}

void get_underground_zones_data(Tab<Point3_vec4> &bboxes);

template <typename Callable>
static void get_underground_zones_buf_ecs_query(Callable c);

ECS_TAG(render)
ECS_REQUIRE(ecs::Tag underground_zone)
ECS_ON_EVENT(on_appear, on_disappear)
static void ground_hole_zone_on_appear_es(const ecs::Event &)
{
  get_underground_zones_buf_ecs_query([&](bool &should_update_ground_holes_zones) { should_update_ground_holes_zones = true; });
}

ECS_TAG(render)
ECS_NO_ORDER
static void ground_holes_zones_before_render_es(
  const UpdateStageInfoBeforeRender &, UniqueBufHolder &hmapHolesZonesBuf, bool &should_update_ground_holes_zones)
{
  Tab<Point3_vec4> bboxes(framemem_ptr());
  get_underground_zones_data(bboxes);
  ground_holes::zones_before_render(hmapHolesZonesBuf, should_update_ground_holes_zones, bboxes);
}

ECS_TAG(render)
ECS_ON_EVENT(AfterDeviceReset)
static void ground_holes_zones_after_device_reset_es(
  const ecs::Event &, UniqueBufHolder &hmapHolesZonesBuf, bool &should_update_ground_holes_zones)
{
  ground_holes::zones_after_device_reset(hmapHolesZonesBuf, should_update_ground_holes_zones);
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static void ground_holes_zones_manager_on_disappear_es(const ecs::Event &, UniqueBufHolder &hmapHolesZonesBuf)
{
  ground_holes::zones_manager_on_disappear(hmapHolesZonesBuf);
}
