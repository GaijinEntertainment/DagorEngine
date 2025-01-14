// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gui/dag_stdGuiRender.h>
#include <shaders/dag_shaders.h>
#include <generic/dag_staticTab.h>

namespace darg
{
class YuvRenderer
{
public:
  bool init();
  void resetOnFrameStart() { currentShader = 0; }

  void startRender(StdGuiRender::GuiContext &ctx, bool *own_render);
  void render(StdGuiRender::GuiContext &ctx, TEXTUREID texIdY, TEXTUREID texIdU, TEXTUREID texIdV, d3d::SamplerHandle smp, float l,
    float t, float r, float b, const Point2 &tc_lt, const Point2 &tc_rb, E3DCOLOR color, BlendMode blend_mode, float saturate);
  void endRender(StdGuiRender::GuiContext &ctx, bool own_render);

  static const char *shaderName;

private:
  StaticTab<StdGuiRender::StdGuiShader, 16> shadersPool; //< we allow upto 16 videoframes on screen rendered simultaneously
  unsigned currentShader = 0;
  int textureYVarId = -1, textureUVarId = -1, textureVVarId = -1;
  int samplerYVarId = -1, samplerUVarId = -1, samplerVVarId = -1;
  int alphaVarId = -1;
};
} // namespace darg
