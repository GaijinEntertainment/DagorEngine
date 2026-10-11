// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/fixed_function.h>
#include <generic/dag_enumerate.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <daECS/core/coreEvents.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/ccExtra.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/rendInstAccess.h>
#include <ecs/rendInst/riExtra.h>
#include <shaders/dag_rendInstRes.h>
#include <util/dag_convar.h>
#include <debug/dag_debug3d.h>
#include <perfMon/dag_statDrv.h>
#include <render/renderEvent.h>
#include <ecs/render/updateStageRender.h>
#include "../riexProcessor.h"
#include "../globalManager.h"
#include "volume.h"

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

// A placer with a cascade limit is skipped in the deeper CSM cascades.
static bool cascade_excluded(int csm_cascade_count, int viewport_cascade)
{
  return csm_cascade_count >= 0 && viewport_cascade >= csm_cascade_count;
}

static const VolumeMappingItem *placer_gather_variant(
  const VolumeMapping &mapping, ecs::EntityId eid, int csm_cascade_count, int viewport_cascade)
{
  if (cascade_excluded(csm_cascade_count, viewport_cascade))
    return nullptr;
  const auto iter = mapping.find(eid);
  return iter == mapping.end() ? nullptr : &iter->second;
}

// The job records one entry per placer per viewport in ECS query order, and the FG node walks the same
// queries in the same order, so the records line up by sequence. Checked in release too, not asserted:
// these records index the job's pools.
template <typename T>
static const T *next_placer_record(const Tab<T> &records, size_t &index, ecs::EntityId eid)
{
  if (index >= records.size())
    return nullptr;
  const T &rec = records[index++];
  if (rec.eid != eid)
    return nullptr;
  return &rec;
}

// The three world volume shapes in one iteration order, shared by the job and the FG node so that both
// always consider the same volumes. Not a template: the generated queries must be instantiated from a
// non-dependent context, otherwise clang reports them as undefined internal.
using WorldVolumeCallback = eastl::fixed_function<4 * sizeof(void *), void(ecs::EntityId, const TMatrix &, float, int)>;

static void for_each_world_volume(const WorldVolumeCallback &on_volume)
{
  volume_boxes_ecs_query(*g_entity_mgr,
    [&](ECS_REQUIRE(ecs::Tag dagdp_volume_box) const ecs::EntityId dagdp_internal__volume_placer_eid, const TMatrix &transform) {
      on_volume(dagdp_internal__volume_placer_eid, transform, 0.5f, VOLUME_TYPE_BOX);
    });

  volume_cylinders_ecs_query(*g_entity_mgr,
    [&](ECS_REQUIRE(ecs::Tag dagdp_volume_cylinder) const ecs::EntityId dagdp_internal__volume_placer_eid, const TMatrix &transform) {
      on_volume(dagdp_internal__volume_placer_eid, transform, 0.5f, VOLUME_TYPE_CYLINDER);
    });

  volume_spheres_ecs_query(*g_entity_mgr,
    [&](ECS_REQUIRE(ecs::Tag dagdp_volume_sphere) const ecs::EntityId dagdp_internal__volume_placer_eid, const TMatrix &transform,
      float sphere_zone__radius) {
      if (sphere_zone__radius <= FLT_EPSILON)
        return;

      on_volume(dagdp_internal__volume_placer_eid, transform, sphere_zone__radius, VOLUME_TYPE_ELLIPSOID);
    });
}

static int totalMissingRendInst, totalUsedRendInst;

void debug_draw_volume(const TMatrix &transform, float scale, vec4f extent2, int volume_type)
{
  if (debug_volumes)
  {
    Point3 center = transform.col[3];
    Point3 extent;
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
        draw_debug_elipse_buffered(transform.getcol(3), transform.getcol(0) * scale, transform.getcol(1) * scale, VOLUME_COLOR, 24, 1);
        draw_debug_elipse_buffered(transform.getcol(3), transform.getcol(1) * scale, transform.getcol(2) * scale, VOLUME_COLOR, 24, 1);
        draw_debug_elipse_buffered(transform.getcol(3), transform.getcol(2) * scale, transform.getcol(0) * scale, VOLUME_COLOR, 24, 1);
        break;
      case VOLUME_TYPE_FULL: break;
    }
  }
}

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

      const float density = dagdp__density * GlobalManager::clampedGlobalDensityMul();

      VolumeMappingItem item;
      item.variantIndex = builder.variants.size();
      item.density = density;
      item.maxDrawDistance = 0;
      item.targetMeshLod = dagdp__target_mesh_lod;
      item.axis = dagdp__volume_axis;
      item.axisLocal = dagdp__volume_axis_local;
      item.csmCascadeCount = dagdp__csm_cascade_count;

      auto &variant = builder.variants.push_back();
      variant.density = density;
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
        objectGroup.effectiveDensity = density;
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
ECS_AFTER(dagdp_update_es)            // view rebuild there frees the job
ECS_BEFORE(animchar_before_render_es) // purely for optimization, p-for jobs wake the workers
static inline void dagdp_volume_start_camera_gather_es(const UpdateStageInfoBeforeRender &evt)
{
  start_gather_camera_volumes(evt.camPos, evt.mainCullingFrustum);
}

ECS_TAG(render)
ECS_NO_ORDER
static inline void dagdp_volume_start_csm_gather_es(const BeforeStartVisibility &, ecs::EntityManager &manager)
{
  start_gather_csm_volumes(manager);
}

// Returns the largest local volume radius, for the frustum box the sources are searched in.
static float collect_local_volumes(const ecs::EidList &volume_box_eids,
  const ecs::EidList &volume_cylinder_eids,
  const ecs::EidList &volume_sphere_eids,
  LocalVolumeSnapshots &out_volumes)
{
  float maxLocalRad = 0;
  for (auto volume_eid : volume_box_eids)
    local_volume_box_ecs_query(*g_entity_mgr, volume_eid, [&](ECS_REQUIRE(ecs::Tag dagdp_local_volume_box) const TMatrix &transform) {
      float r = sqrtf(transform.col[0].lengthSq() + transform.col[1].lengthSq() + transform.col[2].lengthSq()) * 0.5f;
      r += transform.col[3].length();
      maxLocalRad = max(maxLocalRad, r);
      out_volumes.push_back({transform, 0.5f, VOLUME_TYPE_BOX});
    });
  for (auto volume_eid : volume_cylinder_eids)
    local_volume_cylinder_ecs_query(*g_entity_mgr, volume_eid,
      [&](ECS_REQUIRE(ecs::Tag dagdp_local_volume_cylinder) const TMatrix &transform) {
        float r = sqrtf(max(transform.col[0].lengthSq(), transform.col[2].lengthSq()) + transform.col[1].lengthSq()) * 0.5f;
        r += transform.col[3].length();
        maxLocalRad = max(maxLocalRad, r);
        out_volumes.push_back({transform, 0.5f, VOLUME_TYPE_CYLINDER});
      });
  for (auto volume_eid : volume_sphere_eids)
    local_volume_sphere_ecs_query(*g_entity_mgr, volume_eid,
      [&](ECS_REQUIRE(ecs::Tag dagdp_local_volume_sphere) const TMatrix &transform, float sphere_zone__radius) {
        float r =
          sqrtf(max(max(transform.col[0].lengthSq(), transform.col[1].lengthSq()), transform.col[2].lengthSq())) * sphere_zone__radius;
        r += transform.col[3].length();
        maxLocalRad = max(maxLocalRad, r);
        out_volumes.push_back({transform, sphere_zone__radius, VOLUME_TYPE_ELLIPSOID});
      });
  return maxLocalRad;
}

// RIGen sources have no riex handle to fetch later, so only their transforms are collected.
template <typename Container>
static void gather_rigen_source_tms(bbox3f_cref fbox, const ecs::IntList &resource_ids, Container &out_tms)
{
  eastl::fixed_vector<int16_t, 32> riGenPools;
  for (auto id : resource_ids)
    if (int pool = rendinst::getRIExtraPoolRef(id); pool >= 0)
      riGenPools.push_back(pool);
  if (riGenPools.empty())
    return;

  BBox3 box;
  v_stu_bbox3(box, fbox);
  rendinst::getRIGenTMsInBox(box, make_span_const(riGenPools),
    [&out_tms](const rendinst::RendInstDesc & /* desc */, const mat44f &m44) {
      v_mat_43cu_from_mat44(out_tms.push_back().array, m44);
    });
}

static bbox3f extend_around_ri_fbox(bbox3f_cref frustum_box, float max_local_rad)
{
  bbox3f fbox = frustum_box;
  const float RAD_SCALE_FACTOR = 1.5f;
  v_bbox3_extend(fbox, v_splats(max_local_rad * RAD_SCALE_FACTOR));
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
  float max_bounding_radius,
  int target_mesh_lod,
  const RiexProcessor &riex_processor,
  GatherMode mode)
{
  if (job.launched)
    return;

  // Nothing to render into this frame (e.g. no CSM cascades at wide FOV): don't launch.
  if (view_per_frame.viewports.empty())
    return;

  DA_PROFILE_EVENT_DESC(job.startDesc);

  job.reset();
  job.rangeScale = get_global_range_scale();
  job.maxBoundingRadius = max_bounding_radius;
  job.riexProcessor = &riex_processor;
  job.targetMeshLod = target_mesh_lod;
  job.meshesEnabled = !volume_placer_no_meshes;
  job.gridGathered = is_volume_early_grid_gather_enabled();

  for (uint32_t viewportIndex = 0; viewportIndex < view_per_frame.viewports.size(); ++viewportIndex)
  {
    const auto &viewport = view_per_frame.viewports[viewportIndex];
    auto &viewportData = job.addViewport();
    viewportData.worldPos = v_make_vec3f(viewport.worldPos.x, viewport.worldPos.y, viewport.worldPos.z);

    Frustum frustum = viewport.frustum;
    Point4 worldPos(viewport.worldPos.x, viewport.worldPos.y, viewport.worldPos.z, 0.0f);
    auto range = v_splats(min(view_info.maxDrawDistance, viewport.maxDrawDistance) * job.rangeScale);
    shrink_frustum_zfar(frustum, v_ldu(&worldPos.x), range);
    const bbox3f frustumBox = calc_frustum_box(frustum, viewport.worldPos, range);

    if (!v_test_xyz_finite(frustumBox.bmin) || !v_test_xyz_finite(frustumBox.bmax))
      continue;

    viewportData.frustum = frustum;
    viewportData.frustumBox = frustumBox;
    viewportData.valid = true;

    on_ri_placers_ecs_query(*g_entity_mgr, [&](ECS_REQUIRE(ecs::Tag dagdp_placer_on_ri) ecs::EntityId eid,
                                             const ecs::IntList &dagdp__resource_ids, int dagdp__csm_cascade_count) {
      const auto *variant = placer_gather_variant(volume_mapping, eid, dagdp__csm_cascade_count, viewport.csmCascade);
      if (!variant)
        return;

      DagdpRiexGatherJob::PlacerData rec;
      rec.eid = eid;
      rec.meshLod = resolve_mesh_lod(*variant, target_mesh_lod);
      job.addEntries(rec, dagdp__resource_ids, frustumBox);
      viewportData.onRi.push_back(rec);
    });

    around_ri_placers_ecs_query(*g_entity_mgr,
      [&](ECS_REQUIRE(ecs::Tag dagdp_placer_around_ri) ecs::EntityId eid, const ecs::IntList &dagdp__resource_ids,
        const ecs::EidList &dagdp__volume_box_eids, const ecs::EidList &dagdp__volume_cylinder_eids,
        const ecs::EidList &dagdp__volume_sphere_eids, int dagdp__csm_cascade_count) {
        const auto *variant = placer_gather_variant(volume_mapping, eid, dagdp__csm_cascade_count, viewport.csmCascade);
        if (!variant)
          return;

        DagdpRiexGatherJob::AroundRiPlacerData rec;
        rec.eid = eid;
        rec.variant = variant;
        rec.drawRadius = variant->maxDrawDistance * job.rangeScale;
        rec.firstLocalVolume = job.localVolumeCount;
        LocalVolumeSnapshots localVolumes;
        const float maxLocalRad =
          collect_local_volumes(dagdp__volume_box_eids, dagdp__volume_cylinder_eids, dagdp__volume_sphere_eids, localVolumes);
        if (job.gridGathered)
          for (const auto &local : localVolumes)
            job.addLocalVolume() = local;
        rec.localVolumeCount = job.localVolumeCount - rec.firstLocalVolume;
        rec.fbox = extend_around_ri_fbox(frustumBox, maxLocalRad);
        job.addEntries(rec, dagdp__resource_ids, rec.fbox);
        if (job.gridGathered)
        {
          rec.firstRiGenSource = job.riGenSourcePool.size();
          gather_rigen_source_tms(rec.fbox, dagdp__resource_ids, job.riGenSourcePool);
          rec.riGenSourceCount = job.riGenSourcePool.size() - rec.firstRiGenSource;
        }
        viewportData.aroundRi.push_back(rec);
      });
  }

  const auto addWorldVolume = [&](ecs::EntityId placer_eid, const TMatrix &transform, float scale, int volume_type) {
    const auto iter = volume_mapping.find(placer_eid);
    if (iter == volume_mapping.end())
      return;
    const auto &variant = iter->second;

    mat44f tm44;
    v_mat44_make_from_43cu_unsafe(tm44, transform.array);
    DagdpRiexGatherJob::VolumeEntry e{};
    init_volume_entry(e, &variant, transform, tm44, scale, volume_type, max_bounding_radius);
    const float drawRadius = variant.maxDrawDistance * job.rangeScale;

    // Only whether any viewport wants it: gather_process re-culls per viewport on the same inputs.
    bool anyViewport = false;
    for (size_t v = 0; v < job.viewportCount && !anyViewport; ++v)
    {
      const auto &perViewport = job.viewportDataPool[v];
      anyViewport = perViewport.valid && !cascade_excluded(variant.csmCascadeCount, view_per_frame.viewports[v].csmCascade) &&
                    volume_in_draw_range(e.center, e.extent2, perViewport.worldPos, drawRadius) &&
                    perViewport.frustum.testBoxExtentB(v_add(e.center, e.center), e.extent2) != 0;
    }
    if (!anyViewport)
      return;

    prepare_volume_entry(e, tm44);
    job.addVolumeEntry(e);
  };

  if (job.gridGathered)
  {
    for_each_world_volume(addWorldVolume);

    job.worldVolumeCount = job.volumeCount;
  }

  DA_PROFILE_TAG_DESC(job.startDesc, "viewports=%d riex_entries=%d", (int)job.viewportCount, (int)job.entryCount);
  DA_PROFILE_TAG_DESC(job.startDesc, "world=%d local=%d", (int)job.worldVolumeCount, (int)job.localVolumeCount);

  job.start(mode);
}

// The lod scene the placement filter reads for a riex resource: null for no resource, or while that lod is not loaded.
static RenderableInstanceResource *placement_lod_scene(const RenderableInstanceLodsResource *res, int mesh_lod)
{
  if (!res)
    return nullptr;
  const int lod = min<int>(mesh_lod, res->lods.size() - 1);
  if (res->getQlBestLod() > lod)
    return nullptr;
  return res->lods[lod].scene;
}

int filter_volume_meshes(dag::ConstSpan<rendinst::riex_handle_t> handles,
  int mesh_lod,
  const bbox3f *volume_box,
  const RiexProcessor &riex_processor,
  Tab<MeshIntersection> &out_meshes)
{
  int firstUnprocessedRes = -1;
  rendinst::ScopedRIExtraReadLock rd;
  for (const auto handle : handles)
  {
    const int resIndex = rendinst::handle_to_ri_type(handle);
    if (rendinst::isRIGenExtraDynamic(resIndex))
      continue;
    const RenderableInstanceLodsResource *riLodsRes = rendinst::getRIGenExtraRes(resIndex);
    RenderableInstanceResource *riRes = placement_lod_scene(riLodsRes, mesh_lod);
    if (!riRes)
      continue;

    const mat43f &instTm = rendinst::getRIGenExtra43(handle);
    mat44f instTm44;
    v_mat43_transpose_to_mat44(instTm44, instTm);
    if (v_extract_x((v_mat44_max_scale43_x(instTm44))) * riLodsRes->bsphRad < MIN_GEOMETRY_SIZE)
      continue;

    if (volume_box != nullptr)
    {
      bbox3f lbox, wbox;
      v_bbox3_init_by_segment(lbox, v_ldu_p3(&riLodsRes->bbox[0].x), v_ldu_p3(&riLodsRes->bbox[1].x));
      v_bbox3_init(wbox, instTm44, lbox);
      if (!v_bbox3_test_box_intersect(wbox, *volume_box))
        continue;
    }

    const auto *state = riex_processor.find(riRes);
    if (!state)
    {
      if (firstUnprocessedRes < 0)
        firstUnprocessedRes = resIndex;
      continue;
    }

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

      auto &item = out_meshes.push_back();
      item.startIndex = elem.si;
      item.numFaces = elem.numf;
      item.baseVertex = elem.baseVertex;
      item.stride = elem.vertexData->getStride();
      v_stu(&item.meshTmRow0.x, instTm.row0);
      v_stu(&item.meshTmRow1.x, instTm.row1);
      v_stu(&item.meshTmRow2.x, instTm.row2);
      item.areasIndex = state->areasIndices[i];
      item.vbIndex = elem.vertexData->getVbIdx();
      item.volumeIndex = 0;
      item.uniqueId = rendinst::handle_to_ri_inst(handle);
    }
  }
  return firstUnprocessedRes;
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
  TIME_PROFILE(dagdp_volume_gather_process);

  // The one way meshes reach the output, so the processor is asked for emitted volumes only, in emission order: the
  // first one asked is the resource processed this frame.
  const auto emitMeshes = [&](dag::ConstSpan<MeshIntersection> meshes, int unprocessed_res, int mesh_lod, uint32_t volume_index) {
    // Resolved here, because a resource reload in between frees the old lod scenes
    if (unprocessed_res >= 0)
    {
      rendinst::ScopedRIExtraReadLock rd;
      if (RenderableInstanceResource *scene = placement_lod_scene(rendinst::getRIGenExtraRes(unprocessed_res), mesh_lod))
        riex_processor.ask(scene);
    }

    const size_t first = out_result.size();
    out_result.insert(out_result.end(), meshes.begin(), meshes.end());
    for (size_t i = first; i < out_result.size(); ++i)
      out_result[i].volumeIndex = volume_index;
  };

  // The synchronous filter, for volumes the job did not gather.
  const auto processMeshes = [&](dag::ConstSpan<rendinst::riex_handle_t> handles, uint32_t volume_index, int mesh_lod,
                               const bbox3f *volume_bbox) {
    if (volume_placer_no_meshes)
      return;

    job.meshScratch.clear();
    const int firstUnprocessedRes = filter_volume_meshes(handles, mesh_lod, volume_bbox, riex_processor, job.meshScratch);
    emitMeshes(make_span_const(job.meshScratch), firstUnprocessedRes, mesh_lod, volume_index);
  };

  // Keep this outside any rendinst lock scope: the job's read locks queue behind writers.
  // Unconditional, so a job still in flight is drained even after the convar goes off.
  job.waitDone();

  bool gridAsync = false;
  float rangeScale = get_global_range_scale();
  vec4f cullViewPos = v_make_vec3f(viewport.worldPos.x, viewport.worldPos.y, viewport.worldPos.z);

  // Non-null exactly on the async path, so it alone says which gather this viewport reads from.
  const DagdpRiexGatherJob::PerViewportData *viewportData = nullptr;
  if (is_volume_early_riex_gather_enabled())
  {
    G_ASSERT(viewport_index < job.viewportCount);
    const auto &vd = job.viewportDataPool[viewport_index];
    if (!vd.valid)
      return;
    viewportData = &vd;

    gridAsync = job.gridGathered;
    if (gridAsync)
    {
      rangeScale = job.rangeScale;
      cullViewPos = vd.worldPos;
    }
  }

  Frustum frustum = viewport.frustum;
  Point4 worldPos(viewport.worldPos.x, viewport.worldPos.y, viewport.worldPos.z, 0.0f);
  auto range = v_splats(min(viewInfo.maxDrawDistance, viewport.maxDrawDistance) * rangeScale);
  shrink_frustum_zfar(frustum, v_ldu(&worldPos.x), range);

  bbox3f frustumBox;
  if (viewportData)
    frustumBox = viewportData->frustumBox;
  else
  {
    frustumBox = calc_frustum_box(frustum, viewport.worldPos, range);
    if (!v_test_xyz_finite(frustumBox.bmin) || !v_test_xyz_finite(frustumBox.bmax))
      return;
  }

  const auto volumeVisible = [&](const DagdpRiexGatherJob::VolumeEntry &e) {
    if (cascade_excluded(e.variant->csmCascadeCount, viewport.csmCascade))
      return false;

    // cull by placeables draw range first
    if (!volume_in_draw_range(e.center, e.extent2, cullViewPos, e.variant->maxDrawDistance * rangeScale))
      return false;

    // TODO: Performance: can cull more with more precise checks (original box/ellipsoid instead of bbox).
    return frustum.testBoxExtentB(v_add(e.center, e.center), e.extent2) != 0;
  };

  // Expects a prepared entry that passed the cull. Returns the volume index its meshes take.
  const auto emitVolume = [&](const DagdpRiexGatherJob::VolumeEntry &e) {
    debug_draw_volume(e.tm, e.scale, e.extent2, e.volumeType);

    const uint32_t volumeIndex = out_volumes.size();
    auto &volume = out_volumes.push_back();
    v_stu(&volume.itmRow0, e.itmRow0);
    v_stu(&volume.itmRow1, e.itmRow1);
    v_stu(&volume.itmRow2, e.itmRow2);
    volume.axis = e.axis;
    volume.volumeType = e.volumeType;
    volume.variantIndex = e.variant->variantIndex;

    if (!volume_placer_no_tiles)
      for (int x = e.tileMin.x; x < e.tileMax.x; ++x)
        for (int z = e.tileMin.y; z < e.tileMax.y; ++z)
        {
          auto &tile = out_tiles.push_back();
          tile.intPosXZ.x = x;
          tile.intPosXZ.y = z;
          tile.instancePosDelta = e.instancePosDelta;
          tile.volumeIndex = volumeIndex;
        }

    return volumeIndex;
  };

  const auto emitPreGatheredVolume = [&](const DagdpRiexGatherJob::VolumeEntry &e) {
    if (!volumeVisible(e))
      return;
    const uint32_t volumeIndex = emitVolume(e);
    emitMeshes(job.volumeMeshes(e), e.firstUnprocessedRes, resolve_mesh_lod(*e.variant, target_mesh_lod), volumeIndex);
  };

  const auto addVolumeSync = [&](const VolumeMappingItem &variant, const TMatrix &transform, float scale, int volume_type) {
    mat44f tm44;
    v_mat44_make_from_43cu_unsafe(tm44, transform.array);
    DagdpRiexGatherJob::VolumeEntry e{};
    init_volume_entry(e, &variant, transform, tm44, scale, volume_type, max_bounding_radius);
    if (!volumeVisible(e))
      return;
    prepare_volume_entry(e, tm44);

    // Reference: volumePlacerES.cpp.inl, VolumePlacer::gatherGeometryInBox
    rendinst::riex_collidable_t handles;
    rendinst::gatherRIGenExtraCollidableMin(handles, e.box, MIN_GEOMETRY_SIZE);
    const uint32_t volumeIndex = emitVolume(e);
    processMeshes(make_span_const(handles), volumeIndex, resolve_mesh_lod(variant, target_mesh_lod), &e.box);
  };

  if (gridAsync)
  {
    TIME_PROFILE(dagdp_volume_process_world_volumes_async);
    for (size_t i = 0; i < job.worldVolumeCount; ++i)
      emitPreGatheredVolume(job.volumePool[i]);
  }
  else
  {
    TIME_PROFILE(dagdp_volume_process_world_volumes_blocked);
    for_each_world_volume([&](ecs::EntityId placer_eid, const TMatrix &transform, float scale, int volume_type) {
      const auto iter = volume_mapping.find(placer_eid);
      if (iter != volume_mapping.end())
        addVolumeSync(iter->second, transform, scale, volume_type);
    });
  }

  const auto emitFullVolume = [&](const VolumeMappingItem &variant) {
    const uint32_t volumeIndex = out_volumes.size();
    auto &volume = out_volumes.push_back();
    v_stu(&volume.itmRow0, V_C_UNIT_1000);
    v_stu(&volume.itmRow1, V_C_UNIT_0100);
    v_stu(&volume.itmRow2, V_C_UNIT_0010);
    volume.axis = variant.axis;
    volume.volumeType = VOLUME_TYPE_FULL;
    volume.variantIndex = variant.variantIndex;
    return volumeIndex;
  };

  size_t onRiIdx = 0;
  size_t aroundRiIdx = 0;

  on_ri_placers_ecs_query(*g_entity_mgr, [&](ECS_REQUIRE(ecs::Tag dagdp_placer_on_ri) ecs::EntityId eid,
                                           const ecs::IntList &dagdp__resource_ids, int dagdp__csm_cascade_count) {
    const auto *variantPtr = placer_gather_variant(volume_mapping, eid, dagdp__csm_cascade_count, viewport.csmCascade);
    if (!variantPtr)
      return;
    const auto &variant = *variantPtr;

    const auto *rec = viewportData ? next_placer_record(viewportData->onRi, onRiIdx, eid) : nullptr;
    if (rec)
    {
      if (rec->handleCount == 0)
        return;
      const uint32_t volumeIndex = emitFullVolume(variant);
      emitMeshes(job.recordMeshes(*rec), rec->firstUnprocessedRes, rec->meshLod, volumeIndex);
      return;
    }

    rendinst::riex_collidable_t out_handles;
    Tab<rendinst::riex_handle_t> ri_handles;
    for (auto resIdx : dagdp__resource_ids)
    {
      rendinst::getRiGenExtraInstances(ri_handles, resIdx, frustumBox);
      out_handles.insert(out_handles.end(), ri_handles.begin(), ri_handles.end());
      ri_handles.clear(); // just to make sure, getRiGenExtraInstances() clears anyway
    }

    if (out_handles.empty())
      return;

    const uint32_t volumeIndex = emitFullVolume(variant);
    processMeshes(make_span_const(out_handles), volumeIndex, resolve_mesh_lod(variant, target_mesh_lod), nullptr);
  });

  around_ri_placers_ecs_query(*g_entity_mgr,
    [&](ECS_REQUIRE(ecs::Tag dagdp_placer_around_ri) ecs::EntityId eid, const ecs::IntList &dagdp__resource_ids,
      const ecs::EidList &dagdp__volume_box_eids, const ecs::EidList &dagdp__volume_cylinder_eids,
      const ecs::EidList &dagdp__volume_sphere_eids, int dagdp__csm_cascade_count) {
      const auto *variant = placer_gather_variant(volume_mapping, eid, dagdp__csm_cascade_count, viewport.csmCascade);
      if (!variant)
        return;

      const auto *rec = viewportData ? next_placer_record(viewportData->aroundRi, aroundRiIdx, eid) : nullptr;

      // Without a matching record the pool indices mean nothing: gather this placer synchronously.
      if (gridAsync && rec != nullptr)
      {
        for (size_t localIndex = 0; localIndex < rec->localVolumeCount; ++localIndex)
          for (const auto &e : job.localVolumeSources(job.localVolumePool[rec->firstLocalVolume + localIndex]))
            emitPreGatheredVolume(e);
        return;
      }

      LocalVolumeSnapshots localVolumes;
      const float maxLocalRad =
        collect_local_volumes(dagdp__volume_box_eids, dagdp__volume_cylinder_eids, dagdp__volume_sphere_eids, localVolumes);
      const bbox3f fbox = extend_around_ri_fbox(frustumBox, maxLocalRad);

      dag::Vector<TMatrix, framemem_allocator> sources;
      gather_rigen_source_tms(fbox, dagdp__resource_ids, sources);

      const auto appendSourceTms = [&sources](dag::ConstSpan<rendinst::riex_handle_t> handles) {
        rendinst::ScopedRIExtraReadLock rd;
        for (auto handle : handles)
        {
          mat44f m44;
          rendinst::getRIGenExtra44NoLock(handle, m44);
          v_mat_43cu_from_mat44(sources.push_back().array, m44);
        }
      };

      if (rec)
        for (size_t i = 0; i < rec->entryCount; ++i)
          appendSourceTms(make_span_const(job.entryPool[rec->firstEntry + i].handles));
      else
      {
        Tab<rendinst::riex_handle_t> ri_handles;
        for (auto resIdx : dagdp__resource_ids)
        {
          rendinst::getRiGenExtraInstances(ri_handles, resIdx, fbox);
          appendSourceTms(make_span_const(ri_handles));
        }
      }

      for (const auto &local : localVolumes)
        for (const TMatrix &sourceTm : sources)
          addVolumeSync(*variant, sourceTm * local.transform, local.scale, local.volumeType);
    });

  if (viewportData)
  {
    G_ASSERT(onRiIdx == viewportData->onRi.size());
    G_ASSERT(aroundRiIdx == viewportData->aroundRi.size());
  }

  // out_result holds this viewport's meshes; the other two accumulate over the view's viewports.
  DA_PROFILE_TAG(dagdp_volume_gather_process, "meshes=%d volumes_total=%d tiles_total=%d", (int)out_result.size(),
    (int)out_volumes.size(), (int)out_tiles.size());
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
