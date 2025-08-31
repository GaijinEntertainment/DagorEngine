// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cloudsFormLUT.h"
#include "cloudsShaderVars.h"

#include "shaders/clouds2/clouds_density_height_lut.hlsli"

#include <drv/3d/dag_renderTarget.h>
#include <osApiWrappers/dag_miscApi.h>

void CloudsFormLUT::init()
{
  clouds_types_lut.close();
  clouds_erosion_lut.close();

  clouds_types_lut =
    dag::create_tex(NULL, CLOUDS_TYPES_HEIGHT_LUT, CLOUDS_TYPES_LUT, TEXCF_RTARGET | CLOUDS_ESRAM_ONLY, 1, "clouds_types_lut");
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = d3d::AddressMode::Border;
    smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(::get_shader_variable_id("clouds_types_lut_samplerstate"), d3d::request_sampler(smpInfo));
  }
  gen_clouds_types_lut.init("gen_clouds_types_lut");

  clouds_erosion_lut = dag::create_tex(NULL, 32, 1, TEXCF_RTARGET | CLOUDS_ESRAM_ONLY | TEXFMT_R8G8, 1, "clouds_erosion_lut");
  gen_clouds_erosion_lut.init("gen_clouds_erosion_lut");
  invalidate();
}

CloudsChangeFlags CloudsFormLUT::render()
{
  if (frameValid)
    return CLOUDS_NO_CHANGE;
  TIME_D3D_PROFILE(clouds_lut_gen);
  // todo: implement Compute version
  SCOPE_RENDER_TARGET;
  d3d::set_render_target(clouds_types_lut.getTex2D(), 0);
  d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0, 0);
  gen_clouds_types_lut.render();
  d3d::resource_barrier({clouds_types_lut.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  d3d::set_render_target(clouds_erosion_lut.getTex2D(), 0);
  d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0, 0);
  gen_clouds_erosion_lut.render();
  frameValid = true;
  return CLOUDS_INVALIDATED;
}
