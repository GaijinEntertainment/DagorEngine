// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "riGen/riGenData.h"

#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderVar.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <math/dag_bounds2.h>
#include <math/dag_TMatrix4.h>
#include <image/dag_texPixel.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/clipmapShadow.h>

static constexpr float CLIPMAP_SHADOW_DELTA_MUL = 1.f / 16.f;
static int clipmap_shadow_near_far_tc_offsetVarId = -1;
static int clipmap_shadowsBlockId = -1;
static TMatrix look_down_vtm = TMatrix::IDENT;
static int clipmapShadowTexVarId = -1;
static int clipmapShadowFadeOutVarId = -1;

void ClipmapShadow::setUpSampler() const
{
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  smpInfo.border_color = d3d::BorderColor::Color::OpaqueWhite;
  smpInfo.anisotropic_max = 1;
  ShaderGlobal::set_sampler(get_shader_variable_id("clipmap_shadow_tex_samplerstate"), d3d::request_sampler(smpInfo));
}

void ClipmapShadow::init(int shadowSize)
{
  look_down_vtm.setcol(0, 1, 0, 0);
  look_down_vtm.setcol(1, 0, 0, 1);
  look_down_vtm.setcol(2, 0, 1, 0);
  look_down_vtm.setcol(3, 0, 0, 0);
  look_down_vtm = orthonormalized_inverse(look_down_vtm);

  clipmapShadowSize = shadowSize;
  clipmap_shadowsBlockId = ShaderGlobal::getBlockId("clipmap_shadows");

  unsigned clipmapShadowFlags = TEXFMT_L8;
  if (!(d3d::get_texformat_usage(TEXFMT_L8, RES3D_TEX) & d3d::USAGE_RTARGET))
  {
    debug("l8  format not supported - reduce clipmapshadow by half");
    clipmapShadowSize /= 2;
    clipmapShadowFlags = TEXFMT_A8R8G8B8;
  }

  clipmapShadowTex = dag::create_tex(nullptr, clipmapShadowSize * NUM_CLIPMAP_SHADOW_CASCADES, clipmapShadowSize,
    clipmapShadowFlags | TEXCF_RTARGET, 1, "clipmapShadowTex");

  d3d_err(clipmapShadowTex.getTex2D());

  clipmapShadowTex->disableSampler();
  setUpSampler();

  clipmapShadowTexVarId = get_shader_variable_id("clipmap_shadow_tex");
  worldToClipmapShadowVarId[0] = get_shader_variable_id("world_to_far_clipmap_shadow");
  worldToClipmapShadowVarId[1] = get_shader_variable_id("world_to_near_clipmap_shadow");
  clipmap_shadow_near_far_tc_offsetVarId = get_shader_variable_id("clipmap_shadow_near_far_tc_offset");

  clipmapShadowFadeOutVarId = get_shader_variable_id("clipmap_shadow_fade_out", true);

  for (int j = 0; j < NUM_CLIPMAP_SHADOW_CASCADES; ++j)
  {
    torHelpers[j].texSize = clipmapShadowSize;
    torHelpers[j].curOrigin = IPoint2(-1000000, 100000);

    worldToToroidal[j] = Color4(0, 0, 0, 0);
    uvOffset[j] = Point2(0, 0);
    regions[j].clear();
    quadRegions[j].clear();
  }

  invalidate();
  setDistance(5000, 0.3, 0.35);
}

void ClipmapShadow::setDistance(float averageFarPlane, float delta, float near_cascade_scale)
{
  for (unsigned int cascadeNo = 0; cascadeNo < NUM_CLIPMAP_SHADOW_CASCADES; cascadeNo++)
  {
    float scale = (cascadeNo == 0) ? 1.f : near_cascade_scale;
    clipmapShadowWorldSize[cascadeNo] =
      2.f * averageFarPlane * scale * (1.f + CLIPMAP_SHADOW_DELTA_MUL / (1.f - CLIPMAP_SHADOW_DELTA_MUL));
    // clipmapShadowDelta[cascadeNo] = 0.5f * clipmapShadowWorldSize[cascadeNo] * CLIPMAP_SHADOW_DELTA_MUL;
  }
  invalidate();

  // Set clipmap shadow to fade out at max RI distance roughly.

  // A*max_dist + B = 0
  // A*min_dist + B = 1
  // A = 1/(min_dist-maxdist)
  // B = -max_dist/(min_dist-maxdist)
  // A = 1/(maxdist*delta-maxdist)
  // B = 1/(1-delta)

  float deltaRcp = 1.0 - delta;
  debug("deltaShadowRCP = %g A=%g B=%g", 1.f - deltaRcp, -1. / averageFarPlane / (1.f - deltaRcp), 1. / (1.f - deltaRcp));
  ShaderGlobal::set_color4_fast(clipmapShadowFadeOutVarId, -1.f / (averageFarPlane * (1.f - deltaRcp)), 1.f / (1.f - deltaRcp), 0.f,
    0.f);
}

void ClipmapShadow::close() { clipmapShadowTex.close(); }

void ClipmapShadow::switchOff()
{
  close();
  invalidate();
  Color4 worldToClipEmpty(1000000, 1000000, 1e6f, 1e6f);
  TexImage32 img[2];
  img[0].w = img[0].h = 1;
  img[1].w = img[1].h = -1;
  clipmapShadowTex = dag::create_tex(img, 1, 1, TEXFMT_A8R8G8B8, 1, "clipmapShadowWhiteTex");
  clipmapShadowTex->disableSampler();
  clipmapShadowSize = 0;
  d3d_err(clipmapShadowTex.getTex2D());
  setUpSampler();

  clipmapShadowTexVarId = get_shader_variable_id("clipmap_shadow_tex", true);
  ShaderGlobal::set_texture(clipmapShadowTexVarId, clipmapShadowTex.getTexId());
  ShaderGlobal::set_color4(get_shader_variable_id("world_to_far_clipmap_shadow", true), worldToClipEmpty);
  ShaderGlobal::set_color4(get_shader_variable_id("world_to_near_clipmap_shadow", true), worldToClipEmpty);
}

void ClipmapShadow::reset()
{
  if (clipmapShadowTex.getTex2D())
  {
    // If clipmap shadow is turned off, the texture is not a render target, but a white pixel
    TextureInfo ti;
    clipmapShadowTex->getinfo(ti);
    if (ti.cflg & TEXCF_RTARGET)
    {
      d3d::GpuAutoLock gpuLock;
      Driver3dRenderTarget prevRt;
      d3d::get_render_target(prevRt);
      d3d::set_render_target();
      d3d::set_render_target(clipmapShadowTex.getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, 0xFFFFFFFF, 1.f, 0);
      d3d::set_render_target(prevRt);
      d3d::resource_barrier({clipmapShadowTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
  }
  invalidate();
}

void ClipmapShadow::invalidate()
{
  for (int j = 0; j < NUM_CLIPMAP_SHADOW_CASCADES; ++j)
  {
    quadRegions[j].clear();
    torHelpers[j].curOrigin = IPoint2(-10000000, -1000000);
  }
}

bool ClipmapShadow::getBBox(BBox2 &box) const
{
  if (!clipmapShadowTex.getTex2D() || !clipmapShadowSize)
    return false;
  float texelSize = clipmapShadowWorldSize[0] / (float)clipmapShadowSize;
  Point2 halfSize(0.5f * clipmapShadowWorldSize[0], 0.5f * clipmapShadowWorldSize[0]);
  box[0] = Point2(torHelpers[0].curOrigin) * texelSize - halfSize;
  box[1] = Point2(torHelpers[0].curOrigin) * texelSize - halfSize;
  return true;
}


bool ClipmapShadow::update(float min_height, float max_height, const Point3 &view_pos)
{
  if (!clipmapShadowTex.getTex2D() || !RendInstGenData::renderResRequired)
    return false;

  TextureInfo ti;
  clipmapShadowTex->getinfo(ti);
  if (!(ti.cflg & TEXCF_RTARGET))
    return false;

  bool changed = false;

  // start from the largest cascade
  for (int cascadeNo = NUM_CLIPMAP_SHADOW_CASCADES - 1; cascadeNo >= 0; cascadeNo--)
  {
    regions[cascadeNo].clear();

    // vars
    ToroidalHelper &torHelper = torHelpers[cascadeNo];
    float toroidalWorldSize = clipmapShadowWorldSize[cascadeNo];
    float texelSize = clipmapShadowWorldSize[cascadeNo] / (float)clipmapShadowSize;

    IPoint2 newTexelOrigin = torHelpers[cascadeNo].curOrigin;

    IPoint2 center_pos;
    center_pos.x = 4 * floorf(view_pos.x / (4.0f * texelSize));
    center_pos.y = 4 * floorf(view_pos.z / (4.0f * texelSize));

    const int pixelTreshold = 128;

    if ((abs(torHelpers[cascadeNo].curOrigin.x - center_pos.x) < pixelTreshold) &&
        (abs(torHelpers[cascadeNo].curOrigin.y - center_pos.y) < pixelTreshold))
      continue;
    else
    {
      if (abs(torHelpers[cascadeNo].curOrigin.x - center_pos.x) > abs(torHelpers[cascadeNo].curOrigin.y - center_pos.y))
        newTexelOrigin.x = center_pos.x;
      else
        newTexelOrigin.y = center_pos.y;
    }

    // if change of postion is dramatic, update both axes
    if (max(abs(torHelpers[cascadeNo].curOrigin.x - newTexelOrigin.x), abs(torHelpers[cascadeNo].curOrigin.y - newTexelOrigin.y)) >
        pixelTreshold * 2)
      newTexelOrigin = center_pos;

    ToroidalGatherCallback cb(regions[cascadeNo]);
    toroidal_update(newTexelOrigin, torHelper, 0.33f * clipmapShadowSize, cb);

    Point2 worldSpaceOrigin = point2(torHelper.curOrigin) * texelSize;

    worldToToroidal[cascadeNo] = Color4(1.f / toroidalWorldSize, 1.f / toroidalWorldSize,
      0.5f - worldSpaceOrigin.x / toroidalWorldSize, 0.5f - worldSpaceOrigin.y / toroidalWorldSize);

    ShaderGlobal::set_color4(worldToClipmapShadowVarId[cascadeNo], worldToToroidal[cascadeNo]);

    uvOffset[cascadeNo] = -point2((torHelper.mainOrigin - torHelper.curOrigin) % torHelper.texSize) / torHelper.texSize;

    for (int i = 0; i < regions[cascadeNo].size(); ++i)
    {
      const ToroidalQuadRegion &reg = regions[cascadeNo][i];
      append_items(quadRegions[cascadeNo], 1, &reg);
      changed = true;
    }
  }

  ShaderGlobal::set_texture(clipmapShadowTexVarId, clipmapShadowTex.getTexId());
  ShaderGlobal::set_color4(clipmap_shadow_near_far_tc_offsetVarId, Color4(uvOffset[1].x, uvOffset[1].y, uvOffset[0].x, uvOffset[0].y));

  if (!changed)
    return false;
  else
  {
    // save states
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;

    // update all generated regions for all cascades
    for (int cascadeNo = NUM_CLIPMAP_SHADOW_CASCADES - 1; cascadeNo >= 0; cascadeNo--)
    {
      // vars
      float texelSize = clipmapShadowWorldSize[cascadeNo] / (float)clipmapShadowSize;

      if (quadRegions[cascadeNo].size() == 0)
        continue;

      d3d::set_render_target(clipmapShadowTex.getTex2D(), 0);
      d3d::settm(TM_VIEW, look_down_vtm);

      for (int i = 0; i < quadRegions[cascadeNo].size(); ++i)
      {
        ToroidalQuadRegion &reg = quadRegions[cascadeNo][i];
        d3d::setview(clipmapShadowSize * cascadeNo + reg.lt.x, reg.lt.y, reg.wd.x, reg.wd.y, 0, 1);

        d3d::clearview(CLEAR_TARGET, 0xFFFFFFFF, 1.f, 0);
        BBox2 boxReg(point2(reg.texelsFrom) * texelSize, point2(reg.texelsFrom + reg.wd) * texelSize);
        TMatrix4 proj = matrix_ortho_off_center_lh(boxReg[0].x, boxReg[1].x, boxReg[1].y, boxReg[0].y, min_height, max_height);
        d3d::settm(TM_PROJ, &proj);

        ShaderGlobal::setBlock(clipmap_shadowsBlockId, ShaderGlobal::LAYER_FRAME);

        // increase box size for culling to avoid absent shadows from ri outside original box
        const float boxExpand = 100.0;
        boxReg[0] -= Point2(1, 1) * boxExpand;
        boxReg[1] += Point2(1, 1) * boxExpand;

        rendinst::render::renderRIGenShadowsToClipmap(boxReg, cascadeNo);
      }
      quadRegions[cascadeNo].clear();
    }
  }

  d3d::resource_barrier({clipmapShadowTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  return true;
}
