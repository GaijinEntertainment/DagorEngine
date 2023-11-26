#pragma once

#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstDebris.h>
#include <rendInst/rendInstGenDamageInfo.h>
#include <rendInst/visibility.h>
#include <rendInst/renderPass.h>

#include <generic/dag_patchTab.h>
#include <generic/dag_smallTab.h>
#include <EASTL/bitvector.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_rwLock.h>
#include <shaders/dag_rendInstRes.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_roHugeHierBitMap2d.h>
#include <util/dag_stdint.h>
#include <util/dag_fastIntList.h>
#include <util/dag_simpleString.h>
#include <3d/dag_texIdSet.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_vecMathCompatibility.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <EASTL/vector_set.h>
#include <shaders/dag_linearSbufferAllocator.h>
#include <memory/dag_linearHeapAllocator.h>


#define RI_VERBOSE_OUTPUT (DAGOR_DBGLEVEL > 0)

class IGenLoad;
class Point4;
class Point3;
class RenderableInstanceLodsResource;
class CollisionResource;
class DynamicPhysObjectData;
struct Frustum;
struct RiGenVisibility;
typedef void *file_ptr_t;
namespace rendinst::gen::land
{
class AssetData;
}
namespace rendinst::render
{
class RtPoolData;
}
struct RenderStateContext;


struct RendInstGenData
{
  static constexpr int MAX_VISIBLE_CELLS = 256;
  static constexpr int SUBCELL_DIV = 8;
  enum
  {
    IMP_TEX_LOADING_LQ,
    IMP_TEX_LOADING_HQ,
    IMP_TEX_LOADING_DONE
  };
  typedef RoHugeHierBitMap2d<4, 3> HugeBitmask;

  struct EntPool
  {
    int baseOfs;
    int total;
    int avail;
    void *topPtr;
  };
  struct PregenEntPoolDesc
  {
    PatchablePtr<RenderableInstanceLodsResource> riRes;
    PatchablePtr<const char> riName;
    carray<E3DCOLOR, 2> colPair;
    uint32_t posInst : 1, paletteRotation : 1, _resv30 : 30;
    int32_t paletteRotationCount;
  };
  struct PregenEntCounter
  {
    uint32_t riResIdx;
    uint32_t riCount;

    void set(int idx, int cnt)
    {
      riResIdx = idx;
      riCount = cnt;
    }
  };
  struct PregenEntRtAdd
  {
    Tab<int16_t> dataStor;
    Tab<RendInstGenData::PregenEntCounter> cntStor;
    carray<int, SUBCELL_DIV * SUBCELL_DIV + 1> scCntIdx;
    bool needsUpdate;
  };

  struct CollisionResData
  {
    CollisionResource *collRes;
    void *handle;
  };

  struct DebrisProps
  {
    mat44f debrisTm;
    Tab<short> debrisPoolIdx;
    int delayedPoolIdx;
    int fxType;
    float fxScale;
    float damageDelay;
    float submersion;
    float inclination;
    float impulseOnExplosion;
    SimpleString fxTemplate;
    DebrisProps() : delayedPoolIdx(-1), fxType(-1), fxScale(1.f) {} //-V730
  };
  struct DebrisPool
  {
    DebrisProps *props;
    int resIdx;
    Tab<rendinst::DestroyedRi *> delayedRi;

    void clearDelayedRi() { clear_all_ptr_items(delayedRi); }
    DebrisPool() : resIdx(-1), props(nullptr) {}
  };

  struct DestrProps
  {
    DynamicPhysObjectData *res = nullptr; // Exists only on clients for now
    float destructionImpulse = 0.f;
    float collisionHeightScale = 1.0f;
    bool destructable = false;
    bool apexDestructible = false;
    SimpleString apexDestructionOptionsPresetName;
    bool isParent = false;
    bool destructibleByParent = false;
    int destroyNeighbourDepth = 1;
    SimpleString destrName;
    int destrFxId = -1;
    SimpleString destrFxName;
    SimpleString destrFxTemplate;

    DestrProps() = default;
    DestrProps(const DestrProps &p) : DestrProps() { operator=(p); }
    DestrProps(DestrProps &&p) : DestrProps() { operator=(eastl::move(p)); }
    DestrProps &operator=(const DestrProps &p);
    DestrProps &operator=(DestrProps &&p)
    {
      eastl::swap(res, p.res);
      eastl::swap(destructionImpulse, p.destructionImpulse);
      eastl::swap(collisionHeightScale, p.collisionHeightScale);
      destructable = p.destructable;
      apexDestructible = p.apexDestructible;
      eastl::swap(apexDestructionOptionsPresetName, p.apexDestructionOptionsPresetName);
      isParent = p.isParent;
      destructibleByParent = p.destructibleByParent;
      destroyNeighbourDepth = p.destroyNeighbourDepth;
      eastl::swap(destrName, p.destrName);
      eastl::swap(destrFxId, p.destrFxId);
      eastl::swap(destrFxName, p.destrFxName);
      eastl::swap(destrFxTemplate, p.destrFxTemplate);
      return *this;
    }
    ~DestrProps();
  };

  struct RendinstProperties
  {
    int matId;
    bool immortal;
    bool stopsBullets;
    bool bushBehaviour;
    bool treeBehaviour;
    bool canopyTriangle;
    float canopyTopOffset;
    float canopyTopPart;
    float canopyWidthPart;
    float canopyOpacity;
  };

  struct ElemMask
  {
    uint32_t atest;
    uint32_t cullN;
  };

  struct RtData
  {
    LinearHeapAllocatorSbuffer cellsVb;
    Tab<int16_t> riResMapStor;
    Tab<RenderableInstanceLodsResource *> riRes;
    Tab<uint16_t> riResOrder; // using atest material
    Tab<CollisionResData> riCollRes;
    Tab<E3DCOLOR> riColPair;
    eastl::bitvector<> riPosInst;
    eastl::bitvector<> riPaletteRotation;
    Tab<const char *> riResName;
    SmallTab<ElemMask, MidmemAlloc> riResElemMask; // bit-per-elem mask for rendinst::MAX_LOD_COUNT lod of each riRes
    SmallTab<bbox3f, MidmemAlloc> riResBb;
    SmallTab<bbox3f, MidmemAlloc> riCollResBb;
    SmallTab<rendinst::render::RtPoolData *, MidmemAlloc> rtPoolData;
    Tab<RendinstProperties> riProperties;
    SmallTab<uint8_t, MidmemAlloc> riResHideMask;
    Tab<DebrisProps> riDebrisMap;
    Tab<DebrisPool> riDebris;
    Tab<DestrProps> riDestr;
    Tab<rendinst::DestroyedCellData> riDestrCellData;
    Tab<uint16_t> riExtraIdxPair;
    carray<int, 2> maxDebris, curDebris;
    uint32_t dynamicImpostorToUpdateNo;
    vec4f viewImpostorDir, oldViewImpostorDir;
    vec4f viewImpostorUp;
    int layerIdx;
    int bigChangePoolNo, numImpostorsCount, oldImpostorCycle;

    FastIntList toLoad, loaded, toUnload;
    TextureIdSet riImpTexIds;
    IBBox2 loadedCellsBBox;
    Point2 lastPoi;
    float lastPreloadDistance = 0.f;
    SmartReadWriteFifoLock riRwCs;
    WinCritSec updateVbResetCS;
    float rendinstFarPlane;
    float rendinstMaxLod0Dist;
    float rendinstMaxDestructibleSizeSum;
    float settingsPreloadDistance, preloadDistance;
    float settingsTransparencyDeltaRcp, transparencyDeltaRcp;
    float averageFarPlane;
    float rendinstDistMul, rendinstDistMulFar, rendinstDistMulImpostorTrees, rendinstDistMulFarImpostorTrees,
      impostorsFarDistAdditionalMul, impostorsDistAdditionalMul;
    Tab<uint16_t> predicateIndices;
    vec4f occlusionBoxHalfSize;
    bbox3f movedDebrisBbox;
    bbox3f maxCellBbox;
    int nextPoolForShadowImpostors;
    int nextPoolForClipmapShadows;

    Tab<uint16_t> riResIdxPerStage[rendinst::RIEX_STAGE_COUNT];
#if DAGOR_DBGLEVEL > 0
    eastl::vector_set<int> hiddenIdx;
#endif
    SmallTab<PregenEntCounter, MidmemAlloc> entCntForOldFmt;
    void *entCntOldPtr = nullptr;

  public:
    RtData(int layer_idx);
    ~RtData() { clear(); }

    __forceinline float get_range(float range) const { return range * rendinstDistMul; }
    __forceinline float get_last_range(float range) const { return range * rendinstDistMulFar; }
    __forceinline float get_trees_range(float range) const
    {
      return range * rendinstDistMulImpostorTrees * impostorsDistAdditionalMul;
    }
    __forceinline float get_trees_last_range(float range) const
    {
      return range * rendinstDistMulFarImpostorTrees * impostorsFarDistAdditionalMul;
    }
    void setDistMul(float lod_mul, float cull_mul, bool scale_lod1 = true, bool set_preload_dist = true, bool no_mul_limit = false,
      float impostors_far_dist_additional_mul = 1.f);
    inline void setImpostorsDistAddMul(float add_mul) { impostorsDistAdditionalMul = add_mul; }
    inline void setImpostorsFarDistAddMul(float add_mul) { impostorsFarDistAdditionalMul = add_mul; }

    int renderRendinstGlobalShadowsToTextures(const Point3 &sunDir0, bool force_update, bool use_compression = true);
    bool renderRendinstClipmapShadowsToTextures(const Point3 &sunDir0, bool for_sli, bool force_update);

    void initImpostors();
    void updateImpostors(float shadowDistance, const Point3 &sunDir0, const TMatrix &view_itm, const mat44f &proj_tm);
    bool updateImpostorsPreshadow(int poolNo, const Point3 &sunDir0);
    bool updateImpostorsPreshadow(int poolNo, const Point3 &sunDir0, int paletteId, const UniqueTex &depth_atlas);
    void applyImpostorRange(int ri_idx, const DataBlock *ri_ovr, float cell_size);
    void copyVisibileImpostorsData(const RiGenVisibility &visibility, bool clear_data);

    void initDebris(const DataBlock &ri_blk, int (*get_fx_type_by_name)(const char *name));
    void addDebris(mat44f &tm, int pool_idx, unsigned frameNo, bool effect, rendinst::ri_damage_effect_cb effect_cb,
      const Point3 &axis, float accumulatedPower = 0.0f);
    rendinst::DestroyedRi *addExternalDebris(mat44f &tm, int pool_idx);
    void addDebrisForRiExtraRange(const DataBlock &ri_blk, uint32_t res_idx, uint32_t count);
    void updateDebris(uint32_t curFrame, float dt);

    int riResFirstLod(int ri_idx) const { return riRes[ri_idx]->getQlMinAllowedLod(); }
    int riResLodCount(int ri_idx) const
    {
      return min((int)riRes[ri_idx]->lods.size(), (int)rendinst::MAX_LOD_COUNT - (riRes[ri_idx]->hasImpostor() ? 0 : 1));
    }
    RenderableInstanceLodsResource::Lod &riResLod(int ri_idx, int lod)
    {
      unsigned int ind = lod == rendinst::MAX_LOD_COUNT - 1 ? riRes[ri_idx]->lods.size() - 1 : lod;
      return riRes[ri_idx]->lods[ind];
    }
    RenderableInstanceResource *riResLodScene(int ri_idx, uint32_t lod)
    {
      if (lod == rendinst::MAX_LOD_COUNT - 1)
        lod = riRes[ri_idx]->lods.size() - 1;
      riRes[ri_idx]->updateReqLod(min(lod, riRes[ri_idx]->lods.size() - 1));
      if (lod < riRes[ri_idx]->getQlBestLod())
        lod = riRes[ri_idx]->getQlBestLod();
      if (lod >= riRes[ri_idx]->lods.size())
        lod = riRes[ri_idx]->lods.size() - 1;
      return riRes[ri_idx]->lods[lod].scene;
    }
    inline float riResLodRange(int ri_idx, int lod, const DataBlock *ri_ovr)
    {
      static const char *lod_nm[] = {"lod0", "lod1", "lod2", "lod3"};
      if (lod < riRes[ri_idx]->getQlMinAllowedLod())
        return -1; // lod = riRes[ri_idx]->getQlBestLod();

      if (!ri_ovr || lod > 3)
        return riResLod(ri_idx, lod).range;
      return ri_ovr->getReal(lod_nm[lod], riResLod(ri_idx, lod).range);
    }

    void clear();
    void setTextureMinMipWidth(int textureSize, int impostorSize);
    bool isHiddenId(const int ri_idx)
    {
#if DAGOR_DBGLEVEL > 0
      return hiddenIdx.find(ri_idx) != hiddenIdx.end();
#else
      G_UNUSED(ri_idx);
      return false;
#endif
    }
  };
  struct CellRtData
  {
    struct SubCellSlice
    {
      int ofs, sz;
    };
    vec4f cellOrigin;
    SmallTab<EntPool, MidmemAlloc> pools;
    SmallTab<SubCellSlice, MidmemAlloc> scs;
    SmallTab<uint16_t> scsRemap;
    SmallTab<bbox3f, MidmemAlloc> bbox;
    bbox3f pregenRiExtraBbox;
    uint8_t *sysMemData = nullptr;
    RtData *rtData = nullptr; // parent
    PregenEntRtAdd *pregenAdd = nullptr;
    LinearHeapAllocatorSbuffer::RegionId cellVbId;
    float cellHeight = 0.f;
    int vbSize = 0;
    uint32_t heapGen = ~0u;
    enum
    {
      LOADED = 1,
      CLIPMAP_SHADOW_CASCADES = 4,
      CLIPMAP_SHADOW_RENDERED = 2,
      CLIPMAP_SHADOW_RENDERED_ALLCASCADE = (CLIPMAP_SHADOW_RENDERED << CLIPMAP_SHADOW_CASCADES) - 1,
      STATIC_SHADOW_RENDERED = 1 << 21,
    }; // 4 cascades tops
    uint32_t cellStateFlags = LOADED;
    bool burned = false;
    // WinCritSec *updateVbResetCS;

    CellRtData(int ri_cnt, RtData *parent) : rtData(parent) //-V730
    {
      clear_and_resize(pools, ri_cnt);
      mem_set_0(pools);
      v_bbox3_init_empty(pregenRiExtraBbox);
    }
    ~CellRtData()
    {
      clear();
      del_it(pregenAdd);
      rtData = nullptr;
    }

    int getCellIndex(int poolIdx, int subCellIdx) const { return poolIdx * (SUBCELL_DIV * SUBCELL_DIV) + subCellIdx; }

    int remapPoolIndex(int poolIdx) const { return scsRemap[poolIdx]; }

    const SubCellSlice &getCellSlice(int poolIdx, int subCellIdx) const
    {
      return scs[remapPoolIndex(poolIdx) * (SUBCELL_DIV * SUBCELL_DIV) + subCellIdx];
    }
    SubCellSlice &getCellSlice(int poolIdx, int subCellIdx)
    {
      return scs[remapPoolIndex(poolIdx) * (SUBCELL_DIV * SUBCELL_DIV) + subCellIdx];
    }

    void clear();
    void allocate(int idx);
    void update(int size, RendInstGenData &rgd); // in bytes
    void applyBurnDecal(const bbox3f &decal_bbox);
  };

  struct LandClassRec
  {
    PatchablePtr<const char> landClassName;
    PATCHABLE_DATA64(rendinst::gen::land::AssetData *, asset);
    PATCHABLE_DATA64(HugeBitmask *, mask);
    PatchablePtr<int16_t> riResMap;
  };
  struct Cell
  {
    struct LandClassCoverage
    {
      int landClsIdx;
      int msq;
    };
    PatchableTab<LandClassCoverage> cls;
    PATCHABLE_DATA64(CellRtData *, rtData);
    int16_t htMin, htDelta;

    // fixed (non-generated) RI data
    int32_t riDataRelOfs;
    carray<PatchablePtr<PregenEntCounter>, SUBCELL_DIV * SUBCELL_DIV + 1> entCnt;

    CellRtData *isReady()
    {
      if (CellRtData *crt = interlocked_acquire_load_ptr(rtData))
        if (crt->sysMemData)
          return crt;
      return nullptr;
    }
    const CellRtData *isReady() const { return const_cast<Cell *>(this)->isReady(); }
    bool isLoaded() const
    {
      if (const CellRtData *crt = interlocked_acquire_load_ptr((const CellRtData *const volatile &)rtData))
        return crt->sysMemData || crt->vbSize <= 0;
      return false;
    }
    bool hasFarVisiblePregenInstances(const RendInstGenData *rgl, float thres_dist, dag::Span<float> riMaxDist) const;
  };

  //--- BINARY DATA DUMP START! DO NOT CHANGE!
  PATCHABLE_DATA64(RtData *, rtData);
  PatchableTab<Cell> cells;
  int32_t cellNumW;
  int32_t cellNumH;
  int16_t cellSz;
  uint8_t dataFlags, perInstDataDwords;
  float sweepMaskScale;
  PATCHABLE_DATA64(HugeBitmask *, sweepMask);
  PatchableTab<LandClassRec> landCls;

  vec4f world0Vxz, invGridCellSzV, lastCellXZXZ;
  float grid2world;
  float densMapPivotX;
  float densMapPivotZ;

  int32_t pregenDataBaseOfs;
  PatchableTab<PregenEntPoolDesc> pregenEnt;
  PATCHABLE_DATA64(file_ptr_t, fpLevelBin);
  //--- BINARY DATA DUMP END! DO NOT CHANGE!

  static constexpr const uint8_t DFLG_PREGEN_ENT32_CNT32 = 0x01;

public:
  ~RendInstGenData();

  void prepareRtData(int layer_idx);
  void beforeReleaseRtData();
  void addRIGenExtraSubst(const char *ri_res_name);
  void initRender(const DataBlock *level_blk = nullptr);
  void applyLodRanges();

  inline float world0x() { return as_point4(&world0Vxz).x; }
  inline float world0z() { return as_point4(&world0Vxz).y; }

  CellRtData *generateCell(int x, int y);
  int precomputeCell(CellRtData &crt, int x, int y);
  void updateVb(RendInstGenData::CellRtData &crt, int idx);
  void onDeviceReset();
  template <bool use_external_filter = false>
  bool prepareVisibility(const Frustum &frustum, const Point3 &vpos, RiGenVisibility &visibility, bool forShadow,
    rendinst::LayerFlags layer_flags, Occlusion *use_occlusion, bool for_visual_collision = false,
    const rendinst::VisibilityExternalFilter &external_filter = {});
  void sortRIGenVisibility(RiGenVisibility &visibility, const Point3 &viewPos, const Point3 &viewDir, float vertivalFov,
    float horizontalFov, float value);
  void renderPreparedOpaque(rendinst::RenderPass renderPass, const RiGenVisibility &visibility, bool depth_optimized, int lodI,
    int realdLodI, bool &isStarted);
  void renderByCells(rendinst::RenderPass renderPass, const rendinst::LayerFlags layer_flags, const RiGenVisibility &visibility,
    bool optimization_depth_prepass, bool depth_optimized);
  void renderPreparedOpaque(rendinst::RenderPass renderPass, rendinst::LayerFlags layer_flags, const RiGenVisibility &visibility,
    const TMatrix &view_itm, bool depth_optimized);
  void renderOptimizationDepthPrepass(const RiGenVisibility &visibility, const TMatrix &view_itm);
  void renderDebug();
  void renderPerInstance(rendinst::RenderPass renderPass, int lod, int realLodI, const RiGenVisibility &visibility);
  inline void renderInstances(int ri_idx, int realLodI, const vec4f *data, int count, RenderStateContext &context,
    const int max_instances, const int skip_atest_mask, const int last_stage);
  void renderCrossDissolve(rendinst::RenderPass renderPass, int ri_idx, int realLodI, const RiGenVisibility &visibility);

  void render(rendinst::RenderPass renderPass, const RiGenVisibility &visibility, const TMatrix &view_itm,
    rendinst::LayerFlags layer_flags, bool depth_optimized);
  void render(rendinst::LayerFlags layer_flags);

  void applyBurnDecal(const bbox3f &decal)
  {
    for (auto &c : cells)
      if (c.rtData)
        c.rtData->applyBurnDecal(decal);
  }

  static RendInstGenData *create(IGenLoad &crd, int layer_idx);

  static void initRenderGlobals(bool use_color_padding, bool should_init_gpu_objects);
  static void termRenderGlobals();
  static void initPaletteVb();

  static bool renderResRequired;
  static bool useDestrExclForPregenAdd;
  static bool maskGeneratedEnabled;
  static bool isLoading;

  static void (*riGenPrepareAddPregenCB)(CellRtData &crt, int layer_idx, int per_inst_data_dwords, float ox, float oy, float oz,
    float cell_xz_sz, float cell_y_sz);
  static CellRtData *(*riGenValidateGeneratedCell)(RendInstGenData *rgl, CellRtData *crt, int idx, int cx, int cz);

  void renderRendinstShadowsToClipmap(const BBox2 &region, int cascadeNo);
  bool notRenderedClipmapShadowsBBox(BBox2 &box, int cascadeNo);
  bool notRenderedStaticShadowsBBox(BBox3 &box);
  void setClipmapShadowsRendered(int cacscadeNo);
  bool updateLClassColors(const char *name, E3DCOLOR from, E3DCOLOR to); // for tuning
  void clearDelayedRi();
  void updateHeapVb();

  static RendInstGenData *getGenDataByLayer(const rendinst::RendInstDesc &desc);
};
DAG_DECLARE_RELOCATABLE(RendInstGenData::DestrProps);

namespace rendinst
{
inline constexpr int MAX_PREGEN_RI = 16384; // no more than 65536 due to riex_handle_t limitations

struct RiGenDataAttr
{
  float minDistMul, maxDistMul;
  float poiRad;
  int cellSizeDivisor;
  bool needNetSync, render, depthShadows, clipmapShadows;
};

extern StaticTab<RendInstGenData *, 16> rgLayer; //< all riGen layers (primary & secondary), upto 16
extern StaticTab<RiGenDataAttr, 16> rgAttr;      //< riGen layer attributes (parallel to rgLayer)
extern unsigned rgPrimaryLayers;                 //< number of primary layers (primary layers go first in rgLayer)
extern unsigned rgRenderMaskO; //< bitmask (for indices of rgLayer) used for render (opaque, depth shadows, clipmap shadows, ...)
extern unsigned rgRenderMaskDS;
extern unsigned rgRenderMaskCMS;

inline RendInstGenData *getRgLayer(int l) { return (l >= 0 && l < rgLayer.size()) ? rgLayer[l] : nullptr; }
inline bool isRgLayerPrimary(int l) { return l >= 0 && l < rgPrimaryLayers; }

struct ScopedLockAllRgLayersForWrite
{
  ScopedLockAllRgLayersForWrite()
  {
    for (int i = 0; i < rgLayer.size(); i++)
      if (RendInstGenData *rgl = rgLayer[i])
        rgl->rtData->riRwCs.lockWrite();
  }

  ~ScopedLockAllRgLayersForWrite()
  {
    for (int i = rgLayer.size() - 1; i >= 0; i--)
      if (RendInstGenData *rgl = rgLayer[i])
        rgl->rtData->riRwCs.unlockWrite();
  }
};

inline void rebuildRgRenderMasks()
{
  rgRenderMaskO = rgRenderMaskDS = rgRenderMaskCMS = 0;
  for (int i = 0, m = 1; i < rgLayer.size(); i++, m <<= 1)
    if (rgLayer[i])
    {
      if (!isRgLayerPrimary(i) && !rendinstSecondaryLayer)
        continue;
      if (rgAttr[i].render)
        rgRenderMaskO |= m;
      if (rendinstGlobalShadows && rgAttr[i].depthShadows)
        rgRenderMaskDS |= m;
      if (rendinstClipmapShadows && rgAttr[i].clipmapShadows)
        rgRenderMaskCMS |= m;
    }
}

extern Point3 preloadPointForRendInsts;
extern int preloadPointForRendInstsExpirationTimeMs;
extern ri_register_collision_cb regCollCb;
extern ri_unregister_collision_cb unregCollCb;
extern int ri_game_render_mode;
extern bool enable_apex;
extern DataBlock ri_lod_ranges_ovr;

int get_skip_nearest_lods(const char *name, bool has_impostors, int total_lods);
bool is_ri_extra_for_inst_count_only(const char *name);
int getPersistentPackType(RenderableInstanceLodsResource *res, int def);
uint8_t getResHideMask(const char *res_name, const BBox3 *lbox);
inline bool isResHidden(uint8_t hide_mask) { return ri_game_render_mode < 0 ? false : (hide_mask >> ri_game_render_mode) & 1; }

inline bool is_pos_rendinst_data_destroyed(const int16_t *data) { return data[3] == 0; }
inline bool is_tm_rendinst_data_destroyed(const int16_t *data) { return *(const uint64_t *)data == 0; } //-V1032
inline void destroy_pos_rendinst_data(int16_t *data) { data[3] = 0; }
inline void destroy_tm_rendinst_data(int16_t *data)
{
  data[0] = data[1] = data[2] = data[4] = data[5] = data[6] = data[8] = data[9] = data[10] = 0;
}

RenderableInstanceLodsResource *get_stub_res();
void term_stub_res();
inline bool is_rendinst_marked_collision_ignored(const int16_t *data, int per_inst_data, int stride)
{
  if (per_inst_data)
    return data[stride] == 0xBAD;
  return false;
}
}; // namespace rendinst

#define FOR_EACH_RG_LAYER_DO(VAR)                                   \
  for (int _layer = 0; _layer < rendinst::rgLayer.size(); _layer++) \
    if (RendInstGenData *VAR = rendinst::rgLayer[_layer])

#define FOR_EACH_PRIMARY_RG_LAYER_DO(VAR)                            \
  for (int _layer = 0; _layer < rendinst::rgPrimaryLayers; _layer++) \
    if (RendInstGenData *VAR = rendinst::rgLayer[_layer])

#define FOR_EACH_SECONDARY_RG_LAYER_DO(VAR)                                                 \
  for (int _layer = rendinst::rgPrimaryLayers; _layer < rendinst::rgLayer.size(); _layer++) \
    if (RendInstGenData *VAR = rendinst::rgLayer[_layer])

#define FOR_EACH_RG_LAYER_RENDER(VAR, MASK)                                                              \
  for (int _layer = 0, rm = rendinst::MASK; rm && _layer < rendinst::rgLayer.size(); _layer++, rm >>= 1) \
    if (rm & 1)                                                                                          \
      if (RendInstGenData *VAR = rendinst::rgLayer[_layer])

#if RIGEN_PERINST_ADD_DATA_FOR_TOOLS
#define RIGEN_ADD_STRIDE_PER_INST_B(PER_INST_DWORDS) ((((PER_INST_DWORDS) + 3) & ~3) * 2)
#define RIGEN_TM_STRIDE_B(PER_INST_DWORDS)           (12 * 2 + RIGEN_ADD_STRIDE_PER_INST_B(PER_INST_DWORDS))
#define RIGEN_POS_STRIDE_B(PER_INST_DWORDS) \
  (4 * 2) // pos inst don't have per-inst data since https://cvs1.gaijin.lan/#/c/dagor4/+/169657/
#else
#define RIGEN_ADD_STRIDE_PER_INST_B(PER_INST_DWORDS) 0
#define RIGEN_TM_STRIDE_B(PER_INST_DWORDS)           (12 * 2)
#define RIGEN_POS_STRIDE_B(PER_INST_DWORDS)          (4 * 2)
#endif
#define RIGEN_STRIDE_B(POS_INST, PER_INST_DWORDS) \
  ((POS_INST) ? RIGEN_POS_STRIDE_B(PER_INST_DWORDS) : RIGEN_TM_STRIDE_B(PER_INST_DWORDS))
