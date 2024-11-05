// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splineGenGeometry.h"
#include "splineGenGeometryRepository.h"
#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityId.h>
#include <daECS/core/entityComponent.h>
#include <memory/dag_framemem.h>
#include <ecs/render/updateStageRender.h>
#include <drv/3d/dag_resetDevice.h>
#include <frustumCulling/frustumPlanes.h>
#include "splineInterpolation.h"
#include <math/dag_hlsl_floatx.h>
#include "../shaders/spline_gen_buffer.hlsli"

template <typename Callable>
void register_to_lods_ecs_query(Callable c);

bool SplineGenGeometry::onLoaded(ecs::EntityManager &mgr, ecs::EntityId eid)
{
  const ecs::string *lodsNamePtr = mgr.getNullable<ecs::string>(eid, ECS_HASH("spline_gen_geometry__lods_name"));
  G_ASSERT_RETURN(lodsNamePtr != nullptr, false);
  G_ASSERT_RETURN(*lodsNamePtr != "", false);
  ecs::EntityId &lodsEid = mgr.getRW<ecs::EntityId>(eid, ECS_HASH("spline_gen_geometry__lods_eid"));
  lodsEid = ecs::INVALID_ENTITY_ID;
  register_to_lods_ecs_query([&lodsNamePtr, &lodsEid](const ecs::string &spline_gen_lods__lods_name, ecs::EntityId eid) {
    if (*lodsNamePtr != spline_gen_lods__lods_name)
      return;
    lodsEid = eid;
  });
  G_ASSERT_RETURN(lodsEid, false);
  return true;
}

template <typename Callable>
void get_lod_data_ecs_query(ecs::EntityId, Callable);

ECS_TAG(render)
ECS_BEFORE(spline_gen_geometry_manager_update_buffers_es)
static void spline_gen_geometry_select_lod_es(const UpdateStageInfoBeforeRender &stg,
  SplineGenGeometry &spline_gen_geometry_renderer,
  ecs::EntityId spline_gen_geometry__lods_eid,
  int &spline_gen_geometry__lod,
  bool &spline_gen_geometry__is_rendered,
  const Point3 &spline_gen_geometry__radius,
  const ecs::List<Point3> &spline_gen_geometry__points)
{
  get_lod_data_ecs_query(spline_gen_geometry__lods_eid,
    [&](const ecs::FloatList &spline_gen_lods__distances, const ecs::StringList &spline_gen_lods__template_names) {
      constexpr int pointsSampleCount = 5;
      int lastIndex = spline_gen_geometry__points.size() - 1;
      BBox3 bbox;
      for (int i = 0; i < pointsSampleCount; i++)
        bbox += spline_gen_geometry__points[lastIndex * i / (pointsSampleCount - 1)];
      float distanceFromCamera = length(bbox.center() - stg.camPos) - length(bbox.width()) * 0.5f -
                                 max(spline_gen_geometry__radius.x, max(spline_gen_geometry__radius.y, spline_gen_geometry__radius.z));

      const ecs::FloatList &distances = spline_gen_lods__distances;
      int lod = 0;
      for (; lod < distances.size(); lod++)
      {
        if (distanceFromCamera <= distances[lod])
          break;
      }

      if (spline_gen_geometry__lod != lod)
      {
        const ecs::StringList &templateNames = spline_gen_lods__template_names;
        G_ASSERT(templateNames.size() == distances.size());
        spline_gen_geometry_renderer.setLod(lod < templateNames.size() ? templateNames[lod] : ecs::string());
        spline_gen_geometry__lod = lod;
        spline_gen_geometry__is_rendered = spline_gen_geometry_renderer.isRendered();
      }
    });
}

ECS_TAG(render)
ECS_BEFORE(spline_gen_geometry_update_instancing_data_es)
static void spline_gen_geometry_manager_update_buffers_es(const UpdateStageInfoBeforeRender & /*stg*/,
  SplineGenGeometryRepository &spline_gen_repository)
{
  for (auto &assetPair : spline_gen_repository.getAssets())
    assetPair.second->updateBuffers();
  for (auto &managerPair : spline_gen_repository.getManagers())
    managerPair.second->updateBuffers();
}

ECS_TAG(render)
ECS_AFTER(spline_gen_geometry_select_lod_es)
ECS_REQUIRE(eastl::true_type spline_gen_geometry__is_rendered)
static void spline_gen_geometry_request_active_state_es(const UpdateStageInfoBeforeRender & /*stg*/,
  SplineGenGeometry &spline_gen_geometry_renderer,
  bool spline_gen_geometry__request_active,
  bool &spline_gen_geometry__renderer_active)
{
  spline_gen_geometry_renderer.requestActiveState(spline_gen_geometry__request_active);
  spline_gen_geometry__renderer_active = spline_gen_geometry_renderer.isActive();
}

ECS_TAG(render)
ECS_AFTER(spline_gen_geometry_request_active_state_es)
ECS_REQUIRE(eastl::true_type spline_gen_geometry__is_rendered)
ECS_REQUIRE(eastl::true_type spline_gen_geometry__renderer_active)
static void spline_gen_geometry_update_instancing_data_es(const UpdateStageInfoBeforeRender & /*stg*/,
  SplineGenGeometry &spline_gen_geometry_renderer,
  Point3 &spline_gen_geometry__radius,
  float spline_gen_geometry__displacement_strength,
  int spline_gen_geometry__tiles_around,
  float spline_gen_geometry__tile_size_meters,
  const ecs::List<Point3> &spline_gen_geometry__points,
  float spline_gen_geometry__obj_size_mul,
  float spline_gen_geometry__meter_between_objs)
{
  int stripes = spline_gen_geometry_renderer.getManager().stripes;
  if (spline_gen_geometry__radius.y <= 0.0f)
    spline_gen_geometry__radius.y = lerp(spline_gen_geometry__radius.x, spline_gen_geometry__radius.z, (stripes - 1.0f) / stripes);
  G_ASSERT(spline_gen_geometry__radius.x > 0);
  G_ASSERT(spline_gen_geometry__radius.z >= 0);
  G_ASSERT(abs(spline_gen_geometry__displacement_strength) < 1);
  G_ASSERT(spline_gen_geometry__tiles_around >= 1);
  G_ASSERT(spline_gen_geometry__tile_size_meters > 0);
  G_ASSERT(spline_gen_geometry__points.size() >= 2);

  eastl::vector<SplineGenSpline, framemem_allocator> splineVec = interpolate_points(spline_gen_geometry__points, stripes);

  spline_gen_geometry_renderer.updateInstancingData(splineVec, spline_gen_geometry__radius, spline_gen_geometry__displacement_strength,
    spline_gen_geometry__tiles_around, spline_gen_geometry__tile_size_meters, spline_gen_geometry__obj_size_mul,
    spline_gen_geometry__meter_between_objs);
}

ECS_TAG(render)
ECS_AFTER(spline_gen_geometry_update_instancing_data_es)
ECS_REQUIRE(eastl::true_type spline_gen_geometry__is_rendered)
ECS_REQUIRE(eastl::false_type spline_gen_geometry__renderer_active)
static void spline_gen_geometry_update_inactive_es(const UpdateStageInfoBeforeRender & /*stg*/,
  SplineGenGeometry &spline_gen_geometry_renderer)
{
  spline_gen_geometry_renderer.updateInactive();
}

ECS_TAG(render)
ECS_AFTER(spline_gen_geometry_update_inactive_es)
static void spline_gen_geometry_manager_generate_es(const UpdateStageInfoBeforeRender & /*stg*/,
  SplineGenGeometryRepository &spline_gen_repository)
{
  for (auto &managerPair : spline_gen_repository.getManagers())
  {
    managerPair.second->generate();
  }
}

ECS_TAG(render)
static void spline_gen_geometry_render_opaque_es(const UpdateStageInfoRender &stg, SplineGenGeometryRepository &spline_gen_repository)
{
  bool renderColor = stg.hints & UpdateStageInfoRender::RENDER_COLOR;
  set_frustum_planes(stg.cullingFrustum);
  for (auto &managerPair : spline_gen_repository.getManagers())
  {
    managerPair.second->render(renderColor);
  }
}

ECS_REGISTER_BOXED_TYPE(SplineGenGeometry, nullptr);
ECS_AUTO_REGISTER_COMPONENT(SplineGenGeometry, "spline_gen_geometry_renderer", nullptr, 0);

template <typename Callable>
static void reset_spline_gen_geometry_renderer_ecs_query(Callable c);

template <typename Callable>
static void reset_spline_gen_repository_ecs_query(Callable c);

static void spline_gen_geometry_before_reset(bool)
{
  reset_spline_gen_geometry_renderer_ecs_query([](SplineGenGeometry &spline_gen_geometry_renderer, int &spline_gen_geometry__lod) {
    spline_gen_geometry_renderer.reset();
    spline_gen_geometry__lod = -1;
  });
  reset_spline_gen_repository_ecs_query([](SplineGenGeometryRepository &spline_gen_repository) { spline_gen_repository.reset(); });
}

REGISTER_D3D_BEFORE_RESET_FUNC(spline_gen_geometry_before_reset);
