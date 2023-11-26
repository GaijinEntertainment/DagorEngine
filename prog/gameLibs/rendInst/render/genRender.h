#pragma once

#include <rendInst/rendInstGenRender.h>
#include <rendInst/riShaderConstBuffers.h>
#include "riGen/riGenData.h"
#include "riGen/riGenExtra.h"
#include "render/extraRender.h"

#include <generic/dag_smallTab.h>
#include <3d/dag_tex3d.h>
#include <math/dag_Point3.h>
#include <shaders/dag_rendInstRes.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_multidrawContext.h>


//-V:SWITCH_STATES:501
struct RenderStateContext
{
  ShaderElement *curShader = nullptr;
  GlobalVertexData *curVertexData = nullptr;
  Vbuffer *curVertexBuf = nullptr;
  int curStride = 0;

  bool curShaderValid = false;
  RenderStateContext();
};

//-V:SWITCH_STATES_SHADER:501
#define SWITCH_STATES_SHADER()                                        \
  {                                                                   \
    if (elem.e != context.curShader)                                  \
    {                                                                 \
      context.curShader = elem.e;                                     \
      context.curShader->setReqTexLevel(15);                          \
      context.curShaderValid = context.curShader->setStates(0, true); \
    }                                                                 \
    if (!context.curShaderValid)                                      \
      continue;                                                       \
  }

#define SWITCH_STATES_VDATA()                                                                                                        \
  {                                                                                                                                  \
    if (elem.vertexData != context.curVertexData)                                                                                    \
    {                                                                                                                                \
      context.curVertexData = elem.vertexData;                                                                                       \
      if (context.curVertexBuf != elem.vertexData->getVB() || context.curStride != elem.vertexData->getStride())                     \
        d3d_err(d3d::setvsrc(0, context.curVertexBuf = elem.vertexData->getVB(), context.curStride = elem.vertexData->getStride())); \
    }                                                                                                                                \
  }

#define SWITCH_STATES()                          \
  {                                              \
    SWITCH_STATES_SHADER() SWITCH_STATES_VDATA() \
  }

inline constexpr bool RENDINST_FLOAT_POS = true; // for debug switching between floats and halfs

inline constexpr int RENDER_ELEM_SIZE_PACKED = 8;
inline constexpr int RENDER_ELEM_SIZE_UNPACKED = 16;
inline constexpr int RENDER_ELEM_SIZE = RENDINST_FLOAT_POS ? RENDER_ELEM_SIZE_UNPACKED : RENDER_ELEM_SIZE_PACKED;

inline constexpr float LOD0_DISSOLVE_RANGE = 0.0f;
inline constexpr float LOD1_DISSOLVE_RANGE = 6.0f;
inline constexpr float TOTAL_CROSS_DISSOLVE_DIST = LOD0_DISSOLVE_RANGE + LOD1_DISSOLVE_RANGE;

inline constexpr int DYNAMIC_IMPOSTOR_TEX_SHADOW_OFFSET = 2;

extern bool check_occluders;

// TODO: this is ugly :/
extern int globalRendinstRenderTypeVarId;
extern int globalTranspVarId;
extern int useRiGpuInstancingVarId;

// TODO: these should somehow be moved to riShaderConstBuffer.cpp
extern int perinstBuffNo;
extern int instanceBuffNo;

enum : int
{
  RENDINST_RENDER_TYPE_MIXED = 0,
  RENDINST_RENDER_TYPE_RIEX_ONLY = 1
};


inline int get_last_mip_idx(Texture *tex, int dest_size)
{
  TextureInfo ti;
  tex->getinfo(ti);
  int src = min(ti.w, ti.h);
  return min(tex->level_count() - 1, int(src > dest_size ? get_log2i(src / dest_size) : 0));
}

namespace rendinst::render
{
// on disc we store in short4N
#define ENCODED_RENDINST_RESCALE (32767. / 256)

extern shaders::UniqueOverrideStateId afterDepthPrepassOverride;
extern MultidrawContext<rendinst::PerInstanceParameters> multidrawContext;

extern bool use_ri_depth_prepass;
extern int normal_type;
extern void init_depth_VDECL();
extern void close_depth_VDECL();
extern Vbuffer *oneInstanceTmVb;
extern Vbuffer *rotationPaletteTmVb;
extern void cell_set_encoded_bbox(RiShaderConstBuffers &cb, vec4f origin, float grid2worldcellSz, float ht);
extern int globalFrameBlockId;
extern int rendinstSceneBlockId;
extern int rendinstSceneTransBlockId;
extern int rendinstDepthSceneBlockId;
extern int rendinstRenderPassVarId;
extern int rendinstShadowTexVarId;
extern Point3_vec4 dir_from_sun; // fixme it should be initialized to most glancing angle
extern bool per_instance_visibility;
extern bool per_instance_visibility_for_everyone;
extern bool per_instance_front_to_back;
extern bool use_tree_lod0_offset;
extern bool use_cross_dissolve;

extern int dynamicImpostorTypeVarId, dynamicImpostorBackViewDepVarId, dynamicImpostorBackShadowVarId;
extern int dynamicImpostorViewXVarId, dynamicImpostorViewYVarId;

extern int dynamic_impostor_texture_const_no;
extern float rendinst_ao_mul;

extern float globalDistMul;
extern float globalCullDistMul;
extern float settingsDistMul;
extern float settingsMinCullDistMul;
extern float lodsShiftDistMul;
extern bool forceImpostors;
extern bool use_color_padding;
extern bool vertical_billboards;

enum CoordType
{
  COORD_TYPE_TM = 0,
  COORD_TYPE_POS = 1,
  COORD_TYPE_POS_CB = 2
};
inline void setTextureInstancingUsage(bool) {} // deprecated
extern void setCoordType(CoordType type);
extern void setApexInstancing();
eastl::array<uint32_t, 2> getCommonImmediateConstants();

extern void initClipmapShadows();
extern void closeClipmapShadows();
extern void endRenderInstancing();
extern void startRenderInstancing();

enum
{
  IMP_COLOR = 0,
  IMP_NORMAL = 1,
  IMP_TRANSLUCENCY = 2,
  IMP_COUNT = 3
};

struct DynamicImpostor
{
  StaticTab<UniqueTex, IMP_COUNT> tex;
  int baseMip;
  float currentHalfWidth, currentHalfHeight;
  float impostorSphCenterY, shadowSphCenterY;
  float shadowImpostorWd, shadowImpostorHt;
  int renderMips;
  int numColorTexMips;
  float maxFacingLeavesDelta;
  SmallTab<vec4f, MidmemAlloc> points;
  SmallTab<vec4f, MidmemAlloc> shadowPoints; // for computing shadows only, then deleted. array of point, delta
  DynamicImpostor() :
    // translucencyTexId(BAD_TEXTUREID), translucency(nullptr),
    currentHalfWidth(1.f),
    currentHalfHeight(1.f),
    maxFacingLeavesDelta(0.f),
    impostorSphCenterY(0),
    shadowSphCenterY(0),
    renderMips(1),
    numColorTexMips(1),
    baseMip(0),
    shadowImpostorWd(0.f),
    shadowImpostorHt(0.f)
  {}
  void delImpostor()
  {
    for (int i = 0; i < tex.size(); ++i)
      tex[i].close();
    clear_and_shrink(points);
    clear_and_shrink(shadowPoints);
  }
  ~DynamicImpostor() { delImpostor(); }
};

inline void set_no_impostor_tex() { d3d::settex(dynamic_impostor_texture_const_no, nullptr); }


class RtPoolData
{
public:
  float sphereRadius, sphCenterY, cylinderRadius;
  float clipShadowWk, clipShadowHk, clipShadowOrigX, clipShadowOrigY;
  Texture *rendinstClipmapShadowTex, *rendinstGlobalShadowTex;
  TEXTUREID rendinstClipmapShadowTexId, rendinstGlobalShadowTexId;
  DynamicImpostor impostor;
  Texture *shadowImpostorTexture;
  TEXTUREID shadowImpostorTextureId;
  Color4 shadowCol0, shadowCol1, shadowCol2;
  carray<float, MAX_LOD_COUNT_WITH_ALPHA - 1> lodRange; // no range for alpha lod
  bool hadVisibleImpostor;
  int shadowImpostorUpdatePhase;
  uint64_t impostorDataOffsetCache;
  enum
  {
    HAS_NORMALMAP = 0x01,
    HAS_TRANSLUCENCY = 0x02,
    HAS_CLEARED_LIGHT_TEX = 0x04,
    HAS_TRANSITION_LOD = 0x08
  };

  uint32_t flags;
  bool hasUpdatedShadowImpostor;
  bool hasNormalMap() const { return flags & HAS_NORMALMAP; }
  bool hasTranslucency() const { return flags & HAS_TRANSLUCENCY; }
  bool hasTransitionLod() const { return flags & HAS_TRANSITION_LOD; }

  RtPoolData(RenderableInstanceLodsResource *res) :
    shadowImpostorTexture(nullptr), shadowImpostorTextureId(BAD_TEXTUREID), impostorDataOffsetCache(0)
  {
    const Point3 &sphereCenter = res->bsphCenter;
    sphereRadius = res->bsphRad + sqrtf(sphereCenter.x * sphereCenter.x + sphereCenter.z * sphereCenter.z);
    cylinderRadius = sphereRadius;
    sphCenterY = sphereCenter.y;
    rendinstGlobalShadowTexId = rendinstClipmapShadowTexId = BAD_TEXTUREID;
    rendinstClipmapShadowTex = rendinstGlobalShadowTex = nullptr;
    flags = 0;
    clipShadowWk = clipShadowHk = sphereRadius;
    clipShadowOrigX = clipShadowOrigY = 0;
    shadowCol0 = shadowCol1 = shadowCol2 = Color4(0, 0, 0, 0);
    hasUpdatedShadowImpostor = false;
    hadVisibleImpostor = true;
    shadowImpostorUpdatePhase = 0;
  }
  ~RtPoolData()
  {
    ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(rendinstClipmapShadowTexId, rendinstClipmapShadowTex);
    ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(rendinstGlobalShadowTexId, rendinstGlobalShadowTex);
    ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(shadowImpostorTextureId, shadowImpostorTexture);
  }
  bool hasImpostor() const { return impostor.tex.size() != 0; }

  inline void setImpostorParams(RiShaderConstBuffers &cb, float p0, float p1) const
  {
    cb.setBoundingSphere(p0, p1, sphereRadius, cylinderRadius, impostor.impostorSphCenterY);
  }

  inline void setDynamicImpostorBoundingSphere(RiShaderConstBuffers &cb) const
  {
    setImpostorParams(cb, impostor.currentHalfWidth, impostor.currentHalfHeight);
  }

  inline void setNoImpostor(RiShaderConstBuffers &cb) const { setImpostorParams(cb, 0, 0); }

  inline void setShadowImpostorBoundingSphere(RiShaderConstBuffers &cb) const
  {
    setImpostorParams(cb, impostor.shadowImpostorWd, impostor.shadowImpostorHt);
  }

  void setImpostor(RiShaderConstBuffers &cb, bool forShadow) const
  {
    if (!hasImpostor())
    {
      setNoImpostor(cb);
      return;
    }
    if (forShadow)
    {
      d3d::settex(dynamic_impostor_texture_const_no + DYNAMIC_IMPOSTOR_TEX_SHADOW_OFFSET, rendinstGlobalShadowTex);
      setShadowImpostorBoundingSphere(cb);
    }
    ShaderElement::invalidate_cached_state_block();
  }
};

}; // namespace rendinst::render
