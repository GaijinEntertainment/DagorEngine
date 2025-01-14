// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cloudsField.h"
#include "cloudsShaderVars.h"

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_lockTexture.h>
#include <render/viewVecs.h>

#include <util/dag_convar.h>
#include <osApiWrappers/dag_miscApi.h>

CONSOLE_BOOL_VAL("clouds", regenField, false);

// todo: move it to gameParams
CONSOLE_INT_VAL("clouds", downsampled_field_res, 1, 1, 3); // 1<<(4+x)

bool CloudsField::getReadbackData(float &alt_start, float &alt_top) const
{
  if (!readbackData.valid)
    return false;
  alt_start = readbackData.layerAltStart;
  alt_top = readbackData.layerAltTop;
  return true;
}

void CloudsField::closedCompressed()
{
  cloudsFieldVolTemp.close();
  cloudsFieldVolCompressed.close();
}

void CloudsField::initCompressed()
{
  genCloudsFieldCmpr.init("gen_cloud_field_cmpr", "gen_cloud_field_cmpr_ps");
  if (!genCloudsFieldCmpr.isComputeLoaded())
    genCloudLayersNonEmptyCmpr.init("gen_cloud_layers_non_empty_cmpr", nullptr, 0, "gen_cloud_layers_non_empty_cmpr", false);
  // todo: on consoles we don't need temp, we can alias memory
  genCloudsFieldCmpr.initVoltex(cloudsFieldVolTemp, resXZ / 4, resXZ / 4, resY, TEXFMT_R32G32UI | CLOUDS_ESRAM_ONLY, 1,
    "clouds_field_volume_tmp");
  cloudsFieldVolCompressed =
    dag::create_voltex(resXZ, resXZ, resY, TEXFMT_ATI1N | TEXCF_UPDATE_DESTINATION, 1, "clouds_field_volume_compressed");
  cloudsFieldVolCompressed->texaddru(TEXADDR_WRAP);
  cloudsFieldVolCompressed->texaddrv(TEXADDR_WRAP);
  cloudsFieldVolCompressed->texaddrw(TEXADDR_BORDER);
  ShaderGlobal::set_texture(clouds_field_volumeVarId, cloudsFieldVolCompressed);
}

void CloudsField::genFieldGeneral(VoltexRenderer &renderer, DynamicShaderHelper non_empty_fill, ManagedTex &voltex)
{
  if (renderer.isComputeLoaded())
    d3d::set_rwtex(STAGE_CS, 1, layersPixelCount.getTex2D(), 0, 0);
  renderer.render(voltex);
  if (renderer.isComputeLoaded())
    d3d::set_rwtex(STAGE_CS, 1, nullptr, 0, 0);

  d3d::resource_barrier({voltex.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  if (!renderer.isComputeLoaded()) // we didn't fill layersPixelCount
  {
    SCOPE_RENDER_TARGET;
    ShaderGlobal::set_color4(dispatch_sizeVarId, resXZ, resXZ, resY, 0);
    d3d::set_render_target(layersPixelCount.getTex2D(), 0);
    non_empty_fill.shader->setStates(0, true);
    d3d::setvsrc(0, 0, 0);
    d3d::draw_instanced(PRIM_TRILIST, 0, 1, resXZ * resXZ);
  }
  d3d::resource_barrier({layersPixelCount.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
}

void CloudsField::genFieldCompressed()
{
  TIME_D3D_PROFILE(cloud_field_vol_cmpr);
  genFieldGeneral(genCloudsFieldCmpr, genCloudLayersNonEmptyCmpr, cloudsFieldVolTemp);

  // todo: remove me on consoles - we can alias memory
  TIME_D3D_PROFILE(copy_compr);
  cloudsFieldVolCompressed->updateSubRegion(cloudsFieldVolTemp.getVolTex(), 0, 0, 0, 0, // source mip, x,y,z
    max(1, resXZ / 4), max(1, resXZ / 4), resY,                                         // width x height x depth
    0, 0, 0, 0);                                                                        // dest mip, x,y,z
  // todo: we can remove temp texture
}

void CloudsField::genField()
{
  TIME_D3D_PROFILE(cloud_field_vol);
  genFieldGeneral(genCloudsField, genCloudLayersNonEmpty, cloudsFieldVol);
  d3d::resource_barrier({cloudsFieldVol.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
}

void CloudsField::initDownsampledField()
{
  cloudsDownSampledField.close();
  downsampleCloudsField.initVoltex(cloudsDownSampledField, (resXZ + downsampleRatio - 1) / downsampleRatio,
    (resXZ + downsampleRatio - 1) / downsampleRatio, (resY + downsampleRatio - 1) / downsampleRatio, TEXFMT_L8, 1,
    "clouds_field_volume_low");
  cloudsDownSampledField->disableSampler();
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = d3d::AddressMode::Wrap;
  smpInfo.address_mode_v = d3d::AddressMode::Wrap;
  smpInfo.address_mode_w = d3d::AddressMode::Border;
  ShaderGlobal::set_sampler(::get_shader_variable_id("clouds_field_volume_low_samplerstate"), d3d::request_sampler(smpInfo));
  cloudsDownSampledField.setVar();
  d3d::resource_barrier({cloudsDownSampledField.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}

void CloudsField::renderDownsampledField()
{
  TIME_D3D_PROFILE(downsample_field_vol);
  ShaderGlobal::set_int(clouds_field_downsample_ratioVarId, downsampleRatio);
  downsampleCloudsField.render(cloudsDownSampledField);
  d3d::resource_barrier({cloudsDownSampledField.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
}

void CloudsField::init()
{
  initCloudsVolumeRenderer();
  cloudsFieldVol.close();
  closedCompressed();
  if (useCompression)
  {
    initCompressed();
  }
  if (!cloudsFieldVolTemp)
  {
    genCloudsField.init("gen_cloud_field", "gen_cloud_field_ps");
    genCloudsField.initVoltex(cloudsFieldVol, resXZ, resXZ, resY, TEXFMT_L8, 1, "clouds_field_volume");
    if (!genCloudsField.isComputeLoaded())
      genCloudLayersNonEmpty.init("gen_cloud_layers_non_empty", nullptr, 0, "gen_cloud_layers_non_empty", false);
    cloudsFieldVol->disableSampler();
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = d3d::AddressMode::Wrap;
      smpInfo.address_mode_v = d3d::AddressMode::Wrap;
      smpInfo.address_mode_w = d3d::AddressMode::Border;
      ShaderGlobal::set_sampler(::get_shader_variable_id("clouds_field_volume_samplerstate"), d3d::request_sampler(smpInfo));
    }
    cloudsFieldVol.setVar();
    d3d::resource_barrier({cloudsFieldVol.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  }
  downsampleCloudsField.init("downsample_cloud_field", "downsample_cloud_field_ps", 1, 1, 1);
  initDownsampledField();

  layersPixelCount.close();
  layersPixelCount = dag::create_tex(nullptr, resY, 1,
    (genCloudsField.isComputeLoaded() ? TEXCF_UNORDERED : 0) | TEXCF_RTARGET | TEXFMT_R32F, 1, "cloud_layers_non_empty");

  if (VoltexRenderer::is_compute_supported())
    refineAltitudes.reset(new_compute_shader("clouds_refine_altitudes", false));
  else
    refineAltitudesPs.init("clouds_refine_altitudes_ps");
  if (!layersHeights)
  {
    layersHeights = dag::create_tex(nullptr, 2, 1, (refineAltitudes ? TEXCF_UNORDERED : TEXCF_RTARGET) | TEXFMT_A32B32G32R32F, 1,
      "cloud_layers_altitudes_tex");
    layersHeights->disableSampler();
  }
  if (!readbackData.query)
  {
    readbackData.query.reset(d3d::create_event_query());
    readbackData.valid = false;
  }

  frameValid = false;
}

void CloudsField::layersHeightsBarrier() { d3d::resource_barrier({layersHeights.getTex2D(), RB_RO_SRV | RB_STAGE_ALL_SHADERS, 0, 0}); }

void CloudsField::setParams(const DaSkies::CloudsSettingsParams &params)
{
  const int xz = clamp(256 * params.quality, 128, 2048), y = clamp(32, 32 * params.quality, 192);
  const bool nextCompression = !params.fastEvolution && d3d::check_voltexformat(TEXFMT_ATI1N | CLOUDS_ESRAM_ONLY) &&
                               d3d::get_driver_desc().caps.hasResourceCopyConversion;
  const bool compressionChanged = nextCompression != useCompression;
  useCompression = nextCompression;

  if (xz != resXZ || y != resY || compressionChanged)
  {
    resXZ = xz;
    resY = y;
    init();
  }
  int txz = xz, ty = y;
  if (params.competitive_advantage)
  {
    txz = clamp(256 * params.target_quality, 128, 2048), ty = clamp(32, 32 * params.target_quality, 192);
    if (targetXZ != txz || targetY != ty || averaging != params.maximum_averaging_ratio)
      invalidate();
  }
  averaging = params.maximum_averaging_ratio;
  targetXZ = txz;
  targetY = ty;
}
void CloudsField::invalidate()
{
  frameValid = false;
  readbackData.valid = false;
}

void CloudsField::doReadback()
{

  if (!readbackData.valid && d3d::get_event_query_status(readbackData.query.get(), false))
  {
    TIME_PROFILE(end_clouds_gpu_readback);
    if (auto lockedTex = lock_texture<const Point4>(layersHeights.getTex2D(), 0, TEXLOCK_READ))
    {
      Point4 src = lockedTex.at(1, 0);
      readbackData.layerAltStart = src.x;
      readbackData.layerAltTop = src.y;
    }
    readbackData.valid = true;
  }
}

CloudsChangeFlags CloudsField::render()
{
  if (frameValid && !regenField.get())
  {
    int ratio = resXZ >= 512 ? 3 : 2;
    ratio = downsampled_field_res.get() == 1 ? ratio : downsampled_field_res.get();
    if ((1 << ratio) != downsampleRatio)
    {
      downsampleRatio = (1 << ratio);
      initDownsampledField();
      renderDownsampledField();
    }
    doReadback();
    return CLOUDS_NO_CHANGE;
  }
  {
    TIME_D3D_PROFILE(clear_layer);
    d3d_err(d3d::clear_rt({layersPixelCount.getTex2D()}));
  }
  {
    ShaderGlobal::set_color4(clouds_field_resVarId, resXZ, resY, targetXZ, targetY);
    ShaderGlobal::set_real(clouds_average_weightVarId, averaging);
    if (useCompression)
      genFieldCompressed();
    else
      genField();
    renderDownsampledField();
    TIME_D3D_PROFILE(refineLayers);
    if (refineAltitudes)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), layersHeights.getTex2D());
      refineAltitudes->dispatch(1, 1, 1);
    }
    else
    {
      SCOPE_RENDER_TARGET;
      d3d::set_render_target(layersHeights.getTex2D(), 0);
      refineAltitudesPs.render();
    }
    d3d::resource_barrier({layersHeights.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    readbackData.valid = false;
    TIME_D3D_PROFILE(start_gpu_readback);
    /* NOTE: this optimization does not work on dx12 driver and disabled
    After prefetch lockimg(nullptr, ...) texture have a state 0x0 (in a split barrier), so we cannot use the
    texture in any shaders after prefetch and before real fetch (in function doReadback).
    The texture is used in macros DISTANCE_TO_CLOUDS2 in several shaders.
    int stride;
    if (layersHeights.getTex2D()->lockimg(nullptr, stride, 0, TEXLOCK_READ | TEXLOCK_NOSYSLOCK))//start readback
      layersHeights.getTex2D()->unlockimg();*/
    d3d::issue_event_query(readbackData.query.get());
  }
  frameValid = true;
  return CLOUDS_INVALIDATED;
}

void CloudsField::initCloudsVolumeRenderer()
{
  if (d3d::get_driver_desc().shaderModel >= 5.0_sm)
    build_dacloud_volume_cs.reset(new_compute_shader("build_dacloud_volume_cs", false));
  else
    build_dacloud_volume_ps.init("build_dacloud_volume_ps");
}

void CloudsField::renderCloudVolume(VolTexture *cloud_volume, float max_dist, const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  if (!cloud_volume)
    return;
  TextureInfo tinfo;
  cloud_volume->getinfo(tinfo);
  set_viewvecs_to_shader(view_tm, proj_tm);
  ShaderGlobal::set_int4(cloud_volume_resVarId, tinfo.w, tinfo.h, tinfo.d, 0);
  ShaderGlobal::set_real(cloud_volume_distVarId, max_dist);
  if ((tinfo.cflg & TEXCF_UNORDERED) && build_dacloud_volume_cs.get())
  {
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), cloud_volume);
    build_dacloud_volume_cs->dispatchThreads(tinfo.w, tinfo.h, 1);
  }
  else
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(0, cloud_volume, d3d::RENDER_TO_WHOLE_ARRAY, 0);
    build_dacloud_volume_ps.getElem()->setStates();
    d3d::draw_instanced(PRIM_TRILIST, 0, 1, tinfo.d);
  }
}
