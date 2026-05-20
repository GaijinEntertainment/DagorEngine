// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "Brushes/hmlBrush.h"

#include <EditorCore/ec_brush.h>
#include <EditorCore/ec_ObjectEditor.h>

#include <oldEditor/de_interface.h>
#include <oldEditor/de_collision.h>
#include <de3_assetService.h>
#include <de3_bitMaskMgr.h>
#include <de3_hmapStorage.h>
#include "blockedDetTexMap.h"
#include <de3_heightmap.h>
#include <de3_roadsProvider.h>
#include <de3_genObjHierMask.h>
#include <de3_fileTracker.h>
#include <de3_landmesh.h>
#include <math/dag_color.h>
#include <EASTL/bitset.h>
#include <generic/dag_span.h>

#include <3d/dag_texMgr.h>

#include <landMesh/lmeshRenderer.h>
#include <landMesh/lmeshManager.h>
#include "landMeshMap.h"

#include "hmlObjectsEditor.h"
#include "hmlLayers.h"
#include "hmlTypedVars.h"
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
                       public IPluginBeforeClose,
                       public PropPanel::ControlEventHandler,
                       public IWndManagerWindowHandler,
                       public IPostProcessGeometry,
#if defined(USE_HMAP_ACES)
                       public IEnvironmentSettings,
#endif
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
  static bool includeHiddenLayersInExportedNavMesh;
  static bool useASTC(unsigned tc) { return tc == _MAKE4C('iOS') || (tc == _MAKE4C('PC') && pcPreferASTC); }

  static IHmapService *hmlService;
  static IBitMaskImageMgr *bitMaskImgMgr;
  static IDagorPhys *dagPhys;
  static ISplineGenService *splSrv;

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
  ~HmapLandPlugin() override;

  // IGenEditorPlugin
  const char *getInternalName() const override { return "heightmapLand"; }
  const char *getMenuCommandName() const override { return "Landscape"; }
  const char *getHelpUrl() const override { return "/html/Plugins/HeightmapLand/index.htm"; }

  int getRenderOrder() const override { return 100; }
  int getBuildOrder() const override { return 0; }

  bool showSelectAll() const override { return true; }
  bool showInTabs() const override { return true; }

  void registered() override;
  void unregistered() override;
  void loadSettings(const DataBlock &global_settings, const DataBlock &per_app_settings) override;
  void saveSettings(DataBlock &global_settings, DataBlock &per_app_settings) override;
  void beforeMainLoop() override;

  void registerMenuSettings(unsigned menu_id, int base_id) override;
  void registerEditorCommands(IEditorCommandSystem &command_system) override;
  void registerMenuAccelerators() override;
  bool begin(int toolbar_id, unsigned menu_id) override;
  bool end() override;
  IGenEventHandler *getEventHandler() override;

  void setVisible(bool vis) override;
  bool getVisible() const override { return isVisible; }
  bool getSelectionBox(BBox3 &box) const override;
  bool getStatusBarPos(Point3 &pos) const override { return false; }

  void clearObjects() override;
  void onNewProject() override;
  void autoSaveObjects(DataBlock &local_data) override;
  void beforeClose() override;
  void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) override;
  void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path) override;
  bool acceptSaveLoad() const override { return true; }

  void selectAll() override { objEd.selectAll(); }
  void deselectAll() override { objEd.unselectAll(); }
  void invertSelection() override { objEd.invertSelection(); }
  void invalidateObjectProps() { objEd.invalidateObjectProps(); }

  void actObjects(float dt) override;
  void beforeRenderObjects(IGenViewportWnd *vp) override;
  void renderObjects() override;
  void renderTransObjects() override;
  void updateImgui() override;
  void renderGrass(Stage stage);
  void renderGPUGrass(Stage stage);
  void renderWater(Stage stage);
  void renderCables();

  void replaceGrassMask(int landclass_id, const char *newGrassMaskName);
  void resetGrassMask(const DataBlock &grassBlk);
  int getLandclassIndex(const char *landclass_name);
  bool catchEvent(unsigned ev_huid, void *userData) override;

  void *queryInterfacePtr(unsigned huid) override;

  bool onPluginMenuClick(unsigned id) override;
  bool onSettingsMenuClick(unsigned id) override;
  void handleViewportAcceleratorCommand(unsigned id) override;

  // ControlEventHandler
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChangeFinished(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  // IWndManagerWindowHandler
  void *onWmCreateWindow(int type) override;
  bool onWmDestroyWindow(void *window) override;

  // IRenderingService
  void renderGeometry(Stage stage) override;
  void prepare(const Point3 &center_pos, const BBox3 &box) override;

  // IGenEventHandler
  IGenEventHandler *getWrappedHandler() override { return &objEd; }

  // IBrushClient
  void onBrushPaintStart(Brush *brush, int buttons, int key_modif) override;
  void onBrushPaintEnd(Brush *brush, int buttons, int key_modif) override;
  void onBrushPaint(Brush *brush, const Point3 &center, const Point3 &prev_center, const Point3 &normal, int buttons,
    int key_modif) override;
  void onRBBrushPaintStart(Brush *brush, int buttons, int key_modif) override;
  void onRBBrushPaintEnd(Brush *brush, int buttons, int key_modif) override;
  void onRBBrushPaint(Brush *brush, const Point3 &center, const Point3 &prev_center, const Point3 &normal, int buttons,
    int key_modif) override;

  // IBinaryDataBuilder
  bool validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *params) override;
  bool addUsedTextures(ITextureNumerator &tn) override;
  bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *pp) override;
  bool checkMetrics(const DataBlock &metrics_blk) override { return true; }

  // IWriteAddLtinputData
  void writeAddLtinputData(IGenSave &cwr) override;

  // ILandmesh
  BBox3 getBBoxWithHMapWBBox() const override;
  bool isLandmeshRenderingMode() const override;

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
  Point3 getOffset() const;
  float getRIMaxCellSz() const { return riMaxCellSz; }
  int getRIMaxGenLayerCellDiv() const { return riMaxGenLayerCellDivisor; }

  void readLandDetailTexturePixel(unsigned &ret_u, unsigned &ret_u2, int cx, int cy, dag::ConstSpan<uint8_t> type_remap);

  // Loads Land Datail Texture for LandMesh
  void loadLandDetailTexture(int x0, int y0, Texture *tex1, Texture *tex2, carray<uint8_t, LMAX_DET_TEX_NUM> &detail_tex_ids,
    bool *done_mark, int tex_size, int elem_size);

  int loadLandDetailTexture(unsigned targetCode, int x0, int y0, char *imgPtr, int stride, char *imgPtr2, int stride2,
    carray<uint8_t, LMAX_DET_TEX_NUM> &detail_tex_ids, bool *done_mark, int size, int elem_size, bool tex1_rgba, bool tex2_rgba);

  int getMostUsedDetTex(int x0, int y0, int texDataSize, uint8_t *det_tex_ids, uint8_t *idx_remap, int max_dtn);

  // from IGatherStaticGeometry
  void gatherStaticVisualGeometry(StaticGeometryContainer &cont) override
  {
    gatherStaticGeometry(cont, StaticGeometryNode::FLG_RENDERABLE, false);
  }

  void gatherStaticCollisionGeomGame(StaticGeometryContainer &cont) override;

  void gatherStaticCollisionGeomEditor(StaticGeometryContainer &cont) override { gatherStaticGeometry(cont, 0, true); }

  void gatherStaticEnviGeometry(StaticGeometryContainer &container) override {}
  void gatherStaticGeometry(StaticGeometryContainer &cont, int flags, bool collision, int stage = 0)
  {
    objEd.gatherStaticGeometry(cont, flags, collision, stage);
  }

  // IDagorEdCustomCollider
  bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) override;
  bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt) override;
  const char *getColliderName() const override { return "HeightMap"; }
  bool isColliderVisible() const override;

  // IHmapBrushImage
  real getBrushImageData(int x, int y, IHmapBrushImage::Channel channel) override;
  void setBrushImageData(int x, int y, real v, IHmapBrushImage::Channel channel) override;

#if defined(USE_HMAP_ACES)
  // IEnvironmentSettings.
  void getEnvironmentSettings(DataBlock &blk) override;
  void setEnvironmentSettings(DataBlock &blk) override;
#endif

  // IExportToDag.
  void gatherExportToDagGeometry(StaticGeometryContainer &container) override
  {
    if (!useMeshSurface || exportType == EXPORT_PSEUDO_PLANE)
      return;

    if (landMeshMap.isEmpty())
      generateLandMeshMap(landMeshMap, DAGORED2->getConsole(), false, NULL);

    landMeshMap.gatherExportToDagGeometry(container);
  }


  bool traceRayPrivate(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm);

  void createHeightmapFile(CoolConsole &con);
  void upscaleHeightMap(CoolConsole &con);
  void resizeHeightMapDet(CoolConsole &con);
  void createColormapFile(CoolConsole &con);
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
  int getScriptImageWidth(int id) const;
  int getScriptImageHeight(int id) const;
  int getScriptImageBpp(int id) const;
  E3DCOLOR sampleScriptImageUV(int id, real x, real y, bool clamp_x, bool clamp_y) const;
  E3DCOLOR sampleScriptImagePixelUV(int id, real x, real y, bool clamp_x, bool clamp_y) const;
  bool saveImage(int idx);

  void paintScriptImageUV(int id, real x0, real y0, real x1, real y1, bool clamp_x, bool clamp_y, E3DCOLOR color);

  float sampleMask1UV(int id, real x, real y) const;
  float sampleMask1UV(int id, real x, real y, bool clamp_x, bool clamp_y) const;
  float sampleMask8UV(int id, real x, real y) const;
  float sampleMask8UV(int id, real x, real y, bool clamp_x, bool clamp_y) const;
  float sampleMask1PixelUV(int id, real x, real y) const;
  float sampleMask8PixelUV(int id, real x, real y) const;
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
  bool isLoaded() const override { return heightMap.isFileOpened(); }
  real getHeightmapCellSize() const override { return gridCellSize; }
  int getHeightmapSizeX() const override { return heightMap.getMapSizeX(); }
  int getHeightmapSizeY() const override { return heightMap.getMapSizeY(); }

  bool getHeightmapPointHt(Point3 &inout_p, Point3 *out_norm) const override;
  bool getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid) const override;

  Point3 getHeightmapOffset() const override { return Point3(heightMapOffset[0], 0, heightMapOffset[1]); }

  bool getHeightmapHeight(const IPoint2 &cell, real &ht) const;
  bool getHeightmapOnlyPointHt(Point3 &inout_p, Point3 *out_norm) const;

  // IRoadsProvider
  IRoadsProvider::Roads *getRoadsSnapshot() override;

  bool applyHmModifiers1(HeightMapStorage &hm, float gc_sz, bool gen_colors, bool reset_final, IBBox2 *out_dirty_clip,
    IBBox2 *out_sum_dirty, bool *out_colors_changed);
  void applyHmModifiers(bool gen_colors = true, bool reset_final = true, bool finished = true);
  void collapseModifiers(dag::ConstSpan<SplineObject *> collapse_splines);
  void convertDelaunaySplinesToHeightBake();
  bool shouldApplyModOnHeightBakeSplineEdit() const { return applyHeightBakeSplinesOnEdit; }
  void setApplyModOnHeightBakeSplineEdit(bool new_runtime_update) { applyHeightBakeSplinesOnEdit = new_runtime_update; }

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
  void onLightingChanged() override {}
  void onLightingSettingsChanged() override;

  // IOnExportNotify interface
  void onBeforeExport(unsigned target_code) override;
  void onAfterExport(unsigned target_code) override;

  // IPostProcessGeometry
  void processGeometry(StaticGeometryContainer &container) override;

  //! returns number of invisible faces, and marked faces to stay
  int markUndergroundFaces(MeshData &mesh, Bitarray &facesAbove, TMatrix *wtm);
  //! removes faces below ground
  void removeInvisibleFaces(StaticGeometryContainer &container);


  const HmapBitLayerDesc &getDetLayerDesc() const { return (HmapBitLayerDesc &)detLayerDescStorage; }
  const HmapBitLayerDesc &getImpLayerDesc() const { return (HmapBitLayerDesc &)impLayerDescStorage; }
  void *getLayersHandle() const { return landClsLayersHandle; }
  int getDetLayerIdx() const { return detLayerIdx; }
  MapStorage<uint32_t> &getlandClassMap() { return landClsMap; }
  bool isLandClsMapGenerated() const { return landClsMapGenerated; }

  hmap_storage::BlockedDetTexMap *getDetTexMap() const { return detTexMap; }
  void prepareDetTexMaps();

  LandClassSlotsManager &getLandClassMgr() { return *lcMgr; }
  bool setDetailTexSlot(int s, const char *blk_name);
  // True if any genLayer targets this detail-tex slot with writeDetTex
  // enabled; false if the slot is registered (has a name) but every layer
  // feeding it is land-only (writeDetTex=false). Used by the exporter to
  // tell "legitimately no detTex writes" apart from "masks/thresholds drove
  // every pixel to zero for a detTex-writing layer".
  bool isDetTexSlotWritten(int slot_idx) const;

  bool exportLightmapToFile(const char *file_name, int target_code, bool high_quality);

  bool rebuildSplinesPolyBitmask(BBox2 &out_dirty_box);
  bool rebuildSplinesLoftBitmask(int layer, BBox2 &out_dirty_box);
  void rebuildSplinesBitmask(bool auto_regen = true);

  void rebuildRivers();

  bool buildAndWriteSingleNavMesh(BinDumpSaveCB &cwr, int nav_mesh_idx, bool final_export_to_game = true);
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

  bool isShowingLandClassColors() const { return showLandClassColors; }
  void refreshLandClassColorsTex() { updateBlueWhiteMask(nullptr); }

  void refillPanel(bool schedule_regen = false);

  void prepareEditableLandClasses();
  void addGenLayer(const char *name, int insert_before = -1);
  bool moveGenLayer(ScriptParam *gl, bool up);
  bool delGenLayer(ScriptParam *gl);
  // Returns true if some other gen layer already has this paramName. Empty/null names
  // are never considered "in use" so multiple unnamed layers can coexist (their masks
  // are not exposed via mask_<name> anyway). `exclude` skips one layer from the scan,
  // used by rename so renaming to the same name is a no-op rather than a self-conflict.
  bool isGenLayerNameInUse(const char *name, const ScriptParam *exclude = nullptr) const;
  // Returns true if `name` is acceptable for a gen layer's paramName. Empty names
  // are allowed (placeholder layers); non-empty names must consist solely of
  // [A-Za-z0-9_] so the derived mask_<name> binding is a valid identifier and can
  // be referenced from peer / common expressions. Names with spaces or punctuation
  // would otherwise be silently unreferenceable.
  static bool isValidGenLayerName(const char *name);

  // Snapshot every gen layer's current exprValid into `out` (parallel index). Used by
  // the Apply paths together with findFirstRegressedGenLayer to detect a "valid layer
  // becomes invalid" regression after a recompile and roll the change back.
  void snapshotGenLayerValidity(Tab<bool> &out) const;
  // Returns the index of the first gen layer whose exprValid was true in `wasValid`
  // but is false now (filling outName/outErr with that layer's paramName / lastErr),
  // or -1 if no regression. wasValid is expected to have exactly genLayers.size()
  // entries (older snapshots from a different layer count are tolerated by clamping).
  int findFirstRegressedGenLayer(const Tab<bool> &wasValid, SimpleString &outName, SimpleString &outErr) const;
  void rebuildLandSlots();
  // Resync the shared varMap and recompile every gen-layer expression (and the common
  // expression). add/move/del call this internally; entry point for external callers
  // (rename, Apply, slider drag). shrink_arenas=false is the per-keystroke path.
  void recompileGenLayerExpressions(bool shrink_arenas = true);
  // Common-expression accessors (multi-line edit text). The setter stores newline-free
  // text; getCommonExprDisplayText re-inserts `\n` after every `;` so the multi-line
  // edit shows one statement per line. Setter also triggers a recompile.
  const SimpleString &getCommonExprText() const { return commonExprText; }
  void setCommonExprText(const char *txt);
  eastl::string getCommonExprDisplayText() const;

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

  PropPanel::ContainerPropertyControl *getPropPanel() const;

  bool insideDetRectC(int x, int y) const
  {
    return x >= detRectC[0].x && x < detRectC[1].x && y >= detRectC[0].y && y < detRectC[1].y;
  }
  bool insideDetRectC(const IPoint2 &p) const { return insideDetRectC(p.x, p.y); }

  bool loadLevelSettingsBlk(DataBlock &level_blk);
  void initWaterSurface();
  void updateWaterSettings(const DataBlock &level_blk);
  void invalidateDistanceField() { distFieldInvalid = true; }

  void onLayersDlgClosed();
  void selectLayerObjects(int lidx, bool sel = true);
  void moveObjectsToLayer(int lidx, dag::Span<RenderableEditableObject *> objects);

  void onObjectSelectionChanged(RenderableEditableObject *obj);
  void onObjectsRemove();

  bool runLandLtmapCmd(dag::ConstSpan<const char *> params);

private:
  // Caches datailed textures for LandMesh
  int toolbarId;

  // IAssetUpdateNotify
  void onLandClassAssetChanged(landclass::AssetData *data) override {}
  void onLandClassAssetTexturesChanged(landclass::AssetData *data) override;
  void onSplineClassAssetChanged(splineclass::AssetData *data) override {}

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
    real canyonAngle, canyonFadeAngle;
    real hm2YbaseForLod;

    int landTexSize, landTexElemSize;
    int detMapSize, detMapElemSize;

    int hm2displacementQ;
    bool showFinalHM;
    bool useMetricsHM;
    bool useHm2Mirror;

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
  // True once generateLandColors has populated landClsMap (and, when enabled,
  // colorMap / detTexMap) for the current project open session. Needed
  // because resizeLandClassMapFile sizes the map to heightMap*lcmScale up
  // front -- so a getMapSizeX()>0 check passes even for a freshly sized but
  // empty map. Export guards and LandClassSlotsManager::reinitRIGen check
  // this bit to avoid acting on the pre-generate zero state. Cleared by
  // loadObjects and eraseHeightmap; set at the end of generateLandColors.
  bool landClsMapGenerated = false;
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

  bool applyHeightBakeSplines = true;
  bool applyHeightBakeSplinesOnEdit = false;

  hmap_storage::BlockedDetTexMap *detTexMap = nullptr;

  int detDivisor;
  HeightMapStorage heightMapDet;
  BBox2 detRect;
  IBBox2 detRectC;
  Texture *hmapTex[2];
  TEXTUREID hmapTexId[2];

  int numDetailTextures;
  Tab<SimpleString> detailTexBlkName;

  String lastHmapImportPath, lastLandExportPath, lastHmapExportPath, lastColormapExportPath, lastWaterHeightmapExportPath,
    lastTexImportPath, lastGATExportPath, lastWaterHeightmapImportPath, lastHmapImportPathDet, lastHmapImportPathMain,
    lastWaterHeightmapImportPathDet, lastWaterHeightmapImportPathMain;
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
  // Artist-editable typed parameters (Float / Range / Mask / Curve). Persisted
  // under sibling block "landVars" of the genLayers BLK, surfaced in the property
  // panel for add/delete and live edits. recompileGenLayerExpressions registers
  // each entry into the shared landClassEval varMap with its type, builds
  // evalCurves from Curve entries, and populates typedVarRuntime so the bake
  // loop can write the per-pixel value into exprVars[]. Mask resolves its asset
  // name to a script-image index at recompile and samples per-pixel via the
  // shared sampleMask*UV path.
  Tab<TypedVar> typedVars;

  // Per-typedVar runtime binding rebuilt on every recompile, parallel-indexed to
  // typedVars. Definition lives in hmlTypedVars.h so free helpers in
  // hmlGenColorMap.cpp can take it by reference without poking at private state.
  Tab<TypedVarRuntime> typedVarRuntime;

  // CubicCurveSampler array fed to LcExprCurveNode via EvalCtx::userCtx. Index i
  // is the slot stored as the float value of a Curve-typed var; CurveNode reads
  // that float, casts to int, and samples evalCurves[idx]. Rebuilt from
  // typedVars at every recompile; out-of-sync indices return 0 at eval time.
  Tab<CubicCurveSampler> evalCurves;
  // Optional "common expression": run once per pixel before any layer's expression so
  // multiple layers can share intermediate values via `var name = ...`. Source text is
  // stored as a single line in the .blk (newlines stripped on save, `;` -> `;\n` on
  // load); commonExprArena holds the compiled bytecode and commonExprValid gates eval.
  // Arena typed as Tab<uint8_t> (same as lcexpr::Arena) to avoid pulling landClassEval
  // headers into this one. The bitset width 256 mirrors lcexpr::MAX_VAR_ID; a
  // static_assert in hmlGenColorMap.cpp pins the two values together.
  SimpleString commonExprText;
  Tab<uint8_t> commonExprArena;
  uint32_t commonExprRoot = 0;
  eastl::bitset<256> commonExprVarMask;
  bool commonExprValid = false;
  // Last parse/compile error from compile_common_into; empty on success or empty text.
  // Read by the common-expression Apply handler to surface the error in a message box.
  SimpleString commonExprLastErr;
  // Stashed by recompileGenLayerExpressions(): bake-time sizing for the per-pixel
  // exprVars[] buffer and the evalFinite() bound.
  //   commonExprNv     -- nv after compiling base + mask_<enabled_name> + common's
  //                       var/let. Boundary between common-visible slots (shared
  //                       across all layers) and per-layer private user-var slots.
  //   lcMaxNv          -- max named-vars count used by any enabled layer's bytecode,
  //                       PLUS lcexpr::MAX_TEMP_REGS reserved unconditionally for
  //                       `{ block }`-scoped temp regs (caller-side simplification:
  //                       cheaper to always reserve 16 floats than to track each
  //                       expression's actual peak). Per-pixel exprVars[] is sized
  //                       to max(VAR_COUNT + numLayers, lcMaxNv).
  //   lcNumLayerVars   -- genLayers.size() at recompile time -- defines the per-layer
  //                       mask slot range [VAR_COUNT, VAR_COUNT + lcNumLayerVars).
  uint16_t commonExprNv = 0;
  uint16_t lcMaxNv = 0;
  int lcNumLayerVars = 0;
  bool showBlueWhiteMask;
  bool showLandClassColors = false;
  void updateBlueWhiteMask(const IBBox2 *);
  void updateLandClassColorsTex();
  // Shared per-pixel exprVars population + commonExpr eval, factored out of
  // generateLandColors so other consumers (e.g. a debug overlay rebuild) can
  // hit the same code path without copy-pasting the inner loop. layer_mask_needed
  // and used_mask_tvs are passed as ConstSpan so any small-buffer-optimised
  // (RelocatableFixedVector / StaticTab) or heap-backed (Tab) container can be
  // passed verbatim. See implementation comment in hmlGenColorMap.cpp.
  void fillPerPixelExprVarsAndEvalCommon(float *expr_vars_ptr, float fx, float fy, int layer_slots,
    dag::ConstSpan<uint8_t> layer_mask_needed, uint16_t typed_vars_end_nv, dag::ConstSpan<int> used_mask_tvs);
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

  void exportHeightmap(bool export_detailed);
  bool exportHeightmap(String &filename, real min_height, real height_range, bool exp_det_hmap);

  void exportColormap();
  bool exportColormap(String &filename);

  void exportWaterHeightmap(bool export_detailed);
  bool exportWaterHeightmap(const String &filename, real min_height, real height_range, bool export_detailed);

  void exportLand();
  bool exportLand(String &filename);
  bool exportLand(mkbindump::BinDumpSaveCB &cwr); // throws exception on error

  void exportSplines(mkbindump::BinDumpSaveCB &cwr);
  void exportLoftMasks(const char *out_folder, int main_hmap_sz, int det_hmap_sz, float hmin, float hmax, int prefab_dest_idx);

  bool exportLandMesh(mkbindump::BinDumpSaveCB &cwr, IWriterToLandmesh *land_modifier, LandRayTracer *raytracer,
    bool tools_internal = false);

  void generateLandColors(const IBBox2 *in_rect = NULL, bool finished = true, bool may_rebuild_lmesh_if_needed = true);

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

  bool loadGenLayers(const DataBlock &blk);
  bool saveGenLayers(DataBlock &blk);

  // Scan every layer's exprText and the common expression for identifier-boundary
  // references to the typed-var's parser-visible name(s). Returns true if any
  // call sites were found, with human-readable descriptions in `outSites`. For
  // Range, scans both <name>_lo and <name>_hi. Used by the Delete button to
  // refuse removing a variable still in use.
  bool scanTypedVarRefs(const TypedVar &v, Tab<String> &outSites) const;

  // True if `name` would collide with a name the recompile path will register
  // outside the typed-var slot range. Covers the 6 fixed builtins (height,
  // angle, curvature, mask, world_x, world_y) and every mask_<layer> binding
  // for the current genLayers list. Used by the Add dialog so the artist hits
  // a clear error up front instead of silently aliasing an existing slot when
  // recompileGenLayerExpressions runs.
  //
  // Public so the Add dialog (free function in hmlTypedVars.cpp) can call it
  // through HmapLandPlugin::self.
public:
  bool isReservedVarName(const char *name) const;

  // True if `name` is declared as `var <name>` or `let <name>` somewhere in
  // commonExprText or any layer's exprText. lcexpr's parser rejects redeclaring
  // a name already in the shared varMap, so a typed var sharing this name would
  // make the next recompile fail. The Add dialog calls this so the artist hits
  // a clear up-front error instead of seeing a recompile failure after the
  // panel has already mutated. `outSite` returns a human-readable origin
  // (`common expression` or `layer '<name>' expression`).
  bool findVarLetDeclSite(const char *name, SimpleString &outSite) const;

  // True if `probe` matches any current typed var's parser-visible name (bare
  // name for non-Range, <name>_lo / <name>_hi for Range). On a hit `outConflict`
  // returns the offending typed-var's display name. Used by the typed-var Add
  // dialog and by the layer add/rename path (probe = "mask_<layerName>") so a
  // collision is caught before the change commits.
  bool collidesWithTypedVar(const char *probe, SimpleString &outConflict) const;

private:
  // Live-rebuild the CubicCurveSampler at evalCurves[typedVarRuntime[row].curveIdx]
  // from typedVars[row]. Called from onChange when a curve var's control points or
  // approximation type are edited so the next bake samples the updated shape
  // without paying for a full recompileGenLayerExpressions. Safe no-op when the
  // row is not a Curve, has no runtime binding, or the curveIdx is out of range.
  void rebuildEvalCurveForRow(int row);
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

    E3DCOLOR sampleImageUV(real x, real y, bool clamp_x, bool clamp_y) const;
    E3DCOLOR sampleImagePixelUV(real x, real y, bool clamp_x, bool clamp_y) const;
    float sampleMask1UV(real x, real y) const;
    float sampleMask1PixelUV(real x, real y) const;
    float sampleMask8UV(real x, real y) const;
    float sampleMask8PixelUV(real x, real y) const;

    float sampleMask1UV(real x, real y, bool clamp_x, bool clamp_y) const;
    float sampleMask8UV(real x, real y, bool clamp_x, bool clamp_y) const;

    void paintImageUV(real x0, real y0, real x1, real y1, bool clamp_x, bool clamp_y, E3DCOLOR color);
    void paintMask1UV(real x, real y, bool val);
    void paintMask8UV(real x, real y, char val);

    bool isImageModified() const { return isModified; }

  protected:
    SimpleString name;
    bool isModified;
    bool isSaved;
    int fileNotifyId;

    inline void calcClampMapping(float &fx, float &fy, int &ix, int &iy, int &nx, int &ny) const;
    inline void calcClampMapping(float fx, float fy, int &ix, int &iy) const;
    inline void calcMapping(float &fx, float &fy, int &ix, int &iy, int &nx, int &ny, bool clamp_u, bool clamp_v) const;
    void registerFnotify();
    void onFileChanged(int /*file_name_id*/) override; // IFileChangedNotify
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
  float riMaxCellSz = 2048.f;
  int riMaxGenLayerCellDivisor = 8;

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

  int settingsBaseId;
};
