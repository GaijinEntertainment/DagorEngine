// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// Private shared declarations for the land-class / virtual-texture rendering side
// of the land-mesh renderer (lmeshRendererLandClasses.cpp). Carved out of
// lmeshRendererCommon.h; this is the intended future home of LandVtexRenderer
// state (land-class textures, the splatting shader table, the trivial quad).
//
// Include this header AFTER the translation unit's own drv/3d and shader includes.

#include <landMesh/lmeshTools.h> // NUM_TEXTURES_STACK, Texture, ArrayTexture (no dependency on lmeshRenderer.h)
#include <landMesh/landClass.h>  // LandClassType, LC_SIMPLE
#include <shaders/dag_shaders.h>
#include <shaders/dag_overrideStateId.h>
#include <generic/dag_span.h> // dag::ConstSpan
#include <drv/3d/dag_decl.h>  // Sbuffer, Vbuffer
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
#include <EASTL/utility.h>
#include <EASTL/vector.h>

extern Vbuffer *one_quad;

// Texture id + sampler pair used by the land-class textures. Was a nested struct of
// LandMeshRenderer; relocated here as a standalone struct so the moved land-class state
// (LCTexturesLoaded, LandVtexRenderer megaDetails) does not depend on LandMeshRenderer.
struct TidSamplerWithoutMipbiasPair
{
  TEXTUREID tid = BAD_TEXTUREID;
  d3d::SamplerHandle sampler = {};
};

struct ShaderInfo
{
  eastl::unique_ptr<ShaderMaterial> material;
  ShaderElement *elem = 0;
  int vs_const_offset = -1, ps_const_offset = -1;
  int lc_detail_const_offset = -1, lc_textures_sampler = -1;
  int lc_ps_details_cb_register = -1;
  uint64_t nameHash = 0; // 64-bit FNV1 of the shader name; custom entries are deduped by it (see insert_land_shader)
};
extern eastl::vector<ShaderInfo> landclassShader;
extern eastl::vector<uint64_t> landclassShaderFailed; // name hashes whose material build failed; looked up as LC_SIMPLE

struct LCTexturesLoaded
{
  LandClassType lcType = LC_SIMPLE;

  TidSamplerWithoutMipbiasPair colorMap;
  TidSamplerWithoutMipbiasPair grassMask;
  Sbuffer **detailsCB = 0;
  SmallTab<TidSamplerWithoutMipbiasPair, MidmemAlloc> lcTextures;
  carray<Tab<int16_t>, NUM_TEXTURES_STACK> lcDetailTextures;
  SmallTab<Point4, TmpmemAlloc> lcDetailParamsVS; // just CB
  SmallTab<Point4, TmpmemAlloc> lcDetailParamsPS; // just CB
  TidSamplerWithoutMipbiasPair flowmapTex;

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
  if (sampler != d3d::SamplerHandle::Invalid)
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
void resolve_lmesh_shader_constants();

// Sampler info for land-class textures (rendered on clipmap / grass mask, with a
// separate sampler to avoid extra mip bias). Stateless utility.
d3d::SamplerInfo get_texture_sampler_info(TEXTUREID tid);
d3d::SamplerInfo get_texture_sampler_info(TEXTUREID tid, float anisotropic_max);
} // namespace landmesh

// Owns the land-class / virtual-texture data and rendering methods carved out of
// LandMeshRenderer. Owned by LandMeshManager; LandMeshRenderer stores a non-owning pointer.
class LandVtexRenderer
{
public:
  static constexpr int DET_TEX_NUM = ::DET_TEX_NUM; // detail textures per cell; single definition in landMesh/lmeshTools.h

  LandVtexRenderer(LandMeshManager &provider, dag::ConstSpan<LandClassDetailTextures> land_classes, int biome_land_class_idx,
    TEXTUREID vert_tex_id, TEXTUREID vert_nm_tex_id, TEXTUREID vert_det_tex_id, TEXTUREID tile_tex, d3d::SamplerHandle tile_smp,
    real tile_x, real tile_y);
  ~LandVtexRenderer();

  void prepareLandClasses(LandMeshManager &provider);
  void updateCustomSamplers(LandMeshManager &provider);
  void setCustomLcTextures();
  void renderLandclasses(CellState &curState, bool useFilter = false, LandClassType lcFilter = LC_SIMPLE);
  bool reloadGrassMaskTex(int land_class_id, TEXTUREID newGrassMaskTexId);
  void resetTextures();
  void evictSplattingData();

  Tab<LandClassDetailTextures> &getLandClasses() { return landClasses; }
  TEXTUREID getDetailTileTex() const { return tileTexId; }
  d3d::SamplerHandle getDetailTileSmp() const { return tileTexSmp; }
  // Read accessors for the fields the LandMeshRenderer driver consumes (lmeshRenderer.cpp),
  // so field representation stays changeable without touching the driver.
  const Tab<LCTexturesLoaded> &getLandClassesLoaded() const { return landClassesLoaded; }
  float getDetMapTcScale() const { return detMapTcScale; }
  float getDetMapTcOfs() const { return detMapTcOfs; }
  Sbuffer *getPhysmatIdsBuf() const { return physmatIdsBuf; }
  TEXTUREID getVertTexId() const { return vertTexId; }

  // Additive blend override used while splatting land classes into the clipmap.
  shaders::OverrideStateId setStateBlend();

private:
  // Land-class / virtual-texture data moved out of LandMeshRenderer. Private: the driver reads
  // it through the accessors above, external code through the LandMeshRenderer forwarders.
  Tab<LandClassDetailTextures> landClasses;
  Tab<LCTexturesLoaded> landClassesLoaded;
  carray<SmallTab<TidSamplerWithoutMipbiasPair, MidmemAlloc>, NUM_TEXTURES_STACK> megaDetails;
  carray<eastl::pair<TidSamplerWithoutMipbiasPair, ArrayTexture *>, NUM_TEXTURES_STACK> megaDetailsArray;
  int biomeLandClassIdx = -1;
  TEXTUREID vertTexId = BAD_TEXTUREID, vertNmTexId = BAD_TEXTUREID, vertDetTexId = BAD_TEXTUREID;
  TEXTUREID tileTexId = BAD_TEXTUREID;
  d3d::SamplerHandle tileTexSmp = {};
  real tileXSize = 0, tileYSize = 0;
  Sbuffer *physmatIdsBuf = nullptr;
  float detMapTcScale = 0, detMapTcOfs = 0;
  bool has_detailed_land_classes = false;

  // Own override-state cache (separate from LandMeshRenderer's), shared logic via landmesh::set_override.
  landmesh::OverrideStateCache overrideStateMap;
};
