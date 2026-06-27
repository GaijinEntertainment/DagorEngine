// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/ecsQuery.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityComponent.h>
#include <generic/dag_enumerate.h>
#include <debug/dag_debug3d.h>
#include <util/dag_convar.h>
#include <math/dag_hlsl_floatx.h>
#include <render/renderSettings.h>
#include "../globalManager.h"
#include "grid3d.h"

ECS_REGISTER_BOXED_TYPE(dagdp::Grid3dManager, nullptr);

namespace dagdp
{

#define BOUNDING_BOX_COLOR E3DCOLOR_MAKE(64, 64, 255, 255)
#define VOLUME_COLOR       E3DCOLOR_MAKE(255, 64, 255, 255)

static inline float pow2f(float x) { return x * x; }

template <typename Callable>
static inline void grid3d_settings_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void grid3d_placers_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void manager_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void volume_boxes_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void volume_cylinders_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void volume_spheres_ecs_query(ecs::EntityManager &manager, Callable);


ECS_NO_ORDER
static inline void grid3d_view_process_es(
  const dagdp::EventViewProcess &evt, ecs::EntityManager &manager, dagdp::Grid3dManager &dagdp__grid3d_manager)
{
  FRAMEMEM_REGION;

  const auto &rulesBuilder = evt.get<0>();
  // const auto &viewInfo = evt.get<1>();
  auto &builder = dagdp__grid3d_manager.currentBuilder;

  grid3d_placers_ecs_query(manager,
    [&](ECS_REQUIRE(ecs::Tag dagdp_placer_grid3d) ecs::EntityId eid, float dagdp__density, const ecs::string &dagdp__name,
      const Point3 &dagdp__volume_axis, bool dagdp__volume_axis_local, bool dagdp__volume_axis_abs,
      const Point2 &dagdp__distance_based_scale, float dagdp__distance_based_range, const Point3 &dagdp__distance_based_center,
      float dagdp__jitter, int dagdp__seed, int dagdp__csm_cascade_count = -1) {
      auto iter = rulesBuilder.placers.find(eid);
      if (iter == rulesBuilder.placers.end())
        return;

      auto &placer = iter->second;

      if (rulesBuilder.maxObjects == 0)
      {
        logerr("daGdp: %s is disabled, as the max. objects setting is 0", dagdp__name);
        return;
      }

      if (dagdp__density <= 0.0f)
      {
        logerr("daGdp: %s has invalid density", dagdp__name);
        return;
      }

      const uint32_t placerIndex = builder.placers.size();
      auto &placer3d = builder.placers.push_back();
      placer3d.density = dagdp__density;
      placer3d.worldStep = powf(dagdp__density, -1.0f / 3);
      placer3d.csmCascadeCount = dagdp__csm_cascade_count;
      placer3d.axis = dagdp__volume_axis;
      placer3d.axisLocal = dagdp__volume_axis_local;
      placer3d.axisAbs = dagdp__volume_axis_abs;
      placer3d.distBasedScale = dagdp__distance_based_scale;
      placer3d.distBasedRange = dagdp__distance_based_range;
      placer3d.distBasedCenter = dagdp__distance_based_center;
      placer3d.gridJitter = dagdp__jitter;
      placer3d.prngSeed = dagdp__seed;
      placer3d.csmCascadeCount = dagdp__csm_cascade_count;

      for (const auto objectGroupEid : placer.objectGroupEids)
      {
        auto iter = rulesBuilder.objectGroups.find(objectGroupEid);
        if (iter == rulesBuilder.objectGroups.end())
          continue;
        auto &group = placer3d.objectGroups.push_back();
        group.info = &iter->second;
        group.effectiveDensity = 1;
      }

      float maxDrawDistance = 0;
      for (const auto &objectGroup : placer3d.objectGroups)
        maxDrawDistance = eastl::max(maxDrawDistance, objectGroup.info->maxDrawDistance);
      placer3d.maxDrawDistance = maxDrawDistance;

      builder.mapping.insert({eid, placerIndex});
    });
}

static bbox3f calc_frustum_box(const Frustum &frustum, const Point3 &worldPos, vec4f range)
{
  bbox3f frustumBox;
  frustum.calcFrustumBBox(frustumBox);
  bbox3f rangeBox;
  v_bbox3_init_by_bsph(rangeBox, v_make_vec3f(worldPos.x, worldPos.y, worldPos.z), range);
  return v_bbox3_get_box_intersection(frustumBox, rangeBox);
}

void gather(dag::ConstSpan<Grid3dPlacer> placers,
  const Grid3dMapping &placer_mapping,
  const ViewInfo &view_info,
  uint32_t viewport_index,
  const Viewport &viewport,
  float max_bounding_radius,
  Grid3dVolumes &out_volumes,
  Grid3dBoxes &out_boxes)
{
  G_UNUSED(viewport_index);
  Frustum frustum = viewport.frustum;
  Point4 worldPos(viewport.worldPos.x, viewport.worldPos.y, viewport.worldPos.z, 0.0f);
  auto range = v_splats(min(view_info.maxDrawDistance, viewport.maxDrawDistance) * get_global_range_scale());
  shrink_frustum_zfar(frustum, v_ldu(&worldPos.x), range);

  bbox3f frustumBox;
  frustumBox = calc_frustum_box(frustum, viewport.worldPos, range);
  if (!v_test_xyz_finite(frustumBox.bmin) || !v_test_xyz_finite(frustumBox.bmax))
    return;

  const auto addVolume = [&](const ecs::EntityId dagdp_internal__volume_placer_eid, const TMatrix &transform, float scale,
                           int volume_type) {
    const auto iter = placer_mapping.find(dagdp_internal__volume_placer_eid);
    if (iter == placer_mapping.end())
      return;
    const auto placerIndex = iter->second;
    G_ASSERT(placerIndex < placers.size());
    const auto &placer = placers[placerIndex];

    if (placer.csmCascadeCount >= 0 && viewport.csmCascade >= placer.csmCascadeCount)
      return;

    // Reference: volumePlacerES.cpp.inl, VolumePlacer::updateVisibility
    mat44f tm44;
    v_mat44_make_from_43cu_unsafe(tm44, transform.array);

    mat44f itm44;
    v_mat44_inverse43(itm44, tm44);

    mat43f itm43;
    v_mat44_transpose_to_mat43(itm43, itm44);

    const vec4f extent2 = v_add(v_mul(v_add(v_abs(tm44.col0), v_add(v_abs(tm44.col1), v_abs(tm44.col2))), v_splats(2.0f * scale)),
      v_splats(2.0f * max_bounding_radius));
    const vec4f center2 = v_add(tm44.col3, tm44.col3);

    bbox3f volBox;
    v_bbox3_init_by_bsph(volBox, tm44.col3, v_mul(extent2, V_C_HALF));

    // cull by placeables draw range first
    bbox3f drawBox;
    v_bbox3_init_by_bsph(drawBox, v_make_vec3f(P3D(viewport.worldPos)), v_splats(placer.maxDrawDistance * get_global_range_scale()));
    volBox = v_bbox3_get_box_intersection(volBox, drawBox);
    if (v_bbox3_is_empty(volBox))
      return;

    // TODO: Performance: can cull more with more precise checks (original box/ellipsoid instead of bbox).
    if (!frustum.testBoxExtentB(center2, extent2))
      return;
    volBox = v_bbox3_get_box_intersection(volBox, frustumBox);
    if (v_bbox3_is_empty(volBox))
      return;

    debug_draw_volume(transform, scale, extent2, volume_type);

    G_ASSERT(scale > FLT_EPSILON);
    const vec4f invScale = v_splats(1.0f / scale);
    auto &volume = out_volumes.push_back();
    v_stu(&volume.itmRow0, v_mul(itm43.row0, invScale));
    v_stu(&volume.itmRow1, v_mul(itm43.row1, invScale));
    v_stu(&volume.itmRow2, v_mul(itm43.row2, invScale));
    Point3 axis = placer.axis;
    if (placer.axisLocal)
      axis = transform * axis;
    volume.axis = normalize(axis);
    volume.volumeType = volume_type;
    volume.variantIndex = placerIndex;
    out_boxes.emplace_back(volBox);
  };

  volume_boxes_ecs_query(*g_entity_mgr,
    [&](ECS_REQUIRE(ecs::Tag dagdp_volume_box) const ecs::EntityId dagdp_internal__volume_placer_eid, const TMatrix &transform) {
      addVolume(dagdp_internal__volume_placer_eid, transform, 0.5f, VOLUME_TYPE_BOX);
    });

  volume_cylinders_ecs_query(*g_entity_mgr,
    [&](ECS_REQUIRE(ecs::Tag dagdp_volume_cylinder) const ecs::EntityId dagdp_internal__volume_placer_eid, const TMatrix &transform) {
      addVolume(dagdp_internal__volume_placer_eid, transform, 0.5f, VOLUME_TYPE_CYLINDER);
    });

  volume_spheres_ecs_query(*g_entity_mgr,
    [&](ECS_REQUIRE(ecs::Tag dagdp_volume_sphere) const ecs::EntityId dagdp_internal__volume_placer_eid, const TMatrix &transform,
      float sphere_zone__radius) {
      if (sphere_zone__radius <= FLT_EPSILON)
        return;

      addVolume(dagdp_internal__volume_placer_eid, transform, sphere_zone__radius, VOLUME_TYPE_ELLIPSOID);
    });
}

ECS_NO_ORDER static inline void grid3d_view_finalize_es(const dagdp::EventViewFinalize &evt,
  dagdp::Grid3dManager &dagdp__grid3d_manager)
{
  const auto &viewInfo = evt.get<0>();
  const auto &viewBuilder = evt.get<1>();
  auto nodes = evt.get<2>();
  const auto &rulesBuilder = evt.get<3>();

  create_grid3d_nodes(viewInfo, viewBuilder, rulesBuilder, dagdp__grid3d_manager, nodes);

  // Cleanup.
  dagdp__grid3d_manager.currentBuilder = {}; // Reset the state.
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_ON_EVENT(on_disappear)
ECS_TRACK(dagdp__name,
  dagdp__density,
  dagdp__volume_axis,
  dagdp__volume_axis_local,
  dagdp__volume_axis_abs,
  dagdp__distance_based_scale,
  dagdp__distance_based_range,
  dagdp__distance_based_center,
  dagdp__jitter,
  dagdp__seed,
  dagdp__csm_cascade_count)
ECS_REQUIRE(ecs::Tag dagdp_placer_grid3d,
  const ecs::string &dagdp__name,
  float dagdp__density,
  const Point3 &dagdp__volume_axis,
  bool dagdp__volume_axis_local,
  bool dagdp__volume_axis_abs,
  const Point2 &dagdp__distance_based_scale,
  float dagdp__distance_based_range,
  const Point3 &dagdp__distance_based_center,
  float dagdp__jitter,
  int dagdp__seed,
  int dagdp__csm_cascade_count)
static void dagdp_placer_grid3d_changed_es(const ecs::Event &, ecs::EntityManager &manager)
{
  manager_ecs_query(manager, [](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.invalidateRules(); });
}

template <typename Callable>
static inline void volumes_link_ecs_query(ecs::EntityManager &manager, Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(dagdp__name)
ECS_REQUIRE(ecs::Tag dagdp_placer_grid3d)
static void dagdp_grid3d_volume_link_es(
  const ecs::Event &, ecs::EntityManager &manager, ecs::EntityId eid, const ecs::string &dagdp__name)
{
  volumes_link_ecs_query(manager, [&](const ecs::string &dagdp__volume_placer_name, ecs::EntityId &dagdp_internal__volume_placer_eid) {
    if (dagdp__name == dagdp__volume_placer_name)
      dagdp_internal__volume_placer_eid = eid;
  });
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag dagdp_placer_grid3d)
static void dagdp_grid3d_volume_unlink_es(const ecs::Event &, ecs::EntityManager &manager, const ecs::string &dagdp__name)
{
  volumes_link_ecs_query(manager, [&](const ecs::string &dagdp__volume_placer_name, ecs::EntityId &dagdp_internal__volume_placer_eid) {
    if (dagdp__name == dagdp__volume_placer_name)
      dagdp_internal__volume_placer_eid = ecs::INVALID_ENTITY_ID;
  });
}

// TODO: when dagdp__name changes, all volumes that were linked to the old name should ideally be unlinked.
// However, this is a dev-only scenario, and probably a rare one, so it's not implemented for now.

template <typename Callable>
static inline void volume_placers_link_ecs_query(ecs::EntityManager &manager, Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(dagdp__volume_placer_name)
ECS_REQUIRE(ecs::Tag dagdp_volume)
static void dagdp_grid3d_volume_changed_es(const ecs::Event &,
  ecs::EntityManager &manager,
  const ecs::string &dagdp__volume_placer_name,
  ecs::EntityId &dagdp_internal__volume_placer_eid)
{
  volume_placers_link_ecs_query(manager, [&](ecs::EntityId eid, const ecs::string &dagdp__name) {
    if (dagdp__name == dagdp__volume_placer_name)
      dagdp_internal__volume_placer_eid = eid;
  });
}

} // namespace dagdp
