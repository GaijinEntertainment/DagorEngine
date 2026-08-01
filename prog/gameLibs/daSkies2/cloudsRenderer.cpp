// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cloudsRenderer.h"
#include "cloudsShaderVars.h"

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_viewScissor.h>

#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_shaderVarsUtils.h>

#include <math/dag_frustum.h>

#include <render/set_reprojection.h>
#include <render/viewVecs.h>

#include <EASTL/optional.h>
#include <util/dag_convar.h>

#include <osApiWrappers/dag_miscApi.h>

CONSOLE_BOOL_VAL("clouds", use_compute, true);
// parallax rate (rad/s) at which the checker history clamp is fully collapsed to
// the classic neighborhood box
CONSOLE_FLOAT_VAL("clouds", checker_clamp_rate, 0.08f);

// scales the erosion scroll term of the TAA history advection: the cloud envelope
// moves at the origin rate only, so 1 (full advection) matches interior erosion
// detail but makes cloud edges lead under wind; 0 advects at the envelope rate
// and softens interior erosion detail under the EMA instead
CONSOLE_FLOAT_VAL_MINMAX("clouds", taa_erosion_advect, 0.f, 0.0f, 1.0f);

void CloudsRenderer::renderTiledDist(CloudsRendererData &data)
{
  if (!data.clouds_tile_distance)
    return;
  TIME_D3D_PROFILE(tiledDist);
  if (data.useCompute)
  {
    TextureInfo ti;
    data.clouds_tile_distance_tmp->getinfo(ti, 0);
    d3d::set_rwtex(STAGE_CS, 0, data.clouds_tile_distance_tmp.getTex2D(), 0, 0);
    clouds_tile_dist_prepare_cs->dispatchThreads(ti.w, ti.h, 1);
    d3d::set_rwtex(STAGE_CS, 0, data.clouds_tile_distance.getTex2D(), 0, 0);
    clouds_tile_dist_min_cs->dispatchThreads(ti.w, ti.h, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }
  else
  {
    d3d::set_render_target({}, DepthAccess::RW, {{data.clouds_tile_distance_tmp.getTex2D(), 0, 0}});
    clouds_tile_dist_prepare_ps.render();
    d3d::set_render_target({}, DepthAccess::RW, {{data.clouds_tile_distance.getTex2D(), 0, 0}});
    clouds_tile_dist_min_ps.render();
  }
  if (clouds_tile_dist_count_cs && data.clouds_close_layer_is_outside)
  {
    TIME_D3D_PROFILE(non_empty_tiles);
    TextureInfo ti;
    data.clouds_tile_distance_tmp->getinfo(ti, 0);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), data.clouds_close_layer_is_outside.getBuf());
    clouds_tile_dist_count_cs->dispatchThreads(ti.w, ti.h, 1);
  }
}

void CloudsRenderer::init()
{
  clouds2_apply.init("clouds2_apply");
  if (d3d::get_driver_desc().shaderModel >= 5.0_sm)
  {
#define INIT(a) a.reset(new_compute_shader(#a, false));
    INIT(clouds2_temporal_cs);
    INIT(clouds2_close_temporal_cs);
    INIT(clouds2_taa_cs);
    INIT(clouds2_taa_cs_has_empty);
    INIT(clouds2_taa_cs_no_empty);
    INIT(clouds_close_layer_outside);
    INIT(clouds_close_layer_clear);
    INIT(clouds_tile_dist_count_cs);
    INIT(clouds_tile_dist_prepare_cs);
    INIT(clouds_tile_dist_min_cs);
    INIT(clouds_create_indirect);
    INIT(clouds2_apply_blur_cs)
#undef INIT
  }
#define INIT(a) a.init(#a);
  INIT(clouds2_temporal_ps)
  INIT(clouds2_close_temporal_ps)
  INIT(clouds2_taa_ps)
  INIT(clouds2_taa_ps_has_empty)
  INIT(clouds2_taa_ps_no_empty)
  INIT(clouds2_direct)
  INIT(clouds_tile_dist_prepare_ps)
  INIT(clouds_tile_dist_min_ps)
  INIT(clouds2_apply_has_empty)
  INIT(clouds2_apply_no_empty)
  INIT(clouds2_apply_blur_ps)
#undef INIT
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    ShaderGlobal::set_sampler(clouds_target_depth_gbuf_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
}

CloudsRenderer::DispatchGroups2D CloudsRenderer::set_dispatch_groups(int w, int h, int w_sz, int h_sz, bool lowres_close_clouds)
{
  union
  {
    Color4 dgf;
    struct
    {
      int x, y, z, w;
    } dg;
  } castDG;
  castDG.dg.x = (w + w_sz - 1) / w_sz;
  castDG.dg.y = (h + h_sz - 1) / h_sz;
  castDG.dg.z = ((lowres_close_clouds ? w / 2 : w) + w_sz - 1) / w_sz;
  castDG.dg.w = ((lowres_close_clouds ? h / 2 : h) + h_sz - 1) / h_sz;
  ShaderGlobal::set_float4(clouds2_dispatch_groupsVarId, castDG.dgf);
  return DispatchGroups2D{castDG.dg.x, castDG.dg.y, castDG.dg.z, castDG.dg.w};
}

int CloudsRenderer::getNotLesserDepthLevel(CloudsRendererData &data, int &depth_levels, Texture *depth)
{
  depth_levels = 1;
  if (!depth)
    return 0;
  depth_levels = depth->level_count();
  if (!data.cloudTexRes.x)
    return 0;
  TextureInfo depthInfo;
  depth->getinfo(depthInfo, 0);
  int level = 0;
  for (int mw = depthInfo.w, mh = depthInfo.h; mw >= data.cloudTexRes.x && mh >= data.cloudTexRes.y && level < depth_levels;
       mw >>= 1, mh >>= 1, level++)
    ;
  level = max(0, level - 1);
  return level;
}

void CloudsRenderer::setCloudsOriginOffset(float world_size)
{
  ShaderGlobal::set_float4(clouds_origin_offsetVarId, P2D(currentCloudsOffsetCalc(world_size)), 0, 0);
}

void CloudsRenderer::setCloudsOffsetVars(float current_clouds_offset, float world_size)
{
  ShaderGlobal::set_float(clouds_offsetVarId, current_clouds_offset);
  setCloudsOriginOffset(world_size);
}

void CloudsRenderer::renderCloudsPrepare(CloudsRendererData &data, BaseTexture *depth, BaseTexture *prev_depth,
  const Point2 &wind_change_ofs, float world_size, const TMatrix &view_tm, const TMatrix4 &proj_tm,
  const DPoint3 *world_pos /*= nullptr*/, const CloudsRenderFlags flags)
{
  if (!data.cloudsColorPoolRT)
    return;

  const bool isMainView = check_clouds_render_flag(flags, CloudsRenderFlags::MainView);
  const bool setCameraVars = check_clouds_render_flag(flags, CloudsRenderFlags::SetCameraVars);
  const bool restartTAA = check_clouds_render_flag(flags, CloudsRenderFlags::RestartTAA);

  TIME_D3D_PROFILE(render_clouds);
  ScopeReprojection reprojectionScope;
  data.setVars(isMainView);
  if (setCameraVars)
    set_viewvecs_to_shader(view_tm, proj_tm);
  ShaderGlobal::set_float(clouds_restart_taaVarId, data.frameValid && !restartTAA ? 0 : 2);
  ShaderGlobal::set_int(clouds_use_fullresVarId, (data.cloudResolution == CloudsResolution::ForceFullresClouds));

  if (isMainView)
  {
    data.clearTemporalData(get_d3d_full_reset_counter());
    data.frameValid = true;
    data.frameNo++;

    data.nextCloudsColor = nullptr;
    data.prevCloudsColor = nullptr;
    data.prevCloudsWeight = nullptr;
    data.cloudsBlurTextureColor = nullptr;
  }

  const bool isInfinite = !depth;
  const bool changedInfinite = isInfinite != data.lastFrameInfiniteSkies;
  data.lastFrameInfiniteSkies = isInfinite;
  STATE_GUARD_0(ShaderGlobal::set_int(clouds_invalidate_taa_framesVarId, VALUE), changedInfinite);

  ShaderGlobal::set_int(clouds2_current_frameVarId, data.frameNo & 0xFFFF);
  // one inverse per prepare: the clamp collapse and the reprojection both need the
  // camera world position
  const DPoint3 camWorldPos = world_pos ? *world_pos : dpoint3(orthonormalized_inverse(view_tm).getcol(3));
  // the deltas below are main-view temporal inputs: a sub view (camera-in-camera)
  // sharing this data would clobber them with its own timing
  if (isMainView)
  {
    // the checker accumulation weight is calibrated at 60Hz: dt-true (w ~ dt) above
    // 60fps keeps the wall-clock time constant; below 60 each rippled state persists
    // longer on screen, so the tolerable weight falls instead (w ~ dt^-1/3)
    unsigned nowMs = get_time_msec();
    float dtSec = data.lastPrepareMs ? (nowMs - data.lastPrepareMs) * 0.001f : 0.f;
    float dtScale = data.lastPrepareMs ? clamp(dtSec * 60.f, 0.25f, 4.f) : 1.f;
    data.lastPrepareMs = nowMs;
    ShaderGlobal::set_float(clouds_checker_dt_scaleVarId, dtScale <= 1.f ? dtScale : powf(dtScale, -1.f / 3.f));

    // ghosting exists only where parallax does: the clamp collapse follows the
    // PARALLAX RATE - lateral-weighted camera speed over the distance to the
    // nearest potential ray intersection (the cloud layer, this or previous
    // frame). Speed scaling makes the onset distance grow with speed by itself.
    // Rises instantly, decays over ~1/6 s of wall clock so dt noise cannot flicker
    // the box
    {
      DPoint3 vel = dtSec > 1e-5f ? (camWorldPos - data.prevCamPosForClamp) * (1.0 / dtSec) : DPoint3(0, 0, 0);
      data.prevCamPosForClamp = camWorldPos;
      float layerBottom = ShaderGlobal::get_float(clouds_start_altitude2VarId) * 1000.f;
      float layerTop = layerBottom + ShaderGlobal::get_float(clouds_thickness2VarId) * 1000.f;
      float distOut = max(max(layerBottom - (float)camWorldPos.y, (float)camWorldPos.y - layerTop), 0.f);
      // ghosts need parallax that existed recently or can exist shortly: take the
      // worst distance of the previous frame, this frame, and a pessimistic future
      // (same speed magnitude, turned straight at the layer) over the time ghosts
      // take to develop - roughly the accumulation window
      float speedMag = (float)sqrt(lengthSq(vel));
      // wind is a virtual camera translation at the layer distance: the origin motion
      // mis-reprojects through any depth error, and the erosion scroll moves content
      // relative to history at any taa_erosion_advect - both feed the ghost potential.
      // Lateral only, so it stays out of the distFuture approach estimate
      float windSpeed = 0.f;
      if (dtSec > 1e-5f)
        windSpeed = (float)sqrt(sqr(cloudsOfs.x - currentCloudsOfs.x + wind_change_ofs.x) +
                                sqr(cloudsOfs.y - currentCloudsOfs.y + wind_change_ofs.y)) /
                    dtSec;
      float distFuture = max(distOut - speedMag * 0.5f, 0.f);
      float dSafe = max(min(min(distOut, data.prevLayerDist), distFuture), 100.f);
      data.prevLayerDist = distOut;
      float collapse = clamp((speedMag + windSpeed) / (dSafe * max(checker_clamp_rate.get(), 1e-3f)), 0.f, 1.f);
      data.clampCollapseSmooth = max(collapse, lerp(data.clampCollapseSmooth, collapse, 1.f - powf(0.9f, dtSec * 60.f)));
    }
    ShaderGlobal::set_float(clouds_checker_clamp_collapseVarId, data.clampCollapseSmooth);
  }
  setCloudsOffsetVars(data, world_size);
  const bool canBeInsideClouds = bool(data.clouds_color_close) && depth;
  data.closeLayerWasActive = canBeInsideClouds; // the apply must agree with what was traced, not with texture existence

  ShaderGlobal::set_int(clouds_infinite_skiesVarId, isInfinite ? 1 : 0);
  ShaderGlobal::set_int(clouds_has_close_sequenceVarId, canBeInsideClouds ? 1 : 0);

  auto &cloudsCloseLayerIsOutside = isMainView ? data.clouds_close_layer_is_outside : data.clouds_sub_view_close_layer_is_outside;
  if (cloudsCloseLayerIsOutside)
  {
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), cloudsCloseLayerIsOutside.getBuf());
    if (canBeInsideClouds)
    {
      TIME_D3D_PROFILE(clouds_close_layer_is_outside_calc);
      TMatrix4 globtm;
      d3d::calcglobtm(view_tm, proj_tm, globtm);
      extern void set_frustum_planes(const Frustum &frustum);
      set_frustum_planes(Frustum(globtm));
      clouds_close_layer_outside->dispatch(1, 1, 1);
    }
    else
    {
      TIME_D3D_PROFILE(clouds_close_layer_clear);
      clouds_close_layer_clear->dispatch(1, 1, 1);
    }
  }
  bool useCompute = data.useCompute, taaUseCompute = data.taaUseCompute;
#if DAGOR_DBGLEVEL > 0
  useCompute &= use_compute.get();
  taaUseCompute &= use_compute.get();
#endif

  eastl::optional<ScopeRenderTarget> rtScope;
  if ((!useCompute || !taaUseCompute) && isMainView)
    rtScope.emplace();

  const bool checkerCompiled = clouds_checkerboard_compiled();

  if (isMainView)
  {
    data.prevCloudsColor = eastl::move(data.cloudsTextureColor);
    data.cloudsTextureColor = data.cloudsColorPoolRT->acquire();

    data.prevCloudsWeight = eastl::move(data.cloudsTextureWeight);
    data.cloudsTextureWeight = data.cloudsWeightPoolRT->acquire();

    // checkerboard trace: packed quarter-res target - every packed texel is traced
    // every frame (the full-res position it maps to walks the quincunx), so the
    // target never holds garbage and no fill frame exists. Configs that assume the
    // interval off (WTM) ship no checker shader variants, the flag must stay inert
    data.ensureCheckerColor(useCompute && traceCheckerboard && checkerCompiled);
    // the full-res target serves only the classic trace and the blur ping: do not
    // lease it under the packed checker trace (with checkerboard default the blur
    // pool then never even allocates)
    if (!data.cloudsCheckerColor || data.useBlurredClouds)
      data.nextCloudsColor = data.cloudsColorBlurPoolRT->acquire();
    else
      data.nextCloudsColor = nullptr;
  }
  // mode is per-data, not per-render: a sub-view prepare (camera-in-camera lens)
  // must keep the main view's packed target and age/phase weight encoding, or its
  // classic TAA would overwrite checker history with scalar weights
  const bool checkerActive = bool(data.cloudsCheckerColor);
  G_ASSERT(checkerActive || data.nextCloudsColor != nullptr);
  G_ASSERT(data.prevCloudsColor != nullptr);
  G_ASSERT(data.prevCloudsWeight != nullptr);
  BaseTexture *traceCloudsColor = checkerActive ? data.cloudsCheckerColor.getTex2D() : data.nextCloudsColor->getTex2D();
  if (checkerCompiled)
    ShaderGlobal::set_int(clouds_checkerboardVarId, checkerActive ? 1 : 0);
  // the trace target size: consumers must not derive it from the clouds res (the
  // packed checker target is quarter size; classic equals the clouds res)
  ShaderGlobal::set_int4(traced_clouds_resVarId, checkerActive ? (data.cloudTexRes.x + 1) / 2 : data.cloudTexRes.x,
    checkerActive ? (data.cloudTexRes.y + 1) / 2 : data.cloudTexRes.y, 0, 0);

  if (useCompute)
  {
    // one transition batch for this frame's compute targets and history: everything here
    // was last used a frame ago, so this sync point is free and the binds below need none
    BaseTexture *texs[8];
    ResourceBarrier rbs[8];
    unsigned zeros[8] = {};
    unsigned cnt = 0;
    auto add = [&](BaseTexture *t, ResourceBarrier rb) {
      texs[cnt] = t;
      rbs[cnt] = rb;
      cnt++;
    };
    if (data.nextCloudsColor)
      add(data.nextCloudsColor->getTex2D(), RB_RW_UAV | RB_STAGE_COMPUTE);
    if (!data.nextCloudsColor || traceCloudsColor != data.nextCloudsColor->getTex2D())
      add(traceCloudsColor, RB_RW_UAV | RB_STAGE_COMPUTE);
    add(data.cloudsTextureDepth.getTex2D(), RB_RW_UAV | RB_STAGE_COMPUTE);
    if (canBeInsideClouds)
      add(data.clouds_color_close.getTex2D(), RB_RW_UAV | RB_STAGE_COMPUTE);
    if (taaUseCompute)
    {
      add(data.cloudsTextureColor->getTex2D(), RB_RW_UAV | RB_STAGE_COMPUTE);
      add(data.cloudsTextureWeight->getTex2D(), RB_RW_UAV | RB_STAGE_COMPUTE);
      add(data.prevCloudsColor->getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE);
      add(data.prevCloudsWeight->getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE);
    }
    d3d::resource_barrier(ResourceBarrierDesc(texs, rbs, zeros, zeros, cnt));
  }

  renderTiledDist(data);
  int depthLevels = 1;
  int level = getNotLesserDepthLevel(data, depthLevels, depth);

  // the TAA depth texel math needs each texture's really sampled mip size: under
  // dynamic resolution the depth pair follows the current scale while the clouds
  // res stays at the stable maximum, and current vs prev differ on a step frame
  if (depth)
  {
    TextureInfo dti;
    depth->getinfo(dti, level);
    IPoint2 depthDims(dti.w, dti.h), prevDepthDims = depthDims;
    if (prev_depth)
    {
      prev_depth->getinfo(dti, level);
      prevDepthDims = IPoint2(dti.w, dti.h);
    }
    ShaderGlobal::set_float4(clouds_depth_gbuf_dimsVarId, depthDims.x, depthDims.y, 1.f / depthDims.x, 1.f / depthDims.y);
    ShaderGlobal::set_float4(clouds_prev_depth_gbuf_dimsVarId, prevDepthDims.x, prevDepthDims.y, 1.f / prevDepthDims.x,
      1.f / prevDepthDims.y);
  }
  else
  {
    // no scene depth bound: zeros make the shader derive the legacy clouds-based
    // unit itself (halved under fullres clouds), exactly the pre-change fallback
    ShaderGlobal::set_float4(clouds_depth_gbuf_dimsVarId, 0, 0, 0, 0);
    ShaderGlobal::set_float4(clouds_prev_depth_gbuf_dimsVarId, 0, 0, 0, 0);
  }

  int w = data.cloudTexRes.x;
  int h = data.cloudTexRes.y;

  ShaderGlobal::set_texture_unsafe(clouds_depth_gbufVarId, depth);
  DispatchGroups2D dg = set_dispatch_groups(w, h, CLOUD_TRACE_WARP_X, CLOUD_TRACE_WARP_Y, data.lowresCloseClouds);
  if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
  {
    // runs before the far trace (it only reads the tile counters) so that consuming
    // its args later does not force a sync against the trace
    TIME_D3D_PROFILE(create_indirect);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), data.cloudsIndirectBuffer.getBuf());
    clouds_create_indirect->dispatch(1, 1, 1);
  }

  {
    if (level != 0 && depthLevels != 1 && depth)
      depth->texmiplevel(level, level);
    TIME_D3D_PROFILE(current_clouds);
    // when blur is used edges of close object tend to be really exaggerated
    // so we ignoring them is a good idea
    STATE_GUARD(ShaderGlobal::set_int(clouds_ignore_close_objectsVarId, VALUE), data.useBlurredClouds, false);
    if (useCompute)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), traceCloudsColor);
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), data.cloudsTextureDepth.getTex2D());
      const int gx = checkerActive ? ((w + 1) / 2 + CLOUD_TRACE_WARP_X - 1) / CLOUD_TRACE_WARP_X : dg.x;
      const int gy = checkerActive ? ((h + 1) / 2 + CLOUD_TRACE_WARP_Y - 1) / CLOUD_TRACE_WARP_Y : dg.y;
      clouds2_temporal_cs->dispatch(gx, gy, 1); // todo: measure optimal warp size
    }
    else
    {
      d3d::set_render_target({}, DepthAccess::RW,
        {{data.nextCloudsColor->getTex2D(), 0, 0}, {data.cloudsTextureDepth.getTex2D(), 0, 0}});
      clouds2_temporal_ps.render();
    }
  }

  if (useCompute)
  {
    // TAA depends only on the far trace; transitioning its inputs right after the
    // producer lets the close layer trace below and the TAA overlap on the GPU,
    // instead of the TAA binds draining the whole pipe
    ResourceBarrier taaStage = taaUseCompute ? RB_STAGE_COMPUTE : RB_STAGE_PIXEL;
    BaseTexture *texs[] = {traceCloudsColor, data.cloudsTextureDepth.getTex2D()};
    ResourceBarrier rbs[] = {RB_RO_SRV | taaStage, RB_RO_SRV | RB_STAGE_PIXEL | taaStage};
    unsigned zeros[] = {0, 0};
    d3d::resource_barrier(ResourceBarrierDesc(texs, rbs, zeros, zeros, 2));
    if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
      d3d::resource_barrier({data.cloudsIndirectBuffer.getBuf(), RB_RO_INDIRECT_BUFFER});
  }
  if (canBeInsideClouds)
  {
    // todo: use indirect
    if (depthLevels != 1 && level + 1 < depthLevels && data.lowresCloseClouds)
      depth->texmiplevel(level + 1, level + 1);
    TIME_D3D_PROFILE(current_clouds_near);
    if (useCompute)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), data.clouds_color_close.getTex2D());
      if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
      {
        clouds2_close_temporal_cs->dispatch_indirect(data.cloudsIndirectBuffer.getBuf(),
          4 * 4 * (CLOUDS_HAS_CLOSE_LAYER + CLOUDS_APPLY_COUNT_PS));
      }
      else
      {
        clouds2_close_temporal_cs->dispatch(dg.z, dg.w, 1);
      }
    }
    else
    {
      d3d::set_render_target({}, DepthAccess::RW, {{data.clouds_color_close.getTex2D(), 0, 0}});
      if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
      {
        clouds2_close_temporal_ps.getElem()->setStates();
        d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_HAS_CLOSE_LAYER);
      }
      else
        clouds2_close_temporal_ps.render();
    }
    // no barrier here: only the apply reads this texture, and it is transitioned there;
    // a barrier at this spot would flush at the TAA binds and serialize close vs TAA
  }

  {
    if (depthLevels != 1)
    {
      if (depth)
        depth->texmiplevel(level, level);
      if (prev_depth)
      {
        G_ASSERT(prev_depth->level_count() == depth->level_count());
        prev_depth->texmiplevel(level, level);
      }
    }
    TIME_D3D_PROFILE(taa_clouds);
    ShaderGlobal::set_texture_unsafe(clouds_prev_depth_gbufVarId, prev_depth);
    data.prevWorldPos.x -= cloudsOfs.x + wind_change_ofs.x * taa_erosion_advect.get() - currentCloudsOfs.x;
    data.prevWorldPos.z -= cloudsOfs.y + wind_change_ofs.y * taa_erosion_advect.get() - currentCloudsOfs.y;
    if (setCameraVars)
      set_reprojection(view_tm, proj_tm, data.prevProjTm, data.prevWorldPos, data.prevGlobTm, data.prevViewVecLT, data.prevViewVecRT,
        data.prevViewVecLB, data.prevViewVecRB, &camWorldPos);
    ShaderGlobal::set_texture(clouds_color_prevVarId, data.prevCloudsColor->getTexId());
    if (checkerActive)
      ShaderGlobal::set_texture(clouds_colorVarId, data.cloudsCheckerColor);
    else
      ShaderGlobal::set_texture(clouds_colorVarId, *data.nextCloudsColor);
    ShaderGlobal::set_texture(clouds_prev_taa_weightVarId, data.prevCloudsWeight->getTexId());
    if (taaUseCompute)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), data.cloudsTextureColor->getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), data.cloudsTextureWeight->getTex2D());
      if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
      {
        clouds2_taa_cs_has_empty->dispatch_indirect(data.cloudsIndirectBuffer.getBuf(),
          4 * 4 * (CLOUDS_HAS_EMPTY + CLOUDS_APPLY_COUNT_PS));
        d3d::resource_barrier({data.cloudsTextureColor->getTex2D(), RB_NONE, 0, 0});
        d3d::resource_barrier({data.cloudsTextureWeight->getTex2D(), RB_NONE, 0, 0});
        clouds2_taa_cs_no_empty->dispatch_indirect(data.cloudsIndirectBuffer.getBuf(),
          4 * 4 * (CLOUDS_NO_EMPTY + CLOUDS_APPLY_COUNT_PS));
      }
      else
        clouds2_taa_cs->dispatch(dg.x, dg.y, 1); // todo: measure optimal warp size
    }
    else
    {
      d3d::set_render_target({}, DepthAccess::RW,
        {{data.cloudsTextureColor->getTex2D(), 0, 0}, {data.cloudsTextureWeight->getTex2D(), 0, 0}});
      d3d::setview(0, 0, w, h, 0, 1);
      d3d::setscissor(0, 0, w, h);
      d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0, 0);
      if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
      {
        clouds2_taa_ps_has_empty.getElem()->setStates();
        d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_HAS_EMPTY);
        set_program(clouds2_taa_ps_has_empty.getElem(), clouds2_taa_ps_no_empty.getElem());
        // clouds2_taa_ps_no_empty.getElem()->setStates();
        d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_NO_EMPTY);
      }
      else
        clouds2_taa_ps.render();

      d3d::resource_barrier({data.cloudsTextureColor->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      // the weight is read by the next frame's taa; make the read state explicit
      d3d::resource_barrier({data.cloudsTextureWeight->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
    if (depthLevels != 1)
    {
      if (depth)
        depth->texmiplevel(-1, -1);
      if (prev_depth)
        prev_depth->texmiplevel(-1, -1);
    }
  }

  if (data.useBlurredClouds)
  {
    ShaderGlobal::set_texture(clouds_colorVarId, *data.cloudsTextureColor);
    // reads need explicit transitions; the blur depends on the TAA anyway, so this
    // sync point costs no overlap
    d3d::resource_barrier({data.cloudsTextureColor->getTex2D(), RB_RO_SRV | (useCompute ? RB_STAGE_COMPUTE : RB_STAGE_PIXEL), 0, 0});
    if (useCompute)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), data.nextCloudsColor->getTex2D());
      clouds2_apply_blur_cs->dispatch(dg.x, dg.y, 1);
      d3d::resource_barrier({data.nextCloudsColor->getTex2D(), RB_RW_UAV | RB_STAGE_COMPUTE, 0, 0});
    }
    else
    {
      d3d::set_render_target({}, DepthAccess::RW, {{data.nextCloudsColor->getTex2D(), 0, 0}});
      clouds2_apply_blur_ps.render();
      d3d::resource_barrier({data.nextCloudsColor->getTex2D(), RB_RW_RENDER_TARGET | RB_STAGE_PIXEL, 0, 0});
    }
    ShaderGlobal::set_texture(clouds_colorVarId, *data.nextCloudsColor);
    G_ASSERT(data.cloudsBlurTextureColor == nullptr || !isMainView);
    data.cloudsBlurTextureColor = data.nextCloudsColor;
  }

  currentCloudsOfs = cloudsOfs;

  ShaderGlobal::set_texture_unsafe(clouds_depth_gbufVarId, nullptr);
  ShaderGlobal::set_texture_unsafe(clouds_prev_depth_gbufVarId, nullptr);
}

void CloudsRenderer::renderDirect(CloudsRendererData &data)
{
  TIME_D3D_PROFILE(render_clouds_direct);
  G_UNUSED(data);
  if (d3d::get_driver_desc().caps.hasTileBasedArchitecture && VariableMap::isVariablePresent(clouds_direct_sequenceVarId) &&
      !ShaderGlobal::is_var_assumed(clouds_direct_sequenceVarId))
  {
    ShaderGlobal::set_int(clouds_direct_sequenceVarId, 1);
    clouds2_direct.render();
    ShaderGlobal::set_int(clouds_direct_sequenceVarId, 2);
    clouds2_direct.render();
    ShaderGlobal::set_int(clouds_direct_sequenceVarId, 3);
    clouds2_direct.render();
    ShaderGlobal::set_int(clouds_direct_sequenceVarId, 0);
  }
  else
    clouds2_direct.render();
}
/*static*/ void CloudsRenderer::set_program(ShaderElement *oe, ShaderElement *ne)
{
  uint32_t program;
  ShaderStateBlockId state_index;
  shaders::RenderStateId rstate;
  shaders::ConstStateIdx cstate;
  shaders::TexStateIdx tstate;
  int curVariant = get_dynamic_variant_states(ne->native(), program, state_index, rstate, cstate, tstate);
  uint32_t program2;
  ShaderStateBlockId state_index2;
  shaders::RenderStateId rstate2;
  shaders::ConstStateIdx cstate2;
  shaders::TexStateIdx tstate2;
  int curVariant2 = get_dynamic_variant_states(oe->native(), program2, state_index2, rstate2, cstate2, tstate2);
  if (curVariant2 != curVariant || rstate2 != rstate || state_index2 != state_index)
    ne->setStates();
  else
    d3d::set_program(program);
}

void CloudsRenderer::renderCloudsApply(CloudsRendererData &data, BaseTexture *downsampled_depth, BaseTexture *target_depth,
  const Point4 &target_depth_transform, const TMatrix &view_tm, const TMatrix4 &proj_tm, const CloudsRenderFlags flags)
{
  TIME_D3D_PROFILE(render_clouds);
  set_viewvecs_to_shader(view_tm, proj_tm);
  ShaderGlobal::set_int(clouds_use_fullresVarId, (data.cloudResolution == CloudsResolution::ForceFullresClouds));
  ShaderGlobal::set_texture_unsafe(clouds_target_depth_gbufVarId, target_depth);
  ShaderGlobal::set_float4(clouds_target_depth_gbuf_transformVarId, target_depth_transform);
  ShaderGlobal::set_int(clouds_has_close_sequenceVarId, (data.closeLayerWasActive && data.clouds_color_close.getTex2D()) ? 1 : 0);
  ShaderGlobal::set_texture_unsafe(clouds_depth_gbufVarId, downsampled_depth);
  if (!data.cloudsColorPoolRT)
  {
    renderDirect(data);
    ShaderGlobal::set_texture(clouds_target_depth_gbufVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(clouds_depth_gbufVarId, BAD_TEXTUREID);
    return;
  }
  TIME_D3D_PROFILE(apply_bilateral);
  const bool isMainView = check_clouds_render_flag(flags, CloudsRenderFlags::MainView);
  data.setVars(isMainView);
  // close layer transition deferred to here (the only consumer), batching with the taa
  // output transitions into a single sync point right before the apply
  if (data.closeLayerWasActive && data.clouds_color_close.getTex2D())
    d3d::resource_barrier({data.clouds_color_close.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  ShaderGlobal::set_int(clouds_use_blur_applyVarId, data.useBlurredClouds);
  auto &appliedColor = data.useBlurredClouds ? *data.cloudsBlurTextureColor : *data.cloudsTextureColor;
  ShaderGlobal::set_texture(clouds_colorVarId, appliedColor);
  // compute written clouds color becomes readable here, at its consumer, so the work
  // between prepare and apply keeps overlapping the taa dispatches
  d3d::resource_barrier({appliedColor.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  int depthLevels;
  int level = getNotLesserDepthLevel(data, depthLevels, downsampled_depth);
  if (level != 0)
    downsampled_depth->texmiplevel(level, level);
  if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
  {
    clouds2_apply_has_empty.getElem()->setStates();
    d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_HAS_EMPTY);
    set_program(clouds2_apply_has_empty.getElem(), clouds2_apply_no_empty.getElem());
    // clouds2_apply_no_close_no_empty.getElem()->setStates();
    d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_NO_EMPTY);
  }
  else
  {
    clouds2_apply.render();
  }

  if (data.useBlurredClouds)
    data.cloudsBlurTextureColor = nullptr;

  if (level != 0)
    downsampled_depth->texmiplevel(-1, -1);

  ShaderGlobal::set_texture(clouds_target_depth_gbufVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(clouds_depth_gbufVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(clouds_colorVarId, BAD_TEXTUREID);
}
