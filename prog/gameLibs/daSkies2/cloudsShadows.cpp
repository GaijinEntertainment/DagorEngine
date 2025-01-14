// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cloudsShadows.h"
#include "cloudsShaderVars.h"

#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>

#include <util/dag_convar.h>
#include <workCycle/dag_workCycle.h>
#include <osApiWrappers/dag_miscApi.h>

CONSOLE_BOOL_VAL("clouds", regenShadows, false);

void CloudsShadows::initTemporal()
{
  copy_cloud_shadows_volume.init("copy_cloud_shadows_volume", "copy_cloud_shadows_volume_ps");
  gen_cloud_shadows_volume_partial.init("gen_cloud_shadows_volume_partial_cs", "gen_cloud_shadows_volume_partial_ps");
  cloudsShadowsTemp.close();
  copy_cloud_shadows_volume.initVoltex(cloudsShadowsTemp, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD, TEXFMT_R32UI, 1, "cloud_shadows_old_values");
  d3d::SamplerInfo smpInfo;
  smpInfo.filter_mode = d3d::FilterMode::Point;
  smpInfo.mip_map_mode = d3d::MipMapMode::Point;
  ShaderGlobal::set_sampler(::get_shader_variable_id("cloud_shadows_old_values_samplerstate"), d3d::request_sampler(smpInfo));
  cloudsShadowsTemp->disableSampler();

  ShaderGlobal::set_color4(clouds_compute_widthVarId, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD, CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD,
    CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD, 0);
  resetGen = 0;
}

void CloudsShadows::updateTemporalStep(int x, int y, int z)
{
  ShaderGlobal::set_color4(clouds_start_compute_offsetVarId, x, y, z, 0);
  {
    TIME_D3D_PROFILE(cloud_shadows_vol_copy);
    copy_cloud_shadows_volume.render(cloudsShadowsTemp);
    d3d::resource_barrier({cloudsShadowsTemp.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }
  {
    TIME_D3D_PROFILE(cloud_shadows_vol_calc);
    gen_cloud_shadows_volume_partial.render(cloudsShadowsVol, 0,
      CLOUD_SHADOWS_WARP_SIZE_XZ * IPoint3(CLOUDS_MAX_SHADOW_TEMPORAL_XZ, CLOUDS_MAX_SHADOW_TEMPORAL_XZ, CLOUDS_MAX_SHADOW_TEMPORAL_Y),
      IPoint3(x, y, z));
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
    ShaderGlobal::set_color4(clouds_new_shadow_ambient_weightVarId, effect, updateAmbient ? effect : 0.f, 0, 0);
    int xs = (temporalStep % CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS),
        ys = (temporalStep / CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS) % CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS,
        zs = (temporalStep / CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS / CLOUDS_MAX_SHADOW_TEMPORAL_XZ_STEPS) %
             CLOUDS_MAX_SHADOW_TEMPORAL_Y_STEPS;
    updateTemporalStep(xs * CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD, ys * CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD,
      zs * CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD);
    if (++temporalStep != temporalStepFinal)
      return true;
    rerenderShadows2D = true;
    changed = true;
  }
  else if (temporalStep == TEMPORAL_STEP_FORCED)
  {
    TIME_D3D_PROFILE(cloud_shadows_vol_all);
    ShaderGlobal::set_color4(clouds_new_shadow_ambient_weightVarId, 1, updateAmbient ? 1 : 0, 0, 0);
    for (int z = 0; z < CLOUD_SHADOWS_VOLUME_RES_Y; z += CLOUDS_MAX_SHADOW_TEMPORAL_Y_WD)
      for (int y = 0; y < CLOUD_SHADOWS_VOLUME_RES_XZ; y += CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD)
        for (int x = 0; x < CLOUD_SHADOWS_VOLUME_RES_XZ; x += CLOUDS_MAX_SHADOW_TEMPORAL_XZ_WD)
          updateTemporalStep(x, y, z);
    rerenderShadows2D = true;
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
  genCloudShadowsVolume.initVoltex(cloudsShadowsVol, CLOUD_SHADOWS_VOLUME_RES_XZ, CLOUD_SHADOWS_VOLUME_RES_XZ,
    CLOUD_SHADOWS_VOLUME_RES_Y, fmt, 1, "clouds_shadows_volume");
  cloudsShadowsVol->disableSampler();
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
void CloudsShadows::addRecalcAll()
{
  temporalStepFinal += ALL_TEMPORAL_STEPS_LERP;
  while (temporalStepFinal - temporalStep > ALL_TEMPORAL_STEPS_LERP) // if we added more than once
    temporalStepFinal -= ALL_TEMPORAL_STEPS_LERP;
  if (temporalStep > ALL_TEMPORAL_STEPS_LERP) // if we added more than once
  {
    uint32_t fullSteps = temporalStep / ALL_TEMPORAL_STEPS_LERP;
    temporalStepFinal -= fullSteps * ALL_TEMPORAL_STEPS_LERP;
    temporalStep -= fullSteps * ALL_TEMPORAL_STEPS_LERP;
  }
}

void CloudsShadows::forceFullUpdate(const Point3 &main_light_dir, bool update_ambient)
{
  lastLightDir = main_light_dir;
  updateAmbient |= update_ambient;
  addRecalcAll();
}

CloudsChangeFlags CloudsShadows::render(const Point3 &main_light_dir) // todo: use main_light_dir for next temporality
{
  const uint32_t cresetGen = get_d3d_full_reset_counter();
  if (lastLightDir != main_light_dir)
    forceFullUpdate(main_light_dir, false);
  if (cresetGen == resetGen) // and sun light changed
  {
    if (regenShadows.get())
      temporalStep = temporalStepFinal = TEMPORAL_STEP_FORCED;
    CloudsChangeFlags ret = CLOUDS_NO_CHANGE;
    if (updateTemporal())
      ret = CLOUDS_INCREMENTAL;
    if (rerenderShadows2D)
      renderShadows2D();
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
        ShaderGlobal::set_color4(clouds_start_compute_offsetVarId, splitStep * i, 0, 0, 0);
        genCloudShadowsVolume.render(cloudsShadowsVol, 0, {splitStep, -1, -1});
      }
      ShaderGlobal::set_color4(clouds_start_compute_offsetVarId, 0, 0, 0, 0);
    }
    else
    {
      ShaderGlobal::set_color4(clouds_start_compute_offsetVarId, 0, 0, 0, 0);
      genCloudShadowsVolume.render(cloudsShadowsVol);
    }

    d3d::resource_barrier({cloudsShadowsVol.getVolTex(), RB_RO_SRV | RB_STAGE_ALL_GRAPHICS | RB_STAGE_COMPUTE, 0, 0});
    renderShadows2D();
    validate();
    // generateMips2d(cloud1);
  }
  resetGen = cresetGen;
  return CLOUDS_INVALIDATED;
}

void CloudsShadows::renderShadows2D()
{
  if (!cloudsShadows2d)
    return;
  TIME_D3D_PROFILE(clouds_2d_shadows);
  SCOPE_RENDER_TARGET;
  d3d::set_render_target(cloudsShadows2d.getTex2D(), 0);
  build_shadows_2d_ps.render();
  d3d::resource_barrier({cloudsShadows2d.getTex2D(), RB_RO_SRV | RB_STAGE_ALL_GRAPHICS | RB_STAGE_COMPUTE, 0, 0});
  rerenderShadows2D = false;
}

void CloudsShadows::initShadows2D()
{
  if (cloudsShadows2d)
    return;
  // may be build a mip chain?
  cloudsShadows2d =
    dag::create_tex(NULL, CLOUD_SHADOWS_VOLUME_RES_XZ, CLOUD_SHADOWS_VOLUME_RES_XZ, TEXFMT_L8 | TEXCF_RTARGET, 1, "clouds_shadows_2d");
  build_shadows_2d_ps.init("build_shadows_2d_ps");
  rerenderShadows2D = true;
}
