// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include <memory/dag_framemem.h>
#include <gameRes/dag_collisionResource.h>
#include <rendInst/visibility.h>
#include <util/dag_convar.h>
#include "private_worldRenderer.h"
#include <ioSys/dag_dataBlock.h>
#include <render/voxelization_target.h>
#include <daGI2/daGI2.h>
#include <daSDF/worldSDF.h>
#include <daGI2/lruCollisionVoxelization.h>
#include <shaders/dag_shaderBlock.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_statDrv.h>
#include <render/deferredRenderer.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_cube_matrix.h>
#include <util/dag_hash.h>
#include <render/lruCollision.h>
#include <daGI2/treesAboveDepth.h>
#include "global_vars.h"
#include <scene/dag_tiledScene.h>
#include <math/dag_math3d.h>
#include <util/dag_console.h>
#include <util/dag_stlqsort.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <debug/dag_debug3d.h>
#include <math/dag_vecMathCompatibility.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstGenRender.h>

#include "frameGraphNodes/frameGraphNodes.h"
#include "render/world/gbufferConsts.h"
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <GIWindows/GIWindows.h>


CONSOLE_FLOAT_VAL_MINMAX("render", world_sdf_from_collision_rasterize_below, 3, -1, 16);
CONSOLE_FLOAT_VAL_MINMAX("render", world_sdf_rasterize_supersample, 1, 1, 4);
CONSOLE_BOOL_VAL("render", rasterize_sdf_prims, true);
CONSOLE_BOOL_VAL("render", sdf_rasterize_collision, true);
CONSOLE_BOOL_VAL("render", debug_rasterize_lru_collision, false);

enum GiQualitySetting
{
  GI_ONLY_AO = 0,
  GI_COLORED = 1,
  GI_SCREEN_PROBES = 2
};

CONSOLE_INT_VAL("render", gi_quality, GI_COLORED, GI_ONLY_AO, GI_SCREEN_PROBES);
CONSOLE_FLOAT_VAL_MINMAX("render", min_sdf_voxel_with_lights_size, 0.35, 0, 4);
CONSOLE_FLOAT_VAL_MINMAX("render", max_sdf_voxel_with_lights_size, 2.33, 0, 4);
CONSOLE_FLOAT_VAL_MINMAX("render", gi_algorithm_quality, 1, 0, 1);
CONSOLE_BOOL_VAL("render", gi_update_pos, true);
CONSOLE_BOOL_VAL("render", gi_update_scene, true);
CONSOLE_BOOL_VAL("render", gi_enabled, true);
CONSOLE_BOOL_VAL("render", gi_show_resticted_update_box, false);

static ShaderVariableInfo ssr_alternate_reflectionsVarId("ssr_alternate_reflections", true);

extern int rendinstVoxelizeSceneBlockId;
extern float cascade0Dist;

void WorldRenderer::invalidateGI(bool force)
{
  G_UNUSED(force);
  // todo: support force
  if (daGI2)
    daGI2->afterReset();
  if (treesAbove)
    treesAbove->invalidate();
}

void WorldRenderer::drawGIDebug(const Frustum &camera_frustum)
{
  G_UNUSED(camera_frustum);
  if (daGI2)
    daGI2->debugRenderTrans();
}

// static const int DEFERRED_FRAMES_TO_INVALIDATE_GI = 9;
static const int MAX_GI_FRAMES_TO_INVALIDATE_POS = 32;

void WorldRenderer::setGIQualityFromSettings()
{
  if (isThinGBuffer() || is_only_low_gi_supported())
    gi_quality.setMinMax(GI_ONLY_AO, GI_ONLY_AO);
  else
    gi_quality.setMinMax(GI_ONLY_AO, GI_SCREEN_PROBES);

  GiQuality quality = getGiQuality();
  switch (quality.algorithm)
  {
    case GiAlgorithm::OFF:
      closeGI();
      gi_enabled.set(false);
      return;
    case GiAlgorithm::LOW: gi_quality.set(GI_ONLY_AO); break;
    case GiAlgorithm::MEDIUM: gi_quality.set(GI_COLORED); break;
    case GiAlgorithm::HIGH:
    {
      auto q = GI_SCREEN_PROBES;
#if !_TARGET_SCARLETT // no actual support for inline rt on Scarlett yet.
      const Driver3dDesc &caps = d3d::get_driver_desc();
      if (caps.caps.hasRayQuery)
      {
        const DataBlock *graphics = dgs_get_settings()->getBlockByNameEx("graphics");
        const DataBlock *graphicsGI = graphics->getBlockByNameEx("gi");
        if (graphicsGI->getBool("inlineRaytracing", false)) {}
      }
      else
        logwarn("Inline ray-tracing is not supported");
#endif
      gi_quality.set(q);
      break;
    }
    default:
      logwarn("Invalid GI quality");
      gi_quality.set(GI_ONLY_AO);
      break;
  }
  gi_algorithm_quality.set(quality.algorithmQuality);

  initGI();

  gi_enabled.set(true);
  giUpdatePosFrameCounter = 0;
}

bool WorldRenderer::giNeedsReprojection()
{
  return hasFeature(FeatureRenderFlags::DEFERRED_LIGHT) && daGI2 && gi_quality == GI_SCREEN_PROBES &&
         daGI2->getNextSettings().screenProbes.tileSize > 8;
}

void WorldRenderer::giBeforeRender()
{
  if (daGI2 && !gi_enabled.get())
  {
    closeGI();
    makePrepareDepthForPostFxNode();
  }

  if (!daGI2 && gi_enabled.get())
    initGI();

  ShaderGlobal::set_int(gi_qualityVarId, daGI2 ? gi_quality : -1);

  if (!daGI2)
    return;
  TIME_D3D_PROFILE(gi2_before_render)

  const bool hadReprojection = giNeedsReprojection();

  int w, h, maxW, maxH;
  getRenderingResolution(w, h);
  getMaxPossibleRenderingResolution(maxW, maxH);

  auto s = daGI2->getNextSettings();
  constexpr float ALBEDO_MEDIA_MUL = 0.0625f;
  constexpr float LIT_SCENE_MUL = 0.0625f;
  const DataBlock *graphicsGI = dgs_get_settings()->getBlockByNameEx("graphics")->getBlockByNameEx("gi");
  s.voxelScene.sdfResScale = graphicsGI->getReal("voxelSceneResScale", 0.5);
  const bool temporality = gi_quality == GI_SCREEN_PROBES || gi_algorithm_quality > 0.0001;

  const float temporalSpeed = temporality ? lerp<float>(0.05, 1, gi_algorithm_quality) : 0;
  s.mediaScene.temporalSpeed = temporalSpeed * ALBEDO_MEDIA_MUL;
  s.voxelScene.temporalSpeed = temporalSpeed * LIT_SCENE_MUL;
  s.albedoScene.temporalSpeed = temporalSpeed * ALBEDO_MEDIA_MUL;
  s.sdf.clips = clamp(graphicsGI->getInt("sdfClips", bareMinimumPreset ? 5 : 6), 4, 6);
  s.sdf.voxel0Size = clamp(graphicsGI->getReal("sdfVoxel0", 0.15), 0.065f, 0.45f);
  if (gi_quality == GI_ONLY_AO)
  {
    s.skyVisibility.clipW = bareMinimumPreset ? 64 : 56;
    s.skyVisibility.clips = bareMinimumPreset ? 5 : 6;
    s.albedoScene.clips = 0;
    s.radianceGrid.w = 0;
    s.screenProbes.tileSize = 0;
    s.volumetricGI.tileSize = 0;
    s.voxelScene.sdfResScale = 0; // no need for voxel scene
    s.skyVisibility.framesToUpdateTemporally = temporality ? lerp(96, 32, gi_algorithm_quality) : 0;
    s.mediaScene.temporalSpeed = gi_algorithm_quality * ALBEDO_MEDIA_MUL;
  }
  else if (gi_quality == GI_COLORED)
  {
    s.skyVisibility.clipW = 0;
    s.albedoScene.clips = graphicsGI->getInt("albedoClips", 3);
    s.radianceGrid.w = graphicsGI->getInt("radianceGridClipW", 28);
    s.radianceGrid.clips = graphicsGI->getInt("radianceGridClips", 4);
    s.radianceGrid.irradianceProbeDetail = 2.f;
    s.radianceGrid.additionalIrradianceClips = 2;
    s.screenProbes.tileSize = 0;
    s.volumetricGI.tileSize = 64;
    s.radianceGrid.framesToUpdateOneClip = temporality ? lerp(96, 32, gi_algorithm_quality) : 0;
  }
  else if (gi_quality == GI_SCREEN_PROBES)
  {
    s.skyVisibility.clipW = 0;
    s.radianceGrid.w = graphicsGI->getInt("radianceGridClipW", 28);
    s.radianceGrid.clips = graphicsGI->getInt("radianceGridClips", 4);
    s.radianceGrid.irradianceProbeDetail = 1.f;
    s.radianceGrid.additionalIrradianceClips = 0;
    s.albedoScene.clips = graphicsGI->getInt("albedoClips", 3);

    auto remap = [](float min_value, float max_value, float interval_min, float interval_max, float t) -> float {
      G_ASSERT(interval_min < interval_max);
      float lerpS = (t - interval_min) / (interval_max - interval_min);
      lerpS = saturate(lerpS);
      return lerp(min_value, max_value, lerpS);
    };

    auto remapUnclamped = [](float min_value, float max_value, float interval_min, float interval_max, float t) -> float {
      G_ASSERT(interval_min < interval_max);
      float lerpS = (t - interval_min) / (interval_max - interval_min);
      return lerp(min_value, max_value, lerpS);
    };

    if (gi_algorithm_quality >= 0.75f)
      giDynamicQuality = remap(0.55f, 1.0f, 0.75f, 1.0f, gi_algorithm_quality);
    else
      giDynamicQuality = remap(0.1f, 0.55f, 0.25f, 0.5f, gi_algorithm_quality);
    s.screenProbes.temporality = giDynamicQuality;

    // minimum s.screenProbes.tileSize is 20 at 1080p, 24 at 1440p, 32 at 4k
    int w, h;
    getPostFxInternalResolution(w, h);
    float minTileSize = remapUnclamped(16, 32, 1080, 1080 * 2, h);
    float tileSizeF = remap(minTileSize, 8, 0.25, 0.95f, gi_algorithm_quality);
    s.screenProbes.tileSize = static_cast<int>(round(tileSizeF)) & ~1;

    float radianceOctResF = remap(4, 8, 0.f, 0.5f, gi_algorithm_quality);
    s.screenProbes.radianceOctRes = static_cast<int>(round(radianceOctResF));

    s.screenProbes.angleFiltering = gi_algorithm_quality > 0.875f;
    s.volumetricGI.tileSize = (int(lerp(64.f, 48.f, gi_algorithm_quality)) + 1) & ~1;
    s.volumetricGI.slices = int(lerp(16.f, 24.f, gi_algorithm_quality));
  }
  daGI2->setSettings(s);

  ShaderGlobal::set_int(ssr_alternate_reflectionsVarId, ssrWantsAlternateReflections && gi_quality > 0 ? 1 : 0);
  daGI2->beforeRender(w, h, maxW, maxH, currentFrameCamera.viewItm, currentFrameCamera.jitterProjTm,
    currentFrameCamera.noJitterPersp.zn, currentFrameCamera.noJitterPersp.zf);

  if (treesAbove)
  {
    auto &worldSdf = daGI2->getWorldSDF();
    int sdfW, sdfH;
    worldSdf.getResolution(sdfW, sdfH);

    uint16_t w = s.mediaScene.w, d = s.mediaScene.d, clips = s.mediaScene.clips;
    d = d ? d : w * sdfH / sdfW;
    float voxel0 = s.mediaScene.voxel0Size;

    const float sdfV = worldSdf.getVoxelSize(worldSdf.getClipsCount() - 1);
    if (voxel0 <= 0.f)
    {
      voxel0 = max((sdfW * sdfV) / w, (sdfH * sdfV) / d) / (1 << (clips - 1));
    }

    const float halfDist = sdfW * sdfV * 0.5f * 1.2f; // 120% bigger
    treesAbove->init(halfDist, voxel0);
  }
  makePrepareDepthForPostFxNode();
  if (giNeedsReprojection() != hadReprojection)
    createDeferredLightNode();
}

void WorldRenderer::renderGiCollision(const TMatrix &view_itm, const Frustum &)
{
  if (!debug_rasterize_lru_collision || !voxelizeCollision || !voxelizeCollision->renderCollisionElem)
    return;
  DA_PROFILE_GPU;
  // fixme: add frustum gather
  rendinst::riex_collidable_t handles;
  BBox3 sceneBox(view_itm.getcol(3) - Point3(30, 30, 30), view_itm.getcol(3) + Point3(30, 30, 30));
  rendinst::gatherRIGenExtraCollidableMin(handles, v_ldu_bbox3(sceneBox), 0);
  if (staticSceneCollisionResource && (staticSceneCollisionResource->boundingBox & sceneBox))
    handles.push_back(rendinst::RIEX_HANDLE_NULL);
  ShaderGlobal::set_int(get_shader_variable_id("rasterize_collision_type", true), 1);
  if (!handles.empty())
    voxelizeCollision->rasterize(*lruCollision, dag::Span<uint64_t>(handles.data(), handles.size()), nullptr, nullptr,
      voxelizeCollision->renderCollisionElem, 1, false);
}

void WorldRenderer::updateGIPos(const Point3 &pos, const TMatrix &view_itm, float hmin, float hmax)
{
  if (daGI2 && !gi_enabled.get())
    closeGI();

  if (!daGI2 && gi_enabled.get())
    initGI();

  if (!daGI2)
    return;

  if (giWindows)
    giWindows->updatePos(pos);

  // if InitGI called only on level load it can cause GI artifacts, for some reason.
  // Calling it a bit later, or invalidating GI fixes the issue once and for all.
  // if (++giInvalidateDeferred == DEFERRED_FRAMES_TO_INVALIDATE_GI)
  //  daGI2->invalidateDeferred();
  // we check if static shadows are valid in the pos where we moved to
  // static shadows are valid if: either it is completely valid, or it is within the region (although there might be invalid regions)
  // region size is decreased each frame
  const float giUpdateDist = 100.f; // fixme:

  if (!ignoreStaticShadowsForGI && ++giUpdatePosFrameCounter <= MAX_GI_FRAMES_TO_INVALIDATE_POS) // if frames passed < threshold
  {
    if (!shadowsManager.insideStaticShadows(pos, giUpdateDist)) // we are not even inside static shadows
      return;
    if (!shadowsManager.validStaticShadows(pos, giUpdateDist)) // static shadows are invalid
      return;
    // if static shadows are fully updated, render immediately, otherwise try to wait for one frame
    if (giUpdatePosFrameCounter == 1 && !shadowsManager.fullyUpdatedStaticShadows(pos, giUpdateDist))
      return;
  }
  giUpdatePosFrameCounter = 0;

  {
    float minZ, maxZ;
    getMinMaxZ(minZ, maxZ);
    const float treeHeightMax = 24;
    maxZ += treeHeightMax;
    treesAbove->prepare(pos, minZ, maxZ, [&](const BBox3 &box, bool depth_min) {
      DA_PROFILE_GPU;
      TMatrix4 proj = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].z, box[0].z, box[depth_min].y, box[!depth_min].y);
      d3d::settm(TM_PROJ, &proj);

      TMatrix view;
      view.setcol(0, 1, 0, 0);
      view.setcol(1, 0, 0, 1);
      view.setcol(2, 0, 1, 0);
      view.setcol(3, 0, 0, 0);
      d3d::settm(TM_VIEW, view);

      TMatrix4 globtm_ = TMatrix4(view) * proj;
      mat44f globtm;
      v_mat44_make_from_44cu(globtm, globtm_.m[0]);
      if (!depth_min)
        ShaderGlobal::set_int(gbuffer_for_treesaboveVarId, 1);

      rendinst::prepareRIGenVisibility(Frustum(globtm), Point3::xVy(box.center(), minZ), rendinst_trees_visibility, false, NULL);
      uint32_t immediateConsts[] = {0u, 0u, ((uint32_t)float_to_half(-1.f) << 16) | (uint32_t)float_to_half(0.f)};
      d3d::set_immediate_const(STAGE_PS, immediateConsts, countof(immediateConsts));
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
      rendinst::render::before_draw(depth_min ? rendinst::RenderPass::Depth : rendinst::RenderPass::Normal, rendinst_trees_visibility,
        Frustum{globtm}, nullptr);
      rendinst::render::renderRIGen(depth_min ? rendinst::RenderPass::Depth : rendinst::RenderPass::Normal, rendinst_trees_visibility,
        view_itm, rendinst::LayerFlag::NotExtra, rendinst::OptimizeDepthPass::No);
      d3d::set_immediate_const(STAGE_PS, nullptr, 0);
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);

      if (!depth_min)
        ShaderGlobal::set_int(gbuffer_for_treesaboveVarId, 0);
    });
  }
  if (!gi_update_pos.get())
    return;
  int oldBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  auto baseVoxelizeRI = [&](const BBox3 &box_, const Point3 &voxelSize) {
    BBox3 box = box_;
    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
    const IPoint3 res(ceil(div(box.width(), voxelSize)));
    getMinMaxZ(box[0].y, box[1].y);
    int maxRes = max(max(res.x, res.y), res.z);
    G_ASSERT(maxRes > 0);
    SCOPE_RENDER_TARGET;
    set_voxelization_target_and_override(maxRes, maxRes);
    ShaderGlobal::set_color4(voxelize_box0VarId, box[0].x, box[0].y, box[0].z, 0);
    ShaderGlobal::set_color4(voxelize_box1VarId, 1. / box.width().x, 1 / box.width().y, 1 / box.width().z, 0);
    bool ret = false;
    // rendinst::prepareRIGenExtraVisibility(globtm, box.center(), *rendinst_voxelize_visibility, false, NULL);
    bbox3f boxCull = v_ldu_bbox3(box);
    // box visibility, skip small ri
    bbox3f actualBox;
    rendinst::prepareRIGenExtraVisibilityBox(boxCull, 3, voxelSize.x * 0.5, 71, *rendinst_voxelize_visibility, &actualBox);
    if (v_bbox3_test_box_intersect_safe(actualBox, boxCull))
    {
      d3d::setview(0, 0, maxRes, maxRes, 0, 1);
      box[0].y = floorf(v_extract_y(actualBox.bmin) / voxelSize.y) * voxelSize.y;
      box[1].y = ceilf(v_extract_y(actualBox.bmax) / voxelSize.y) * voxelSize.y;

      const Point3 voxelize_box_sz = max(Point3(1e-6f, 1e-6f, 1e-6f), box.width());
      const int resY = int(ceilf((box[1].y - box[0].y) / voxelSize.y));

      // use fixed aspect ratio of 1. for oversampling and HW clipping
      const Point3 voxelize_aspect_ratio = point3(max(IPoint3(res.x, resY, res.z), IPoint3(1, 1, 1))) / maxRes;

      const Point3 mult = 2. * div(voxelize_aspect_ratio, voxelize_box_sz);
      const Point3 add = -mul(box[0], mult) - voxelize_aspect_ratio;
      ShaderGlobal::set_color4(voxelize_world_to_rasterize_space_mulVarId, P3D(mult), 0);
      ShaderGlobal::set_color4(voxelize_world_to_rasterize_space_addVarId, P3D(add), 0);

      SCENE_LAYER_GUARD(rendinstVoxelizeSceneBlockId);
      rendinst::render::renderRIGen(rendinst::RenderPass::VoxelizeAlbedo, rendinst_voxelize_visibility, view_itm,
        rendinst::LayerFlag::Opaque, rendinst::OptimizeDepthPass::No, 3);
      ret = true;
    }
    reset_voxelization_override();

    ShaderElement::invalidate_cached_state_block();
    return ret;
  };

  Point3 giPos = pos;
  G_UNUSED(hmax);
  giPos.y = min(giPos.y, hmax + giHmapOffsetFromUp);
  giPos.y = max(giPos.y, hmin + giHmapOffsetFromLow);

  TIME_D3D_PROFILE(gi2_update_pos)
  bool lightsInsideFrustum = true;
  daGI2->updatePosition(
    [&](int sdf_clip, const BBox3 &box, float voxelSize, uintptr_t &) {
      G_UNUSED(sdf_clip);
      BBox3 sceneBox = box;
      // we increase by maximum written + 1 where we rounding due to rasterization
      sceneBox[0] -= Point3(voxelSize, voxelSize, voxelSize);
      sceneBox[1] += Point3(voxelSize, voxelSize, voxelSize);

      rendinst::riex_collidable_t handles;
      if (sdf_rasterize_collision)
      {
        TIME_PROFILE(sdf_from_collision)
        rendinst::gatherRIGenExtraCollidableMin(handles, v_ldu_bbox3(sceneBox), voxelSize * 0.5f);
      }
      if (staticSceneCollisionResource && (staticSceneCollisionResource->boundingBox & sceneBox))
      {
        handles.push_back(rendinst::RIEX_HANDLE_NULL);
      }
      const bool intersectLevel = false;
      if (handles.empty() && !intersectLevel)
        return UpdateGiQualityStatus::NOTHING;
      {
        TIME_PROFILE(sort_instances)
        stlsort::sort(handles.begin(), handles.end(),
          [](auto a, auto b) { return rendinst::handle_to_ri_type(a) < rendinst::handle_to_ri_type(b); });
      }
      if (lights.initialized())
      {
        float sdfVoxelizeQuality = float(gi_algorithm_quality) * (float(gi_quality) / 2.0);
        float maxSz = max(float(min_sdf_voxel_with_lights_size), float(max_sdf_voxel_with_lights_size));
        if (voxelSize <= lerp(float(min_sdf_voxel_with_lights_size), maxSz, sdfVoxelizeQuality) &&
            voxelSize <= world_sdf_from_collision_rasterize_below && daGI2->sdfClipHasVoxelsLitSceneClip(sdf_clip))
        {
          TIME_PROFILE(cull_lights)
          alignas(16) TMatrix4 globtm = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].y, box[0].y, box[0].z, box[1].z);
          lights.cullOutOfFrustumLights((mat44f_cref)globtm, SpotLightsManager::GI_LIGHT_MASK, OmniLightsManager::GI_LIGHT_MASK);
          lights.setOutOfFrustumLightsToShader();
        }
        else
          lights.setEmptyOutOfFrustumLights();
        lightsInsideFrustum = false;
      }
      Point3 voxelize_box0 = sceneBox[0];
      Point3 voxelize_box_sz = max(Point3(1e-6f, 1e-6f, 1e-6f), sceneBox.width());
      IPoint3 res = ipoint3(floor(sceneBox.width() / voxelSize + Point3(0.5, 0.5, 0.5)));
      // Point3 voxelize_box1 = Point3(1./sceneBox.width().x, 1./sceneBox.width().y, 1./sceneBox.width().z);
      const int maxRes = max(max(res.x, res.y), max(res.z, 1));
      Point3 voxelize_aspect_ratio = point3(max(res, IPoint3(1, 1, 1))) / maxRes; // use fixed aspect ratio of 1. for oversampling
                                                                                  // and HW clipping
      Point3 mult = 2. * div(voxelize_aspect_ratio, voxelize_box_sz);
      Point3 add = -mul(voxelize_box0, mult) - voxelize_aspect_ratio;
      ShaderGlobal::set_color4(voxelize_world_to_rasterize_space_mulVarId, P3D(mult), 0);
      ShaderGlobal::set_color4(voxelize_world_to_rasterize_space_addVarId, P3D(add), 0);
      if (handles.size())
      {
        if (voxelSize <= world_sdf_from_collision_rasterize_below)
        {
          SCOPE_RENDER_TARGET;
          set_voxelization_target_and_override(maxRes * world_sdf_rasterize_supersample, maxRes * world_sdf_rasterize_supersample);
          bool prims = rasterize_sdf_prims.get() && d3d::get_driver_desc().shaderModel < 6.1_sm;
          voxelizeCollision->rasterizeSDF(*lruCollision, dag::Span<uint64_t>(handles.data(), handles.size()), prims);
          reset_voxelization_override();
          ShaderElement::invalidate_cached_state_block();
        }
        else
        {
          voxelizeCollision->voxelizeSDFCompute(*lruCollision, dag::Span<uint64_t>(handles.data(), handles.size()));
        }
      }
      if (intersectLevel) // static scene
      {}
      return UpdateGiQualityStatus::RENDERED;
    },
    [&](const BBox3 &box, float voxelSize, uintptr_t &) {
      SCOPE_VIEW_PROJ_MATRIX;
      const bool intersectLevel = baseVoxelizeRI(box, Point3(voxelSize, voxelSize, voxelSize));
      return intersectLevel ? UpdateGiQualityStatus::RENDERED : UpdateGiQualityStatus::NOTHING;
    });

  if (!lightsInsideFrustum)
    lights.setInsideOfFrustumLightsToShader();

  ShaderGlobal::setBlock(oldBlock, ShaderGlobal::LAYER_FRAME);
}

#include <sceneRay/dag_sceneRay.h>
namespace dacoll
{
DeserializedStaticSceneRayTracer *get_frt();
}

void WorldRenderer::initGIWindows(eastl::unique_ptr<scene::TiledScene> &&windows)
{
  giWindows.reset();
  giWindows = eastl::make_unique<GIWindows>();
  giWindows->init(eastl::move(windows));
}

void WorldRenderer::initGI()
{
  closeGI();
  if (getGiQuality().algorithm == GiAlgorithm::OFF)
  {
    createGiNodes();
    createResolveGbufferNode();
    return;
  }

  if (DeserializedStaticSceneRayTracer *frt = dacoll::get_frt())
  {
    if (frt->getFacesCount())
    {
      staticSceneCollisionResource.reset(new CollisionResource);
      staticSceneCollisionResource->vFullBBox.bmin = v_ldu(&frt->getBox()[0].x);
      staticSceneCollisionResource->vFullBBox.bmax = v_ldu(&frt->getBox()[1].x);
      staticSceneCollisionResource->vBoundingSphere = v_make_vec4f(P3D(frt->getSphere().c), frt->getSphere().r2);
      staticSceneCollisionResource->boundingBox = frt->getBox();
      staticSceneCollisionResource->boundingSphereRad = frt->getSphere().r;

      uint16_t *indices = (uint16_t *)midmem->alloc(frt->getFacesCount() * 3 * sizeof(uint16_t));
      if (frt->getVertsCount() > 65536)
        logerr("not supporting frt dumps with %d verts correctly. Split into two collision resources", frt->getVertsCount());
      for (uint32_t ii = 0, ie = frt->getFacesCount(); ii != ie;)
      {
        auto &node = staticSceneCollisionResource->createNode();
        node.flags = CollisionNode::FLAG_VERTICES_ARE_REFS | CollisionNode::IDENT;
        node.type = COLLISION_NODE_TYPE_MESH;
        if (ii != 0)
          node.flags |= CollisionNode::FLAG_INDICES_ARE_REFS;
        node.modelBBox = frt->getBox();
        node.boundingSphere = frt->getSphere();

        uint32_t indicesCnt = frt->getFacesCount() * 3;
        node.resetVertices(dag::Span<Point3_vec4>(&frt->verts(0), frt->getVertsCount()));
        for (uint32_t j = 0, f = ii / 3; j < indicesCnt; j += 3, ++f)
        {
          indices[j + 0] = frt->faces(f).v[0];
          indices[j + 1] = frt->faces(f).v[1];
          indices[j + 2] = frt->faces(f).v[2];
        }
        node.resetIndices(dag::Span<uint16_t>(indices, indicesCnt));
        indices += indicesCnt;
        ii += indicesCnt / 3;
      }
    }
  }


  daGI2 = create_dagi();
  init_voxelization();
  voxelizeCollision = new LRUCollisionVoxelization;
  voxelizeCollision->init();
  // const DataBlock *graphics = dgs_get_settings()->getBlockByNameEx("graphics");
  // const DataBlock *graphicsGI = graphics->getBlockByNameEx("gi");

  lruCollision.reset();
  lruCollision.reset(new LRURendinstCollision);

  rendinst_voxelize_visibility = rendinst::createRIGenVisibility(midmem);
  rendinst::setRIGenVisibilityMinLod(rendinst_voxelize_visibility, 0, 2);

  rendinst_trees_visibility = rendinst::createRIGenVisibility(midmem);
  rendinst::setRIGenVisibilityMinLod(rendinst_trees_visibility, 0, 4);

  treesAbove.reset(new TreesAboveDepth());

  createGiNodes();
  createResolveGbufferNode();

  ShaderGlobal::set_sampler(specular_tex_samplerstateVarId, d3d::request_sampler({}));
}

void WorldRenderer::resetGI()
{
  if (lruCollision)
    lruCollision->reset();
  if (daGI2)
    daGI2->afterReset();
  if (treesAbove)
    treesAbove->invalidate();
  giUpdatePosFrameCounter = 0;
}

void WorldRenderer::closeGI()
{
  staticSceneCollisionResource.reset();
  treesAbove.reset();
  lruCollision.reset();
  rendinst::destroyRIGenVisibility(rendinst_voxelize_visibility);
  rendinst_voxelize_visibility = nullptr;

  if (daGI2)
    close_voxelization();
  del_it(daGI2);
  del_it(voxelizeCollision);
  createGiNodes();
  createResolveGbufferNode();

  rendinst::destroyRIGenVisibility(rendinst_trees_visibility);
  rendinst_trees_visibility = nullptr;
}

const scene::TiledScene *WorldRenderer::getWallsScene() const { return nullptr; }

const scene::TiledScene *WorldRenderer::getWindowsScene() const { return giWindows ? giWindows->windows.get() : nullptr; }

void WorldRenderer::invalidateGI(const BBox3 &model_bbox, const TMatrix &tm, const BBox3 &approx)
{
  G_UNUSED(approx);
  // fixme: add
  // if (daGI2)
  //   daGI2->invalidate(model_bbox, tm, approx);
  if (treesAbove)
    treesAbove->invalidateTrees2d(model_bbox, tm);
}

void WorldRenderer::createGiNodes()
{
  if (isForwardRender())
    return;

  if (!daGI2)
  {
    giRenderFGNode = {};

    giCalcFGNode = {};
    giFeedbackFGNode = {};
    giScreenDebugFGNode = {};
    return;
  }

  giCalcFGNode = makeGiCalcNode();
  giFeedbackFGNode = makeGiFeedbackNode();
#if DAGOR_DBGLEVEL > 0
  giScreenDebugFGNode = makeGiScreenDebugNode();
  giScreenDebugDepthFGNode = makeGiScreenDebugDepthNode();
#endif
}

void WorldRenderer::createDeferredLightNode()
{
  if (isForwardRender())
    return;

  deferredLightNode =
    hasFeature(FeatureRenderFlags::DEFERRED_LIGHT) ? makeDeferredLightNode(giNeedsReprojection()) : dabfg::NodeHandle{};
}

void WorldRenderer::createResolveGbufferNode()
{
  if (isForwardRender())
    return;

  const char *resolve_shader = nullptr;
  const char *resolve_cshader = nullptr, *classify_cshader = nullptr;

  if (isThinGBuffer())
    resolve_shader = THIN_GBUFFER_RESOLVE_SHADER;
  else
  {
    resolve_shader = bareMinimumPreset ? "deferred_shadow_bare_minimum" : "deferred_shadow_to_buffer";
    resolve_cshader = bareMinimumPreset ? "" : "deferred_shadow_compute";
    classify_cshader = "deferred_shadow_classify_tiles";
  }

  const ShadingResolver::PermutationsDesc resolve_permutations = {
    {int(dynamic_lights_countVarId)} /* shaderVarIds */, 1,                /* shaderVarCount */
    {{0 /* LIGHTS_OFF */, ShadingResolver::USE_CURRENT_SHADER_VAR_VALUE}}, /* shaderVarValues */
    {0 /* LIGHTS_OFF */}                                                   /* shaderVarValuesToSkipClassifications */
  };

  createDeferredLightNode();
  resolveGbufferNode = makeResolveGbufferNode(resolve_shader, resolve_cshader, classify_cshader, resolve_permutations);
}
