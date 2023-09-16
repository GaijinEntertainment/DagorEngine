#pragma once

#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstGenGpuObjects.h>
#include <rendInst/riShaderConstBuffers.h>
#include "riGen/riGenData.h"
#include "riGen/riGenExtra.h"

#include <generic/dag_smallTab.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point3.h>
#include <shaders/dag_rendInstRes.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_indirectDrawcallsBuffer.h>
#include <3d/dag_dynLinearAllocBuffer.h>


#define MIN_RI_SHADOW_SUN_Y 0.3f
// fixme!: should be made 32

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

#define RENDINST_FLOAT_POS        1 // for debug switching between floats and halfs
#define RENDER_ELEM_SIZE          (RENDINST_FLOAT_POS ? RENDER_ELEM_SIZE_UNPACKED : RENDER_ELEM_SIZE_PACKED)
#define RENDER_ELEM_SIZE_PACKED   8
#define RENDER_ELEM_SIZE_UNPACKED 16

class Sbuffer;
typedef Sbuffer Vbuffer;

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

namespace rendinstgenrender
{
// on disc we store in short4N
#define ENCODED_RENDINST_RESCALE (32767. / 256)

extern shaders::UniqueOverrideStateId afterDepthPrepassOverride;
extern IndirectDrawcallsBuffer<DrawIndexedIndirectArgs> indirectDrawCalls;
extern DynLinearAllocBuffer<rendinst::PerInstanceParameters> indirectDrawCallIds;

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

extern int dynamicImpostorTypeVarId, dynamicImpostorBackViewDepVarId, dynamicImpostorBackShadowVarId;
extern int dynamicImpostorViewXVarId, dynamicImpostorViewYVarId;

extern int dynamic_impostor_texture_const_no;
extern float rendinst_ao_mul;

extern float globalDistMul;
extern float globalCullDistMul;
extern float settingsDistMul;
extern float settingsMinCullDistMul;
extern float lodsShiftDistMul;
extern float additionalRiExtraLodDistMul;
extern float riExtraLodsShiftDistMul;
extern float riExtraMulScale;
extern bool forceImpostors;

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
    // translucencyTexId(BAD_TEXTUREID), translucency(NULL),
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

inline void set_no_impostor_tex() { d3d::settex(dynamic_impostor_texture_const_no, NULL); }


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
    shadowImpostorTexture(NULL), shadowImpostorTextureId(BAD_TEXTUREID), impostorDataOffsetCache(0)
  {
    const Point3 &sphereCenter = res->bsphCenter;
    sphereRadius = res->bsphRad + sqrtf(sphereCenter.x * sphereCenter.x + sphereCenter.z * sphereCenter.z);
    cylinderRadius = sphereRadius;
    sphCenterY = sphereCenter.y;
    rendinstGlobalShadowTexId = rendinstClipmapShadowTexId = BAD_TEXTUREID;
    rendinstClipmapShadowTex = rendinstGlobalShadowTex = NULL;
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

  void setImpostor(RiShaderConstBuffers &cb, bool forShadow, BaseTexture *baked_preshadow = nullptr) const
  {
    if (!hasImpostor())
    {
      setNoImpostor(cb);
      return;
    }
    if (forShadow)
    {
      // ShaderGlobal::set_texture_fast(dynamicImpostorTextureVarId, rendinstGlobalShadowTexId);
      d3d::settex(dynamic_impostor_texture_const_no, rendinstGlobalShadowTex);
      setShadowImpostorBoundingSphere(cb);
    }
    else
    {
      if (baked_preshadow != nullptr)
      {
        d3d::settex(dynamic_impostor_texture_const_no, baked_preshadow);
      }
      else
      {
        for (int j = 0; j < impostor.tex.size(); ++j)
        {
          if (!impostor.tex[j].getBaseTex())
            break;
          d3d::settex(dynamic_impostor_texture_const_no + j, impostor.tex[j].getBaseTex());
        }
      }
      setDynamicImpostorBoundingSphere(cb);
    }
    ShaderElement::invalidate_cached_state_block();
  }
};

}; // namespace rendinstgenrender

enum
{
  VIS_HAS_OPAQUE = 1,
  VIS_HAS_TRANSP = 2,
  VIS_HAS_IMPOSTOR = 4
};
struct RenderRanges
{
  void init()
  {
    mem_set_0(endCell);
    mem_set_0(startCell);
    vismask = 0;
  }
  RenderRanges() { init(); }

  carray<uint16_t, rendinstgenrender::MAX_LOD_COUNT_WITH_ALPHA> startCell;
  carray<uint16_t, rendinstgenrender::MAX_LOD_COUNT_WITH_ALPHA> endCell;
  uint8_t vismask;
  bool hasOpaque() const { return vismask & VIS_HAS_OPAQUE; }
  bool hasTransparent() const { return vismask & VIS_HAS_TRANSP; }
  bool hasImpostor() const { return vismask & VIS_HAS_IMPOSTOR; }
  bool hasCells(int lodI) const { return endCell[lodI] > startCell[lodI]; }
};

constexpr uint32_t INVALID_VB_EXTRA_GEN = 0;
struct RiGenExtraVisibility
{
  static constexpr int LARGE_LOD_CNT = rendinst::RiExtraPool::MAX_LODS / 2; // sort only first few lods

  int forcedExtraLod = -1;
  struct Order
  {
    int d;
    uint32_t id;
    bool operator<(const Order &rhs) const { return d < rhs.d; }
  };
  struct UVec2
  {
    uint32_t x, y;
  };
  SmallTab<vec4f> largeTempData;                      // temp array
  SmallTab<SmallTab<Order>> riexLarge[LARGE_LOD_CNT]; // temp array
  SmallTab<SmallTab<vec4f>> riexData[rendinst::RiExtraPool::MAX_LODS];
  SmallTab<float> minSqDistances[rendinst::RiExtraPool::MAX_LODS];
  SmallTab<IPoint2> vbOffsets[rendinst::RiExtraPool::MAX_LODS]; // after copied to buffer
  SmallTab<unsigned short> riexPoolOrder;

  rendinst::VbExtraCtx *vbexctx = nullptr;
  volatile uint32_t vbExtraGeneration = INVALID_VB_EXTRA_GEN;

  uint32_t riExLodNotEmpty = 0;
  int riexInstCount = 0;
};

struct RiGenVisibility
{
  struct SubCellRange
  {
    uint16_t ofs, cnt;
    SubCellRange() {}
    SubCellRange(uint16_t ofs_, uint16_t cnt_) : ofs(ofs_), cnt(cnt_) {}
  };
  struct CellRange
  {
    int startVbOfs;
    // if ever assert appeared - change to bitfield!
    uint16_t startSubCell;    // in subCells array. Since subCells are not more than 64*MAX_VISIBLE_CELLS, 64*256 < 64k, in each render
                              // range. However, it really depends on render ranges count.
    uint16_t startSubCellCnt; // in subCells array, always <=64
    // change to it if assert!
    // uint32_t startSubCell:25;//
    // uint32_t startSubCellCnt:7;// in subCells array, always <=64.
    uint16_t x, z;
    CellRange() {}
    CellRange(int x_, int z_, int startVbOfs_, int startSubCell_, int startSubCellCnt_ = 0) :
      x(x_), z(z_), startVbOfs(startVbOfs_), startSubCell(startSubCell_), startSubCellCnt(startSubCellCnt_)
    {}
  };

  Tab<SubCellRange> subCells;
  Tab<RenderRanges> renderRanges;
  carray<Tab<CellRange>, rendinstgenrender::MAX_LOD_COUNT_WITH_ALPHA> cellsLod;
  carray<int, rendinstgenrender::MAX_LOD_COUNT_WITH_ALPHA + 1> instNumberCounter;

  int forcedLod = -1;
  enum
  {
    SKIP_NO_ATEST = 0,
    SKIP_ATEST = 1,
    RENDER_ALL = 2
  };
  int atest_skip_mask = RENDER_ALL;
  rendinst::VisibilityRenderingFlags rendering = rendinst::VisibilityRenderingFlag::All;
  bbox3f cameraBBox;
  int8_t stride = 0;
  uint8_t vismask = 0;

  RiGenExtraVisibility riex;

  bool hasCells(int ri, int lodI) const { return renderRanges[ri].hasCells(lodI); }
  void startRenderRange(int rri)
  {
    for (int i = 0; i < cellsLod.size(); ++i)
    {
      renderRanges[rri].startCell[i] = cellsLod[i].size();
      renderRanges[rri].endCell[i] = 0;
    }
  }
  void endRenderRange(int rri)
  {
    for (int i = 0; i < cellsLod.size(); ++i)
      renderRanges[rri].endCell[i] = cellsLod[i].size();
  }

  IMemAlloc *getmem() { return dag::get_allocator(renderRanges); }

  bool hasOpaque() const { return vismask & VIS_HAS_OPAQUE; }
  bool hasTransparent() const { return vismask & VIS_HAS_TRANSP; }
  bool hasImpostor() const { return vismask & VIS_HAS_IMPOSTOR; }

  RiGenVisibility(IMemAlloc *allocator) : renderRanges(allocator), subCells(allocator)
  {
    subCells.reserve(4096);
    mem_set_0(instNumberCounter);
    for (int i = 0; i < cellsLod.size(); ++i)
      dag::set_allocator(cellsLod[i], allocator);
  }
  void resizeRanges(int cnt, int mul)
  {
    if (renderRanges.size() != cnt)
    {
      clear_and_resize(crossDissolveRange, cnt);
      mem_set_0(crossDissolveRange);
      clear_and_resize(renderRanges, cnt);
    }
    mem_set_0(renderRanges); // for (int i = 0; i < renderRanges.size(); ++i) renderRanges[i].init();
    for (int i = 0; i < cellsLod.size(); ++i)
    {
      cellsLod[i].reserve(cnt * (i ? (mul << 1) : mul));
      cellsLod[i].clear();
    }
  }

  void closeRanges(int lodI) // shrink
  {
    CellRange &cell = cellsLod[lodI].back();
    int at = cell.startSubCell + cell.startSubCellCnt;
    erase_items(subCells, at, subCells.size() - at);
  }
  unsigned getOfs(int lodI, int ci, int ri, int _stride) const
  {
    const CellRange &cell = cellsLod[lodI][ci];
    G_ASSERT(ri < cell.startSubCellCnt);
    return cell.startVbOfs + subCells[cell.startSubCell + ri].ofs * _stride;
  }

  void addRange(int lodI, unsigned ofs, unsigned sz)
  {
    CellRange &cell = cellsLod[lodI].back();
    ofs -= cell.startVbOfs;
    ofs /= stride;
    sz /= stride;
    int endSubCell = int(cell.startSubCell + cell.startSubCellCnt);
    int lastSubCell = endSubCell - 1;
    if (cell.startSubCellCnt && subCells[lastSubCell].ofs + subCells[lastSubCell].cnt == ofs)
    {
      subCells[lastSubCell].cnt += sz;
      G_ASSERT(subCells[lastSubCell].cnt <= 0xFFFF);
      return;
    }
    subCells[endSubCell] = SubCellRange(ofs, sz);
    cell.startSubCellCnt++;
  }
  G_STATIC_ASSERT(rendinst::MAX_LOD_COUNT >= 2);
  // original lods + 2 dissolve for impostor + alpha
  static constexpr int PER_INSTANCE_LODS = rendinst::MAX_LOD_COUNT + 3;
  enum
  {
    PI_ALPHA_LOD = PER_INSTANCE_LODS - 1,
    PI_IMPOSTOR_LOD = PER_INSTANCE_LODS - 2,
    PI_DISSOLVE_LOD1 = PER_INSTANCE_LODS - 4,
    PI_DISSOLVE_LOD0 = PER_INSTANCE_LODS - 3,
    PI_LAST_MESH_LOD = PER_INSTANCE_LODS - 5
  };
  carray<Tab<IPoint2>, PER_INSTANCE_LODS> perInstanceVisibilityCells; // todo: replace with list of cells for each ri_idx
  carray<Tab<vec4f>, PER_INSTANCE_LODS> instanceData;                 // plus cross dissolve between 0 and 1 lod
  Tab<float> crossDissolveRange;
  int max_per_instance_instances = 0;

  void addTreeInstance(int ri_idx, int &internal_cell, vec4f instance_data, int lod)
  {
    if (internal_cell < 0)
    {
      internal_cell = perInstanceVisibilityCells[lod].size();
      perInstanceVisibilityCells[lod].push_back(IPoint2(ri_idx, instanceData[lod].size()));
    }
    instanceData[lod].push_back(instance_data);
  }

  int addTreeInstances(int ri_idx, int &internal_cell, const vec4f *instance_data, int cnt, int lod)
  {
    if (internal_cell < 0)
    {
      internal_cell = perInstanceVisibilityCells[lod].size();
      perInstanceVisibilityCells[lod].push_back(IPoint2(ri_idx, instanceData[lod].size()));
    }
    return append_items(instanceData[lod], cnt, instance_data);
  }
  void startTreeInstances()
  {
    for (int i = 0; i < perInstanceVisibilityCells.size(); ++i)
    {
      perInstanceVisibilityCells[i].clear();
      instanceData[i].clear();
    }
  }
  void closeTreeInstances()
  {
    for (int i = 0; i < perInstanceVisibilityCells.size(); ++i)
      if (perInstanceVisibilityCells[i].size())
        perInstanceVisibilityCells[i].push_back(IPoint2(-1, instanceData[i].size()));
  }

  int addCell(int lodI, int x, int z, int startVbOfs,
    int rangeCount = RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV / 2 + 1)
  {
    int startSubCell = append_items(subCells, rangeCount);
    G_ASSERT(subCells.size() <= 0xFFFF); // if ever appeared, change startSubCell, startSubCellCnt to bitfield (see above)
    int idx = cellsLod[lodI].size();
    cellsLod[lodI].push_back(CellRange(x, z, startVbOfs, startSubCell));
    return idx;
  }

  unsigned getCount(int lodI, int ci, int ri) const { return subCells[cellsLod[lodI][ci].startSubCell + ri].cnt; }
  unsigned getRangesCount(int lodI, int ci) const { return cellsLod[lodI][ci].startSubCellCnt; }
  int gpuObjectsCascadeId = -1;

  float riDistMul = 1.0f;
  void setRiDistMul(float mul) { riDistMul = mul; }
};

extern eastl::unique_ptr<gpu_objects::GpuObjects> gpu_objects_mgr;

namespace eastl
{
template <typename Count>
inline void uninitialized_default_fill_n(RiGenExtraVisibility::Order *, Count)
{}
template <typename Count>
inline void uninitialized_default_fill_n(RiGenExtraVisibility::UVec2 *, Count)
{}
} // namespace eastl
