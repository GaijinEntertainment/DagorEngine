// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "portalRenderer.h"
#include <render/world/cameraParams.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_threadPool.h>
#include <util/dag_convar.h>
#include <math/dag_mathUtils.h>
#include "global_vars.h"
#include <render/deferredRenderer.h>
#include "render/screencap.h"
#include "camera/sceneCam.h"
#include <render/viewVecs.h>
#include <rendInst/visibility.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <shaders/dag_shaderBlock.h>
#include <rendInst/rendInstGenRender.h>
#include <main/main.h>
#include <generic/dag_enumerate.h>
#include <perfMon/dag_statDrv.h>
#include "render/renderEvent.h"
#include <ecs/core/entityManager.h>
#include <shaders/portal_render.hlsli>
#include <rendInst/riExtraRenderer.h>

#include <ecs/camera/getActiveCameraSetup.h>

#include <math/dag_cube_matrix.h>


namespace var
{
static ShaderVariableInfo use_portal_rendering{"use_portal_rendering", true};
static ShaderVariableInfo portal_use_dense_fog{"portal_use_dense_fog", true};
static ShaderVariableInfo portal_fog_start_distance{"portal_fog_start_distance", true};
static ShaderVariableInfo disable_wind{"disable_wind", true};
static ShaderVariableInfo disable_obstacle_interaction{"disable_obstacle_interaction", true};
} // namespace var


namespace WorldRenderPortal
{
static constexpr float Z_NEAR = 0.01f;
static constexpr int cube_res = 512;
// CONSOLE_INT_VAL("render_portal", cube_res, 512, 64, 4096);
CONSOLE_FLOAT_VAL_MINMAX("render_portal", activation_distance_threshold, 4.0f, 0.0f, 100.0f);

CONSOLE_BOOL_VAL("render_portal", draw_once, false);
CONSOLE_BOOL_VAL("render_portal", draw_every_frame, false); // technically not every frame, as it takes multiple frames to render a
                                                            // portal
CONSOLE_INT_VAL("render_portal", ri_min_lod, -1, -1, 5);

// TODO: make it optional, per portal
CONSOLE_FLOAT_VAL_MINMAX("render_portal", portal_pair_instant_activation_distance, 2.0f, 0.0f, 4.0f);

CONSOLE_FLOAT_VAL_MINMAX("render_portal", portal_dissolve_speed, 1.0f, 0.0f, 4.0f);


struct ScopedCameraParams
{
  CameraParams originalCamera;
  CameraParams &cameraRef;

  ScopedCameraParams(CameraParams &orig) : cameraRef(orig), originalCamera(orig) {}

  ~ScopedCameraParams() { cameraRef = originalCamera; }
};

struct ScopedShVars
{
  Color4 originalZnZfar;
  Color4 originalWorldPos;
  Color4 originalVolfogParams;
  int glassDynamicLight;
  int disableWind;
  int disableObstacleInteraction;

  ScopedShVars(const Point3 &in_pos, const PortalParams &portal_params)
  {
    originalZnZfar = ShaderGlobal::get_color4(zn_zfarVarId);
    originalWorldPos = ShaderGlobal::get_color4(world_view_posVarId);
    originalVolfogParams = ShaderGlobal::get_color4(volfog_froxel_range_paramsVarId);
    glassDynamicLight = ShaderGlobal::get_int(glass_dynamic_lightVarId);
    disableWind = ShaderGlobal::get_int(var::disable_wind);
    disableObstacleInteraction = ShaderGlobal::get_int(var::disable_obstacle_interaction);
    ShaderGlobal::set_color4(world_view_posVarId, in_pos.x, in_pos.y, in_pos.z, 1);
    ShaderGlobal::set_color4(zn_zfarVarId, Z_NEAR, portal_params.portalRange, 0, 0);
    ShaderGlobal::set_color4(volfog_froxel_range_paramsVarId, 0, 0, 0, 0); // disable volfog
    ShaderGlobal::set_int(var::portal_use_dense_fog, portal_params.useDenseFog);
    ShaderGlobal::set_real(var::portal_fog_start_distance, portal_params.fogStartDist);
    ShaderGlobal::set_int(glass_dynamic_lightVarId, 0);
    ShaderGlobal::set_int(var::disable_wind, 1);
    ShaderGlobal::set_int(var::disable_obstacle_interaction, 1);
  }

  ~ScopedShVars()
  {
    ShaderGlobal::set_color4(zn_zfarVarId, originalZnZfar);
    ShaderGlobal::set_color4(world_view_posVarId, originalWorldPos);
    ShaderGlobal::set_color4(volfog_froxel_range_paramsVarId, originalVolfogParams);
    ShaderGlobal::set_int(var::portal_use_dense_fog, 0);
    ShaderGlobal::set_real(var::portal_fog_start_distance, 0);
    ShaderGlobal::set_int(glass_dynamic_lightVarId, glassDynamicLight);
    ShaderGlobal::set_int(var::disable_wind, disableWind);
    ShaderGlobal::set_int(var::disable_obstacle_interaction, disableObstacleInteraction);
  }
};

struct ScopedNoExposure
{
  ScopedNoExposure() { g_entity_mgr->broadcastEventImmediate(RenderSetExposure{false}); }
  ~ScopedNoExposure() { g_entity_mgr->broadcastEventImmediate(RenderSetExposure{true}); }
};
} // namespace WorldRenderPortal

using namespace WorldRenderPortal;


static d3d::SamplerHandle get_portal_screen_mask_sampler()
{
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Mirror;
  smpInfo.filter_mode = d3d::FilterMode::Linear;
  return d3d::request_sampler(smpInfo);
}

class PortalVisibilityCullJob final : public cpujobs::IJob
{
  mat44f cullTm;
  Point3 viewPos;
  RiGenVisibility *visibility = nullptr;

public:
  PortalVisibilityCullJob() = default;
  PortalVisibilityCullJob(const PortalVisibilityCullJob &) = delete;

  void start(const mat44f &cull_tm, const Point3 &view_pos, RiGenVisibility *vis)
  {
    cullTm = cull_tm;
    viewPos = view_pos;
    visibility = vis;
    wait();
    threadpool::add(this, threadpool::PRIO_LOW, false);
  }

  void wait() { threadpool::wait(this); }

  const char *getJobName(bool &) const override { return "portal_visibility_cull"; }

  void doJob() override
  {
    G_ASSERT_RETURN(!!visibility, );
    {
      TIME_D3D_PROFILE(portal_render_ri_extra);
      rendinst::prepareRIGenExtraVisibility(cullTm, viewPos, *visibility, false, nullptr, {}, rendinst::RiExtraCullIntention::MAIN,
        false, false, false, nullptr);
    }
    {
      TIME_D3D_PROFILE(portal_render_ri_gen);
      rendinst::prepareRIGenVisibility(Frustum(cullTm), viewPos, visibility, false, nullptr);
    }
    visibility = nullptr;
  }
};

static PortalVisibilityCullJob portal_visibility_cull_job;

void PortalRenderer::PortalVisibility::init(int min_lod)
{
  close();

  riGenVisibility = rendinst::createRIGenVisibility(midmem);
  rendinst::setRIForcedLocalPoolOrder(riGenVisibility, true);
  rendinst::setRIGenVisibilityMinLod(riGenVisibility, min_lod, min_lod);
}

void PortalRenderer::PortalVisibility::close()
{
  portal_visibility_cull_job.wait();
  if (riGenVisibility)
  {
    rendinst::destroyRIGenVisibility(riGenVisibility);
    riGenVisibility = nullptr;
  }
}

RiGenVisibility *PortalRenderer::PortalVisibility::getVisibility() const
{
  portal_visibility_cull_job.wait();
  return riGenVisibility;
}

void PortalRenderer::PortalVisibility::startVisibilityJob(const mat44f &cull_tm, const Point3 &view_pos)
{
  portal_visibility_cull_job.start(cull_tm, view_pos, getVisibility());
}

PortalRenderer::PortalRenderer()
{
  G_STATIC_ASSERT(PortalRenderer::MAX_RENDERED_CUBES < (1 << PORTAL_INDEX_BITS));

  for (auto &activeIndex : activePortalIndices)
    activeIndex = -1;
  for (auto &activeLife : activePortalLifeArr)
    activeLife = 0;

  copyTargetRenderer = PostFxRenderer("copy_texture");

  unsigned texFmt[3] = {TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, TEXFMT_A2B10G10R10, TEXFMT_A8R8G8B8};
  uint32_t expectedRtNum = countof(texFmt);

  renderTargetGbuf.reset();
  renderTargetGbuf = eastl::make_unique<DeferredRenderTarget>("portal_render_resolve", "portal_target_gbuf", cube_res, cube_res,
    DeferredRT::StereoMode::MonoOrMultipass, 0, expectedRtNum, texFmt, TEXFMT_DEPTH32);

  const uint32_t renderTargetFmt = TEXFMT_R11G11B10F;
  tmpRenderTargetTex.close();
  tmpRenderTargetTex = dag::create_tex(NULL, cube_res, cube_res, TEXCF_RTARGET | renderTargetFmt, 1, "portal_target");


  // TODO: compression maybe
  // TODO: mips?

  envCubeTexArr = dag::create_cube_array_tex(cube_res, MAX_RENDERED_CUBES, renderTargetFmt | TEXCF_RTARGET | TEXCF_CLEAR_ON_CREATE, 1,
    "portal_env_cube_arr");
  ShaderGlobal::set_sampler(get_shader_variable_id("portal_env_cube_arr_samplerstate", true), d3d::request_sampler({}));

  const char *shatteredGlassMaskTexName = "shattered_glass_mask_tex_n";
  shatteredGlassMaskTex = dag::get_tex_gameres(shatteredGlassMaskTexName, "portal_screen_mask");
  ShaderGlobal::set_sampler(get_shader_variable_id("portal_screen_mask_samplerstate", true), get_portal_screen_mask_sampler());

  if (!shatteredGlassMaskTex)
    logerr("Failed to load texture %s", shatteredGlassMaskTexName);
  // TODO: use mirror sampler ->> OR better generate it procedurally
}

PortalRenderer::~PortalRenderer() {}

static TMatrix getModifiedCameraMatrix(const TMatrix view_itm)
{
  // the cubemap faces are always rendered in the same direction, and the raycast is modified in the shader to match the transformation
  TMatrix tm = TMatrix::IDENT;
  tm.setcol(3, view_itm.getcol(3));
  return tm;
}

void PortalRenderer::renderCube(int portal_cube_index, CameraParams &camera_params)
{
  TIME_D3D_PROFILE(portal_render);

  {
    const int portalIndex = activePortalIndices[portal_cube_index];
    G_ASSERT_RETURN(portalIndex >= 0 && portalIndex < portalDataVec.size(), );
  }

  G_ASSERT_RETURN(getParamsCb, );
  CallbackParams callbackParams = getParamsCb();

  const auto &cachedActivePortalData = bakingData.getCachedData();
  const Point3 &targetPos = cachedActivePortalData.transform.getcol(3);
  callbackParams.prepareClipmap(targetPos);

  const TMatrix view_itm =
    getModifiedCameraMatrix(cachedActivePortalData.transform) * cube_matrix(TMatrix::IDENT, bakingData.getFace());
  const TMatrix view = inverse(view_itm);
  const Driver3dPerspective persp = Driver3dPerspective(1, 1, Z_NEAR, cachedActivePortalData.params.portalRange);
  const TexStreamingContext currentTexCtx = TexStreamingContext(persp, renderTargetGbuf->getWidth());
  const TMatrix4 proj = matrix_perspective(persp.hk, persp.wk, persp.zn, persp.zf);
  const TMatrix4 globtm_ = TMatrix4(view) * proj;

  mat44f globtm;
  v_mat44_make_from_44cu(globtm, globtm_.m[0]);

  if (bakingData.getStage() == RenderStage::Start)
  {
    portalVisibility.init(ri_min_lod);
    portalVisibility.startVisibilityJob(globtm, targetPos);
    return;
  }

  const RiGenVisibility *riGenVisibilityPtr = portalVisibility.getVisibility();
  G_ASSERT_RETURN(!!riGenVisibilityPtr, );
  const RiGenVisibility &riGenVisibility = *riGenVisibilityPtr;

  const rendinst::render::RiExtraRenderer *riex_renderer = nullptr;

  ScopedShVars shVarsOverrides(targetPos, cachedActivePortalData.params);
  ScopedCameraParams cameraRestore(camera_params);
  ScopedNoExposure noExposureOverride;
  SCOPE_RENDER_TARGET;

  d3d::settm(TM_PROJ, &proj);
  d3d::settm(TM_VIEW, view);
  set_viewvecs_to_shader(view, proj);
  set_inv_globtm_to_shader(view, proj, true);
  const Frustum frustum = Frustum{globtm};
  camera_params.viewItm = view_itm;
  camera_params.jitterFrustum = frustum;
  camera_params.noJitterFrustum = frustum;

  STATE_GUARD_0(ShaderGlobal::set_int(var::use_portal_rendering, VALUE), 1);

  renderTargetGbuf->setRt();

  if (bakingData.getStage() != RenderStage::ClipmapPrepared)
  {
    TIME_D3D_PROFILE(portal_render_warmup);

    if (bakingData.getStage() != RenderStage::WarmupRi)
      return;

    // TODO: prepare texture streaming properly
    constexpr float PREPARE_Z = 1; // for perf: we only prepare ri because of tex streaming, no need to render anything
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, 0, PREPARE_Z, 0);

    FRAME_LAYER_GUARD(globalFrameBlockId);

    {
      TIME_D3D_PROFILE(portal_render_ri_before_draw);
      rendinst::render::before_draw(rendinst::RenderPass::Normal, &riGenVisibility, frustum, nullptr);
    }
    {
      TIME_D3D_PROFILE(portal_render_ri_prepass);
      callbackParams.renderRiPrepass(view_itm, riGenVisibility, currentTexCtx, riex_renderer);
    }
    {
      TIME_D3D_PROFILE(portal_render_ri);
      callbackParams.renderRiNormal(view_itm, riGenVisibility, currentTexCtx, riex_renderer);
    }
    {
      // dummy transparent rendering (gbuffer is used as target), only used for texture streaming
      TIME_D3D_PROFILE(portal_render_ri_trans);
      callbackParams.renderRiTrans(view_itm, riGenVisibility, currentTexCtx, riex_renderer);
    }
    return;
  }

  TIME_D3D_PROFILE(portal_render_final);

  d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, 0, 0, 0);

  {
    FRAME_LAYER_GUARD(globalFrameBlockId);

    {
      TIME_D3D_PROFILE(portal_render_landmesh);
      callbackParams.renderLandmesh(globtm, proj, frustum, targetPos);
    }

    {
      TIME_D3D_PROFILE(portal_render_ri_before_draw);
      rendinst::render::before_draw(rendinst::RenderPass::Normal, &riGenVisibility, frustum, nullptr);
    }

    {
      TIME_D3D_PROFILE(portal_render_ri_prepass);
      callbackParams.renderRiPrepass(view_itm, riGenVisibility, currentTexCtx, riex_renderer);
    }

    {
      TIME_D3D_PROFILE(portal_render_ri);
      callbackParams.renderRiNormal(view_itm, riGenVisibility, currentTexCtx, riex_renderer);
    }

    {
      TIME_D3D_PROFILE(portal_render_resolve);
      renderTargetGbuf->resolve(tmpRenderTargetTex.getTex2D(), view, proj, nullptr, ShadingResolver::ClearTarget::Yes, globtm_);
    }
  }

  {
    TIME_D3D_PROFILE(portal_render_water);
    ShaderGlobal::set_texture(downsampled_far_depth_texVarId, renderTargetGbuf->getDepthId());
    callbackParams.renderWater(tmpRenderTargetTex.getBaseTex(), view_itm, renderTargetGbuf->getDepth());
  }

  {
    TIME_D3D_PROFILE(portal_render_ri_trans);
    FRAME_LAYER_GUARD(globalFrameBlockId);
    d3d::set_render_target({renderTargetGbuf->getDepth(), 0}, DepthAccess::SampledRO, {{tmpRenderTargetTex.getBaseTex(), 0}});
    callbackParams.renderRiTrans(view_itm, riGenVisibility, currentTexCtx, riex_renderer);
  }

  copyFrame(portal_cube_index, bakingData.getFace());
}

static constexpr float PORTAL_INSTANT_BAKE_DISTANCE = 0;

static float calc_portal_distance(const Point3 &portal_pos, const Point3 &target_pos, const Point3 &camera_pos)
{
  if (portal_pair_instant_activation_distance > 0)
  {
    float cameraToTargetDistSq = (target_pos - camera_pos).lengthSq();
    if (cameraToTargetDistSq < portal_pair_instant_activation_distance * portal_pair_instant_activation_distance)
      return PORTAL_INSTANT_BAKE_DISTANCE;
  }
  return (portal_pos - camera_pos).length();
}

static constexpr float INVALID_PORTAL_DISTANCE = 100000;

// TODO: use distance^2
float PortalRenderer::calcDistanceToPortal(int portal_index, const CameraParams &camera_params) const
{
  G_ASSERT_RETURN(portal_index >= 0 && portal_index < (int)portalDataVec.size(), INVALID_PORTAL_DISTANCE);
  auto &data = portalDataVec[portal_index].renderingData;
  return calc_portal_distance(data.sourcePos, data.transform.getcol(3), camera_params.viewItm.getcol(3));
}

void PortalRenderer::getFurthestActivePortal(
  int best_inactive_portal_index, const CameraParams &camera_params, int &cube_slot, float &max_distance)
{
  cube_slot = -1;
  max_distance = 0;
  for (int i = 0; i < activePortalIndices.size(); ++i)
  {
    int portalIndex = activePortalIndices[i];
    G_ASSERTF_RETURN(portalIndex < (int)portalDataVec.size(), , "Inactive portal index: %d, cube slot: %d", portalIndex, i);
    if (portalIndex < 0)
    {
      cube_slot = i;
      max_distance = -1;
      // logerr("Found unoccupied cube slot %d", i);
      break;
    }

    auto &data = portalDataVec[portalIndex];
    if (data.state == PortalState::Inactive || data.state == PortalState::Deleted ||
        (data.state == PortalState::Dirty && best_inactive_portal_index == portalIndex))
    {
      cube_slot = i;
      max_distance = -1;
      // logerr("Found outdated cube slot %d", i);
      break;
    }

    float distance = calcDistanceToPortal(portalIndex, camera_params);
    if (cube_slot < 0 || distance > max_distance)
    {
      cube_slot = i;
      max_distance = distance;
      // logerr("Found better cube slot %d", i);
    }
  }
}

void PortalRenderer::getClosestInactivePortal(const CameraParams &camera_params, int &portal_index, float &min_distance)
{
  portal_index = -1;
  min_distance = INVALID_PORTAL_DISTANCE;
  for (int portalIndex = 0; portalIndex < portalDataVec.size(); ++portalIndex)
  {
    auto &data = portalDataVec[portalIndex];
    if (data.state == PortalState::Deleted)
      continue;
    if (data.state == PortalState::Rendered || data.state == PortalState::IsBaking)
      continue;

    float distance = calcDistanceToPortal(portalIndex, camera_params);
    if (portal_index < 0 || distance < min_distance)
    {
      portal_index = portalIndex;
      min_distance = distance;
    }
  }
}

// among active portals, but only valid ones
int PortalRenderer::getClosestValidPortalCubeSlot(const CameraParams &camera_params) const
{
  int activeIndex = -1;
  float portalDistance = INVALID_PORTAL_DISTANCE;
  for (int i = 0; i < activePortalIndices.size(); ++i)
  {
    int portalIndex = activePortalIndices[i];
    G_ASSERTF_RETURN(portalIndex < (int)portalDataVec.size(), -1, "Inactive portal index: %d, cube slot: %d", portalIndex, i);
    if (portalIndex < 0)
      continue;

    auto &data = portalDataVec[portalIndex];
    if (data.state == PortalState::Deleted)
      continue;

    float distance = calcDistanceToPortal(portalIndex, camera_params);
    if (activeIndex < 0 || distance < portalDistance)
    {
      activeIndex = i;
      portalDistance = distance;
      // logerr("Found better cube slot %d", i);
    }
  }
  return activeIndex;
}

void PortalRenderer::startCubeRendering(int cube_slot, bool is_same_portal_index)
{
  int portalIndex = activePortalIndices[cube_slot];
  if (portalIndex == -1)
  {
    G_ASSERTF(false, "Failed to start cube rendering for slot %d: no active portal", cube_slot);
    return;
  }

  portalDataVec[portalIndex].state = PortalState::IsBaking;
  bakingData.startBaking(cube_slot, portalDataVec[portalIndex].renderingData, is_same_portal_index);
}

void PortalRenderer::updateActivePortalData(const CameraParams &camera_params)
{
  if (bakingData.isBaking()) // first finish rendering the current cube
    return;

  int bestInactivePortalIndex = -1;
  float bestInactivePortalDistance = 0;
  getClosestInactivePortal(camera_params, bestInactivePortalIndex, bestInactivePortalDistance);

  if (bestInactivePortalIndex < 0) // no more portals to activate
  {
    // logerr("No more inactive portals to activate");
    return;
  }

  // if the best inactive portal is already rendered, no need to update
  PortalState state = portalDataVec[bestInactivePortalIndex].state;
  if (getActivePortalCubeSlot(bestInactivePortalIndex) >= 0 && (state == PortalState::Rendered || state == PortalState::IsBaking))
  {
    // logerr("Best inactive portal %d is already rendered", bestInactivePortalIndex);
    return;
  }

  int worstActivePortalCubeSlot = -1;
  float worstActivePortalDistance = 0;
  getFurthestActivePortal(bestInactivePortalIndex, camera_params, worstActivePortalCubeSlot, worstActivePortalDistance);
  G_ASSERT_RETURN(worstActivePortalCubeSlot >= 0, );

  // TODO: use smarter threshold, eg. make it depend on relative distance
  if (worstActivePortalDistance < 0 || bestInactivePortalDistance < worstActivePortalDistance - activation_distance_threshold)
  {
    int prevActivePortalIndex = activePortalIndices[worstActivePortalCubeSlot];
    if (prevActivePortalIndex >= 0)
    {
      portalDataVec[prevActivePortalIndex].state = PortalState::Inactive;
      // logerr("Portal %d deactivated in cube %d", prevActivePortalIndex, worstActivePortalCubeSlot);
    }
    activePortalIndices[worstActivePortalCubeSlot] = bestInactivePortalIndex;

    float bestPortalDistance = calcDistanceToPortal(bestInactivePortalIndex, camera_params);
    bool isBestPortalInstantBaked = bestPortalDistance == PORTAL_INSTANT_BAKE_DISTANCE;
    activePortalLifeArr[worstActivePortalCubeSlot] = isBestPortalInstantBaked ? 1.0f : 0.0f;
    startCubeRendering(worstActivePortalCubeSlot, prevActivePortalIndex == bestInactivePortalIndex);
    // logerr("Portal %d activated in cube %d", bestInactivePortalIndex, worstActivePortalCubeSlot);
  }
  else
  {
    // logerr("Best inactive portal %d is too far, distance: %f, worst active portal distance: %f", bestInactivePortalIndex,
    // bestInactivePortalDistance, worstActivePortalDistance);
  }
}

void PortalRenderer::update(float dt)
{
  for (int cubeSlot = 0; cubeSlot < activePortalIndices.size(); ++cubeSlot)
  {
    if (activePortalIndices[cubeSlot] < 0)
      continue;

    if (bakingData.isCubeBakingNew(cubeSlot))
      continue;

    float &life = activePortalLifeArr[cubeSlot];
    life = clamp(life + dt * portal_dissolve_speed, 0.0f, 1.0f);
  }
}

void PortalRenderer::render(CameraParams &camera_params)
{
  if (!getParamsCb)
    return;

  if (skipFrameCount++ < SKIP_FRAME_COUNT)
    return;

  updateActivePortalData(camera_params);

  if (!bakingData.isBaking() && (draw_every_frame || draw_once))
  {
    draw_once = false;
    int closestActivePortalCubeSlot = getClosestValidPortalCubeSlot(camera_params);
    if (closestActivePortalCubeSlot != -1)
      startCubeRendering(closestActivePortalCubeSlot, true);
  }

  if (bakingData.isBaking())
  {
    renderCube(bakingData.getCubeIndex(), camera_params);
    bakingData.advanceBakingStage();

    if (bakingData.isFinished())
    {
      int activePortalIndex = activePortalIndices[bakingData.getCubeIndex()];
      portalDataVec[activePortalIndex].state = PortalState::Rendered;

      bakingData.resetBaking();

      portalVisibility.close();
    }
  }
}


void PortalRenderer::copyFrameImpl(int cube_index, int face_start, int face_count, int from_mip, int mips_count)
{
  int mips = 1; // TODO: maybe use mips
  const int cubeSize = cube_res;
  ArrayTexture *arrayTex = envCubeTexArr.getArrayTex();
  for (int mip = from_mip; mip < from_mip + mips_count; ++mip)
  {
    const int cubeMipSide = max(1, cubeSize >> min(mip, mips - 1));
    for (int faceNumber = face_start; faceNumber < face_start + face_count; ++faceNumber)
    {
      const int dstCubeFace = mip + mips * (faceNumber + cube_index * 6);
      arrayTex->updateSubRegion(tmpRenderTargetTex.getTex2D(), mip, 0, 0, 0, cubeMipSide, cubeMipSide, 1, dstCubeFace, 0, 0, 0);
    }
  }
}

void PortalRenderer::copyFrame(int cube_index, int cube_face)
{
  TIME_D3D_PROFILE(portal_render_copy_face);
  copyFrameImpl(cube_index, cube_face, 1, 0, 1);
}

bool PortalRenderer::checkPortalIndex(int portal_index) const
{
  if (portal_index < 0 || portal_index >= (int)portalDataVec.size())
  {
    logerr("Portal index out of bounds: %d", portal_index);
    return false;
  }
  return true;
}

// TODO: we don't need to rebake portals if only TM orientations change, also, life shouldn't change
void PortalRenderer::updatePortal(int portal_index, const TMatrix &source_tm, const TMatrix &target_tm, const PortalParams &params)
{
  if (!checkPortalIndex(portal_index))
    return;

  auto &data = portalDataVec[portal_index];
  data.state = PortalState::Dirty;

  data.renderingData.params = params;
  data.renderingData.sourcePos = source_tm.getcol(3);

  TMatrix tmpSrcTm = source_tm;
  tmpSrcTm.setcol(3, Point3::ZERO);
  TMatrix tmpDstTm = target_tm;
  tmpDstTm.setcol(3, Point3::ZERO);
  data.renderingData.transform = tmpSrcTm * inverse(tmpDstTm);
  data.renderingData.transform.setcol(3, target_tm.getcol(3));

  if (bakingData.isBaking() && activePortalIndices[bakingData.getCubeIndex()] == portal_index)
    bakingData.resetBaking();

  // logerr("Portal %d updated", portal_index);
}


int PortalRenderer::allocatePortal()
{
  for (int i = 0; i < portalDataVec.size(); ++i)
  {
    if (portalDataVec[i].state == PortalState::Deleted)
    {
      portalDataVec[i].state = PortalState::Inactive;
      return i;
    }
  }
  int portalIndex = portalDataVec.size();
  auto &data = portalDataVec.push_back();
  data.state = PortalState::Inactive;
  return portalIndex;
}

void PortalRenderer::freePortal(int portal_index)
{
  if (!checkPortalIndex(portal_index))
    return;

  int activeCubeIndex = getActivePortalCubeSlot(portal_index);
  if (activeCubeIndex >= 0)
  {
    activePortalIndices[activeCubeIndex] = -1;
    activePortalLifeArr[activeCubeIndex] = 0;

    if (bakingData.isBaking() && bakingData.getCubeIndex() == activeCubeIndex)
      bakingData.resetBaking();
  }

  auto &data = portalDataVec[portal_index];
  data.state = PortalState::Deleted;
  // logerr("Portal %d deleted", portal_index);
}

int PortalRenderer::getActivePortalCubeSlot(int portal_index) const
{
  for (int i = 0; i < activePortalIndices.size(); ++i) // max active portal count is tiny
    if (activePortalIndices[i] == portal_index)
      return i;
  return -1;
}

int PortalRenderer::getRenderedPortalCubeSlot(int portal_index, TMatrix &portal_tm, float &life, bool &is_bidirectional) const
{
  life = 0;
  is_bidirectional = false;

  if (!checkPortalIndex(portal_index))
    return -1;

  int cubeSlot = getActivePortalCubeSlot(portal_index);
  if (cubeSlot < 0)
    return -1;

  if (bakingData.isCubeBakingNew(cubeSlot))
    return -1;

  auto &data = portalDataVec[portal_index].renderingData;
  portal_tm = data.transform;
  is_bidirectional = data.params.isBidirectional;

  life = activePortalLifeArr[cubeSlot];

  return cubeSlot;
}


void portal_renderer_mgr::update_portal(
  int portal_index, const TMatrix &source_tm, const TMatrix &target_tm, const PortalParams &params)
{
  if (auto portalRenderer = query_portal_renderer())
    portalRenderer->updatePortal(portal_index, source_tm, target_tm, params);
}
int portal_renderer_mgr::allocate_portal()
{
  if (auto portalRenderer = query_portal_renderer())
    return portalRenderer->allocatePortal();
  logerr("PortalRenderer not found during allocatePortal");
  return -1;
}
void portal_renderer_mgr::free_portal(int portal_index)
{
  if (auto portalRenderer = query_portal_renderer())
    portalRenderer->freePortal(portal_index);
}
int portal_renderer_mgr::get_rendered_portal_cube_index(int portal_index, TMatrix &portal_tm, float &life, bool &is_bidirectional)
{
  if (auto portalRenderer = query_portal_renderer())
    return portalRenderer->getRenderedPortalCubeSlot(portal_index, portal_tm, life, is_bidirectional);
  return -1;
}