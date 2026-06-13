// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// Private shared declarations for the land-class / virtual-texture rendering side
// of the land-mesh renderer (lmeshRendererLandClasses.cpp). Carved out of
// lmeshRendererCommon.h; this is the intended future home of LandVtexRenderer
// state (land-class textures, the splatting shader table, the trivial quad).
//
// Include this header AFTER the translation unit's own drv/3d and shader includes.

#include <landMesh/lmeshRenderer.h> // LandMeshRenderer::TidSamplerWithoutMipbiasPair, NUM_TEXTURES_STACK, Texture
#include <landMesh/landClass.h>     // LandClassType, LC_SIMPLE
#include <shaders/dag_shaders.h>
#include <shaders/dag_overrideStateId.h>
#include <EASTL/vector_map.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_sampler.h>
#include <3d/dag_texMgr.h>
#include <math/dag_color.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint4.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_carray.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <EASTL/hash_map.h>
#include <EASTL/string.h>

extern Vbuffer *one_quad;

struct ShaderInfo
{
  eastl::unique_ptr<ShaderMaterial> material;
  ShaderElement *elem = 0;
  int vs_const_offset = -1, ps_const_offset = -1;
  int lc_detail_const_offset = -1, lc_textures_sampler = -1;
  int lc_ps_details_cb_register = -1;
};
extern eastl::vector<ShaderInfo> landclassShader;
extern eastl::hash_map<eastl::string, int> shadersNames;

struct LCTexturesLoaded
{
  LandClassType lcType = LC_SIMPLE;

  LandMeshRenderer::TidSamplerWithoutMipbiasPair colorMap = {BAD_TEXTUREID, {}};
  LandMeshRenderer::TidSamplerWithoutMipbiasPair grassMask = {BAD_TEXTUREID, {}};
  Sbuffer **detailsCB = 0;
  SmallTab<LandMeshRenderer::TidSamplerWithoutMipbiasPair, MidmemAlloc> lcTextures;
  carray<Tab<int16_t>, NUM_TEXTURES_STACK> lcDetailTextures;
  SmallTab<Point4, TmpmemAlloc> lcDetailParamsVS; // just CB
  SmallTab<Point4, TmpmemAlloc> lcDetailParamsPS; // just CB
  LandMeshRenderer::TidSamplerWithoutMipbiasPair flowmapTex = {BAD_TEXTUREID, {}};

  float textureDimensions = 1;
  float weightMapNoiseScale = 0;
  Point2 invColormapSize = {1, 1};
  Point4 displacementMin = {0.0, 0.0, 0.0, 0.0};
  Point4 displacementMax = {0.0, 0.0, 0.0, 0.0};
  Point4 bumpScales = {1.0, 1.0, 1.0, 1.0};
  Point4 compatibilityDiffuseScales = {1.0, 1.0, 1.0, 1.0};
  Point4 randomFlowmapParams = {64.0, 0.0, 0.0, 0.0};
  Point4 flowmapMask = {1.0, 1.0, 1.0, 1.0};
  Point4 waterDecalBumpScale = {1.0, 0.0, 0.0, 0.0};
  Point4 weighMapMulOffset = {-1, -1, -1, -1};
  IPoint4 physmatIDs = {0, 0, 0, 0};
  Point4 puddleScales = {1.0, 1.0, 1.0, 1.0};
  Point4 finalColorMul = {1.0, 1.0, 1.0, 1.0};

  mutable bool lastUsedGrassMask = false;
};

// Optional PS-const setters: silently skip if the register slot was not found in the shader.
static inline void set_ps_const_opt(int reg, const Point4 &v)
{
  if (reg >= 0)
    d3d::set_ps_const(reg, &v.x, 1);
}
static inline void set_ps_const1_opt(int reg, float a, float b = 0, float c = 0, float d = 0)
{
  if (reg >= 0)
    d3d::set_ps_const1(reg, a, b, c, d);
}

// Sample tex width into texSizes[slot] when slot < 4. Used to build the
// `texsizes` PS constant from the first up-to-4 textures of a stack.
static inline void store_tex_width(Texture *tex, int slot, Point4 &out_widths)
{
  if (!tex || unsigned(slot) >= 4u)
    return;
  TextureInfo info;
  tex->getinfo(info);
  out_widths[slot] = info.w;
}

// Bind a managed texture (by id) and its sampler to a PS slot; returns the resolved base texture.
static inline Texture *bind_managed_tex_ps(int slot, TEXTUREID tid, d3d::SamplerHandle sampler)
{
  mark_managed_tex_lfu(tid);
  Texture *tex = D3dResManagerData::getBaseTex(tid);
  d3d::set_tex(STAGE_PS, slot, tex);
  d3d::set_sampler(STAGE_PS, slot, sampler);
  return tex;
}

class LandMeshManager;
struct CellState;

// Shared land-mesh override-state helpers. LandMeshRenderer (geometry passes) and
// LandVtexRenderer (land-class splatting) each keep their own cache map but share the
// create/lookup logic, so neither references the other for override state.
namespace landmesh
{
using OverrideStateCache = eastl::vector_map<uint32_t, eastl::vector<shaders::UniqueOverrideStateId>>;
shaders::OverrideStateId set_override(OverrideStateCache &cache, const shaders::OverrideState &new_state);
void reset_override(shaders::OverrideStateId &prev_state);

// Sampler info for land-class textures (rendered on clipmap / grass mask, with a
// separate sampler to avoid extra mip bias). Stateless utility.
d3d::SamplerInfo get_texture_sampler_info(TEXTUREID tid);
d3d::SamplerInfo get_texture_sampler_info(TEXTUREID tid, float anisotropic_max);
} // namespace landmesh
