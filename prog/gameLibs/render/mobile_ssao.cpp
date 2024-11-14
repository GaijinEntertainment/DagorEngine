// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/mobile_ssao.h>

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>

#include <math/dag_mathUtils.h>


#define GLOBAL_VARS_LIST  \
  VAR(ssao_tex)           \
  VAR(ssao_radius_factor) \
  VAR(ssao_texel_offset)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

MobileSSAORenderer::MobileSSAORenderer(int w, int h, int, uint32_t flags)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR

  aoWidth = w;
  aoHeight = h;

  uint32_t format = ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(flags));

  int viewCounter = 0;
  for (int i = 0; i < ssaoTex.size(); ++i)
  {
    String name(128, "ssao_tex_%d_%d", viewCounter, i);
    ssaoTex[i] = dag::create_tex(nullptr, aoWidth, aoHeight, format | TEXCF_RTARGET, 1, name.c_str());
    ssaoTex[i].getTex2D()->texbordercolor(0xFFFFFFFF);
    ssaoTex[i].getTex2D()->texaddr(TEXADDR_CLAMP);
  }

  aoRenderer.reset(create_postfx_renderer(ssao_sh_name));
  ssaoBlurRenderer.reset(create_postfx_renderer(blur_sh_name));

  setFactor();
}

void MobileSSAORenderer::setFactor()
{
  // samping ellipse around coordinate:
  // x = +-(1.0 / ssao_radius_factor) ; y = +-(lowres_rt_params.x / (lowres_rt_params.y*ssao_radius_factor))
  // so to make radius smaller increase the factor

  // Values were tuned after some testing
  constexpr float lowEndWidth = 400.f;
  constexpr float highEndWidth = 600.f;
  constexpr float baseFactor = 128.f; // factor for aoWidth > highEndWidth

  constexpr float scale = highEndWidth / lowEndWidth;
  constexpr float scaledFactor = baseFactor * scale; // factor for aoWidth < lowEndWidth

  const float factor = cvt(aoWidth, lowEndWidth, highEndWidth, scaledFactor, baseFactor);

  ShaderGlobal::set_real(ssao_radius_factorVarId, factor);
}

MobileSSAORenderer::~MobileSSAORenderer()
{
  ShaderGlobal::set_texture(ssao_texVarId, BAD_TEXTUREID);
  for (auto &tex : ssaoTex)
    tex.close();
}

void MobileSSAORenderer::renderSSAO(BaseTexture * /*depth_to_use*/)
{
  d3d::set_render_target(ssaoTex[renderId].getTex2D(), 0);
  d3d::clearview(CLEAR_DISCARD, 0xFFFFFFFF, 1.0, 0);
  aoRenderer->render();
}

void MobileSSAORenderer::setFrameNo() {}

void MobileSSAORenderer::applyBlur()
{
  Color4 texelOffset(0.5, 0.5, 1.f / aoWidth, 1.f / aoHeight);

  ssaoBlurRenderer->getMat()->set_color4_param(ssao_texel_offsetVarId, texelOffset);

  d3d::set_render_target(ssaoTex[blurId].getTex2D(), 0);
  d3d::clearview(CLEAR_DISCARD, 0, 0, 0);

  ShaderGlobal::set_texture(ssao_texVarId, ssaoTex[renderId]);
  ssaoBlurRenderer->render();
}

void MobileSSAORenderer::render(const TMatrix &, const TMatrix4 &, BaseTexture *ssaoDepthTexUse, const ManagedTex *ssao_tex,
  const ManagedTex *prev_ssao_tex, const ManagedTex *tmp_tex, const DPoint3 *, SubFrameSample)
{
  TIME_D3D_PROFILE(SSAO_total)
  SCOPE_RENDER_TARGET;
  SCOPE_RESET_SHADER_BLOCKS;

  // Other AO renderers (SSAO and GTAO) support work with external textures to be used in framegraph in daNetGame
  // MobileSSAORenderer shares the same interface but does not support it, so nullptr for these textures required
  G_ASSERT(!ssao_tex && !prev_ssao_tex && !tmp_tex);
  G_UNUSED(ssao_tex);
  G_UNUSED(prev_ssao_tex);
  G_UNUSED(tmp_tex);

  G_ASSERT(ssaoDepthTexUse);

  setFrameNo();

  {
    TIME_D3D_PROFILE(SSAO_render)
    renderSSAO(ssaoDepthTexUse);
  }

  d3d::resource_barrier({ssaoTex[renderId].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  extern bool g_ssao_blur;
  if (g_ssao_blur)
  {
    TIME_D3D_PROFILE(blurSSAO)
    applyBlur();

    d3d::resource_barrier({ssaoTex[blurId].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  // Restore state
  ShaderGlobal::set_texture(ssao_texVarId, ssaoTex[g_ssao_blur ? blurId : renderId]);
}

Texture *MobileSSAORenderer::getSSAOTex()
{
  extern bool g_ssao_blur;
  return ssaoTex[g_ssao_blur ? blurId : renderId].getTex2D();
}

TEXTUREID MobileSSAORenderer::getSSAOTexId()
{
  extern bool g_ssao_blur;
  return ssaoTex[g_ssao_blur ? blurId : renderId].getTexId();
};

void MobileSSAORenderer::setCurrentView(int)
{
  // no use for view on mobile
}
#undef GLOBAL_VARS_LIST
