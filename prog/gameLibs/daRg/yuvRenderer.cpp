// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "yuvRenderer.h"

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_materialData.h>
#include <osApiWrappers/dag_miscApi.h>
#include <math/dag_bounds2.h>


namespace darg
{
const char *YuvRenderer::shaderName = "yuv_movie";


bool YuvRenderer::init()
{
  textureYVarId = VariableMap::getVariableId("texY");
  textureUVarId = VariableMap::getVariableId("texU");
  textureVVarId = VariableMap::getVariableId("texV");
  samplerYVarId = VariableMap::getVariableId("texY_samplerstate");
  samplerUVarId = VariableMap::getVariableId("texU_samplerstate");
  samplerVVarId = VariableMap::getVariableId("texV_samplerstate");
  alphaVarId = get_shader_variable_id("transparent", true);

  return true;
}

void YuvRenderer::startRender(StdGuiRender::GuiContext &ctx, bool *own_render)
{
  *own_render = !ctx.isRenderStarted();
  if (*own_render)
  {
    ctx.resetFrame();
    ctx.start_render();
  }
  if (currentShader == shadersPool.size())
    if (!shadersPool.push_back().init(YuvRenderer::shaderName, false))
      logerr("failed to init yuv renderer shader");
  G_ASSERTF(currentShader < shadersPool.size(), //
    "currentShader=%u shadersPool.size()=%u (%d max)", currentShader, shadersPool.size(), shadersPool.capacity());
}

void YuvRenderer::render(StdGuiRender::GuiContext &ctx, TEXTUREID texIdY, TEXTUREID texIdU, TEXTUREID texIdV, d3d::SamplerHandle smp,
  float l, float t, float r, float b, const Point2 &tc_lt, const Point2 &tc_rb, E3DCOLOR color, BlendMode blend_mode, float saturate)
{
  auto &shader = shadersPool[currentShader];
  shader.material->set_texture_param(textureYVarId, texIdY);
  shader.material->set_sampler_param(samplerYVarId, smp);
  shader.material->set_texture_param(textureUVarId, texIdU);
  shader.material->set_sampler_param(samplerUVarId, smp);
  shader.material->set_texture_param(textureVVarId, texIdV);
  shader.material->set_sampler_param(samplerVVarId, smp);
  ShaderGlobal::set_int(alphaVarId, blend_mode);
  ctx.setShader(&shader);
  ctx.set_color(color);
  ctx.set_texture(texIdY, smp);
  if (saturate != 1.0f)
    ctx.set_picquad_color_matrix_saturate(saturate);

  ctx.render_rect_t(l, t, r, b, tc_lt, tc_rb);

  ctx.setShader(nullptr);
  ctx.set_picquad_color_matrix(nullptr);
}

void YuvRenderer::endRender(StdGuiRender::GuiContext &ctx, bool own_render)
{
  if (own_render)
    ctx.end_render();
  currentShader++;
}

} // namespace darg
