// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// Private shared state for the LandMeshRenderer implementation, split across
// lmeshRenderer.cpp (core cell/geometry rendering) and lmeshRendererLandClasses.cpp
// (land-class / virtual-texture rendering and texture preparation).
//
// The definitions of the extern data below live in lmeshRenderer.cpp.
// Include this header AFTER the translation unit's own drv/3d and shader includes.

#include <landMesh/lmeshRenderer.h>
#include <landMesh/lmeshManager.h>
#include <landMesh/landClass.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_sampler.h>
#include <3d/dag_texMgr.h>
#include <math/dag_color.h>
#include <math/dag_Point4.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <EASTL/hash_map.h>
#include <EASTL/string.h>

// Cached shader variables. Defined (one ShaderVariableInfo each) in lmeshRenderer.cpp.
namespace var
{
#define LMESH_VAR_LIST               \
  VAR(lmesh_rendering_mode)          \
  VAR(num_detail_textures)           \
  VAR(world_view_pos)                \
  VAR(vertical_tex)                  \
  VAR(vertical_nm_tex)               \
  VAR(vertical_det_tex)              \
  VAR(indicestexDimensions)          \
  VAR(decals_detail_overrideSampler) \
  VAR(bottom)                        \
  VAR(skip_detail_lod_calculation)   \
  VAR(landmesh_debug_cells_scale)
#define VAR(a) extern ShaderVariableInfo a;
LMESH_VAR_LIST
#undef VAR
} // namespace var

// Shader-constant register slots. Resolved once from int shader vars in the ctor
// (kept at the listed default when the slot is absent from the shader).
// Defined (one int each) in lmeshRenderer.cpp.
#define LMESH_CONST_LIST                           \
  /* VS constants */                               \
  VAR(lmesh_vs_const__pos_to_world, -1)            \
  VAR(lmesh_vs_const__mul_offset_base, -1)         \
  VAR(lmesh_vs_weight_params, -1)                  \
  /* PS constants */                               \
  VAR(lmesh_ps_const__mirror_scale, -1)            \
  VAR(lmesh_ps_weight_params, -1)                  \
  VAR(lmesh_ps_const__weight, 30)                  \
  VAR(lmesh_ps_const__invtexturesizes, 28)         \
  VAR(lmesh_ps_const__lcdetailstexsizes, -1)       \
  VAR(lmesh_ps_const__lctexsizes, -1)              \
  VAR(lmesh_ps_const__detailarraytexsizes, -1)     \
  VAR(lmesh_ps_const__bumpscales, -1)              \
  VAR(lmesh_ps_const__puddlescales, -1)            \
  VAR(lmesh_ps_const__displacementmin, -1)         \
  VAR(lmesh_ps_const__displacementmax, -1)         \
  VAR(lmesh_ps_const__compdiffusescales, -1)       \
  VAR(lmesh_ps_const__finalcolormul, -1)           \
  VAR(lmesh_ps_const__randomFlowmapParams, -1)     \
  VAR(lmesh_ps_const__water_decal_bump_scale, -1)  \
  VAR(lmesh_ps_const_land_detail_array_slices, -1) \
  /* samplers */                                   \
  VAR(lmesh_sampler__land_detail_map, -1)          \
  VAR(lmesh_sampler__land_detail_map2, -1)         \
  VAR(lmesh_sampler__land_detail_tex1, -1)         \
  VAR(lmesh_sampler__land_detail_ntex1, -1)        \
  VAR(lmesh_sampler__flowmap_tex, -1)              \
  VAR(lmesh_sampler__max_used_sampler, -1)         \
  VAR(lmesh_sampler__land_detail_array1, -1)       \
  VAR(lmesh_physmats__buffer_idx, -1)

#define VAR(a, def) extern int a;
LMESH_CONST_LIST
#undef VAR

// Resolve an int shader constant by name, falling back to def when absent.
int get_shader_int_constant(const char *name, int def);

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

struct CellState
{
public:
  carray<ShaderMesh::RElem *, LandMeshManager::LOD_COUNT> lmeshElems;
  struct
  {
    uint32_t border0 : 3;
    uint32_t border1 : 3;
    uint32_t above0 : 3;
    uint32_t above1 : 3;
    uint32_t below0 : 3;
    uint32_t below1 : 3;
    uint32_t deep0 : 3;
    uint32_t deep1 : 3;
  };
  static constexpr int MAX_ELEMS_PER_CELL = 4;
  carray<carray<uint8_t, MAX_ELEMS_PER_CELL>, LandMeshManager::LOD_COUNT> landBottom; // todo: 4 bits is enough
  void init()
  {
    mirrorState.x = mirrorState.y = 0;
    memset(lcIds.data(), 0xFF, data_size(lcIds));
    mem_set_0(landBottom);
    mem_set_0(lmeshElems);
    map1 = map2 = NULL;
    numDetailTextures = 0;
    trivial = true;
  }
  CellState() { init(); }
  BaseTexture *map1, *map2;
  enum
  {
    NOT_MIRRORED = 0,
    MIRRORED_POS = 1,
    MIRRORED_NEG = 2,
    MIRRORED_TOTAL
  }; // actually, 3 are unused, as it is impossible to be both neg and pos
  Color4 mul_offset[MIRRORED_TOTAL][MIRRORED_TOTAL][8];
  struct
  {
    uint8_t x, y;
  } mirrorState;
  int numDetailTextures;
  carray<uint8_t, LandMeshRenderer::DET_TEX_NUM> lcIds;

  Point4 posToWorld[2];
  Color4 detMapTc;
  Color4 invTexSizes[2];

  bool trivial;
  static inline void initMirrorScaleState()
  {
    // currentMirrorScaleState.x = currentMirrorScaleState.z = 1;
    // d3d::set_ps_const1(lmesh_ps_const__mirror_scale, currentMirrorScaleState.x, currentMirrorScaleState.z, 0, 0);
  }
  inline void setBase() const
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = d3d::AddressMode::Clamp;
    d3d::SamplerHandle clampSampler = d3d::request_sampler(smpInfo);
    d3d::settex(lmesh_sampler__land_detail_map, map1);
    d3d::set_sampler(STAGE_PS, lmesh_sampler__land_detail_map, clampSampler);
    d3d::settex(lmesh_sampler__land_detail_map2, map2);
    d3d::set_sampler(STAGE_PS, lmesh_sampler__land_detail_map2, clampSampler);
    ShaderGlobal::set_int(var::num_detail_textures, numDetailTextures);
    // d3d::set_vs_const(lmesh_vs_const__mul_offset_base+8, (const float*)detMapTcSet, 1);
  }
  inline void set(bool grass_mask, dag::ConstSpan<LCTexturesLoaded> lc, bool force_trivial, bool water_decals,
    Sbuffer *physmatIdsBuf = NULL) const
  {
    if (!water_decals)
      setBase();
    uint32_t physmats[4 * 7];
    // fixme: fill with nulls or leave samplers untouched?
    if ((force_trivial || trivial || grass_mask) && !water_decals)
    {
      for (int i = 0; i < numDetailTextures; ++i) // LandMeshRenderer::DET_TEX_NUM
      {
        if (lcIds[i] < lc.size())
        {
          LandMeshRenderer::TidSamplerWithoutMipbiasPair tex = grass_mask ? lc[lcIds[i]].grassMask : lc[lcIds[i]].colorMap;
          mark_managed_tex_lfu(tex.tid);
          d3d::set_tex(STAGE_PS, lmesh_sampler__land_detail_tex1 + i, D3dResManagerData::getBaseTex(tex.tid));
          d3d::set_sampler(STAGE_PS, lmesh_sampler__land_detail_tex1 + i, tex.sampler);
          if (grass_mask)
          {
            physmats[i * 4 + 0] = lc[lcIds[i]].physmatIDs.x;
            physmats[i * 4 + 1] = lc[lcIds[i]].physmatIDs.y;
            physmats[i * 4 + 2] = lc[lcIds[i]].physmatIDs.z;
            physmats[i * 4 + 3] = lc[lcIds[i]].physmatIDs.w;
            lc[lcIds[i]].lastUsedGrassMask = true;
          }
        }
        else
          d3d::set_tex(STAGE_PS, lmesh_sampler__land_detail_tex1 + i, nullptr);
      }
      d3d::set_vs_const(lmesh_vs_const__mul_offset_base, (float *)mul_offset[mirrorState.x][mirrorState.y], 8);
      // if (normalmap)
      if (!grass_mask && lmesh_ps_const__invtexturesizes >= 0)
        d3d::set_ps_const(lmesh_ps_const__invtexturesizes, (float *)&invTexSizes[0].r, 2);
      if (grass_mask && lmesh_physmats__buffer_idx > 0 && physmatIdsBuf)
      {
        physmatIdsBuf->updateDataWithLock(0, 4 * 7 * 4, &physmats[0], VBLOCK_WRITEONLY | VBLOCK_DISCARD);
        d3d::set_buffer(STAGE_PS, lmesh_physmats__buffer_idx, physmatIdsBuf);
      }
    }
  }

  void prepareMirrorMul(float minX, float maxX, float minZ, float maxZ)
  {
    for (int x = 0; x < MIRRORED_TOTAL; ++x)
      for (int y = 0; y < MIRRORED_TOTAL; ++y)
      {
        for (int j = 0; j < 8; ++j)
          mul_offset[x][y][j] = mul_offset[0][0][j];
        Point2 mulScale(1.f, 1.f);
        Point2 offset(0.f, 0.f);
        // original tc = mul*(worldPos+add); => mul = mul, offset=mul*add
        // mirrored tc = -mul*worldPos+2*mul*wrapPt+mul*add;
        // mirroredMul = -mul, mirroredOffset = mul*add + mul*2*(wrapPt) => mirroredOffset = offset + -mirroredMul*2*(wrapPt)
        // check:
        // original tc at wrap Point = mul*wrapPt+offset;
        // mirrored tc at wrap Point = -mul*wrapPt+offset+2*mul*wrapPt;
        if (x)
        {
          mulScale.x = -1.f;
          if (x == MIRRORED_POS)
            offset.x = 2.f * maxX;
          if (x == MIRRORED_NEG)
            offset.x = 2.f * minX;
        }
        if (y)
        {
          mulScale.y = -1.f;
          if (y == MIRRORED_POS)
            offset.y = 2.f * maxZ;
          if (y == MIRRORED_NEG)
            offset.y = 2.f * minZ;
        }
        for (int j = 0; j < 4; ++j)
          mul_offset[x][y][j] = Color4(mulScale.x * mul_offset[x][y][j][0], mulScale.y * mul_offset[x][y][j][1],
            mulScale.x * mul_offset[x][y][j][2], mulScale.y * mul_offset[x][y][j][3]);
        for (int j = 4; j < 8; ++j)
          mul_offset[x][y][j] -= Color4(offset.x * mul_offset[x][y][j - 4][0], offset.y * mul_offset[x][y][j - 4][1],
            offset.x * mul_offset[x][y][j - 4][2], offset.y * mul_offset[x][y][j - 4][3]);
      }
  }
};
