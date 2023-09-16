#pragma once

#include <gui/dag_stdGuiRender.h>
#include <shaders/dag_shaders.h>


namespace darg
{
class YuvRenderer
{
public:
  bool init();

  void startRender(StdGuiRender::GuiContext &ctx, bool *own_render);
  void render(StdGuiRender::GuiContext &ctx, TEXTUREID texIdY, TEXTUREID texIdU, TEXTUREID texIdV, float l, float t, float r, float b,
    const Point2 &tc_lt, const Point2 &tc_rb, E3DCOLOR color, BlendMode blend_mode, float saturate);
  void endRender(StdGuiRender::GuiContext &ctx, bool own_render);
  void term();

  static const char *shaderName;

private:
  StdGuiRender::StdGuiShader shader;
  int textureYVarId = -1, textureUVarId = -1, textureVVarId = -1;
  int alphaVarId = -1;
};
} // namespace darg
