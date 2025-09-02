// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resolution.h"
#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"

void get_display_resolution(int &w, int &h)
{
  if (WorldRenderer *renderer = (WorldRenderer *)get_world_renderer())
  {
    renderer->getDisplayResolution(w, h);
  }
  else
  {
    w = h = 0;
    logerr("WorldRenderer doesn't exist, failed get_display_resolution");
  }
}

void get_rendering_resolution(int &w, int &h)
{
  if (WorldRenderer *renderer = (WorldRenderer *)get_world_renderer())
  {
    renderer->getRenderingResolution(w, h);
  }
  else
  {
    w = h = 0;
    logerr("WorldRenderer doesn't exist, failed get_rendering_resolution");
  }
}

void get_postfx_resolution(int &w, int &h)
{
  if (WorldRenderer *renderer = (WorldRenderer *)get_world_renderer())
  {
    renderer->getPostFxInternalResolution(w, h);
  }
  else
  {
    w = h = 0;
    logerr("WorldRenderer doesn't exist, failed get_postfx_resolution");
  }
}

void get_final_render_target_resolution(int &w, int &h)
{
  if (WorldRenderer *renderer = (WorldRenderer *)get_world_renderer())
  {
    TextureInfo info;
    renderer->getFinalTargetTex()->getinfo(info);
    w = info.w;
    h = info.h;
  }
  else
  {
    w = h = 0;
    logerr("WorldRenderer doesn't exist, failed get_final_render_target_resolution");
  }
}

void get_max_possible_rendering_resolution(int &w, int &h)
{
  if (WorldRenderer *renderer = (WorldRenderer *)get_world_renderer())
  {
    renderer->getMaxPossibleRenderingResolution(w, h);
  }
  else
  {
    w = h = 0;
    logerr("WorldRenderer doesn't exist, failed get_max_possible_rendering_resolution");
  }
}

IPoint2 get_display_resolution()
{
  int w = 0, h = 0;
  get_display_resolution(w, h);
  return IPoint2(w, h);
}

IPoint2 get_rendering_resolution()
{
  int w = 0, h = 0;
  get_rendering_resolution(w, h);
  return IPoint2(w, h);
}

IPoint2 get_postfx_resolution()
{
  int w = 0, h = 0;
  get_postfx_resolution(w, h);
  return IPoint2(w, h);
}

IPoint2 get_final_render_target_resolution()
{
  int w = 0, h = 0;
  get_final_render_target_resolution(w, h);
  return IPoint2(w, h);
}

IPoint2 get_max_possible_rendering_resolution()
{
  int w = 0, h = 0;
  get_max_possible_rendering_resolution(w, h);
  return IPoint2(w, h);
}

bool is_upsampling()
{
  if (auto wr = (const WorldRenderer *)get_world_renderer())
  {
    return wr->isUpsampling();
  }
  return false;
}
