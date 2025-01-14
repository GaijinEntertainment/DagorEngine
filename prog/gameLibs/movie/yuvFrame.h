// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gui/dag_stdGuiRender.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_materialData.h>
#include <osApiWrappers/dag_miscApi.h>
#include <math/dag_bounds2.h>
#include "movieAspectRatio.h"


namespace yuvframerender
{
static StdGuiRender::StdGuiShader shader;
static float l, t, r, b;
static bool ownRender = false;
static int textureYVarId, textureUVarId, textureVVarId;
static int samplerYVarId, samplerUVarId, samplerVVarId;
static void render(TEXTUREID texIdY, TEXTUREID texIdU, TEXTUREID texIdV, d3d::SamplerHandle smp);
extern bool prewarm_done;

static bool init(const IPoint2 &frameSize, bool native)
{
  float movieAspect = 1;
  if (forced_movie_inv_aspect_ratio > 0)
    movieAspect = forced_movie_inv_aspect_ratio;
  else if (frameSize.x)
    movieAspect = float(frameSize.y) / float(frameSize.x);

  Point2 vp0 = StdGuiRender::get_viewport().leftTop;
  Point2 vp1 = StdGuiRender::get_viewport().rightBottom;
  BBox2 vpBox(vp0, vp1);

  float screenAspect = vpBox.width().y / vpBox.width().x;
  l = vp0.x, t = vp0.y, r = vp1.x, b = vp1.y;
  if (screenAspect > movieAspect)
  {
    t = (vpBox.width().y - vpBox.width().x * movieAspect) / 2;
    b = t + vpBox.width().x * movieAspect;
  }
  else
  {
    l = (vpBox.width().x - vpBox.width().y / movieAspect) / 2;
    r = l + vpBox.width().y / movieAspect;
  }

  (void)native;

  textureYVarId = VariableMap::getVariableId("texY");
  textureUVarId = VariableMap::getVariableId("texU");
  textureVVarId = VariableMap::getVariableId("texV");
  samplerYVarId = VariableMap::getVariableId("texY_samplerstate");
  samplerUVarId = VariableMap::getVariableId("texU_samplerstate");
  samplerVVarId = VariableMap::getVariableId("texV_samplerstate");

  if (!shader.init("yuv_movie"))
    return false;

  return true;
}

static void startRender()
{
  ownRender = !StdGuiRender::is_render_started();
  if (ownRender)
  {
    StdGuiRender::reset_per_frame_dynamic_buffer_pos();
    StdGuiRender::start_render();
  }
}

static void render(TEXTUREID texIdY, TEXTUREID texIdU, TEXTUREID texIdV, d3d::SamplerHandle smp)
{
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  shader.material->set_texture_param(textureYVarId, texIdY);
  shader.material->set_sampler_param(samplerYVarId, smp);
  shader.material->set_texture_param(textureUVarId, texIdU);
  shader.material->set_sampler_param(samplerUVarId, smp);
  shader.material->set_texture_param(textureVVarId, texIdV);
  shader.material->set_sampler_param(samplerVVarId, smp);
  StdGuiRender::set_shader(&shader);
  StdGuiRender::set_color(0xFFFFFFFF);
  StdGuiRender::set_texture(texIdY, smp);
  StdGuiRender::render_rect(l, t, r, b);
  StdGuiRender::reset_shader();
}

static void endRender()
{
  if (ownRender)
    StdGuiRender::end_render();
  ownRender = false;
}

static void term()
{
  StdGuiRender::reset_shader();
  shader.close();
  ownRender = false;
  StdGuiRender::reset_per_frame_dynamic_buffer_pos();
}
} // namespace yuvframerender
