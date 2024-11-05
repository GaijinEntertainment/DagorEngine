// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "Brushes/hmlBrush.h"
#include <squirrel.h>

#include <EditorCore/ec_brush.h>
#include <EditorCore/ec_ObjectEditor.h>

#include <oldEditor/de_interface.h>
#include <oldEditor/de_clipping.h>
#include <de3_assetService.h>
#include <de3_bitMaskMgr.h>
#include <de3_hmapStorage.h>
#include <de3_heightmap.h>
#include <de3_roadsProvider.h>
#include <de3_genObjHierMask.h>
#include <de3_fileTracker.h>
#include <de3_landmesh.h>
#include <coolConsole/iConsoleCmd.h>
#include <math/dag_color.h>

#include <3d/dag_texMgr.h>

#include <landMesh/lmeshRenderer.h>
#include <landMesh/lmeshManager.h>
#include "landMeshMap.h"

#include "hmlObjectsEditor.h"
#include "hmlLayers.h"
#include <de3_grassSrv.h>
#include <de3_gpuGrassService.h>
#include <de3_waterSrv.h>
#include <de3_waterProjFxSrv.h>
#include <de3_cableSrv.h>
#include "gpuGrassPanel.h"
#include "navmeshAreasProcessing.h"

#define DECAL_BITMAP_SZ          32
#define KERNEL_RAD               2
#define EXTENDED_DECAL_BITMAP_SZ (DECAL_BITMAP_SZ + 2 * 2 * KERNEL_RAD)
#define MAX_NAVMESHES            2

namespace PropPanel
{
class DialogWindow;
}

class CoolConsole;
class IObjectCreator;
class HmapLandBrush;

struct HmapBitLayerDesc;
struct MemoryChainedData;

class LandClassSlotsManager;
class GenHmapData;
class RoadsSnapshot;
class PostFxRenderer;

class IHmapService;
class IBitMaskImageMgr;
class IDagorPhys;
class Bitarray;

class DebugPrimitivesVbuffer;

namespace mkbindump
{
class BinDumpSaveCB;
}


enum GrassLayerPanelElements
{
  // GL = Grass Layer
  GL_PID_ASSET_NAME,

  GL_PID_MASK_R,
  GL_PID_MASK_G,
  GL_PID_MASK_B,
  GL_PID_MASK_A,

  GL_PID_DENSITY,
  GL_PID_HORIZONTAL_SCALE_MIN,
  GL_PID_HORIZONTAL_SCALE_MAX,
  GL_PID_VERTICAL_SCALE_MIN,
  GL_PID_VERTICAL_SCALE_MAX,
  GL_PID_WIND_MUL,
  GL_PID_RADIUS_MUL,

  GL_PID_GRASS_RED_COLOR_FROM,
  GL_PID_GRASS_RED_COLOR_TO,
  GL_PID_GRASS_GREEN_COLOR_FROM,
  GL_PID_GRASS_GREEN_COLOR_TO,
  GL_PID_GRASS_BLUE_COLOR_FROM,
  GL_PID_GRASS_BLUE_COLOR_TO,

  GL_PID_REMOVE_LAYER,

  GL_PID_ELEM_COUNT
};

class GrassLayerPanel : public DObject
{
public:
  String layerName;
  int grass_layer_i;

  PropPanel::ContainerPropertyControl *newLayerGrp;
  int pidBase;
  bool remove_this_layer;

  GrassLayerPanel(const char *name, int layer_i);

  void fillParams(PropPanel::ContainerPropertyControl *parent_panel, int &pid);
  void onClick(int pid, PropPanel::ContainerPropertyControl &p);
  bool onPPChangeEx(int pid, PropPanel::ContainerPropertyControl &p);
  void onPPChange(int pid, PropPanel::ContainerPropertyControl &panel);

  void saveLayer(DataBlock &blk);
};


class HmapLandPlugin : public IGenEditorPlugin,
                       public IGenEventHandlerWrapper,
                       public IBrushClient,
                       public IBinaryDataBuilder,
                       public IWriteAddLtinputData,
                       public IDagorEdCustomCollider,
                       public IHmapBrushImage,
                       public IHeightmap,
                       public ILandmesh,
                       public IRoadsProvider,
                       public ILightingChangeClient,
                       public IGatherStaticGeometry,
                       public IRenderingService,
                       public IOnExportNotify,
                       public IPluginAutoSave,
                       public PropPanel::ControlEventHandler,
                       public IWndManagerWindowHandler,
                       public IPostProcessGeometry,
#if defined(USE_HMAP_ACES)
                       public IEnvironmentSettings,
#endif
                       public IConsoleCmd,
                       public IExportToDag,
                       public IAssetUpdateNotify
{
public:
  enum
  {
    HMAX_DET_TEX_NUM = LandMeshManager::DET_TEX_NUM,
    LMAX_DET_TEX_NUM = LandMeshManager::DET_TEX_NUM
  };
  enum HeightmapTypes
  {
    HEIGHTMAP_MAIN,
    HEIGHTMAP_DET,
    HEIGHTMAP_WATER_DET,
    HEIGHTMAP_WATER_MAIN
  };
  static HmapLandPlugin *self;
  static bool defMipOrdRev;
  static bool preferZstdPacking;
  static bool allowOodlePacking;
  static bool pcPreferASTC;
  static bool useASTC(unsigned tc) { return tc == _MAKE4C('iOS') || (tc == _MAKE4C('PC') && pcPreferASTC); }

  static IHmapService *hmlService;
  static IBitMaskImageMgr *bitMaskImgMgr;
  static IDagorPhys *dagPhys;

  Tab<uint8_t> lcRemap;

  static bool prepareRequiredServices();
  static void processTexName(SimpleString &out_name, const char *in_name);
  void buildAndWritePhysMap(BinDumpSaveCB &cwr);


  static IGrassService *grassService;
  static IGPUGrassService *gpuGrassService;
  static IWaterService *waterService;
  static ICableService *cableService;
  bool enableGrass;
  DataBlock *grassBlk = nullptr, *gpuGrassBlk = nullptr;
  GPUGrassPanel gpuGrassPanel;

  static IWaterProjFxService *waterProjectedFxSrv;
  void updateWaterProjectedFx();

  HmapLandPlugin();
  ~HmapLandPlugin();

  // IGenEditorPlugin
  virtual const char *getInternalName() const { return "heightmapLand"; }
  virtual const char *getMenuCommandName() const { return "Landscape"; }
  virtual const char *getHelpUrl() const { return "/html/Plugins/HeightmapLand/index.htm"; }

  virtual int getRenderOrder() const { return 100; }
  virtual int getBuildOrder() const { return 0; }

  virtual bool showSelectAll() const { return true; }
  virtual bool showInTabs() const { return true; }

  virtual void registered();
  virtual void unregistered();
  virtual void loadSettings(const DataBlock &global_settings) override;
  virtual void saveSettings(DataBlock &global_settings) override;
  virtual void beforeMainLoop();

  void registerMenuAccelerators() override;
  virtual bool begin(int toolbar_id, unsigned menu_id);
  virtual bool end();
  virtual IGenEventHandler *getEventHandler();

  virtual void setVisible(bool vis);
  virtual bool getVisible() const { return isVisible; }
  virtual bool getSelectionBox(BBox3 &box) const;
  virtual bool getStatusBarPos(Point3 &pos) const { return false; }

  virtual void clearObjects();
  virtual void onNewProject();
  virtual void autoSaveObjects(DataBlock &local_data);
  virtual void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path);
  virtual void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path);
  virtual bool acceptSaveLoad() const { return true; }

  virtual void selectAll() { objEd.selectAll(); }
  virtual void deselectAll() { objEd.unselectAll(); }
  void invalidateObjectProps() { objEd.invalidateObjectProps(); }

  virtual void actObjects(float dt);
  virtual void beforeRenderObjects(IGenViewportWnd *vp);
  virtual void renderObjects();
  virtual void renderTransObjects();
  virtual void updateImgui() override;
  virtual void renderGrass(Stage stage);
  virtual void renderGPUGrass(Stage stage);
  virtual void renderWater(Stage stage);
  virtual void renderCables();

  virtual void replaceGrassMask(int landclass_id, const char *newGrassMaskName);
  virtual void resetGrassMask(const DataBlock &grassBlk);
  virtual int getLandclassIndex(const char *landclass_name);
  virtual bool catchEvent(unsigned ev_huid, void *userData);

  virtual void *queryInterfacePtr(unsigned huid);

  virtual bool onPluginMenuClick(unsigned id);
  virtual void handleViewportAcceleratorCommand(unsigned id) override;

  // ControlEventHandler
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual void onChangeFinished(int pcb_id, PropPanel::ContainerPropertyControl *panel);

  // IWndManagerWindowHandler
  virtual void *onWmCreateWindow(int type) override;
  virtual bool onWmDestroyWindow(void *window) override;

  // IRenderingService
  virtual void renderGeometry(Stage stage);
  virtual void prepare(const Point3 &center_pos, const BBox3 &box) override;
  virtual int setSubDiv(int lod) override;

  // IGenEventHandler
  virtual IGenEventHandler *getWrappedHandler() { return &objEd; }

  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif);
  virtual void handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif);

  // IBrushClient
  virtual void onBrushPaintStart(Brush *brush, int buttons, int key_modif);
  virtual void onBrushPaintEnd(Brush *brush, int buttons, int key_modif);
  virtual void onBrushPaint(Brush *brush, const Point3 &center, const Point3 &prev_center, const Point3 &normal, int buttons,
    int key_modif);
  virtual void onRBBrushPaintStart(Brush *brush, int buttons, int key_modif);
  virtual void onRBBrushPaintEnd(Brush *brush, int buttons, int key_modif);
  virtual void onRBBrushPaint(Brush *brush, const Point3 &center, const Point3 &prev_center, const Point3 &normal, int buttons,
    int key_modif);

  // IBinaryDataBuilder
  virtual bool validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *params);
  virtual bool addUsedTextures(ITextureNumerator &tn);
  virtual bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *pp);
  virtual bool checkMetrics(const DataBlock &metrics_blk) { return true; }

  // IWriteAddLtinputData
  virtual void writeAddLtinputData(IGenSave &cwr);

  // IConsoleCmd
  virtual bool onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params);
  virtual const char *onConsoleCommandHelp(const char *cmd);

  // IHeightmapProvider
  virtual void requestHeightmapData(int x0, int y0, int step, int x_size, int y_size);

  virtual void getHeightmapData(int x0, int y0, int step, int x_size, int y_size, real *data, int stride_bytes);

  virtual bool isLandPreloadRequestComplete();

  // ILandmesh
  virtual BBox3 getBBoxWithHMapWBBox() const;

  void updateLandDetailTexture(unsigned i);

  float getLandCellSize() const;
  int getNumCellsX() const;
  int getNumCellsY() const;
  IPoint2 getCellOrigin() const;
  const IBBox2 *getExclCellBBox();

  float getGridCellSize() const { return gridCellSize; }
  float getMeshCellSize()
  {
    meshCells = clamp(meshCells, 1, 256);
    int mapSize = getHeightmapSizeX();
    return max((mapSize / meshCells) * getGridCellSize(), 1.0f);
  }
  virtual Point3 getOffset() const;

  virtual TEXTUREID getLightMap();
  // Loads Land Datail Texture for LandMesh
  void loadLandDetailTexture(int x0, int y0, Texture *tex1, Texture *tex2, carray<uint8_t, LMAX_DET_TEX_NUM> &detail_tex_ids,
    bool *done_mark, int tex_size, int elem_size);

  int loadLandDetailTexture(unsigned targetCode, int x0, int y0, char *imgPtr, int stride, char *imgPtr2, int stride2,
    carray<uint8_t, LMAX_DET_TEX_NUM> &detail_tex_ids, bool *done_mark, int size, int elem_size, bool tex1_rgba, bool tex2_rgba);

  int getMostUsedDetTex(int x0, int y0, int texDataSize, uint8_t *det_tex_ids, uint8_t *idx_remap, int max_dtn);

  template <class UpdateLC>
  int updateLandclassWeight(int x0, int y0, int x1, int y1, UpdateLC &update_cb);

  // from IGatherStaticGeometry
  virtual void gatherStaticVisualGeometry(StaticGeometryContainer &cont)
  {
    gatherStaticGeometry(cont, StaticGeometryNode::FLG_RENDERABLE, false);
  }

  virtual void gatherStaticCollisionGeomGame(StaticGeometryContainer &cont);

  virtual void gatherStaticCollisionGeomEditor(StaticGeometryContainer &cont) { gatherStaticGeometry(cont, 0, true); }

  virtual void gatherStaticEnviGeometry(StaticGeometryContainer &container) {}
  void gatherStaticGeometry(StaticGeometryContainer &cont, int flags, bool collision, int stage = 0)
  {
    objEd.gatherStaticGeometry(cont, flags, collision, stage);
  }

  // IDagorEdCustomCollider
  virtual bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm);
  virtual bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt);
  virtual const char *getColliderName() const { return "HeightMap"; }
  virtual bool isColliderVisible() const;

  // IHmapBrushImage
  virtual real getBrushImageData(int x, int y, IHmapBrushImage::Channel channel);
  virtual void setBrushImageData(int x, int y, real v, IHmapBrushImage::Channel channel);

#if defined(USE_HMAP_ACES)
  // IEnvironmentSettings.
  virtual void getEnvironmentSettings(DataBlock &blk);
  virtual void setEnvironmentSettings(DataBlock &blk);
#endif

  // IExportToDag.
  void gatherExportToDagGeometry(StaticGeometryContainer &container)
  {
    if (!useMeshSurface || exportType == EXPORT_PSEUDO_PLANE)
      return;

    if (landMeshMap.isEmpty())
      generateLandMeshMap(landMeshMap, DAGORED2->getConsole(), false, NULL);

    landMeshMap.gatherExportToDagGeometry(container);
  }


  bool traceRayPrivate(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm);

  void createHeightmapFile(CoolConsole &con);
  void resizeHeightMapDet(CoolConsole &con);
  void createColormapFile(CoolConsole &con);
  void createLightmapFile(CoolConsole &con);
  void resizeLandClassMapFile(CoolConsole &con);

  void resetLandmesh() { pendingLandmeshRebuild = true; }
  void resetRenderer(bool immediate = false);
  void invalidateRenderer();
  void updateHeightMapTex(bool det_hmap, const IBBox2 *dirty_box = NULL);
  void updateHeightMapConstants();
  void updateHmap2Tesselation();

  int getNumDetailTextures() const { return numDetailTextures; }


  void updateScriptImageList();
  bool importScriptImage(String *name = NULL);
  bool createMask(int bpp, String *name = NULL);

  const char *pickScriptImage(const char *current_name, int bpp);
  void prepareScriptImage(const char *name, int img_size_mul, int img_bpp);
  int getScriptImage(const char *name, int img_size_mul = 0, int img_bpp = 0);
  int getScriptImageWidth(int id);
  int getScriptImageHeight(int id);
  int getScriptImageBpp(int id);
  E3DCOLOR sampleScriptImageUV(int id, real x, real y, bool clamp_x, bool clamp_y);
  E3DCOLOR sampleScriptImagePixelUV(int id, real x, real y, bool clamp_x, bool clamp_y);
  bool saveImage(int idx);

  void paintScriptImageUV(int id, real x0, real y0, real x1, real y1, bool clamp_x, bool clamp_y, E3DCOLOR color);

  float sampleMask1UV(int id, real x, real y);
  float sampleMask1UV(int id, real x, real y, bool clamp_x, bool clamp_y);
  float sampleMask8UV(int id, real x, real y);
  float sampleMask8UV(int id, real x, real y, bool clamp_x, bool clamp_y);
  float sampleMask1PixelUV(int id, real x, real y);
  float sampleMask8PixelUV(int id, real x, real y);
  void paintMask1UV(int id, real x0, real y0, bool c);
  void paintMask8UV(int id, real x0, real y0, char c);

  using PackedDecalBitmap = carray<uint32_t, DECAL_BITMAP_SZ>;
  using ExtendedDecalBitmap = carray<char, EXTENDED_DECAL_BITMAP_SZ * EXTENDED_DECAL_BITMAP_SZ>;
  class ScriptParam : public DObject
  {
  public:
    SimpleString paramName;

    ScriptParam(const char *name) : paramName(name) {}

    virtual void fillParams(PropPanel::ContainerPropertyControl &panel, int &pid) = 0;
    virtual void onPPChange(int pid, PropPanel::ContainerPropertyControl &panel) = 0;
    virtual void onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel) {}

    virtual void save(DataBlock &blk) = 0;
    virtual void load(const DataBlock &blk) = 0;

    virtual void setToScript(HSQUIRRELVM vm) = 0;

    virtual bool onPPChangeEx(int pid, PropPanel::ContainerPropertyControl &panel)
    {
      onPPChange(pid, panel);
      return false;
    }
    virtual bool onPPBtnPressedEx(int pid, PropPanel::ContainerPropertyControl &panel)
    {
      onPPBtnPressed(pid, panel);
      return false;
    }
  };


  ScriptParam *getEditedScriptImage();
  void editScriptImage(ScriptParam *image, int idx = -1);

  IHmapBrushImage::Channel getEditedChannel() const;
  bool showEditedMask() const;

  bool importTileTex();

  // IHeightmap
  virtual bool isLoaded() const { return heightMap.isFileOpened(); }
  virtual real getHeightmapCellSize() const { return gridCellSize; }
  virtual int getHeightmapSizeX() const { return heightMap.getMapSizeX(); }
  virtual int getHeightmapSizeY() const { return heightMap.getMapSizeY(); }

  virtual bool getHeightmapPointHt(Point3 &inout_p, Point3 *out_norm) const;
  virtual bool getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid) const;

  virtual Point3 getHeightmapOffset() const { return Point3(heightMapOffset[0], 0, heightMapOffset[1]); }

  bool getHeightmapHeight(const IPoint2 &cell, real &ht) const;
  bool getHeightmapOnlyPointHt(Point3 &inout_p, Point3 *out_norm) const;

  // IRoadsProvider
  virtual IRoadsProvider::Roads *getRoadsSnapshot();

  bool applyHmModifiers1(HeightMapStorage &hm, float gc_sz, bool gen_colors, bool reset_final, IBBox2 *out_dirty_clip,
    IBBox2 *out_sum_dirty, bool *out_colors_changed);
  void applyHmModifiers(bool gen_colors = true, bool reset_final = true, bool finished = true);
  void collapseModifiers(dag::ConstSpan<SplineObject *> collapse_splines);
  void copyFinalHmapToInitial();
  void restoreBackup();

  void heightmapChanged(bool ch = true) { heightMap.changed = ch; }
  void invalidateFinalBox(const BBox3 &box);

  bool hmBackupCanBeRestored()
  {
    if (heightMap.getInitialMap().canBeReverted())
      return true;
    if (detDivisor && heightMapDet.getInitialMap().canBeReverted())
      return true;
    return false;
  }
  bool hmCommitChanges();

  bool getTexEntry(const char *s, String *path, String *name) const;

  void calcGoodLandLightingInBox(const IBBox2 &calc_box);

  // ILightingChangeClient
  virtual void onLightingChanged() {}
  virtual void onLightingSettingsChanged();

  // IOnExportNotify interface
  virtual void onBeforeExport(unsigned target_code);
  virtual void onAfterExport(unsigned target_code);

  // IPostProcessGeometry
  virtual void processGeometry(StaticGeometryContainer &container);

  //! returns number of invisible faces, and marked faces to stay
  int markUndergroundFaces(MeshData &mesh, Bitarray &facesAbove, TMatrix *wtm);
  //! removes faces below ground
  void removeInvisibleFaces(StaticGeometryContainer &container);


  const HmapBitLayerDesc &getDetLayerDesc() const { return (HmapBitLayerDesc &)detLayerDescStorage; }
  const HmapBitLayerDesc &getImpLayerDesc() const { return (HmapBitLayerDesc &)impLayerDescStorage; }
  void *getLayersHandle() const { return landClsLayersHandle; }
  int getDetLayerIdx() const { return detLayerIdx; }
  MapStorage<uint32_t> &getlandClassMap() { return landClsMap; }

  MapStorage<uint64_t> *getDetTexIdxMap() const { return detTexIdxMap; }
  MapStorage<uint64_t> *getDetTexWtMap() const { return detTexWtMap; }
  void prepareDetTexMaps();

  LandClassSlotsManager &getLandClassMgr() { return *lcMgr; }
  bool setDetailTexSlot(int s, const char *blk_name);

  bool exportLightmapToFile(const char *file_name, int target_code, bool high_quality);

  bool rebuildSplinesPolyBitmask(BBox2 &out_dirty_box);
  bool rebuildSplinesLoftBitmask(int layer, BBox2 &out_dirty_box);
  void rebuildSplinesBitmask(bool auto_regen = true);

  void rebuildRivers();

  bool buildAndWriteSingleNavMesh(BinDumpSaveCB &cwr, int nav_mesh_idx);
  bool buildAndWriteNavMesh(BinDumpSaveCB &cwr);
  void clearNavMesh();
  void renderNavMeshDebug();

  bool rebuildHtConstraintBitmask();
  bool applyHtConstraintBitmask(Mesh &mesh); // returns true if changed

  bool isSnowAvailable() { return (snowValSVId != -1 && snowPrevievSVId != -1); }
  float getSnowValue() { return ambSnowValue; }
  bool hasSnowSpherePreview() { return (snowSpherePreview && snowDynPreview); }
  void updateSnowSources() { objEd.updateSnowSources(); }

  void setSelectMode();

  bool renderDebugLines;
  bool renderAllSplinesAlways, renderSelSplinesAlways;

  bool hasWaterSurf() const { return hasWaterSurface; }
  float getWaterSurfLevel() const { return waterSurfaceLevel; }
  bool isPointUnderWaterSurf(float x, float z);
  bool isPseudoPlane() const { return exportType == EXPORT_PSEUDO_PLANE; }

  bool getHeight(const Point2 &p, real &ht, Point3 *normal) const;
  void setShowBlueWhiteMask();

  void refillPanel(bool schedule_regen = false);

  void prepareEditableLandClasses();
  void addGenLayer(const char *name, int insert_before = -1);
  bool moveGenLayer(ScriptParam *gl, bool up);
  bool delGenLayer(ScriptParam *gl);

  struct HMDetGH
  {
    static Point3 getHeightmapOffset() { return HmapLandPlugin::self->getHeightmapOffset(); }
    static real getHeightmapCellSize() { return HmapLandPlugin::self->getHeightmapCellSize() / HmapLandPlugin::self->detDivisor; }
    static bool getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid)
    {
      h0 = HmapLandPlugin::self->heightMapDet.getFinalData(cell.x, cell.y);
      hx = HmapLandPlugin::self->heightMapDet.getFinalData(cell.x + 1, cell.y);
      hy = HmapLandPlugin::self->heightMapDet.getFinalData(cell.x, cell.y + 1);
      hxy = HmapLandPlugin::self->heightMapDet.getFinalData(cell.x + 1, cell.y + 1);

      hmid = (h0 + hx + hy + hxy) * 0.25f;
      return true;
    }
  };
  struct HMDetTR : public HMDetGH
  {
    static int getHeightmapSizeX() { return HmapLandPlugin::self->getHeightmapSizeX() * HmapLandPlugin::self->detDivisor; }
    static int getHeightmapSizeY() { return HmapLandPlugin::self->getHeightmapSizeY() * HmapLandPlugin::self->detDivisor; }
    static bool getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid)
    {
      if (HmapLandPlugin::self->insideDetRectC(cell) && HmapLandPlugin::self->insideDetRectC(cell.x + 1, cell.y + 1))
        return HMDetGH::getHeightmapCell5Pt(cell, h0, hx, hy, hxy, hmid);
      return false;
    }
  };

  bool hasDetaledRect() const { return detDivisor; }
  bool mapGlobalTCtoDetRectTC(float &inout_fx, float &inout_fy) const
  {
    inout_fx *= detDivisor * getHeightmapSizeX();
    inout_fy *= detDivisor * getHeightmapSizeY();
    if (!detDivisor || !insideDetRectC(inout_fx, inout_fy))
      return false;
    inout_fx = (inout_fx - detRectC[0].x) / (detRectC[1].x - detRectC[0].x);
    inout_fy = (inout_fy - detRectC[0].y) / (detRectC[1].y - detRectC[0].y);
    return true;
  }

  bool usesGenScript() const { return !colorGenScriptFilename.empty(); }
  PropPanel::ContainerPropertyControl *getPropPanel() const;

  bool insideDetRectC(int x, int y) const
  {
    return x >= detRectC[0].x && x < detRectC[1].x && y >= detRectC[0].y && y < detRectC[1].y;
  }
  bool insideDetRectC(const IPoint2 &p) const { return insideDetRectC(p.x, p.y); }

  bool loadLevelSettingsBlk(DataBlock &level_blk);
  void updateWaterSettings(const DataBlock &level_blk);
  void invalidateDistanceField() { distFieldInvalid = true; }

  void onLayersDlgClosed();
  void selectLayerObjects(int lidx, bool sel = true);
  void moveObjectsToLayer(int lidx, dag::Span<RenderableEditableObject *> objects);

  void onObjectsRemove();

private:
  // Caches datailed textures for LandMesh
  int toolbarId;

  // IAssetUpdateNotify
  virtual void onLandClassAssetChanged(landclass::AssetData *data) override {}
  virtual void onLandClassAssetTexturesChanged(landclass::AssetData *data) override;
  virtual void onSplineClassAssetChanged(splineclass::AssetData *data) override {}

  void clearTexCache();
  void refillTexCache();
  void resetTexCacheRect(const IBBox2 &box);

  class HmapLandPanel;
  BBox3 landBox;
  class HmlSelTexDlg;

  enum
  {
    HILLUP_BRUSH,
    HILLDOWN_BRUSH,
    SMOOTH_BRUSH,
    ALIGN_BRUSH,
    SHADOWS_BRUSH,
    SCRIPT_BRUSH,

    BRUSHES_COUNT
  };

  SmallTab<HmapLandBrush *, MidmemAlloc> brushes;
  int currentBrushId;
  IBBox2 brushFullDirtyBox;
  bool noTraceNow; // for align to collision

  HmapLandPanel *propPanel;
  HmapLandObjectEditor objEd;

  const char *hmapImportCaption = "TIFF 8/16 bit (*.tif)|*.tif;*.tiff|"
                                  "Raw 32f (*.r32)|*.r32|"
                                  "Raw 16 (*.r16)|*.r16|"
                                  "Photoshop Raw 16 (*.raw)|*.raw|"
                                  "Raw 32f with header (*.height)|*.height|"
                                  "|All files(*.*)|*.*";

  DataBlock mainPanelState;

  real gridCellSize;
  Point2 heightMapOffset;

  String levelBlkFName;
  DataBlock levelBlk;

  struct
  {
    Point2 ofs, sz;
    bool show;
  } collisionArea;

  struct RenderParams
  {
    int gridStep, elemSize, radiusElems, ringElems, numLods, maxDetailLod;
    real detailTile, canyonHorTile, canyonVertTile;
    int holesLod;
    real canyonAngle, canyonFadeAngle;
    real hm2YbaseForLod;

    int landTexSize, landTexElemSize;
    int detMapSize, detMapElemSize;

    int hm2displacementQ;
    bool showFinalHM;

    RenderParams() { init(); }

    void init();
  };

  RenderParams render;

  real sunAzimuth, sunZenith;

  struct SunSkyLight
  {
    E3DCOLOR sunColor, skyColor, specularColor;
    real sunPower, skyPower, specularMul, specularPower;

    SunSkyLight() { init(); }

    void init()
    {
      sunColor = E3DCOLOR(255, 240, 140);
      sunPower = 1.2f;
      skyColor = E3DCOLOR(60, 120, 255);
      skyPower = 0.5f;
      specularColor = E3DCOLOR(255, 240, 140);
      specularMul = 1.0f;
      specularPower = 16;
    }

    Color3 getSunLightColor() const { return color3(sunColor) * sunPower; }
    Color3 getSkyLightColor() const { return color3(skyColor) * skyPower; }
    Color3 getSpecularColor() const { return color3(specularColor) * specularMul; }
    void save(const char *prefix, DataBlock &) const;
    void load(const char *prefix, const DataBlock &);
  };

  SunSkyLight ldrLight;

  real shadowBias, shadowTraceDist, shadowDensity;

  SimpleString origBasePath;
  HeightMapStorage heightMap;

  MapStorage<uint32_t> &landClsMap;
  int lcmScale;
  void *landClsLayersHandle;
  dag::Span<HmapBitLayerDesc> landClsLayer;
  LandClassSlotsManager *lcMgr;

  MapStorage<E3DCOLOR> &colorMap;
  MapStorage<uint32_t> &lightMapScaled;
  int lightmapScaleFactor;

  bool geomLoftBelowAll;
  bool hasWaterSurface, hasWorldOcean;
  SimpleString waterMatAsset;
  float waterSurfaceLevel, minUnderwaterBottomDepth, worldOceanExpand;
  float worldOceanShorelineTolerance;
  objgenerator::HugeBitmask waterMask;
  int waterMaskScale;

  TEXTUREID lmlightmapTexId;
  BaseTexture *lmlightmapTex;
  TEXTUREID bluewhiteTexId;
  Texture *bluewhiteTex;

  LandMeshMap landMeshMap;

  MapStorage<uint64_t> *detTexIdxMap, *detTexWtMap;

  int detDivisor;
  HeightMapStorage heightMapDet;
  BBox2 detRect;
  IBBox2 detRectC;
  Texture *hmapTex[2];
  TEXTUREID hmapTexId[2];

  int numDetailTextures;
  Tab<SimpleString> detailTexBlkName;

  String lastHmapImportPath, lastLandExportPath, lastHmapExportPath, lastColormapExportPath, colorGenScriptFilename, lastTexImportPath,
    lastGATExportPath, lastWaterHeightmapImportPath, lastHmapImportPathDet, lastHmapImportPathMain, lastWaterHeightmapImportPathDet,
    lastWaterHeightmapImportPathMain;
  struct FileChangeStat
  {
    int64_t size = 0, mtime = 0;
  } lastChangeDet, lastChangeMain, lastChangeWaterDet, lastChangeWaterMain;

  bool isVisible;
  bool doAutocenter;

  real lastMinHeight[2];
  real lastHeightRange[2];

  HeightMapStorage waterHeightmapDet;
  HeightMapStorage waterHeightmapMain;
  Point2 waterHeightMinRangeDet;
  Point2 waterHeightMinRangeMain;

  String lastExpLoftFolder;
  bool lastExpLoftMain = true, lastExpLoftDet = true;
  int lastExpLoftMainSz = 4096, lastExpLoftDetSz = 4096;
  bool lastExpLoftCreateAreaSubfolders = true;
  bool lastExpLoftUseRect[2] = {false, false};
  BBox2 lastExpLoftRect[2];

  LandMeshManager *landMeshManager;
  LandMeshRenderer *landMeshRenderer;
  MemoryChainedData *lmDump;
  bool pendingLandmeshRebuild;
  TEXTUREID lightmapTexId;

  PtrTab<ScriptParam> colorGenParams;
  Ptr<ScriptParam> detailedLandMask;
  DataBlock *colorGenParamsData;

  Ptr<ScriptParam> editedScriptImage;
  int editedScriptImageIdx;
  int esiGridW, esiGridH;
  float esiGridStep;
  Point2 esiOrigin;

  DataBlock navMeshProps[MAX_NAVMESHES];
  int shownExportedNavMeshIdx;
  bool showExportedNavMesh;
  bool showExportedCovers;
  bool showExportedNavMeshContours;
  bool showExpotedObstacles = false;
  bool disableZtestForDebugNavMesh = false;

  PtrTab<ScriptParam> genLayers;
  bool showBlueWhiteMask;
  void updateBlueWhiteMask(const IBBox2 *);
  void updateGenerationMask(const IBBox2 *rect);

  bool showMonochromeLand;
  E3DCOLOR monochromeLandCol;

  bool showHtLevelCurves;
  float htLevelCurveStep, htLevelCurveThickness, htLevelCurveOfs, htLevelCurveDarkness;

  void addGrassLayer(const char *name);
  void exportGrass(BinDumpSaveCB &cwr);
  bool saveGrassLayers(DataBlock &blk);
  bool loadGrassLayers(const DataBlock &blk, bool update_grass_blk = true);
  void loadGrassFromLevelBlk();
  bool loadGrassFromBlk(const DataBlock &level_blk);
  void loadGPUGrassFromLevelBlk();
  bool loadGPUGrassFromBlk(const DataBlock &level_blk);

  PropPanel::ContainerPropertyControl *grass_panel;
  PtrTab<GrassLayerPanel> grassLayers;
  struct
  {
    float defaultMinDensity;
    float defaultMaxDensity;
    float maxRadius;
    int texSize;
    eastl::vector<eastl::pair<String, String>> masks;

    float sowPerlinFreq;
    float fadeDelta;
    float lodFadeDelta;
    float blendToLandDelta;

    float noise_first_rnd;
    float noise_first_val_mul;
    float noise_first_val_add;

    float noise_second_rnd;
    float noise_second_val_mul;
    float noise_second_val_add;

    float directWindMul;
    float noiseWindMul;
    float windPerlinFreq;
    float directWindFreq;
    float directWindModulationFreq;
    float windWaveLength;
    float windToColor;
    float helicopterHorMul;
    float helicopterVerMul;
    float helicopterToDirectWindMul;
    float helicopterFalloff;
  } grass;


  PropPanel::DialogWindow *brushDlg;
  int brushDlgHideRequestFrameCountdown = 0;


  Point3 calcSunLightDir() const;
  inline void getNormalAndSky(int x, int y, Point3 &normal, real &sky, float *ht = NULL);
  unsigned calcFastLandLightingAt(float x, float y, const Point3 &sun_light_dir);
  inline void getNormal(int x, int y, Point3 &normal);
  unsigned calcNormalAt(float ox, float oy);
  real calcSunShadow(float x, float y, const Point3 &sun_light_dir, float *other_ht = NULL);

  void createPropPanel();
  void createMenu(unsigned menu_id);
  void refillBrush();
  void fillPanel(PropPanel::ContainerPropertyControl &panel);
  void updateLightGroup(PropPanel::ContainerPropertyControl &panel);
  void fillAreatypeRectPanel(PropPanel::ContainerPropertyControl &panel, int navmesh_idx, int base_ofs);

  void createHeightmap();
  void importHeightmap();
  void reimportHeightmap();
  bool checkAndReimport(String &path, FileChangeStat &lastChangeOld, HeightmapTypes is_det);
  bool importHeightmap(String &filename, HeightmapTypes type);
  void eraseHeightmap();
  void importWaterHeightmap(bool det);
  void eraseWaterHeightmap();
  void autocenterHeightmap(PropPanel::ContainerPropertyControl *panel, bool reset_render = true);

  void exportHeightmap();
  bool exportHeightmap(String &filename, real min_height, real height_range, bool exp_det_hmap);

  void exportColormap();
  bool exportColormap(String &filename);

  void exportLand();
  bool exportLand(String &filename);
  bool exportLand(mkbindump::BinDumpSaveCB &cwr); // throws exception on error

  void exportSplines(mkbindump::BinDumpSaveCB &cwr);
  void exportLoftMasks(const char *out_folder, int main_hmap_sz, int det_hmap_sz, float hmin, float hmax, int prefab_dest_idx);

  bool exportLandMesh(mkbindump::BinDumpSaveCB &cwr, IWriterToLandmesh *land_modifier, LandRayTracer *raytracer,
    bool tools_internal = false);

  void generateLandColors(const IBBox2 *in_rect = NULL, bool finished = true);

  void onLandRegionChanged(int x0, int y0, int x1, int y1, bool recalc_all = false, bool finished = true);
  void onWholeLandChanged();
  void onLandSizeChanged();

  bool getAllSunSettings();
  bool getDirectionSunSettings();
  template <bool cast_shadows, bool dxt_high>
  bool calcLandLighting(const IBBox2 *calc_box = NULL);
  void calcFastLandLighting();
  void calcGoodLandLighting();
  void blurLightmap(int kernel_size, float sigma, const IBBox2 *calc_box = NULL);
  void recalcLightingInRect(const IBBox2 &rect);
  void updateRendererLighting();

  void updateLandOnPaint(Brush *brush, bool finished);
  void delayedResetRenderer();
  void rebuildLandmeshDump();
  void rebuildLandmeshManager();
  void rebuildLandmeshPhysMap();

  template <class T>
  void loadMapFile(MapStorage<T> &map, const char *filename, CoolConsole &con);

  template <class T>
  void createMapFile(MapStorage<T> &map, const char *filename, CoolConsole &con);

  void createWaterHmapFile(CoolConsole &con, bool det);

  bool generateLandMeshMap(LandMeshMap &map, CoolConsole &con, bool import_sgeom, LandRayTracer **out_tracer,
    bool strip_det_hmap_from_tracer = true);

  void rebuildWaterSurface(Tab<Point3> *loft_pt_cloud = NULL, Tab<Point3> *water_border_polys = NULL,
    Tab<Point2> *hmap_sweep_polys = NULL);

  bool compileAndRunColorGenScript(bool &was_inited, SQPRINTFUNCTION &old_print_func, SQPRINTFUNCTION &old_err_func);
  void closeColorGenScript(bool &was_inited, SQPRINTFUNCTION &old_print_func, SQPRINTFUNCTION &old_err_func);
  bool createDefaultScript(const char *path) const;

  bool getColorGenVarsFromScript();

  bool loadGenLayers(const DataBlock &blk);
  bool saveGenLayers(DataBlock &blk);
  void regenLayerTex();
  void storeLayerTex();

  // void prepareDetailTexTileAndOffset(Tab<float> &out_tile, Tab<Point2> &out_offset);

  void renderHeight();
  void renderGrassMask();
#if defined(USE_HMAP_ACES)
  bool getRenderNoTex() { return !(showBlueWhiteMask && editedScriptImage); }
#else
  bool getRenderNoTex() { return false; }
#endif

  void updateVertTex();
  void acquireVertTexRef();
  void releaseVertTexRef();

  void updateHorizontalTex();
  void acquireHorizontalTexRef();
  void releaseHorizontalTexRef();
  void saveHorizontalTex(DataBlock &blk);
  void loadHorizontalTex(const DataBlock &blk);

  void updateLandModulateColorTex();
  void acquireLandModulateColorTexRef();
  void releaseLandModulateColorTexRef();
  void saveLandModulateColorTex(DataBlock &blk);
  void loadLandModulateColorTex(const DataBlock &blk);

  void setRendinstlayeredDetailColor();
  void saveRendinstDetail2Color(DataBlock &blk);
  void loadRendinstDetail2Color(const DataBlock &blk);

  void updateHtLevelCurves();
  void applyClosingPostProcess(ExtendedDecalBitmap &accum_bitmap);
  PackedDecalBitmap to_1bit_bitmap(const ExtendedDecalBitmap &accum_bitmap);

  class ScriptImage : public IBitMaskImageMgr::BitmapMask, private IFileChangedNotify
  {
  public:
    ScriptImage(const char *name);
    ~ScriptImage();

    bool loadImage();
    bool saveImage();

    const char *getName() const { return name; }

    E3DCOLOR sampleImageUV(real x, real y, bool clamp_x, bool clamp_y);
    E3DCOLOR sampleImagePixelUV(real x, real y, bool clamp_x, bool clamp_y);
    float sampleMask1UV(real x, real y);
    float sampleMask1PixelUV(real x, real y);
    float sampleMask8UV(real x, real y);
    float sampleMask8PixelUV(real x, real y);

    float sampleMask1UV(real x, real y, bool clamp_x, bool clamp_y);
    float sampleMask8UV(real x, real y, bool clamp_x, bool clamp_y);

    void paintImageUV(real x0, real y0, real x1, real y1, bool clamp_x, bool clamp_y, E3DCOLOR color);
    void paintMask1UV(real x, real y, bool val);
    void paintMask8UV(real x, real y, char val);

    bool isImageModified() const { return isModified; }

  protected:
    SimpleString name;
    bool isModified;
    bool isSaved;
    int fileNotifyId;

    inline void calcClampMapping(float &fx, float &fy, int &ix, int &iy, int &nx, int &ny);
    inline void calcClampMapping(float fx, float fy, int &ix, int &iy);
    inline void calcMapping(float &fx, float &fy, int &ix, int &iy, int &nx, int &ny, bool clamp_u, bool clamp_v);
    void registerFnotify();
    virtual void onFileChanged(int /*file_name_id*/); // IFileChangedNotify
  };

  Tab<ScriptImage *> scriptImages;


  SimpleString horizontalTexName;
  SimpleString horizontalDetailTexName;
  real horizontalTex_TileSizeX, horizontalTex_TileSizeY;
  real horizontalTex_OffsetX, horizontalTex_OffsetY;
  real horizontalTex_DetailTexSizeX, horizontalTex_DetailTexSizeY;
  bool useHorizontalTex;

  SimpleString landModulateColorTexName;
  real landModulateColorEdge0, landModulateColorEdge1;
  bool useLandModulateColorTex;

  bool useRendinstDetail2Modulation;
  E3DCOLOR rendinstDetail2ColorFrom, rendinstDetail2ColorTo;

  real tileXSize, tileYSize;
  SimpleString tileTexName;
  TEXTUREID tileTexId;

  bool useVertTex;
  bool useVertTexForHMAP;
  SimpleString vertTexName;
  SimpleString vertNmTexName;
  SimpleString vertDetTexName;
  float vertTexXZtile, vertTexYtile, vertTexYOffset, vertTexAng0, vertTexAng1, vertTexHorBlend;
  float vertDetTexXZtile, vertDetTexYtile, vertDetTexYOffset;

  char detLayerDescStorage[8];
  char impLayerDescStorage[8];
  int detLayerIdx;
  int impLayerIdx;

  bool syncLight, syncDirLight, pendingResetRenderer;
  bool requireTileTex, hasColorTex, hasLightmapTex, useMeshSurface;
  bool useNormalMap;
  bool storeNxzInLtmapTex;
  GenHmapData *genHmap;

  Ptr<RoadsSnapshot> roadsSnapshot;
  static int hmapSubtypeMask;
  static int lmeshSubtypeMask;
  static int lmeshDetSubtypeMask;
  static int grasspSubtypeMask;
  static int navmeshSubtypeMask;

  float meshPreviewDistance;
  // float meshCellSize;
  int meshCells;
  float meshErrorThreshold;
  int numMeshVertices;
  int lod1TrisDensity;
  float importanceMaskScale;

  bool distFieldInvalid;
  int distFieldBuiltAt;

  enum
  {
    EXPORT_PSEUDO_PLANE,
    EXPORT_HMAP,
    EXPORT_LANDMESH,
  };

  int exportType;
  bool debugLmeshCells;

  bool snowDynPreview, snowSpherePreview;
  float ambSnowValue, dynSnowValue;
  int snowValSVId, snowPrevievSVId;
  bool calculating_shadows; // calculating shadows - now

  DebugPrimitivesVbuffer *navMeshBuf;
  DebugPrimitivesVbuffer *coversBuf;
  DebugPrimitivesVbuffer *contoursBuf;
  DebugPrimitivesVbuffer *obstaclesBuf;

  Tab<PhysMap *> physMaps;

  bool bothPropertiesAndObjectPropertiesWereOpenLastTime = false;

  Tab<NavmeshAreasProcessing> navmeshAreasProcessing;
};
