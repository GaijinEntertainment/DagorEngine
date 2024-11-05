// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "minimapContext.h"
#include <debug/dag_debug.h>

#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <ecs/core/entityManager.h>
#include "game/player.h"
#include "game/gameEvents.h"

#include <3d/dag_texMgr.h>
#include <ioSys/dag_dataBlock.h>
#include <gameRes/dag_gameResources.h>


const float MinimapContext::def_visible_radius = 100.0f;
static MinimapContext *s_minimap_ctx = NULL;

MinimapContext::MinimapContext() :
  mapTexId(BAD_TEXTUREID),
  backTexId(BAD_TEXTUREID),
  mapSampler(d3d::INVALID_SAMPLER_HANDLE),
  backSampler(d3d::INVALID_SAMPLER_HANDLE),
  worldLeftTop(0, 0),
  worldRightBottom(0, 0),
  backWorldLeftTop(0, 0),
  backWorldRightBottom(0, 0),
  northAngle(0)
{
  maskTexId = ::get_tex_gameres("white_circle");
  maskSampler = get_texture_separate_sampler(maskTexId);
  blendMaskTexId = ::get_tex_gameres("map_blend");
  blendMaskSampler = get_texture_separate_sampler(blendMaskTexId);

  G_ASSERT(s_minimap_ctx == NULL);
  s_minimap_ctx = this;
}

MinimapContext::~MinimapContext()
{
  release_managed_tex(mapTexId);
  release_managed_tex(backTexId);
  release_managed_tex(maskTexId);
  release_managed_tex(blendMaskTexId);

  G_ASSERT(s_minimap_ctx == this);
  s_minimap_ctx = NULL;
}

void minimap_on_render_ui(const RenderEventUI &evt)
{
  if (s_minimap_ctx)
  {
    const TMatrix &viewItm = evt.get<1>();
    float wk = evt.get<3>().wk;

    s_minimap_ctx->curViewItm = viewItm;
    s_minimap_ctx->isPersp = true;
    s_minimap_ctx->perspWk = wk;
  }
}


void MinimapContext::clamp_square(float &x, float &y, float r)
{
  float lx = fabsf(x);
  if (lx > r)
  {
    x = x / lx * r;
    y = y / lx * r;
  }

  float ly = fabsf(y);
  if (ly > r)
  {
    x = x / ly * r;
    y = y / ly * r;
  }
}


void MinimapContext::setup(Sqrat::Object cfg)
{
  int iMapColor = cfg.RawGetSlotValue<int>("mapColor", 0xFFFFFFFF);
  int iFovColor = cfg.RawGetSlotValue<int>("fovColor", 0xFFFFFFFF);
  colorMap.u = (unsigned int &)iMapColor;
  colorFov.u = (unsigned int &)iFovColor;

#define GET(n)                                                    \
  {                                                               \
    auto obj##n = cfg.GetSlot("img" #n);                          \
    if (const char *objVal = sq_objtostring(&obj##n.GetObject())) \
      pic##n.init(objVal);                                        \
  }

  GET(Tick)
  GET(N)
  GET(E)
  GET(S)
  GET(W)

#undef GET

  TEXTUREID oldMapTexId = mapTexId;
  TEXTUREID oldBackTexId = backTexId;
  mapTexId = BAD_TEXTUREID;
  backTexId = BAD_TEXTUREID;
  mapSampler = d3d::INVALID_SAMPLER_HANDLE;
  backSampler = d3d::INVALID_SAMPLER_HANDLE;

  worldLeftTop = cfg.RawGetSlotValue<Point2>("left_top", Point2(0, 0));
  worldRightBottom = cfg.RawGetSlotValue<Point2>("right_bottom", Point2(0, 0));
  backWorldLeftTop = cfg.RawGetSlotValue<Point2>("back_left_top", Point2(0, 0));
  backWorldRightBottom = cfg.RawGetSlotValue<Point2>("back_right_bottom", Point2(0, 0));
  northAngle = DegToRad(cfg.RawGetSlotValue<float>("northAngle", 0));

  if (lengthSq(worldLeftTop - worldRightBottom) > 0)
  {
    if (const char *objVal = sq_objtostring(&cfg.RawGetSlot("mapTex").GetObject()))
    {
      mapTexId = get_managed_texture_id(objVal);
      acquire_managed_tex(mapTexId);
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      mapSampler = d3d::request_sampler(smpInfo);
    }
  }
  if (lengthSq(backWorldLeftTop - backWorldRightBottom) > 0)
  {
    if (const char *objVal = sq_objtostring(&cfg.RawGetSlot("backMapTex").GetObject()))
    {
      backTexId = get_managed_texture_id(objVal);
      acquire_managed_tex(backTexId);
      backSampler = d3d::request_sampler({});
    }
  }
  release_managed_tex(oldMapTexId);
  release_managed_tex(oldBackTexId);
}


float MinimapContext::limitVisibleRadius(float radius)
{
  float maxRadius = maxVisibleRadius();
  if (maxRadius < 1e-3f)
    return radius;

  return ::min(radius, maxRadius);
}

float MinimapContext::maxVisibleRadius()
{
  Point2 backDim = ::abs(backWorldRightBottom - backWorldLeftTop);
  Point2 worldDim = ::abs(worldRightBottom - worldLeftTop);
  worldDim.x = max(worldDim.x, backDim.x);
  worldDim.y = max(worldDim.y, backDim.y);
  return ::min(worldDim.x / 2, worldDim.y / 2);
}

Point3 MinimapContext::clampPan(const Point3 &pan, float vis_radius)
{
  Point2 backDim = ::abs(backWorldRightBottom - backWorldLeftTop);
  Point2 worldDim = ::abs(worldRightBottom - worldLeftTop);
  worldDim.x = max(worldDim.x, backDim.x);
  worldDim.y = max(worldDim.y, backDim.y);
  Point3 res = pan;
  if (worldDim.x > 2 * vis_radius)
  {
    float leftEdge = ::min(::min(worldLeftTop.x, worldRightBottom.x), ::min(backWorldLeftTop.x, backWorldRightBottom.x));
    float rightEdge = ::max(::max(worldLeftTop.x, worldRightBottom.x), ::max(backWorldLeftTop.x, backWorldRightBottom.x));
    res.x = ::clamp(pan.x, leftEdge + vis_radius, rightEdge - vis_radius);
  }
  if (worldDim.y > 2 * vis_radius)
  {
    float minEdge = ::min(::min(worldLeftTop.y, worldRightBottom.y), ::min(backWorldLeftTop.y, backWorldRightBottom.y));
    float maxEdge = ::max(::max(worldLeftTop.y, worldRightBottom.y), ::max(backWorldLeftTop.y, backWorldRightBottom.y));
    res.z = ::clamp(pan.z, minEdge + vis_radius, maxEdge - vis_radius);
  }
  return res;
}
