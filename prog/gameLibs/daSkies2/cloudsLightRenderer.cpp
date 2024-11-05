// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cloudsLightRenderer.h"

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_info.h>
#include <render/computeShaderFallback/voltexRenderer.h>
#include <osApiWrappers/dag_miscApi.h>

#include "shaders/clouds2/cloud_settings.hlsli"

void CloudsLightRenderer::init()
{
  if (VoltexRenderer::is_compute_supported())
    gen_clouds_light_texture_cs.reset(new_compute_shader("gen_clouds_light_texture_cs", true));
  else
    gen_clouds_light_texture_ps.init("gen_clouds_light_texture_ps");
  clouds_light_color.close();
  int texflags = gen_clouds_light_texture_cs ? TEXCF_UNORDERED : TEXCF_RTARGET;
  int texfmt =
    (d3d::get_texformat_usage(TEXFMT_R11G11B10F, RES3D_TEX) & texflags) == texflags ? TEXFMT_R11G11B10F : TEXFMT_A16B16G16R16F;
  if (VoltexRenderer::is_compute_supported() && d3d::get_driver_desc().issues.hasBrokenComputeFormattedOutput)
    texfmt = TEXFMT_A32B32G32R32F;
  clouds_light_color = dag::create_voltex(8, 8, 2 * CLOUDS_LIGHT_TEXTURE_WIDTH, texfmt | texflags, 1, "clouds_light_color");
  clouds_light_color->disableSampler();
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = d3d::AddressMode::Clamp;
  smpInfo.address_mode_v = d3d::AddressMode::Clamp;
  smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  ShaderGlobal::set_sampler(::get_shader_variable_id("clouds_light_color_samplerstate"), d3d::request_sampler(smpInfo));
  resetGen = 0;
}

CloudsChangeFlags CloudsLightRenderer::render(const Point3 &main_light_dir, const Point3 &second_light_dir)
{
  if (fabsf(lastMainLightDirY - main_light_dir.y) > 1e-4f || fabsf(lastSecondLightDirY - second_light_dir.y) > 1e-4f) // todo: check
                                                                                                                      // for
                                                                                                                      // brightness
                                                                                                                      // as well?
    invalidate();
  const uint32_t cresetGen = get_d3d_full_reset_counter();
  if (resetGen == cresetGen) // nothing to update
    return CLOUDS_NO_CHANGE;
  TIME_D3D_PROFILE(render_clouds_light);
  lastMainLightDirY = main_light_dir.y;
  lastSecondLightDirY = second_light_dir.y;

  if (gen_clouds_light_texture_cs)
  {
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), clouds_light_color.getVolTex());
    gen_clouds_light_texture_cs->dispatch(1, 1, 2 * CLOUDS_LIGHT_TEXTURE_WIDTH);
  }
  else
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(0, clouds_light_color.getVolTex(), d3d::RENDER_TO_WHOLE_ARRAY, 0);
    gen_clouds_light_texture_ps.getElem()->setStates();
    d3d::draw_instanced(PRIM_TRILIST, 0, 1, 2 * CLOUDS_LIGHT_TEXTURE_WIDTH);
  }
  d3d::resource_barrier({clouds_light_color.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  resetGen = cresetGen;
  return CLOUDS_INCREMENTAL; // although it can be that its first time, but then shadows will return INVALIDATED
}
