//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_texMgr.h>
#include <3d/dag_materialData.h>
#include <generic/dag_span.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point3.h>
#include <util/dag_stdint.h>
#include <libTools/util/makeBindump.h>
#include <physMap/physMap.h>

class HeightmapLandRenderer;
class IHeightmapLandProvider;
class LandMeshRenderer;
class LandMeshManager;
class IBBox2;
class Point2;
class DataBlock;
class BaseTexture;
typedef BaseTexture Texture;
class ITextureNumerator;
class StaticGeometryContainer;
class GeomObject;
class Point3;
struct LoadElement;

template <class T>
class MapStorage;
typedef MapStorage<float> FloatMapStorage;
typedef MapStorage<uint64_t> Uint64MapStorage;
typedef MapStorage<uint32_t> Uint32MapStorage;
typedef MapStorage<uint16_t> Uint16MapStorage;
typedef MapStorage<E3DCOLOR> ColorMapStorage;
class HeightMapStorage;
class GenHmapData;
struct HmapBitLayerDesc;
struct HmapDetLayerProps;

namespace landmesh
{
struct Cell;
}

struct LandClassDetailTextures;
struct LandClassDetailInfo;

class IHmapService
{
public:
  static constexpr unsigned HUID = 0xA36C15FAu; // IHmapService

  //! height map (initial and optional final) management
  virtual bool createStorage(HeightMapStorage &hm, int sx, int sy, bool sep_final) = 0;
  virtual void destroyStorage(HeightMapStorage &hm) = 0;

  //! creators for typed MapStorage
  virtual FloatMapStorage *createFloatMapStorage(int sx, int sy, float defval = 0) = 0;
  virtual Uint64MapStorage *createUint64MapStorage(int sx, int sy, uint64_t defval = 0) = 0;
  virtual Uint32MapStorage *createUint32MapStorage(int sx, int sy, unsigned defval = 0) = 0;
  virtual Uint16MapStorage *createUint16MapStorage(int sx, int sy, unsigned defval = 0) = 0;
  virtual ColorMapStorage *createColorMapStorage(int sx, int sy, E3DCOLOR defval = 0) = 0;

  //! landclass layers management
  virtual void *createBitLayersList(const DataBlock &desc) = 0;
  virtual void destroyBitLayersList(void *handle) = 0;
  virtual dag::Span<HmapBitLayerDesc> getBitLayersList(void *handle) = 0;
  virtual int getBitLayerIndexByName(void *handle, const char *layer_name) = 0;
  virtual const char *getBitLayerName(void *handle, int layer_idx) = 0;
  virtual int getBitLayerAttrId(void *handle, const char *attr_name) = 0;
  virtual bool testBitLayerAttr(void *handle, int layer_idx, int attr_id) = 0;
  virtual int findBitLayerByAttr(void *handle, int attr_id, int start_with_layer = 0) = 0;

  //! GenHmapData registering for external use
  virtual GenHmapData *registerGenHmap(const char *name, HeightMapStorage *hm, Uint32MapStorage *landclsmap,
    dag::ConstSpan<HmapBitLayerDesc> lndclass_layers, ColorMapStorage *colormap, Uint32MapStorage *ltmap, const Point2 &hmap_ofs,
    float cell_sz) = 0;
  virtual bool unregisterGenHmap(const char *name) = 0;
  virtual GenHmapData *findGenHmap(const char *name) = 0;

  //! Heightmap2 renderer support
  virtual void initLodGridHm2(const DataBlock &blk) = 0;
  virtual void setupRenderHm2(float sx, float sy, float ax, float ay, Texture *htTexMain, TEXTUREID htTexIdMain, float mx0, float my0,
    float mw, float mh, Texture *htTexDet, TEXTUREID htTexIdDet, float dx0, float dy0, float dw, float dh) = 0;
  virtual void renderHm2(const Point3 &vpos, bool infinite, bool render_hm = false) const = 0;
  virtual void renderHm2VSM(const Point3 &vpos) const = 0;

  //! LandMesh manager support
  virtual LandMeshManager *createLandMeshManager(IGenLoad &crd) = 0;
  virtual void destroyLandMeshManager(LandMeshManager *&p) const = 0;
  virtual BBox3 getBBoxWithHMapWBBox(LandMeshManager &p) const = 0;

  //! LandMesh renderer support
  virtual LandMeshRenderer *createLandMeshRenderer(LandMeshManager &p) = 0;
  virtual void destroyLandMeshRenderer(LandMeshRenderer *&r) const = 0;
  virtual void prepareLandMesh(LandMeshRenderer &r, LandMeshManager &p, const Point3 &pos) const = 0;
  virtual void invalidateClipmap(bool force_redraw, bool rebuild_last_clip = true) = 0;
  virtual void prepareClipmap(LandMeshRenderer &r, LandMeshManager &p, float ht_rel) = 0;
  virtual void renderLandMesh(LandMeshRenderer &r, LandMeshManager &p) const = 0;
  virtual void renderLandMeshClipmap(LandMeshRenderer &r, LandMeshManager &p) const = 0;
  virtual void renderLandMeshVSM(LandMeshRenderer &r, LandMeshManager &p) const = 0;
  virtual void renderLandMeshDepth(LandMeshRenderer &r, LandMeshManager &p) const = 0;
  virtual void renderLandMeshHeight(LandMeshRenderer &r, LandMeshManager &p) const = 0;
  virtual void renderDecals(LandMeshRenderer &r, LandMeshManager &p) const = 0;
  virtual void renderLandMeshGrassMask(LandMeshRenderer &r, LandMeshManager &p) const = 0;
  virtual void startUAVFeedback() const = 0;
  virtual void endUAVFeedback() const = 0;

  virtual void updateGrassTranslucency(LandMeshRenderer &r, LandMeshManager &p) const = 0;

  virtual void updatePropertiesFromLevelBlk(const DataBlock &level_blk) = 0;

  virtual void resetTexturesLandMesh(LandMeshRenderer &r) const = 0;

  virtual bool exportToGameLandMesh(mkbindump::BinDumpSaveCB &cwr, dag::ConstSpan<landmesh::Cell> cells,
    dag::ConstSpan<MaterialData> materials, int lod1TrisDensity, bool tools_internal) const = 0;

  virtual bool exportGeomToShaderMesh(mkbindump::BinDumpSaveCB &cwr, StaticGeometryContainer &geom, const char *tmp_lms_fn,
    const ITextureNumerator &tn) const = 0;

  //! LandMesh CachedTexElement support
  virtual bool mapStorageFileExists(const char *fname) const = 0;
  virtual void onLevelBlkLoaded(const DataBlock &blk) = 0;

  //! detail coloring support
  // obsolete
  virtual void createDetLayerClassList(const DataBlock &blk) {}
  virtual dag::Span<HmapDetLayerProps> getDetLayerClassList() { return {}; }
  virtual int resolveDetLayerClassName(const char *nm) { return -1; }

  virtual void *createDetailRenderData(const DataBlock &layers, LandClassDetailInfo *detTex) { return 0; }
  virtual void destroyDetailRenderData(void *handle) {}
  virtual void updateDetailRenderData(void *handle, const DataBlock &layers) {}
  virtual Texture *getDetailRenderDataMaskTex(void *handle, const char *name) { return 0; }
  virtual TEXTUREID getDetailRenderDataTex(void *handle, const char *name) { return BAD_TEXTUREID; }
  virtual void storeDetailRenderData(void *handle, const char *prefix, bool store_cmap, bool store_smap) {}
  //==

  virtual const char *getLandPhysMatName(int pmatId) = 0;
  virtual int getPhysMatId(const char *name) = 0;
  virtual int getBiomeIndices(const Point3 &p) = 0;
  virtual bool getIsSolidByPhysMatName(const char *name) = 0;
  virtual void getPuddlesParams(float &power_scale, float &seed, float &noise_influence) = 0;

  virtual bool exportLoftMasks(const char *fn_mask, const char *fn_id, const char *fn_ht, const char *fn_dir, int tex_sz,
    const Point3 &w0, const Point3 &w1, GeomObject *g) = 0;

  virtual void setGrassBlk(const DataBlock *grassBlk) = 0;

  virtual void prepare(LandMeshRenderer &r, LandMeshManager &p, const Point3 &center_pos, const BBox3 &box) = 0;
  virtual int setGrassMaskRenderingMode() = 0;
  virtual void restoreRenderingMode(int oldMode) = 0;
  virtual int setLod0SubDiv(int) = 0;
  virtual BBox3 getLMeshBBoxWithHMapWBBox(LandMeshManager &p) const = 0;
  virtual PhysMap *loadPhysMap(LandMeshManager *landMeshManager, IGenLoad &crd) = 0;
  virtual void beforeRender() = 0;
};

struct HmapBitLayerDesc
{
  unsigned wordMask;
  short shift, bitCount;

  unsigned getLayerData(unsigned w) const { return (w & wordMask) >> shift; }
  unsigned setLayerData(unsigned w, unsigned d) const { return (w & ~wordMask) | ((d << shift) & wordMask); }

  bool checkOverflow(unsigned d) const { return (d & ~(wordMask >> shift)) != 0; }
};
