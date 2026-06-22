// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_enumerate.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <daECS/core/coreEvents.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/rendInstAccess.h>
#include <shaders/dag_rendInstRes.h>
#include <util/dag_convar.h>
#include <debug/dag_debug3d.h>
#include <render/renderEvent.h>
#include "../riexProcessor.h"
#include "../globalManager.h"
#include "../../shaders/dagdp_heightmap.hlsli"
#include "volume.h"

constexpr float MIN_GEOMETRY_SIZE = 1.0f;

ECS_REGISTER_BOXED_TYPE(dagdp::VolumeManager, nullptr);

namespace material_var
{
static ShaderVariableInfo is_rendinst_clipmap("is_rendinst_clipmap");
static ShaderVariableInfo enable_hmap_blend("enable_hmap_blend");
} // namespace material_var

namespace dagdp
{

CONSOLE_BOOL_VAL("dagdp", volume_placer_no_tiles, false);
CONSOLE_BOOL_VAL("dagdp", volume_placer_no_meshes, false);
CONSOLE_BOOL_VAL("dagdp", debug_volumes, false);

#define BOUNDING_BOX_COLOR E3DCOLOR_MAKE(64, 64, 255, 255)
#define VOLUME_COLOR       E3DCOLOR_MAKE(255, 64, 255, 255)

template <typename Callable>
static inline void on_mesh_placers_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void on_ri_placers_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void around_ri_placers_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void volume_boxes_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void volume_cylinders_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void volume_spheres_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void local_volume_box_ecs_query(ecs::EntityManager &manager, const ecs::EntityId, Callable);

template <typename Callable>
static inline void local_volume_cylinder_ecs_query(ecs::EntityManager &manager, const ecs::EntityId, Callable);

template <typename Callable>
static inline void local_volume_sphere_ecs_query(ecs::EntityManager &manager, const ecs::EntityId, Callable);

static int totalMissingRendInst, totalUsedRendInst;

ECS_NO_ORDER
static inline void volume_view_process_es(
  const dagdp::EventViewProcess &evt, ecs::EntityManager &manager, dagdp::VolumeManager &dagdp__volume_manager)
{
  const auto &rulesBuilder = evt.get<0>();
  auto &builder = dagdp__volume_manager.currentBuilder;

  totalMissingRendInst = 0;
  totalUsedRendInst = 0;

  on_mesh_placers_ecs_query(manager,
    [&](ECS_REQUIRE(ecs::Tag dagdp_placer_on_meshes) ecs::EntityId eid, float dagdp__density, float dagdp__volume_min_triangle_area,
      const ecs::string &dagdp__name, const Point3 &dagdp__volume_axis, bool dagdp__volume_axis_local, bool dagdp__volume_axis_abs,
      const Point2 &dagdp__distance_based_scale, float dagdp__distance_based_range, const Point3 &dagdp__distance_based_center,
      int dagdp__target_mesh_lod, float dagdp__sample_range = -1, int dagdp__csm_cascade_count = -1) {
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
      item.maxDrawDistance = 0;
      item.targetMeshLod = dagdp__target_mesh_lod;
      item.axis = dagdp__volume_axis;
      item.axisLocal = dagdp__volume_axis_local;
      item.csmCascadeCount = dagdp__csm_cascade_count;

      auto &variant = builder.variants.push_back();
      variant.density = dagdp__density;
      variant.minTriangleArea = dagdp__volume_min_triangle_area;
      variant.axisAbs = dagdp__volume_axis_abs;
      variant.distBasedScale = dagdp__distance_based_scale;
      variant.distBasedRange = dagdp__distance_based_range;
      variant.distBasedCenter = dagdp__distance_based_center;
      variant.sampleRange = dagdp__sample_range;

      for (const auto objectGroupEid : placer.objectGroupEids)
      {
        auto iter = rulesBuilder.objectGroups.find(objectGroupEid);
        if (iter == rulesBuilder.objectGroups.end())
          continue;

        auto &objectGroup = variant.objectGroups.push_back();
        objectGroup.effectiveDensity = dagdp__density;
        objectGroup.info = &iter->second;
        item.maxDrawDistance = max(item.maxDrawDistance, iter->second.maxDrawDistance);
      }

      builder.mapping.insert({eid, item});
    });
}

ECS_NO_ORDER static inline void volume_view_finalize_es(const dagdp::EventViewFinalize &evt,
  dagdp::VolumeManager &dagdp__volume_manager)
{
  const auto &viewInfo = evt.get<0>();
  const auto &viewBuilder = evt.get<1>();
  auto nodes = evt.get<2>();
  const auto &rulesBuilder = evt.get<3>();

  create_volume_nodes(viewInfo, viewBuilder, dagdp__volume_manager, nodes, rulesBuilder);

  debug("daGDP: placing on %d RendInst, %d missing RendInst specified", totalUsedRendInst, totalMissingRendInst);

  dagdp__volume_manager.currentBuilder = {};
}

ECS_TAG(render)
ECS_NO_ORDER
static inline void dagdp_volume_before_draw_es(const BeforeDraw &evt, ecs::EntityManager &manager)
{
  start_gather_before_draw_volumes(manager, evt.camPos, evt.frustum);
}

static bbox3f compute_around_ri_fbox(bbox3f_cref frustum_box,
  const ecs::EidList &volume_box_eids,
  const ecs::EidList &volume_cylinder_eids,
  const ecs::EidList &volume_sphere_eids)
{
  float maxLocalRad = 0;
  for (auto volume_eid : volume_box_eids)
    local_volume_box_ecs_query(*g_entity_mgr, volume_eid, [&](ECS_REQUIRE(ecs::Tag dagdp_local_volume_box) const TMatrix &transform) {
      float r = sqrtf(transform.col[0].lengthSq() + transform.col[1].lengthSq() + transform.col[2].lengthSq()) * 0.5f;
      r += transform.col[3].length();
      maxLocalRad = max(maxLocalRad, r);
    });
  for (auto volume_eid : volume_cylinder_eids)
    local_volume_cylinder_ecs_query(*g_entity_mgr, volume_eid,
      [&](ECS_REQUIRE(ecs::Tag dagdp_local_volume_cylinder) const TMatrix &transform) {
        float r = sqrtf(max(transform.col[0].lengthSq(), transform.col[2].lengthSq()) + transform.col[1].lengthSq()) * 0.5f;
        r += transform.col[3].length();
        maxLocalRad = max(maxLocalRad, r);
      });
  for (auto volume_eid : volume_sphere_eids)
    local_volume_sphere_ecs_query(*g_entity_mgr, volume_eid,
      [&](ECS_REQUIRE(ecs::Tag dagdp_local_volume_sphere) const TMatrix &transform, float sphere_zone__radius) {
        float r =
          sqrtf(max(max(transform.col[0].lengthSq(), transform.col[1].lengthSq()), transform.col[2].lengthSq())) * sphere_zone__radius;
        r += transform.col[3].length();
        maxLocalRad = max(maxLocalRad, r);
      });

  bbox3f fbox = frustum_box;
  const float RAD_SCALE_FACTOR = 1.5f;
  v_bbox3_extend(fbox, v_splats(maxLocalRad * RAD_SCALE_FACTOR));
  return fbox;
}

static bbox3f calc_frustum_box(const Frustum &frustum, const Point3 &worldPos, vec4f range)
{
  bbox3f frustumBox;
  frustum.calcFrustumBBox(frustumBox);
  bbox3f rangeBox;
  v_bbox3_init_by_bsph(rangeBox, v_make_vec3f(worldPos.x, worldPos.y, worldPos.z), range);
  return v_bbox3_get_box_intersection(frustumBox, rangeBox);
}

void gather_start(DagdpRiexGatherJob &job,
  const VolumeMapping &volume_mapping,
  const ViewInfo &view_info,
  const ViewPerFrameData &view_per_frame,
  GatherMode mode)
{
  if (job.launched)
    return;
  job.reset();

  for (uint32_t viewportIndex = 0; viewportIndex < view_per_frame.viewports.size(); ++viewportIndex)
  {
    const auto &viewport = view_per_frame.viewports[viewportIndex];
    auto &viewportData = job.addViewport();

    Frustum frustum = viewport.frustum;
    Point4 worldPos(viewport.worldPos.x, viewport.worldPos.y, viewport.worldPos.z, 0.0f);
    auto range = v_splats(min(view_info.maxDrawDistance, viewport.maxDrawDistance) * get_global_range_scale());
    shrink_frustum_zfar(frustum, v_ldu(&worldPos.x), range);
    const bbox3f frustumBox = calc_frustum_box(frustum, viewport.worldPos, range);

    if (!v_test_xyz_finite(frustumBox.bmin) || !v_test_xyz_finite(frustumBox.bmax))
      continue;

    viewportData.frustumBox = frustumBox;
    viewportData.valid = true;

    on_ri_placers_ecs_query(*g_entity_mgr, [&](ECS_REQUIRE(ecs::Tag dagdp_placer_on_ri) ecs::EntityId eid,
                                             const ecs::IntList &dagdp__resource_ids, int dagdp__csm_cascade_count) {
      if (dagdp__csm_cascade_count >= 0 && viewport.csmCascade >= dagdp__csm_cascade_count)
        return;
      if (volume_mapping.find(eid) == volume_mapping.end())
        return;

      DagdpRiexGatherJob::PerViewportData::PlacerData rec;
      rec.eid = eid;
      rec.firstEntry = job.entryCount;
      for (int resIdx : dagdp__resource_ids)
        job.addEntry(resIdx, frustumBox);
      rec.entryCount = job.entryCount - rec.firstEntry;
      viewportData.onRi.push_back(rec);
    });

    around_ri_placers_ecs_query(*g_entity_mgr,
      [&](ECS_REQUIRE(ecs::Tag dagdp_placer_around_ri) ecs::EntityId eid, const ecs::IntList &dagdp__resource_ids,
        const ecs::EidList &dagdp__volume_box_eids, const ecs::EidList &dagdp__volume_cylinder_eids,
        const ecs::EidList &dagdp__volume_sphere_eids, int dagdp__csm_cascade_count) {
        if (dagdp__csm_cascade_count >= 0 && viewport.csmCascade >= dagdp__csm_cascade_count)
          return;

        const bbox3f fbox =
          compute_around_ri_fbox(frustumBox, dagdp__volume_box_eids, dagdp__volume_cylinder_eids, dagdp__volume_sphere_eids);

        DagdpRiexGatherJob::PerViewportData::PlacerData rec;
        rec.eid = eid;
        rec.firstEntry = job.entryCount;
        for (int resIdx : dagdp__resource_ids)
          job.addEntry(resIdx, fbox);
        rec.entryCount = job.entryCount - rec.firstEntry;
        viewportData.aroundRi.push_back(rec);
      });
  }

  job.start(mode);
}

void gather_process(DagdpRiexGatherJob &job,
  const VolumeMapping &volume_mapping,
  const ViewInfo &viewInfo,
  uint32_t viewport_index,
  const Viewport &viewport,
  float max_bounding_radius,
  int target_mesh_lod,
  RiexProcessor &riex_processor,
  RelevantMeshes &out_result,
  RelevantTiles &out_tiles,
  RelevantVolumes &out_volumes)
{
  const auto processMeshes = [&](const rendinst::riex_collidable_t &out_handles, uint32_t volume_index, int mesh_lod) {
    if (volume_placer_no_meshes)
      return;
    for (const auto handle : out_handles)
    {
      const uint32_t resIndex = rendinst::handle_to_ri_type(handle);
      const RenderableInstanceLodsResource *riLodsRes = rendinst::getRIGenExtraRes(resIndex);
      const int bestLod = riLodsRes->getQlBestLod();
      const int actualLod = min<int>((mesh_lod >= 0 ? mesh_lod : target_mesh_lod), riLodsRes->lods.size() - 1);
      if (bestLod > actualLod)
        continue;

      const mat43f &instTm = rendinst::getRIGenExtra43(handle);
      mat44f instTm44;
      v_mat43_transpose_to_mat44(instTm44, instTm);
      if (v_extract_x((v_mat44_max_scale43_x(instTm44))) * riLodsRes->bsphRad < MIN_GEOMETRY_SIZE)
        continue;

      RenderableInstanceResource *riRes = riLodsRes->lods[actualLod].scene;
      const auto *state = riex_processor.ask(riRes);

      if (state)
      {
        const dag::ConstSpan<ShaderMesh::RElem> elems = riRes->getMesh()->getMesh()->getMesh()->getElems(ShaderMesh::STG_opaque);

        G_ASSERT(elems.size() == state->areasIndices.size());
        for (auto [i, elem] : enumerate(elems))
        {
          // NOTE: I disabled the is_rendinst_clipmap/enable_hmap_blend check below
          // because it's not clear what its purpose is, and it prevents placement
          // on the meshes we want to place on.
          // This shouldn't affect anyone, because daGdp volume placers are not used
          // anywhere except for some tests yet (unlike heightmap/biome placers).

          // int isRendinstClipmap = 0;
          // int isHeightmapBlended = 0;

          // if (elem.mat->getIntVariable(material_var::is_rendinst_clipmap.get_var_id(), isRendinstClipmap) && isRendinstClipmap ||
          //     elem.mat->getIntVariable(material_var::enable_hmap_blend.get_var_id(), isHeightmapBlended) && isHeightmapBlended)
          // {
          //   // Skip materials that are rendered into the clipmap. See RiExtraPool::isRendinstClipmap.
          //   continue;
          // }

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
          item.volumeIndex = volume_index;
          item.uniqueId = rendinst::handle_to_ri_inst(handle);
        }
      }
    }
  };

  const bool asyncPath = is_volume_early_riex_gather_enabled();

  Frustum frustum = viewport.frustum;
  Point4 worldPos(viewport.worldPos.x, viewport.worldPos.y, viewport.worldPos.z, 0.0f);
  auto range = v_splats(min(viewInfo.maxDrawDistance, viewport.maxDrawDistance) * get_global_range_scale());
  shrink_frustum_zfar(frustum, v_ldu(&worldPos.x), range);

  bbox3f frustumBox;
  if (asyncPath)
  {
    G_ASSERT(viewport_index < job.viewportCount);
    if (!job.viewportDataPool[viewport_index].valid)
      return;
    frustumBox = job.viewportDataPool[viewport_index].frustumBox;
  }
  else
  {
    frustumBox = calc_frustum_box(frustum, viewport.worldPos, range);
    if (!v_test_xyz_finite(frustumBox.bmin) || !v_test_xyz_finite(frustumBox.bmax))
      return;
  }

  const auto addVolume = [&](const ecs::EntityId dagdp_internal__volume_placer_eid, const TMatrix &transform, float scale,
                           int volume_type) {
    const auto iter = volume_mapping.find(dagdp_internal__volume_placer_eid);
    if (iter == volume_mapping.end())
      return;
    const auto &variant = iter->second;

    if (variant.csmCascadeCount >= 0 && viewport.csmCascade >= variant.csmCascadeCount)
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
    v_bbox3_init_by_bsph(drawBox, v_make_vec3f(viewport.worldPos.x, viewport.worldPos.y, viewport.worldPos.z),
      v_splats(variant.maxDrawDistance * get_global_range_scale()));
    if (!v_bbox3_test_box_intersect(drawBox, volBox))
      return;

    // TODO: Performance: can cull more with more precise checks (original box/ellipsoid instead of bbox).
    if (!frustum.testBoxExtentB(center2, extent2))
      return;

    if (debug_volumes)
    {
      Point3 center, extent;
      v_stu_p3(&center.x, tm44.col3);
      v_stu_p3(&extent.x, extent2);
      draw_debug_box_buffered(BBox3(center - extent * 0.5, center + extent * 0.5), BOUNDING_BOX_COLOR, 1);

      switch (volume_type)
      {
        case VOLUME_TYPE_BOX: draw_debug_box_buffered(BBox3(Point3::ZERO, scale * 2), transform, VOLUME_COLOR, 1); break;
        case VOLUME_TYPE_CYLINDER:
          draw_debug_elipse_buffered(transform * Point3(0, -1, 0), transform.getcol(0) * scale, transform.getcol(2) * scale,
            VOLUME_COLOR, 24, 1);
          draw_debug_elipse_buffered(transform * Point3(0, +1, 0), transform.getcol(0) * scale, transform.getcol(2) * scale,
            VOLUME_COLOR, 24, 1);
          draw_debug_line_buffered(transform * Point3(0, -1, 0), transform * Point3(0, +1, 0), VOLUME_COLOR, 1);
          break;
        case VOLUME_TYPE_ELLIPSOID:
          draw_debug_elipse_buffered(transform.getcol(3), transform.getcol(0) * scale, transform.getcol(1) * scale, VOLUME_COLOR, 24,
            1);
          draw_debug_elipse_buffered(transform.getcol(3), transform.getcol(1) * scale, transform.getcol(2) * scale, VOLUME_COLOR, 24,
            1);
          draw_debug_elipse_buffered(transform.getcol(3), transform.getcol(2) * scale, transform.getcol(0) * scale, VOLUME_COLOR, 24,
            1);
          break;
        case VOLUME_TYPE_FULL: break;
      }
    }

    G_ASSERT(scale > FLT_EPSILON);
    const vec4f invScale = v_splats(1.0f / scale);
    const uint32_t volumeIndex = out_volumes.size();
    auto &volume = out_volumes.push_back();
    v_stu(&volume.itmRow0, v_mul(itm43.row0, invScale));
    v_stu(&volume.itmRow1, v_mul(itm43.row1, invScale));
    v_stu(&volume.itmRow2, v_mul(itm43.row2, invScale));
    Point3 axis = variant.axis;
    if (variant.axisLocal)
      axis = transform * axis;
    volume.axis = normalize(axis);
    volume.volumeType = volume_type;
    volume.variantIndex = variant.variantIndex;

    // Reference: volumePlacerES.cpp.inl, VolumePlacer::gatherGeometryInBox
    bbox3f bbox;
    v_bbox3_init(bbox, tm44, {v_splats(-scale), v_splats(scale)});
    rendinst::riex_collidable_t out_handles;
    rendinst::gatherRIGenExtraCollidableMin(out_handles, bbox, MIN_GEOMETRY_SIZE);

    if (!volume_placer_no_tiles)
    {
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
    }

    processMeshes(out_handles, volumeIndex, variant.targetMeshLod);
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

  const DagdpRiexGatherJob::PerViewportData *viewportData = nullptr;
  size_t onRiIdx = 0;
  size_t aroundRiIdx = 0;
  if (asyncPath)
  {
    job.waitDone();
    viewportData = &job.viewportDataPool[viewport_index];
  }

  on_ri_placers_ecs_query(*g_entity_mgr, [&](ECS_REQUIRE(ecs::Tag dagdp_placer_on_ri) ecs::EntityId eid,
                                           const ecs::IntList &dagdp__resource_ids, int dagdp__csm_cascade_count) {
    if (dagdp__csm_cascade_count >= 0 && viewport.csmCascade >= dagdp__csm_cascade_count)
      return;

    const auto iter = volume_mapping.find(eid);
    if (iter == volume_mapping.end())
      return;
    const auto &variant = iter->second;

    rendinst::riex_collidable_t out_handles;
    if (asyncPath)
    {
      G_ASSERT(onRiIdx < viewportData->onRi.size());
      const auto &rec = viewportData->onRi[onRiIdx++];
      G_ASSERT(rec.eid == eid);
      for (size_t i = 0; i < rec.entryCount; ++i)
      {
        const auto &h = job.entryPool[rec.firstEntry + i].handles;
        out_handles.insert(out_handles.end(), h.begin(), h.end());
      }
    }
    else
    {
      Tab<rendinst::riex_handle_t> ri_handles;
      for (int resIdx : dagdp__resource_ids)
      {
        rendinst::getRiGenExtraInstances(ri_handles, resIdx, frustumBox);
        out_handles.insert(out_handles.end(), ri_handles.begin(), ri_handles.end());
        ri_handles.clear(); // just to make sure, getRiGenExtraInstances() clears anyway
      }
    }

    if (out_handles.empty())
      return;

    const uint32_t volumeIndex = out_volumes.size();
    auto &volume = out_volumes.push_back();
    v_stu(&volume.itmRow0, V_C_UNIT_1000);
    v_stu(&volume.itmRow1, V_C_UNIT_0100);
    v_stu(&volume.itmRow2, V_C_UNIT_0010);
    volume.axis = variant.axis;
    volume.volumeType = VOLUME_TYPE_FULL;
    volume.variantIndex = variant.variantIndex;

    processMeshes(out_handles, volumeIndex, variant.targetMeshLod);
  });

  around_ri_placers_ecs_query(*g_entity_mgr,
    [&](ECS_REQUIRE(ecs::Tag dagdp_placer_around_ri) ecs::EntityId eid, const ecs::IntList &dagdp__resource_ids,
      const ecs::EidList &dagdp__volume_box_eids, const ecs::EidList &dagdp__volume_cylinder_eids,
      const ecs::EidList &dagdp__volume_sphere_eids, int dagdp__csm_cascade_count) {
      if (dagdp__csm_cascade_count >= 0 && viewport.csmCascade >= dagdp__csm_cascade_count)
        return;

      const bbox3f fbox =
        compute_around_ri_fbox(frustumBox, dagdp__volume_box_eids, dagdp__volume_cylinder_eids, dagdp__volume_sphere_eids);

      eastl::fixed_vector<int16_t, 32> riGenPools;
      for (auto id : dagdp__resource_ids)
        if (int pool = rendinst::getRIExtraPoolRef(id); pool >= 0)
          riGenPools.push_back(pool);

      BBox3 box;
      v_stu_bbox3(box, fbox);

      dag::Vector<mat44f, framemem_allocator> sources;
      rendinst::getRIGenTMsInBox(box, make_span_const(riGenPools),
        [&sources](const rendinst::RendInstDesc & /* desc */, const mat44f &m44) { sources.push_back(m44); });

      if (asyncPath)
      {
        G_ASSERT(aroundRiIdx < viewportData->aroundRi.size());
        const auto &rec = viewportData->aroundRi[aroundRiIdx++];
        G_ASSERT(rec.eid == eid);
        for (size_t i = 0; i < rec.entryCount; ++i)
        {
          const auto &h = job.entryPool[rec.firstEntry + i].handles;
          for (auto handle : h)
            rendinst::getRIGenExtra44(handle, sources.push_back());
        }
      }
      else
      {
        Tab<rendinst::riex_handle_t> ri_handles;
        for (int resIdx : dagdp__resource_ids)
        {
          rendinst::getRiGenExtraInstances(ri_handles, resIdx, fbox);
          for (auto h : ri_handles)
            rendinst::getRIGenExtra44(h, sources.push_back());
          ri_handles.clear(); // just to make sure, getRiGenExtraInstances() clears anyway
        }
      }

      if (sources.empty())
        return;

      const auto addLocalVolume = [&, eid](const TMatrix &transform, float scale, int volume_type) {
        DECL_ALIGN16(TMatrix, tm);
        for (const auto &m44 : sources)
        {
          v_mat_43ca_from_mat44(&tm[0][0], m44);
          addVolume(eid, tm * transform, scale, volume_type);
        }
      };

      for (auto volume_eid : dagdp__volume_box_eids)
        local_volume_box_ecs_query(*g_entity_mgr, volume_eid,
          [&](ECS_REQUIRE(ecs::Tag dagdp_local_volume_box) const TMatrix &transform) {
            addLocalVolume(transform, 0.5f, VOLUME_TYPE_BOX);
          });

      for (auto volume_eid : dagdp__volume_cylinder_eids)
        local_volume_cylinder_ecs_query(*g_entity_mgr, volume_eid,
          [&](ECS_REQUIRE(ecs::Tag dagdp_local_volume_cylinder) const TMatrix &transform) {
            addLocalVolume(transform, 0.5f, VOLUME_TYPE_CYLINDER);
          });

      for (auto volume_eid : dagdp__volume_sphere_eids)
        local_volume_sphere_ecs_query(*g_entity_mgr, volume_eid,
          [&](ECS_REQUIRE(ecs::Tag dagdp_local_volume_sphere) const TMatrix &transform, float sphere_zone__radius) {
            addLocalVolume(transform, sphere_zone__radius, VOLUME_TYPE_ELLIPSOID);
          });
    });

  if (asyncPath)
  {
    G_ASSERT(onRiIdx == viewportData->onRi.size());
    G_ASSERT(aroundRiIdx == viewportData->aroundRi.size());
  }
}

template <typename Callable>
static inline void manager_ecs_query(ecs::EntityManager &manager, Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_ON_EVENT(on_disappear)
ECS_TRACK(dagdp__name,
  dagdp__density,
  dagdp__volume_min_triangle_area,
  dagdp__volume_axis,
  dagdp__volume_axis_local,
  dagdp__volume_axis_abs,
  dagdp__distance_based_scale,
  dagdp__distance_based_range,
  dagdp__distance_based_center,
  dagdp__target_mesh_lod,
  dagdp__sample_range)
ECS_REQUIRE(ecs::Tag dagdp_placer_on_meshes,
  const ecs::string &dagdp__name,
  float dagdp__density,
  float dagdp__volume_min_triangle_area,
  const Point3 &dagdp__volume_axis,
  bool dagdp__volume_axis_local,
  bool dagdp__volume_axis_abs,
  const Point2 &dagdp__distance_based_scale,
  float dagdp__distance_based_range,
  const Point3 &dagdp__distance_based_center,
  int dagdp__target_mesh_lod)
static void dagdp_placer_volume_changed_es(const ecs::Event &, ecs::EntityManager &manager, float dagdp__sample_range = -1)
{
  G_UNUSED(dagdp__sample_range);
  manager_ecs_query(manager, [](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.invalidateRules(); });
}

ECS_TAG(render)
ECS_REQUIRE(ecs::Tag dagdp_placer_on_meshes)
ECS_BEFORE(volume_view_finalize_es)
static inline void dagdp_placer_on_meshes_resource_link_es(
  const dagdp::EventViewFinalize &, const ecs::StringList &dagdp__resource_names, ecs::IntList &dagdp__resource_ids)
{
  dagdp__resource_ids.clear();
  dagdp__resource_ids.reserve(dagdp__resource_names.size());
  for (const auto &name : dagdp__resource_names)
  {
    int resIdx = rendinst::getRIGenExtraResIdx(name.c_str());
    if (resIdx == -1)
    {
      debug("daGDP: can't find RendInst '%s' to place on", name.c_str());
      totalMissingRendInst++;
      continue;
    }
    dagdp__resource_ids.push_back(resIdx);
    totalUsedRendInst++;
  }
}

template <typename Callable>
static inline void volumes_link_ecs_query(ecs::EntityManager &manager, Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(dagdp__name)
ECS_REQUIRE(ecs::Tag dagdp_placer_volume)
static void dagdp_placer_volume_link_es(
  const ecs::Event &, ecs::EntityManager &manager, ecs::EntityId eid, const ecs::string &dagdp__name)
{
  volumes_link_ecs_query(manager, [&](const ecs::string &dagdp__volume_placer_name, ecs::EntityId &dagdp_internal__volume_placer_eid) {
    if (dagdp__name == dagdp__volume_placer_name)
      dagdp_internal__volume_placer_eid = eid;
  });
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag dagdp_placer_volume)
static void dagdp_placer_volume_unlink_es(const ecs::Event &, ecs::EntityManager &manager, const ecs::string &dagdp__name)
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
static void dagdp_volume__link_es(const ecs::Event &,
  ecs::EntityManager &manager,
  const ecs::string &dagdp__volume_placer_name,
  ecs::EntityId &dagdp_internal__volume_placer_eid)
{
  volume_placers_link_ecs_query(manager, [&](ecs::EntityId eid, const ecs::string &dagdp__name) {
    if (dagdp__name == dagdp__volume_placer_name)
      dagdp_internal__volume_placer_eid = eid;
  });
}

template <typename Callable>
static inline void local_volume_boxes_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void local_volume_cylinders_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void local_volume_spheres_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void local_volume_placers_link_box_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void local_volume_placers_link_cylinder_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void local_volume_placers_link_sphere_ecs_query(ecs::EntityManager &manager, Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(dagdp__volume_placer_name)
ECS_REQUIRE(ecs::Tag dagdp_local_volume_box)
static void dagdp_local_volume_box__link_es(const ecs::Event &,
  ecs::EntityManager &manager,
  ecs::EntityId eid,
  const ecs::string &dagdp__volume_placer_name,
  ecs::EntityId &dagdp_internal__volume_placer_eid)
{
  auto volumeEid = eid;
  local_volume_placers_link_box_ecs_query(manager,
    [&](ecs::EntityId eid, const ecs::string &dagdp__name, ecs::EidList &dagdp__volume_box_eids) {
      if (dagdp__name == dagdp__volume_placer_name)
      {
        dagdp_internal__volume_placer_eid = eid;
        dagdp__volume_box_eids.push_back(volumeEid);
      }
    });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(dagdp__volume_placer_name)
ECS_REQUIRE(ecs::Tag dagdp_local_volume_cylinder)
static void dagdp_local_volume_cylinder__link_es(const ecs::Event &,
  ecs::EntityManager &manager,
  ecs::EntityId eid,
  const ecs::string &dagdp__volume_placer_name,
  ecs::EntityId &dagdp_internal__volume_placer_eid)
{
  auto volumeEid = eid;
  local_volume_placers_link_cylinder_ecs_query(manager,
    [&](ecs::EntityId eid, const ecs::string &dagdp__name, ecs::EidList &dagdp__volume_cylinder_eids) {
      if (dagdp__name == dagdp__volume_placer_name)
      {
        dagdp_internal__volume_placer_eid = eid;
        dagdp__volume_cylinder_eids.push_back(volumeEid);
      }
    });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(dagdp__volume_placer_name)
ECS_REQUIRE(ecs::Tag dagdp_local_volume_sphere)
static void dagdp_local_volume_sphere__link_es(const ecs::Event &,
  ecs::EntityManager &manager,
  ecs::EntityId eid,
  const ecs::string &dagdp__volume_placer_name,
  ecs::EntityId &dagdp_internal__volume_placer_eid)
{
  auto volumeEid = eid;
  local_volume_placers_link_sphere_ecs_query(manager,
    [&](ecs::EntityId eid, const ecs::string &dagdp__name, ecs::EidList &dagdp__volume_sphere_eids) {
      if (dagdp__name == dagdp__volume_placer_name)
      {
        dagdp_internal__volume_placer_eid = eid;
        dagdp__volume_sphere_eids.push_back(volumeEid);
      }
    });
}

} // namespace dagdp
