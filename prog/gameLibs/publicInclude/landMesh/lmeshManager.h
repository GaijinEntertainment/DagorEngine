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
#include <physMap/physMap.h>


class IBaseLoad;
class LandRayTracer;
class DataBlock;
class HeightmapHandler;
struct Trace;

struct LoadElement
{
  static constexpr int DET_TEX_NUM = 7;
  carray<uint8_t, DET_TEX_NUM> detTexIds;
  TEXTUREID tex1Id, tex2Id;
  BaseTexture *tex1, *tex2;
  LoadElement();
  LoadElement(const LoadElement &) = delete;
  ~LoadElement();
};
DAG_DECLARE_RELOCATABLE(LoadElement);

class LandMeshManager
{
public:
  static constexpr int DET_TEX_NUM = LoadElement::DET_TEX_NUM;

  friend struct LandMeshCullingState;

  static constexpr int LOD_COUNT = 2;
  GlobalVertexData *lmeshVdata, *combinedVdata;
  struct ElemsData
  {
    SmallTab<IBBox2, MidmemAlloc> elemBoxes; // minx, miny, maxx, maxy
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

    void load(IGenLoad &cb, int base_ofs, bool tools_internal);
    void getLandDetailTexture(int index, TEXTUREID &tex1, TEXTUREID &tex2, uint8_t detail_tex_ids[DET_TEX_NUM]);
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
  d3d::SamplerHandle vertTexSmp = d3d::INVALID_SAMPLER_HANDLE;
  d3d::SamplerHandle vertNmTexSmp = d3d::INVALID_SAMPLER_HANDLE;
  d3d::SamplerHandle vertDetTexSmp = d3d::INVALID_SAMPLER_HANDLE;
  LandRayTracer *landTracer;
  bool useVertTexforHMAP;
  bool toolsInternal;

public:
  bool mayRenderHmap;

protected:
  int fileVersion;

  TEXTUREID tileTexId;
  real tileXSize, tileYSize;

  DetailMap detailMap;
  int visRange;

  unsigned srcFileMeshMapOfs = 0;
  const char *srcFileName = nullptr;

  void close();
  bool loadMeshData(IGenLoad &loadCb);

  void loadDetailData(IGenLoad &loadCb);
  void loadRaytracerData(IGenLoad &loadCb, IMemAlloc *rayTracerAllocator = midmem);

public:
  LandMeshManager(bool tools_internal = false);
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
  void setHmapLodDistance(int lodD);
  int getHmapLodDistance() const;
  bool loadHeightmapDump(IGenLoad &loadCb, bool load_render_data);
  PhysMap *loadPhysMap(IGenLoad &loadCb, bool lmp2);
  const carray<Tab<TEXTUREID>, NUM_TEXTURES_STACK> &getMegaDetailsId() const { return megaDetailsId; }

  int getLCEditorId(int i) const { return landClassesEditorId[i]; }
  int getLCCount() const { return landClasses.size(); }
  bool isInTools() const { return toolsInternal; }

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
  bool loadDump(const char *filename, int start_offset = 0, bool load_render_data = true); ///< async load from file
  bool loadDump(IGenLoad &loadCb, IMemAlloc *rayTracerAllocator = midmem, bool load_render_data = true);

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

  //! loads some of required items in small time quantum (async streaming)
  int getBaseOffset() const { return baseDataOffset; }

  void getLandDetailTexture(int x0, int y0, TEXTUREID &tex1, TEXTUREID &tex2, uint8_t detail_tex_ids[DET_TEX_NUM + 1]);
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
    int cellId = x + y * mapSizeX;
    return cells[cellId].decal;
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

  LandMeshCullingState cullingState;

  LandRayTracer *getLandTracer() { return landTracer; }

  inline bool noVertTexHeightmap() { return !useVertTexforHMAP; }

  Tab<LandClassDetailTextures> &getLandClasses() { return landClasses; }
  const Tab<int> &getDetailGroupsToPhysMats() const { return detailGroupsToPhysMats; }

private:
  eastl::optional<LandMeshHolesManager> holesMgr; // Meant to be last member (since it might be absent)
};
DAG_DECLARE_RELOCATABLE(LandMeshManager::CellData);
