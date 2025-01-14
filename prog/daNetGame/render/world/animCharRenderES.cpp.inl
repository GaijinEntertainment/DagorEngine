// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
// #include <ecs/core/attributeEx.h>
#include <ecs/render/updateStageRender.h>
#include "render/renderEvent.h"
#include <ecs/anim/anim.h>
#include <perfMon/dag_statDrv.h>
#include <animChar/dag_animCharVis.h>
#include <startup/dag_globalSettings.h>
#include <gamePhys/collision/collisionLib.h>
#include <shaders/dag_shaderBlock.h>
#include "game/player.h"
#include "game/gameEvents.h"
#include <util/dag_threadPool.h>
#include <util/dag_convar.h>
#include <util/dag_finally.h>
#include <util/dag_console.h>

#define INSIDE_RENDERER 1 // fixme: move to jam

#include "animCharRenderUtil.h"
#include <shaders/dag_dynSceneRes.h>
#include "private_worldRenderer.h"
#include "global_vars.h"
#include "dynModelRenderer.h"
#include <ecs/anim/animchar_visbits.h>
#include <ecs/core/utility/ecsRecreate.h>

// for console command
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <shaders/dag_shaderVarsUtils.h>

#include <render/world/dynModelRenderPass.h>
#include <render/world/wrDispatcher.h>
#include <render/world/shadowsManager.h>
#include <drv/3d/dag_matricesAndPerspective.h>


ECS_REGISTER_EVENT(AnimcharRenderAsyncEvent)

template <typename Callable>
static inline void attach_bbox_bsph_ecs_query(ecs::EntityId eid, Callable fn);
template <typename Callable>
static inline void attach_bsph_ecs_query(ecs::EntityId eid, Callable fn);
template <typename Callable>
inline void vehicle_cockpit_show_hide_ecs_query(Callable c);
template <typename Callable>
inline void animchar_csm_distance_ecs_query(Callable ECS_CAN_PARALLEL_FOR(c, 4));
template <typename Callable>
inline void animchar_csm_visibility_ecs_query(Callable ECS_CAN_PARALLEL_FOR(c, 50));
template <typename Callable>
inline void animchar_attaches_inherit_visibility_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
inline void draw_shadow_occlusion_boxes_ecs_query(Callable c);
template <typename Callable>
inline void process_animchar_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
inline void count_animchar_renderer_ecs_query(Callable c);
template <typename Callable>
inline void gather_animchar_renderer_ecs_query(Callable c);

class AnimCharShadowOcclusionManager;
AnimCharShadowOcclusionManager *get_animchar_shadow_occlusion();
bool is_bbox_visible_in_shadows(const AnimCharShadowOcclusionManager *manager, bbox3f_cref bbox);

extern int dynamicSceneTransBlockId, dynamicSceneBlockId, dynamicDepthSceneBlockId;

using namespace dynmodel_renderer;

VECTORCALL DAGOR_NOINLINE static vec4f add_three(vec4f a, vec4f b, vec4f c)
{
  vec4f ab = v_add(a, b);
  return v_add(ab, c);
}

static void update_geom_tree(AnimV20::AnimcharRendComponent &animchar_render,
  const AnimcharNodesMat44 &animchar_node_wtm,
  vec4f negative_rounded_cam_pos,
  vec4f negative_remainder_cam_pos)
{
  DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
  dag::ConstSpan<mat44f> nodeWtm = animchar_node_wtm.nwtm;

  // we subtract difference between accurate double camera position and camera position in floats (used in render)

  // vec3f ofs = v_sub(animchar_node_wtm.wofs, rounded_cam_pos);
  // ofs = v_sub(ofs, remainder_cam_pos);
  // should be accurate enough.
  // However, with fast-math enabled, clang replaces two ordered sub_ps with unordered add, basically calculating
  // (rounded_cam_pos+remainder_cam_pos) first that is not only slower (more instructions), but also leads to inaccurate calculations
  // unless we switch off fast-math, which isn't easy to do, we should workaround it
  //-ffast-math (or fp:fast) in clang _driver_ enables following optimization in clang frontend:
  //   -fassociative-math -freciprocal-math -fno-math-errno -ffinite-math-only -fno-trapping-math -fno-signed-zeros -ffp-contract=fast
  //   -funsafe-math-optimizations
  // and, additionally -ffast-math (in clang frontend), which is different optimization!
  // unfortunately there is no way to switch it off. If we enable all the optimization above, clang driver discover it and
  // automatically adds -ffast-math the only way (as it seems) is to get clang frontend command line (-### flag) and directly invoke
  // clang frontend with that command-line - but without "-ffast-math" or, alternatively disable one of the optimizations (i.e.
  // -fno-signed-zeros) as for now we work-around this using NEGATIVES. clang has no point to 'optimize' anything with that vec3f ofs =
  // v_add(animchar_node_wtm.wofs, negative_rounded_cam_pos);
  vec3f ofs = add_three(animchar_node_wtm.wofs, negative_rounded_cam_pos, negative_remainder_cam_pos);
  for (auto &n : animchar_render.getNodeMap().map())
  {
    mat44f m4 = nodeWtm[n.nodeIdx.index()];
    m4.col3 = v_add(m4.col3, ofs);
    scene->setNodeWtm(n.id, m4);
  }
}

static void prepare_for_render(AnimV20::AnimcharRendComponent &animchar_render)
{
  DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
  G_ASSERT_RETURN(scene != nullptr, );

  scene->savePrevNodeWtm();
  animchar_render.setSceneBeforeRenderCalled(false); // so we can still call animchar.render()
  scene->clearNodeCollapser();
}


template <class T>
inline void animchar_render_objects_prepare_ecs_query(T ECS_CAN_PARALLEL_FOR(b, 4));
template <class T>
inline void animchar_process_objects_in_shadow_ecs_query(T);
template <class T>
inline void animchar_render_update_lod_ecs_query(T);
template <typename T>
static inline void update_animchar_hmap_deform_ecs_query(T cb);

ECS_TAG(render)
ECS_NO_ORDER
static __forceinline void animchar_before_render_es(const UpdateStageInfoBeforeRender &stg)
{
  const Point3 &lodChoosePos = stg.camPos;
  vec4f camPos = v_ldu(&lodChoosePos.x);
  bbox3f activeShadowVolume = WRDispatcher::getShadowsManager().getActiveShadowVolume();
  bool activeShadowVolumeUsed = !v_bbox3_is_empty(activeShadowVolume);
  if (activeShadowVolumeUsed && stg.mainCullingFrustum.testBox(activeShadowVolume.bmin, activeShadowVolume.bmax) == Frustum::INSIDE)
  {
    // Full shadow volume is contained within the main view -> shadow volume won't contribute anything to the result
    activeShadowVolumeUsed = false;
  }

  vec4f dirFromSunV = v_ldu(&stg.dirFromSun.x);

  animchar_render_objects_prepare_ecs_query(
    [&](AnimV20::AnimcharRendComponent &animchar_render, const AnimcharNodesMat44 &animchar_node_wtm,
      const float &animchar_render__dist_sq, vec3f &animchar_render__root_pos, bbox3f *animchar_attaches_bbox,
      const bbox3f *animchar_attaches_bbox_precalculated, vec4f &animchar_bsph, const vec4f *animchar_bsph_precalculated,
      const BBox3 *animchar_bbox_precalculated, const BBox3 *animchar_shadow_cull_bbox_precalculated, bbox3f &animchar_bbox,
      bbox3f &animchar_shadow_cull_bbox, uint8_t &animchar_visbits, float &animchar_render__shadow_cast_dist,
      bool animchar__usePrecalculatedData = false, bool animchar_render__enabled = true,
      const ecs::Tag *animchar__actOnDemand = nullptr, bool animchar__updatable = true, float animchar_extra_culling_dist = 100,
      bool animchar__use_precise_shadow_culling = false) {
      if (animchar__usePrecalculatedData)
      {
        // note: this data already calculated in skeleton_attach_es (no reason to calculate it again for attaches)
        if (animchar_bsph_precalculated)
          animchar_bsph = *animchar_bsph_precalculated;
        if (animchar_bbox_precalculated)
          animchar_bbox = v_ldu_bbox3(*animchar_bbox_precalculated);
        if (animchar_shadow_cull_bbox_precalculated)
          animchar_shadow_cull_bbox = v_ldu_bbox3(*animchar_shadow_cull_bbox_precalculated);
        if (animchar_attaches_bbox && animchar_attaches_bbox_precalculated)
          *animchar_attaches_bbox = *animchar_attaches_bbox_precalculated;
      }
      animchar_visbits = 0;
      if (!animchar_render__enabled)
        return;
      const float dist2 = v_extract_x(v_length3_sq_x(v_sub(camPos, animchar_render__root_pos))); // shadows for 100meters.
      float scaledDist2 = dist2 * stg.mainInvHkSq;
      if (scaledDist2 >= animchar_render__dist_sq)
        return;
      animchar_visbits |= VISFLG_WITHIN_RANGE;
      // note: prepareSphere and calculating bbox for entities with attaches_list performed in skeleton_attach_es
      if (!animchar__usePrecalculatedData)
      {
        animchar_bsph = animchar_render.prepareSphere(animchar_node_wtm);
        animchar_render.calcWorldBox(animchar_bbox, animchar_node_wtm, true);
        // For culling in shadows it's more appropriate to use coarse, but smaller bbox.
        // Rare cases, when something is being culled, will be hard to notice, but culling this way is more effective.
        animchar_render.calcWorldBox(animchar_shadow_cull_bbox, animchar_node_wtm, false);
      }

      if (animchar__use_precise_shadow_culling)
      {
        // TODO: just use an infinite cylinder frustum test, the distance check above culls it anyway
        // the distance doesn't really matter (we could even avoid the sqrt)
        vec4f sweptSphereDist = v_sqrt(v_splats(animchar_render__dist_sq));
        if (!stg.mainCullingFrustum.testSweptSphere(animchar_bsph, sweptSphereDist, dirFromSunV))
          return;
      }
      else
      {
        // instead of testing swept sphere, we use an ugly extended bbox test
        vec4f extendedSphere = v_madd(dirFromSunV, v_splats(animchar_extra_culling_dist), animchar_bsph);
        bbox3f box;
        box.bmin = box.bmax = extendedSphere;
        v_bbox3_add_pt(box, v_sub(animchar_bsph, v_splat_w(animchar_bsph)));
        v_bbox3_add_pt(box, v_add(animchar_bsph, v_splat_w(animchar_bsph)));
        if (!stg.mainCullingFrustum.testBoxB(box.bmin, box.bmax))
          return;
      }

      animchar_visbits |= VISFLG_MAIN_AND_SHADOW_VISIBLE;
      if (animchar__actOnDemand || animchar__updatable)
        animchar_render__shadow_cast_dist = -1.f;

      prepare_for_render(animchar_render);
      DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
      G_ASSERT_RETURN(scene != nullptr, );
      scene->chooseLodByDistSq(v_extract_x(v_length3_sq_x(v_sub(camPos, animchar_bsph))));

      update_geom_tree(animchar_render, animchar_node_wtm, stg.negRoundedCamPos, stg.negRemainderCamPos);
    });

  if (activeShadowVolumeUsed) // it's practically for dynamic lights only
  {
    animchar_process_objects_in_shadow_ecs_query(
      [&](AnimV20::AnimcharRendComponent &animchar_render, const AnimcharNodesMat44 &animchar_node_wtm, bbox3f &animchar_bbox,
        const vec4f &animchar_bsph, uint8_t &animchar_visbits, bool animchar_render__enabled = true) {
        if (!animchar_render__enabled || (animchar_visbits & VISFLG_MAIN_AND_SHADOW_VISIBLE) ||
            !v_bbox3_test_box_intersect(activeShadowVolume, animchar_bbox))
          return;

        animchar_visbits |= VISFLG_MAIN_AND_SHADOW_VISIBLE;

        // we have to call it here, as otherwise node collapser won't work // probably no longer true
        prepare_for_render(animchar_render);

        DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
        G_ASSERT_RETURN(scene != nullptr, );
        scene->chooseLodByDistSq(v_extract_x(v_length3_sq_x(v_sub(camPos, animchar_bsph))));

        update_geom_tree(animchar_render, animchar_node_wtm, stg.negRoundedCamPos, stg.negRemainderCamPos);
      });
  }

  if (auto hmapDeformRectOpt = WRDispatcher::getHmapDeformRect())
  {
    TIME_PROFILE(deform_hmap_update_geom_tree)
    vec4f hmapDeformRect = // same as in RenderHmapDeform
      v_make_vec4f(hmapDeformRectOpt->z, hmapDeformRectOpt->w, -hmapDeformRectOpt->x, -hmapDeformRectOpt->y);
    update_animchar_hmap_deform_ecs_query(
      [&](ECS_REQUIRE_NOT(const Point3 semi_transparent__placingColor) ECS_REQUIRE_NOT(ecs::Tag invisibleUpdatableAnimchar)
            AnimV20::AnimcharRendComponent &animchar_render,
        const AnimcharNodesMat44 &animchar_node_wtm, const bbox3f &animchar_bbox, uint8_t &animchar_visbits,
        const ecs::Tag *animchar__actOnDemand, const ecs::Tag *slot_attach, bool animchar__updatable = true) {
        if ((!animchar__updatable || animchar__actOnDemand != nullptr) && slot_attach == nullptr)
          return;
        bool isInDeformRect =
          v_signmask(v_cmp_gt(hmapDeformRect, v_perm_xzac(animchar_bbox.bmin, v_neg(animchar_bbox.bmax)))) == 0b1111;
        if (!isInDeformRect)
          return;
        animchar_visbits |= VISFLG_RENDER_CUSTOM;
        if (animchar_visbits & VISFLG_MAIN_AND_SHADOW_VISIBLE)
          ; // Skip if it was already updated
        else
          update_geom_tree(animchar_render, animchar_node_wtm, stg.negRoundedCamPos, stg.negRemainderCamPos);
      });
  }
}

void preprocess_visible_animchars_in_frustum(
  // Before render stage is needed, because it holds the correct rounded camera positions (those are NOT the same as the eye pos)
  const UpdateStageInfoBeforeRender &stg,
  const Frustum &frustum,
  const vec3f &eye_pos,
  AnimV20::AnimcharRendComponent &animchar_render,
  const AnimcharNodesMat44 &animchar_node_wtm,
  const vec4f &animchar_bsph,
  uint8_t &animchar_visbits,
  bool animchar_render__enabled)
{
  if (animchar_visbits & VISFLG_MAIN_AND_SHADOW_VISIBLE)
    return; // It's already updated
  if (!animchar_render__enabled || !(animchar_visbits & VISFLG_WITHIN_RANGE))
    return;
  // animchar_bsph is already calculated if the animchar is within range
  if (!frustum.testSphereB(animchar_bsph, v_splat_w(animchar_bsph)))
    return;

  animchar_visbits |= VISFLG_RENDER_CUSTOM;

  // we have to call it here, as otherwise node collapser won't work // probably no longer true
  prepare_for_render(animchar_render);

  DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
  G_ASSERT_RETURN(scene != nullptr, );
  scene->chooseLodByDistSq(v_extract_x(v_length3_sq_x(v_sub(eye_pos, animchar_bsph))));

  update_geom_tree(animchar_render, animchar_node_wtm, stg.negRoundedCamPos, stg.negRemainderCamPos);
}

template <typename T>
static inline void gather_animchar_ecs_query(T cb);
template <typename T>
static inline void ignore_occlusion_visibility_ecs_query(T cb);

static float get_shadow_distance(const Point3 &from_sun_dir, bbox3f_cref object_bbox, float csm_shadows_max_dist)
{
  vec3f topCenterVec = v_mul(v_add(object_bbox.bmax, v_perm_xbzw(object_bbox.bmin, object_bbox.bmax)), v_splats(0.5));
  Point3_vec4 topCenter;
  v_st(&topCenter, topCenterVec);
  float traceDist = csm_shadows_max_dist;
  if (!dacoll::traceray_normalized_lmesh(topCenter, from_sun_dir, traceDist, nullptr, nullptr))
    return -1.0;
  return traceDist;
}

static bool is_shadow_volume_occluded(
  const Occlusion &occlusion, const Point3 &from_sun_dir, bbox3f_cref object_bbox, float trace_dist)
{
  vec3f fromSunDir = v_mul(v_ldu(reinterpret_cast<const float *>(&from_sun_dir)), v_splats(trace_dist));
  vec3f bottomCenterVec = v_mul(v_add(object_bbox.bmin, v_perm_xbzw(object_bbox.bmax, object_bbox.bmin)), v_splats(0.5));
  bbox3f extendedBbox = object_bbox;
  v_bbox3_add_pt(extendedBbox, v_add(fromSunDir, bottomCenterVec));
  return !occlusion.isVisibleBox(extendedBbox.bmax, extendedBbox.bmin);
}

void update_csm_length(const Frustum &frustum, const Point3 &dir_from_sun, float csm_shadows_max_dist)
{
  TIME_D3D_PROFILE(update_csm_length);
  const AnimCharShadowOcclusionManager *animCharShadowOcclMgr = get_animchar_shadow_occlusion();
  animchar_csm_distance_ecs_query(
    [&](ECS_REQUIRE_NOT(ecs::Tag cockpitEntity) uint8_t &animchar_visbits, const vec4f &animchar_bsph, const bbox3f &animchar_bbox,
      const bbox3f &animchar_shadow_cull_bbox, float &animchar_render__shadow_cast_dist, const ecs::EidList *attaches_list,
      ecs::EntityId slot_attach__attachedTo = ecs::INVALID_ENTITY_ID) {
      if (!(animchar_visbits & VISFLG_MAIN_AND_SHADOW_VISIBLE) || !!slot_attach__attachedTo)
        return;
      if (!frustum.testSphereB(animchar_bsph, v_splat_w(animchar_bsph)) ||
          !is_bbox_visible_in_shadows(animCharShadowOcclMgr, animchar_shadow_cull_bbox))
      {
        animchar_visbits &= ~VISFLG_MAIN_AND_SHADOW_VISIBLE;
        animchar_visbits |= VISFLG_MAIN_VISIBLE;
        return;
      }
      if (animchar_render__shadow_cast_dist < 0.0f)
      {
        bbox3f extendedBbox = animchar_bbox;
        if (attaches_list)
        {
          vec3f offset = v_splats(1.5f);
          extendedBbox.bmin = v_sub(extendedBbox.bmin, offset);
          extendedBbox.bmax = v_add(extendedBbox.bmax, offset);
        }
        animchar_render__shadow_cast_dist = get_shadow_distance(dir_from_sun, extendedBbox, csm_shadows_max_dist);
      }
    });
}

void compute_csm_visibility(const Occlusion &occlusion, const Point3 &dir_from_sun)
{
  TIME_D3D_PROFILE(csm_occlusion);
  animchar_csm_visibility_ecs_query(
    [&](ECS_REQUIRE_NOT(ecs::Tag cockpitEntity) ECS_REQUIRE_NOT(ecs::Tag attachedToParent) uint8_t &animchar_visbits,
      const bbox3f &animchar_bbox, const bbox3f *animchar_attaches_bbox, const float &animchar_render__shadow_cast_dist,
      const ecs::EidList *attaches_list, const ecs::EntityId *slot_attach__attachedTo) {
      if (animchar_render__shadow_cast_dist < 0.0f ||
          (slot_attach__attachedTo && *slot_attach__attachedTo /* ignore non roots of attaches */) ||
          !(animchar_visbits & VISFLG_MAIN_AND_SHADOW_VISIBLE))
        return;
      bbox3f extendedBbox = animchar_attaches_bbox ? *animchar_attaches_bbox : animchar_bbox;
      if (is_shadow_volume_occluded(occlusion, dir_from_sun, extendedBbox, animchar_render__shadow_cast_dist))
      {
        animchar_visbits &= ~VISFLG_MAIN_AND_SHADOW_VISIBLE;
        if (attaches_list)
          for (ecs::EntityId aeid : *attaches_list)
            animchar_attaches_inherit_visibility_ecs_query(aeid,
              [&](uint8_t &animchar_visbits) { animchar_visbits &= ~VISFLG_MAIN_AND_SHADOW_VISIBLE; });
      }
    });
}

void debug_draw_shadow_occlusion_bboxes(const Point3 &dir_from_sun, bool final_extend)
{
  draw_shadow_occlusion_boxes_ecs_query(
    [&](const uint8_t &animchar_visbits, bbox3f &animchar_bbox, float &animchar_render__shadow_cast_dist,
      const bbox3f *animchar_attaches_bbox, const ecs::EntityId *slot_attach__attachedTo) {
      if (animchar_render__shadow_cast_dist < 0.0f || !(animchar_visbits & VISFLG_MAIN_AND_SHADOW_VISIBLE) ||
          (slot_attach__attachedTo && *slot_attach__attachedTo != ecs::INVALID_ENTITY_ID))
        return;

      bbox3f extendedBbox = animchar_attaches_bbox ? *animchar_attaches_bbox : animchar_bbox;
      if (final_extend)
      {
        vec3f fromSunDir = v_mul(v_ldu(reinterpret_cast<const float *>(&dir_from_sun)), v_splats(animchar_render__shadow_cast_dist));
        vec3f bottomCenterVec = v_mul(v_add(extendedBbox.bmin, v_perm_xbzw(extendedBbox.bmax, extendedBbox.bmin)), v_splats(0.5));
        v_bbox3_add_pt(extendedBbox, v_add(fromSunDir, bottomCenterVec));
      }
      begin_draw_cached_debug_lines(false, false);
      BBox3 resBox;
      v_stu_bbox3(resBox, extendedBbox);
      draw_cached_debug_box(resBox, E3DCOLOR(255, 0, 0));

      end_draw_cached_debug_lines();
      set_cached_debug_lines_wtm(TMatrix::IDENT);
    });
}

template <class T>
inline void gather_animchar_async_ecs_query(T b);

ECS_TAG(render)
ECS_NO_ORDER
static void animchar_render_opaque_async_es(const AnimcharRenderAsyncEvent &stg)
{
  DynModelRenderingState &state = stg.state;

  const uint8_t add_vis_bits = stg.add_vis_bits;
  const uint8_t check_bits = stg.check_bits;
  // for shadows we use imprecise 'just sphere' culling.
  // we can test box, but usually sphere is good enough
  const bool needPreviousMatrices = stg.needPrevious;

  //< store visibility result for render trans
  const Occlusion *occlusion = stg.occlusion;

  // TODO compare with gather_animchar_ecs_query and separate setting threshold to independent system
  gather_animchar_async_ecs_query([&](ECS_REQUIRE_NOT(const Point3 semi_transparent__placingColor) ecs::EntityId eid,
                                    const ecs::Tag *invisibleUpdatableAnimchar, uint8_t &animchar_visbits, const vec4f &animchar_bsph,
                                    const bbox3f &animchar_bbox, const bbox3f *animchar_attaches_bbox,
                                    const AnimV20::AnimcharRendComponent &animchar_render, const ecs::Point4List *additional_data,
                                    const ecs::UInt8List *animchar_render__nodeVisibleStgFilters,
                                    bool animchar__renderPriority = false, bool needImmediateConstPS = false) {
    if (
      stg.eidFilter > AnimcharRenderAsyncFilter::ARF_ANY_IDX &&
      (hash_int(ecs::entity_id_t(eid)) & ecs::entity_id_t(AnimcharRenderAsyncFilter::ARF_IDX_MASK)) != ecs::entity_id_t(stg.eidFilter))
      return;

    // Check visibility
    if (!(interlocked_relaxed_load(animchar_visbits) & check_bits) ||
        !stg.cullingFrustum.testSphereB(animchar_bsph, v_splat_w(animchar_bsph)) ||
        (occlusion && !occlusion->isVisibleBox(animchar_attaches_bbox ? animchar_attaches_bbox->bmin : animchar_bbox.bmin,
                        animchar_attaches_bbox ? animchar_attaches_bbox->bmax : animchar_bbox.bmax)))
      return;
    interlocked_or(animchar_visbits, add_vis_bits);
    if (invisibleUpdatableAnimchar)
      return;
    const DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
    G_ASSERT_RETURN(scene != nullptr, );

    // add to render list, and process bones
    const bool noAdditionalData = additional_data == nullptr || additional_data->empty();
    const Point4 *additionalData = noAdditionalData ? ZERO_PTR<Point4>() : additional_data->data();
    const uint32_t additionalDataSize = noAdditionalData ? 1 : additional_data->size();
    const uint8_t *filter = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->begin() : nullptr;
    const uint32_t pathFilterSize = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->size() : 0;
    G_ASSERT(!animchar_render__nodeVisibleStgFilters || pathFilterSize == scene->getNodeCount());
    state.process_animchar(0, ShaderMesh::STG_imm_decal, scene, additionalData, additionalDataSize, needPreviousMatrices, nullptr,
      filter, pathFilterSize, stg.filterMask, needImmediateConstPS,
      animchar__renderPriority ? dynmodel_renderer::RenderPriority::HIGH : dynmodel_renderer::RenderPriority::DEFAULT,
      stg.globVarsState, stg.texCtx);
  });
}

template <class ConstState>
static void render_state(ConstState &state, dynmodel_renderer::BufferType buf_type, const TMatrix &vtm, int block)
{
  if (!eastl::is_const_v<eastl::remove_reference_t<ConstState>>) // only if not const
    const_cast<DynModelRenderingState &>(state).prepareForRender();

  const DynamicBufferHolder *buffer = state.requestBuffer(buf_type);
  if (!buffer)
    return;

  d3d::settm(TM_VIEW, vtm);

  state.setVars(buffer->buffer.getBufId());
  SCENE_LAYER_GUARD(block);
  state.render(buffer->curOffset);
}

static struct AnimcharRenderMainJob final : public cpujobs::IJob
{
  static inline int jobsLeft = 0;
  AnimcharRenderAsyncFilter flt;
  uint32_t hints;
  const Frustum *frustum; // Note: points to `current_camera` blob within daBfg
  dynmodel_renderer::DynModelRenderingState *dstate;
  static inline GlobalVariableStates globalVarsState;
  TexStreamingContext texCtx = TexStreamingContext(0);
  void doJob() override;
} animchar_render_main_jobs[AnimcharRenderAsyncFilter::ARF_IDX_COUNT];
static uint32_t animchar_render_main_jobs_tpqpos = 0;

void AnimcharRenderMainJob::doJob()
{
  if (this == animchar_render_main_jobs) // First one wake others up
    threadpool::wake_up_all();
  G_FAST_ASSERT(dstate && frustum);
  const uint8_t add_vis_bits = VISFLG_MAIN_VISIBLE | VISFLG_MAIN_CAMERA_RENDERED;
  const uint8_t check_bits = VISFLG_MAIN_AND_SHADOW_VISIBLE | VISFLG_MAIN_VISIBLE;
  const uint8_t filterMask = UpdateStageInfoRender::RENDER_MAIN;
  g_entity_mgr->broadcastEventImmediate(
    AnimcharRenderAsyncEvent(*dstate, &globalVarsState, current_occlusion, *frustum, add_vis_bits, check_bits, filterMask,
      /*needPreviousMatrices*/ (hints & UpdateStageInfoRender::RENDER_MOTION_VECS) != 0, flt, texCtx));
  // To consider: pre-sort partially here to make final sort faster?
  if (interlocked_decrement(jobsLeft) == 0) // Last one finalizes to 0th
  {
    for (int n = 1; n < AnimcharRenderAsyncFilter::ARF_IDX_COUNT; n++)
      animchar_render_main_jobs[0].dstate->addStateFrom(eastl::move(*animchar_render_main_jobs[n].dstate));
    animchar_render_main_jobs[0].dstate->prepareForRender();
  }
}

extern const char *fmt_csm_render_pass_name(int cascade, char tmps[], int csm_task_id = 0);

void start_async_animchar_main_render(const Frustum &fr, uint32_t hints, TexStreamingContext texCtx)
{
  G_ASSERT(g_entity_mgr->isConstrainedMTMode());

  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
  G_ASSERT(
    animchar_render_main_jobs[0].globalVarsState.empty() /* Expected to be cleared by previous `wait_async_animchar_main_render` */);
  animchar_render_main_jobs[0].globalVarsState.set_allocator(framemem_ptr());
  copy_current_global_variables_states(animchar_render_main_jobs[0].globalVarsState);

  interlocked_relaxed_store(AnimcharRenderMainJob::jobsLeft, AnimcharRenderAsyncFilter::ARF_IDX_COUNT);
  char tmps[] = "csm#000";
  int i = 0;
  for (auto &job : animchar_render_main_jobs)
  {
    job.flt = AnimcharRenderAsyncFilter(i);
    job.hints = hints;
    job.frustum = &fr;
    job.dstate = dynmodel_renderer::create_state(fmt_csm_render_pass_name(i++, tmps));
    job.texCtx = texCtx;
    using namespace threadpool;
    add(&job, PRIO_HIGH, animchar_render_main_jobs_tpqpos, AddFlags::None);
  }
  threadpool::wake_up_one();
}

void wait_async_animchar_main_render()
{
  for (int j = AnimcharRenderAsyncFilter::ARF_IDX_COUNT - 1; j >= 0; j--)
  {
    AnimcharRenderMainJob &job = animchar_render_main_jobs[j];
    if (!interlocked_acquire_load(job.done))
    {
      TIME_PROFILE_DEV(wait_animchar_async_render_main);
      if (j == AnimcharRenderAsyncFilter::ARF_IDX_COUNT - 1) // Last
        threadpool::barrier_active_wait_for_job(&job, threadpool::PRIO_HIGH, animchar_render_main_jobs_tpqpos);
      for (int i = j; i >= 0; i--)
        threadpool::wait(&animchar_render_main_jobs[i]);
      break;
    }
  }
  G_ASSERT(!interlocked_relaxed_load(AnimcharRenderMainJob::jobsLeft));
  auto &job = animchar_render_main_jobs[0];
  G_ASSERT(job.globalVarsState.get_allocator() == framemem_ptr() || job.globalVarsState.empty());
  job.globalVarsState.clear(); // Free framemem allocated data
}

ECS_TAG(render)
ECS_NO_ORDER
static void animchar_render_opaque_es(const UpdateStageInfoRender &stg)
{
  if ((stg.hints & stg.RENDER_MAIN) && stg.pState)
  {
    TIME_D3D_PROFILE(animchar_render_sync);
    TMatrix vtm = stg.viewTm;
    vtm.setcol(3, 0, 0, 0);
    render_state(*stg.pState, dynmodel_renderer::get_buffer_type_from_render_pass(stg.renderPass), vtm,
      (stg.hints & UpdateStageInfoRender::RENDER_COLOR) ? dynamicSceneBlockId : dynamicDepthSceneBlockId);
    d3d::settm(TM_VIEW, stg.viewTm);
    return;
  }
  TIME_D3D_PROFILE(animchar_render);

  const uint32_t mainHints = (stg.RENDER_MAIN | stg.RENDER_COLOR);
  const uint8_t add_vis_bits = (stg.hints & (mainHints | stg.RENDER_SHADOW)) == mainHints
                                 ? VISFLG_MAIN_VISIBLE | VISFLG_MAIN_CAMERA_RENDERED
                               : stg.hints & stg.RENDER_SHADOW ? VISFLG_CSM_SHADOW_RENDERED
                                                               : 0;
  const uint8_t check_bits =
    (stg.hints & UpdateStageInfoRender::RENDER_MAIN)
      ? ((stg.hints & UpdateStageInfoRender::RENDER_SHADOW) ? VISFLG_MAIN_AND_SHADOW_VISIBLE
                                                            : (VISFLG_MAIN_AND_SHADOW_VISIBLE | VISFLG_MAIN_VISIBLE))
      : VISFLG_WITHIN_RANGE | VISFLG_COCKPIT_VISIBLE;
  // for shadows we use imprecise 'just sphere' culling.
  // we can test box, but usually sphere is good enough

  const bool needPreviousMatrices = (stg.hints & UpdateStageInfoRender::RENDER_MOTION_VECS) ? true : false;

  //< store visibility result for render trans
  const Occlusion *occlusion = (stg.hints & stg.RENDER_SHADOW) ? NULL : stg.occlusion;
  DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();

  // TODO compare with gather_animchar_ecs_query and separate setting threshold to independent system
  gather_animchar_ecs_query(
    [&](ECS_REQUIRE_NOT(const Point3 semi_transparent__placingColor) uint8_t &animchar_visbits, const vec4f &animchar_bsph,
      const bbox3f &animchar_bbox, const bbox3f *animchar_attaches_bbox, const AnimV20::AnimcharRendComponent &animchar_render,
      const ecs::Point4List *additional_data, const ecs::UInt8List *animchar_render__nodeVisibleStgFilters,
      const ecs::Tag *invisibleUpdatableAnimchar, bool animchar__renderPriority = false, bool needImmediateConstPS = false) {
      if (!(animchar_visbits & check_bits) || !stg.cullingFrustum.testSphereB(animchar_bsph, v_splat_w(animchar_bsph)) ||
          (occlusion && !occlusion->isVisibleBox(animchar_attaches_bbox ? animchar_attaches_bbox->bmin : animchar_bbox.bmin,
                          animchar_attaches_bbox ? animchar_attaches_bbox->bmax : animchar_bbox.bmax)))
        return;

      animchar_visbits |= add_vis_bits;

      if (invisibleUpdatableAnimchar)
        return;

      const DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
      G_ASSERT_RETURN(scene != nullptr, );

      // Render
      const bool noAdditionalData = additional_data == nullptr || additional_data->empty();
      const Point4 *additionalData = noAdditionalData ? ZERO_PTR<Point4>() : additional_data->data();
      const uint32_t additionalDataSize = noAdditionalData ? 1 : additional_data->size();
      const uint8_t *filter = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->begin() : nullptr;
      const uint32_t pathFilterSize = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->size() : 0;
      G_ASSERT(!animchar_render__nodeVisibleStgFilters || pathFilterSize == scene->getNodeCount());
      state.process_animchar(0, ShaderMesh::STG_imm_decal, scene, additionalData, additionalDataSize, needPreviousMatrices, nullptr,
        filter, pathFilterSize, stg.hints & (stg.RENDER_SHADOW | stg.RENDER_MAIN | stg.FORCE_NODE_COLLAPSER_ON), needImmediateConstPS,
        animchar__renderPriority ? dynmodel_renderer::RenderPriority::HIGH : dynmodel_renderer::RenderPriority::DEFAULT, nullptr,
        stg.texCtx);
    });

  if (state.empty())
    return;
  TMatrix vtm;
  if (stg.hints & stg.RENDER_MAIN)
  {
    vtm = stg.viewTm;
    vtm.setcol(3, 0, 0, 0);
  }
  else
  {
    TMatrix itm = stg.viewItm;
    itm.setcol(3, itm.getcol(3) - stg.mainCamPos);
    vtm = inverse(itm);
  }
  render_state(state, dynmodel_renderer::get_buffer_type_from_render_pass(stg.renderPass), vtm,
    (stg.hints & UpdateStageInfoRender::RENDER_COLOR) ? dynamicSceneBlockId : dynamicDepthSceneBlockId);
  d3d::settm(TM_VIEW, stg.viewTm);
}

template <typename T>
static inline void gather_animchar_vehicle_cockpit_ecs_query(T cb);

ECS_TAG(render)
ECS_NO_ORDER
static void animchar_vehicle_cockpit_render_depth_prepass_es(const VehicleCockpitPrepass &event)
{
  DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  gather_animchar_vehicle_cockpit_ecs_query(
    [&](ECS_REQUIRE(ecs::Tag cockpitEntity) ECS_REQUIRE(ecs::EntityId cockpit__vehicleEid)
          const AnimV20::AnimcharRendComponent &animchar_render,
      const ecs::Point4List *additional_data, const uint8_t &animchar_visbits, bool needImmediateConstPS = false) {
      // Use interlocked load b/c it's might intersect with `animchar_render_main_job` which adds bits
      if (!(interlocked_relaxed_load(animchar_visbits) & VISFLG_MAIN_AND_SHADOW_VISIBLE))
        return;

      const DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
      G_ASSERT_RETURN(scene != nullptr, );

      // Render
      const bool noAdditionalData = additional_data == nullptr || additional_data->empty();
      const Point4 *additionalData = noAdditionalData ? ZERO_PTR<Point4>() : additional_data->data();
      const uint32_t additionalDataSize = noAdditionalData ? 1 : additional_data->size();
      state.process_animchar(0, ShaderMesh::STG_imm_decal, scene, additionalData, additionalDataSize, false, nullptr, nullptr, 0, 0,
        needImmediateConstPS);
    });
  if (state.empty())
    return;
  TMatrix vtm;
  vtm = event.viewTm;
  vtm.setcol(3, 0, 0, 0);
  STATE_GUARD_0(ShaderGlobal::set_int(is_hero_cockpitVarId, VALUE), 1);
  render_state(state, BufferType::OTHER, vtm, dynamicSceneBlockId);
  d3d::settm(TM_VIEW, event.viewTm);
}

template <typename T>
static inline void render_animchar_hmap_deform_ecs_query(T cb);
ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // This ES is event one but it's still assumed to be called after
static void animchar_render_hmap_deform_es(const RenderHmapDeform &event)
{
  TIME_D3D_PROFILE(animchar_render_hmap_deform);

  DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();

  // TODO compare with gather_animchar_ecs_query and separate setting threshold to independent system
  render_animchar_hmap_deform_ecs_query(
    [&](ECS_REQUIRE_NOT(const Point3 semi_transparent__placingColor) ECS_REQUIRE_NOT(ecs::Tag invisibleUpdatableAnimchar)
          AnimV20::AnimcharRendComponent &animchar_render,
      uint8_t animchar_visbits, const ecs::Point4List *additional_data, const int *forced_lod_for_hmap_deform,
      const ecs::UInt8List *vehicle_trails__nodesFilter = nullptr) {
      if (!(animchar_visbits & VISFLG_RENDER_CUSTOM)) // It was set by `update_animchar_hmap_deform_ecs_query`
        return;
      const DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
      G_ASSERT_RETURN(scene != nullptr, );
      int deformLod = scene->getLodsCount() - 1;
      if (forced_lod_for_hmap_deform != nullptr)
      {
        G_ASSERT(*forced_lod_for_hmap_deform >= 0 && *forced_lod_for_hmap_deform < scene->getLodsCount());
        deformLod = clamp(*forced_lod_for_hmap_deform, 0, deformLod);
      }
      deformLod = max(deformLod, scene->getLodsResource()->getQlBestLod());

      // Render
      const bool noAdditionalData = additional_data == nullptr || additional_data->empty();
      const Point4 *additionalData = noAdditionalData ? ZERO_PTR<Point4>() : additional_data->data();
      const uint32_t additionalDataSize = noAdditionalData ? 1 : additional_data->size();
      const DynamicRenderableSceneResource *lodResource = scene->getLodsResource()->lods[deformLod].scene;

      const uint8_t *filter = vehicle_trails__nodesFilter ? vehicle_trails__nodesFilter->data() : nullptr;
      int filter_size = vehicle_trails__nodesFilter ? vehicle_trails__nodesFilter->size() : 0;
      state.process_animchar(0, ShaderMesh::STG_opaque, scene, lodResource, additionalData, additionalDataSize, false, nullptr, filter,
        filter_size, 1);
    });

  TMatrix itm = event.viewItm;
  itm.setcol(3, itm.getcol(3) - event.mainCamPos);
  render_state(state, BufferType::OTHER, inverse(itm), dynamicDepthSceneBlockId);
  d3d::settm(TM_VIEW, event.viewTm);
}

template <typename Callable>
__forceinline void animchar_render_trans_first_ecs_query(Callable c);
template <typename Callable>
__forceinline void animchar_render_trans_second_ecs_query(Callable c);
template <typename Callable>
__forceinline void animchar_render_distortion_ecs_query(Callable c);
template <typename Callable>
__forceinline ecs::QueryCbResult animchar_render_find_any_visible_distortion_ecs_query(Callable c);

static inline bool isRenderableInStage(uint32_t start_stage, uint32_t end_stage, const DynamicRenderableSceneInstance *scene)
{
  const int lodsCount = scene->getLodsCount();
  for (int i = 0; i < lodsCount; i++)
  {
    DynamicRenderableSceneResource *lodResource = scene->getLodsResource()->lods[i].scene;
    auto checkMeshInStage = [&](const ShaderMesh &mesh) {
      for (uint8_t stage = start_stage; stage <= end_stage; ++stage)
        for (auto &elem : mesh.getElems(stage))
          if (elem.e)
            return true;
      return false;
    };
    // rigids
    for (auto &o : lodResource->getRigids())
      if (checkMeshInStage(*o.mesh->getMesh()))
        return true;
    // skins
    for (int i = 0; i < lodResource->getSkinsCount(); i++)
      if (checkMeshInStage(lodResource->getSkins()[i]->getMesh()->getShaderMesh()))
        return true;
  }
  return false;
}

ECS_ON_EVENT(on_appear)
ECS_REQUIRE_NOT(ecs::Tag late_transparent_render)
ECS_REQUIRE_NOT(ecs::Tag invisibleUpdatableAnimchar)
ECS_REQUIRE_NOT(const Point3 semi_transparent__placingColor)
ECS_REQUIRE_NOT(ecs::Tag requires_trans_render)
static void animchar_render_trans_init_es_event_handler(const ecs::Event &,
  ecs::EntityId eid,
  const AnimV20::AnimcharRendComponent &animchar_render,
  const AnimV20::AnimcharBaseComponent &animchar,
  ecs::Tag *animchar_contains_trans = nullptr)
{
  if (animchar_contains_trans)
    debug("Animchar %s should have transparent elems.", animchar.getCreateInfo()->resName.c_str());
  const DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
  bool requireTrans = false;
  FINALLY([&] {
    if (requireTrans)
      add_sub_template_async(eid, "requires_trans_render");
  });
  if (!scene)
  {
    logerr("Animchar %s doesn't have scene while we are trying to check trans elems.", animchar.getCreateInfo()->resName.c_str());
    requireTrans = animchar_contains_trans != nullptr;
    return;
  }
  bool requiresTransRender = isRenderableInStage(ShaderMesh::STG_trans, ShaderMesh::STG_trans, scene);
  if (!requiresTransRender && animchar_contains_trans)
    logerr("Animchar %s with %d LODs requires trans elems, but they weren't found in loaded %d LODs.",
      animchar.getCreateInfo()->resName.c_str(), scene->getLodsCount(), scene->getLodsResource()->getQlBestLod());
  requireTrans = requiresTransRender || animchar_contains_trans != nullptr;
}

ECS_TAG(render)
static __forceinline void animchar_render_trans_es(const UpdateStageInfoRenderTrans &stg)
{
  DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();

  animchar_render_trans_first_ecs_query(
    [&state, &stg](ECS_REQUIRE(ecs::Tag requires_trans_render) ECS_REQUIRE_NOT(ecs::Tag late_transparent_render)
                     AnimV20::AnimcharRendComponent &animchar_render,
      uint8_t animchar_visbits, const ecs::UInt8List *animchar_render__nodeVisibleStgFilters) {
      if (animchar_visbits & VISFLG_MAIN_VISIBLE) //< reuse visibility check from render
      {
        const DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
        const uint8_t *filter = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->begin() : nullptr;
        const uint32_t pathFilterSize = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->size() : 0;
        G_ASSERT(!animchar_render__nodeVisibleStgFilters || pathFilterSize == scene->getNodeCount());
        state.process_animchar(ShaderMesh::STG_trans, ShaderMesh::STG_trans, scene, nullptr, 0, false, nullptr, filter, pathFilterSize,
          UpdateStageInfoRender::RENDER_MAIN, false, RenderPriority::HIGH, nullptr, stg.texCtx);
      }
    });

  TMatrix vtm = stg.viewTm;
  vtm.setcol(3, 0, 0, 0);
  render_state(state, BufferType::TRANSPARENT_MAIN, vtm, dynamicSceneTransBlockId);
  d3d::settm(TM_VIEW, stg.viewTm);
}

ECS_TAG(render)
ECS_NO_ORDER
static __forceinline void animchar_render_distortion_es(const UpdateStageInfoRenderDistortion &stg)
{
  DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();

  animchar_render_distortion_ecs_query(
    [&state, &stg](ECS_REQUIRE(ecs::Tag requires_distortion_render) AnimV20::AnimcharRendComponent &animchar_render,
      uint8_t animchar_visbits, const ecs::UInt8List *animchar_render__nodeVisibleStgFilters) {
      if (animchar_visbits & VISFLG_MAIN_VISIBLE) //< reuse visibility check from render
      {
        const DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
        const uint8_t *filter = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->begin() : nullptr;
        const uint32_t pathFilterSize = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->size() : 0;
        G_ASSERT(!animchar_render__nodeVisibleStgFilters || pathFilterSize == scene->getNodeCount());
        state.process_animchar(ShaderMesh::STG_distortion, ShaderMesh::STG_distortion, scene, nullptr, 0, false, nullptr, filter,
          pathFilterSize, UpdateStageInfoRender::RENDER_MAIN, false, RenderPriority::HIGH, nullptr, stg.texCtx);
      }
    });

  TMatrix vtm = stg.viewTm;
  vtm.setcol(3, 0, 0, 0);
  render_state(state, BufferType::MAIN, vtm, dynamicSceneBlockId);
  d3d::settm(TM_VIEW, stg.viewTm);
}

bool animchar_has_any_visible_distortion()
{
  return animchar_render_find_any_visible_distortion_ecs_query(
           [](ECS_REQUIRE(ecs::Tag requires_distortion_render) uint8_t animchar_visbits) {
             if (animchar_visbits & VISFLG_MAIN_VISIBLE)
               return ecs::QueryCbResult::Stop;
             return ecs::QueryCbResult::Continue;
           }) == ecs::QueryCbResult::Stop;
}

void render_mainhero_trans(const TMatrix &view_tm)
{
  DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();

  animchar_render_trans_second_ecs_query(
    [&state](ECS_REQUIRE(ecs::Tag late_transparent_render) AnimV20::AnimcharRendComponent &animchar_render, uint8_t animchar_visbits,
      const ecs::UInt8List *animchar_render__nodeVisibleStgFilters) {
      if (animchar_visbits & VISFLG_MAIN_VISIBLE) //< reuse visibility check from render
      {
        const DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
        const uint8_t *filter = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->begin() : nullptr;
        const uint32_t pathFilterSize = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->size() : 0;
        G_ASSERT(!animchar_render__nodeVisibleStgFilters || pathFilterSize == scene->getNodeCount());
        state.process_animchar(ShaderMesh::STG_trans, ShaderMesh::STG_trans, scene, nullptr, 0, false, nullptr, filter, pathFilterSize,
          UpdateStageInfoRender::RENDER_MAIN, false, RenderPriority::DEFAULT, nullptr,
          ((WorldRenderer *)get_world_renderer())->getTexCtx());
      }
    });

  TMatrix vtm = view_tm;
  vtm.setcol(3, 0, 0, 0);
  render_state(state, BufferType::TRANSPARENT_MAIN, vtm, dynamicSceneTransBlockId);
  d3d::settm(TM_VIEW, view_tm);
}

ECS_TAG(render)
static void close_animchar_bindpose_buffer_es(const EventOnGameShutdown &)
{
  DynamicRenderableSceneLodsResource::closeBindposeBuffer();
}

static bool animchar_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("animchar", "verify_lods", 1, 2)
  {
    float fov = argc > 1 ? console::to_real(argv[1]) : 90.f;
    float tg = tan(fov / 180.f * PI * 0.5f);

    eastl::string cvs_dump = "name;count;rendered;bBox rad;"
                             "L0 dips;L1 dips;L2 dips;L3 dips;"
                             "L0 tris;L1 tris;L2 tris;L3 tris;"
                             "L0 dist;L1 dist;L2 dist;L3 dist;"
                             "L0 screen%;L1 screen%;L2 screen%;L3 screen%;"
                             "\n";
    eastl::vector_map<ecs::string, IPoint2> nameCountMap;

    count_animchar_renderer_ecs_query([&](const ecs::string &animchar__res, uint8_t animchar_visbits) {
      IPoint2 &count_rendered = nameCountMap[animchar__res];
      count_rendered.x += 1;
      count_rendered.y +=
        (animchar_visbits & (VISFLG_MAIN_CAMERA_RENDERED | VISFLG_SEMI_TRANS_RENDERED | VISFLG_COCKPIT_VISIBLE)) != 0;
    });

    gather_animchar_renderer_ecs_query([&](const AnimV20::AnimcharRendComponent &animchar_render, const ecs::string &animchar__res) {
      auto lodsResources = animchar_render.getSceneInstance()->getLodsResource();
      if (!lodsResources)
        return;
      IPoint2 &count_rendered_ref = nameCountMap[animchar__res];
      IPoint2 count_rendered = count_rendered_ref;
      if (count_rendered.x == 0) // already processed animchars with that name
        return;
      count_rendered_ref.x = 0;
      Point3 bboxWidth = lodsResources->bbox.width();
      float maxBoxEdge = max(max(bboxWidth.x, bboxWidth.y), bboxWidth.z) * 0.5f; // half of edge like a radius

      cvs_dump += eastl::string(eastl::string::CtorSprintf{}, "%s;%d;%d;%f;", animchar__res.c_str(), count_rendered.x,
        count_rendered.y, maxBoxEdge);


      struct LodInfo
      {
        int totalTris, totalDrawcalls;
        int skinTris, skinDrawcalls, rigidsTris, rigidsDrawcalls;
        float lodDistance, screenPercent;
        LodInfo() = default;
      };
      constexpr int MAX_LODS = 4;
      eastl::fixed_vector<LodInfo, MAX_LODS> lodsInfo;


      for (const auto &lod : lodsResources->lods)
      {
        LodInfo info = LodInfo();
        for (const auto &skin : lod.scene->getSkins())
        {
          skin->getMesh()->bonesCount();
          const auto &mesh = skin->getMesh()->getShaderMesh();
          info.skinTris += mesh.calcTotalFaces();
          info.skinDrawcalls += mesh.getAllElems().size();
        }
        for (const auto &rigid : lod.scene->getRigidsConst())
        {
          const auto &mesh = *rigid.mesh->getMesh();
          info.rigidsTris += mesh.calcTotalFaces();
          info.rigidsDrawcalls += mesh.getAllElems().size();
        }

        float lodDistance = lod.range;
        float sizeScale = 2.f / (tg * lodDistance);
        // float spherePartOfScreen = bSphereRad * sizeScale;
        float boxPartOfScreen = maxBoxEdge * sizeScale;
        info.totalTris = info.rigidsTris + info.skinTris;
        info.totalDrawcalls = info.rigidsDrawcalls + info.skinDrawcalls;
        info.screenPercent = boxPartOfScreen * 100;
        info.lodDistance = lodDistance;
        lodsInfo.emplace_back(info);
      }
#define DUMP(format, var_name)                  \
  for (uint32_t lod = 0; lod < MAX_LODS; lod++) \
    cvs_dump += lod < lodsInfo.size() ? eastl::string(eastl::string::CtorSprintf{}, format, lodsInfo[lod].var_name) : ";";
      DUMP("%d;", totalDrawcalls)
      DUMP("%d;", totalTris)
      DUMP("%.0f;", lodDistance)
      DUMP("%.1f;", screenPercent)
      cvs_dump += "\n";
    });
    FullFileSaveCB cb("animchar_profiling_statistic.csv", DF_WRITE | DF_CREATE);
    cb.write(cvs_dump.c_str(), cvs_dump.size() - 1);
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(animchar_console_handler);
