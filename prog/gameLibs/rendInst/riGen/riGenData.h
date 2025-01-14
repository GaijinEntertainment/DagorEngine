// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstDebris.h>
#include <rendInst/rendInstGenDamageInfo.h>
#include <rendInst/visibility.h>
#include <rendInst/renderPass.h>

#include <generic/dag_patchTab.h>
#include <generic/dag_smallTab.h>
#include <EASTL/bitvector.h>
#include <EASTL/unique_ptr.h>
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
#include <dag/dag_vectorSet.h>
#include <shaders/dag_linearSbufferAllocator.h>
#include <memory/dag_linearHeapAllocator.h>

// use additional data as hashVal only when in Tools (De3X, AV2)
#define RIGEN_PERINST_ADD_DATA_FOR_TOOLS _TARGET_PC_TOOLS_BUILD

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
bool renderRIGenGlobalShadowsToTextures(const Point3 &sunDir0, bool force_update, bool use_compression, bool free_temp_resources);
} // namespace rendinst::render
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
    int topOfs;
    uint8_t *topPtr(uint8_t *base) const { return base + topOfs; }
  };
  struct PregenEntPoolDesc
  {
    PatchablePtr<RenderableInstanceLodsResource> riRes;
    PatchablePtr<const char> riName;
    carray<E3DCOLOR, 2> colPair;
    uint32_t posInst : 1, paletteRotation : 1, zeroInstSeeds : 1, _resv29 : 29;
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
    bool needsUpdate() const { return submersion > 0.f || inclination > 0.f; }
  };
  struct DebrisPool
  {
    const DebrisProps *props = nullptr;
    int resIdx = -1;
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
    float soundOcclusion;
    rendinstdestr::TreeDestr::BranchDestr treeBranchDestrFromDamage;
    rendinstdestr::TreeDestr::BranchDestr treeBranchDestrOther;
  };

  struct ElemMask
  {
    uint32_t atest;
    uint32_t cullN;
    uint32_t plod;
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
    eastl::bitvector<> riZeroInstSeeds;
    Tab<const char *> riResName;
    SmallTab<ElemMask, MidmemAlloc> riResElemMask; // bit-per-elem mask for rendinst::MAX_LOD_COUNT lod of each riRes
    SmallTab<bbox3f, MidmemAlloc> riResBb;
    SmallTab<bbox3f, MidmemAlloc> riCollResBb;
    SmallTab<rendinst::render::RtPoolData *, MidmemAlloc> rtPoolData;
    Tab<RendinstProperties> riProperties;
    SmallTab<uint8_t, MidmemAlloc> riResHideMask;
    Tab<DebrisProps> riDebrisMap;
    Tab<DebrisPool> riDebris;
    Tab<eastl::unique_ptr<rendinst::DestroyedRi>> riDebrisDelayedRi; // TODO: remove indirection (put by value)
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
    WinCritSec updateVbResetCS; // TODO: ensure that `cellsVb` mutation is done in main thread only and remove this
    float rendinstFarPlane;
    float rendinstMaxLod0Dist;
    float rendinstMaxDestructibleSizeSum;
    float settingsPreloadDistance, preloadDistance;
    float settingsTransparencyDeltaRcp, transparencyDeltaRcp;
    float averageFarPlane;
    float rendinstDistMul, rendinstDistMulFar, rendinstDistMulImpostorTrees, rendinstDistMulFarImpostorTrees,
      impostorsFarDistAdditionalMul, impostorsDistAdditionalMul, impostorsMinRange;
    Tab<uint16_t> predicateIndices;
    vec4f occlusionBoxHalfSize;
    bbox3f maxCellBbox;
    enum class GlobalShadowPhase
    {
      // Render impostor into small texture
      LOW_PASS = 0,
      // Read back low pass texture from gpu, calculate bounding box,
      // render bounding box part into bigger texture and
      // compress it (if use_compress=true)
      HIGH_PASS = 1,
      // After complete of high pass for all rotation in rotation palette
      READY = 2,
    };
    struct GlobalShadowTask
    {
      int batchIndex = 0;
      GlobalShadowPhase phase = GlobalShadowPhase::LOW_PASS;
      int poolNo = 0;
      int rotationId = 0;
    };
    dag::VectorSet<GlobalShadowTask> globalShadowTask;
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
      return min(range * rendinstDistMulImpostorTrees * impostorsDistAdditionalMul, impostorsMinRange);
    }
    __forceinline float get_trees_last_range(float range) const
    {
      return range * rendinstDistMulFarImpostorTrees * impostorsFarDistAdditionalMul;
    }
    void setDistMul(float lod_mul, float cull_mul, bool scale_lod1 = true, bool set_preload_dist = true, bool no_mul_limit = false,
      float impostors_far_dist_additional_mul = 1.f);
    inline void setImpostorsDistAddMul(float add_mul) { impostorsDistAdditionalMul = add_mul; }
    inline void setImpostorsFarDistAddMul(float add_mul) { impostorsFarDistAdditionalMul = add_mul; }
    inline void setImpostorsMinRange(float min_range) { impostorsMinRange = min_range; }

    enum class GlobalShadowRet
    {
      ERR = 0,
      WAIT_NEXT_FRAME = 1,
      DONE = 2,
    };
    friend bool rendinst::render::renderRIGenGlobalShadowsToTextures(const Point3 &sunDir0, bool force_update, bool use_compression,
      bool free_temp_resources);

  private:
    bool areImpostorsReady(bool force_update);
    bool haveNextPoolForShadowImpostors();
    bool shouldRenderGlobalShadows();
    GlobalShadowRet renderGlobalShadow(GlobalShadowTask &task, const Point3 &sun_dir_0, bool force_update, bool use_compression);

  public:
    GlobalShadowRet renderRendinstGlobalShadowsToTextures(const Point3 &sun_dir_0, bool force_update, bool use_compression);
    bool renderRendinstClipmapShadowsToTextures(const Point3 &sunDir0, bool for_sli, bool force_update);

    void initImpostors();
    void updateImpostors(float shadowDistance, const Point3 &sunDir0, const TMatrix &view_itm, const mat44f &proj_tm);
    bool updateImpostorsPreshadow(int poolNo, const Point3 &sunDir0);
    bool updateImpostorsPreshadow(int poolNo, const Point3 &sunDir0, int paletteId, const UniqueTex &depth_atlas);
    void applyImpostorRange(int ri_idx, const DataBlock *ri_ovr, float cell_size);
    void copyVisibileImpostorsData(const RiGenVisibility &visibility, bool clear_data);

    void initDebris(const DataBlock &ri_blk, int (*get_fx_type_by_name)(const char *name));
    void addDebris(mat44f &tm, int pool_idx, unsigned frameNo, const Point3 &axis, float accumulatedPower = 0.0f);
    rendinst::DestroyedRi *addExternalDebris(mat44f &tm, int pool_idx);
    void addDebrisForRiExtraRange(const DataBlock &ri_blk, uint32_t res_idx, uint32_t count);
    void updateDelayedDebrisRi(float dt, bbox3f *movedDebrisBbox);
    void updateDebris(float dt, bbox3f *movedDebrisBbox)
    {
      if (!riDebrisDelayedRi.empty())
        updateDelayedDebrisRi(dt, movedDebrisBbox);
    }

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
    bool isHiddenId(const int ri_idx) const
    {
#if DAGOR_DBGLEVEL > 0
      return hiddenIdx.find(ri_idx) != hiddenIdx.end();
#else
      G_UNUSED(ri_idx);
      return false;
#endif
    }
  };

  struct CellRtDataLoaded // Note: this part of cellRtData could be unloaded in runtime
  {
    struct SubCellSlice
    {
      int ofs, sz;
    };
    SmallTab<SubCellSlice, MidmemAlloc> scs;
    SmallTab<uint16_t> scsRemap;
    SmallTab<bbox3f, MidmemAlloc> bbox;
    eastl::unique_ptr<uint8_t[]> sysMemData;
  };

  struct CellRtData : CellRtDataLoaded
  {
    vec4f cellOrigin;
    SmallTab<EntPool, MidmemAlloc> pools;
    bbox3f pregenRiExtraBbox;
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

  float world0x() const { return v_extract_x(world0Vxz); }
  float world0z() const { return v_extract_y(world0Vxz); }
  vec4f getWorld0Vxz() const { return world0Vxz; }

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
  void renderPreparedOpaque(rendinst::RenderPass renderPass, rendinst::LayerFlags layer_flags, const RiGenVisibility &visibility,
    const TMatrix &view_itm, bool depth_optimized);
  void renderOptimizationDepthPrepass(const RiGenVisibility &visibility, const TMatrix &view_itm);
  void renderDebug();

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
  void updateHeapVbNoLock();

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

inline bool is_pos_rendinst_data_destroyed(const int16_t *data) { return interlocked_relaxed_load((const uint16_t &)data[3]) == 0; }
inline bool is_tm_rendinst_data_destroyed(const int16_t *data)
{
  const uint32_t *data32 = (const uint32_t *)data; //-V1032
  return interlocked_relaxed_load(data32[0]) == 0 && interlocked_relaxed_load(data32[1]) == 0;
}
inline void destroy_pos_rendinst_data(int16_t *data, dag::ConstSpan<uint8_t> mrange)
{
  G_UNUSED(mrange);
#ifdef _DEBUG_TAB_
  G_FAST_ASSERT((uint8_t *)data >= mrange.data() && (uint8_t *)&data[3] < mrange.end());
#endif
  interlocked_release_store((uint16_t &)data[3], 0);
}
inline void destroy_tm_rendinst_data(int16_t *data, dag::ConstSpan<uint8_t> mrange)
{
  G_UNUSED(mrange);
#ifdef _DEBUG_TAB_
  G_FAST_ASSERT((uint8_t *)data >= mrange.data() && (uint8_t *)&data[10] < mrange.end());
#endif
  uint32_t *data32 = reinterpret_cast<uint32_t *>(data); //-V1032
  // Begin with storing sentinel value to the data32[1], this value
  // cant occur naturally, so it will be treated as invalid, if loaded.
  // Because we start with release-storing this value, reader is guaranteed
  // to see it, if it reads data after this operation, see acquire_load_tm_data_first_row
  // NOTE: we cant use 64 bit atomics here, because data is always aligned by 4 bytes, but not by 8
  interlocked_release_store(data32[1], 0x80008000u);
  interlocked_relaxed_store(data32[0], 0);
  for (int i = 4; i < 11; i++)
    interlocked_relaxed_store((uint16_t &)data[i], 0);
  // replace sentinel value with zero, release-store instead of relaxed, so reader will be
  // guaranteed to see other data zeroed too
  interlocked_release_store(data32[1], 0);
}

RenderableInstanceLodsResource *get_stub_res();
void term_stub_res();
inline bool is_rendinst_marked_collision_ignored(const int16_t *data, int per_inst_data, int stride)
{
  if (per_inst_data)
    return data[stride] == 0xBAD;
  return false;
}

inline bool getRIGenCanopyBBox(const RendInstGenData::RendinstProperties &prop, const BBox3 &ri_world_bbox, BBox3 &out_canopy_bbox)
{
  float canopyWidth = prop.canopyWidthPart * (ri_world_bbox[1].y - ri_world_bbox[0].y) * 0.5f;
  float boxHeight = ri_world_bbox[1].y - ri_world_bbox[0].y;
  Point3 boxCenter = ri_world_bbox.center();
  out_canopy_bbox[0].set(boxCenter.x - canopyWidth, ri_world_bbox[1].y - boxHeight * (prop.canopyTopPart + prop.canopyTopOffset),
    boxCenter.z - canopyWidth);
  out_canopy_bbox[1].set(boxCenter.x + canopyWidth, ri_world_bbox[1].y - boxHeight * prop.canopyTopOffset, boxCenter.z + canopyWidth);

  return prop.canopyOpacity > 0.0;
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

#define FOR_EACH_RG_LAYER_RENDER_EX(VAR, MASK, LAYER)                                  \
  for (int _layer = 0, rm = (MASK); rm && _layer < (LAYER).size(); _layer++, rm >>= 1) \
    if (rm & 1)                                                                        \
      if (RendInstGenData *VAR = (LAYER)[_layer])

#define FOR_EACH_RG_LAYER_RENDER(VAR, MASK) FOR_EACH_RG_LAYER_RENDER_EX (VAR, rendinst::MASK, rendinst::rgLayer)

#if RIGEN_PERINST_ADD_DATA_FOR_TOOLS
#define RIGEN_ADD_STRIDE_PER_INST_B(ZERO_INST_SEEDS, PER_INST_DWORDS) ((ZERO_INST_SEEDS) ? 0 : ((((PER_INST_DWORDS) + 3) & ~3) * 2))
#define RIGEN_TM_STRIDE_B(ZERO_INST_SEEDS, PER_INST_DWORDS)           (12 * 2 + RIGEN_ADD_STRIDE_PER_INST_B(ZERO_INST_SEEDS, PER_INST_DWORDS))
#define RIGEN_POS_STRIDE_B(ZERO_INST_SEEDS, PER_INST_DWORDS) \
  (4 * 2) // pos inst don't have per-inst data since https://cvs1.gaijin.lan/#/c/dagor4/+/169657/
#else
#define RIGEN_ADD_STRIDE_PER_INST_B(ZERO_INST_SEEDS, PER_INST_DWORDS) 0
#define RIGEN_TM_STRIDE_B(ZERO_INST_SEEDS, PER_INST_DWORDS)           (12 * 2)
#define RIGEN_POS_STRIDE_B(ZERO_INST_SEEDS, PER_INST_DWORDS)          (4 * 2)
#endif
#define RIGEN_STRIDE_B(POS_INST, ZERO_INST_SEEDS, PER_INST_DWORDS) \
  ((POS_INST) ? RIGEN_POS_STRIDE_B(ZERO_INST_SEEDS, PER_INST_DWORDS) : RIGEN_TM_STRIDE_B(ZERO_INST_SEEDS, PER_INST_DWORDS))
