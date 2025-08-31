// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/smaa.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_computeShaders.h>
#include <math/dag_TMatrix4D.h>
#include <debug/dag_assert.h>
#include <EASTL/iterator.h>
#include <memory/dag_framemem.h>
#include <image/dag_texPixel.h>
#include <3d/dag_lockTexture.h>

#define SHADER_VAR_LIST \
  VAR(smaa_rt_size)     \
  VAR(smaa_color_tex)   \
  VAR(smaa_stencil_test)

#define VAR(a) static int a##_var_id = -1;
SHADER_VAR_LIST
#undef VAR

#define VAR(a) a##_var_id = ::get_shader_variable_id(#a, true);
static void init_shader_vars() { SHADER_VAR_LIST; }
#undef VAR

CONSOLE_BOOL_VAL("smaa", useStencil, true);

SMAA::SMAA(const IPoint2 &resolution) : resolution(resolution)
{
  edge_detect.init("smaa_edge_detection");
  blend_weights.init("smaa_blend_weights");
  apply_smaa.init("smaa_neighborhood_blending");
  init_shader_vars();
  ShaderGlobal::set_color4(smaa_rt_size_var_id, 1.f / resolution.x, 1.f / resolution.y, resolution.x, resolution.y);

  edgeDetect.set(d3d::create_tex(nullptr, resolution.x, resolution.y, TEXFMT_R8G8 | TEXCF_RTARGET, 1, "smaa_edges_tex"),
    "smaa_edges_tex");
  edgeDetect.setVar();
  blendWeights.set(d3d::create_tex(nullptr, resolution.x, resolution.y, TEXFMT_R8G8B8A8 | TEXCF_RTARGET, 1, "smaa_blend_tex"),
    "smaa_blend_tex");
  blendWeights.setVar();


  areaTex = SharedTexHolder(dag::get_tex_gameres("smaa_area_tex"), "smaa_area_tex");
  searchTex = SharedTexHolder(dag::get_tex_gameres("smaa_search_tex"), "smaa_search_tex");

  {
    d3d::SamplerInfo smpInfo{};
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    smpInfo.address_mode_u = d3d::AddressMode::Clamp;
    smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(get_shader_variable_id("smaa_linear_sampler"), d3d::request_sampler(smpInfo));
  }
  {
    d3d::SamplerInfo smpInfo{};
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.address_mode_u = d3d::AddressMode::Clamp;
    smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(get_shader_variable_id("smaa_point_sampler"), d3d::request_sampler(smpInfo));
  }

  TEXTUREID waitTextures[] = {areaTex.getTexId(), searchTex.getTexId()};
  prefetch_managed_textures(dag::ConstSpan<TEXTUREID>(waitTextures, 2));

  depthStencilTex.set(
    d3d::create_tex(nullptr, resolution.x, resolution.y, TEXFMT_DEPTH32_S8 | TEXCF_RTARGET, 1, "smaa_depth_stencil"));
}

void SMAA::apply(const TextureIDPair &source, const TextureIDPair &destination)
{
  TIME_D3D_PROFILE(smaa);
  ShaderGlobal::set_int(smaa_stencil_test_var_id, useStencil);
  ShaderGlobal::set_texture(smaa_color_tex_var_id, source.getId());
  {
    TIME_D3D_PROFILE(edge_detect);
    d3d::set_render_target(0, edgeDetect.getTex2D(), 0);
    if (useStencil)
      d3d::set_depth(depthStencilTex.getTex2D(), DepthAccess::RW);
    d3d::clearview(CLEAR_TARGET | (useStencil ? CLEAR_STENCIL : 0), 0, 0, 0);
    edge_detect.render();
    d3d::resource_barrier({edgeDetect.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }
  {
    TIME_D3D_PROFILE(blend_weights);
    d3d::set_render_target(0, blendWeights.getTex2D(), 0);
    if (useStencil)
      d3d::set_depth(depthStencilTex.getTex2D(), DepthAccess::RW);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    blend_weights.render();
    d3d::resource_barrier({blendWeights.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }
  {
    TIME_D3D_PROFILE(apply_smaa);
    if (destination.getTex2D())
      d3d::set_render_target(destination.getTex2D(), 0);
    else
      d3d::set_render_target();
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0.f, 0);
    apply_smaa.render();
  }
}