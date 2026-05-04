// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <ecs/phys/physEvents.h>

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
#include <render/world/bvh.h>


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
static ShaderVariableInfo gi_enable_lights_at_radiance_traceVarId("gi_enable_lights_at_radiance_trace", true);

extern ShaderBlockIdHolder rendinstVoxelizeSceneBlockId;
extern float cascade0Dist;

struct GiDatablockCache
{
  float voxelSceneResScale = 0.5f;
  uint8_t sdfClips = 0; // 0 - autodetect
  float sdfVoxel0 = 0.15f;
  uint8_t albedoClips = 3;
  uint8_t radianceGridClipW = 28;
  uint8_t radianceGridClips = 4;
};

static GiDatablockCache giDatablockCache;

static void update_gi_datablock_cache()
{
  const DataBlock *graphicsGI = dgs_get_settings()->getBlockByNameEx("graphics")->getBlockByNameEx("gi");
  giDatablockCache.voxelSceneResScale = graphicsGI->getReal("voxelSceneResScale", giDatablockCache.voxelSceneResScale);
  giDatablockCache.sdfClips = graphicsGI->getInt("sdfClips", giDatablockCache.sdfClips);
  giDatablockCache.sdfVoxel0 = clamp(graphicsGI->getReal("sdfVoxel0", giDatablockCache.sdfVoxel0), 0.065f, 0.45f);
  giDatablockCache.albedoClips = graphicsGI->getInt("albedoClips", giDatablockCache.albedoClips);
  giDatablockCache.radianceGridClipW = graphicsGI->getInt("radianceGridClipW", giDatablockCache.radianceGridClipW);
  giDatablockCache.radianceGridClips = graphicsGI->getInt("radianceGridClips", giDatablockCache.radianceGridClips);
}

static void set_voxelization_transform(int max_res, IPoint3 res, const BBox3 &box)
{
  const Point3 voxelize_box_sz = max(Point3(1e-6f, 1e-6f, 1e-6f), box.width());

  // use fixed aspect ratio of 1. for oversampling and HW clipping
  const Point3 voxelize_aspect_ratio = point3(max(res, IPoint3(1, 1, 1))) / max_res;

  const Point3 mult = 2. * div(voxelize_aspect_ratio, voxelize_box_sz);
  const Point3 add = -mul(box[0], mult) - voxelize_aspect_ratio;
  ShaderGlobal::set_float4(voxelize_world_to_rasterize_space_mulVarId, P3D(mult), 0);
  ShaderGlobal::set_float4(voxelize_world_to_rasterize_space_addVarId, P3D(add), 0);
}

void WorldRenderer::processGIInvalidationRequests()
{
  if (enviProbeNeedsReload)
    return;

  if (pendingFullGiInvalidationRequest.has_value())
  {
    doInvalidateGI(pendingFullGiInvalidationRequest->force);
  }
  else if (pendingPartialGiInvalidationRequest.has_value())
  {
    doInvalidateGI(pendingPartialGiInvalidationRequest->modelBbox, pendingPartialGiInvalidationRequest->tm,
      pendingPartialGiInvalidationRequest->approx);
  }

  pendingFullGiInvalidationRequest = eastl::nullopt;
  pendingPartialGiInvalidationRequest = eastl::nullopt;
}

ECS_TAG(render)
ECS_REQUIRE(ecs::Tag hero)
ECS_NO_ORDER
ECS_ON_EVENT(EventOnEntityTeleported)
static void wr_handle_on_hero_teleport_es(const ecs::Event &)
{
  if (auto *wr = get_world_renderer())
    static_cast<WorldRenderer *>(wr)->onHeroTeleportation();
}

void WorldRenderer::processHeroTeleportation()
{
  if (!hasPendingHeroTeleportation)
    return;

  const float cameraMove = length(currentFrameCamera.cameraWorldPos - prevFrameCamera.cameraWorldPos);
  const float teleportationDistanceThreshold =
    dgs_get_settings()->getBlockByNameEx("graphics")->getReal("teleportationDistanceThreshold", 3.0);

  if (cameraMove <= teleportationDistanceThreshold)
  {
    debug("WR: hero teleportation: skipped render invalidation because movement is too short (%f <= %f)", cameraMove,
      teleportationDistanceThreshold);
    hasPendingHeroTeleportation = false;
    return;
  }

  extern void invalidate_ssr_history(int frames);
  giUpdatePosFrameCounter = 0;
  invalidateGI(true);
  invalidate_ssr_history(3);

  hasPendingHeroTeleportation = false;
}

void WorldRenderer::doInvalidateGI(const bool force)
{
  G_UNUSED(force);
  // todo: support force
  if (daGI2)
    daGI2->afterReset();
  if (treesAbove)
    treesAbove->invalidate();
}

void WorldRenderer::invalidateGI(const bool force) { pendingFullGiInvalidationRequest = GiFullInvalidationRequest{.force = force}; }
void WorldRenderer::invalidateGI(const BBox3 &model_bbox, const TMatrix &tm, const BBox3 &approx)
{
  pendingPartialGiInvalidationRequest = GiPartialInvalidateRequest{.modelBbox = model_bbox, .tm = tm, .approx = approx};
}

void WorldRenderer::drawGIDebug(const Frustum &camera_frustum)
{
  G_UNUSED(camera_frustum);
  if (daGI2)
    daGI2->debugRenderTrans();
}

// static const int DEFERRED_FRAMES_TO_INVALIDATE_GI = 9;
static const int MAX_GI_FRAMES_TO_INVALIDATE_POS = 32;
static ShaderVariableInfo use_hw_rt_giVarId("use_hw_rt_gi", true);

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
    case GiAlgorithm::HIGH: gi_quality.set(GI_SCREEN_PROBES); break;
    default:
      logwarn("Invalid GI quality");
      gi_quality.set(GI_ONLY_AO);
      break;
  }
  if (is_rtgi_enabled())
  {
    gi_quality.set(GI_SCREEN_PROBES);
    ShaderGlobal::set_int(use_hw_rt_giVarId, 2);
  }
  else
  {
    ShaderGlobal::set_int(use_hw_rt_giVarId, 0);
    if (is_ptgi_enabled())
      gi_quality.set(GI_COLORED);
  }
  gi_algorithm_quality.set(quality.algorithmQuality);

  initGI();

  gi_enabled.set(true);
  giUpdatePosFrameCounter = 0;

  update_gi_datablock_cache();
}

bool WorldRenderer::giNeedsReprojection()
{
  return hasFeature(FeatureRenderFlags::DEFERRED_LIGHT) && daGI2 && gi_quality == GI_SCREEN_PROBES &&
         daGI2->getNextSettings().screenProbes.tileSize > 8;
}

uint32_t WorldRenderer::getGIHistoryFrames() const { return daGI2 ? daGI2->getHistoryFrames() : 0; }

void WorldRenderer::setGILightsToShader(bool allow_frustum_lights)
{
  if (!lights.initialized())
    return;

  if (giGlobalLightsPrepared)
    lights.setOutOfFrustumLightsToShader(); // Relying on out-of-frustum culling result being unchanged after
                                            // WorldRenderer::updateGIPos().
  else if (allow_frustum_lights)
    lights.setInsideOfFrustumLightsToShader();
  else
    lights.setEmptyOutOfFrustumLights();
}

uint32_t get_gi_history_frames() { return static_cast<WorldRenderer *>(get_world_renderer())->getGIHistoryFrames(); }

void WorldRenderer::overrideGISettings(const DaGISettings &settings)
{
  if (!giSettingsOverride)
    giSettingsOverride = eastl::make_unique<DaGISettings>();
  *giSettingsOverride = settings;
}

void WorldRenderer::resetGISettingsOverride() { giSettingsOverride.reset(); }

DaGISettings WorldRenderer::getGISettings() const
{
  if (giSettingsOverride)
    return *giSettingsOverride;

  auto s = daGI2->getNextSettings();
  constexpr float ALBEDO_MEDIA_MUL = 0.0625f;
  constexpr float LIT_SCENE_MUL = 0.0625f;
  s.voxelScene.sdfResScale = giDatablockCache.voxelSceneResScale;
  const bool temporality = gi_quality == GI_SCREEN_PROBES || gi_algorithm_quality > 0.0001;

  const float temporalSpeed = temporality ? lerp<float>(0.05, 1, gi_algorithm_quality) : 0;
  s.mediaScene.temporalSpeed = temporalSpeed * ALBEDO_MEDIA_MUL;
  s.voxelScene.temporalSpeed = temporalSpeed * LIT_SCENE_MUL;
  s.albedoScene.temporalSpeed = temporalSpeed * ALBEDO_MEDIA_MUL;
  s.sdf.clips = clamp(giDatablockCache.sdfClips > 0 ? giDatablockCache.sdfClips : (bareMinimumPreset ? 5 : 6), 4, 6);
  s.sdf.voxel0Size = giDatablockCache.sdfVoxel0;
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
    s.albedoScene.clips = giDatablockCache.albedoClips;
    s.radianceGrid.w = giDatablockCache.radianceGridClipW;
    s.radianceGrid.clips = giDatablockCache.radianceGridClips;
    s.radianceGrid.irradianceProbeDetail = 2.f;
    s.radianceGrid.additionalIrradianceClips = 2;
    s.screenProbes.tileSize = 0;
    s.volumetricGI.tileSize = 64;
    s.radianceGrid.temporalSpeed = temporality ? 1.0f / lerp(96.0f, 32.0f, gi_algorithm_quality) : 0.0f;
  }
  else if (gi_quality == GI_SCREEN_PROBES)
  {
    s.skyVisibility.clipW = 0;
    s.radianceGrid.w = giDatablockCache.radianceGridClipW;
    s.radianceGrid.clips = giDatablockCache.radianceGridClips;
    s.radianceGrid.irradianceProbeDetail = 1.f;
    s.radianceGrid.additionalIrradianceClips = is_rtr_enabled() ? 2 : 0;
    s.albedoScene.clips = giDatablockCache.albedoClips;

    s.screenProbes.temporality = giDynamicQuality;

    // minimum s.screenProbes.tileSize is 20 at 1080p, 24 at 1440p, 32 at 4k
    int w, h;
    getPostFxInternalResolution(w, h);
    float minTileSize = cvt_ex(h, 1080, 1080 * 2, 16, 32);
    float tileSizeF = cvt(gi_algorithm_quality, 0.25, 0.95f, minTileSize, 8);
    s.screenProbes.tileSize = static_cast<int>(round(tileSizeF)) & ~1;

    float radianceOctResF = cvt(gi_algorithm_quality, 0.f, 0.5f, 4, 8);
    s.screenProbes.radianceOctRes = static_cast<int>(round(radianceOctResF));

    s.screenProbes.angleFiltering = gi_algorithm_quality > 0.975f;
    s.volumetricGI.tileSize = (int(lerp(64.f, 48.f, gi_algorithm_quality)) + 1) & ~1;
    s.volumetricGI.slices = int(lerp(16.f, 24.f, gi_algorithm_quality));
  }

  return s;
}

void WorldRenderer::giBeforeRender()
{
  if (daGI2 && !gi_enabled.get())
    closeGI();

  if (!daGI2 && gi_enabled.get())
    initGI();

  ShaderGlobal::set_int(gi_qualityVarId, daGI2 ? gi_quality : -1);

  giGlobalLightsPrepared = false;

  if (!daGI2)
    return;
  TIME_D3D_PROFILE(gi2_before_render)

  if (gi_algorithm_quality >= 0.75f)
    giDynamicQuality = cvt(gi_algorithm_quality, 0.75f, 1.0f, 0.55f, 1.0f);
  else
    giDynamicQuality = cvt(gi_algorithm_quality, 0.25f, 0.5f, 0.1f, 0.55f);

  int w, h, maxW, maxH;
  getRenderingResolution(w, h);
  getMaxPossibleRenderingResolution(maxW, maxH);

  const bool giNeededReprojection = giNeedsReprojection();

  DaGISettings s = getGISettings();
  daGI2->setSettings(s);

  // If we have voxels lit scene lights are handled inside its rasterization / updation.
  bool hasVoxelsLitScene = s.voxelScene.sdfResScale > 0.0f;
  bool useGiGlobalLights = gi_quality > GI_ONLY_AO && !hasVoxelsLitScene;
  if (useGiGlobalLights)
    useGiGlobalLights &= !ShaderGlobal::is_var_assumed(gi_enable_lights_at_radiance_traceVarId) ||
                         ShaderGlobal::get_interval_assumed_value(gi_enable_lights_at_radiance_traceVarId) > 0;
  if (useGiGlobalLights)
  {
    int sdfClipsWithLights = s.sdf.clips - 2; // Could be a parameter.
    float xzSize = (1 << (sdfClipsWithLights - 1)) * s.sdf.texWidth * s.sdf.voxel0Size;
    float ySize = xzSize * s.sdf.yResScale;
    Point3 center = currentFrameCamera.viewItm.getcol(3);
    Point3 halfSize = Point3(xzSize, ySize, xzSize) * 0.5f;
    giCurrentGlobalLightsBox = BBox3(center - halfSize, center + halfSize);
  }
  else
  {
    giCurrentGlobalLightsBox.setempty();
  }
  ShaderGlobal::set_int(gi_enable_lights_at_radiance_traceVarId, useGiGlobalLights ? 1 : 0);

  ShaderGlobal::set_int(ssr_alternate_reflectionsVarId, ssrWantsAlternateReflections && gi_quality > 0 ? 1 : 0);
  // if InitGI called only on level load it can cause GI artifacts, for some reason.
  // Calling it a bit later, or invalidating GI fixes the issue once and for all.
  // if (++giInvalidateDeferred == DEFERRED_FRAMES_TO_INVALIDATE_GI)
  //  daGI2->invalidateDeferred();
  // we check if static shadows are valid in the pos where we moved to
  // static shadows are valid if: either it is completely valid, or it is within the region (although there might be invalid regions)
  // region size is decreased each frame
  const float giUpdateDist = 100.f; // fixme:
  DaGI::RadianceUpdate ru = DaGI::RadianceUpdate::On;
  // todo: check teleportation and set giUpdatePosFrameCounter = 0 on teleportation
  if (!ignoreStaticShadowsForGI && ++giUpdatePosFrameCounter <= MAX_GI_FRAMES_TO_INVALIDATE_POS) // if frames passed < threshold
  {
    auto turnOffGiUpdate = [&]() {
      auto staticShadows = shadowsManager.getStaticShadows();
      if (!staticShadows)
        return true;
      const Point3 &cameraPos = currentFrameCamera.viewItm.getcol(3);
      BBox3 shBox = staticShadows->getWorldBox();
      Point3 pos = min(max(cameraPos, shBox[0] + Point3(giUpdateDist, giUpdateDist, giUpdateDist)),
        shBox[1] - Point3(giUpdateDist, giUpdateDist, giUpdateDist));
      BBox3 checkBox = BBox3(pos, giUpdateDist);
      checkBox[0] = max(checkBox[0], shBox[0]);
      checkBox[1] = min(checkBox[1], shBox[1]);

      if (!staticShadows->isInside(checkBox)    // we are not even inside static shadows
          || !staticShadows->isValid(checkBox)) // static shadows are invalid
        return true;
      // if static shadows are fully updated, render immediately, otherwise try to wait for one frame
      if (giUpdatePosFrameCounter == 1 && !staticShadows->isCompletelyValid(checkBox))
        return true;
      return false;
    }();
    if (turnOffGiUpdate)
      ru = DaGI::RadianceUpdate::Off;
  }

  daGI2->beforeRender(w, h, maxW, maxH, currentFrameCamera.viewItm, currentFrameCamera.jitterProjTm,
    currentFrameCamera.noJitterPersp.zn, currentFrameCamera.noJitterPersp.zf, ru);

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

  if (gi_quality.pullValueChange() || gi_algorithm_quality.pullValueChange() || giNeededReprojection != giNeedsReprojection())
  {
    requestFgRecreation("gi shvars");
  }
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

      STATE_GUARD_0(ShaderGlobal::set_int(gbuffer_for_treesaboveVarId, VALUE), depth_min ? 0 : 1);

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
    ShaderGlobal::set_float4(voxelize_box0VarId, box[0].x, box[0].y, box[0].z, 0);
    ShaderGlobal::set_float4(voxelize_box1VarId, 1. / box.width().x, 1 / box.width().y, 1 / box.width().z, 0);
    bool ret = false;
    // rendinst::prepareRIGenExtraVisibility(globtm, box.center(), *rendinst_voxelize_visibility, false, NULL);
    bbox3f boxCull = v_ldu_bbox3(box);
    // box visibility, skip small ri
    bbox3f actualBox;
    v_bbox3_init_empty(actualBox);
    rendinst::prepareRIGenExtraVisibilityBox(boxCull, 3, voxelSize.x * 0.5, 71, *rendinst_voxelize_visibility, &actualBox);
    if (v_bbox3_test_box_intersect_safe(actualBox, boxCull))
    {
      box[0].y = floorf(v_extract_y(actualBox.bmin) / voxelSize.y) * voxelSize.y;
      box[1].y = ceilf(v_extract_y(actualBox.bmax) / voxelSize.y) * voxelSize.y;
      const int resY = int(ceilf((box[1].y - box[0].y) / voxelSize.y));

      set_voxelization_transform(maxRes, IPoint3(res.x, resY, res.z), box);

      SCENE_LAYER_GUARD(rendinstVoxelizeSceneBlockId);
      rendinst::render::renderRIGen(rendinst::RenderPass::VoxelizeAlbedo, rendinst_voxelize_visibility, view_itm,
        rendinst::LayerFlag::Opaque, rendinst::OptimizeDepthPass::No, 3);
      ret = true;
    }

    set_voxelization_transform(maxRes, res, box_);
    RenderAlbedoVoxelization ecsRenderEvent(box_, false);
    g_entity_mgr->broadcastEventImmediate(ecsRenderEvent);
    const bool ecsRenderedAnything = ecsRenderEvent.get<1>();
    ret |= ecsRenderedAnything;

    reset_voxelization_override();
    ShaderElement::invalidate_cached_state_block();

    return ret;
  };

  Point3 giPos = pos;
  G_UNUSED(hmax);
  giPos.y = min(giPos.y, hmax + giHmapOffsetFromUp);
  giPos.y = max(giPos.y, hmin + giHmapOffsetFromLow);

  bool lightsInsideFrustum = true;
  bool useGiGlobalLights = !giCurrentGlobalLightsBox.isempty();
  if (lights.initialized() && useGiGlobalLights)
  {
    TIME_PROFILE(cull_global_lights)
    alignas(16) TMatrix4 globtm = matrix_ortho_off_center_lh(giCurrentGlobalLightsBox[0].x, giCurrentGlobalLightsBox[1].x,
      giCurrentGlobalLightsBox[1].y, giCurrentGlobalLightsBox[0].y, giCurrentGlobalLightsBox[0].z, giCurrentGlobalLightsBox[1].z);
    lights.cullOutOfFrustumLights((mat44f_cref)globtm, SpotLightMaskType::SPOT_LIGHT_MASK_GI, OmniLightMaskType::OMNI_LIGHT_MASK_GI);
    lights.setOutOfFrustumLightsToShader();
    lightsInsideFrustum = false;
    giGlobalLightsPrepared = true;
  }

  TIME_D3D_PROFILE(gi2_update_pos)
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
      {
        TIME_PROFILE(sort_instances)
        stlsort::sort(handles.begin(), handles.end(),
          [](auto a, auto b) { return rendinst::handle_to_ri_type(a) < rendinst::handle_to_ri_type(b); });
      }
      if (lights.initialized() && !useGiGlobalLights)
      {
        float sdfVoxelizeQuality = float(gi_algorithm_quality) * (float(gi_quality) / 2.0);
        float maxSz = max(float(min_sdf_voxel_with_lights_size), float(max_sdf_voxel_with_lights_size));
        if (voxelSize <= lerp(float(min_sdf_voxel_with_lights_size), maxSz, sdfVoxelizeQuality) &&
            voxelSize <= world_sdf_from_collision_rasterize_below && daGI2->sdfClipHasVoxelsLitSceneClip(sdf_clip))
        {
          TIME_PROFILE(cull_lights)
          alignas(16) TMatrix4 globtm = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].y, box[0].y, box[0].z, box[1].z);
          lights.cullOutOfFrustumLights((mat44f_cref)globtm, SpotLightMaskType::SPOT_LIGHT_MASK_GI,
            OmniLightMaskType::OMNI_LIGHT_MASK_GI);
          lights.setOutOfFrustumLightsToShader();
        }
        else
          lights.setEmptyOutOfFrustumLights();
        lightsInsideFrustum = false;
      }
      IPoint3 res = ipoint3(floor(sceneBox.width() / voxelSize + Point3(0.5, 0.5, 0.5)));
      // Point3 voxelize_box1 = Point3(1./sceneBox.width().x, 1./sceneBox.width().y, 1./sceneBox.width().z);
      const int maxRes = max(max(res.x, res.y), max(res.z, 1));

      SCOPE_RENDER_TARGET;
      set_voxelization_transform(maxRes, res, sceneBox);
      set_voxelization_target_and_override(maxRes * world_sdf_rasterize_supersample, maxRes * world_sdf_rasterize_supersample);

      if (!handles.empty())
      {
        if (voxelSize <= world_sdf_from_collision_rasterize_below)
        {
          bool prims = rasterize_sdf_prims.get() && d3d::get_driver_desc().shaderModel < 6.1_sm;
          voxelizeCollision->rasterizeSDF(*lruCollision, dag::Span<uint64_t>(handles.data(), handles.size()), prims);
        }
        else
        {
          voxelizeCollision->voxelizeSDFCompute(*lruCollision, dag::Span<uint64_t>(handles.data(), handles.size()));
        }
      }

      RenderSdfVoxelization ecsRenderEvent(sceneBox, false);
      g_entity_mgr->broadcastEventImmediate(ecsRenderEvent);
      const bool ecsRenderedAnything = ecsRenderEvent.get<1>();

      reset_voxelization_override();
      ShaderElement::invalidate_cached_state_block();

      bool intersectLevel = !handles.empty() || ecsRenderedAnything;
      return intersectLevel ? UpdateGiQualityStatus::RENDERED : UpdateGiQualityStatus::NOTHING;
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
const StaticSceneRayTracer *get_frt(); // To consider: move to public interface?
}

void WorldRenderer::initGIWindows(eastl::unique_ptr<scene::TiledScene> &&windows)
{
  giWindows.reset();
  giWindows = eastl::make_unique<GIWindows>();
  giWindows->init(eastl::move(windows));
}

void WorldRenderer::initGI()
{
  requestFgRecreation("initGI");

  closeGI();
  if (getGiQuality().algorithm == GiAlgorithm::OFF)
  {
    createGiNodes();
    return;
  }

  if (const StaticSceneRayTracer *frt = dacoll::get_frt())
  {
    if (frt->getFacesCount())
    {
      if (frt->getVertsCount() > 65536)
        logerr("not supporting frt dumps with %d verts correctly. Split into two collision resources", frt->getVertsCount());

      uint32_t indicesCnt = frt->getFacesCount() * 3;
      uint16_t *indices = (uint16_t *)midmem->alloc(indicesCnt * sizeof(uint16_t));
      uint32_t validIndices = 0;
      for (uint32_t j = 0, f = 0; j < indicesCnt; j += 3, ++f)
      {
        uint32_t v0 = frt->faces(f).v[0], v1 = frt->faces(f).v[1], v2 = frt->faces(f).v[2];
        if (v0 >= 65536 || v1 >= 65536 || v2 >= 65536)
          continue;
        indices[validIndices + 0] = v0;
        indices[validIndices + 1] = v1;
        indices[validIndices + 2] = v2;
        validIndices += 3;
      }
      staticSceneCollisionResource.reset(CollisionResource::createSingleMeshNonOwningVerts(
        dag::Span<Point3_vec4>((Point3_vec4 *)&frt->verts(0), min<uint32_t>(65536, frt->getVertsCount())),
        dag::Span<uint16_t>(indices, validIndices), frt->getBox(), frt->getSphere()));
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
  if (giWindows)
    giWindows->afterReset();
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

  rendinst::destroyRIGenVisibility(rendinst_trees_visibility);
  rendinst_trees_visibility = nullptr;

  requestFgRecreation("close GI");
}

const scene::TiledScene *WorldRenderer::getWallsScene() const { return nullptr; }

const scene::TiledScene *WorldRenderer::getWindowsScene() const { return giWindows ? giWindows->windows.get() : nullptr; }

void WorldRenderer::doInvalidateGI(const BBox3 &model_bbox, const TMatrix &tm, const BBox3 &approx)
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

DaGI *WorldRenderer::getGI() { return daGI2; }
