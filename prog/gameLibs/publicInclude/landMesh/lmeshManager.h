//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_color.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint2.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <3d/dag_texMgr.h>
#include <ioSys/dag_fileIo.h>
#include <shaders/dag_shaderMesh.h>
#include <ioSys/dag_dataBlock.h>
#include <landMesh/lmeshTools.h>
#include <landMesh/lmeshCulling.h>
#include <landMesh/lmeshRenderer.h>
#include <landMesh/lmeshHoles.h>
#include <generic/dag_carray.h>
#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>
#include <physMap/physMap.h>


class IBaseLoad;
class LandRayTracerSoA4;
typedef LandRayTracerSoA4 land_tracer_t;
class LandVtexRenderer;
class DataBlock;
class HeightmapHandler;
struct Trace;
struct LandWeightAtlas;

// How data lost with the device in a reset comes back: restored by the driver
// from system copies, or re-read from the level file by afterDeviceReset() -
// which costs no resident memory but is only whole if the owner calls it.
enum class LandMeshReset
{
  SysCopy,
  ReloadFromSource
};
class LandWeightAtlasBuilder;

struct LoadElement
{
  static constexpr int DET_TEX_NUM = ::DET_TEX_NUM; // single definition in landMesh/lmeshTools.h
  carray<uint8_t, DET_TEX_NUM> detTexIds;
  LoadElement() { memset(detTexIds.data(), 0xFF, DET_TEX_NUM); }
};
DAG_DECLARE_RELOCATABLE(LoadElement);

class LandMeshManager
{
public:
  static constexpr int DET_TEX_NUM = LoadElement::DET_TEX_NUM;
  static constexpr int DECALS_OVERRIDE_SAMPLERS_COUNT = 2;

  friend struct LandMeshCullingState;

  static constexpr int LOD_COUNT = 2;
  GlobalVertexData *lmeshVdata, *combinedVdata;
  struct ElemsData
  {
    SmallTab<IBBox2, MidmemAlloc> elemBoxes; // minx, miny, maxx, maxy
    SmallTab<Point3, MidmemAlloc> firstVertexPos;
    SmallTab<bool, MidmemAlloc> shouldRenderElem;
  };
  class DetailMap
  {
  protected:
    void load(int i, IGenLoad &cb);

  public:
    int sizeX, sizeY;

    int texSize, texElemSize;


    Tab<LoadElement> cells;

    DetailMap();
    ~DetailMap() { clear(); }
    void clear();

    // converts the per-cell weight textures into one atlas (see lmeshWeightAtlas.h);
    // out_atlas is null when there is no device to hold it
    void load(IGenLoad &cb, int base_ofs, bool tools_internal, LandWeightAtlas **out_atlas, unsigned weight_tex_cflg);
  };
  DetailMap &getDetailMap() { return detailMap; } // for tools
  TEXTUREID getMegaDetailsArrayId(int detail) const { return megaDetailsArrayId[detail]; }

protected:
  HeightmapHandler *hmapHandler;
  uint32_t renderDataNeeded;
  Ptr<ShaderMatVdata> smvd;
  real gridCellSize;
  real landCellSize;
  int baseDataOffset;
  int mapSizeX, mapSizeY;
  IPoint2 origin;
  Point3 offset;
  BBox3 landBbox;

  Tab<int> detailGroupsToPhysMats;
  SmallTab<int, MidmemAlloc> landClassesEditorId;
  Tab<LandClassDetailTextures> landClasses;
  carray<Tab<TEXTUREID>, NUM_TEXTURES_STACK> megaDetailsId;
  int biomeLandClassIdx = -1;

  carray<TEXTUREID, NUM_TEXTURES_STACK> megaDetailsArrayId;

  void loadLandClasses(IGenLoad &loadCb);
  DataBlock grassMaskBlk; //
  struct CellData
  {
    union
    {
      ShaderMesh *meshes[LOD_COUNT + 3] = {};
      struct
      {
        carray<ShaderMesh *, LOD_COUNT> land;
        ShaderMesh *decal;
        ShaderMesh *combined;
        ShaderMesh *patches;
      };
    };
    SmallTab<carray<d3d::SamplerHandle, DECALS_OVERRIDE_SAMPLERS_COUNT>, MidmemAlloc> decalsNoMipbiasSamplers;
    SmallTab<bool, MidmemAlloc> isCombinedBig;
    CellData() = default;
    CellData(const CellData &) = delete;
    ~CellData() { clear(); }
    void clear();
  };
  SmallTab<CellData, MidmemAlloc> cells;
  SmallTab<BBox3, MidmemAlloc> cellBoundings;
  SmallTab<float, MidmemAlloc> cellBoundingsRadius;
  Tab<ElemsData> decalElems;
  TEXTUREID vertTexId;
  TEXTUREID vertNmTexId;
  TEXTUREID vertDetTexId;
  land_tracer_t *landTracer;
  eastl::unique_ptr<LandVtexRenderer> vtex;
  bool useVertTexforHMAP;
  bool toolsInternal;

public:
  bool mayRenderHmap;

protected:
  int fileVersion;

  TEXTUREID tileTexId;
  real tileXSize, tileYSize;

  DetailMap detailMap;
  LandWeightAtlas *weightAtlas = nullptr;
  int visRange;

  unsigned srcFileMeshMapOfs = 0;
  unsigned srcFileDetailMapOfs = 0;
  const char *srcFileName = nullptr;
  LandMeshReset dataReset = LandMeshReset::SysCopy;

  void close();
  bool loadMeshData(IGenLoad &loadCb);

  void loadDetailData(IGenLoad &loadCb);
  void loadRaytracerData(IGenLoad &loadCb, IMemAlloc *rayTracerAllocator = midmem);

public:
  LandMeshManager(bool tools_internal = false, LandMeshCullingState::CullMode cull_mode = LandMeshCullingState::ASYNC_CULLING);
  ~LandMeshManager();
  LandClassData getRenderDataNeeded() const { return (LandClassData)renderDataNeeded; }
  HeightmapHandler *getHmapHandler() const { return hmapHandler; }
  const LandMeshHolesManager *getHolesManager() const { return holesMgr ? &*holesMgr : nullptr; }
  LandMeshHolesManager *getHolesManager() { return holesMgr ? &*holesMgr : nullptr; }
  void initHolesManager()
  {
    if (hmapHandler)
      holesMgr.emplace(*hmapHandler);
  }
  bool clearAndAddHoles(const Tab<LandMeshHolesManager::HoleArgs> &holes)
  {
    if (!holesMgr)
      return false;
    holesMgr->clearAndAddHoles(holes);
    return true;
  }
  void clearHoles()
  {
    if (holesMgr)
      holesMgr->clearHoles();
  }
  void afterDeviceReset(LandMeshRenderer *lrend, bool full_reset);
  void updateOverrideSamplers();
  bool loadHeightmapDump(IGenLoad &loadCb, bool load_render_data, float water_level = -1000000, float shore_error_meters = 2.0f);
  PhysMap *loadPhysMap(IGenLoad &loadCb, bool lmp2);
  void filterHeighLandmeshDecals(const DataBlock &levelBlk);
  const carray<Tab<TEXTUREID>, NUM_TEXTURES_STACK> &getMegaDetailsId() const { return megaDetailsId; }

  int getLCEditorId(int i) const { return landClassesEditorId[i]; }
  int getLCCount() const { return landClasses.size(); }
  bool isInTools() const { return toolsInternal; }

  bool forceHeightmapRendering = false;

  void evictSplattingData(); // remove all data for splatting. If splatting data is removed, you can not render last clip around or
                             // vtex
  void setRenderDataNeeded(uint32_t data_needed)
  {
    if (!landClasses.size())
      renderDataNeeded |= data_needed;
  }
  void resetRenderDataNeeded(uint32_t data_not_needed)
  {
    if (!landClasses.size())
      renderDataNeeded &= ~data_not_needed;
  }
  bool loadDump(const char *filename, int start_offset = 0, bool load_render_data = true,
    LandMeshReset reset = LandMeshReset::SysCopy); ///< async load from file
  bool loadDump(IGenLoad &loadCb, IMemAlloc *rayTracerAllocator = midmem, bool load_render_data = true,
    LandMeshReset reset = LandMeshReset::SysCopy);

  //! Tests ray hit to closest object and returns parameters of hit (if happen)
  bool traceray(const Point3 &p, const Point3 &dir, real &t, Point3 *normal, bool cull = true);

  //! Get maximum height at point (and normal, if needed), ht is out parameter only
  bool getHeight(const Point2 &p, real &ht, Point3 *normal);

  //! Get maximum height below point (and normal, if needed), return height in parameter ht
  // return false if there is no hit in between [ht, pos3.y], ht is inout parameter!
  bool getHeightBelow(const Point3 &pos3, float &ht, Point3 *normal);

  //! Tests ray hit to any object and returns parameters of hit (if happen)
  bool rayhitNormalized(const Point3 &p, const Point3 &normDir, real t);

  //! Tests ray hit to landmesh and tests if ray lies under landscape in any case and returns true is it is.
  bool rayUnderNormalized(const Point3 &p, const Point3 &normDir, real t, real hmap_height_offs = 0.f);

  // return 0 if no intersect, 1 if intersect, -1 if error
  int traceDownHmapMultiRay(dag::Span<Trace> traces);

  const Tab<ElemsData> &getDecalElems() const { return decalElems; }
  LandMeshRenderer *createRenderer();
  LandVtexRenderer *getVtexRenderer() const { return vtex.get(); }

  //! loads some of required items in small time quantum (async streaming)
  int getBaseOffset() const { return baseDataOffset; }
  void getLandDetailTexIds(int x0, int y0, uint8_t detail_tex_ids[DET_TEX_NUM]);
  ShaderMesh *getCellLandShaderMesh(int x, int y, int lod = 0)
  {
    return getCellLandShaderMeshOffseted(x - origin.x, y - origin.y, lod);
  }

  ShaderMesh *getCellLandShaderMeshOffseted(int x, int y, int lod)
  {
    if (x < 0 || x >= mapSizeX || y < 0 || y >= mapSizeY)
      return NULL;
    return getCellLandShaderMeshRaw(x, y, lod);
  }
  ShaderMesh *getCellDecalShaderMesh(int x, int y) { return getCellDecalShaderMeshOffseted(x - origin.x, y - origin.y); }

  ShaderMesh *getCellCombinedShaderMesh(int x, int y, bool **out_is_combined_big = NULL)
  {
    return getCellCombinedShaderMeshOffseted(x - origin.x, y - origin.y, out_is_combined_big);
  }

  ShaderMesh *getCellLandShaderMeshRaw(int x, int y, int lod)
  {
    int cellId = x + y * mapSizeX;
    return cells[cellId].land[lod] ? cells[cellId].land[lod] : cells[cellId].land[0];
  }


  ShaderMesh *getCellDecalShaderMeshOffseted(int x, int y)
  {
    if (x < 0 || x >= mapSizeX || y < 0 || y >= mapSizeY)
      return NULL;
    return getCellDecalShaderMeshRaw(x, y);
  }

  dag::Span<const d3d::SamplerHandle> getCellDecalElemSamplersNoMipbiasMeshOffseted(int x, int y, int i)
  {
    if (x < 0 || x >= mapSizeX || y < 0 || y >= mapSizeY)
      return {};

    int cellId = x + y * mapSizeX;
    return cells[cellId].decalsNoMipbiasSamplers[i];
  }

  ShaderMesh *getCellCombinedShaderMeshOffseted(int x, int y, bool **out_is_combined_big = NULL)
  {
    if (x < 0 || x >= mapSizeX || y < 0 || y >= mapSizeY)
      return NULL;
    return getCellCombinedShaderMeshRaw(x, y, out_is_combined_big);
  }

  ShaderMesh *getCellDecalShaderMeshRaw(int x, int y) { return cells[x + y * mapSizeX].decal; }
  ShaderMesh *getCellCombinedShaderMeshRaw(int x, int y, bool **out_is_combined_big)
  {
    int cellId = x + y * mapSizeX;
    if (out_is_combined_big)
      *out_is_combined_big = cells[cellId].isCombinedBig.data();
    return cells[cellId].combined;
  }

  ShaderMesh *getCellPatchesShaderMesh(int x, int y)
  {
    if (x < 0 || x >= mapSizeX || y < 0 || y >= mapSizeY)
      return NULL;
    return cells[x + y * mapSizeX].patches;
  }

  void setVisibilityRangeCells(int vr) { visRange = vr; }
  int getVisibilityRangeCells() { return visRange; }
  const LandWeightAtlas *getWeightAtlas() const { return weightAtlas; }
  LandWeightAtlas *getWeightAtlasForEdit() { return weightAtlas; } // daEditor paints into it
  void getDetailMapSize(int &elem_size, int &tex_size)
  {
    tex_size = detailMap.texSize;
    elem_size = detailMap.texElemSize;
  }

  int getNumCellsX() const { return mapSizeX; }
  int getNumCellsY() const { return mapSizeY; }
  IPoint2 getCellOrigin() const { return origin; }
  float getLandCellSize() const { return landCellSize; }
  float getGridCellSize() const { return gridCellSize; }
  BBox3 getBBox(int x, int y, float *sphere_radius = NULL);
  const BBox3 &getBBox() const { return landBbox; }
  BBox3 getBBoxWithHMapWBBox() const;
  Point3 getOffset() const { return offset; }
  TEXTUREID getLightMap() { return BAD_TEXTUREID; }
  const IBBox2 *getExclCellBBox() { return &cullingState.exclBox; }
  bool isDecodedToWorldPos() const { return lmeshVdata != NULL; }
  GlobalVertexData *getLMeshVdata() const { return lmeshVdata; }
  GlobalVertexData *getCombinedVdata() const { return combinedVdata; }

  void setGrassMaskBlk(const DataBlock &blk);

  // Terrain mirroring config: border cells mirrored on each side of the map. Level data, set once at
  // load (like exclBox); the renderer derives its scaled/clamped per-cell tables from this. Kept on
  // the manager so culling can consume it without a renderer reference.
  struct MirroringCfg
  {
    int numBorderCellsXPos = 0, numBorderCellsXNeg = 0, numBorderCellsZPos = 0, numBorderCellsZNeg = 0;
  };
  void setMirroring(int x_pos, int x_neg, int z_pos, int z_neg) { mirrorCfg = {x_pos, x_neg, z_pos, z_neg}; }
  const MirroringCfg &getMirroringCfg() const { return mirrorCfg; }
  // Cell count of one 4096m visibility/mirroring unit, and the mirroring cfg scaled/clamped by it.
  // The renderer's mirror tables and the desc-based cull share these so their cell indexing agrees.
  int getScaleVisRange() const { return (int)floorf(4096.0f / landCellSize + 0.5f); }
  void getScaledBorderCells(int &x_pos, int &x_neg, int &z_pos, int &z_neg) const
  {
    const int scale = getScaleVisRange();
    x_pos = min(mirrorCfg.numBorderCellsXPos * scale, mapSizeX);
    x_neg = min(mirrorCfg.numBorderCellsXNeg * scale, mapSizeX);
    z_pos = min(mirrorCfg.numBorderCellsZPos * scale, mapSizeY);
    z_neg = min(mirrorCfg.numBorderCellsZNeg * scale, mapSizeY);
  }
  MirroringCfg mirrorCfg;

  LandMeshCullingState cullingState;

  // a null tracer is a legitimate state (pure-heightmap levels carry none)
  land_tracer_t *getLandTracer() { return landTracer; }
  const land_tracer_t *getLandTracer() const { return landTracer; }

  inline bool noVertTexHeightmap() { return !useVertTexforHMAP; }

  Tab<LandClassDetailTextures> &getLandClasses() { return landClasses; }
  const Tab<int> &getDetailGroupsToPhysMats() const { return detailGroupsToPhysMats; }

  void replaceHeightmapHandler(HeightmapHandler *h, bool need_land_tracer);

  static carray<int, DECALS_OVERRIDE_SAMPLERS_COUNT> getDecalsTexReferencesForSamplers();
  static carray<d3d::SamplerHandle, DECALS_OVERRIDE_SAMPLERS_COUNT> getOverrideSamplers(const ShaderMesh::RElem &re,
    const carray<int, DECALS_OVERRIDE_SAMPLERS_COUNT> &tex_refs);

private:
  void initHmapCullingState();

  eastl::optional<LandMeshHolesManager> holesMgr; // Meant to be last member (since it might be absent)
};
DAG_DECLARE_RELOCATABLE(LandMeshManager::CellData);
