// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cloudsShadows.h"
#include "cloudsShaderVars.h"

#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_viewScissor.h>

#include <util/dag_convar.h>
#include <math/dag_adjpow2.h>
#include <workCycle/dag_workCycle.h>
#include <osApiWrappers/dag_miscApi.h>

CONSOLE_BOOL_VAL("clouds", regenShadows, false);
CONSOLE_BOOL_VAL("clouds", use_bsm, true); // Beer shadow map: the base receiver shadow, replaces the legacy 2D map
// sun rotation (deg) that abandons convergence for an instant re-render; the linear
// blend hides even large swings, so only true set-jumps qualify; 180 = never
CONSOLE_FLOAT_VAL_MINMAX("clouds", bsm_sun_jump_deg, 15.f, 0.f, 180.f);

// footprint per Hillaire (SIGGRAPH 2020): one 300km window around the camera (slid
// up-sun at low sun); smoothness comes from the curve reconstruction + gen-time
// filtering, not texel density
static constexpr float BSM_SPAN = 300000.f;

// beyond a quarter span the blend would converge toward a mostly-new window: pop once instead
static bool bsm_out_of_window(const Point2 &desired_origin, const Point2 &live_origin)
{
  return lengthSq(desired_origin - live_origin) > (BSM_SPAN / 4.f) * (BSM_SPAN / 4.f);
}

void CloudsShadows::initTemporal()
{
  copy_cloud_shadows_volume.init("copy_cloud_shadows_volume", "copy_cloud_shadows_volume_ps");
  gen_cloud_shadows_volume_partial.init("gen_cloud_shadows_volume_partial_cs", "gen_cloud_shadows_volume_partial_ps");
  cloudsShadowsTemp.close();
  // the copy pass packs two X-adjacent voxels per texel, so X is half the block width
  copy_cloud_shadows_volume.initVoltex(cloudsShadowsTemp, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD / 2, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD, TEXFMT_R32UI, 1, "cloud_shadows_old_values");

  ShaderGlobal::set_float4(clouds_compute_widthVarId, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD, 0);
  resetGen = 0;
}

void CloudsShadows::updateTemporalStep(int x, int y, int z)
{
  ShaderGlobal::set_float4(clouds_start_compute_offsetVarId, x, y, z, 0);
  {
    TIME_D3D_PROFILE(cloud_shadows_vol_copy);
    copy_cloud_shadows_volume.render(cloudsShadowsTemp);
    d3d::resource_barrier({cloudsShadowsTemp.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }
  {
    TIME_D3D_PROFILE(cloud_shadows_vol_calc);
    gen_cloud_shadows_volume_partial.render(cloudsShadowsVol, 0,
      IPoint3(CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD, CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD), IPoint3(x, y, z));
    d3d::resource_barrier({cloudsShadowsVol.getVolTex(), RB_RO_SRV | RB_STAGE_ALL_GRAPHICS | RB_STAGE_COMPUTE, 0, 0});
    // generateMips2d(cloud1);
  }
}

bool CloudsShadows::updateTemporal()
{
  bool changed = false;
  if (temporalStep < temporalStepFinal)
  {
    TIME_D3D_PROFILE(cloud_shadows_vol_gradual);
    const uint32_t lerpStep = (temporalStepFinal - temporalStep - 1) / TEMPORAL_STEPS;
    float effect = 1.f - float(lerpStep) / (lerpStep + 1.f);
    ShaderGlobal::set_float4(clouds_new_shadow_ambient_weightVarId, effect, updateAmbient ? effect : 0.f, 0, 0);
    int xs = (temporalStep % CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS),
        ys = (temporalStep / CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS) % CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS,
        zs = (temporalStep / CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS / CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS) %
             CLOUDS_MAX_SHADOW_TEMPORAL_Y_STEPS;
    updateTemporalStep(xs * CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD, ys * CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD,
      zs * CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD);
    if (++temporalStep != temporalStepFinal)
      return true;
    changed = true;
  }
  else if (temporalStep == TEMPORAL_STEP_FORCED)
  {
    TIME_D3D_PROFILE(cloud_shadows_vol_all);
    ShaderGlobal::set_float4(clouds_new_shadow_ambient_weightVarId, 1, updateAmbient ? 1 : 0, 0, 0);
    for (int z = 0; z < CLOUD_SHADOWS_VOLUME_RES_Y; z += CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD)
      for (int y = 0; y < CLOUD_SHADOWS_VOLUME_RES_XZ; y += CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD)
        for (int x = 0; x < CLOUD_SHADOWS_VOLUME_RES_XZ; x += CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD)
          updateTemporalStep(x, y, z);
    changed = true;
  }
  validate(); // we re finally finished
  return changed;
}

void CloudsShadows::init()
{
  initTemporal();
  genCloudShadowsVolume.init("gen_cloud_shadows_volume_cs", "gen_cloud_shadows_volume_ps");
  int fmt = (VoltexRenderer::is_compute_supported() && d3d::get_driver_desc().issues.hasBrokenComputeFormattedOutput) ? TEXFMT_G32R32F
                                                                                                                      : TEXFMT_R8G8;
  // NOTE: this texture may be used while skies are not yet fully prepared
  genCloudShadowsVolume.initVoltex(cloudsShadowsVol, CLOUD_SHADOWS_VOLUME_RES_XZ, CLOUD_SHADOWS_VOLUME_RES_XZ,
    CLOUD_SHADOWS_VOLUME_RES_Y, fmt | TEXCF_CLEAR_ON_CREATE, 1, "clouds_shadows_volume");
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = d3d::AddressMode::Wrap;
    smpInfo.address_mode_v = d3d::AddressMode::Wrap;
    smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(::get_shader_variable_id("clouds_shadows_volume_samplerstate"), d3d::request_sampler(smpInfo));
  }
}

void CloudsShadows::validate()
{
  temporalStep = temporalStepFinal = 0;
  updateAmbient = false;
}

bool CloudsShadows::isConverged() const
{
  if (temporalStep != temporalStepFinal || resetGen == 0)
    return false;
  if (!use_bsm.get())
    return true;
  return bsmLiveValid && !bsmNeedFull && bsmCycleFrame == 0 && !bsmPendingValid;
}
void CloudsShadows::addRecalcAll()
{
  // the remaining window must cover every temporal block at least once starting from
  // the current step, otherwise a mid-cycle light change leaves stale blocks behind
  temporalStep %= ALL_TEMPORAL_STEPS_LERP; // folding keeps counters small and preserves block phase
  temporalStepFinal = temporalStep + ALL_TEMPORAL_STEPS_LERP;
}

void CloudsShadows::forceFullUpdate(const Point3 &main_light_dir, bool update_ambient)
{
  lastLightDir = main_light_dir;
  updateAmbient |= update_ambient;
  addRecalcAll();
}

CloudsChangeFlags CloudsShadows::render(const Point3 &main_light_dir, const Point2 &camera_xz) // todo: use main_light_dir for next
                                                                                               // temporality
{
  const uint32_t cresetGen = get_d3d_full_reset_counter();
  if (cresetGen != resetGen)
    bsmNeedFull = true; // device reset loses the map content
  updateBSM(camera_xz, main_light_dir);
  if (!are_approximately_equal(lastLightDir, main_light_dir))
    forceFullUpdate(main_light_dir, false);
  if (cresetGen == resetGen) // and sun light changed
  {
    if (regenShadows.get())
      temporalStep = temporalStepFinal = TEMPORAL_STEP_FORCED;
    CloudsChangeFlags ret = CLOUDS_NO_CHANGE;
    if (updateTemporal())
      ret = CLOUDS_INCREMENTAL;
    return ret;
  }
  {
    TIME_D3D_PROFILE(cloud_shadows_vol);

    bool splitWorkload = d3d::get_driver_desc().issues.hasComputeTimeLimited != 0 && genCloudShadowsVolume.isComputeLoaded();
    if (splitWorkload)
    {
      TextureInfo tinfo;
      cloudsShadowsVol->getinfo(tinfo);
      const int splits = 8;

      int splitStep = tinfo.w / splits;
      G_ASSERT((splitStep / CLOUD_SHADOWS_WARP_SIZE_XZ >= 1) && (splitStep % CLOUD_SHADOWS_WARP_SIZE_XZ == 0));
      for (int i = 0; i < splits; ++i)
      {
        if (is_main_thread())
          // this code runs on main thread in blocking manner, avoid CloudsShadows::ANR
          ::dagor_process_sys_messages();
        else
        {
          // even if we run on non main thread, GPU ownership will block it,
          // release and reacquire ownership after frame is rendered in supposed to be "loading screen"
          int lockedFrame = dagor_frame_no();
          d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);
          spin_wait([lockedFrame]() { return lockedFrame >= dagor_frame_no(); });
          d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
        }

        d3d::GPUWorkloadSplit gpuWorkSplit(true /*do_split*/, true /*split_at_end*/, String(32, "cloud_shadows_vol_split%u", i));
        ShaderGlobal::set_float4(clouds_start_compute_offsetVarId, splitStep * i, 0, 0, 0);
        genCloudShadowsVolume.render(cloudsShadowsVol, 0, {splitStep, -1, -1});
      }
      ShaderGlobal::set_float4(clouds_start_compute_offsetVarId, 0, 0, 0, 0);
    }
    else
    {
      ShaderGlobal::set_float4(clouds_start_compute_offsetVarId, 0, 0, 0, 0);
      genCloudShadowsVolume.render(cloudsShadowsVol);
    }

    d3d::resource_barrier({cloudsShadowsVol.getVolTex(), RB_RO_SRV | RB_STAGE_ALL_GRAPHICS | RB_STAGE_COMPUTE, 0, 0});
    validate();
    // generateMips2d(cloud1);
  }
  resetGen = cresetGen;
  return CLOUDS_INVALIDATED;
}

void CloudsShadows::initBSM()
{
  if (cloudsBSM[0])
    return;
  // shaders survive a use_bsm off/on cycle: PostFxRenderer double-init is not safe
  if (!gen_clouds_bsm_cs && !gen_clouds_bsm_ps.getElem())
  {
    if (VoltexRenderer::is_compute_supported())
      gen_clouds_bsm_cs.reset(new_compute_shader("gen_clouds_bsm", true));
    if (!gen_clouds_bsm_cs)
      gen_clouds_bsm_ps.init("gen_clouds_bsm_ps", true);
    // all of them ship in clouds_bsm_gen.dshl: a project bindump without it has none.
    // Bail before creating textures - cloudsBSM[0] stays null, updateBSM early-outs
    // every frame and clouds_bsm stays unbound (the null texture reads zero, which
    // decodes to unshadowed), clouds_bsm_world.z stays 0 for the fog regime switch
    if (!gen_clouds_bsm_cs && !gen_clouds_bsm_ps.getElem())
      return;
    clouds_bsm_filter_ps.init("clouds_bsm_filter_ps");
    clouds_bsm_blend_ps.init("clouds_bsm_blend_ps");
  }
  d3d::SamplerInfo smpInfo;
  smpInfo.filter_mode = d3d::FilterMode::Linear;
  smpInfo.address_mode_u = smpInfo.address_mode_v = d3d::AddressMode::Clamp;
  // R11G11B10F is enough for the curve params (Hillaire, SIGGRAPH 2020, stores the
  // same three channels in it)
  for (int i = 0; i < 2; ++i)
  {
    cloudsBSM[i] = dag::create_tex(NULL, bsmResolution, bsmResolution, TEXCF_RTARGET | TEXFMT_R11G11B10F, 1,
      i ? "clouds_bsm_1" : "clouds_bsm_0", RESTAG_DASKIES2);
    cloudsBSMFilt[i] = dag::create_tex(NULL, bsmResolution, bsmResolution, TEXCF_RTARGET | TEXFMT_R11G11B10F, 1,
      i ? "clouds_bsm_filt_1" : "clouds_bsm_filt_0", RESTAG_DASKIES2);
  }
  cloudsBSMPub =
    dag::create_tex(NULL, bsmResolution, bsmResolution, TEXCF_RTARGET | TEXFMT_R11G11B10F, 1, "clouds_bsm", RESTAG_DASKIES2);
  if (gen_clouds_bsm_cs)
  {
    cloudsBSMRaw = dag::buffers::create_ua_sr_byte_address(bsmResolution * bsmResolution * 2, "clouds_bsm_raw");
    ShaderGlobal::set_buffer(clouds_bsm_rawVarId, cloudsBSMRaw.getBufId());
  }
  else
  {
    // no compute: the trace stores through an RT instead of a UAV, in the final map's
    // R11G11B10F (this path serves compat targets - bandwidth over raw depth precision)
    cloudsBSMRawTex =
      dag::create_tex(NULL, bsmResolution, bsmResolution, TEXCF_RTARGET | TEXFMT_R11G11B10F, 1, "clouds_bsm_raw_tex", RESTAG_DASKIES2);
    ShaderGlobal::set_texture(clouds_bsm_raw_texVarId, cloudsBSMRawTex.getTexId());
  }
  ShaderGlobal::set_int(clouds_bsm_gen_from_texVarId, gen_clouds_bsm_cs ? 0 : 1);
  ShaderGlobal::set_sampler(clouds_bsm_samplerstateVarId, d3d::request_sampler(smpInfo));
  ShaderGlobal::set_sampler(clouds_bsm_blend_hist_samplerstateVarId, d3d::request_sampler(smpInfo));
  // CPU-published texel size: consumer preshaders must stay plain register copies
  // (see clouds_bsm.dshl)
  ShaderGlobal::set_float4(clouds_bsm_texelVarId, 1.f / bsmResolution, 1.f / bsmResolution, 0, 0);
  bsmLiveValid = false;
  bsmPendingValid = false;
  bsmCycleFrame = 0;
}

void CloudsShadows::closeBSM()
{
  if (!cloudsBSM[0])
    return;
  // consumers must not keep binding the destroyed textures
  ShaderGlobal::set_texture(clouds_bsmVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(clouds_bsm_raw_texVarId, BAD_TEXTUREID);
  for (int i = 0; i < 2; ++i)
  {
    cloudsBSM[i].close();
    cloudsBSMFilt[i].close();
  }
  cloudsBSMPub.close();
  cloudsBSMRaw.close();
  cloudsBSMRawTex.close();
  // no flag to clear: the unbound map reads zero = unshadowed. Zeroed window
  // consts pin lookups to the in-window null fetch, so no stale out-of-window
  // statistical blend survives the close
  ShaderGlobal::set_float4(clouds_bsm_worldVarId, 0, 0, 0, 0);
  bsmLiveValid = false;
}

void CloudsShadows::setBSMLog2AmortizeFrames(int log2_frames)
{
  log2_frames = clamp(log2_frames, 0, 6); // 64 Bayer slots / rects max
  if (bsmLog2AmortizeFrames == log2_frames)
    return;
  bsmLog2AmortizeFrames = log2_frames;
  // a partially traced attempt indexes its slots by the old cadence: restart it
  bsmCycleFrame = 0;
}

Point2 CloudsShadows::computeBSMOrigin(const Point2 &camera_xz, const Point3 &light_dir) const
{
  // texels index sun rays by their layer-BOTTOM plane crossing: a ground receiver's
  // crossing sits at most bottom_alt/tan(elevation) up-sun, so a small slide keeps
  // ground-to-layer-bottom receivers centered at any sun elevation (the cap is a
  // safety net for high layer bottoms)
  const float botAltM = 1000.f * ShaderGlobal::get_float(clouds_start_altitude2VarId);
  Point2 sunOfs = Point2(light_dir.x, light_dir.z) * (0.5f * botAltM / max(0.02f, light_dir.y));
  const float maxOfs = BSM_SPAN * 0.3f;
  const float ofsLenSq = lengthSq(sunOfs);
  if (ofsLenSq > maxOfs * maxOfs)
    sunOfs *= maxOfs / sqrtf(ofsLenSq);
  const Point2 desired = camera_xz + sunOfs;
  const float texel = BSM_SPAN / bsmResolution;
  return Point2(floorf(desired.x / texel + 0.5f) * texel, floorf(desired.y / texel + 0.5f) * texel);
}

void CloudsShadows::setBSMGenVars(const BSMState &state)
{
  ShaderGlobal::set_float4(clouds_bsm_genVarId, state.origin.x, state.origin.y, BSM_SPAN, bsmResolution);
  ShaderGlobal::set_float4(clouds_bsm_gen_sunVarId, state.sun.x, state.sun.y, state.sun.z, 0.f);
  ShaderGlobal::set_float4(clouds_bsm_gen_ofsVarId, state.cloudsOfs.x, state.cloudsOfs.y, 0.f, 0.f);
  ShaderGlobal::set_float4(clouds_bsm_gen_holeVarId, state.holeOfs.x, state.holeOfs.y, 0.f, 0.f);
}

void CloudsShadows::bsmTraceFrame(int frame, int frames_num)
{
  TIME_D3D_PROFILE(clouds_bsm_trace);
  if (gen_clouds_bsm_cs)
  {
    const int perQuad = 64 / frames_num; // Bayer slots of every 8x8 quad this frame
    const Color4 genOfs = ShaderGlobal::get_float4(clouds_bsm_gen_ofsVarId);
    ShaderGlobal::set_float4(clouds_bsm_gen_ofsVarId, genOfs.r, genOfs.g, frame, perQuad);
    gen_clouds_bsm_cs->dispatchThreads((bsmResolution / 8) * (bsmResolution / 8) * perQuad, 1, 1);
    d3d::resource_barrier({cloudsBSMRaw.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    return;
  }
  // a pixel shader cannot scatter to Bayer slots: amortize in rects instead -
  // frame k of N renders one viewport rect of the raw RT with the same frozen state
  const int log2n = get_log2i(frames_num); // frames_num is pow2
  const int tilesX = 1 << ((log2n + 1) / 2), tilesY = 1 << (log2n / 2);
  const int w = bsmResolution / tilesX, h = bsmResolution / tilesY;
  SCOPE_RENDER_TARGET;
  d3d::set_render_target({}, DepthAccess::RW, {{cloudsBSMRawTex.getTex2D(), 0, 0}});
  d3d::setview((frame % tilesX) * w, (frame / tilesX) * h, w, h, 0, 1);
  gen_clouds_bsm_ps.render();
  d3d::resource_barrier({cloudsBSMRawTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}

void CloudsShadows::bsmFilterInto(const UniqueTex &target)
{
  TIME_D3D_PROFILE(clouds_bsm_filter);
  if (cloudsBSMRaw)
    d3d::resource_barrier({cloudsBSMRaw.getBuf(), RB_RO_SRV | RB_STAGE_PIXEL});
  SCOPE_RENDER_TARGET;
  d3d::set_render_target({}, DepthAccess::RW, {{target.getTex2D(), 0, 0}});
  clouds_bsm_filter_ps.render();
  d3d::resource_barrier({target.getTex2D(), RB_RO_SRV | RB_STAGE_ALL_GRAPHICS | RB_STAGE_COMPUTE, 0, 0});
}

void CloudsShadows::publishBSM(const BSMState &state, const UniqueTex &tex)
{
  // copy into THE published texture instead of republishing the ping-pong id:
  // consumers that sample sparsely over time (GI voxel relight) must see one
  // stable, never-mid-frame-mutating texture
  if (tex.getTex2D() != cloudsBSMPub.getTex2D())
  {
    d3d::stretch_rect(tex.getTex2D(), cloudsBSMPub.getTex2D());
    d3d::resource_barrier({cloudsBSMPub.getTex2D(), RB_RO_SRV | RB_STAGE_ALL_GRAPHICS | RB_STAGE_COMPUTE, 0, 0});
  }
  ShaderGlobal::set_texture(clouds_bsmVarId, cloudsBSMPub.getTexId());
  // lookup consts are CPU-computed: consumer preshaders stay plain register copies
  const float botAltM = 1000.f * ShaderGlobal::get_float(clouds_start_altitude2VarId);
  const float thicknessM = 1000.f * ShaderGlobal::get_float(clouds_thickness2VarId);
  const float invY = 1.f / max(0.02f, state.sun.y);
  ShaderGlobal::set_float4(clouds_bsm_worldVarId, state.origin.x, state.origin.y, 1.f / BSM_SPAN, thicknessM * invY);
  ShaderGlobal::set_float4(clouds_bsm_constsVarId, botAltM, state.sun.x * invY, state.sun.z * invY, invY);
  // out-of-window statistical fallback: shell radii and the coverage-mean extinction
  // in the map's own units (natural log, last-octave), calibrated like the godray
  // statistical estimate (clouds_shadow_coverage * 0.5)
  const float planetR = ShaderGlobal::get_float(::get_shader_variable_id("skies_planet_radius", true));
  const float rBot = planetR + botAltM * 0.001f, rTop = rBot + thicknessM * 0.001f;
  const float statSigma = ShaderGlobal::get_float(global_clouds_sigmaVarId) * ShaderGlobal::get_float(clouds_bsm_ms_attn_sqVarId) *
                          ShaderGlobal::get_float(clouds_shadow_coverageVarId) * 1000.f * 0.5f;
  ShaderGlobal::set_float4(clouds_bsm_statVarId, rBot * rBot, rTop * rTop, statSigma, planetR);
}

void CloudsShadows::renderBSMFull(const BSMState &state)
{
  TIME_D3D_PROFILE(clouds_bsm);
  setBSMGenVars(state);
  bsmTraceFrame(0, 1);
  bsmFilterInto(cloudsBSM[bsmLiveIdx]);
  // the live copy stays the blend history for the amortized cycles that follow
  publishBSM(state, cloudsBSM[bsmLiveIdx]);
}

bool CloudsShadows::bsmStatesDiffer(const BSMState &a, const BSMState &b) const
{
  const float halfTexel = 0.5f * BSM_SPAN / bsmResolution;
  return a.origin != b.origin || dot(a.sun, b.sun) < 0.9999996f || lengthSq(a.cloudsOfs - b.cloudsOfs) > halfTexel * halfTexel ||
         lengthSq(a.holeOfs - b.holeOfs) > halfTexel * halfTexel;
}

void CloudsShadows::ensureBSMWindow(const Point2 &camera_xz, const Point3 &light_dir)
{
  if (!cloudsBSM[0] || !bsmLiveValid || !bsm_out_of_window(computeBSMOrigin(camera_xz, light_dir), bsmLive.origin))
    return;
  updateBSM(camera_xz, light_dir); // the teleport branch re-renders in full this frame
}

void CloudsShadows::updateBSM(const Point2 &camera_xz, const Point3 &light_dir)
{
  if (!use_bsm.get())
  {
    closeBSM();
    return;
  }
  initBSM();
  if (!cloudsBSM[0])
    return;
  const int n = 1 << bsmLog2AmortizeFrames;
  const Color4 liveOfs = ShaderGlobal::get_float4(clouds_origin_offsetVarId);
  // the hole displaces the sampled field (gen_bsm adds it): a moved hole re-renders
  const Color4 holePos = ShaderGlobal::get_float4(clouds_hole_posVarId);
  BSMState desired;
  desired.sun = light_dir;
  desired.cloudsOfs = Point2(liveOfs.r, liveOfs.g);
  desired.holeOfs = Point2(holePos.r, holePos.g);
  desired.origin = computeBSMOrigin(camera_xz, light_dir);
  // window hysteresis: keep the live origin while the desired one is close
  if (bsmLiveValid && lengthSq(desired.origin - bsmLive.origin) <= (BSM_SPAN / 16.f) * (BSM_SPAN / 16.f))
    desired.origin = bsmLive.origin;
  const float halfTexel = 0.5f * BSM_SPAN / bsmResolution;
  const bool originChanged = !bsmLiveValid || desired.origin != bsmLive.origin;
  // static mode tolerates ~0.5 deg of sun drift (the old whole-bake cadence);
  // amortized picks up ~0.05 deg so continuous time-of-day converges smoothly
  const bool sunChanged = !bsmLiveValid || dot(desired.sun, bsmLive.sun) < (n == 1 ? 0.99996f : 0.9999996f);
  const bool ofsChanged = !bsmLiveValid || lengthSq(desired.cloudsOfs - bsmLive.cloudsOfs) > halfTexel * halfTexel ||
                          lengthSq(desired.holeOfs - bsmLive.holeOfs) > halfTexel * halfTexel;
  // majors: pop once instead of converging toward a wildly different state
  const bool teleport = bsmLiveValid && bsm_out_of_window(desired.origin, bsmLive.origin);
  const bool sunJump = bsmLiveValid && dot(desired.sun, bsmLive.sun) < cosf(bsm_sun_jump_deg.get() * DEG_TO_RAD);
  if (bsmNeedFull || !bsmLiveValid || teleport || sunJump || (n == 1 && (originChanged || sunChanged || ofsChanged)))
  {
    renderBSMFull(desired);
    bsmLive = desired;
    bsmLiveValid = true;
    bsmNeedFull = false;
    bsmPendingValid = false;
    bsmCycleFrame = 0;
    return;
  }
  const bool changed = originChanged || sunChanged || ofsChanged;
  if (bsmCycleFrame == 0 && !changed && !bsmPendingValid)
    return; // converged and static: no work at all
  // build: frames 0..n-1 trace Bayer subsets with the frozen attempt state,
  // frame n runs the full (cheap) filter to finish the attempt
  if (bsmCycleFrame == 0)
  {
    // a build toward the state the pending attempt already delivers would finish
    // equal to live and be discarded: run a blend-only cycle instead
    bsmBuildActive = !bsmPendingValid || bsmStatesDiffer(desired, bsmPending);
    if (bsmBuildActive)
    {
      bsmBuild = desired;
      setBSMGenVars(bsmBuild);
    }
  }
  if (bsmBuildActive && bsmCycleFrame < n)
    bsmTraceFrame(bsmCycleFrame, n);
  // blend the previously finished attempt into the live ping-pong (frames 0..n-1);
  // w reaches 1 at k = n-1, one frame before the next attempt is ready
  if (bsmPendingValid)
  {
    TIME_D3D_PROFILE(clouds_bsm_blend);
    const int k = min(bsmCycleFrame, n - 1);
    const float w = 1.f / float(n - k);
    const Point2 deltaUv = k == 0 ? (bsmPending.origin - bsmLive.origin) * (1.f / BSM_SPAN) : Point2(0, 0);
    ShaderGlobal::set_texture(clouds_bsm_blend_srcVarId, cloudsBSMFilt[bsmFiltIdx ^ 1].getTexId());
    ShaderGlobal::set_texture(clouds_bsm_blend_histVarId, cloudsBSM[bsmLiveIdx].getTexId());
    ShaderGlobal::set_float4(clouds_bsm_blendVarId, w, deltaUv.x, deltaUv.y, 0.f);
    SCOPE_RENDER_TARGET;
    d3d::set_render_target({}, DepthAccess::RW, {{cloudsBSM[bsmLiveIdx ^ 1].getTex2D(), 0, 0}});
    clouds_bsm_blend_ps.render();
    d3d::resource_barrier({cloudsBSM[bsmLiveIdx ^ 1].getTex2D(), RB_RO_SRV | RB_STAGE_ALL_GRAPHICS | RB_STAGE_COMPUTE, 0, 0});
    bsmLiveIdx ^= 1;
    if (k == 0)
    {
      bsmBlendSunFrom = bsmLive.sun;
      bsmLive = bsmPending; // the blend output space is the new attempt's window/sun
    }
    // the published lookup slant follows the content mix: a per-attempt step in the
    // receiver geometry reads as a shadow pop, worst at low sun
    BSMState published = bsmLive;
    const float mixW = float(k + 1) / float(n);
    published.sun = normalize(bsmBlendSunFrom + (bsmLive.sun - bsmBlendSunFrom) * mixW);
    publishBSM(published, cloudsBSM[bsmLiveIdx]);
    if (w >= 1.f)
      bsmPendingValid = false;
  }
  if (++bsmCycleFrame > n)
  {
    // an attempt identical to what is live would blend forever to no effect
    if (bsmBuildActive && bsmStatesDiffer(bsmBuild, bsmLive))
    {
      bsmFilterInto(cloudsBSMFilt[bsmFiltIdx]);
      bsmPending = bsmBuild;
      bsmPendingValid = true;
      bsmFiltIdx ^= 1;
    }
    bsmCycleFrame = 0;
  }
}
