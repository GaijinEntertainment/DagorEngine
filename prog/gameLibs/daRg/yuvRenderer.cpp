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
  alphaVarId = get_shader_variable_id("transparent", true);

  if (!shader.init(YuvRenderer::shaderName, false))
    return false;

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
}

void YuvRenderer::render(StdGuiRender::GuiContext &ctx, TEXTUREID texIdY, TEXTUREID texIdU, TEXTUREID texIdV, float l, float t,
  float r, float b, const Point2 &tc_lt, const Point2 &tc_rb, E3DCOLOR color, BlendMode blend_mode, float saturate)
{
  shader.material->set_texture_param(textureYVarId, texIdY);
  shader.material->set_texture_param(textureUVarId, texIdU);
  shader.material->set_texture_param(textureVVarId, texIdV);
  ShaderGlobal::set_int(alphaVarId, blend_mode);
  ctx.setShader(&shader);
  ctx.set_color(color);
  ctx.set_texture(texIdY, d3d::INVALID_SAMPLER_HANDLE); // TODO: Use actual sampler IDs
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
}

void YuvRenderer::term() { shader.close(); }

} // namespace darg
