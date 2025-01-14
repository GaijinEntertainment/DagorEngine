// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_enumerate.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <daECS/core/coreEvents.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <shaders/dag_rendInstRes.h>
#include "../riexProcessor.h"
#include "../globalManager.h"
#include "../../shaders/dagdp_heightmap.hlsli"
#include "volume.h"

constexpr int RI_FIXED_LOD = 2;
constexpr float MIN_GEOMETRY_SIZE = 1.0f;

ECS_REGISTER_BOXED_TYPE(dagdp::VolumeManager, nullptr);

namespace material_var
{
static ShaderVariableInfo is_rendinst_clipmap("is_rendinst_clipmap");
static ShaderVariableInfo enable_hmap_blend("enable_hmap_blend");
} // namespace material_var

namespace dagdp
{

template <typename Callable>
static inline void volume_placers_ecs_query(Callable);

template <typename Callable>
static inline void volume_boxes_ecs_query(Callable);

template <typename Callable>
static inline void volume_cylinders_ecs_query(Callable);

template <typename Callable>
static inline void volume_spheres_ecs_query(Callable);

ECS_NO_ORDER
static inline void volume_view_process_es(const dagdp::EventViewProcess &evt, dagdp::VolumeManager &dagdp__volume_manager)
{
  const auto &rulesBuilder = evt.get<0>();
  auto &builder = dagdp__volume_manager.currentBuilder;
  auto &viewBuilder = *evt.get<2>();

  volume_placers_ecs_query([&](ECS_REQUIRE(ecs::Tag dagdp_placer_volume) ecs::EntityId eid, float dagdp__density,
                             float dagdp__volume_min_triangle_area, const ecs::string &dagdp__name) {
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

    VolumeMappingItem item;
    item.variantIndex = builder.variants.size();
    item.density = dagdp__density;

    builder.mapping.insert({eid, item});
    auto &variant = builder.variants.push_back();
    variant.density = dagdp__density;
    variant.minTriangleArea = dagdp__volume_min_triangle_area;

    for (const auto objectGroupEid : placer.objectGroupEids)
    {
      auto iter = rulesBuilder.objectGroups.find(objectGroupEid);
      if (iter == rulesBuilder.objectGroups.end())
        continue;

      auto &objectGroup = variant.objectGroups.push_back();
      objectGroup.effectiveDensity = dagdp__density;
      objectGroup.info = &iter->second;
    }

    viewBuilder.hasDynamicPlacers |= !variant.objectGroups.empty();
  });
}

ECS_NO_ORDER static inline void volume_view_finalize_es(const dagdp::EventViewFinalize &evt,
  dagdp::VolumeManager &dagdp__volume_manager)
{
  const auto &viewInfo = evt.get<0>();
  const auto &viewBuilder = evt.get<1>();
  auto nodes = evt.get<2>();

  create_volume_nodes(viewInfo, viewBuilder, dagdp__volume_manager, nodes);

  dagdp__volume_manager.currentBuilder = {};
}

void gather(const VolumeMapping &volume_mapping,
  const ViewInfo &viewInfo,
  const Viewport &viewport,
  float max_bounding_radius,
  RiexProcessor &riex_processor,
  RelevantMeshes &out_result,
  RelevantTiles &out_tiles,
  RelevantVolumes &out_volumes)
{
  const auto addVolume = [&](const ecs::EntityId dagdp_internal__volume_placer_eid, const TMatrix &transform, float scale,
                           int volume_type) {
    const auto iter = volume_mapping.find(dagdp_internal__volume_placer_eid);
    if (iter == volume_mapping.end())
      return;
    const auto &variant = iter->second;

    // Reference: volumePlacerES.cpp.inl, VolumePlacer::updateVisibility
    mat44f tm44;
    v_mat44_make_from_43cu_unsafe(tm44, transform.array);

    mat44f itm44;
    v_mat44_inverse43(itm44, tm44);

    mat43f itm43;
    v_mat44_transpose_to_mat43(itm43, itm44);

    const vec4f extent2 = v_add(v_add(v_abs(tm44.col0), v_add(v_abs(tm44.col1), v_abs(tm44.col2))), v_splats(max_bounding_radius));
    const vec4f center2 = v_add(tm44.col3, tm44.col3);

    Frustum frustum = viewport.frustum;
    Point4 worldPos(viewport.worldPos.x, viewport.worldPos.y, viewport.worldPos.z, 0.0f);
    shrink_frustum_zfar(frustum, v_ldu(&worldPos.x), v_splats(min(viewInfo.maxDrawDistance, viewport.maxDrawDistance)));
    // TODO: Performance: can cull more with more precise checks (original box/ellipsoid instead of bbox).
    const bool isIntersecting = frustum.testBoxExtentB(center2, extent2);

    if (!isIntersecting)
      return;

    G_ASSERT(scale > FLT_EPSILON);
    const vec4f invScale = v_splats(1.0f / scale);
    const uint32_t volumeIndex = out_volumes.size();
    auto &volume = out_volumes.push_back();
    v_stu(&volume.itmRow0, v_mul(itm43.row0, invScale));
    v_stu(&volume.itmRow1, v_mul(itm43.row1, invScale));
    v_stu(&volume.itmRow2, v_mul(itm43.row2, invScale));
    volume.volumeType = volume_type;
    volume.variantIndex = variant.variantIndex;

    // Reference: volumePlacerES.cpp.inl, VolumePlacer::gatherGeometryInBox
    bbox3f bbox;
    v_bbox3_init(bbox, tm44, {v_splats(-scale), v_splats(scale)});
    rendinst::riex_collidable_t out_handles;
    rendinst::gatherRIGenExtraCollidableMin(out_handles, bbox, MIN_GEOMETRY_SIZE);

    const float instancePosDelta = 1.0f / sqrtf(variant.density);
    const float tileWorldSize = TILE_INSTANCE_COUNT_1D * instancePosDelta;

    int2 minPos;
    int2 maxPos;
    minPos.x = static_cast<int>(floorf(v_extract_x(bbox.bmin) / tileWorldSize));
    minPos.y = static_cast<int>(floorf(v_extract_z(bbox.bmin) / tileWorldSize));
    maxPos.x = static_cast<int>(ceilf(v_extract_x(bbox.bmax) / tileWorldSize));
    maxPos.y = static_cast<int>(ceilf(v_extract_z(bbox.bmax) / tileWorldSize));

    for (int x = minPos.x; x < maxPos.x; ++x)
      for (int z = minPos.y; z < maxPos.y; ++z)
      {
        auto &tile = out_tiles.push_back();
        tile.intPosXZ.x = x;
        tile.intPosXZ.y = z;
        tile.instancePosDelta = instancePosDelta;
        tile.volumeIndex = volumeIndex;
      }

    for (const auto handle : out_handles)
    {
      const uint32_t resIndex = rendinst::handle_to_ri_type(handle);
      const RenderableInstanceLodsResource *riLodsRes = rendinst::getRIGenExtraRes(resIndex);
      const int bestLod = riLodsRes->getQlBestLod();
      const int actualLod = min<int>(RI_FIXED_LOD, riLodsRes->lods.size() - 1);
      if (bestLod > actualLod)
        continue;

      const mat43f &instTm = rendinst::getRIGenExtra43(handle);
      RenderableInstanceResource *riRes = riLodsRes->lods[actualLod].scene;
      const auto *state = riex_processor.ask(riRes);

      if (state)
      {
        const dag::ConstSpan<ShaderMesh::RElem> elems = riRes->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque);

        G_ASSERT(elems.size() == state->areasIndices.size());
        for (auto [i, elem] : enumerate(elems))
        {
          int isRendinstClipmap = 0;
          int isHeightmapBlended = 0;

          if (elem.mat->getIntVariable(material_var::is_rendinst_clipmap.get_var_id(), isRendinstClipmap) && isRendinstClipmap ||
              elem.mat->getIntVariable(material_var::enable_hmap_blend.get_var_id(), isHeightmapBlended) && isHeightmapBlended)
          {
            // Skip materials that are rendered into the clipmap. See RiExtraPool::isRendinstClipmap.
            continue;
          }

          auto &item = out_result.push_back();
          item.startIndex = elem.si;
          item.numFaces = elem.numf;
          item.baseVertex = elem.baseVertex;
          item.stride = elem.vertexData->getStride();
          v_stu(&item.meshTmRow0.x, instTm.row0);
          v_stu(&item.meshTmRow1.x, instTm.row1);
          v_stu(&item.meshTmRow2.x, instTm.row2);
          item.areasIndex = state->areasIndices[i];
          item.vbIndex = elem.vertexData->getVbIdx();
          item.volumeIndex = volumeIndex;
          item.uniqueId = rendinst::handle_to_ri_inst(handle);
        }
      }
    }
  };

  volume_boxes_ecs_query(
    [&](ECS_REQUIRE(ecs::Tag dagdp_volume_box) const ecs::EntityId dagdp_internal__volume_placer_eid, const TMatrix &transform) {
      addVolume(dagdp_internal__volume_placer_eid, transform, 0.5f, VOLUME_TYPE_BOX);
    });

  volume_cylinders_ecs_query(
    [&](ECS_REQUIRE(ecs::Tag dagdp_volume_cylinder) const ecs::EntityId dagdp_internal__volume_placer_eid, const TMatrix &transform) {
      addVolume(dagdp_internal__volume_placer_eid, transform, 0.5f, VOLUME_TYPE_CYLINDER);
    });

  volume_spheres_ecs_query([&](ECS_REQUIRE(ecs::Tag dagdp_volume_sphere) const ecs::EntityId dagdp_internal__volume_placer_eid,
                             const TMatrix &transform, float sphere_zone__radius) {
    if (sphere_zone__radius <= FLT_EPSILON)
      return;

    addVolume(dagdp_internal__volume_placer_eid, transform, sphere_zone__radius, VOLUME_TYPE_ELLIPSOID);
  });
}

template <typename Callable>
static inline void manager_ecs_query(Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_ON_EVENT(on_disappear)
ECS_TRACK(dagdp__name, dagdp__density, dagdp__volume_min_triangle_area)
ECS_REQUIRE(ecs::Tag dagdp_placer_volume, const ecs::string &dagdp__name, float dagdp__density, float dagdp__volume_min_triangle_area)
static void dagdp_placer_volume_changed_es(const ecs::Event &)
{
  manager_ecs_query([](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.invalidateRules(); });
}

template <typename Callable>
static inline void volumes_link_ecs_query(Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(dagdp__name)
ECS_REQUIRE(ecs::Tag dagdp_placer_volume)
static void dagdp_placer_volume_link_es(const ecs::Event &, ecs::EntityId eid, const ecs::string &dagdp__name)
{
  volumes_link_ecs_query([&](const ecs::string &dagdp__volume_placer_name, ecs::EntityId &dagdp_internal__volume_placer_eid) {
    if (dagdp__name == dagdp__volume_placer_name)
      dagdp_internal__volume_placer_eid = eid;
  });
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag dagdp_placer_volume)
static void dagdp_placer_volume_unlink_es(const ecs::Event &, const ecs::string &dagdp__name)
{
  volumes_link_ecs_query([&](const ecs::string &dagdp__volume_placer_name, ecs::EntityId &dagdp_internal__volume_placer_eid) {
    if (dagdp__name == dagdp__volume_placer_name)
      dagdp_internal__volume_placer_eid = ecs::INVALID_ENTITY_ID;
  });
}

// TODO: when dagdp__name changes, all volumes that were linked to the old name should ideally be unlinked.
// However, this is a dev-only scenario, and probably a rare one, so it's not implemented for now.

template <typename Callable>
static inline void volume_placers_link_ecs_query(Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(dagdp__volume_placer_name)
ECS_REQUIRE(ecs::Tag dagdp_volume)
static void dagdp_volume__link_es(
  const ecs::Event &, const ecs::string &dagdp__volume_placer_name, ecs::EntityId &dagdp_internal__volume_placer_eid)
{
  volume_placers_link_ecs_query([&](ecs::EntityId eid, const ecs::string &dagdp__name) {
    if (dagdp__name == dagdp__volume_placer_name)
      dagdp_internal__volume_placer_eid = eid;
  });
}

} // namespace dagdp
