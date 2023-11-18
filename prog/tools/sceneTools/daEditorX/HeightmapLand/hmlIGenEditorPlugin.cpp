#include "hmlPlugin.h"
#include "hmlCm.h"
#include "hmlPanel.h"

#include "hmlExportSizeDlg.h"
#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include "hmlEntity.h"

#include "Brushes/hmlBrush.h"
#include "landClassSlotsMgr.h"

#include <de3_interface.h>
#include <de3_entityFilter.h>
#include <de3_hmapService.h>
#include <de3_lightService.h>
#include <de3_lightProps.h>
#include <de3_genHmapData.h>
#include <de3_editorEvents.h>
#include <de3_landClassData.h>
#include <de3_skiesService.h>
#include <de3_rendInstGen.h>
#include <de3_entityUserData.h>
#include <de3_prefabs.h>
#include <assets/asset.h>

#include <dllPluginCore/core.h>

#include <libTools/util/iLogWriter.h>
#include <libTools/util/colorUtil.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/util/makeBindump.h>
#include "editorLandRayTracer.h"

#include <3d/dag_render.h>
#include <3d/dag_drv3d.h>
#include <render/dag_cur_view.h>
#include <perfMon/dag_cpuFreq.h>

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_findFiles.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <ioSys/dag_btagCompr.h>
#include <osApiWrappers/dag_direct.h>
#include <fftWater/fftWater.h>

#include <coolConsole/coolConsole.h>

#include <heightMapLand/dag_hmlTraceRay.h>

#include <debug/dag_debug3d.h>
#include <shaders/dag_shaderMesh.h>

#include <debug/dag_debug.h>

#include <oldEditor/de_workspace.h>
#include "common.h"
#include <image/dag_texPixel.h>
#include <ioSys/dag_memIo.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <shaders/dag_shaderBlock.h>
#include <image/dag_tga.h>
#include <image/dag_tiff.h>
#include <obsolete/dag_cfg.h>

#include <math/dag_color.h>
#include <stdio.h>
#include <util/dag_texMetaData.h>
#include <util/dag_fastIntList.h>
#include <util/dag_stlqsort.h>
#include <util/dag_parallelForInline.h>

#include <winGuiWrapper/wgw_dialogs.h>

#include <propPanel2/c_control_event_handler.h>
#include <propPanel2/c_panel_base.h>
#include <propPanel2/comWnd/dialog_window.h>
#include <heightmap/heightmapHandler.h>
#include "renderLandNormals.h"

#include <libTools/util/binDumpTreeBitmap.h>

using hdpi::_pxScaled;

extern bool allow_debug_bitmap_dump;


#define HEIGHTMAP_FILENAME            "heightmap.dat"
#define LANDCLSMAP_FILENAME           "landclsmap.dat"
#define COLORMAP_FILENAME             "colormap.dat"
#define LIGHTMAP_FILENAME             "lightmap.dat"
#define DETTEXMAP_FILENAME            "detTexIdx.dat"
#define DETTEXWMAP_FILENAME           "detTexWt.dat"
#define WATER_HEIGHTMAP_DET_FILENAME  "waterHeightmapDet.dat"
#define WATER_HEIGHTMAP_MAIN_FILENAME "waterHeightmapMain.dat"

#define BLUR_LM_KEREL_SIZE 1
#define BLUR_LM_SIGMA      0.7


static class DefNewHMParams
{
public:
  Point2 sizePixels;
  Point2 sizeMeters;
  real cellSize;
  real heightScale;
  real heightOffset;
  Point2 originOffset;
  Point2 collisionOffset;
  Point2 collisionSize;
  bool collisionShow;
  bool doAutocenter;
  IPoint2 tileTexSz;
  String scriptPath;
  String tileTex;

  DefNewHMParams() :
    sizePixels(1024, 1024),
    sizeMeters(1024, 1024),
    cellSize(1.0),
    heightScale(200.0),
    heightOffset(0.0),
    originOffset(0, 0),
    collisionOffset(0, 0),
    collisionSize(100, 100),
    tileTexSz(32, 32),
    collisionShow(false),
    doAutocenter(true)
  {
    originOffset = -sizeMeters * cellSize / 2;
  }
} defaultHM;


static bool ignoreEvents = true;
static int transformZVarId = -1;
static int znZfarVarId = -1;
static int landTileVerticalTexVarId = -1;
static int landTileVerticalNmTexVarId = -1;
static int landTileVerticalDetTexVarId = -1;

static bool skipExportLtmap = false;
static bool missing_tile_reported = false;

static void setZTransformPersp(float zn, float zf)
{
  if (transformZVarId < 0)
    return;
  float q = zf / (zf - zn);
  dagGeom->shaderGlobalSetColor4(transformZVarId, Color4(q, 1, -zn * q, 0));
  dagGeom->shaderGlobalSetColor4(znZfarVarId, Color4(zn, zf, 0, 0));
}

static bool is_tga_or_tiff(const char *fn)
{
  return trail_stricmp(fn, ".tga") || trail_stricmp(fn, ".tif") || trail_stricmp(fn, ".tiff");
}

static TexImage32 *load_tga_or_tiff(const char *fn, IMemAlloc *mem)
{
  if (trail_stricmp(fn, ".tga"))
    return load_tga32(fn, mem);
  else
    return load_tiff32(fn, mem);
}

static TexImage32 *try_find_and_load_tga_or_tiff(const char *fn, IMemAlloc *mem)
{
  String fnWithExt(256, "%s.tif", fn);
  TexImage32 *image = load_tiff32(fnWithExt, mem);
  if (image)
    return image;

  fnWithExt.printf(256, "%s.tiff", fn);
  image = load_tiff32(fnWithExt, mem);
  if (image)
    return image;

  fnWithExt.printf(256, "%s.tga", fn);
  return load_tga32(fnWithExt, mem);
}

// IGenEditorPlugin
//==============================================================================
void HmapLandPlugin::registered()
{
#define REGISTER_COMMAND(cmd_name)                                          \
  if (!dagConsole->registerCommand(DAGORED2->getConsole(), cmd_name, this)) \
  DAEDITOR3.conError("[%s] Couldn't register command '" cmd_name "'", getMenuCommandName())
  REGISTER_COMMAND("land.ltmap");
  REGISTER_COMMAND("land.rebuild_colors");
  REGISTER_COMMAND("land.commit_changes");
#undef REGISTER_COMMAND

  znZfarVarId = dagGeom->getShaderVariableId("zn_zfar");
  transformZVarId = dagGeom->getShaderVariableId("transform_z");


  clear_and_resize(brushes, BRUSHES_COUNT);
  brushes[HILLUP_BRUSH] = heightmap_land::getUpHillBrush(this, *this);
  brushes[HILLDOWN_BRUSH] = heightmap_land::getDownHillBrush(this, *this);
  brushes[SMOOTH_BRUSH] = heightmap_land::getSmoothBrush(this, *this);
  brushes[ALIGN_BRUSH] = heightmap_land::getAlignBrush(this, *this);
  brushes[SHADOWS_BRUSH] = heightmap_land::getShadowsBrush(this, *this);
  brushes[SCRIPT_BRUSH] = heightmap_land::getScriptBrush(this, *this);

  DAGORED2->registerCustomCollider(this);
  DAGORED2->registerCustomCollider(&objEd);
  DAGORED2->registerCustomCollider(&objEd.loftGeomCollider);
  DAGORED2->registerCustomCollider(&objEd.polyGeomCollider);

  if (hmapSubtypeMask == -1)
    hmapSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("hmap_obj");

  if (lmeshSubtypeMask == -1)
    lmeshSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("lmesh_obj");

  if (lmeshDetSubtypeMask == -1)
    lmeshDetSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("lmesh_obj_det");

  if (grasspSubtypeMask == -1)
    grasspSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("grass_obj");

  if (navmeshSubtypeMask == -1)
    navmeshSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("navmesh");


  snowPrevievSVId = dagGeom->getShaderVariableId("snow_preview_mode");
  snowValSVId = dagGeom->getShaderVariableId("snow_preview_avg_level");
  landTileVerticalTexVarId = dagGeom->getShaderVariableId("vertical_tex");
  landTileVerticalNmTexVarId = dagGeom->getShaderVariableId("vertical_nm_tex");
  landTileVerticalDetTexVarId = dagGeom->getShaderVariableId("vertical_det_tex");
}


//==============================================================================
void HmapLandPlugin::unregistered()
{
  if (IRendInstGenService *rigenSrv = DAGORED2->queryEditorInterface<IRendInstGenService>())
  {
    rigenSrv->clearRtRIGenData();
    rigenSrv->setCustomGetHeight(NULL);
  }
  dagConsole->unregisterCommand(DAGORED2->getConsole(), "land.ltmap", this);
  dagConsole->unregisterCommand(DAGORED2->getConsole(), "land.rebuild_colors", this);
  dagConsole->unregisterCommand(DAGORED2->getConsole(), "land.commit_changes", this);
  DAGORED2->unregisterCustomCollider(&objEd.polyGeomCollider);
  DAGORED2->unregisterCustomCollider(&objEd.loftGeomCollider);
  DAGORED2->unregisterCustomCollider(&objEd);
  DAGORED2->unregisterCustomCollider(this);

  hmlService->destroyLandMeshRenderer(landMeshRenderer);
  hmlService->destroyLandMeshManager(landMeshManager);

  heightMap.closeFile();
  landClsMap.closeFile();
  colorMap.closeFile();
  lightMapScaled.closeFile();
  waterHeightmapDet.closeFile();
  waterHeightmapMain.closeFile();

  if (detTexIdxMap)
    detTexIdxMap->closeFile();
  if (detTexWtMap)
    detTexWtMap->closeFile();

  for (int i = 0; i < brushes.size(); ++i)
    del_it(brushes[i]);

  clear_and_shrink(brushes);
  objEd.unloadRoadBuilderDll();

  self = NULL;
}

bool HmapLandPlugin::onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params)
{
  CoolConsole &con = DAGORED2->getConsole();

  if (!stricmp(cmd, "land.ltmap"))
  {
    int errCnt = con.getErrorsCount() + con.getFatalsCount();
    if (params.size() == 1)
      skipExportLtmap = true;
    calcGoodLandLighting();
    if (params.size() == 1)
      resetRenderer();
    skipExportLtmap = false;

    if (params.size() == 1)
    {
      unsigned target = _MAKE4C('DDS');
      if (stricmp(dd_get_fname_ext(params[0]), ".tga") == 0)
        target = _MAKE4C('TGA');

      String prj(DAGORED2->getSdkDir());
      exportLightmapToFile(::make_full_path(prj, params[0]), target, true);
    }
    return con.getErrorsCount() + con.getFatalsCount() == errCnt;
  }
  if (!stricmp(cmd, "land.rebuild_colors"))
  {
    DAEDITOR3.conNote("batch cmd: rebuild HMAP colors");
    onPluginMenuClick(CM_REBUILD);
    return true;
  }
  if (!stricmp(cmd, "land.commit_changes"))
  {
    DAEDITOR3.conNote("batch cmd: commit HMAP changes");
    onPluginMenuClick(CM_COMMIT_HM_CHANGES);
    return true;
  }

  return false;
}


const char *HmapLandPlugin::onConsoleCommandHelp(const char *cmd)
{
  if (!stricmp(cmd, "land.ltmap"))
    return "Type:\n"
           "'land.ltmap [file_path]' to calculate and save lightmaps;\n"
           "optional file_path is relative to develop;\n"
           "with no file_path builtScene/lightmap.dds is saved.\n";

  if (!stricmp(cmd, "land.rebuild_colors"))
    return "Type:\n"
           "'land.rebuild_colors'\n"
           "to rebuild colors/lighting/detailmaps for landmesh/heightmap in Landscape plugin";

  if (!stricmp(cmd, "land.commit_changes"))
    return "Type:\n"
           "'land.commit_changes'\n"
           "to save DAT files in Landscape plugin";

  return NULL;
}

//==============================================================================
bool HmapLandPlugin::begin(int toolbar_id, unsigned menu_id)
{
  createMenu(menu_id);
  toolbarId = toolbar_id;
  createPropPanel();

  PropertyContainerControlBase *toolbar = DAGORED2->getCustomPanel(toolbar_id);
  G_ASSERT(toolbar);
  objEd.initUi(toolbar_id);
  // objEd.objectPropBar->hidePanel();

  IWndManager *manager = IEditorCoreEngine::get()->getWndManager();
  manager->registerWindowHandler(this);


  if (propPanel) // && (objEd.toolBar->isButtonChecked(CM_SHOW_PANEL)))
    propPanel->showPropPanel(true);

  brushDlg = DAGORED2->createDialog(_pxScaled(220), _pxScaled(440), "Heightmap brush");
  brushDlg->showButtonPanel(false);
  PropertyContainerControlBase *_panel = brushDlg->getPanel();
  _panel->setEventHandler(this);

  updateSnowSources();

  return true;
}


//==============================================================================
bool HmapLandPlugin::end()
{
  if (propPanel->isVisible())
    propPanel->showPropPanel(false);

  IWndManager *manager = IEditorCoreEngine::get()->getWndManager();
  manager->unregisterWindowHandler(this);

  objEd.closeUi();
  DAGORED2->endBrushPaint();
  objEd.setEditMode(CM_OBJED_MODE_SELECT);
  return true;
}


//==============================================================================


IWndEmbeddedWindow *HmapLandPlugin::onWmCreateWindow(void *handle, int type)
{
  switch (type)
  {
    case PROPBAR_EDITOR_WTYPE:
    {
      if (propPanel->getPanelWindow())
        return NULL;

      IWndManager *manager = IEditorCoreEngine::get()->getWndManager();
      manager->setCaption(handle, "Properties");

      CPanelWindow *_panel_window = IEditorCoreEngine::get()->createPropPanel(this, handle);

      if (_panel_window)
      {
        propPanel->setPanelWindow(_panel_window);
        _panel_window->setEventHandler(propPanel);
        propPanel->fillPanel(false);
      }

      PropertyContainerControlBase *tool_bar = DAGORED2->getCustomPanel(toolbarId);
      if (tool_bar)
        tool_bar->setBool(CM_SHOW_PANEL, true);

      return propPanel->getPanelWindow();
    }
    break;
  }

  return NULL;
}


bool HmapLandPlugin::onWmDestroyWindow(void *handle)
{
  if (propPanel->getPanelWindow() && propPanel->getPanelWindow()->getParentWindowHandle() == handle)
  {
    mainPanelState.reset();
    propPanel->getPanelWindow()->saveState(mainPanelState);

    CPanelWindow *_panel_window = propPanel->getPanelWindow();
    propPanel->setPanelWindow(NULL);

    IEditorCoreEngine::get()->deleteCustomPanel(_panel_window);

    PropertyContainerControlBase *tool_bar = DAGORED2->getCustomPanel(toolbarId);
    if (tool_bar)
      tool_bar->setBool(CM_SHOW_PANEL, false);

    return true;
  }

  return false;
}

//==============================================================================


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

void HmapLandPlugin::updateHtLevelCurves()
{
  static int heightmap_show_level_curves_gvid = dagGeom->getShaderVariableId("heightmap_show_level_curves");
  static int heightmap_level_curves_params_gvid = dagGeom->getShaderVariableId("heightmap_level_curves_params");
  dagGeom->shaderGlobalSetInt(heightmap_show_level_curves_gvid, showHtLevelCurves ? 1 : 0);
  dagGeom->shaderGlobalSetColor4(heightmap_level_curves_params_gvid,
    Color4(htLevelCurveStep, htLevelCurveThickness * 0.5f, htLevelCurveOfs, 1.0f - htLevelCurveDarkness));
}

void HmapLandPlugin::updateRendererLighting()
{
  genHmap->sunColor = ldrLight.getSunLightColor();
  genHmap->skyColor = ldrLight.getSkyLightColor();
  DAGORED2->repaint();
}


void HmapLandPlugin::resetRenderer(bool imm)
{
  if (imm)
    delayedResetRenderer();
  else
    pendingResetRenderer = true;
}


bool HmapLandPlugin::loadLevelSettingsBlk(DataBlock &level_blk)
{
  String fn(0, "levels/%s", DAGORED2->getProjectFileName());
  String app_root(DAGORED2->getWorkspace().getAppDir());
  DataBlock appblk(String(260, "%s/application.blk", app_root));

  class LevelsFolderIncludeFileResolver : public DataBlock::IIncludeFileResolver
  {
  public:
    LevelsFolderIncludeFileResolver() : prefix(tmpmem), appDir(NULL) {}
    virtual bool resolveIncludeFile(String &inout_fname)
    {
      String fn;
      for (int i = 0; i < prefix.size(); i++)
      {
        fn.printf(0, "%s/%s/levels/%s", appDir, prefix[i], inout_fname);
        if (dd_file_exists(fn))
        {
          inout_fname = fn;
          return true;
        }
      }
      return false;
    }
    void preparePrefixes(const DataBlock *b, const char *app_dir)
    {
      prefix.clear();
      appDir = app_dir;
      if (!b)
        return;
      for (int i = 0; i < b->paramCount(); i++)
        if (b->getParamType(i) == DataBlock::TYPE_STRING)
          prefix.push_back(b->getStr(i));
    }
    Tab<const char *> prefix;
    const char *appDir;
  };
  static LevelsFolderIncludeFileResolver inc_resv;

  inc_resv.preparePrefixes(appblk.getBlockByName("levelsBlkPrefix"), app_root);
  DataBlock::setIncludeResolver(&inc_resv);

  while (char *p = (char *)dd_get_fname_ext(fn))
    *p = '\0';
  fn.updateSz();
  fn += ".blk";

  debug("Loading level settings from \"%s\"", fn);
  bool loaded = false;
  int prefix_tried = 0;
  for (int i = 0; i < inc_resv.prefix.size(); i++)
  {
    String fpath(0, "%s/%s/%s", app_root, inc_resv.prefix[i], fn);
    if (dd_file_exists(fpath) && level_blk.load(fpath))
    {
      loaded = true;
      levelBlkFName = fpath;
      break;
    }
    else
    {
      debug("%s is %s", fpath, dd_file_exists(fpath) ? "CORRUPT" : "MISSING");
      prefix_tried++;
    }
  }

  if (!loaded)
  {
    loaded = level_blk.load(fn);
    if (loaded)
      levelBlkFName = fn;
  }
  if (!loaded && DAGORED2->getExportPath(_MAKE4C('PC')) &&
      appblk.getBlockByNameEx("levelsBlkPrefix")->getBool("useExportBinPath", false))
  {
    String fn;
    fn = DAGORED2->getExportPath(_MAKE4C('PC'));
    if (!fn.empty())
    {
      while (char *p = (char *)dd_get_fname_ext(fn))
        *p = '\0';
      fn.updateSz();
      fn += ".blk";
      debug("try export path level blk %s", fn);
      loaded = level_blk.load(fn);
      if (loaded)
        levelBlkFName = fn;
    }
  }

  if (!loaded)
    DAEDITOR3.conWarning("cannot load %s (tried also with %d prefixes)", fn, prefix_tried);
  hmlService->onLevelBlkLoaded(loaded ? level_blk : DataBlock());
  if (IRendInstGenService *rendInstGenService = DAGORED2->queryEditorInterface<IRendInstGenService>())
    rendInstGenService->onLevelBlkLoaded(loaded ? level_blk : DataBlock());

  DataBlock::setRootIncludeResolver(app_root);
  return loaded;
}

void HmapLandPlugin::updateWaterSettings(const DataBlock &level_blk)
{
  if (!waterService)
    return;

  waterService->set_level(level_blk.getReal("water_level", 0.f));

  Point2 windDir(0.6, 0.8);
  windDir = level_blk.getPoint2("windDir", windDir);
  const DataBlock *weatherTypeBlk = NULL;
  if (ISkiesService *skiesSrv = DAGORED2->queryEditorInterface<ISkiesService>())
    weatherTypeBlk = skiesSrv->getWeatherTypeBlk();

  float stormStrength = (weatherTypeBlk ? weatherTypeBlk->getReal("wind_strength", 4) : 4) * level_blk.getReal("wind_scale", 1);
  stormStrength = min(stormStrength, level_blk.getReal("max_wind_strength", 6));
  stormStrength = max(stormStrength, 1.0f);
  waterService->set_wind(stormStrength, windDir);

  BBox2 water3DQuad;
  water3DQuad[0] = level_blk.getPoint2("waterCoord0", Point2(-256000, -256000));
  water3DQuad[1] = level_blk.getPoint2("waterCoord1", Point2(+256000, +256000));
  waterService->set_render_quad(water3DQuad.width().x < 128000 ? water3DQuad : BBox2());
}

void HmapLandPlugin::rebuildLandmeshDump()
{
  if (lmDump)
    lmDump->deleteChain(lmDump);
  lmDump = NULL;

  mkbindump::BinDumpSaveCB cwr(32 << 20, _MAKE4C('PC'), false);
  if (!exportLandMesh(cwr, NULL, NULL, true))
    DAEDITOR3.conError("exportLandMesh failed");
  else
    lmDump = cwr.getRawWriter().takeMem();
}
void HmapLandPlugin::rebuildLandmeshManager()
{
  hmlService->destroyLandMeshRenderer(landMeshRenderer);
  hmlService->destroyLandMeshManager(landMeshManager);
  if (lmDump)
  {
    MemoryLoadCB crd(lmDump, false);
    landMeshManager = hmlService->createLandMeshManager(crd);
    if (const IBBox2 *b = getExclCellBBox())
    {
      landMeshManager->cullingState.useExclBox = true;
      landMeshManager->cullingState.exclBox[0] = b->lim[0] + landMeshManager->getCellOrigin();
      landMeshManager->cullingState.exclBox[1] = b->lim[1] + landMeshManager->getCellOrigin();
    }
    else
      landMeshManager->cullingState.useExclBox = false;
  }
  clearTexCache();
}
void HmapLandPlugin::delayedResetRenderer()
{
  if (!hmlService)
    return;
  hmlService->destroyLandMeshRenderer(landMeshRenderer);
  if (!cableService)
    cableService = DAGORED2->queryEditorInterface<ICableService>();
  if (exportType == EXPORT_PSEUDO_PLANE)
  {
    pendingResetRenderer = false;
    return;
  }

  if (render.gridStep < 1)
    render.gridStep = 1;
  if (render.elemSize < 2)
    render.elemSize = 2;
  if (render.radiusElems < 1)
    render.radiusElems = 1;
  if (render.ringElems < 1)
    render.ringElems = 1;
  if (render.numLods < 0)
    render.numLods = 0;
  if (render.maxDetailLod < 0)
    render.maxDetailLod = 0;
  if (render.maxDetailLod > render.numLods)
    render.maxDetailLod = render.numLods;
  if (!float_nonzero(render.detailTile))
    render.detailTile = 1;
  if (!float_nonzero(render.canyonHorTile))
    render.canyonHorTile = 1;
  if (!float_nonzero(render.canyonVertTile))
    render.canyonVertTile = 1;
  if (render.holesLod < 0)
    render.holesLod = 0;
  if (render.holesLod > render.numLods)
    render.holesLod = render.numLods;

  String fn;
  if (heightMap.isFileOpened())
  {
    getTexEntry(tileTexName, &fn, NULL);

    if (strchr(fn, '*') || ::dd_file_exist(fn))
      tileTexId = dagRender->addManagedTexture(fn);
    else if (DAGORED2->isInBatchOp() && requireTileTex)
    {
      tileTexId = BAD_TEXTUREID;
      DAEDITOR3.conError("HeightMap tile texture is missing!");
    }
    else if (requireTileTex)
    {
      tileTexId = BAD_TEXTUREID;
      if (!missing_tile_reported)
        DAEDITOR3.conError("HeightMap tile texture is missing!");
      else
      {
        wingw::message_box(wingw::MBS_HAND, "Error", "HeightMap tile texture is missing <%s>!", fn);
        missing_tile_reported = true;
      }
    }
  }

  if (loadLevelSettingsBlk(levelBlk))
  {
    dagGeom->shaderGlobalSetVarsFromBlk(*levelBlk.getBlockByNameEx("shader_vars"));
    loadGPUGrassFromBlk(levelBlk);
    loadGrassFromBlk(levelBlk);
  }

  if (ISkiesService *skiesSrv = DAGORED2->queryEditorInterface<ISkiesService>())
    skiesSrv->overrideWeather(0, NULL, NULL, -1, NULL, levelBlk.getBlockByNameEx("stars"));

  if (useMeshSurface && !landMeshMap.isEmpty() && lmDump)
  {
    // prepareEditableLandClasses();

    int texElemSize, texSize;

    texElemSize = real2int(landMeshMap.getCellSize() / gridCellSize);
    for (texSize = 1; texSize < texElemSize; texSize <<= 1)
      ;

    render.detMapSize = texSize;
    render.detMapElemSize = texElemSize;
    int landTypeId = DAEDITOR3.getAssetTypeId("land");
    /*landClassInfo.clear();
    landClassInfo.resize(detailTexBlkName.size());
    const DataBlock &grass_blk = *level_blk.getBlockByNameEx("randomGrass");
    for (int i = 0; i < landClassInfo.size(); ++i)
    {
      landClassInfo[i].editorId = i;
      DagorAsset *asset = DAEDITOR3.getAssetByName(detailTexBlkName[i], landTypeId);
      if (!asset)
        continue;
      const DataBlock *blk = asset->props.getBlockByName("detail");
      if (!blk)
        continue;
      load_land_class_info(landClassInfo[i], *blk, LC_ALL_DATA);
      landClassInfo[i].editorId = i;
      landClassInfo[i].name = detailTexBlkName[i].str();
      if (blk->getStr("detailmap", NULL) && blk->getStr("splattingmap", NULL))
      {
        DAEDITOR3.conError("Ambiguos landclass = <%s>, it has both splattingmap and detailmap!", detailTexBlkName[i].str());
      }
    }*/

    if (!landMeshManager)
      rebuildLandmeshManager();

    G_ASSERT(landMeshManager);
    landMeshRenderer = hmlService->createLandMeshRenderer(*landMeshManager);

    lcRemap.resize(detailTexBlkName.size());
    mem_set_ff(lcRemap);
    for (int i = 0; i < landMeshManager->getLCCount(); i++)
      lcRemap[landMeshManager->getLCEditorId(i)] = i;

    // fixme: check if landclasses has same colormap (in landmesh via debug callback)
    /*for (int i = 0; i < landClasses.size(); ++i)
    {
      if (landClasses[i].colormapId == BAD_TEXTUREID)
        continue;
      int j;
      for (j = i+1; j < landClasses.size(); ++j)
        if (landClasses[i].colormapId == landClasses[j].colormapId)
          break;
      if (j < landClasses.size())
        DAEDITOR3.conWarning(
          "two different landclasses %d<%s> and %d<%s> has same colormap = <%s>", i, detailTexBlkName[i].str(),
            j, detailTexBlkName[j].str(), landInfo[i].colormapName);
    }*/

    if (!landMeshRenderer)
    {
      DAEDITOR3.conError("cannot create landMesh renderer (shader is missing?), switched to pseudo-plane");
      exportType = EXPORT_PSEUDO_PLANE;
      genHmap->altCollider = NULL;
      pendingResetRenderer = false;
      if (propPanel)
        propPanel->getPanelWindow()->setInt(PID_HM_EXPORT_TYPE, exportType);
      return;
    }

    G_ASSERT(landMeshRenderer->checkVerLabel());


    if (!grassService)
    {
      grassService = DAGORED2->queryEditorInterface<IGrassService>();
      if (grassService)
      {
        if (grassBlk != NULL)
        {
          grassService->create_grass(*grassBlk);
          grassService->enableGrass(enableGrass);
        }
        else
          grassBlk = grassService->create_default_grass();
        loadGrassFromBlk(*grassBlk);
        loadGrassLayers(*grassBlk, false);
      }
    }

    if (!gpuGrassService)
    {
      gpuGrassService = DAGORED2->queryEditorInterface<IGPUGrassService>();
      if (gpuGrassService)
      {
        if (gpuGrassBlk != nullptr)
        {
          gpuGrassService->createGrass(*gpuGrassBlk);
          gpuGrassService->enableGrass(enableGrass);
        }
        else
          gpuGrassBlk = gpuGrassService->createDefaultGrass();

        loadGPUGrassFromBlk(*gpuGrassBlk);
      }
    }

    if (!waterProjectedFxSrv)
      waterProjectedFxSrv = DAGORED2->queryEditorInterface<IWaterProjFxService>();


    if (!waterService)
    {
      waterService = DAGORED2->queryEditorInterface<IWaterService>();
      if (waterService)
        waterService->loadSettings(DataBlock());
    }

    if (waterService)
    {
      bool has_water = hasWaterSurf();

      for (int i = 0; i < objEd.objectCount(); i++)
        if (SplineObject *o = RTTI_cast<SplineObject>(objEd.getObject(i)))
          if (o->isPoly() && o->polyGeom.altGeom)
            has_water = true;

      if (!has_water)
        if (IGenEditorPlugin *p = DAGORED2->getPluginByName("_waterSurf"))
          p->setVisible(false);

      updateWaterSettings(levelBlk);
    }

    hmlService->updatePropertiesFromLevelBlk(levelBlk);

    SunSkyLight &lt = ldrLight;

    refillTexCache();
    updateVertTex();
  }

  if (heightMap.isFileOpened())
  {
    int texElemSize, texSize;

    texElemSize = (render.elemSize * render.gridStep) << render.maxDetailLod;
    for (texSize = 1; texSize < texElemSize; texSize <<= 1)
      ;

    render.detMapSize = texSize;
    render.detMapElemSize = texElemSize;

    texElemSize = (render.elemSize * render.gridStep) << render.numLods;
    for (texSize = 1; texSize < texElemSize; texSize <<= 1)
      ;

    render.landTexSize = texSize;
    render.landTexElemSize = texElemSize;
  }

  // Set lightmap texture.
  if (lightmapTexId != BAD_TEXTUREID)
    dagRender->releaseManagedTex(lightmapTexId);
  lightmapTexId = BAD_TEXTUREID;

  String prj;
  DAGORED2->getProjectFolderPath(prj);
  String fileName = ::make_full_path(prj, "builtScene/lightmap.tga");

  if (!::dd_file_exist(fileName))
  {
    DAEDITOR3.conWarning("missing ltmap: %s, trying to use commonData/tex/lmesh_ltmap.tga instead", fileName.str());
    fileName = ::make_full_start_path("../commonData/tex/lmesh_ltmap.tga");
  }

  if (::dd_file_exist(fileName))
  {
    lightmapTexId = dagRender->addManagedTexture(fileName);
    dagRender->acquireManagedTex(lightmapTexId);

    dagGeom->shaderGlobalSetTexture(dagGeom->getShaderVariableId("lightmap_tex"), lightmapTexId);

    if (getNumCellsX() > 0)
    {
      Color4 world_to_lightmap(1.f / (getNumCellsX() * getLandCellSize()), 1.f / (getNumCellsY() * getLandCellSize()),
        -(float)getCellOrigin().x / getNumCellsX() - getOffset().x / getLandCellSize() / getNumCellsX(),
        -(float)getCellOrigin().y / getNumCellsY() - getOffset().z / getLandCellSize() / getNumCellsY());

      world_to_lightmap.b += 0.5 * world_to_lightmap.r * gridCellSize;
      world_to_lightmap.a += 0.5 * world_to_lightmap.g * gridCellSize;

      dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("world_to_lightmap"), world_to_lightmap);
    }
  }
  else
    DAEDITOR3.conError("missing ltmap: %s", fileName.str());

  if (syncLight)
    getAllSunSettings();
  else if (syncDirLight)
    getDirectionSunSettings();

  pendingResetRenderer = false;
  if (landMeshRenderer)
    landMeshRenderer->setCellsDebug(debugLmeshCells);
  DAGORED2->invalidateViewportCache();
}


void HmapLandPlugin::updateHorizontalTex()
{
  if (!useHorizontalTex)
  {
    dagGeom->shaderGlobalSetTexture(dagGeom->getShaderVariableId("land_geometry_horizontal_tex"), BAD_TEXTUREID);
    dagGeom->shaderGlobalSetTexture(dagGeom->getShaderVariableId("land_geometry_horizontal_det_tex"), BAD_TEXTUREID);
    return;
  }

  TEXTUREID tid = horizontalTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", horizontalTexName.str()));
  TEXTUREID detail_tid =
    horizontalDetailTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", horizontalDetailTexName.str()));

  dagGeom->shaderGlobalSetTexture(dagGeom->getShaderVariableId("land_geometry_horizontal_tex"), tid);
  dagGeom->shaderGlobalSetTexture(dagGeom->getShaderVariableId("land_geometry_horizontal_det_tex"), detail_tid);

  // Color4(1.f / max(vertDetTexXZtile, 1e-3f), -1.f / max(vertDetTexYtile, 1e-3f), vertDetTexYOffset / max(vertDetTexYtile, 1e-3f),
  // 0.f));

  dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("land_geometry_horizontal_tex_map_tc"),
    Color4(1.f / max(horizontalTex_TileSizeX, 1e-3f), -1.f / max(horizontalTex_TileSizeY, 1e-3f),
      horizontalTex_OffsetX / max(horizontalTex_TileSizeX, 1e-3f), horizontalTex_OffsetY / max(horizontalTex_TileSizeY, 1e-3f)));
  dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("land_geometry_horizontal_detailtex_map_tc"),
    Color4(1.f / max(horizontalTex_DetailTexSizeX, 1e-3f), -1.f / max(horizontalTex_DetailTexSizeY, 1e-3f), 0.f, 0.f));
}

void HmapLandPlugin::acquireHorizontalTexRef()
{
  dagRender->acquireManagedTex(
    horizontalTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", horizontalTexName.str())));
  dagRender->acquireManagedTex(
    horizontalDetailTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", horizontalDetailTexName.str())));
}

void HmapLandPlugin::releaseHorizontalTexRef()
{
  dagRender->releaseManagedTex(
    horizontalTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", horizontalTexName.str())));
  dagRender->releaseManagedTex(
    horizontalDetailTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", horizontalDetailTexName.str())));
}


void HmapLandPlugin::saveHorizontalTex(DataBlock &blk)
{
  blk.setBool("use", useHorizontalTex);
  blk.setStr("texName", horizontalTexName.str());
  blk.setStr("detailTexName", horizontalDetailTexName.str());

  blk.setReal("tileSizeX", horizontalTex_TileSizeX);
  blk.setReal("tileSizeY", horizontalTex_TileSizeY);

  blk.setReal("offsetX", horizontalTex_OffsetX);
  blk.setReal("offsetY", horizontalTex_OffsetY);

  blk.setReal("detailTexSizeX", horizontalTex_DetailTexSizeX);
  blk.setReal("detailTexSizeY", horizontalTex_DetailTexSizeY);
}
void HmapLandPlugin::loadHorizontalTex(const DataBlock &blk)
{
  useHorizontalTex = blk.getBool("use", false);
  horizontalTexName = blk.getStr("texName", "");
  horizontalDetailTexName = blk.getStr("detailTexName", "");

  horizontalTex_TileSizeX = blk.getReal("tileSizeX", 128.f);
  horizontalTex_TileSizeY = blk.getReal("tileSizeY", 128.f);

  horizontalTex_OffsetX = blk.getReal("offsetX", 0.f);
  horizontalTex_OffsetY = blk.getReal("offsetY", 0.f);

  horizontalTex_DetailTexSizeX = blk.getReal("detailTexSizeX", 128.f);
  horizontalTex_DetailTexSizeY = blk.getReal("detailTexSizeY", 128.f);
  updateHorizontalTex();
}


void HmapLandPlugin::updateLandModulateColorTex()
{
  if (!useLandModulateColorTex)
  {
    dagGeom->shaderGlobalSetTexture(dagGeom->getShaderVariableId("land_modulate_color_tex"), BAD_TEXTUREID);
    return;
  }

  TEXTUREID tid =
    landModulateColorTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", landModulateColorTexName.str()));

  float edge1 = landModulateColorEdge0;
  float edge2 = landModulateColorEdge1;
  dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("land_modulate_color_edges"),
    Color4(-1.0f / (edge2 - edge1), 1.0f + edge1 / (edge2 - edge1), 0.f, 0.f));

  dagGeom->shaderGlobalSetTexture(dagGeom->getShaderVariableId("land_modulate_color_tex"), tid);

  if (hmlService)
    hmlService->invalidateClipmap(true);
}

void HmapLandPlugin::acquireLandModulateColorTexRef()
{
  dagRender->acquireManagedTex(landModulateColorTexName.empty()
                                 ? BAD_TEXTUREID
                                 : dagRender->addManagedTexture(String(32, "%s*", landModulateColorTexName.str())));
}
void HmapLandPlugin::releaseLandModulateColorTexRef()
{
  dagRender->releaseManagedTex(landModulateColorTexName.empty()
                                 ? BAD_TEXTUREID
                                 : dagRender->addManagedTexture(String(32, "%s*", landModulateColorTexName.str())));
}

void HmapLandPlugin::saveLandModulateColorTex(DataBlock &blk)
{
  blk.setBool("use", useLandModulateColorTex);
  blk.setStr("texName", landModulateColorTexName.str());
  blk.setReal("edge0", landModulateColorEdge0);
  blk.setReal("edge1", landModulateColorEdge1);
}
void HmapLandPlugin::loadLandModulateColorTex(const DataBlock &blk)
{
  useLandModulateColorTex = blk.getBool("use", false);
  landModulateColorTexName = blk.getStr("texName", "");
  landModulateColorEdge0 = blk.getReal("edge0", 0);
  landModulateColorEdge1 = blk.getReal("edge1", 1);
  updateLandModulateColorTex();
}


void HmapLandPlugin::setRendinstlayeredDetailColor()
{
  Color4 colorFrom, colorTo;
  if (useRendinstDetail2Modulation)
  {
    colorFrom = color4(rendinstDetail2ColorFrom);
    colorTo = color4(rendinstDetail2ColorTo);
  }
  else
  {
    colorFrom = Color4(0.5f, 0.5f, 0.5f, 0.5f);
    colorTo = Color4(0.5f, 0.5f, 0.5f, 0.5f);
  }
  dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("detail2_color_from"), colorFrom);
  dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("detail2_color_to"), colorTo);
}

void HmapLandPlugin::saveRendinstDetail2Color(DataBlock &blk)
{
  blk.setBool("useRendinstDetail2Modulation", useRendinstDetail2Modulation);
  blk.setE3dcolor("detail2_color_from", rendinstDetail2ColorFrom);
  blk.setE3dcolor("detail2_color_to", rendinstDetail2ColorTo);
}
void HmapLandPlugin::loadRendinstDetail2Color(const DataBlock &blk)
{
  useRendinstDetail2Modulation = blk.getBool("useRendinstDetail2Modulation", false);
  rendinstDetail2ColorFrom = blk.getE3dcolor("detail2_color_from", E3DCOLOR(128, 128, 128, 128));
  rendinstDetail2ColorTo = blk.getE3dcolor("detail2_color_to", E3DCOLOR(128, 128, 128, 128));
  setRendinstlayeredDetailColor();
}

//---

void HmapLandPlugin::updateVertTex()
{
  if (!useVertTex || vertTexAng0 >= vertTexAng1)
  {
    dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("vertical_param"), Color4(1.f, 1.f, 1000.f, 0.0f));
    dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("vertical_param_2"), Color4(1.f, 0.f, 0.f, 0.f));
    dagGeom->shaderGlobalSetTexture(landTileVerticalTexVarId, BAD_TEXTUREID);
    dagGeom->shaderGlobalSetTexture(landTileVerticalNmTexVarId, BAD_TEXTUREID);
    dagGeom->shaderGlobalSetTexture(landTileVerticalDetTexVarId, BAD_TEXTUREID);
    return;
  }

  float blendFromY = cosf(DEG_TO_RAD * vertTexAng0);
  float blendToY = cosf(DEG_TO_RAD * vertTexAng1);
  float sxz = 1.f / max(vertTexXZtile, 1e-3f), sy = -1.f / max(vertTexYtile, 1e-3f);
  TEXTUREID tid = vertTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", vertTexName.str()));
  TEXTUREID tid_nm = vertNmTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", vertNmTexName.str()));
  TEXTUREID tid_det = vertDetTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", vertDetTexName.str()));

  dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("vertical_param"),
    Color4(sxz, sy, 1.f / (blendFromY - blendToY), -blendToY / (blendFromY - blendToY)));

  dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("vertical_param_2"),
    Color4(vertTexHorBlend, -vertTexYOffset * sy, 0.f, 0.f));

  dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("vertical_param_3"),
    Color4(1.f / max(vertDetTexXZtile, 1e-3f), -1.f / max(vertDetTexYtile, 1e-3f), vertDetTexYOffset / max(vertDetTexYtile, 1e-3f),
      0.f));

  dagGeom->shaderGlobalSetTexture(landTileVerticalTexVarId, tid);
  dagGeom->shaderGlobalSetTexture(landTileVerticalNmTexVarId, tid_nm);
  dagGeom->shaderGlobalSetTexture(landTileVerticalDetTexVarId, tid_det);
}
void HmapLandPlugin::acquireVertTexRef()
{
  dagRender->acquireManagedTex(
    vertTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", vertTexName.str())));
  dagRender->acquireManagedTex(
    vertNmTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", vertNmTexName.str())));
  dagRender->acquireManagedTex(
    vertDetTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", vertDetTexName.str())));
}
void HmapLandPlugin::releaseVertTexRef()
{
  dagRender->releaseManagedTex(
    vertTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", vertTexName.str())));
  dagRender->releaseManagedTex(
    vertNmTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", vertNmTexName.str())));
  dagRender->releaseManagedTex(
    vertDetTexName.empty() ? BAD_TEXTUREID : dagRender->addManagedTexture(String(32, "%s*", vertDetTexName.str())));
}

void HmapLandPlugin::invalidateRenderer() { hmlService->destroyLandMeshRenderer(landMeshRenderer); }


void HmapLandPlugin::gatherStaticCollisionGeomGame(StaticGeometryContainer &cont)
{
  gatherStaticGeometry(cont, 0, true);
#if !defined(USE_LMESH_ACES)
  if (useMeshSurface && exportType == EXPORT_LANDMESH &&
      (DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION) & lmeshSubtypeMask))
  {
    if (landMeshMap.isEmpty())
      generateLandMeshMap(landMeshMap, DAGORED2->getConsole(), false, NULL);
    landMeshMap.addStaticCollision(cont);
  }
#endif
}

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

bool HmapLandPlugin::getHeight(const Point2 &p, real &ht, Point3 *normal) const
{
  if (detDivisor && (detRect & p))
  {
    HMDetGH det_hm;
    return get_height_midpoint_heightmap(det_hm, p, ht, normal);
  }

  if (!landMeshMap.getGameLandRayTracer())
    return false;
  return landMeshMap.getGameLandRayTracer()->getHeight(p, ht, normal);
}

bool HmapLandPlugin::exportLightmapToFile(const char *file_name, int target_code, bool high_qual)
{
  if (!hasLightmapTex)
    return false;

  CoolConsole &con = DAGORED2->getConsole();

  if (!lightMapScaled.isFileOpened())
  {
    con.addMessage(ILogWriter::ERROR, "No lightmap data");
    con.endLog();
    return false;
  }

  int time0 = dagTools->getTimeMsec();

  // Convert to image.

  int width = lightMapScaled.getMapSizeX();
  int height = lightMapScaled.getMapSizeY();

  SmallTab<TexPixel32, TmpmemAlloc> image;
  clear_and_resize(image, width * height);

  if (useNormalMap)
  {
    for (unsigned int y = 0, id = 0; y < height; y++)
      for (unsigned int x = 0; x < width; x++, id++)
      {
        unsigned lt = lightMapScaled.getData(x, y);
        unsigned nx = (lt >> 16) & 0xFF;
        unsigned nz = (lt >> 24) & 0xFF;
        image[id].c = E3DCOLOR(0, nx, 0, nz);
      }
  }
  else if (storeNxzInLtmapTex)
  {
    for (unsigned int y = 0, id = 0; y < height; y++)
      for (unsigned int x = 0; x < width; x++, id++)
      {
        unsigned lt = lightMapScaled.getData(x, y);
        unsigned sunLight = (lt >> 0) & 0xFF;
        unsigned skyLight = (lt >> 8) & 0xFF;
        unsigned nx = (lt >> 16) & 0xFF;
        unsigned nz = (lt >> 24) & 0xFF;
        image[id].c = E3DCOLOR(nx, nz, skyLight, sunLight);
      }
  }
  else
  {
    for (unsigned int y = 0, id = 0; y < height; y++)
      for (unsigned int x = 0; x < width; x++, id++)
      {
        unsigned lt = lightMapScaled.getData(x, y);
        unsigned sunLight = (lt >> 0) & 0xFF;
        unsigned skyLight = (lt >> 8) & 0xFF;
        image[id].c = E3DCOLOR(skyLight, skyLight, skyLight, sunLight);
      }
  }
  con.addMessage(ILogWriter::REMARK, "lightmap converted to rgba for %.1f sec", (dagTools->getTimeMsec() - time0) / 1000.0f);
  time0 = dagTools->getTimeMsec();
  // Save textrue.
  if (target_code == _MAKE4C('TGA'))
  {
    ::save_tga32(file_name, image.data(), width, height, width * 4);
    return true;
  }

  ddstexture::Converter cnv;
  cnv.format = ddstexture::Converter::fmtDXT5;
  if (target_code == _MAKE4C('iOS'))
    cnv.format = ddstexture::Converter::fmtASTC4;
  cnv.mipmapType = ddstexture::Converter::mipmapGenerate;
  cnv.mipmapCount = ddstexture::Converter::AllMipMaps;
  cnv.mipmapFilter = ddstexture::Converter::filterBox;
  cnv.quality = high_qual ? ddstexture::Converter::QualityProduction : ddstexture::Converter::QualityFastest;
  int memneeded = width * height * 2;
  DynamicMemGeneralSaveCB memcwr(tmpmem, 0, memneeded);
  bool res = dagTools->ddsConvertImage(cnv, memcwr, &image[0], width, height, width * sizeof(TexPixel32));
  if (!res)
  {
    con.addMessage(ILogWriter::ERROR, "Failed to convert lightmap to DDS");
    // con.showConsole(true);
    return false;
  }

  if (target_code == _MAKE4C('DDS'))
  {
    FullFileSaveCB ddsFile(file_name);
    ddsFile.write(memcwr.data(), memcwr.size());
  }
  else
  {
    ddsx::Buffer b;
    ddsx::ConvertParams cp;
    cp.packSzThres = 8 << 10;
    cp.allowNonPow2 = false;
    cp.addrU = ddsx::ConvertParams::ADDR_CLAMP;
    cp.addrV = ddsx::ConvertParams::ADDR_CLAMP;
    cp.mipOrdRev = HmapLandPlugin::defMipOrdRev;
    res = dagTools->ddsxConvertDds(target_code, b, memcwr.data(), memcwr.size(), cp);
    if (!res)
    {
      con.addMessage(ILogWriter::ERROR, "Failed to convert lightmap to DDSX");
      // con.showConsole(true);
      return false;
    }

    FullFileSaveCB cwr(file_name);
    if (!cwr.fileHandle)
    {
      con.addMessage(ILogWriter::ERROR, "Failed to save lightmap to '%s'", file_name);
      // con.showConsole(true);
      return false;
    }

    cwr.write(b.ptr, b.len);

    dagTools->ddsxFreeBuffer(b);
  }

  con.addMessage(ILogWriter::REMARK, "lightmap converted to dds for %.1f sec", (dagTools->getTimeMsec() - time0) / 1000.0f);

  return true;
}

extern bool game_res_sys_v2;

bool HmapLandPlugin::buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel2 *params)
{
  // Temporary show not exported RIs for buildAndWriteNavmesh
  for (uint32_t i = 0; i < EditLayerProps::layerProps.size(); ++i)
    if (EditLayerProps::layerProps[i].type == EditLayerProps::ENT && !EditLayerProps::layerProps[i].exp)
      DAEDITOR3.setEntityLayerHiddenMask(DAEDITOR3.getEntityLayerHiddenMask() & ~(1ull << i));

  if (pendingResetRenderer)
    delayedResetRenderer();
  storeLayerTex();

  int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
  String tmp_stor;

  const int edMode = objEd.getEditMode();
  if (edMode == CM_CREATE_ENTITY)
    objEd.setEditMode(CM_OBJED_MODE_MOVE);

  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  int time0 = dagTools->getTimeMsec();

  if ((st_mask & lmeshSubtypeMask) && exportType == EXPORT_LANDMESH)
  {
    // export land
    int localStart = dagTools->getTimeMsec();
    con.addMessage(ILogWriter::REMARK, "Exporting land mesh...");
    LandRayTracer *gameTracer = NULL;
#if defined(USE_LMESH_ACES)
    bool import_sgeom = true;
#else
    bool import_sgeom = false;
#endif
    bool strip_det_hmap_from_tracer = (st_mask & hmapSubtypeMask) && detDivisor;
    if (!generateLandMeshMap(landMeshMap, DAGORED2->getConsole(), import_sgeom, &gameTracer, strip_det_hmap_from_tracer))
    {
      DAEDITOR3.conError("failed to generateLandMeshMap");
      return false;
    }
    con.addMessage(ILogWriter::REMARK, "generated land mesh in %g seconds.", (dagTools->getTimeMsec() - localStart) / 1000.0);

    localStart = dagTools->getTimeMsec();
    onWholeLandChanged(); // Place objects on current hmap.
    objEd.updateSplinesGeom();
    con.addMessage(ILogWriter::REMARK, "objects placed in %g seconds.", (dagTools->getTimeMsec() - localStart) / 1000.0);

    localStart = dagTools->getTimeMsec();

    cwr.beginTaggedBlock(_MAKE4C('lmap'));
    try
    {
      if (!exportLandMesh(cwr, NULL, gameTracer))
        return false;
      con.addMessage(ILogWriter::REMARK, "export landmesh in %g seconds.", (dagTools->getTimeMsec() - localStart) / 1000.0);
    }
    catch (IGenSave::SaveException e)
    {
      CoolConsole &con = DAGORED2->getConsole();
      con.startLog();
      con.addMessage(ILogWriter::ERROR, "Error exporting heightmap '%s'", e.excDesc);

      con.endLog();
      return false;
    }
    if (gameTracer)
      del_it(gameTracer);

    cwr.align8();
    cwr.endBlock();


    //---export land modulate color tex
    if (useLandModulateColorTex && !landModulateColorTexName.empty())
    {
      cwr.beginTaggedBlock(_MAKE4C('lmct')); // lmct = land modulate color texture

      cwr.writeDwString(landModulateColorTexName.str());
      cwr.writeReal(landModulateColorEdge0);
      cwr.writeReal(landModulateColorEdge1);

      cwr.align8();
      cwr.endBlock();
    }

    //---export rendinst detail2 color
    if (useRendinstDetail2Modulation)
    {
      cwr.beginTaggedBlock(_MAKE4C('d2co')); // d2co = detail 2 color

      Color4 colorFrom = color4(rendinstDetail2ColorFrom);
      Color4 colorTo = color4(rendinstDetail2ColorTo);

#define WRITE_COLOR(v) \
  cwr.writeReal(v.r);  \
  cwr.writeReal(v.g);  \
  cwr.writeReal(v.b);  \
  cwr.writeReal(v.a);

      WRITE_COLOR(colorFrom)
      WRITE_COLOR(colorTo)

      cwr.align8();
      cwr.endBlock();
    }

    //---export rendinst vertical tex
    if (useHorizontalTex && !horizontalTexName.empty()) // && !rendinstVerticalDiffuseDetailTexName.isEmpty())
    {
      cwr.beginTaggedBlock(_MAKE4C('lght')); // lght = landscape geometry horizontal texture

      cwr.writeDwString(horizontalTexName.str());
      cwr.writeDwString(horizontalDetailTexName.str());

      cwr.writeReal(horizontalTex_TileSizeX);
      cwr.writeReal(horizontalTex_TileSizeY);

      cwr.writeReal(horizontalTex_OffsetX);
      cwr.writeReal(horizontalTex_OffsetY);

      cwr.writeReal(horizontalTex_DetailTexSizeX);
      cwr.writeReal(horizontalTex_DetailTexSizeY);

      cwr.align8();
      cwr.endBlock();
    }


    if (detDivisor && (st_mask & hmapSubtypeMask))
    {
      DataBlock app_blk;
      HmapVersion exp_ver = HmapVersion::HMAP_DELTA_COMPRESSION_VER;
      unsigned export_chunk_sz = 0, exp_chunk_cnt = 0;
      unsigned hrb_subsz = 0;
      if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
        DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());
      else if (app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("hmap")->getBool("exportCBlockDC", false))
      {
        exp_ver = HmapVersion::HMAP_CBLOCK_DELTAC_VER;
        export_chunk_sz = app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("hmap")->getInt("exportChunkBytes", 4 << 20);
        hrb_subsz = app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("hmap")->getInt("htRangeBlocksSubSz", 64);
      }

      cwr.beginTaggedBlock(_MAKE4C('HM2'));

      IBBox2 eb = *getExclCellBBox();
      float hmin = heightMapDet.getFinalData(detRectC[0].x, detRectC[0].y), hmax = hmin;
      int st_pos = cwr.tell();

      for (int y = detRectC[0].y; y < detRectC[1].y; y++)
        for (int x = detRectC[0].x; x < detRectC[1].x; x++)
        {
          float h = heightMapDet.getFinalData(x, y);
          inplace_min(hmin, h);
          inplace_max(hmax, h);
        }
      debug("detailed HMAP minimax: %.4f - %.4f", hmin, hmax);
      float hdelta = ceil(hmax + 1) - floor(hmin);
      if (hdelta >= 256)
        hdelta = ceilf(hdelta / 256.0f) * 256.0;
      else
        for (int i = 1; i <= 256; i *= 2)
          if (hdelta < i)
          {
            hdelta = ceilf(hdelta / i) * i;
            break;
          }
      hmax = ceil(hmax + 1);
      hmin = hmax - hdelta;
      debug("detailed HMAP minimax, rounded: %.4f - %.4f, eps=%.4f", hmin, hmax, hdelta / 65536);

      cwr.writeFloat32e(gridCellSize / detDivisor);
      cwr.writeFloat32e(hmin);
      cwr.writeFloat32e(hdelta);
      cwr.writeFloat32e(detRect[0].x);
      cwr.writeFloat32e(detRect[0].y);
      int width = detRectC[1].x - detRectC[0].x;
      int height = detRectC[1].y - detRectC[0].y;
      const int hmap_version = (int)exp_ver;
      cwr.writeInt32e(width | (hmap_version << HMAP_WIDTH_BITS));
      cwr.writeInt32e(height);
      cwr.writeInt32e(eb[0].x);
      cwr.writeInt32e(eb[0].y);
      cwr.writeInt32e(eb[1].x);
      cwr.writeInt32e(eb[1].y);

      CompressedHeightmap c_hmap;
      Tab<uint8_t> c_hmap_stor;
      if (exp_ver == HmapVersion::HMAP_CBLOCK_DELTAC_VER)
      {
        uint8_t block_shift = 3;
        if (width != height)
          hrb_subsz = 0;
        else if (hrb_subsz * 2 > width)
          hrb_subsz = width / 2;
        uint8_t hrb_subsz_bits = hrb_subsz > 1 ? __bsf(hrb_subsz) : 0;
        if (!hrb_subsz_bits)
          hrb_subsz = 0;
        else if ((1 << hrb_subsz_bits) != hrb_subsz)
        {
          DAEDITOR3.conError("bad htRangeBlocksSubSz=%d, must be > 1 and pow-of-2 (hrb_subsz_bits=%d)", hrb_subsz, hrb_subsz_bits);
          hrb_subsz = hrb_subsz_bits = 0;
        }
        c_hmap_stor.resize(CompressedHeightmap::calc_data_size_needed(width, height, block_shift, hrb_subsz));

        Tab<uint16_t> hmap16;
        hmap16.reserve(width * height);
        for (int y = detRectC[0].y; y < detRectC[1].y; y++)
          for (int x = detRectC[0].x; x < detRectC[1].x; x++)
            hmap16.push_back(floorf((heightMapDet.getFinalData(x, y) - hmin) * 65535.0 / (hdelta)));
        c_hmap =
          CompressedHeightmap::compress(c_hmap_stor.data(), c_hmap_stor.size(), hmap16.data(), width, height, block_shift, hrb_subsz);

        if (export_chunk_sz & 0xFFFu)
        {
          DAEDITOR3.conError("bad exportChunkBytes=%d, must be >= %d", hrb_subsz, hrb_subsz_bits, 1 << 12);
          export_chunk_sz = 0;
        }
        export_chunk_sz = ((export_chunk_sz >> c_hmap.block_size_shift) << c_hmap.block_size_shift) & ~0xFFFu;
        if (hmap16.size() <= export_chunk_sz)
          export_chunk_sz = 0;
        if (export_chunk_sz)
          exp_chunk_cnt = (hmap16.size() + export_chunk_sz - 1) / export_chunk_sz;

        cwr.writeInt32e(export_chunk_sz | (unsigned(hrb_subsz_bits) << 8) | c_hmap.block_width_shift);
      }

      Tab<mkbindump::BinDumpSaveCB *> cwr_chunk;
      cwr_chunk.resize(exp_chunk_cnt);
      for (unsigned i = 0; i < exp_chunk_cnt; i++)
        cwr_chunk[i] = new mkbindump::BinDumpSaveCB(64 << 10, cwr.getTarget(), cwr.WRITE_BE);

      unsigned written_raw_data_sz = 0;
      if (exp_chunk_cnt)
        threadpool::init(min(cpujobs::get_physical_core_count(), 4), exp_chunk_cnt, 256 << 10);
      threadpool::parallel_for_inline(0, exp_chunk_cnt + 1, 1, [&](uint32_t begin, uint32_t, uint32_t) {
        mkbindump::BinDumpSaveCB cwr_hm(16 << 10, cwr.getTarget(), cwr.WRITE_BE);
        if (exp_ver == HmapVersion::HMAP_CBLOCK_DELTAC_VER)
        {
          unsigned b0 = 0, b1 = c_hmap.bw * c_hmap.bh;
          if (begin == 0)
          {
            cwr_hm.writeRaw(c_hmap.fullData, c_hmap.blockVariance - c_hmap.fullData);
            if (exp_chunk_cnt) // when writing more than 1 chunk we write block data separately
              b1 = b0;
          }
          else
          {
            b0 = (begin - 1) * (export_chunk_sz >> c_hmap.block_size_shift);
            b1 = min(b1, begin * (export_chunk_sz >> c_hmap.block_size_shift));
          }

          for (auto *b = c_hmap.getBlockVariance(b0), *be = c_hmap.getBlockVariance(b1); b < be; b++)
          {
            cwr_hm.writeInt8e(*b);
            for (unsigned bsz = (1 << c_hmap.block_size_shift) - 1; bsz > 0; bsz--, b++)
              cwr_hm.writeInt8e(b[1] - b[0]);
          }

          if (begin == 0 && hrb_subsz)
            cwr_hm.writeRaw(c_hmap.htRangeBlocks, c_hmap.fullData + c_hmap_stor.size() - (uint8_t *)c_hmap.htRangeBlocks);
        }
        else
          for (int y = detRectC[0].y; y < detRectC[1].y; y++)
          {
            uint16_t prev_data = 0;
            for (int x = detRectC[0].x; x < detRectC[1].x; x++)
            {
              uint16_t c_data = floorf((heightMapDet.getFinalData(x, y) - hmin) * 65535.0 / (hdelta));
              cwr_hm.writeInt16e(x == detRectC[0].x ? c_data : (int16_t)(int(c_data) - int(prev_data)));
              prev_data = c_data;
            }
          }

        mkbindump::BinDumpSaveCB &dest = (begin == 0) ? cwr : *cwr_chunk[begin - 1];
        dest.beginBlock();
        MemoryLoadCB mcrd(cwr_hm.getMem(), false);
        if (preferZstdPacking && allowOodlePacking)
          oodle_compress_data(dest.getRawWriter(), mcrd, cwr_hm.getSize());
        else if (preferZstdPacking || exp_ver == HmapVersion::HMAP_CBLOCK_DELTAC_VER)
          zstd_compress_data(dest.getRawWriter(), mcrd, cwr_hm.getSize(), 1 << 20, 19);
        else
          lzma_compress_data(dest.getRawWriter(), 9, mcrd, cwr_hm.getSize());
        dest.endBlock(preferZstdPacking ? (allowOodlePacking ? btag_compr::OODLE : btag_compr::ZSTD) : btag_compr::UNSPECIFIED);
        interlocked_add(written_raw_data_sz, cwr_hm.getSize());
      });
      if (exp_chunk_cnt)
        threadpool::shutdown();

      for (auto *c : cwr_chunk)
      {
        c->copyDataTo(cwr.getRawWriter());
        delete c;
      }
      cwr_chunk.clear();

      cwr.align8();
      cwr.endBlock();
      debug("HM2 written, %d packed, %d unpacked (ver=%d: %d chunks, chunk_sz=%d, CBsz=%dx%d HRBsubsz=%dx%d)", cwr.tell() - st_pos,
        written_raw_data_sz, (int)exp_ver, exp_chunk_cnt + 1, export_chunk_sz, //
        c_hmap.block_width, c_hmap.block_width, hrb_subsz, hrb_subsz);
    }
#if defined(USE_LMESH_ACES)
    generateLandMeshMap(landMeshMap, DAGORED2->getConsole(), false, NULL);
#endif

    if (true)
    {
      int phys_li = hmlService->getBitLayerIndexByName(getLayersHandle(), "phys");
      if (phys_li < 0)
      {
        phys_li = hmlService->getBitLayerIndexByName(getLayersHandle(), "land");
      }
      HmapBitLayerDesc layer = landClsLayer[phys_li];
      SmallTab<unsigned char, TmpmemAlloc> pmMap;
      SmallTab<unsigned char, TmpmemAlloc> pmIndexedMap;
      FastNameMap pmNames;

      SmallTab<unsigned char, TmpmemAlloc> physMap;
      int physMap_w = detDivisor ? detRectC.width().x : 0, physMap_h = detDivisor ? detRectC.width().y : 0;

      clear_and_resize(pmMap, 5 << (layer.bitCount + 1));
      mem_set_0(pmMap);
      pmNames.addNameId(hmlService->getLandPhysMatName(0));
      for (int i = 0; i < pmMap.size(); i += 5)
        if (landclass::AssetData *d = getLandClassMgr().getLandClass(phys_li, i / 5))
        {
          pmMap[i + 0] = pmNames.addNameId(hmlService->getLandPhysMatName(d->physMatId[0]));
          pmMap[i + 1] = pmNames.addNameId(hmlService->getLandPhysMatName(d->physMatId[1]));
          pmMap[i + 2] = pmNames.addNameId(hmlService->getLandPhysMatName(d->physMatId[2]));
          pmMap[i + 3] = pmNames.addNameId(hmlService->getLandPhysMatName(d->physMatId[3]));
          pmMap[i + 4] = pmNames.addNameId(hmlService->getLandPhysMatName(d->physMatId[4]));
        }
      clear_and_resize(pmIndexedMap, 64 << (layer.bitCount + 1));
      mem_set_0(pmIndexedMap);
      for (int i = 0; i < pmIndexedMap.size(); i += 64)
        if (landclass::AssetData *d = getLandClassMgr().getLandClass(phys_li, i / 64))
          for (int j = 0; j < 64; j++)
            pmIndexedMap[i + j] = pmNames.addNameId(hmlService->getLandPhysMatName(d->indexedPhysMatId[j]));

      if (detDivisor)
      {
        clear_and_resize(physMap, physMap_w * physMap_h);
        mem_set_0(physMap);

        SmallTab<TexImage32 *, TmpmemAlloc> stexMap;
        SmallTab<int, TmpmemAlloc> splattingTypesMap;
        int texTypeId = DAEDITOR3.getAssetTypeId("tex");

        clear_and_resize(stexMap, 1 << layer.bitCount);
        mem_set_ff(stexMap);

        enum
        {
          UNDEFINED = 0,
          RGB_SPLATTING = 1,
          INDEXED_SPLATTING = 2
        };

        clear_and_resize(splattingTypesMap, 1 << layer.bitCount);
        for (int i = 0; i < splattingTypesMap.size(); i++)
          splattingTypesMap[i] = UNDEFINED;

        for (int y = detRectC[0].y; y < detRectC[1].y; y++)
          for (int x = detRectC[0].x; x < detRectC[1].x; x++)
          {
            int land = layer.getLayerData(landClsMap.getData(x * lcmScale / detDivisor, y * lcmScale / detDivisor));
            physMap[(y - detRectC[0].y) * physMap_w + x - detRectC[0].x] = pmMap[5 * land];
            if (intptr_t(stexMap[land]) == intptr_t(-1))
            {
              stexMap[land] = NULL;
              if (landclass::AssetData *d = getLandClassMgr().getLandClass(phys_li, land))
              {
                if (d->detTex && d->detTex->getStr("splattingmap", nullptr))
                {
                  const char *splattingMap = d->detTex->getStr("splattingmap");
                  if (splattingMap[0] == '*')
                  {
                    SimpleString nm(splattingMap + 1);
                    if (char *p = strchr(nm.str(), ':'))
                      *p = '\0';
                    String fn(0, "%s/%s", DAGORED2->getPluginFilePath(this, "elc"), nm);
                    // debug("load %s", fn);
                    stexMap[land] = try_find_and_load_tga_or_tiff(fn, tmpmem);
                    splattingTypesMap[land] = RGB_SPLATTING;
                  }
                  else if (DagorAsset *a = DAEDITOR3.getAssetByName(splattingMap, texTypeId))
                  {
                    String fn(a->getTargetFilePath());
                    if (is_tga_or_tiff(fn))
                    {
                      // debug("load %s", fn);
                      stexMap[land] = load_tga_or_tiff(fn, tmpmem);
                      splattingTypesMap[land] = RGB_SPLATTING;
                    }
                    else
                      DAEDITOR3.conError("texture asset <%s> refernces to non-compatible %s", a->getName(), fn);
                  }
                  else
                    DAEDITOR3.conError("failed to resolve texture asset <%s> referenced by detailMap in landclass", splattingMap);
                }
                else if (d->indicesTex && d->indicesTex->getStr("indices"))
                {
                  const char *indicesMap = d->indicesTex->getStr("indices");
                  if (indicesMap[0] == '*')
                  {
                    SimpleString nm(indicesMap + 1);
                    if (char *p = strchr(nm.str(), ':'))
                      *p = '\0';
                    String fn(0, "%s/%s", DAGORED2->getPluginFilePath(this, "elc"), nm);
                    // debug("load %s", fn);
                    stexMap[land] = try_find_and_load_tga_or_tiff(fn, tmpmem);
                    splattingTypesMap[land] = INDEXED_SPLATTING;
                  }
                  else if (DagorAsset *a = DAEDITOR3.getAssetByName(indicesMap, texTypeId))
                  {
                    String fn(a->getTargetFilePath());
                    if (is_tga_or_tiff(fn))
                    {
                      // debug("load %s", fn);
                      stexMap[land] = load_tga_or_tiff(fn, tmpmem);
                      splattingTypesMap[land] = INDEXED_SPLATTING;
                    }
                    else
                      DAEDITOR3.conError("texture asset <%s> refernces to non-compatible %s", a->getName(), fn);
                  }
                  else
                    DAEDITOR3.conError("failed to resolve texture asset <%s> referenced by detailMap in landclass", indicesMap);
                }
              }
              if (stexMap[land] && intptr_t(stexMap[land]) != intptr_t(-1))
              {
                // We need to reverse image in vertical way
                TexPixel32 *pix = stexMap[land]->getPixels();
                Tab<TexPixel32> tempBuffer;
                tempBuffer.resize(stexMap[land]->w);
                for (int y = 0; y < stexMap[land]->h / 2; ++y)
                {
                  memcpy(tempBuffer.data(), &pix[(stexMap[land]->h - y - 1) * stexMap[land]->w], data_size(tempBuffer));
                  memcpy(&pix[(stexMap[land]->h - y - 1) * stexMap[land]->w], &pix[y * stexMap[land]->w], data_size(tempBuffer));
                  memcpy(&pix[y * stexMap[land]->w], tempBuffer.data(), data_size(tempBuffer));
                }
              }
              // debug("stexMap[%d]=%p", land, stexMap[land]);
            }
            if (stexMap[land] && intptr_t(stexMap[land]) != intptr_t(-1))
            {
              landclass::AssetData *d = getLandClassMgr().getLandClass(phys_li, land);
              // fixme: this is awful copy paste of 'opaque' landclass logic in order to create physmap
              float tile = safediv(1.f, d->detTex->getPoint2("size", Point2(1, 1)).x);
              Point2 offset = d->detTex->getPoint2("offset", Point2(0, 0)); // mask origin relative to main HMAP origin
              //
              const TexPixel32 *pix = stexMap[land]->getPixels();

              int w = stexMap[land]->w, h = stexMap[land]->h;
              float fx = -0.001f + fmodf(((x + 0.5f) * gridCellSize / detDivisor - offset.x) * tile, 1.0f) * w;
              float fy = -0.001f + fmodf(((y + 0.5f) * gridCellSize / detDivisor - offset.y) * tile, 1.0f) * h;

              if (fx < 0)
                fx += w;
              if (fy < 0)
                fy += h;
              int ix = floorf(fx), iy = floorf(fy);
              float tx = fx - ix, ty = fy - iy;

              E3DCOLOR p00(pix[iy * w + ix].u);
              E3DCOLOR p10(pix[iy * w + ((ix + 1) % w)].u);
              E3DCOLOR p01(pix[((iy + 1) % h) * w + ix].u);
              E3DCOLOR p11(pix[((iy + 1) % h) * w + ((ix + 1) % w)].u);

              if (splattingTypesMap[land] == RGB_SPLATTING) // mega landclass
              {
                float ch[4];
                ch[0] = ((p00.r * (1 - tx) + p01.r * tx) * (1 - ty) + (p10.r * (1 - tx) + p11.r * tx) * ty) / 255.0f;
                ch[1] = ((p00.g * (1 - tx) + p01.g * tx) * (1 - ty) + (p10.g * (1 - tx) + p11.g * tx) * ty) / 255.0f;
                ch[2] = ((p00.b * (1 - tx) + p01.b * tx) * (1 - ty) + (p10.b * (1 - tx) + p11.b * tx) * ty) / 255.0f;
                ch[3] = clamp(1 - ch[0] - ch[1] - ch[2], 0.f, 1.f);
                for (int i = 0; i < 4; i++)
                  if (ch[i] > 0.5)
                    physMap[(y - detRectC[0].y) * physMap_w + x - detRectC[0].x] = pmMap[5 * land + 1 + i];
              }
              if (splattingTypesMap[land] == INDEXED_SPLATTING) // indexed landclass
              {
                int index00 = floorf(p00.r + 0.1f);
                int index01 = floorf(p01.r + 0.1f);
                int index10 = floorf(p10.r + 0.1f);
                int index11 = floorf(p11.r + 0.1f);
                float weight00 = (1.0f - tx) * (1.0f - ty);
                float weight01 = tx * (1.0f - ty);
                float weight10 = (1.0f - tx) * ty;
                float weight11 = tx * ty;

                unsigned char phys00, phys01, phys10, phys11;
                phys00 = pmIndexedMap[64 * land + index00];
                phys01 = pmIndexedMap[64 * land + index01];
                phys10 = pmIndexedMap[64 * land + index10];
                phys11 = pmIndexedMap[64 * land + index11];

                // physmats can be the same, so choose not from individual weights of indicies, but from physmat weights
                float Sweight00 =
                  weight00 + (phys00 == phys01) * weight01 + (phys00 == phys10) * weight10 + (phys00 == phys11) * weight11;
                float Sweight01 = weight01 + (phys01 == phys10) * weight10 + (phys01 == phys11) * weight11;
                float Sweight10 = weight10 + (phys10 == phys11) * weight11;
                float Sweight11 = weight11;

                float maxWeight = max(max(Sweight00, Sweight01), max(Sweight10, Sweight11));
                unsigned char mat = phys00;
                mat = maxWeight == Sweight01 ? phys01 : mat;
                mat = maxWeight == Sweight10 ? phys10 : mat;
                mat = maxWeight == Sweight11 ? phys11 : mat;

                physMap[(y - detRectC[0].y) * physMap_w + x - detRectC[0].x] = mat;
              }
            }
          }
        for (int i = 0; i < stexMap.size(); i++)
          if (stexMap[i] && intptr_t(stexMap[i]) != intptr_t(-1))
            memfree(stexMap[i], tmpmem);
      }

      bool full_zero = true;
      for (int i = 0; i < physMap.size() && full_zero; i += 8)
        if ((*(int64_t *)&physMap[i]))
          full_zero = false;

      if (allow_debug_bitmap_dump)
      {
        if (physMap_w && physMap_h)
        {
          IBitMaskImageMgr::BitmapMask img;
          bitMaskImgMgr->createBitMask(img, physMap_w, physMap_h, 8);
          for (int y = 0; y < physMap_h; y++)
            for (int x = 0; x < physMap_w; x++)
              img.setMaskPixel8(x, y, physMap[y * physMap_w + x] * 8);
          bitMaskImgMgr->saveImage(img, ".", "pmMap2");
          bitMaskImgMgr->destroyImage(img);
        }
      }
      StaticGeometryContainer *geoCont = dagGeom->newStaticGeometryContainer();
      for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
      {
        IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
        IGatherStaticGeometry *geom = plugin->queryInterface<IGatherStaticGeometry>();
        if (!geom)
          continue;
        geom->gatherStaticVisualGeometry(*geoCont);
      }

      struct TexturePhysInfo
      {
        int alphaThreshold;
        bool heightAbove; // invert alpha
      };

      Tab<StaticGeometryNode *> decalNodes(tmpmem);
      Tab<SmallTab<int, TmpmemAlloc>> physMatIds(tmpmem);
      Tab<SmallTab<int, TmpmemAlloc>> bitmapIds(tmpmem);
      FastNameMap textureNames;
      Tab<TexturePhysInfo> texturePhysInfos(tmpmem);
      Tab<int> hasAtLeastOneSolidMat(tmpmem);
      decalNodes.reserve(geoCont->nodes.size());
      int hasAtLeastOneSolidMatCounter = 0;
      for (int i = 0; i < geoCont->nodes.size(); ++i)
      {
        StaticGeometryNode *node = geoCont->nodes[i];

        if (!node)
          continue;
        if (!node->mesh->mesh.check_mesh())
          continue;

        SmallTab<int, TmpmemAlloc> phMatIds;
        SmallTab<int, TmpmemAlloc> bmpIds;
        dag::Vector<bool> isSolidMat;
        clear_and_resize(phMatIds, node->mesh->mats.size());
        mem_set_ff(phMatIds);
        clear_and_resize(bmpIds, node->mesh->mats.size());
        mem_set_ff(bmpIds);
        clear_and_resize(isSolidMat, node->mesh->mats.size());
        mem_set_0(isSolidMat);
        bool nodeAdded = false;
        int solidMatCounter = 0;
        for (int mi = 0; mi < node->mesh->mats.size(); ++mi)
        {
          if (!node->mesh->mats[mi])
            continue;
          bool isDecal = false;
          CfgReader c;
          c.getdiv_text(String(128, "[q]\r\n%s\r\n", node->mesh->mats[mi]->scriptText.str()), "q");
          if (strstr(node->mesh->mats[mi]->className.str(), "decal"))
            isDecal = true;
          else if (
            strstr(node->mesh->mats[mi]->scriptText.str(), "render_landmesh_combined") && !c.getbool("render_landmesh_combined", 1))
            isDecal = true;
          if (isDecal)
          {
            const char *physMatName = c.getstr("phmat", NULL);
            if (!physMatName)
              continue;
            phMatIds[mi] = pmNames.addNameId(physMatName);
            if (hmlService->getIsSolidByPhysMatName(physMatName))
            {
              isSolidMat[mi] = true;
              solidMatCounter++;
            }
            if (!nodeAdded)
              decalNodes.push_back(node);
            const char *textureName = node->mesh->mats[mi]->textures[0]->fileName.str();
            if (textureNames.getNameId(textureName) < 0)
            {
              TexturePhysInfo &physInfo = texturePhysInfos.push_back();
              physInfo.alphaThreshold = c.getint("alpha_threshold", 127);
              physInfo.heightAbove = c.getint("height_above", 1);
            }
            bmpIds[mi] = textureNames.addNameId(textureName);
            nodeAdded = true;
            full_zero = false;
          }
        }
        if (nodeAdded)
        {
          if (solidMatCounter > 0)
          {
            int listSize = phMatIds.size();
            // We will copy solid ids to end of vector, so reserve space for them now
            phMatIds.reserve(solidMatCounter + listSize);
            bmpIds.reserve(solidMatCounter + listSize);
            // copy soft ids to begin of vector and solid ids to end of vector
            int copyTo = 0;
            for (int i = 0; i < listSize; i++)
            {
              if (isSolidMat[i])
              {
                // Make a copy of solid mat at the end of vector
                phMatIds.push_back(phMatIds[i]);
                bmpIds.push_back(bmpIds[i]);
              }
              else
              {
                if (i != copyTo)
                {
                  phMatIds[copyTo] = phMatIds[i];
                  bmpIds[copyTo] = bmpIds[i];
                }
                copyTo++;
              }
            }
            // Values in interval [copyTo..listSize) contain just part of initial vector;
            // all these values were copied to [0..copyTo) or [listSize..newSize) intervals. So this interval can be removed now.
            phMatIds.erase(phMatIds.begin() + copyTo, phMatIds.begin() + listSize);
            bmpIds.erase(bmpIds.begin() + copyTo, bmpIds.begin() + listSize);
            hasAtLeastOneSolidMatCounter++;
            hasAtLeastOneSolidMat.push_back(1);
          }
          else
          {
            hasAtLeastOneSolidMat.push_back(0);
          }
          physMatIds.push_back(phMatIds);
          bitmapIds.push_back(bmpIds);
        }
      }
      // making sort in order that solid physmats are drawn last
      int listSize = physMatIds.size();
      // We will copy solid ids to end of vector, so reserve space for them now
      physMatIds.reserve(hasAtLeastOneSolidMatCounter + listSize);
      bitmapIds.reserve(hasAtLeastOneSolidMatCounter + listSize);
      decalNodes.reserve(hasAtLeastOneSolidMatCounter + listSize);
      // copy soft ids to begin of vector and solid ids to end of vector
      int copyTo = 0;
      for (int i = 0; i < listSize; i++)
      {
        if (hasAtLeastOneSolidMat[i])
        {
          // Make a copy of solid mat at the end of vector
          physMatIds.push_back(physMatIds[i]);
          bitmapIds.push_back(bitmapIds[i]);
          decalNodes.push_back(decalNodes[i]);
        }
        else
        {
          if (i != copyTo)
          {
            physMatIds[copyTo] = physMatIds[i];
            bitmapIds[copyTo] = bitmapIds[i];
            decalNodes[copyTo] = decalNodes[i];
          }
          copyTo++;
        }
      }
      // Values in interval [copyTo..listSize) contain just part of initial vector;
      // all these values were copied to [0..copyTo) or [listSize..newSize) intervals. So this interval can be removed now.
      physMatIds.erase(physMatIds.begin() + copyTo, physMatIds.begin() + listSize);
      bitmapIds.erase(bitmapIds.begin() + copyTo, bitmapIds.begin() + listSize);
      decalNodes.erase(decalNodes.begin() + copyTo, decalNodes.begin() + listSize);

      if (!full_zero)
      {
        // write physmat map
        cwr.beginTaggedBlock(_MAKE4C('LMp2'));
        int st_pos = cwr.tell();

        int version = 1;
        cwr.writeInt32e(version);
        cwr.writeInt32e(pmNames.nameCount());
        for (int i = 0; i < pmNames.nameCount(); i++)
          cwr.writeDwString(pmNames.getName(i));

        cwr.writeInt32e(physMap_w);
        cwr.writeInt32e(physMap_h);
        cwr.writeFloat32e(detDivisor ? detRect[0].x : 0);
        cwr.writeFloat32e(detDivisor ? detRect[0].y : 0);
        cwr.writeFloat32e(detDivisor ? gridCellSize / detDivisor : 0);

        mkbindump::BinDumpSaveCB cwr_pm(2 << 10, cwr.getTarget(), cwr.WRITE_BE);

        TreeBitmapNode physMapParent;
        physMapParent.create(physMap, IPoint2(physMap_w, physMap_h));
        mkbindump::save_tree_bitmap(cwr_pm, &physMapParent);

        // write meshes
        debug("decal nodes %d", decalNodes.size());
        cwr_pm.writeInt16e(decalNodes.size());
        for (int i = 0; i < decalNodes.size(); ++i)
        {
          StaticGeometryNode *node = decalNodes[i];

          // Prepare
          Mesh tempMesh = node->mesh->mesh;
          tempMesh.transform(node->wtm);

          // Optimize
          Bitarray usedFaces;
          usedFaces.resize(tempMesh.face.size());
          usedFaces.reset();
          for (int fidx = 0; fidx < tempMesh.face.size(); ++fidx)
            if (physMatIds[i][tempMesh.face[fidx].mat] >= 0)
              usedFaces.set(fidx);
          tempMesh.removeFacesFast(usedFaces);
          tempMesh.kill_unused_verts();
          tempMesh.kill_bad_faces();
          tempMesh.sort_faces_by_mat();

          // Simplify
          SmallTab<Point2, TmpmemAlloc> vertices;
          SmallTab<Point2, TmpmemAlloc> texCoords;
          clear_and_resize(vertices, tempMesh.vert.size());
          for (int vert = 0; vert < tempMesh.vert.size(); ++vert)
            vertices[vert] = Point2::xz(tempMesh.vert[vert]);
          cwr_pm.writeInt16e(vertices.size());
          cwr_pm.writeRaw(vertices.data(), data_size(vertices));
          cwr_pm.writeInt16e(tempMesh.tvert[0].size());
          cwr_pm.writeRaw(tempMesh.tvert[0].data(), data_size(tempMesh.tvert[0]));

          if (tempMesh.face.empty())
          {
            cwr_pm.writeInt8e(0);  // 0 mat
            cwr_pm.writeInt16e(0); // 0 faces
            // useless node actually for some reason
            continue;
          }
          int curPhysMat = -1;
          int curFaceIdx = 0;
          int faceNumOffset = -1;
          for (int fi = 0; fi < tempMesh.face.size(); ++fi)
          {
            int physMat = physMatIds[i][tempMesh.face[fi].mat];
            if (physMat != curPhysMat)
            {
              cwr_pm.writeInt8e(physMat);
              curPhysMat = physMat;
              if (faceNumOffset >= 0)
              {
                cwr_pm.seekto(faceNumOffset);
                cwr_pm.writeInt16e(fi - curFaceIdx);
                cwr_pm.seekToEnd();
                cwr_pm.writeInt16e(bitmapIds[i][tempMesh.face[fi].mat]);
                for (int tfi = curFaceIdx; tfi < fi; ++tfi)
                {
                  cwr_pm.writeInt16e(tempMesh.tface[0][tfi].t[0]);
                  cwr_pm.writeInt16e(tempMesh.tface[0][tfi].t[1]);
                  cwr_pm.writeInt16e(tempMesh.tface[0][tfi].t[2]);
                }
              }
              faceNumOffset = cwr_pm.tell();
              curFaceIdx = fi;
              cwr_pm.writeInt16e(0); // 0 faces, we don't know number yet
            }
            cwr_pm.writeInt16e(tempMesh.face[fi].v[0]);
            cwr_pm.writeInt16e(tempMesh.face[fi].v[1]);
            cwr_pm.writeInt16e(tempMesh.face[fi].v[2]);
          }
          if (!tempMesh.tface[0].empty())
            cwr_pm.writeInt16e(bitmapIds[i][tempMesh.face.back().mat]);
          for (int tfi = curFaceIdx; tfi < tempMesh.tface[0].size(); ++tfi)
          {
            cwr_pm.writeInt16e(tempMesh.tface[0][tfi].t[0]);
            cwr_pm.writeInt16e(tempMesh.tface[0][tfi].t[1]);
            cwr_pm.writeInt16e(tempMesh.tface[0][tfi].t[2]);
          }
          cwr_pm.seekto(faceNumOffset);
          cwr_pm.writeInt16e(tempMesh.face.size() - curFaceIdx);
          cwr_pm.seekToEnd();

          // stop
          cwr_pm.writeInt8e(0);
          cwr_pm.writeInt16e(0);
        }

        const int delta = (EXTENDED_DECAL_BITMAP_SZ - DECAL_BITMAP_SZ) / 2;
        cwr_pm.writeInt16e(textureNames.nameCount());
        int texTypeId = DAEDITOR3.getAssetTypeId("tex");
        for (int i = 0; i < textureNames.nameCount(); ++i)
        {
          int numOfDrawedPixels = 0;
          HmapLandPlugin::PackedDecalBitmap bitmap;
          HmapLandPlugin::ExtendedDecalBitmap accumulated_bitmap;
          mem_set_0(bitmap);
          mem_set_0(accumulated_bitmap);
          String aname(DagorAsset::fpath2asset(TextureMetaData::decodeFileName(textureNames.getName(i), &tmp_stor)));
          if (aname.length() > 0 && aname[aname.length() - 1] == '*')
            erase_items(aname, aname.length() - 1, 1);
          DagorAsset *a = DAEDITOR3.getAssetByName(aname.str(), texTypeId);
          String fname;
          if (a)
            fname = eastl::move(a->getTargetFilePath());
          if (!fname.empty() && (is_tga_or_tiff(fname)))
          {
            TexImage32 *image = load_tga_or_tiff(fname, tmpmem);
            TexPixel32 *pixels = image->getPixels();
            for (int y = 0; y < DECAL_BITMAP_SZ; ++y)
            {
              for (int x = 0; x < DECAL_BITMAP_SZ; ++x)
              {
                float accumulatedAlpha = 0.f;
                for (int yy = (y * image->h) / DECAL_BITMAP_SZ; yy < ((y + 1) * image->h) / DECAL_BITMAP_SZ; ++yy)
                  for (int xx = (x * image->w) / DECAL_BITMAP_SZ; xx < ((x + 1) * image->w) / DECAL_BITMAP_SZ; ++xx)
                    accumulatedAlpha += pixels[yy * image->w + xx].a;

                const TexturePhysInfo &physInfo = texturePhysInfos[i];
                float threshold = physInfo.alphaThreshold;
                accumulatedAlpha *= float(sqr(DECAL_BITMAP_SZ)) / float(image->h * image->w);
                if ((accumulatedAlpha > threshold) == physInfo.heightAbove)
                {
                  accumulated_bitmap[((y + delta) * EXTENDED_DECAL_BITMAP_SZ) + (x + delta)] = 1;
                  numOfDrawedPixels++;
                }
              }
            }
            // assume that if less then 10% of bitmap are filled with physmat value then something is strange happened
            // we do it before closing process to understand how many pixels we have from alpha texture
            if (numOfDrawedPixels <= (DECAL_BITMAP_SZ * DECAL_BITMAP_SZ) * 0.1f)
            {
              DAEDITOR3.conWarning("There is something strange with texture \"%s\"\nBitmap filled only %d pixel of 1024\n"
                                   "Please check alpha texture or params in mat.blk \n",
                textureNames.getName(i), numOfDrawedPixels);
            }

            applyClosingPostProcess(accumulated_bitmap);
            bitmap = to_1bit_bitmap(accumulated_bitmap);
          }
          else
          {
            DAEDITOR3.conError("Unsuported texture '%s' used in decal for PhysMap, only tga's or tiff's supported",
              textureNames.getName(i));
            if (fname)
              DAEDITOR3.conError("Texture was tried to be loaded from '%s'", fname);
          }
          cwr_pm.write32ex(bitmap.data(), data_size(bitmap));
        }

        cwr.beginBlock();
        MemoryLoadCB mcrd(cwr_pm.getMem(), false);
        if (preferZstdPacking && allowOodlePacking)
        {
          cwr.writeInt32e(cwr_pm.getSize());
          oodle_compress_data(cwr.getRawWriter(), mcrd, cwr_pm.getSize());
        }
        else if (preferZstdPacking)
          zstd_compress_data(cwr.getRawWriter(), mcrd, cwr_pm.getSize(), 1 << 20, 19);
        else
          lzma_compress_data(cwr.getRawWriter(), 9, mcrd, cwr_pm.getSize());
        cwr.endBlock(preferZstdPacking ? (allowOodlePacking ? btag_compr::OODLE : btag_compr::ZSTD) : btag_compr::UNSPECIFIED);

        cwr.align8();
        cwr.endBlock();
        debug("LMpm written, %d packed, %d unpacked", cwr.tell() - st_pos, cwr_pm.getSize());
      }
      dagGeom->deleteStaticGeometryContainer(geoCont);
    }

    IWaterService *waterSrv = EDITORCORE->queryEditorInterface<IWaterService>();
    if (waterSrv)
      waterSrv->exportHeightmap(cwr, preferZstdPacking, allowOodlePacking);

    int genTime = dagTools->getTimeMsec() - time0;
    con.addMessage(ILogWriter::REMARK, "exported in %g seconds", genTime / 1000.0f);
    con.endLog();
  }
#if defined(USE_LMESH_ACES)
  else if ((st_mask & hmapSubtypeMask) && exportType == EXPORT_HMAP)
  {
    con.addMessage(ILogWriter::NOTE, "Exporting land...");

    if (!heightMap.isFileOpened())
    {
      con.addMessage(ILogWriter::ERROR, "No heightmap data");
      con.endLog();
      return false;
    }

    if (!landClsMap.isFileOpened())
    {
      con.addMessage(ILogWriter::ERROR, "No landClsMap data");
      con.endLog();
      return false;
    }

    if (hasColorTex && !colorMap.isFileOpened())
    {
      con.addMessage(ILogWriter::ERROR, "No colormap data");
      con.endLog();
      return false;
    }

    if (hasLightmapTex && !lightMapScaled.isFileOpened())
    {
      con.addMessage(ILogWriter::ERROR, "No lightmap data");
      con.endLog();
      return false;
    }

    if (exportType == EXPORT_HMAP)
    {
      cwr.beginTaggedBlock(_MAKE4C('hmap'));
      try
      {
        cwr.setOrigin();
        exportLand(cwr);
        cwr.popOrigin();
      }
      catch (IGenSave::SaveException e)
      {
        CoolConsole &con = DAGORED2->getConsole();
        con.startLog();
        con.addMessage(ILogWriter::ERROR, "Error exporting heightmap '%s'", e.excDesc);

        con.endLog();
        return false;
      }

      cwr.align8();
      cwr.endBlock();

      cwr.beginTaggedBlock(_MAKE4C('hset'));
      cwr.writeInt32e(render.gridStep);
      cwr.writeInt32e(render.radiusElems);
      cwr.writeInt32e(render.ringElems);
      cwr.align8();
      cwr.endBlock();
    }
  }
#endif

  exportSplines(cwr);
  if (cableService)
    cableService->exportCables(cwr);

  bool exp_err = false;
  BBox2 water_bb;
  bool has_water = false;
  water_bb.setempty();
  for (int i = 0; i < objEd.objectCount(); i++)
  {
    SplineObject *o = RTTI_cast<SplineObject>(objEd.getObject(i));
    BBox3 wbb;
    if (o && EditLayerProps::layerProps[o->getEditLayerIdx()].exp && o->isPoly() && o->polyGeom.altGeom &&
        o->polyGeom.bboxAlignStep > 0 && o->getWorldBox(wbb))
    {
      float grid = max(o->polyGeom.bboxAlignStep, 1.0f);
      wbb[0].x = floorf(wbb[0].x / grid) * grid;
      wbb[0].z = floorf(wbb[0].z / grid) * grid;
      wbb[1].x = ceilf(wbb[1].x / grid) * grid;
      wbb[1].z = ceilf(wbb[1].z / grid) * grid;
      water_bb += Point2::xz(wbb[0]);
      water_bb += Point2::xz(wbb[1]);
      has_water = true;
    }
  }
  if (has_water)
  {
    DAEDITOR3.conNote("export: has water");
    cwr.beginTaggedBlock(_MAKE4C('wt3d'));
    cwr.writeInt32e(0x20150909);
    if (water_bb.isempty())
      cwr.writeInt32e(0);
    else
    {
      cwr.writeInt32e(1);
      cwr.write32ex(&water_bb, sizeof(water_bb));
      DAEDITOR3.conNote("export: water bbox=%@", water_bb);
    }
    cwr.align8();
    cwr.endBlock();
  }
  BBox3 worldBBox;
  if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
    worldBBox = rigenSrv->calcWorldBBox();

  if (landMeshManager)
    worldBBox += hmlService->getLMeshBBoxWithHMapWBBox(*landMeshManager);
  if (!worldBBox.isempty())
  {
    cwr.beginTaggedBlock(_MAKE4C('WBBX'));
    cwr.write32ex(&worldBBox, sizeof(worldBBox));
    cwr.endBlock();
  }
  buildAndWriteNavMesh(cwr);

  // Navmesh exported, hide not exported RIs as before
  for (uint32_t i = 0; i < EditLayerProps::layerProps.size(); ++i)
    if (EditLayerProps::layerProps[i].type == EditLayerProps::ENT && !EditLayerProps::layerProps[i].exp)
      DAEDITOR3.setEntityLayerHiddenMask(DAEDITOR3.getEntityLayerHiddenMask() | (1ull << i));

  GeomObject *obj = dagGeom->newGeomObject(midmem);
  gatherStaticGeometry(*obj->getGeometryContainer(), StaticGeometryNode::FLG_RENDERABLE, false, 1);
  if (obj->getGeometryContainer()->nodes.size())
  {
    cwr.beginTaggedBlock(_MAKE4C('rivM'));
    if (!hmlService->exportGeomToShaderMesh(cwr, *obj->getGeometryContainer(), DAGORED2->getPluginFilePath(this, ".work/rivers.dat"),
          tn))
    {
      DAEDITOR3.conError("failed to export river geom");
      exp_err = true;
    }
    cwr.align8();
    cwr.endBlock();
  }
  dagGeom->deleteGeomObject(obj);

  int genTime = dagTools->getTimeMsec() - time0;
  con.addMessage(ILogWriter::REMARK, "exported in %g seconds", genTime / 1000.0f);
  con.endLog();

  return !exp_err;
}

void HmapLandPlugin::applyClosingPostProcess(HmapLandPlugin::ExtendedDecalBitmap &accum_bitmap)
{
  // dilation pass
  carray<char, EXTENDED_DECAL_BITMAP_SZ * EXTENDED_DECAL_BITMAP_SZ> temp;
  mem_copy_to(accum_bitmap, temp.data());
  for (int y = KERNEL_RAD; y < EXTENDED_DECAL_BITMAP_SZ - KERNEL_RAD; y++)
  {
    for (int x = KERNEL_RAD; x < EXTENDED_DECAL_BITMAP_SZ - KERNEL_RAD; x++)
    {
      for (int yy = -KERNEL_RAD; yy <= KERNEL_RAD; yy++)
      {
        for (int xx = -KERNEL_RAD; xx <= KERNEL_RAD; xx++)
        {
          char centralVal = accum_bitmap[y * EXTENDED_DECAL_BITMAP_SZ + x];
          char kernalVal = temp[(y + yy) * EXTENDED_DECAL_BITMAP_SZ + (x + xx)];
          accum_bitmap[y * EXTENDED_DECAL_BITMAP_SZ + x] = eastl::max(centralVal, kernalVal);
        }
      }
    }
  }
  // erosion pass
  mem_copy_to(accum_bitmap, temp.data());
  for (int y = KERNEL_RAD; y < EXTENDED_DECAL_BITMAP_SZ - KERNEL_RAD; y++)
  {
    for (int x = KERNEL_RAD; x < EXTENDED_DECAL_BITMAP_SZ - KERNEL_RAD; x++)
    {
      for (int yy = -KERNEL_RAD; yy <= KERNEL_RAD; yy++)
      {
        for (int xx = -KERNEL_RAD; xx <= KERNEL_RAD; xx++)
        {
          char centralVal = accum_bitmap[y * EXTENDED_DECAL_BITMAP_SZ + x];
          char kernalVal = temp[(y + yy) * EXTENDED_DECAL_BITMAP_SZ + (x + xx)];
          accum_bitmap[y * EXTENDED_DECAL_BITMAP_SZ + x] = eastl::min(centralVal, kernalVal);
        }
      }
    }
  }
}

HmapLandPlugin::PackedDecalBitmap HmapLandPlugin::to_1bit_bitmap(const HmapLandPlugin::ExtendedDecalBitmap &accum_bitmap)
{
  HmapLandPlugin::PackedDecalBitmap outputBitmap;
  mem_set_0(outputBitmap);
  int delta = (EXTENDED_DECAL_BITMAP_SZ - DECAL_BITMAP_SZ) / 2;
  for (int y = delta; y < DECAL_BITMAP_SZ + delta; y++)
  {
    for (int x = delta; x < DECAL_BITMAP_SZ + delta; x++)
    {
      outputBitmap[y - delta] |= accum_bitmap[y * EXTENDED_DECAL_BITMAP_SZ + x] << (x - delta);
    }
  }
  return outputBitmap;
}

void HmapLandPlugin::writeAddLtinputData(IGenSave &cwr)
{
  class BlankTextureRemap : public ITextureNumerator
  {
  public:
    virtual int addTextureName(const char *fname) { return -1; }
    virtual int addDDSxTexture(const char *fname, ddsx::Buffer &b) { return -1; }
    virtual int getTextureOrdinal(const char *fname) const { return -1; }
    virtual int getTargetCode() const { return _MAKE4C('PC'); }
  };

  BlankTextureRemap temp;
  mkbindump::BinDumpSaveCB bdcwr(128 << 20, _MAKE4C('PC'), false);
  buildAndWrite(bdcwr, temp, NULL);
  bdcwr.copyDataTo(cwr);
}
// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void HmapLandPlugin::requestHeightmapData(int x0, int y0, int step, int x_size, int y_size) {}

bool HmapLandPlugin::isLandPreloadRequestComplete() { return true; }


void HmapLandPlugin::getHeightmapData(int x0, int y0, int step, int x_size, int y_size, real *data, int stride_bytes)
{
  char *ptr = (char *)data;
  /*
  if (exportType == EXPORT_PSEUDO_PLANE)
  {
    float y0 = 0; //heightMap.heightOffset;
    for (int y=0, yc=y0; y<y_size; ++y, yc+=step)
      for (int x=0, xc=x0; x<x_size; ++x, xc+=step, ptr+=stride_bytes)
        *(real*)ptr=y0;
    return;
  }
  */

  if (render.showFinalHM)
  {
    for (int y = 0, yc = y0; y < y_size; ++y, yc += step)
      for (int x = 0, xc = x0; x < x_size; ++x, xc += step, ptr += stride_bytes)
        *(real *)ptr = heightMap.getFinalData(xc, yc);
  }
  else
  {
    for (int y = 0, yc = y0; y < y_size; ++y, yc += step)
      for (int x = 0, xc = x0; x < x_size; ++x, xc += step, ptr += stride_bytes)
        *(real *)ptr = heightMap.getInitialData(xc, yc);
  }
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static __forceinline uint16_t make565GrayColor(uint8_t c) { return ((c * 31 / 255) << 11) | ((c * 63 / 255) << 5) | (c * 31 / 255); }


/*
static __forceinline void packDxtCellDetailMap(uint8_t cell[16], uint8_t* &output,
  uint8_t detail_tex_ids[4])
{
  *(uint16_t*)output=0x07E0;
  output+=2;
  *(uint16_t*)output=0xF800;
  output+=2;

  uint32_t code=0;
  for (int i=0; i<16; ++i)
  {
    if (cell[i]==detail_tex_ids[0])
      code|=1<<(i*2);
    else if (cell[i]==detail_tex_ids[1])
      {} // zero code
    else
      code|=3<<(i*2);
  }

  *(uint32_t*)output=code;
  output+=4;
}
*/


static __forceinline void packDxtCellGrayColor(uint8_t cell[16], uint8_t *&output)
{
  uint32_t bestCode = 0;

  uint8_t minColor = cell[0], maxColor = cell[0];

  for (int i = 1; i < 16; ++i)
  {
    if (cell[i] < minColor)
      minColor = cell[i];
    else if (cell[i] > maxColor)
      maxColor = cell[i];
  }

  static uint8_t ramp[4];
  ramp[0] = minColor;
  ramp[1] = maxColor;
  ramp[2] = (minColor * 2 + maxColor + 1) / 3;
  ramp[3] = (maxColor * 2 + minColor + 1) / 3;

  for (int i = 0; i < 16; ++i)
  {
    uint32_t bestInd = 0;
    int bestDiff = abs(cell[i] - ramp[bestInd]);

    for (int j = 1; j < 4; ++j)
    {
      int d = abs(cell[i] - ramp[j]);
      if (d < bestDiff)
      {
        bestDiff = d;
        bestInd = j;
      }
    }

    bestCode |= bestInd << (i * 2);
  }

  // output packed color
  uint16_t pc1 = ::make565GrayColor(minColor);
  uint16_t pc2 = ::make565GrayColor(maxColor);

  if (pc1 == pc2)
  {
    bestCode = 0;
  }
  else if (pc1 < pc2)
  {
    // swap
    uint16_t c = pc1;
    pc1 = pc2;
    pc2 = c;
    bestCode ^= 0x55555555;
  }

  *(uint16_t *)output = pc1;
  output += 2;
  *(uint16_t *)output = pc2;
  output += 2;

  *(uint32_t *)output = bestCode;
  output += 4;
}


static __forceinline void packDxtCellInterpAlpha(uint8_t cell[16], uint8_t *&output)
{
  uint8_t minAlpha = 255, maxAlpha = 0;
  bool useBlack = false;

  int i;
  for (i = 0; i < 16; ++i)
  {
    if (cell[i] == 0)
      useBlack = true;
    else
    {
      if (cell[i] < minAlpha)
        minAlpha = cell[i];
      if (cell[i] > maxAlpha)
        maxAlpha = cell[i];
    }
  }

  static uint8_t alpha[8];

  if (useBlack)
  {
    // 6-alpha block.
    // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
    alpha[0] = minAlpha;
    alpha[1] = maxAlpha;
    alpha[2] = (4 * alpha[0] + 1 * alpha[1] + 2) / 5; // Bit code 010
    alpha[3] = (3 * alpha[0] + 2 * alpha[1] + 2) / 5; // Bit code 011
    alpha[4] = (2 * alpha[0] + 3 * alpha[1] + 2) / 5; // Bit code 100
    alpha[5] = (1 * alpha[0] + 4 * alpha[1] + 2) / 5; // Bit code 101
    alpha[6] = 0;                                     // Bit code 110
    alpha[7] = 255;                                   // Bit code 111
  }
  else
  {
    // 8-alpha block.
    // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
    alpha[0] = maxAlpha;
    alpha[1] = minAlpha;
    alpha[2] = (6 * alpha[0] + 1 * alpha[1] + 3) / 7; // bit code 010
    alpha[3] = (5 * alpha[0] + 2 * alpha[1] + 3) / 7; // bit code 011
    alpha[4] = (4 * alpha[0] + 3 * alpha[1] + 3) / 7; // bit code 100
    alpha[5] = (3 * alpha[0] + 4 * alpha[1] + 3) / 7; // bit code 101
    alpha[6] = (2 * alpha[0] + 5 * alpha[1] + 3) / 7; // bit code 110
    alpha[7] = (1 * alpha[0] + 6 * alpha[1] + 3) / 7; // bit code 111
  }

  __int64 code = 0;
  for (i = 0; i < 16; ++i)
  {
    int a = cell[i];

    int bestCode = 0;
    int bestDiff = abs(a - alpha[0]);

    for (int j = 1; j < 8; ++j)
    {
      int d = abs(a - alpha[j]);
      if (d < bestDiff)
      {
        bestDiff = d;
        bestCode = j;
      }
    }

    code |= __int64(bestCode) << (i * 3);
  }

  // output packed alpha
  output[0] = alpha[0];
  output[1] = alpha[1];
  output += 2;

  *(uint32_t *)output = code;
  output += 4;
  *(uint16_t *)output = code >> 32;
  output += 2;
}


/*void HmapLandPlugin::loadLandDetailTexture(int x0, int y0, Texture *tex, Texture *tex2,
  uint8_t detail_tex_ids[HMAX_DET_TEX_NUM+1], bool *done_mark)
{
  loadLandDetailTexture(x0, y0, tex, tex2, detail_tex_ids, done_mark,
    render.detMapSize, render.detMapElemSize);
}*/

void HmapLandPlugin::loadLandDetailTexture(int x0, int y0, Texture *tex1, Texture *tex2,
  carray<uint8_t, LMAX_DET_TEX_NUM> &detail_tex_ids, bool *done_mark, int texSize, int elemSize)
{
  char *imgPtr = NULL;
  char *imgPtr2 = NULL;
  int stride = 0, stride2 = 0;
  d3d_err(tex1->lockimg((void **)&imgPtr, stride, 0, TEXLOCK_WRITE));
  d3d_err(tex2->lockimg((void **)&imgPtr2, stride2, 0, TEXLOCK_WRITE));
  loadLandDetailTexture(_MAKE4C('PC'), x0, y0, imgPtr, stride, imgPtr2, stride2, detail_tex_ids, done_mark, texSize, elemSize, true,
    true);

  d3d_err(tex1->unlockimg());
  d3d_err(tex2->unlockimg());
}

#define PROFILE_BLOCK_START(varn) int varn##Start = dagTools->getTimeMsec();
#define PROFILE_BLOCK_END(varn)   int varn = dagTools->getTimeMsec() - varn##Start;
static __forceinline unsigned convert_4bit_to8bit(unsigned a)
{
  a &= 0xF;
  return (a << 4) | a;
}
static __forceinline unsigned convert_argb4_to_argb8(unsigned a)
{
  return (convert_4bit_to8bit(a)) | (convert_4bit_to8bit(a >> 4) << 8) | (convert_4bit_to8bit(a >> 8) << 16) |
         (convert_4bit_to8bit(a >> 12) << 24);
}

int HmapLandPlugin::loadLandDetailTexture(unsigned targetCode, int x0, int y0, char *tex1, int stride, char *tex2, int stride2,
  carray<uint8_t, LMAX_DET_TEX_NUM> &detail_tex_ids, bool *done_mark, int det_size, int det_elem_size, bool tex1_rgba, bool tex2_rgba)
{
  uint16_t *imgPtr = (uint16_t *)tex1;
  uint8_t *imgPtr2 = (uint8_t *)tex2;
  x0 *= det_elem_size;
  y0 *= det_elem_size;

  int texSize = det_size;
  int texDataSize = min(det_elem_size + 4, texSize);

  SmallTab<uint8_t, TmpmemAlloc> typeRemap;
  clear_and_resize(typeRemap, 256);
  mem_set_ff(typeRemap);
  memset(detail_tex_ids.data(), 0xFF, LMAX_DET_TEX_NUM);

  PROFILE_BLOCK_START(calcUsedTypes)
  int typesUsed = getMostUsedDetTex(x0, y0, texDataSize, detail_tex_ids.data(), typeRemap.data(), tex2 ? LMAX_DET_TEX_NUM : 5);
  PROFILE_BLOCK_END(calcUsedTypes)
  if (tex1_rgba && tex2_rgba)
    for (int i = 0; i < detail_tex_ids.size(); i++)
      if (detail_tex_ids[i] != 0xFF)
        detail_tex_ids[i] = lcRemap.size() ? lcRemap[detail_tex_ids[i]] : 0xFF;

  // encode texels
  PROFILE_BLOCK_START(encodeTexels);
  SmallTab<TexPixel32, TmpmemAlloc> pix2Data;
  unsigned pix2DataPtrStride = 0;
  if (tex2 && !tex2_rgba)
  {
    clear_and_resize(pix2Data, texSize * texSize);
    pix2DataPtrStride = texSize * 4;
    mem_set_0(pix2Data);
  }
  if (tex2_rgba)
  {
    G_ASSERT(stride2 >= texSize * 4);
    pix2DataPtrStride = stride2;
  }
  TexPixel32 *pix2DataPtr = tex2_rgba ? (TexPixel32 *)tex2 : pix2Data.data();

  int add_step1 = (stride / (tex1_rgba ? 4 : 2) - texDataSize) * (tex1_rgba ? 2 : 1);
  int add_step2 = pix2DataPtrStride / 4 - texDataSize;
  if (detTexWtMap)
  {
    uint16_t *p = (uint16_t *)tex1;
    TexPixel32 *p2 = pix2DataPtr;

    for (int yc = y0, yce = yc + texDataSize; yc < yce; yc++, p += add_step1, p2 += add_step2)
      for (int xc = x0, xce = xc + texDataSize; xc < xce; xc++, p++, p2++)
      {
        uint64_t p_wt = detTexWtMap->getData(xc, yc);
        uint64_t p_idx = detTexIdxMap->getData(xc, yc);
        unsigned u = 0, u2 = 0xFF000000;

        for (int i = 0; i < 8; i++, p_wt >>= 8, p_idx >>= 8)
          if (p_wt)
          {
            int idx = typeRemap[int(p_idx & 0xFF)];
            if (idx == 0xFF)
              continue;
            if (idx < 3)
              u |= ((p_wt >> 4) & 0xF) << (8 - idx * 4);
            else if (idx == 3)
              u |= (p_wt & 0xF0) << 8;
            else if (idx < 6)
              u2 |= (p_wt & 0xFF) << (16 - (idx - 4) * 8);
          }
          else
            break;
        if (tex1_rgba)
        {
          *(unsigned *)p = convert_argb4_to_argb8(u); //-V1032
          p++;
        }
        else
          *p = u;
        if (tex2)
          p2->u = u2;
      }
    G_ASSERT((char *)p <= tex1 + stride * texSize);
    G_ASSERT(!tex2 || (char *)p2 <= ((char *)pix2DataPtr) + pix2DataPtrStride * texSize);
  }
  else
  {
    G_ASSERTF(lcmScale <= 1, "lcmScale > 1 without detTex WeightMap not supported!");
    uint16_t *p = (uint16_t *)tex1;
    TexPixel32 *p2 = pix2DataPtr;

    for (int yc = y0, yce = yc + texDataSize; yc < yce; yc++, p += add_step1, p2 += add_step2)
      for (int xc = x0, xce = xc + texDataSize; xc < xce; xc++, p++, p2++)
      {
        uint8_t idx = typeRemap[getDetLayerDesc().getLayerData(landClsMap.getData(xc, yc))];
        if (idx == 0xFF)
          idx = 0;
        if (idx < 3)
          *p = 0xF << (8 - idx * 4);
        else if (idx == 3)
          *p = 0xF << 12;
        else if (idx < 6 && tex2)
          p2->u = (0xFF << (16 - (idx - 4) * 8)) | 0xFF000000;
        if (tex1_rgba)
        {
          *(unsigned *)p = convert_argb4_to_argb8(*p); //-V1032
          p++;
        }
      }
    G_ASSERT((char *)p <= tex1 + stride * texSize);
    G_ASSERT(!tex2 || (char *)p2 <= ((char *)pix2DataPtr) + pix2DataPtrStride * texSize);
  }
  PROFILE_BLOCK_END(encodeTexels);

  PROFILE_BLOCK_START(dxtCvt);
  if (tex2 && !tex2_rgba)
  {
    ddstexture::Converter cnv;
    cnv.format = ddstexture::Converter::fmtDXT1a;
    if (targetCode == _MAKE4C('iOS'))
      cnv.format = ddstexture::Converter::fmtASTC8, stride2 = stride2 / 2;
    cnv.mipmapType = ddstexture::Converter::mipmapNone;
    cnv.mipmapCount = 0;

    DynamicMemGeneralSaveCB cwr(tmpmem, texSize * texSize + 256, 64 << 10);
    if (!dagTools->ddsConvertImage(cnv, cwr, pix2DataPtr, texSize, texSize, texSize * 4))
      DAEDITOR3.conError("cannot convert %dx%d image at (%d,%d)", texSize, texSize, x0, y0);
    else
    {
      if (0 && typesUsed > 1 && cnv.format != ddstexture::Converter::fmtASTC8)
      {
        save_tga32(String(260, "det_wt_%d,%d_%d.tga", x0, y0, typesUsed), pix2DataPtr, texSize, texSize, texSize * 4);

        Tab<TexPixel32> pix(tmpmem);
        pix.resize(texSize * texSize);
        uint16_t *p = (uint16_t *)tex1;
        for (int i = 0; i < pix.size(); i++, p++)
          pix[i].u = ((*p & 0xF) << 4) | ((*p & 0xF0) << 8) | ((*p & 0xF00) << 12) | ((*p & 0xF000) << 16);
        save_tga32(String(260, "det_wt1_%d,%d_%d.tga", x0, y0, typesUsed), pix.data(), texSize, texSize, texSize * 4);

        FullFileSaveCB fcwr(String(260, "det_wt_%d,%d_%d.dds", x0, y0, typesUsed));
        fcwr.write(cwr.data(), cwr.size());
      }
      G_ASSERT((texSize / 4) * stride2 == cwr.size() - 128);
      memcpy(tex2, cwr.data() + 128, (texSize / 4) * stride2);
    }
  }
  PROFILE_BLOCK_END(dxtCvt);

  int totalMs = calcUsedTypes + encodeTexels + dxtCvt;
  if (totalMs > 30)
    debug("%s done for %d msec (calcUsed %d, %d encode, %d dxtCvt), texSize %d", __FUNCTION__, totalMs, calcUsedTypes, encodeTexels,
      dxtCvt, texSize);

  *done_mark = true;
  return typesUsed;
}

static const int MAX_DETAILS_PER_QUAD = 64;
struct QuadData
{
  uint8_t *data;
  int w, h;

  QuadData() : data(NULL), w(0), h(0) {}
  ~QuadData()
  {
    if (data)
      memfree_anywhere(data);
  }
};
class UpdateRawTextures
{
public:
  Texture *texture[256];
  bool textureLocked[256];
  uint8_t *textureData[256];
  int textureStride[256];
  uint8_t lockedTextures[256];
  int lockedTexturesCnt;
  int wd, ht;
  UpdateRawTextures(int w, int h) : wd(w), ht(h)
  {
    memset(texture, 0, sizeof(texture));
    memset(textureLocked, 0, sizeof(textureLocked));
    memset(textureData, 0, sizeof(textureData));
    memset(textureStride, 0, sizeof(textureStride));
    memset(lockedTextures, 0, sizeof(lockedTextures));
    lockedTexturesCnt = 0;
  }
  void startUpdateData(int x0, int y0, int x1, int y1) { G_ASSERT(lockedTexturesCnt == 0); }

  void updateData(uint8_t texId, int x, int y, uint8_t val)
  {
    if (!textureLocked[texId])
    {
      if (!texture[texId])
        texture[texId] = d3d::create_tex(NULL, wd, ht, TEXFMT_L8 | TEXCF_READABLE | TEXCF_DYNAMIC, 1, "blueWhite");
      G_ASSERT(texture[texId]);
      unsigned texLockFlags = TEXLOCK_UPDATEFROMSYSTEX | TEXLOCK_NOSYSLOCK | TEXLOCK_RWMASK | TEXLOCK_SYSTEXLOCK;
      if (texture[texId]->lockimg((void **)&textureData[texId], textureStride[texId], 0, texLockFlags))
      {
        textureLocked[texId] = true;
        lockedTextures[lockedTexturesCnt] = texId;
        lockedTexturesCnt++;
      }
    }
    if (!textureLocked[texId])
      return;
    textureData[texId][clamp(x, 0, wd - 1) + clamp(y, 0, ht - 1) * textureStride[texId]] = val;
  }
  void endUpdateData(uint8_t texId, int x, int y, uint8_t val)
  {
    for (int i = 0; i < lockedTexturesCnt; ++i)
    {
      int texId = lockedTextures[i];
      G_ASSERT(textureLocked[texId]);
      if (textureLocked[texId])
      {
        texture[texId]->unlockimg();
        textureLocked[texId] = false;
      }
    }
    lockedTexturesCnt = 0;
  }
};

template <class UpdateLC>
int HmapLandPlugin::updateLandclassWeight(int x0, int y0, int x1, int y1, UpdateLC &update_cb)
{
  G_ASSERT(detTexWtMap)
  update_cb.startUpdateData(x0, y0, x1, y1);
  int w = y1 - y0 + 1, h = x1 - x0 + 1;
  for (int yc = y0; yc <= y1; yc++)
    for (int xc = x0; xc <= x1; xc++)
    {
      uint64_t p_wt = detTexWtMap->getData(xc, yc);
      uint64_t p_idx = detTexIdxMap->getData(xc, yc);
      unsigned u = 0, u2 = 0xFF000000;

      for (int i = 0; i < 8; i++, p_wt >>= 8, p_idx >>= 8)
        if (p_wt)
        {
          int idx = int(p_idx & 0xFF);
          update_cb.updateData(idx, xc, yc, uint8_t(p_wt & 0xFF));
        }
        else
          break;
    }
  update_cb.endUpdateData();

  return typesUsed;
}

// Return total types should be used. If more, than max_detail_tex - the cell is invalid
int HmapLandPlugin::getMostUsedDetTex(int x0, int y0, int texDataSize, uint8_t *det_tex_ids, uint8_t *idx_remap, int max_dtn)
{
  int texUsed = 0;

  // get most used types
  SmallTab<unsigned, TmpmemAlloc> typeCnt;
  clear_and_resize(typeCnt, numDetailTextures >= 1 ? numDetailTextures : 1);
  mem_set_0(typeCnt);

  if (detTexWtMap)
  {
    for (int yc = y0, yce = yc + texDataSize; yc < yce; yc++)
      for (int xc = x0, xce = xc + texDataSize; xc < xce; xc++)
      {
        uint64_t p_wt = detTexWtMap->getData(xc, yc);
        uint64_t p_idx = detTexIdxMap->getData(xc, yc);
        for (int i = 0; i < 8; i++, p_wt >>= 8, p_idx >>= 8)
          if (p_wt)
          {
            if (unsigned(p_idx & 0xFF) < typeCnt.size())
              typeCnt[int(p_idx & 0xFF)] += p_wt & 0xFF;
          }
          else
            break;
      }
  }
  else
    for (int y = 0, yc = y0; y < texDataSize; ++y, yc++)
      for (int x = 0, xc = x0; x < texDataSize; ++x, xc++)
      {
        uint8_t t = getDetLayerDesc().getLayerData(landClsMap.getData(xc, yc));
        if (t >= typeCnt.size())
          t = 0;
        typeCnt[t] += 255;
      }
  uint8_t taken[256];
  memset(taken, 0xFF, 256);
  for (int i = 0; i < max_dtn; ++i)
  {
    int bi = 0;
    unsigned bc = 0;

    for (int j = 0; j < typeCnt.size(); ++j)
      if (taken[j] == 0xFF && typeCnt[j] > bc)
      {
        bc = typeCnt[j];
        bi = j;
      }

    if (!bc)
      break;
    det_tex_ids[i] = bi;
    taken[bi] = i;
    // idx_remap[bi] = i;
    texUsed++;
  }

  struct AscendCompare
  {
    bool operator()(uint8_t a, uint8_t b) const { return a < b; }
  };
  stlsort::sort(det_tex_ids, det_tex_ids + texUsed, AscendCompare());
  for (int i = 0; i < texUsed; ++i)
    idx_remap[det_tex_ids[i]] = i;

  return texUsed;
}

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//
void autocenterHM(PropPanel2 &panel)
{
  bool autocenter = panel.getBool(PID_HEIGHTMAP_AUTOCENTER);
  if (autocenter)
  {
    Point2 size = panel.getPoint2(PID_HM_SIZE_METERS);
    Point2 offset = Point2(-size.x / 2, -size.y / 2);

    panel.setPoint2(PID_HEIGHTMAP_OFFSET, offset);
  }

  panel.setEnabledById(PID_HEIGHTMAP_OFFSET, !autocenter);
};


void HmapLandPlugin::createHeightmap()
{
  class CreateHeightmapEventHandler : public ControlEventHandler
  {
  public:
    CreateHeightmapEventHandler(HmapLandPlugin *plugin) : mPlugin(plugin) {}

    void onChange(int pcb_id, PropPanel2 *panel)
    {
      Point2 p2;

      switch (pcb_id)
      {
        case PID_HM_SIZE_PIXELS:
        {
          p2 = panel->getPoint2(PID_HM_SIZE_PIXELS);
          if (p2.x != real2int(p2.x) || p2.y != real2int(p2.y))
          {
            p2.x = real2int(p2.x);
            p2.y = real2int(p2.y);
            panel->setPoint2(PID_HM_SIZE_PIXELS, p2);
          }
        }
        break;

        case PID_GRID_CELL_SIZE:
        {
          p2 = panel->getPoint2(PID_HM_SIZE_PIXELS) * panel->getFloat(PID_GRID_CELL_SIZE);

          panel->setPoint2(PID_HM_SIZE_METERS, p2);
          autocenterHM(*panel);
        }
        break;

        case PID_HEIGHTMAP_AUTOCENTER: autocenterHM(*panel); break;

        case PID_SCRIPT_FILE:
        {
          String newfn(panel->getText(pcb_id));

          if (newfn.length())
            ::defaultHM.scriptPath = newfn;
        }
        break;
      }
    }

  private:
    HmapLandPlugin *mPlugin;
  };

  CreateHeightmapEventHandler eventHandler(this);


  CDialogWindow *myDlg = DAGORED2->createDialog(_pxScaled(280), _pxScaled(600), "Create heightmap...");
  PropertyContainerControlBase *myPanel = myDlg->getPanel();
  myPanel->setEventHandler(&eventHandler);


  PropertyContainerControlBase *grp = myPanel->createGroupBox(-1, "Heightmap size");

  grp->createPoint2(PID_HM_SIZE_PIXELS, "Size in pixels:", ::defaultHM.sizePixels);
  grp->createPoint2(PID_HM_SIZE_METERS, "Size in meters:", ::defaultHM.sizeMeters, 0);

  grp = myPanel->createGroupBox(-1, "Heightmap parameters");

  grp->createEditFloat(PID_GRID_CELL_SIZE, "Cell size:", ::defaultHM.cellSize);
  grp->createEditFloat(PID_HEIGHT_SCALE, "Height scale:", ::defaultHM.heightScale);
  grp->createEditFloat(PID_HEIGHT_OFFSET, "Height offset:", ::defaultHM.heightOffset);
  grp->createPoint2(PID_HEIGHTMAP_OFFSET, "Origin offset:", ::defaultHM.originOffset, !::defaultHM.doAutocenter);
  grp->createCheckBox(PID_HEIGHTMAP_AUTOCENTER, "Autocenter", ::defaultHM.doAutocenter);

  PropertyContainerControlBase *subGrp = grp->createGroupBox(-1, "Preload collision area");

  subGrp->createPoint2(PID_HM_COLLISION_OFFSET, "Offset (in %):", ::defaultHM.collisionOffset);
  subGrp->createPoint2(PID_HM_COLLISION_SIZE, "Size (in %):", ::defaultHM.collisionSize);
  subGrp->createCheckBox(PID_HM_COLLISION_SHOW, "Show area", ::defaultHM.collisionShow);

  grp->createFileEditBox(PID_SCRIPT_FILE, "Script file:", "" /*::defaultHM.scriptPath*/);

  Tab<String> filters(tmpmem);
  filters.push_back() = "Script files (*.nut)|*.nut";
  filters.push_back() = "All files (*.*)|*.*";
  myPanel->setStrings(PID_SCRIPT_FILE, filters);
  // myPanel->setText(PID_SCRIPT_FILE, DAGORED2->getSdkDir());


  if (myDlg->showDialog() == DIALOG_ID_OK)
  {
    const char *scriptPath = ::defaultHM.scriptPath;

    if (scriptPath && *scriptPath && !::dd_file_exist(scriptPath))
    {
      wingw::message_box(wingw::MBS_EXCL, "Creation error", "Color generation script file not exists of not specified.");
      return;
    }

    ::defaultHM.sizePixels = myPanel->getPoint2(PID_HM_SIZE_PIXELS);
    ::defaultHM.sizeMeters = myPanel->getPoint2(PID_HM_SIZE_METERS);
    ::defaultHM.cellSize = myPanel->getFloat(PID_GRID_CELL_SIZE);
    ::defaultHM.heightScale = myPanel->getFloat(PID_HEIGHT_SCALE);
    ::defaultHM.heightOffset = myPanel->getFloat(PID_HEIGHT_OFFSET);
    ::defaultHM.doAutocenter = myPanel->getBool(PID_HEIGHTMAP_AUTOCENTER);
    ::defaultHM.originOffset = myPanel->getPoint2(PID_HEIGHTMAP_OFFSET);
    ::defaultHM.collisionOffset = myPanel->getPoint2(PID_HM_COLLISION_OFFSET);
    ::defaultHM.collisionSize = myPanel->getPoint2(PID_HM_COLLISION_SIZE);
    ::defaultHM.collisionShow = myPanel->getBool(PID_HM_COLLISION_SHOW);

    colorGenScriptFilename = ::defaultHM.scriptPath;

    gridCellSize = ::defaultHM.cellSize;
    if (gridCellSize <= 0)
      gridCellSize = 1.0;

    heightMap.heightScale = ::defaultHM.heightScale;
    heightMap.heightOffset = ::defaultHM.heightOffset;
    heightMapOffset = ::defaultHM.originOffset;
    doAutocenter = ::defaultHM.doAutocenter;
    collisionArea.ofs = ::defaultHM.collisionOffset;
    collisionArea.sz = ::defaultHM.collisionSize;
    collisionArea.show = ::defaultHM.collisionShow;
    tileTexName = defaultHM.tileTex;
    tileXSize = defaultHM.tileTexSz.x;
    tileYSize = defaultHM.tileTexSz.y;
    if (colorGenScriptFilename.empty() && !genLayers.size())
      addGenLayer("default");

    if (propPanel)
      propPanel->fillPanel();

    IPoint2 size(::defaultHM.sizePixels.x, ::defaultHM.sizePixels.y);
    CoolConsole &con = DAGORED2->getConsole();

    MapStorage<float> &hm = heightMap.getInitialMap();
    hm.reset(size.x, size.y, 0);
    createHeightmapFile(con);

    heightMap.resetFinal();

    if (!heightMap.flushData())
    {
      con.addMessage(ILogWriter::ERROR, "Error saving heightmap data");
      con.endLog();
      return;
    }

    getColorGenVarsFromScript();
    applyHmModifiers(false);
    generateLandColors();
    calcFastLandLighting();
    // autocenterHeightmap(propPanel->getPanel(), false);
    autocenterHeightmap(propPanel->getPanelWindow(), false);

    resetRenderer();
    onLandSizeChanged();
  }

  DAGORED2->deleteDialog(myDlg);
}


void HmapLandPlugin::importHeightmap()
{
  int import_det_hmap = 0;
  if (detDivisor)
    switch (wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNOCANCEL, "Select heightmap to import",
      "Level contains both main and detailed heightmap\n"
      "Select YES to import MAIN heightmap, or NO to import DETAILED heightmap"))
    {
      case wingw::MB_ID_YES: import_det_hmap = 0; break;
      case wingw::MB_ID_NO: import_det_hmap = 1; break;
      default: return;
    }

  String path;
  path = wingw::file_open_dlg(NULL, "Import heightmap", hmapImportCaption, "r32", lastHmapImportPath);

  if (path.length())
  {
    lastHmapImportPath = path;
    if (import_det_hmap)
      lastHmapImportPathDet = path;
    else
      lastHmapImportPathMain = path;

    const char *ext = dd_get_fname_ext(path);

    if (stricmp(ext, ".r32") != 0 && stricmp(ext, ".height") != 0 && !import_det_hmap && lastMinHeight[import_det_hmap] != MAX_REAL &&
        lastHeightRange[import_det_hmap] != MAX_REAL)
    {
      if (wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNO, "Set parameters from last export",
            "Set offset and scale from last heightmap export?\n"
            "Last offset = %g, scale = %g",
            lastMinHeight[import_det_hmap], lastHeightRange[import_det_hmap]) == wingw::MB_ID_YES)
      {
        heightMap.heightOffset = lastMinHeight[import_det_hmap];
        heightMap.heightScale = lastHeightRange[import_det_hmap];
      }
    }
    if (stricmp(ext, ".r32") != 0 && stricmp(ext, ".height") != 0)
    {
      class HmapImportSizeDlg : public ControlEventHandler
      {
        enum
        {
          ID_MIN_HEIGHT = 1,
          ID_HEIGHT_RANGE,
        };

      public:
        HmapImportSizeDlg(float &min_height, float &height_range) : minHeight(min_height), heightRange(height_range), mDialog(NULL)
        {
          mDialog = DAGORED2->createDialog(_pxScaled(240), _pxScaled(170), "Import HMAP data range");

          PropPanel2 &panel = *mDialog->getPanel();
          panel.setEventHandler(this);

          if (minHeight > MAX_REAL / 2 && heightRange > MAX_REAL / 2)
            minHeight = 0, heightRange = 8192;
          else
            panel.createStatic(-1, "From last HMAP export:");
          panel.createEditFloat(ID_MIN_HEIGHT, "Minimal height:", minHeight);
          panel.createEditFloat(ID_HEIGHT_RANGE, "Height range:", heightRange);
        }

        ~HmapImportSizeDlg() { del_it(mDialog); }

        virtual bool execute()
        {
          int ret = mDialog->showDialog();

          if (ret == DIALOG_ID_OK)
          {
            PropPanel2 &panel = *mDialog->getPanel();
            minHeight = panel.getFloat(ID_MIN_HEIGHT);
            heightRange = panel.getFloat(ID_HEIGHT_RANGE);
            return true;
          }
          return false;
        }

      private:
        float &minHeight;
        float &heightRange;

        CDialogWindow *mDialog;
      };

      HmapImportSizeDlg dlg(lastMinHeight[import_det_hmap], lastHeightRange[import_det_hmap]);
      if (!dlg.execute())
        return;
    }

    if (importHeightmap(path, import_det_hmap ? HeightmapTypes::HEIGHTMAP_DET : HeightmapTypes::HEIGHTMAP_MAIN))
    {
      updateHeightMapTex(false);
      updateHeightMapTex(true);
      if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Rebuild everything",
            "Rebuild everything for updated HMAP data?\n(recommended)") == wingw::MB_ID_YES)
      {
        generateLandColors();
        calcFastLandLighting();
      }
      autocenterHeightmap(propPanel ? propPanel->getPanelWindow() : NULL, false);
      resetRenderer();
    }
  }
}

bool HmapLandPlugin::checkAndReimport(String &path, FileChangeStat &lastChangeOld, HeightmapTypes type)
{
  const char *type_str = "?";
  switch (type)
  {
    case HeightmapTypes::HEIGHTMAP_DET: type_str = "detailed"; break;
    case HeightmapTypes::HEIGHTMAP_MAIN: type_str = "main"; break;
    case HeightmapTypes::HEIGHTMAP_WATER_DET: type_str = "water det"; break;
    case HeightmapTypes::HEIGHTMAP_WATER_MAIN: type_str = "water main"; break;
  }

  if (path.length())
  {
    DagorStat s;
    if (df_stat(path, &s) < 0)
    {
      DAEDITOR3.conError("Failed get filestat for: %s", path);
      return false;
    }
    if (s.mtime == lastChangeOld.mtime && s.size == lastChangeOld.size)
    {
      DAEDITOR3.conWarning("Skip reloading %s heightmap: %s (not changed)", type_str, path);
      return false;
    }
    if (importHeightmap(path, type))
    {
      lastChangeOld.mtime = s.mtime;
      lastChangeOld.size = s.size;
      DAEDITOR3.conNote("=== Reloaded %s heightmap: %s", type_str, path);

      if (type == HeightmapTypes::HEIGHTMAP_MAIN)
        updateHeightMapTex(false);
      else if (type == HeightmapTypes::HEIGHTMAP_DET)
        updateHeightMapTex(true);
      else if (type == HeightmapTypes::HEIGHTMAP_WATER_DET || type == HeightmapTypes::HEIGHTMAP_WATER_MAIN)
      {
        if (IWaterService *waterSrv = EDITORCORE->queryEditorInterface<IWaterService>())
        {
          Point2 hmapSize(heightMap.getMapSizeX() * gridCellSize, heightMap.getMapSizeY() * gridCellSize);
          Point2 hmapEnd = heightMapOffset + hmapSize;
          waterSrv->setHeightmap(waterHeightmapDet, waterHeightmapMain, waterHeightMinRangeDet, waterHeightMinRangeMain, detRect,
            BBox2(heightMapOffset.x, heightMapOffset.y, hmapEnd.x, hmapEnd.y));
        }
      }
      return true;
    }
    else
    {
      DAEDITOR3.conError("Failed to re-import %s heightmap: %s", type_str, path);
      return false;
    }
  }
  else
  {
    DAEDITOR3.conWarning("No %s heightmap to be reloaded, import it first", type_str);
    return false;
  }
}

void HmapLandPlugin::reimportHeightmap()
{
  bool rebuildDet = checkAndReimport(lastHmapImportPathDet, lastChangeDet, HeightmapTypes::HEIGHTMAP_DET);
  bool rebuildMain = checkAndReimport(lastHmapImportPathMain, lastChangeMain, HeightmapTypes::HEIGHTMAP_MAIN);
  bool rebuildWaterDet = checkAndReimport(lastWaterHeightmapImportPathDet, lastChangeWaterDet, HeightmapTypes::HEIGHTMAP_WATER_DET);
  bool rebuildWaterMain =
    checkAndReimport(lastWaterHeightmapImportPathMain, lastChangeWaterMain, HeightmapTypes::HEIGHTMAP_WATER_MAIN);

  if (rebuildDet || rebuildMain || rebuildWaterDet || rebuildWaterMain)
  {
    DAEDITOR3.conNote("Rebuild reloaded heightmaps");
    generateLandColors();
    calcFastLandLighting();
    autocenterHeightmap(propPanel ? propPanel->getPanelWindow() : NULL, false);
    resetRenderer();
  }
}


void HmapLandPlugin::eraseHeightmap()
{
  if (wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNO, "Confirm erase", "Erase heightmap?") == wingw::MB_ID_YES)
  {
    heightMap.clear();
    landMeshMap.clear(true);
    colorMap.eraseFile();
    lightMapScaled.eraseFile();
    landClsMap.eraseFile();
    if (detTexIdxMap)
      detTexIdxMap->eraseFile();
    if (detTexWtMap)
      detTexWtMap->eraseFile();
    resetRenderer();
    onLandSizeChanged();
  }
}

void HmapLandPlugin::importWaterHeightmap(bool det)
{
  IWaterService *waterSrv = EDITORCORE->queryEditorInterface<IWaterService>();
  if (!waterSrv)
    return;
  enum
  {
    ID_MIN_HEIGHT = 1,
    ID_HEIGHT_RANGE,
  };
  eastl::string dialogText{eastl::string::CtorSprintf{}, "Importing water %s heightmap.", det ? "DET" : "MAIN"};
  CDialogWindow *dialog = DAGORED2->createDialog(_pxScaled(240), _pxScaled(170), dialogText.c_str());
  PropPanel2 &panel = *dialog->getPanel();
  panel.setEventHandler(this);
  Point2 *waterHeightMinRange = &waterHeightMinRangeDet;
  HeightMapStorage *waterHeightmap = &waterHeightmapDet;
  if (!det)
  {
    waterHeightMinRange = &waterHeightMinRangeMain;
    waterHeightmap = &waterHeightmapMain;
  }
  if (waterHeightMinRange->y <= 0)
  {
    waterHeightMinRange->x = heightMap.heightOffset;
    waterHeightMinRange->y = heightMap.heightScale;
  }
  panel.createEditFloat(ID_MIN_HEIGHT, "Minimal height:", waterHeightMinRange->x);
  panel.createEditFloat(ID_HEIGHT_RANGE, "Height range:", waterHeightMinRange->y);
  int ret = dialog->showDialog();
  if (ret == DIALOG_ID_OK)
  {
    PropPanel2 &panel = *dialog->getPanel();
    waterHeightMinRange->x = panel.getFloat(ID_MIN_HEIGHT);
    waterHeightMinRange->y = panel.getFloat(ID_HEIGHT_RANGE);
    String path;
    String initialPath = det ? lastWaterHeightmapImportPathDet : lastWaterHeightmapImportPathMain;
    if (initialPath.empty())
      initialPath = lastWaterHeightmapImportPath;
    path = wingw::file_open_dlg(NULL, "Import heightmap", hmapImportCaption, "r32", initialPath);
    del_it(dialog);

    if (path.length())
    {
      lastWaterHeightmapImportPath = path;
      if (det)
        lastWaterHeightmapImportPathDet = path;
      else
        lastWaterHeightmapImportPathMain = path;
      waterHeightmap->heightOffset = 0.0;
      waterHeightmap->heightScale = 1.0;
      hmlService->destroyStorage(*waterHeightmap);
      hmlService->createStorage(*waterHeightmap, detRectC[1].x - detRectC[0].x, detRectC[1].y - detRectC[0].y, false);
      createWaterHmapFile(DAGORED2->getConsole(), det);
      if (!importHeightmap(path, det ? HeightmapTypes::HEIGHTMAP_WATER_DET : HeightmapTypes::HEIGHTMAP_WATER_MAIN))
      {
        hmlService->destroyStorage(*waterHeightmap);
        hmlService->createStorage(*waterHeightmap, 0, 0, false);
        waterHeightmap->getInitialMap().eraseFile();
      }
      else
      {
        Point2 hmapSize(heightMap.getMapSizeX() * gridCellSize, heightMap.getMapSizeY() * gridCellSize);
        Point2 hmapEnd = heightMapOffset + hmapSize;
        waterSrv->setHeightmap(waterHeightmapDet, waterHeightmapMain, waterHeightMinRangeDet, waterHeightMinRangeMain, detRect,
          BBox2(heightMapOffset.x, heightMapOffset.y, hmapEnd.x, hmapEnd.y));
      }
    }
  }
}

void HmapLandPlugin::eraseWaterHeightmap()
{
  enum
  {
    ID_REMOVE_MAIN = 1,
    ID_REMOVE_DET,
  };
  eastl::string dialogText{eastl::string::CtorSprintf{}, "Choose heightmaps for erase."};
  CDialogWindow *dialog = DAGORED2->createDialog(_pxScaled(240), _pxScaled(170), dialogText.c_str());
  PropPanel2 &panel = *dialog->getPanel();
  panel.setEventHandler(this);
  panel.createCheckBox(ID_REMOVE_MAIN, "Erase MAIN hmap");
  panel.createCheckBox(ID_REMOVE_DET, "Erase DET hmap");
  int ret = dialog->showDialog();
  if (ret == DIALOG_ID_OK)
  {
    PropPanel2 &panel = *dialog->getPanel();
    bool removeCheckboxes[2] = {panel.getBool(ID_REMOVE_MAIN), panel.getBool(ID_REMOVE_DET)};
    HeightMapStorage *waterHeightmap[2] = {&waterHeightmapMain, &waterHeightmapDet};
    for (int i = 0; i < 2; i++)
    {
      if (!removeCheckboxes[i])
        continue;
      hmlService->destroyStorage(*waterHeightmap[i]);
      hmlService->createStorage(*waterHeightmap[i], 0, 0, false);
      waterHeightmap[i]->getInitialMap().eraseFile();
    }
    Point2 hmapSize(heightMap.getMapSizeX() * gridCellSize, heightMap.getMapSizeY() * gridCellSize);
    Point2 hmapEnd = heightMapOffset + hmapSize;
    IWaterService *waterSrv = EDITORCORE->queryEditorInterface<IWaterService>();
    if (waterSrv)
      waterSrv->setHeightmap(waterHeightmapDet, waterHeightmapMain, waterHeightMinRangeDet, waterHeightMinRangeMain, detRect,
        BBox2(heightMapOffset.x, heightMapOffset.y, hmapEnd.x, hmapEnd.y));
  }
}

void HmapLandPlugin::exportLand()
{
  String path = wingw::file_save_dlg(NULL, "Export land to game", "Land files (*.lnd.dat)|*.lnd.dat|All files (*.*)|*.*", "lnd.dat",
    lastLandExportPath);

  if (path.length())
  {
    lastLandExportPath = path;

    if (exportLand(path))
      wingw::message_box(wingw::MBS_INFO, "Export land to game", "Export was successful.");
  }
}

void HmapLandPlugin::exportLoftMasks(const char *_out_folder, int main_hmap_sz, int det_hmap_sz, float hmin, float hmax,
  int prefab_dest_idx)
{
  ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>();
  FastIntList loft_layers;
  OAHashNameMap<true> loft_tags;
  uint64_t layers_hide_mask = DAEDITOR3.getEntityLayerHiddenMask();

  String out_folder[2];
  for (int i = 0; i < 2; i++)
  {
    out_folder[i] = _out_folder;
    if (lastExpLoftUseRect[i] && lastExpLoftCreateAreaSubfolders)
    {
      out_folder[i].aprintf(0, "/.loft-area(%g,%g)-%gx%g", lastExpLoftRect[i][0].x, lastExpLoftRect[i][0].y,
        lastExpLoftRect[i].width().x, lastExpLoftRect[i].width().y);
      dd_mkdir(out_folder[i]);
    }
  }

  loft_layers.addInt(0);
  if (prefab_dest_idx >= 0)
    prefab_dest_idx = loft_tags.addNameId(String(0, "prefabs_%d", prefab_dest_idx));

  if (splSrv)
  {
    splSrv->gatherLoftLayers(loft_layers, false);
    splSrv->gatherLoftTags(loft_tags);
  }
  for (int i = 0; i < objEd.splinesCount(); ++i)
    objEd.getSpline(i)->gatherPolyGeomLoftTags(loft_tags);

  for (int i = 0; i < 2; i++)
  {
    Tab<SimpleString> old_files;
    find_files_in_folder(old_files, out_folder[i], ".ddsx", false, true, false);
    for (SimpleString &s : old_files)
    {
      const char *fn = dd_get_fname(s);
      if (strncmp(fn, "loftMask_", 9) != 0)
        continue;
      if ((i == 0 && strncmp(fn + 9, "m_", 2) == 0) || (i == 1 && strncmp(fn + 9, "d_", 2) == 0))
        if (strchr("mhif", *(dd_get_fname_ext(fn) - 1)))
          dd_erase(s);
    }
  }

  GeomObject *all_loft_obj = dagGeom->newGeomObject(midmem);
  for (int lli = 0; lli < loft_layers.size(); lli++)
  {
    int ll = loft_layers.getList()[lli];
    if (splSrv)
      splSrv->gatherStaticGeometry(*all_loft_obj->getGeometryContainer(), StaticGeometryNode::FLG_RENDERABLE, false, ll, 2, -1,
        layers_hide_mask);
    for (int i = 0; i < objEd.splinesCount(); ++i)
      if (((layers_hide_mask >> objEd.getSpline(i)->getEditLayerIdx()) & 1) == 0)
        objEd.getSpline(i)->gatherStaticGeometry(*all_loft_obj->getGeometryContainer(), StaticGeometryNode::FLG_RENDERABLE, false, ll,
          2);
  }

  int layers_exported = 0;
  auto gather_layer_func = [&](int ltNid, const char *ln) {
    GeomObject *obj = dagGeom->newGeomObject(midmem);

    if (ltNid == prefab_dest_idx)
    {
      if (IGenEditorPlugin *p = DAGORED2->getPluginByName("_prefabEntMgr"))
        if (IGatherStaticGeometry *geom = p->queryInterface<IGatherStaticGeometry>())
          geom->gatherStaticVisualGeometryForStage(*obj->getGeometryContainer(), prefabs::LOFT_MASK_EXPORT);
    }
    else
      for (int i = 0; i < all_loft_obj->getGeometryContainer()->nodes.size(); ++i)
      {
        StaticGeometryNode *node = all_loft_obj->getGeometryContainer()->nodes[i];
        if (node && ltNid == loft_tags.getNameId(node->script.getStr("layerTag", "")))
        {
          obj->getGeometryContainer()->nodes.push_back(node);
          all_loft_obj->getGeometryContainer()->nodes[i] = NULL;
        }
      }

    if (obj->getGeometryContainer()->nodes.size())
    {
      obj->setTm(TMatrix::IDENT);
      obj->notChangeVertexColors(true);
      dagGeom->geomObjectRecompile(*obj);
      obj->notChangeVertexColors(false);

      String fn0, fn1, fnI, fnF;
      if (main_hmap_sz > 0)
      {
        fn0.printf(0, "%s/loftMask_m_%s_m.ddsx", out_folder[0], ln);
        fn1.printf(0, "%s/loftMask_m_%s_h.ddsx", out_folder[0], ln);
        fnI.printf(0, "%s/loftMask_m_%s_i.ddsx", out_folder[0], ln);
        fnF.printf(0, "%s/loftMask_m_%s_f.ddsx", out_folder[1], ln);
        hmlService->exportLoftMasks(fn0, fnI, fn1, ltNid == prefab_dest_idx ? nullptr : fnF.str(), main_hmap_sz,
          lastExpLoftUseRect[0] ? Point3::xVy(lastExpLoftRect[0][0], hmin) : Point3::xVz(landBox[0], hmin),
          lastExpLoftUseRect[0] ? Point3::xVy(lastExpLoftRect[0][1], hmax) : Point3::xVz(landBox[1], hmax), obj);
      }
      if (det_hmap_sz > 0)
      {
        fn0.printf(0, "%s/loftMask_d_%s_m.ddsx", out_folder[1], ln);
        fn1.printf(0, "%s/loftMask_d_%s_h.ddsx", out_folder[1], ln);
        fnI.printf(0, "%s/loftMask_d_%s_i.ddsx", out_folder[1], ln);
        fnF.printf(0, "%s/loftMask_d_%s_f.ddsx", out_folder[1], ln);
        hmlService->exportLoftMasks(fn0, fnI, fn1, ltNid == prefab_dest_idx ? nullptr : fnF.str(), det_hmap_sz,
          Point3::xVy(lastExpLoftUseRect[1] ? lastExpLoftRect[1][0] : detRect[0], hmin),
          Point3::xVy(lastExpLoftUseRect[1] ? lastExpLoftRect[1][1] : detRect[1], hmax), obj);
      }
      layers_exported++;
    }
    dagGeom->deleteGeomObject(obj);
  };
  gather_layer_func(-1, "def");
  iterate_names(loft_tags, gather_layer_func);
  dagGeom->deleteGeomObject(all_loft_obj);
  wingw::message_box(layers_exported ? wingw::MBS_INFO : wingw::MBS_EXCL, "Loft masks exported",
    (lastExpLoftUseRect[0] || lastExpLoftUseRect[1]) && lastExpLoftCreateAreaSubfolders
      ? String(0, "Exported %d loft masks (stage:i=2) to\n\n%s\n%s", layers_exported, main_hmap_sz > 0 ? out_folder[0].str() : "",
          det_hmap_sz > 0 ? out_folder[1].str() : "")
      : String(0, "Exported %d loft masks (stage:i=2) to\n  %s", layers_exported, _out_folder));
}

void HmapLandPlugin::exportHeightmap()
{
  int export_det_hmap = 0;
  if (detDivisor)
    switch (wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNOCANCEL, "Select heightmap to export",
      "Level contains both main and detailed heightmap\n"
      "Select YES to export MAIN heightmap, or NO to export DETAILED heightmap"))
    {
      case wingw::MB_ID_YES: export_det_hmap = 0; break;
      case wingw::MB_ID_NO: export_det_hmap = 1; break;
      default: return;
    }

  String path = wingw::file_save_dlg(NULL, "Export heightmap",
    "TIFF 16-bit files (*.tif)|*.tif;*.tiff|"
    "Raw 32f (*.r32)|*.r32|Raw 16 (*.r16)|*.r16|Photoshop Raw 16 (*.raw)|*.raw|"
    "Raw 32f with header (*.height)|*.height|TGA files (*.tga)|*.tga|All files (*.*)|*.*",
    "r32", lastHmapExportPath);

  if (path.length())
  {
    lastHmapExportPath = path;

    bool doExport = true;
    real minHeight = lastMinHeight[export_det_hmap];
    real heightRange = lastHeightRange[export_det_hmap];

    const char *ext = dd_get_fname_ext(path);
    if (!ext)
    {
      path += ".r16";
      ext = dd_get_fname_ext(path);
    }

    if (stricmp(ext, ".r32") != 0 && stricmp(ext, ".height") != 0)
    {
      HeightMapStorage &hm = export_det_hmap ? heightMapDet : heightMap;
      int x0 = export_det_hmap ? detRectC[0].x : 0;
      int y0 = export_det_hmap ? detRectC[0].y : 0;
      int x1 = export_det_hmap ? detRectC[1].x : heightMap.getMapSizeX();
      int y1 = export_det_hmap ? detRectC[1].y : heightMap.getMapSizeY();

      real minHeightHm;
      real heightRangeHm;
      real minH = hm.getFinalData(x0, x0);
      real maxH = minH;

      for (int y = y0; y < y1; ++y)
      {
        for (int x = x0; x < x1; ++x)
        {
          real h = hm.getFinalData(x, y);

          if (h < minH)
            minH = h;
          else if (h > maxH)
            maxH = h;
        }
      }

      minHeightHm = minH;
      heightRangeHm = maxH - minH;

      if (minHeight == MAX_REAL || heightRange == MAX_REAL)
      {
        minHeight = minHeightHm;
        heightRange = heightRangeHm;
      }

      HmapExportSizeDlg dlg(minHeight, heightRange, minHeightHm, heightRangeHm);
      doExport = dlg.execute();
    }

    if (doExport)
    {
      lastMinHeight[export_det_hmap] = minHeight;
      lastHeightRange[export_det_hmap] = heightRange;

      exportHeightmap(path, minHeight, heightRange, export_det_hmap);
    }
  }
}


void HmapLandPlugin::autocenterHeightmap(PropPanel2 *panel, bool reset_render)
{
  Point2 oldOffset = heightMapOffset;

  if (doAutocenter)
  {
    heightMapOffset.x = -heightMap.getMapSizeX() * gridCellSize / 2;
    heightMapOffset.y = -heightMap.getMapSizeY() * gridCellSize / 2;

    if (heightMapOffset != oldOffset && reset_render)
      resetRenderer();
  }

  if (panel)
  {
    panel->setPoint2(PID_HEIGHTMAP_OFFSET, heightMapOffset);
    panel->setEnabledById(PID_HEIGHTMAP_OFFSET, !doAutocenter);
  }
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void HmapLandPlugin::exportColormap()
{
  String path =
    wingw::file_save_dlg(NULL, "Export color map", "TGA files (*.tga)|*.tga|All files (*.*)|*.*", "tga", lastColormapExportPath);

  if (path.length())
  {
    lastColormapExportPath = path;
    exportColormap(path);
  }
}


static bool exportColormapAsTga(const char *filename, MapStorage<E3DCOLOR> &colormap)
{
  CoolConsole &con = DAGORED2->getConsole();

  file_ptr_t handle = ::df_open(filename, DF_WRITE | DF_CREATE);

  if (!handle)
  {
    con.addMessage(ILogWriter::ERROR, "Can't create file '%s'", filename);
    return false;
  }

  int mapSizeX = colormap.getMapSizeX();
  int mapSizeY = colormap.getMapSizeY();

  SmallTab<E3DCOLOR, TmpmemAlloc> buf;
  clear_and_resize(buf, mapSizeX);

  con.startProgress();
  con.setActionDesc("exporting color map...");
  con.setTotal(mapSizeY);

  ::df_write(handle, "\0\0\2\0\0\0\0\0\0\0\0\0", 12);
  ::df_write(handle, &mapSizeX, 2);
  ::df_write(handle, &mapSizeY, 2);
  ::df_write(handle, "\x20\x08", 2);

  for (int y = 0; y < mapSizeY; ++y)
  {
    for (int x = 0; x < mapSizeX; ++x)
      buf[x] = colormap.getData(x, y);

    if (::df_write(handle, buf.data(), data_size(buf)) != data_size(buf))
    {
      con.addMessage(ILogWriter::ERROR, "Error writing file '%s'", filename);
      ::df_close(handle);
      con.endProgress();
      return false;
    }

    colormap.unloadUnchangedData(y + 1);

    con.incDone();
  }

  colormap.unloadAllUnchangedData();

  con.endProgress();

  ::df_close(handle);

  return true;
}


bool HmapLandPlugin::exportColormap(String &filename)
{
  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  if (exportColormapAsTga(filename, colorMap))
    con.addMessage(ILogWriter::NOTE, "Exported color map to '%s'", (const char *)filename);

  con.endLog();
  return true;
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static real skyFunc(real t)
{
  if (t <= 0)
    return 0.25f;
  return 0.25f / ((1 + t));
}


Point3 HmapLandPlugin::calcSunLightDir() const
{
  return -Point3(cosf(sunAzimuth) * cosf(sunZenith), sinf(sunZenith), sinf(sunAzimuth) * cosf(sunZenith));
}

static float calc_sky(float h, float hl, float hr, float hu, float hd, float gridCellSize)
{
  float sky;
  sky = skyFunc((hu - h) / gridCellSize);
  sky += skyFunc((hd - h) / gridCellSize);
  sky += skyFunc((hl - h) / gridCellSize);
  sky += skyFunc((hr - h) / gridCellSize);
  return sky;
}

static Point3 get_normal(float hl, float hr, float hu, float hd, float gridCellSize)
{
  return normalize(Point3(hl - hr, 2 * gridCellSize, hd - hu));
}


inline void HmapLandPlugin::getNormal(int x, int y, Point3 &normal)
{
  real hl, hr, hu, hd;
  hl = heightMap.getFinalData(x - 1, y);
  hr = heightMap.getFinalData(x + 1, y);
  hu = heightMap.getFinalData(x, y + 1);
  hd = heightMap.getFinalData(x, y - 1);
  normal = get_normal(hl, hr, hu, hd, gridCellSize);
}

inline void HmapLandPlugin::getNormalAndSky(int x, int y, Point3 &normal, real &sky, float *height)
{
  real h, hl, hr, hu, hd;
  h = heightMap.getFinalData(x, y);
  hl = heightMap.getFinalData(x - 1, y);
  hr = heightMap.getFinalData(x + 1, y);
  hu = heightMap.getFinalData(x, y + 1);
  hd = heightMap.getFinalData(x, y - 1);

  normal = get_normal(hl, hr, hu, hd, gridCellSize);
  if (!height)
    sky = calc_sky(h, hl, hr, hu, hd, gridCellSize);
  else
  {
    int ht = heightMap.getMapSizeY(), wd = heightMap.getMapSizeX();
    h = height[clamp(y, 0, ht - 1) * wd + clamp(x, 0, wd - 1)];
    hl = height[clamp(y, 0, ht - 1) * wd + clamp(x - 1, 0, wd - 1)];
    hr = height[clamp(y, 0, ht - 1) * wd + clamp(x + 1, 0, wd - 1)];
    hu = height[clamp(y + 1, 0, ht - 1) * wd + clamp(x - 1, 0, wd - 1)];
    hd = height[clamp(y - 1, 0, ht - 1) * wd + clamp(x + 1, 0, wd - 1)];
    sky = calc_sky(h, hl, hr, hu, hd, gridCellSize);
  }
}

unsigned HmapLandPlugin::calcFastLandLightingAt(float ox, float oy, const Point3 &sun_light_dir)
{
  Point2 frac, cell;
  int x, y;
  frac.x = modff(ox, &cell.x);
  frac.y = modff(oy, &cell.y);
  x = cell.x;
  y = cell.y;
  Point3 normal;
  real sky;
  if (fabsf(ox - x) < 0.001 && fabsf(oy - y) < 0.001)
  {
    getNormalAndSky(x, y, normal, sky);
  }
  else
  {
    Point3 normalLU, normalRU, normalLD, normalRD;
    real skyLU, skyRU, skyLD, skyRD;
    getNormalAndSky(x, y, normalLU, skyLU);
    getNormalAndSky(x + 1, y, normalRU, skyRU);
    getNormalAndSky(x, y + 1, normalLD, skyLD);
    getNormalAndSky(x + 1, y + 1, normalRD, skyRD);
    normal =
      normalize((normalLU * (1 - frac.x) + normalRU * frac.x) * (1 - frac.y) + (normalLD * (1 - frac.x) + normalRD * frac.x) * frac.y);
    sky = (skyLU * (1 - frac.x) + skyRU * frac.x) * (1 - frac.y) + (skyLD * (1 - frac.x) + skyRD * frac.x) * frac.y;
  }

  //  if ( true /*render.showFinalHM*/ )
  //  {

  int sunLight = real2int(normal * sun_light_dir * -255);
  if (sunLight < 0)
    sunLight = 0;
  else if (sunLight > 255)
    sunLight = 255;

  int skyLight = sky * 255;
  if (skyLight < 0)
    skyLight = 0;
  else if (skyLight > 255)
    skyLight = 255;

  unsigned lt = sunLight | (skyLight << 8);
  return lt;
}

unsigned HmapLandPlugin::calcNormalAt(float ox, float oy)
{
  Point2 frac, cell;
  int x, y;
  frac.x = modff(ox, &cell.x);
  frac.y = modff(oy, &cell.y);
  x = cell.x;
  y = cell.y;
  Point3 normal;

  if (fabsf(ox - x) < 0.001 && fabsf(oy - y) < 0.001)
  {
    getNormal(x, y, normal);
  }
  else
  {
    Point3 normalLU, normalRU, normalLD, normalRD;
    getNormal(x, y, normalLU);
    getNormal(x + 1, y, normalRU);
    getNormal(x, y + 1, normalLD);
    getNormal(x + 1, y + 1, normalRD);

    normal =
      normalize((normalLU * (1 - frac.x) + normalRU * frac.x) * (1 - frac.y) + (normalLD * (1 - frac.x) + normalRD * frac.x) * frac.y);
  }

  unsigned nx = real2uchar(normal.x * 0.5 + 0.5);

  unsigned nz = real2uchar(normal.z * 0.5 + 0.5);

  unsigned nm = nx | (nz << 8);
  return nm;
}

void HmapLandPlugin::blurLightmap(int kernel_size, float sigma, const IBBox2 *calc_box)
{

  int time0 = dagTools->getTimeMsec();
  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  con.startProgress();
  con.setActionDesc("gaussian blur on lightmap");
  con.setTotal(lightMapScaled.getMapSizeY());

  static float weights[5 * 5], totalWeight;
  static int previos_kernel_size = 0;
  static float previos_sigma = 0.0;
  if (previos_sigma != sigma || previos_kernel_size != kernel_size)
  {
    int weightsStride = (kernel_size * 2 + 1);
    G_ASSERT(weightsStride <= 5);
    totalWeight = 0;
    for (int j = -kernel_size, id = 0; j <= kernel_size; ++j)
      for (int i = -kernel_size; i <= kernel_size; ++i, ++id)
      {
        float expPow = -(i * i + j * j) / (2.0f * sigma * sigma * kernel_size * kernel_size); //-V636
        float w = expf(expPow) / (sqrtf(TWOPI) * sigma);
        weights[id] = w;
        totalWeight += w;
      }
    for (int i = 0; i < weightsStride * weightsStride; ++i)
      weights[i] /= totalWeight * 255.0f;
  }

  IBBox2 calcBox;
  if (calc_box)
    calcBox = *calc_box;
  else
  {
    calcBox[0].x = 0;
    calcBox[1].x = lightMapScaled.getMapSizeX();
    calcBox[0].y = 0;
    calcBox[1].y = lightMapScaled.getMapSizeY();
  }
  IBBox2 filterBox;
  filterBox = calcBox;
  filterBox[0] -= IPoint2(1, 1);
  filterBox[1] += IPoint2(1, 1);
  filterBox[0].x = max(filterBox[0].x, 0);
  filterBox[0].y = max(filterBox[0].y, 0);

  filterBox[1].x = min(filterBox[0].x, lightMapScaled.getMapSizeX());
  filterBox[1].y = min(filterBox[0].y, lightMapScaled.getMapSizeY());

  SmallTab<uint32_t, TmpmemAlloc> lightMapBlurred;
  // todo: can be optimized by coping only required info
  // lightMapBlurred.resize(filterBox.width().x*filterBox.width().y);

  int width = lightMapScaled.getMapSizeX();
  int height = lightMapScaled.getMapSizeY();
  clear_and_resize(lightMapBlurred, width * height);
  for (int i = 0, y = 0; y < lightMapScaled.getMapSizeY(); ++y)
    for (int x = 0; x < lightMapScaled.getMapSizeX(); ++x, ++i)
    {
      lightMapBlurred[i] = lightMapScaled.getData(x, y);
    }

  for (int y = calcBox[0].y; y < calcBox[1].y; ++y)
  {
    for (int x = calcBox[0].x; x < calcBox[1].x; ++x)
    {
      float ltSky = 0, ltSun = 0;
      for (int j = -kernel_size, id = 0; j <= kernel_size; ++j)
      {
        for (int i = -kernel_size; i <= kernel_size; ++i, ++id)
        {
          unsigned lt = lightMapBlurred[width * clamp(y + j, 0, height - 1) + clamp(x + i, 0, width - 1)];
          lightMapScaled.getData(x + i, y + j);
          float w = weights[id];
          ltSky += (lt & 0xFF) * w;
          ltSun += ((lt >> 8) & 0xFF) * w;
        }
      }
      lightMapScaled.setData(x, y, real2uchar(ltSky) | (real2uchar(ltSun) << 8) | (lightMapBlurred[width * y + x] & 0xFFFF0000));
    }
    con.incDone();
  }
  con.endProgress();
  int genTime = dagTools->getTimeMsec() - time0;
  con.addMessage(ILogWriter::REMARK, "reloaded in %g seconds", genTime / 1000.0f);
  con.endLog();
}

void HmapLandPlugin::calcGoodLandLighting()
{
#if defined(USE_LMESH_ACES)
  calcLandLighting<false, true>();
#else
  calcLandLighting<true, true>(); //< something wrong here
#endif
}


void HmapLandPlugin::calcGoodLandLightingInBox(const IBBox2 &calc_box)
{
  IBBox2 box;
  box[0].x = calc_box[0].x * lightmapScaleFactor;
  box[0].y = calc_box[0].y * lightmapScaleFactor;
  box[1].x = calc_box[1].x * lightmapScaleFactor;
  box[1].y = calc_box[1].y * lightmapScaleFactor;

#if defined(USE_LMESH_ACES)
  calcLandLighting<false, false>(&box);
#else
  calcLandLighting<true, false>(&box);
#endif

  resetRenderer();
}


void HmapLandPlugin::calcFastLandLighting() { calcLandLighting<false, false>(); }
// #include<image/dag_tga.h>

template <bool cast_shadows, bool high_quality>
bool HmapLandPlugin::calcLandLighting(const IBBox2 *calc_box)
{
  if (!hasLightmapTex)
    return false;

  CoolConsole &con = DAGORED2->getConsole();
  applyHmModifiers(false);

  if (!calc_box)
  {
    lightMapScaled.reset(heightMap.getMapSizeX() * lightmapScaleFactor, heightMap.getMapSizeY() * lightmapScaleFactor, 0xFFFF);

    createLightmapFile(con);
  }
  else
  {
    String fname(DAGORED2->getPluginFilePath(this, LIGHTMAP_FILENAME));
    if (!::dd_file_exist(fname))
      createLightmapFile(con);
  }

  int time0 = dagTools->getTimeMsec();

  Point3 sunLightDir = calcSunLightDir();
  if (shadowDensity < 0)
    shadowDensity = 0;
  else if (shadowDensity > 1)
    shadowDensity = 1;

  int startX = calc_box ? calc_box->lim[0].x : 0;
  int endX = calc_box ? calc_box->lim[1].x + 1 : lightMapScaled.getMapSizeX();

  int startY = calc_box ? calc_box->lim[0].y : 0;
  int endY = calc_box ? calc_box->lim[1].y + 1 : lightMapScaled.getMapSizeY();

  if (startX < 0)
    startX = 0;

  if (endX > lightMapScaled.getMapSizeX())
    endX = lightMapScaled.getMapSizeX();

  if (startY < 0)
    startY = 0;

  if (endY > lightMapScaled.getMapSizeY())
    endY = lightMapScaled.getMapSizeY();
  if (endX <= startX || endY <= startY)
    return true;

  con.startLog();

#if USE_LMESH_ACES
  const int normalsSS = 3;
  SmallTab<uint16_t, TmpmemAlloc> normalsMap;
  SmallTab<float, TmpmemAlloc> landHeightMap;
  if (storeNxzInLtmapTex || useNormalMap)
  {
    SmallTab<float, TmpmemAlloc> landHeightMapSS;
    int ltime0 = dagTools->getTimeMsec();
    int w = (endX - startX) * normalsSS;
    int h = (endY - startY) * normalsSS;
    clear_and_resize(normalsMap, w * h);
    mem_set_ff(normalsMap);
    clear_and_resize(landHeightMapSS, normalsMap.size());
    for (int i = 0; i < normalsMap.size(); ++i)
      landHeightMapSS[i] = MIN_REAL;
    NormalFrameBuffer frameBuffer;
    frameBuffer.ht = frameBuffer.start_ht = landHeightMapSS.data();
    frameBuffer.normal = normalsMap.data();
    float scale = float(normalsSS) / gridCellSize;
    if (!::create_lmesh_normal_map(landMeshMap, frameBuffer,
          startX * gridCellSize + heightMapOffset[0] - (0.5 * normalsSS) / scale, //  -.5f
          startY * gridCellSize + heightMapOffset[1] - (0.5 * normalsSS) / scale, //  -.5f
          scale, (endX - startX) * normalsSS, (endY - startY) * normalsSS))
      clear_and_shrink(normalsMap);
    con.addMessage(ILogWriter::REMARK, "calculated lmesh normals in %g seconds", (dagTools->getTimeMsec() - ltime0) / 1000.0f);
    if (cast_shadows && normalsMap.size() && startX == 0 && endX == lightMapScaled.getMapSizeX())
    {
      clear_and_resize(landHeightMap, (endX - startX) * (endY - startY));

      for (int hi = 0, y = 0; y < (endY - startY); ++y)
        for (int x = 0; x < (endX - startX); ++x, ++hi)
        {
          int mapssi = y * w * normalsSS + x * normalsSS;
          double newHt = 0;
          double origHt = heightMap.getFinalData(x + startX, y + startY);
          for (int j = 0; j < normalsSS; ++j, mapssi += w - normalsSS)
            for (int i = 0; i < normalsSS; ++i, ++mapssi)
            {
              if (normalsMap[mapssi] == 0xFFFF)
                newHt += origHt;
              else
              {
                newHt += landHeightMapSS[mapssi];
              }
            }
          landHeightMap[hi] = newHt / (normalsSS * normalsSS);
        }
    }
    /*
    SmallTab<TexPixel32, TmpmemAlloc> pix;
    pix.resize(normalsMap.size());
    mem_set_ff(pix);
    for (int i = 0; i<normalsMap.size(); ++i)
    {
      if (landHeightMap[i]>MIN_REAL)
      {
        pix[i].a = normalsMap[i] == 0xFFFF?128:0;
        pix[i].r = normalsMap[i]&0xFF;
        pix[i].g = normalsMap[i]>>8;
        pix[i].b = real2uchar((landHeightMap[i]-700.0)/1000.0);
      }
    }
    save_tga32("normals1.tga", pix.data(), w,h, w*4);
    pix.resize((endY-startY)*(endX-startX));

    for (int i=0,y=startY; y<endY; ++y)
      for (int x=startX; x<endX; ++x, ++i)
      {
        unsigned normal = calcNormalAt(float(x)/lightmapScaleFactor, float(y)/lightmapScaleFactor);
        pix[i].r = normal&0xFF;
        pix[i].g = normal>>8;
        float h
          = heightMap.getFinalData(float(x)/lightmapScaleFactor, float(y)/lightmapScaleFactor);
        pix[i].b = real2uchar((h-700.0)/1000.0);
      }

    save_tga32("normals2.tga", pix.data(), endX-startX, endY-startY, (endX-startX)*4);
    //map.clear();
    pix.clear();
    //*/
    // landHeightMap.clear();
  }
#endif


  if (useNormalMap)
    con.setActionDesc("land normal mapping...");
  else
    con.setActionDesc(cast_shadows ? "land lighting..." : "fast land lighting...");
  SimpleProgressCB progressCB;

  con.startProgress(&progressCB);

  con.setTotal(endY - startY);

  for (int y = startY, yi = 0, cnt = 0; y < endY; ++y, ++yi)
  {
    if (progressCB.mustStop)
    {
      con.endLogAndProgress();
      return false;
    }

    if (--cnt <= 0)
      cnt = lightMapScaled.getElemSize();

    for (int xi = 0, x = startX; x < endX; ++x, ++xi)
    {
      unsigned lt = 0;

#if USE_LMESH_ACES
      // fixme:
      // todo:not supported detailed lightmaps now
      G_ASSERT(lightmapScaleFactor == 1);
      Point3 origNormal;
      float sky;
      getNormalAndSky(float(x) / lightmapScaleFactor, float(y) / lightmapScaleFactor, origNormal, sky, landHeightMap.data());
      Point3 newNormal(0, 0, 0);
      if (normalsMap.size())
      {
        // Point3 origNormal
        //  = calcP3NormalAt(float(x)/lightmapScaleFactor, float(y)/lightmapScaleFactor);
        int wd = (endX - startX) * normalsSS;
        int mapssi = (yi * normalsSS) * wd + xi * normalsSS;
        for (int j = 0; j < normalsSS; ++j, mapssi += wd - normalsSS)
          for (int i = 0; i < normalsSS; ++i, ++mapssi)
          {
            if (normalsMap[mapssi] == 0xFFFF)
              newNormal += origNormal;
            else
            {
              Point3 n;
              n.x = (normalsMap[mapssi] & 0xFF) * 2.0f / 255.0f - 1.f;
              n.z = (normalsMap[mapssi] >> 8) * 2.0f / 255.0f - 1.f;
              n.y = sqrtf(1.0f - n.x * n.x - n.z * n.z);
              newNormal += n;
            }
          }
        newNormal.normalize();
      }
      else
        newNormal = origNormal;
      unsigned nx = real2uchar(newNormal.x * 0.5 + 0.5);

      unsigned nz = real2uchar(newNormal.z * 0.5 + 0.5);

      int sunLight = clamp(real2int((newNormal * sunLightDir) * -255), 0, 255);
      if (cast_shadows && sunLight > 0)
      {
        real shadow =
          calcSunShadow(float(x) / lightmapScaleFactor, float(y) / lightmapScaleFactor, sunLightDir, landHeightMap.data()) *
          shadowDensity;
        sunLight *= (1 - shadow);
      }
      int skyLight = clamp(real2int(sky * 255), 0, 255);

      lt = unsigned(sunLight | (skyLight << 8)) | ((nx | (nz << 8)) << 16);
#else

      if (useNormalMap)
        lt = (calcNormalAt(float(x) / lightmapScaleFactor, float(y) / lightmapScaleFactor) << 16) | 0xFFFF;
      else
      {
        lt = calcFastLandLightingAt(float(x) / lightmapScaleFactor, float(y) / lightmapScaleFactor, sunLightDir);

        if (cast_shadows && (lt & 0xFF) != 0)
        {
          real shadow = calcSunShadow(float(x) / lightmapScaleFactor, float(y) / lightmapScaleFactor, sunLightDir) * shadowDensity;
          lt = (lt & ~0xFF) | (real2int((lt & 0xFF) * (1 - shadow)));
          // lt=(lt&~0xFF)|(real2int((1-shadow)*255));
        }
        if (storeNxzInLtmapTex)
          lt |= calcNormalAt(float(x) / lightmapScaleFactor, float(y) / lightmapScaleFactor) << 16;
      }
#endif

      lightMapScaled.setData(x, y, lt);
    }
    if (cnt == lightMapScaled.getElemSize())
    {
      if (!lightMapScaled.flushData())
        con.addMessage(ILogWriter::ERROR, "Error writing data to light map file");

      heightMap.unloadUnchangedData(y + 1);
      lightMapScaled.unloadUnchangedData(y + 1);
    }

    con.setDone(y - startY + 1);
  }
  con.endProgress();

  heightMap.unloadAllUnchangedData();
  lightMapScaled.unloadAllUnchangedData();

  if (!lightMapScaled.flushData())
    con.addMessage(ILogWriter::ERROR, "Error writing data to light map file");

  int genTime = dagTools->getTimeMsec() - time0;
  con.addMessage(ILogWriter::REMARK, "calculated in %g seconds", genTime / 1000.0f);
  if (high_quality)
    blurLightmap(BLUR_LM_KEREL_SIZE, BLUR_LM_SIGMA, calc_box);

  // Set lightmap texture to shader.
  if (hasLightmapTex && !useNormalMap && !skipExportLtmap)
  {
    time0 = dagTools->getTimeMsec();

    String prj;
    DAGORED2->getProjectFolderPath(prj);
    ::dd_mkdir(::make_full_path(prj, "builtScene"));

    dagGeom->shaderGlobalSetTexture(dagGeom->getShaderVariableId("lightmap_tex"), BAD_TEXTUREID);
    exportLightmapToFile(::make_full_path(prj, "builtScene/lightmap.tga"), _MAKE4C('TGA'), high_quality);

    con.addMessage(ILogWriter::NOTE, "exported <builtScene/lightmap.tga> in %.1f sec", (dagTools->getTimeMsec() - time0) / 1000.0f);
  }

  con.endLog();
  return true;
}

void HmapLandPlugin::recalcLightingInRect(const IBBox2 &rect)
{
  if (!hasLightmapTex)
    return;

  applyHmModifiers(false);
  Point3 sunLightDir = calcSunLightDir();

  for (int y = rect[0].y * lightmapScaleFactor; y <= rect[1].y * lightmapScaleFactor; ++y)
    for (int x = rect[0].x * lightmapScaleFactor; x <= rect[1].x * lightmapScaleFactor; ++x)
    {
      unsigned lt = 0;
      if (useNormalMap)
        lt = (calcNormalAt(float(x) / lightmapScaleFactor, float(y) / lightmapScaleFactor) << 16) | 0xFFFF;
      else
      {
        lt = calcFastLandLightingAt(float(x) / lightmapScaleFactor, float(y) / lightmapScaleFactor, sunLightDir);
        if (storeNxzInLtmapTex)
          lt |= calcNormalAt(float(x) / lightmapScaleFactor, float(y) / lightmapScaleFactor) << 16;
      }

      lightMapScaled.setData(x, y, lt);
    }
}

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//
struct HeightmapProvider
{
  float *height;
  int wd, ht;
  Point3 offset;
  float cellSz;
  HeightmapProvider(float *ht, int w, int h, const Point3 &ofs, float cellsz) : height(ht), wd(w), ht(h), cellSz(cellsz), offset(ofs)
  {}
  float get_ht(int x, int y) const { return height[x + y * wd]; };
  int getHeightmapSizeX() const { return wd; }
  int getHeightmapSizeY() const { return ht; }
  float getHeightmapCellSize() const { return cellSz; }
  Point3 getHeightmapOffset() const { return offset; }
  bool getHeightmapHeight(const IPoint2 &cell, real &h) const
  {
    if (cell.x < 0 || cell.y < 0 || cell.x >= wd || cell.y >= ht)
      return false;

    h = height[cell.x + cell.y * wd];
    return true;
  }
  bool getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid) const
  {
    if (cell.x < 0 || cell.y < 0 || cell.x + 1 >= wd || cell.y + 1 >= ht)
      return false;

    h0 = get_ht(cell.x, cell.y);
    hx = get_ht(cell.x + 1, cell.y);
    hy = get_ht(cell.x, cell.y + 1);
    hxy = get_ht(cell.x + 1, cell.y + 1);

    hmid = (h0 + hx + hy + hxy) * 0.25f;

    return true;
  }
};

real HmapLandPlugin::calcSunShadow(float ox, float oy, const Point3 &sun_light_dir, float *height)
{
  float ht[3][3], baseht[4][4];

  int numHit = 0;
  Point2 frac, cell;
  int x, y;
  frac.x = modff(ox, &cell.x);
  frac.y = modff(oy, &cell.y);
  x = cell.x;
  y = cell.y;
  if (height)
  {
    int mapSizeX = heightMap.getMapSizeX();
    int mapSizeY = heightMap.getMapSizeY();
    int stride = mapSizeX;
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
        baseht[i][j] = height[clamp(y + i - 1, 0, mapSizeY - 1) * stride + clamp(x + j - 1, 0, mapSizeX - 1)];
  }
  else
  {
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j)
        baseht[i][j] = heightMap.getFinalData(x + j - 1, y + i - 1);
  }

  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
    {
      ht[i][j] = lerp(lerp(baseht[i][j], baseht[i][j + 1], frac.x), lerp(baseht[i + 1][j], baseht[i + 1][j + 1], frac.x), frac.y);
    }
  Point3 stpt((ox + 0.5f / lightmapScaleFactor) * gridCellSize + heightMapOffset.x, ht[1][1] + shadowBias,
    (oy + 0.5f / lightmapScaleFactor) * gridCellSize + heightMapOffset.y);

  float httest;
  if (height && getHeightmapHeight(IPoint2(0, 0), httest))
  {
    HeightmapProvider hmp(height, heightMap.getMapSizeX(), heightMap.getMapSizeY(), getHeightmapOffset(), getHeightmapCellSize());
    if (ray_hit_midpoint_heightmap_approximate(hmp, stpt, -sun_light_dir, shadowTraceDist))
      return 1.0;
  }
  else if (ray_hit_midpoint_heightmap_approximate(*this, stpt, -sun_light_dir, shadowTraceDist))
    return 1.0;

  ht[1][1] *= 0.75f / 2;
  ht[1][0] *= 0.5f / 2;
  ht[0][1] *= 0.5f / 2;
  ht[2][1] *= 0.5f / 2;
  ht[1][2] *= 0.5f / 2;
  ht[0][0] *= 0.25f / 2;
  ht[2][2] *= 0.25f / 2;
  ht[0][2] *= 0.25f / 2;
  ht[2][0] *= 0.25f / 2;
  calculating_shadows = true;
  for (int dy = 0; dy < 2; ++dy)
    for (int dx = 0; dx < 2; ++dx)
    {
      real h = (ht[dy][dx] + ht[dy][dx + 1] + ht[dy + 1][dx] + ht[dy + 1][dx + 1]); //*0.25f;

      Point3 pt((ox + (0.25f + dx * 0.5) / lightmapScaleFactor) * gridCellSize + heightMapOffset.x, h + shadowBias,
        (oy + (0.25f + dy * 0.5) / lightmapScaleFactor) * gridCellSize + heightMapOffset.y);

      real t = shadowTraceDist;
      if (DAGORED2->shadowRayHitTest(pt, -sun_light_dir, t))
        numHit++;
    }
  calculating_shadows = false;

  return numHit / 4.0f;
}


bool HmapLandPlugin::getDirectionSunSettings()
{
  ISceneLightService *ltService = DAGORED2->queryEditorInterface<ISceneLightService>();
  debug("get dir");
  if (ltService)
  {
    sunAzimuth = HALFPI - ltService->getSunEx(0).azimuth;
    sunZenith = ltService->getSunEx(0).zenith;

    if (DAGORED2->curPlugin() == this)
      propPanel->updateLightGroup();

    updateRendererLighting();
  }
  else
    return false;

  return true;
}

bool HmapLandPlugin::getAllSunSettings()
{
  ISceneLightService *ltService = DAGORED2->queryEditorInterface<ISceneLightService>();

  if (ltService)
  {
    Point3 sun1_dir, sun2_dir;
    Color3 sun1_col, sun2_col, sky_col;
    ltService->getDirectLight(sun1_dir, sun1_col, sun2_dir, sun2_col, sky_col);

    ldrLight.skyPower = ldrLight.sunPower = 1.0;
    ldrLight.skyColor = dagRender->normalizeColor4(::color4(sky_col, 1), ldrLight.skyPower);
    ldrLight.sunColor = dagRender->normalizeColor4(::color4(sun1_col, 1), ldrLight.sunPower);
    ldrLight.specularColor = ldrLight.sunColor;
    sunAzimuth = HALFPI - ltService->getSunEx(0).azimuth;
    sunZenith = ltService->getSunEx(0).zenith;

    if (DAGORED2->curPlugin() == this)
      propPanel->updateLightGroup();

    updateRendererLighting();
  }
  else
    return false;

  return true;
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

template <class T>
bool flushDataTo(const char *base_path, const char *orig_base_path, T &map)
{
  int base_path_slen = i_strlen(base_path);
  if (strcmp(base_path, orig_base_path) == 0)
  {
    if (strncmp(map.getFileName(), base_path, base_path_slen) != 0)
      DAEDITOR3.conError("Internal error: map=%s, base_path=%s", map.getFileName(), base_path);
    return map.flushData();
  }

  debug("SaveAs: flushDataTo(%s, %s, %p(%s))", base_path, orig_base_path, &map, map.getFileName());

  int orig_base_path_slen = i_strlen(orig_base_path);
  if (strncmp(map.getFileName(), orig_base_path, orig_base_path_slen) != 0)
  {
    DAEDITOR3.conError("Internal error: map=%s, orig_base_path=%s", map.getFileName(), orig_base_path);
    return false;
  }

  String orig_fn(map.getFileName());
  if (orig_fn.suffix("/") || orig_fn.suffix("\\"))
    erase_items(orig_fn, orig_fn.size() - 2, 1);
  String orig_backup_fn(0, "%s.orig#", orig_fn);
  String new_fn(0, "%s%s", base_path, orig_fn + orig_base_path_slen);
  Tab<SimpleString> flist;

  // clear dest
  find_files_in_folder(flist, new_fn, "", false, true, false);
  for (int i = 0; i < flist.size(); i++)
    remove(flist[i]);

  // copy last to backup
  dd_mkdir(orig_backup_fn);
  flist.clear();
  find_files_in_folder(flist, orig_fn, "", false, true, false);
  for (int i = 0; i < flist.size(); i++)
  {
    String dest(0, "%s%s", orig_backup_fn, flist[i].str() + orig_fn.length());
    dagTools->copyFile(flist[i], dest);
  }

  // flush changes
  dd_mkdir(orig_fn);
  bool res = map.flushData();
  map.closeFile(true);

  // move final changes to dest
  dd_mkdir(new_fn);
  flist.clear();
  find_files_in_folder(flist, orig_fn, "", false, true, false);
  for (int i = 0; i < flist.size(); i++)
  {
    String dest(0, "%s%s", new_fn, flist[i].str() + orig_fn.length());
    rename(flist[i], dest);
  }

  // move backup copy back
  flist.clear();
  find_files_in_folder(flist, orig_backup_fn, "", false, true, false);
  for (int i = 0; i < flist.size(); i++)
  {
    String dest(0, "%s%s", orig_fn, flist[i].str() + orig_backup_fn.length());
    rename(flist[i], dest);
  }
  dd_rmdir(orig_backup_fn);

  // reopen data in new place
  map.openFile(new_fn + ".dat");
  return res;
}

template <class T>
void HmapLandPlugin::loadMapFile(MapStorage<T> &map, const char *filename, CoolConsole &con)
{
  String fileName(DAGORED2->getPluginFilePath(this, filename));

  if (HmapLandPlugin::hmlService->mapStorageFileExists(fileName))
  {
    if (!map.openFile(fileName))
      con.addMessage(ILogWriter::ERROR, "Error loading %s file", filename);
    else
      con.addMessage(ILogWriter::NOTE, "Loaded %dx%d %s", map.getMapSizeX(), map.getMapSizeY(), filename);
  }
}

static MapStorage<uint64_t> *loadMap64File(const char *filename, CoolConsole &con)
{
  String fileName(DAGORED2->getPluginFilePath(HmapLandPlugin::self, filename));

  if (HmapLandPlugin::hmlService->mapStorageFileExists(fileName))
  {
    MapStorage<uint64_t> *map = HmapLandPlugin::hmlService->createUint64MapStorage(512, 512, 0);

    if (!map->openFile(fileName))
    {
      con.addMessage(ILogWriter::ERROR, "Error loading %s file", filename);
      delete map;
      return NULL;
    }
    else
      con.addMessage(ILogWriter::NOTE, "Loaded %dx%d %s", map->getMapSizeX(), map->getMapSizeY(), filename);
    return map;
  }
  return NULL;
}
void HmapLandPlugin::prepareDetTexMaps()
{
  String fileName;
  int w = heightMap.getMapSizeX(), h = heightMap.getMapSizeY();

  if (!detTexIdxMap)
  {
    fileName = DAGORED2->getPluginFilePath(this, DETTEXMAP_FILENAME);
    detTexIdxMap = hmlService->createUint64MapStorage(w, h, 0);
    detTexIdxMap->createFile(fileName);
  }
  if (!detTexWtMap)
  {
    fileName = DAGORED2->getPluginFilePath(this, DETTEXWMAP_FILENAME);
    detTexWtMap = hmlService->createUint64MapStorage(w, h, 0xFF);
    detTexWtMap->createFile(fileName);
  }
}

void HmapLandPlugin::loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path)
{
#define LD_LOCAL_VAR(X, TYPE) X = local_data.get##TYPE(#X, X)
  EditLayerProps::resetLayersToDefauls();
  if (blk.getBlockByName("layers"))
    EditLayerProps::loadLayersConfig(blk, local_data);

  origBasePath = base_path;
  if (d3d::is_stub_driver())
  {
    // avoid unpredictable caches in DE3xq
    DAEDITOR3.conNote("removing delaunayGen.cache.bin");
    dd_erase(DAGORED2->getPluginFilePath(HmapLandPlugin::self, "delaunayGen.cache.bin"));
  }

  ignoreEvents = true;

  lastHmapImportPath = local_data.getStr("lastHmapImportPath", lastHmapImportPath);
  LD_LOCAL_VAR(lastHmapImportPathMain, Str);
  LD_LOCAL_VAR(lastChangeMain.size, Int64);
  LD_LOCAL_VAR(lastChangeMain.mtime, Int64);
  LD_LOCAL_VAR(lastHmapImportPathDet, Str);
  LD_LOCAL_VAR(lastChangeDet.size, Int64);
  LD_LOCAL_VAR(lastChangeDet.mtime, Int64);
  lastWaterHeightmapImportPath = local_data.getStr("lastHmapImportPathWater", lastWaterHeightmapImportPath);
  LD_LOCAL_VAR(lastWaterHeightmapImportPathDet, Str);
  LD_LOCAL_VAR(lastChangeWaterDet.size, Int64);
  LD_LOCAL_VAR(lastChangeWaterDet.mtime, Int64);
  LD_LOCAL_VAR(lastWaterHeightmapImportPathMain, Str);
  LD_LOCAL_VAR(lastChangeWaterMain.size, Int64);
  LD_LOCAL_VAR(lastChangeWaterMain.mtime, Int64);

  lastLandExportPath = local_data.getStr("lastLandExportPath", lastLandExportPath);
  lastHmapExportPath = local_data.getStr("lastHmapExportPath", lastHmapExportPath);
  lastColormapExportPath = local_data.getStr("lastColormapExportPath", lastColormapExportPath);
  lastTexImportPath = local_data.getStr("lastTexImportPath", lastTexImportPath);
  objEd.autoUpdateSpline = local_data.getBool("autoUpdateSpline", true);
  objEd.maxPointVisDist = local_data.getReal("maxPointVisDist", 5000.0);

  lastExpLoftFolder = local_data.getStr("lastExpLoftFolder", "");
  lastExpLoftMain = blk.getBool("lastExpLoftMain", local_data.getBool("lastExpLoftMain", true));
  lastExpLoftDet = blk.getBool("lastExpLoftDet", local_data.getBool("lastExpLoftDet", true));
  lastExpLoftMainSz = blk.getInt("lastExpLoftMainSz", local_data.getInt("lastExpLoftMainSz", 4096));
  lastExpLoftDetSz = blk.getInt("lastExpLoftDetSz", local_data.getInt("lastExpLoftDetSz", 4096));
  lastExpLoftUseRect[0] = blk.getBool("lastExpLoftUseRectM", local_data.getBool("lastExpLoftUseRectM", false));
  lastExpLoftRect[0][0] = blk.getPoint2("lastExpLoftRectM0", local_data.getPoint2("lastExpLoftRectM0", Point2(0, 0)));
  lastExpLoftRect[0][1] = blk.getPoint2("lastExpLoftRectM1", local_data.getPoint2("lastExpLoftRectM1", Point2(1024, 1024)));
  lastExpLoftUseRect[1] = blk.getBool("lastExpLoftUseRectD", local_data.getBool("lastExpLoftUseRectD", false));
  lastExpLoftRect[1][0] = blk.getPoint2("lastExpLoftRectD0", local_data.getPoint2("lastExpLoftRectD0", Point2(0, 0)));
  lastExpLoftRect[1][1] = blk.getPoint2("lastExpLoftRectD1", local_data.getPoint2("lastExpLoftRectD1", Point2(1024, 1024)));
  lastExpLoftCreateAreaSubfolders =
    blk.getBool("lastExpLoftCreateAreaSubfolders", local_data.getBool("lastExpLoftCreateAreaSubfolders", true));

  SimpleString exportTypeStr(blk.getStr("hmapExportType", useMeshSurface ? "lmesh" : "hmap"));
  if (stricmp(exportTypeStr, "hmap") == 0)
    exportType = EXPORT_HMAP;
  else if (stricmp(exportTypeStr, "lmesh") == 0)
    exportType = EXPORT_LANDMESH;
  else if (stricmp(exportTypeStr, "plane") == 0)
    exportType = EXPORT_PSEUDO_PLANE;
  genHmap->altCollider = (exportType == EXPORT_LANDMESH) ? this : NULL;

  meshPreviewDistance = blk.getReal("meshPreviewDistance", meshPreviewDistance);
  meshCells = blk.getInt("meshCells", meshCells);
  meshErrorThreshold = blk.getReal("meshErrorThreshold", meshErrorThreshold);
  numMeshVertices = blk.getInt("numMeshVertices", numMeshVertices);

  lod1TrisDensity = blk.getInt("lod1TrisDensity", -1);
  if (lod1TrisDensity < 0)
  {
    DataBlock appblk(String(260, "%s/application.blk", DAGORED2->getWorkspace().getAppDir()));
    lod1TrisDensity = appblk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("hmap")->getInt("lod1TrisDensity", 30);
  }

  if (lod1TrisDensity <= 0)
    lod1TrisDensity = 1;
  if (lod1TrisDensity > 100)
    lod1TrisDensity = 100;

  importanceMaskScale = blk.getReal("importanceMaskScale", importanceMaskScale);
  geomLoftBelowAll = blk.getBool("geomLoftBelowAll", false);

  hasWaterSurface = blk.getBool("hasWaterSurface", false);
  waterMatAsset = blk.getStr("waterMatAsset", NULL);
  waterSurfaceLevel = blk.getReal("waterSurfaceLevel", 0);
  minUnderwaterBottomDepth = blk.getReal("minUnderwaterBottomDepth", 2);
  hasWorldOcean = blk.getBool("hasWorldOcean", true);
  worldOceanExpand = blk.getReal("worldOceanExpand", 100);
  worldOceanShorelineTolerance = blk.getReal("worldOceanShorelineTolerance", 0.3);
  waterMask.resize(0, 0);
  waterMaskScale = 1;

  lastMinHeight[0] = blk.getReal("lastMinHeight", MAX_REAL);
  lastHeightRange[0] = blk.getReal("lastHeightRange", MAX_REAL);
  lastMinHeight[1] = blk.getReal("lastMinHeightDet", MAX_REAL);
  lastHeightRange[1] = blk.getReal("lastHeightRangeDet", MAX_REAL);

  waterHeightMinRangeDet = blk.getPoint2("waterHeightMinRangeDet", Point2(0, 0));
  waterHeightMinRangeMain = blk.getPoint2("waterHeightMinRangeMain", Point2(0, 0));

  colorGenScriptFilename = blk.getStr("colorGenScriptFilename", "");

  if (!colorGenScriptFilename.empty())
    colorGenScriptFilename = ::find_in_base_smart(colorGenScriptFilename, DAGORED2->getSdkDir(), colorGenScriptFilename);

  doAutocenter = blk.getBool("doAutocenter", doAutocenter);

  gridCellSize = blk.getReal("gridCellSize", gridCellSize);

  heightMap.heightScale = blk.getReal("heightScale", heightMap.heightScale);
  heightMap.heightOffset = blk.getReal("heightOffset", heightMap.heightOffset);

  heightMapOffset = blk.getPoint2("heightMapOffset", heightMapOffset);


  detDivisor = blk.getInt("detDivisor", 0);
  detRect[0] = blk.getPoint2("detRect0", Point2(0, 0));
  detRect[1] = blk.getPoint2("detRect1", Point2(0, 0));
  detRectC[0].set_xy((detRect[0] - heightMapOffset) / gridCellSize);
  detRectC[0] *= detDivisor;
  detRectC[1].set_xy((detRect[1] - heightMapOffset) / gridCellSize);
  detRectC[1] *= detDivisor;

  collisionArea.ofs = blk.getPoint2("collisionArea_ofs", collisionArea.ofs);
  collisionArea.sz = blk.getPoint2("collisionArea_sz", collisionArea.sz);
  collisionArea.show = blk.getBool("collisionArea_show", collisionArea.show);

  shadowBias = blk.getReal("shadowBias", shadowBias);
  shadowTraceDist = blk.getReal("shadowTraceDist", shadowTraceDist);
  shadowDensity = blk.getReal("shadowDensity", shadowDensity);

  tileXSize = blk.getReal("tileXSize", tileXSize);
  tileYSize = blk.getReal("tileYSize", tileYSize);
  tileTexName = blk.getStr("tileTexName", NULL);

  useVertTex = blk.getBool("useVertTex", useVertTex);
  useVertTexForHMAP = blk.getBool("useVertTexForHMAP", useVertTexForHMAP);
  vertTexName = blk.getStr("vertTexName", NULL);
  vertNmTexName = blk.getStr("vertNmTexName", NULL);
  if (!vertTexName.empty() && vertNmTexName.empty())
  {
    vertNmTexName = String(128, "%s_nm", vertTexName.str());
    if (!DAEDITOR3.getAssetByName(vertNmTexName, DAEDITOR3.getAssetTypeId("tex")))
      vertNmTexName = NULL;
  }

  vertDetTexName = blk.getStr("vertDetTexName", NULL);

  vertTexXZtile = blk.getReal("vertTexXZtile", vertTexXZtile);
  vertTexYtile = blk.getReal("vertTexYtile", vertTexYtile);
  vertTexYOffset = blk.getReal("vertTexYOffset", vertTexYOffset);
  vertTexAng0 = blk.getReal("vertTexAng0", vertTexAng0);
  vertTexAng1 = blk.getReal("vertTexAng1", vertTexAng1);
  vertTexHorBlend = blk.getReal("vertTexHorBlend", vertTexHorBlend);
  vertDetTexXZtile = blk.getReal("vertDetTexXZtile", vertDetTexXZtile);
  vertDetTexYtile = blk.getReal("vertDetTexYtile", vertDetTexYtile);
  vertDetTexYOffset = blk.getReal("vertDetTexYOffset", vertDetTexYOffset);

  const DataBlock *brushBlk;

  brushBlk = local_data.getBlockByName("smoothBrush");

  if (brushBlk)
    brushes[SMOOTH_BRUSH]->loadFromBlk(*brushBlk);

  brushBlk = local_data.getBlockByName("alignBrush");

  if (brushBlk)
    brushes[ALIGN_BRUSH]->loadFromBlk(*brushBlk);

  brushBlk = local_data.getBlockByName("shadowsBrush");

  if (brushBlk)
    brushes[SHADOWS_BRUSH]->loadFromBlk(*brushBlk);

  brushBlk = local_data.getBlockByName("scriptBrush");

  if (brushBlk)
    brushes[SCRIPT_BRUSH]->loadFromBlk(*brushBlk);

  brushBlk = local_data.getBlockByName("hillUpBrush");

  if (brushBlk)
    brushes[HILLUP_BRUSH]->loadFromBlk(*brushBlk);

  brushBlk = local_data.getBlockByName("hillDownBrush");

  if (brushBlk)
    brushes[HILLDOWN_BRUSH]->loadFromBlk(*brushBlk);

  const DataBlock &renderBlk = *blk.getBlockByNameEx("render");

  render.gridStep = renderBlk.getInt("gridStep", render.gridStep);
  render.elemSize = renderBlk.getInt("elemSize", render.elemSize);
  render.radiusElems = renderBlk.getInt("radiusElems", render.radiusElems);
  render.ringElems = renderBlk.getInt("ringElems", render.ringElems);
  render.numLods = renderBlk.getInt("numLods", render.numLods);
  render.maxDetailLod = renderBlk.getInt("maxDetailLod", render.maxDetailLod);
  render.detailTile = renderBlk.getReal("detailTile", render.detailTile);
  render.canyonHorTile = renderBlk.getReal("canyonHorTile", render.canyonHorTile);
  render.canyonVertTile = renderBlk.getReal("canyonVertTile", render.canyonVertTile);
  render.canyonAngle = renderBlk.getReal("canyonAngle", render.canyonAngle);
  render.canyonFadeAngle = renderBlk.getReal("canyonFadeAngle", render.canyonFadeAngle);
  render.showFinalHM = renderBlk.getInt("showFinalHM", render.showFinalHM);
  monochromeLandCol = renderBlk.getE3dcolor("monoLand", monochromeLandCol);

  LD_LOCAL_VAR(showMonochromeLand, Bool);

  LD_LOCAL_VAR(showHtLevelCurves, Bool);
  LD_LOCAL_VAR(htLevelCurveStep, Real);
  LD_LOCAL_VAR(htLevelCurveThickness, Real);
  LD_LOCAL_VAR(htLevelCurveOfs, Real);
  LD_LOCAL_VAR(htLevelCurveDarkness, Real);
  LD_LOCAL_VAR(render.hm2YbaseForLod, Real);
  render.hm2displacementQ = clamp(local_data.getInt("hm2displacementQ", 1), 0, 5);
#undef LD_LOCAL_VAR

  ldrLight.load("", blk);

  sunAzimuth = blk.getReal("sunAzimuth", sunAzimuth);
  sunZenith = blk.getReal("sunZenith", sunZenith);

  lcmScale = blk.getInt("landClsMapScaleFactor", 1);
  lightmapScaleFactor = blk.getInt("lightmapScaleFactor", 1);
  syncLight = blk.getBool("syncLight", true);
  syncDirLight = blk.getBool("syncDirLight", false);

  colorGenParamsData->setFrom(blk.getBlockByNameEx("colorGenParams"));

  const DataBlock *disShadows = blk.getBlockByName("disabled_shadows");

  if (disShadows)
  {
    const int colCnt = DAGORED2->getCustomCollidersCount();

    for (int ci = 0; ci < colCnt; ++ci)
    {
      const IDagorEdCustomCollider *collider = DAGORED2->getCustomCollider(ci);
      if (collider)
      {
        bool doEnable = true;
        const char *shadowName = collider->getColliderName();

        for (int i = 0; i < disShadows->paramCount(); ++i)
        {
          const char *disName = disShadows->getStr(i);

          if (disName && !::stricmp(shadowName, disName))
          {
            doEnable = false;
            break;
          }
        }

        if (doEnable)
          DAGORED2->enableCustomShadow(shadowName);
        else
          DAGORED2->disableCustomShadow(shadowName);
      }
    }
  }

  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  showBlueWhiteMask = blk.getBool("showBlueWhiteMask", true);

  loadMapFile(heightMap.getInitialMap(), HEIGHTMAP_FILENAME, con);
  heightMap.resetFinal();

  if (detDivisor)
  {
    loadMapFile(heightMapDet.getInitialMap(), "det-" HEIGHTMAP_FILENAME, con);
    heightMapDet.resetFinal();

    int dw = heightMap.getMapSizeX() * detDivisor, dh = heightMap.getMapSizeY() * detDivisor;

    if (!heightMapDet.isFileOpened() || heightMapDet.getMapSizeX() != dw || heightMapDet.getMapSizeY() != dh)
      resizeHeightMapDet(con);
  }

  loadMapFile(landClsMap, LANDCLSMAP_FILENAME, con);
  resizeLandClassMapFile(con);

  detTexIdxMap = ::loadMap64File(DETTEXMAP_FILENAME, con);
  detTexWtMap = ::loadMap64File(DETTEXWMAP_FILENAME, con);

  if (hasColorTex)
    loadMapFile(colorMap, COLORMAP_FILENAME, con);
  if (hasLightmapTex && exportType != EXPORT_PSEUDO_PLANE)
  {
    loadMapFile(lightMapScaled, LIGHTMAP_FILENAME, con);
    if (!lightMapScaled.isFileOpened() && heightMap.isFileOpened())
    {
      String fname(DAGORED2->getPluginFilePath(this, LIGHTMAP_FILENAME));
      bool ltmapfile_exists = ::dd_file_exist(fname);

      lightMapScaled.reset(heightMap.getMapSizeX() * lightmapScaleFactor, heightMap.getMapSizeY() * lightmapScaleFactor, 0xFFFF);
      createMapFile(lightMapScaled, LIGHTMAP_FILENAME, con);
      if (ltmapfile_exists)
      {
        DAEDITOR3.conWarning("Obsolete/invalid lightmap.dat detected, starting Rebuild lighting");
        calcFastLandLighting();
      }
    }
  }

  IWaterService *waterSrv = EDITORCORE->queryEditorInterface<IWaterService>();
  if (waterSrv)
  {
    loadMapFile(waterHeightmapDet.getInitialMap(), WATER_HEIGHTMAP_DET_FILENAME, con);
    loadMapFile(waterHeightmapMain.getInitialMap(), WATER_HEIGHTMAP_MAIN_FILENAME, con);
    if (waterHeightmapDet.isFileOpened() || waterHeightmapMain.isFileOpened())
    {
      Point2 hmapSize(heightMap.getMapSizeX() * gridCellSize, heightMap.getMapSizeY() * gridCellSize);
      Point2 hmapEnd = heightMapOffset + hmapSize;
      waterSrv->setHeightmap(waterHeightmapDet, waterHeightmapMain, waterHeightMinRangeDet, waterHeightMinRangeMain, detRect,
        BBox2(heightMapOffset.x, heightMapOffset.y, hmapEnd.x, hmapEnd.y));
    }
  }

  autocenterHeightmap(NULL, false);

  genHmap->offset = heightMapOffset;
  genHmap->cellSize = gridCellSize;

  loadGenLayers(*blk.getBlockByNameEx("genLayers"));
  getColorGenVarsFromScript(); // Before landmesh to load importance mask.

  const DataBlock *grassBlock = blk.getBlockByNameEx("grassLayers", NULL);
  if (grassBlock)
    loadGrassLayers(*grassBlock);

  const DataBlock *rendinstDetail2ColorBlock = blk.getBlockByNameEx("rendinstDetail2Color", NULL);
  if (rendinstDetail2ColorBlock)
    loadRendinstDetail2Color(*rendinstDetail2ColorBlock);

  const DataBlock *landModulateColorTexBlock = blk.getBlockByNameEx("landModulateColorTex", NULL);
  if (landModulateColorTexBlock)
    loadLandModulateColorTex(*landModulateColorTexBlock);

  const DataBlock *horizontalTexBlock = blk.getBlockByNameEx("horizontalTex", NULL);
  if (horizontalTexBlock)
    loadHorizontalTex(*horizontalTexBlock);


  objEd.lastExportPath = local_data.getStr("lastExportPath", objEd.lastExportPath);
  objEd.lastImportPath = local_data.getStr("lastImportPath", objEd.lastImportPath);
  objEd.lastExportPath = ::make_full_path(DAGORED2->getSdkDir(), objEd.lastExportPath);
  objEd.lastImportPath = ::make_full_path(DAGORED2->getSdkDir(), objEd.lastImportPath);

  const DataBlock &loadBlk = *blk.getBlockByNameEx("objects");
  objEd.load(loadBlk);

  String blkPath(128, "%s/splines.blk", base_path);
  DataBlock splinesBlk(blkPath);
  blkPath = String(128, "%s/polys.blk", base_path);
  DataBlock polysBlk(blkPath);
  blkPath = String(128, "%s/entities.blk", base_path);
  DataBlock entBlk(blkPath);
  blkPath = String(128, "%s/lights.blk", base_path);
  DataBlock ltBlk;
  if (dd_file_exist(blkPath))
    ltBlk.load(blkPath);

  objEd.load(splinesBlk, polysBlk, entBlk, ltBlk, -1);
  if (DAGORED2->curPlugin() == this)
    objEd.updateToolbarButtons();

  for (int i = 0; i < EditLayerProps::layerProps.size(); i++)
  {
    if (EditLayerProps::layerProps[i].nameId == 0)
      continue;

    const char *lname = EditLayerProps::layerProps[i].name();
    switch (EditLayerProps::layerProps[i].type)
    {
      case EditLayerProps::SPL:
        splinesBlk.load(String(128, "%s/splines.%s.blk", base_path, lname));
        objEd.load(splinesBlk, DataBlock::emptyBlock, DataBlock::emptyBlock, DataBlock::emptyBlock, i);
        break;
      case EditLayerProps::PLG:
        polysBlk.load(String(128, "%s/polys.%s.blk", base_path, lname));
        objEd.load(DataBlock::emptyBlock, polysBlk, DataBlock::emptyBlock, DataBlock::emptyBlock, i);
        break;
      case EditLayerProps::ENT:
        entBlk.load(String(128, "%s/entities.%s.blk", base_path, lname));
        objEd.load(DataBlock::emptyBlock, DataBlock::emptyBlock, entBlk, DataBlock::emptyBlock, i);
        break;
    }
  }

  con.endLog();

  updateHmap2Tesselation();
  resetRenderer(true);
  onLandSizeChanged();

  const DataBlock *panelStateBlk = local_data.getBlockByName("panel_state");
  if (panelStateBlk)
    mainPanelState.setFrom(panelStateBlk);

  snowDynPreview = blk.getBool("snow_dyn_prewiew", false);
  snowSpherePreview = blk.getBool("snow_sphere_prewiew", false);
  ambSnowValue = blk.getReal("snow_amb_value", 0);
  dynSnowValue = ambSnowValue;

  dagGeom->shaderGlobalSetInt(snowPrevievSVId, (snowDynPreview && snowSpherePreview) ? 2 : (snowDynPreview ? 1 : 0));
  dagGeom->shaderGlobalSetReal(snowValSVId, dynSnowValue);

  if (snowSpherePreview)
    updateSnowSources();

  acquireVertTexRef();

  for (int i = 0; i < MAX_NAVMESHES; ++i)
    navMeshProps[i] = *blk.getBlockByNameEx(i == 0 ? String("navMesh") : String(50, "navMesh%d", i + 1));

  shownExportedNavMeshIdx = local_data.getInt("shownExportedNavMeshIdx", 0);
  showExportedNavMesh = local_data.getBool("showExportedNavMesh", true);
  showExportedCovers = local_data.getBool("showExportedCovers", true);
  showExportedNavMeshContours = local_data.getBool("showExportedNavMeshContours", true);
  showExpotedObstacles = local_data.getBool("showExpotedObstacles", true);
  disableZtestForDebugNavMesh = local_data.getBool("disableZtestForDebugNavMesh", false);

  if (lcmScale > 1)
    prepareDetTexMaps();
}


void HmapLandPlugin::beforeMainLoop()
{
  ignoreEvents = false;
  applyHmModifiers(false, true, false);
  updateHeightMapTex(false);
  updateHeightMapTex(true);

  if (useMeshSurface && exportType != EXPORT_PSEUDO_PLANE && landMeshMap.isEmpty()) // Before entities to place them correctly.
  {
    rebuildHtConstraintBitmask();
    generateLandMeshMap(landMeshMap, DAGORED2->getConsole(), false, NULL);
  }

  if (syncLight)
    getAllSunSettings();
  else if (syncDirLight)
    getDirectionSunSettings();

  resetRenderer(true);

  lastLandExportPath = DAGORED2->getWorkspace().getLevelsBinDir();
  rebuildSplinesBitmask(false);
  onLandRegionChanged(0, 0, landClsMap.getMapSizeX(), landClsMap.getMapSizeY(), true);
  updateHtLevelCurves();

  // gizmoTranformMode was set during loading in LandscapeEntityObject::load() (and LandscapeEntityObject::propsChanged(true))
  for (int i = 0; i < objEd.objectCount(); i++)
    if (LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(objEd.getObject(i)))
      o->setGizmoTranformMode(false);
}


void HmapLandPlugin::restoreBackup()
{
  heightMap.closeFile();
  heightMapDet.closeFile();

  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();
  con.addMessage(ILogWriter::NOTE, "restoring heightmap from backup...");

  DAGOR_TRY
  {
    heightMap.getInitialMap().revertChanges();
    if (detDivisor)
      heightMapDet.getInitialMap().revertChanges();
  }
  DAGOR_CATCH(DagorException e)
  {
    con.addMessage(ILogWriter::ERROR, "Error restoring heightmap backup: '%s'", e.excDesc);
    con.endLog();

    return;
  }

  objEd.updateToolbarButtons();

  loadMapFile(heightMap.getInitialMap(), HEIGHTMAP_FILENAME, con);
  if (detDivisor)
    loadMapFile(heightMapDet.getInitialMap(), "det-" HEIGHTMAP_FILENAME, con);
  heightMap.resetFinal();
  if (detDivisor)
    heightMapDet.resetFinal();
  applyHmModifiers(false);
  resetRenderer();
  updateHeightMapTex(false);
  updateHeightMapTex(true);

  con.addMessage(ILogWriter::NOTE, "Heightmap backup restored");
  con.endLog();
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


template <class T>
void HmapLandPlugin::createMapFile(MapStorage<T> &map, const char *filename, CoolConsole &con)
{
  con.startProgress();
  con.setActionDesc("Creating %s file...", filename);

  String fileName(DAGORED2->getPluginFilePath(this, filename));
  if (!map.createFile(fileName))
    con.addMessage(ILogWriter::ERROR, "Error creating %s file", filename);
  else
    con.addMessage(ILogWriter::NOTE, "Created %dx%d %s file", map.getMapSizeX(), map.getMapSizeY(), filename);

  con.endProgress();
}

void HmapLandPlugin::createHeightmapFile(CoolConsole &con)
{
  createMapFile(heightMap.getInitialMap(), HEIGHTMAP_FILENAME, con);
  heightMap.resetFinal();
  // reset water mask when heightmap size is changed
  if (waterMask.getW() / waterMaskScale != getHeightmapSizeX() || waterMask.getH() / waterMaskScale != getHeightmapSizeY())
  {
    waterMask.resize(0, 0);
    waterMaskScale = 1;
  }
  applyHmModifiers(false);
}

void HmapLandPlugin::createWaterHmapFile(CoolConsole &con, bool det)
{
  HeightMapStorage &hms = det ? waterHeightmapDet : waterHeightmapMain;
  createMapFile(hms.getInitialMap(), det ? WATER_HEIGHTMAP_DET_FILENAME : WATER_HEIGHTMAP_MAIN_FILENAME, con);
}

void HmapLandPlugin::resizeHeightMapDet(CoolConsole &con)
{
  int dw = heightMap.getMapSizeX() * detDivisor, dh = heightMap.getMapSizeY() * detDivisor;
  heightMapDet.getInitialMap().reset(dw, dh, 0);
  heightMapDet.resetFinal();
  createMapFile(heightMapDet.getInitialMap(), "det-" HEIGHTMAP_FILENAME, con);
  heightMapDet.flushData();
}

void HmapLandPlugin::createColormapFile(CoolConsole &con)
{
  createMapFile(landClsMap, LANDCLSMAP_FILENAME, con);
  if (hasColorTex)
    createMapFile(colorMap, COLORMAP_FILENAME, con);
}


void HmapLandPlugin::createLightmapFile(CoolConsole &con)
{
  if (heightMap.isFileOpened())
    createMapFile(lightMapScaled, LIGHTMAP_FILENAME, con);
}
void HmapLandPlugin::resizeLandClassMapFile(CoolConsole &con)
{
  if (heightMap.getMapSizeX() * lcmScale == landClsMap.getMapSizeX() && heightMap.getMapSizeY() * lcmScale == landClsMap.getMapSizeY())
    return;

  if (landClsMap.isFileOpened())
    landClsMap.eraseFile();
  landClsMap.reset(heightMap.getMapSizeX() * lcmScale, heightMap.getMapSizeY() * lcmScale, 0);
  createMapFile(landClsMap, LANDCLSMAP_FILENAME, con);
  landClsMap.flushData();
}

void HmapLandPlugin::updateHeightMapConstants()
{
  bool show_hmap = (DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER) & hmapSubtypeMask);
  bool show_lmap = (DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER) & lmeshSubtypeMask);
  real gcsz = detDivisor ? gridCellSize / detDivisor : gridCellSize;
  real calign = ((detDivisor && !show_hmap) ? gcsz : gridCellSize) * 32;

  hmlService->setupRenderHm2(gcsz, gcsz, calign, calign, show_hmap ? hmapTex[0] : NULL, hmapTexId[0], heightMapOffset.x,
    heightMapOffset.y, heightMap.getMapSizeX() * gridCellSize, heightMap.getMapSizeY() * gridCellSize,
    (detDivisor && (show_hmap || show_lmap)) ? hmapTex[1] : NULL, hmapTexId[1], detRect[0].x, detRect[0].y, detRect.width().x,
    detRect.width().y);
}
void HmapLandPlugin::updateHeightMapTex(bool det_hmap, const IBBox2 *dirty_box)
{
  if (d3d::is_stub_driver() || !hmlService)
    return;

  HeightMapStorage &hm = det_hmap ? heightMapDet : heightMap;
  Texture *&tex = hmapTex[det_hmap ? 1 : 0];
  TEXTUREID &texId = hmapTexId[det_hmap ? 1 : 0];

  if (!hm.isFileOpened())
    return;
  if (det_hmap && !detDivisor)
    return;
  int dmsz_x = det_hmap ? detRectC[1].x - detRectC[0].x : hm.getMapSizeX();
  int dmsz_y = det_hmap ? detRectC[1].y - detRectC[0].y : hm.getMapSizeY();

  int ox = det_hmap ? detRectC[0].x : 0;
  int oy = det_hmap ? detRectC[0].y : 0;
  int x0 = 0, y0 = 0, x1 = dmsz_x, y1 = dmsz_y;
  if (dirty_box)
  {
    x0 = (*dirty_box)[0].x - ox;
    y0 = (*dirty_box)[0].y - oy;
    x1 = (*dirty_box)[1].x - ox + 1;
    y1 = (*dirty_box)[1].y - oy + 1;
    if (x0 < 0)
      x0 = 0;
    if (y0 < 0)
      y0 = 0;
    if (x1 > dmsz_x)
      x1 = dmsz_x;
    if (y1 > dmsz_y)
      y1 = dmsz_y;
  }
  if (x1 <= x0 || y1 <= y0)
    return;

  if (tex)
  {
    TextureInfo ti;
    if (tex->getinfo(ti))
      if (ti.w != dmsz_x || ti.h != dmsz_y)
      {
        debug("updateHeightMapTex(%d): %dx%d -> %dx%d", det_hmap, ti.w, ti.h, dmsz_x, dmsz_y);
        hmlService->setupRenderHm2(1, 1, 1, 1, NULL, BAD_TEXTUREID, 0, 0, 1, 1, NULL, BAD_TEXTUREID, 0, 0, 1, 1);
        dagRender->releaseManagedTexVerified(texId, tex);
      }
  }
  if (!tex)
  {
    tex = d3d::create_tex(NULL, dmsz_x, dmsz_y, TEXFMT_R32F | TEXCF_READABLE | TEXCF_DYNAMIC, 1, det_hmap ? "hmapDet" : "hmapMain");
    debug("updateHeightMapTex(%d): tex=%p %dx%d", det_hmap, tex, dmsz_x, dmsz_y);
    if (tex)
    {
      tex->texaddr(TEXADDR_CLAMP); //== we render finite HMAP! (det_hmap ? TEXADDR_CLAMP : TEXADDR_MIRROR);
      texId = dagRender->registerManagedTex(det_hmap ? "!hmapDet" : "!hmapMain", tex);
    }
  }

  if (!tex)
  {
    logerr_ctx("failed to create HMAP tex");
    return;
  }

  // fill HMAP texture
  unsigned texLockFlags = TEXLOCK_UPDATEFROMSYSTEX | TEXLOCK_RWMASK | TEXLOCK_SYSTEXLOCK;
  float *imgPtr;
  int stride;

  if (tex->lockimgEx(&imgPtr, stride, 0, texLockFlags))
  {
    imgPtr += stride / sizeof(*imgPtr) * y0;
    if (render.showFinalHM)
    {
      for (; y0 < y1; y0++, imgPtr += stride / sizeof(*imgPtr))
        for (int x = x0; x < x1; x++)
          imgPtr[x] = hm.getFinalData(x + ox, y0 + oy);
    }
    else
    {
      for (; y0 < y1; y0++, imgPtr += stride / sizeof(*imgPtr))
        for (int x = x0; x < x1; x++)
          imgPtr[x] = hm.getInitialData(x + ox, y0 + oy);
    }
    tex->unlockimg();
  }
}
void HmapLandPlugin::updateHmap2Tesselation()
{
  if (!hmlService)
    return;

  DataBlock blk;
  if (detDivisor)
  {
    float hmapCellSize = gridCellSize / detDivisor;
    blk.setInt("tesselation", max(1 + render.hm2displacementQ + (int)(log2f(hmapCellSize)), 0));
  }
  hmlService->initLodGridHm2(blk);
}

void HmapLandPlugin::SunSkyLight::save(const char *prefix, DataBlock &blk) const
{
  blk.setE3dcolor(String(128, "sun%sLightColor", prefix), sunColor);
  blk.setE3dcolor(String(128, "sky%sLightColor", prefix), skyColor);
  blk.setReal(String(128, "sun%sLightPower", prefix), sunPower);
  blk.setReal(String(128, "sky%sLightPower", prefix), skyPower);
  blk.setE3dcolor(String(128, "%sspecularColor", prefix), specularColor);
  blk.setReal(String(128, "%sspecularMul", prefix), specularMul);
  blk.setReal(String(128, "%sspecularPower", prefix), specularPower);
}

void HmapLandPlugin::SunSkyLight::load(const char *prefix, const DataBlock &blk)
{
  sunColor = blk.getE3dcolor(String(128, "sun%sLightColor", prefix), sunColor);
  skyColor = blk.getE3dcolor(String(128, "sky%sLightColor", prefix), skyColor);
  sunPower = blk.getReal(String(128, "sun%sLightPower", prefix), sunPower);
  skyPower = blk.getReal(String(128, "sky%sLightPower", prefix), skyPower);
  specularColor = blk.getE3dcolor(String(128, "%sspecularColor", prefix), specularColor);
  specularMul = blk.getReal(String(128, "%sspecularMul", prefix), specularMul);
  specularPower = blk.getReal(String(128, "%sspecularPower", prefix), specularPower);
}

void HmapLandPlugin::autoSaveObjects(DataBlock &local_data)
{
  DataBlock &autoBlk = *local_data.addBlock("panel_state");
  if (propPanel && propPanel->getPanelWindow())
  {
    mainPanelState.reset();
    propPanel->getPanelWindow()->saveState(mainPanelState);
  }
  autoBlk.setFrom(&mainPanelState);

#define ST_LOCAL_VAR(X, TYPE) local_data.set##TYPE(#X, X)
  ST_LOCAL_VAR(showMonochromeLand, Bool);

  ST_LOCAL_VAR(showHtLevelCurves, Bool);
  ST_LOCAL_VAR(htLevelCurveStep, Real);
  ST_LOCAL_VAR(htLevelCurveThickness, Real);
  ST_LOCAL_VAR(htLevelCurveOfs, Real);
  ST_LOCAL_VAR(htLevelCurveDarkness, Real);

  ST_LOCAL_VAR(shownExportedNavMeshIdx, Int);
  ST_LOCAL_VAR(showExportedNavMesh, Bool);
  ST_LOCAL_VAR(showExportedCovers, Bool);
  ST_LOCAL_VAR(showExportedNavMeshContours, Bool);
  ST_LOCAL_VAR(showExpotedObstacles, Bool);
  ST_LOCAL_VAR(disableZtestForDebugNavMesh, Bool);

  ST_LOCAL_VAR(lastHmapImportPath, Str);
  ST_LOCAL_VAR(lastHmapImportPathDet, Str);
  ST_LOCAL_VAR(lastHmapImportPathMain, Str);
  local_data.setStr("lastHmapImportPathWater", lastWaterHeightmapImportPath);
  ST_LOCAL_VAR(lastWaterHeightmapImportPathDet, Str);
  ST_LOCAL_VAR(lastWaterHeightmapImportPathMain, Str);

  ST_LOCAL_VAR(lastLandExportPath, Str);
  ST_LOCAL_VAR(lastHmapExportPath, Str);
  ST_LOCAL_VAR(lastColormapExportPath, Str);
  ST_LOCAL_VAR(lastTexImportPath, Str);

  local_data.setBool("autoUpdateSpline", objEd.autoUpdateSpline);
  local_data.setReal("maxPointVisDist", objEd.maxPointVisDist);
  local_data.setStr("lastExportPath", objEd.lastExportPath);
  local_data.setStr("lastImportPath", objEd.lastImportPath);

  ST_LOCAL_VAR(lastExpLoftFolder, Str);
  ST_LOCAL_VAR(lastExpLoftMain, Bool);
  ST_LOCAL_VAR(lastExpLoftDet, Bool);
  ST_LOCAL_VAR(lastExpLoftMainSz, Int);
  ST_LOCAL_VAR(lastExpLoftDetSz, Int);
  local_data.setBool("lastExpLoftUseRectM", lastExpLoftUseRect[0]);
  local_data.setPoint2("lastExpLoftRectM0", lastExpLoftRect[0][0]);
  local_data.setPoint2("lastExpLoftRectM1", lastExpLoftRect[0][1]);
  local_data.setBool("lastExpLoftUseRectD", lastExpLoftUseRect[1]);
  local_data.setPoint2("lastExpLoftRectD0", lastExpLoftRect[1][0]);
  local_data.setPoint2("lastExpLoftRectD1", lastExpLoftRect[1][1]);
  local_data.setBool("lastExpLoftCreateAreaSubfolders", lastExpLoftCreateAreaSubfolders);

  if (render.hm2displacementQ != 1)
    local_data.setInt("hm2displacementQ", render.hm2displacementQ);
#undef ST_LOCAL_VAR
}

void HmapLandPlugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path)
{
#define ST_LOCAL_VAR(X, TYPE) local_data.set##TYPE(#X, X)
  EditLayerProps::saveLayersConfig(blk, local_data);

  storeLayerTex();

  local_data.setStr("lastHmapImportPath", lastHmapImportPath);
  ST_LOCAL_VAR(lastHmapImportPathMain, Str);
  if (!lastHmapImportPathMain.empty())
  {
    ST_LOCAL_VAR(lastChangeMain.size, Int64);
    ST_LOCAL_VAR(lastChangeMain.mtime, Int64);
  }
  ST_LOCAL_VAR(lastHmapImportPathDet, Str);
  if (!lastHmapImportPathDet.empty())
  {
    ST_LOCAL_VAR(lastChangeDet.size, Int64);
    ST_LOCAL_VAR(lastChangeDet.mtime, Int64);
  }
  local_data.setStr("lastHmapImportPathWater", lastWaterHeightmapImportPath);
  ST_LOCAL_VAR(lastWaterHeightmapImportPathDet, Str);
  if (!lastWaterHeightmapImportPathDet.empty())
  {
    ST_LOCAL_VAR(lastChangeWaterDet.size, Int64);
    ST_LOCAL_VAR(lastChangeWaterDet.mtime, Int64);
  }
  ST_LOCAL_VAR(lastWaterHeightmapImportPathMain, Str);
  if (!lastWaterHeightmapImportPathMain.empty())
  {
    ST_LOCAL_VAR(lastChangeWaterMain.size, Int64);
    ST_LOCAL_VAR(lastChangeWaterMain.mtime, Int64);
  }
  local_data.setStr("lastLandExportPath", lastLandExportPath);
  local_data.setStr("lastHmapExportPath", lastHmapExportPath);
  local_data.setStr("lastColormapExportPath", lastColormapExportPath);
  local_data.setStr("lastTexImportPath", lastTexImportPath);
  local_data.setBool("autoUpdateSpline", objEd.autoUpdateSpline);
  local_data.setReal("maxPointVisDist", objEd.maxPointVisDist);
  local_data.setStr("lastExpLoftFolder", lastExpLoftFolder);
  blk.setBool("lastExpLoftMain", lastExpLoftMain);
  blk.setBool("lastExpLoftDet", lastExpLoftDet);
  blk.setInt("lastExpLoftMainSz", lastExpLoftMainSz);
  blk.setInt("lastExpLoftDetSz", lastExpLoftDetSz);
  blk.setBool("lastExpLoftUseRectM", lastExpLoftUseRect[0]);
  blk.setPoint2("lastExpLoftRectM0", lastExpLoftRect[0][0]);
  blk.setPoint2("lastExpLoftRectM1", lastExpLoftRect[0][1]);
  blk.setBool("lastExpLoftUseRectD", lastExpLoftUseRect[1]);
  blk.setPoint2("lastExpLoftRectD0", lastExpLoftRect[1][0]);
  blk.setPoint2("lastExpLoftRectD1", lastExpLoftRect[1][1]);
  blk.setBool("lastExpLoftCreateAreaSubfolders", lastExpLoftCreateAreaSubfolders);

  if (exportType == EXPORT_HMAP)
    blk.setStr("hmapExportType", "hmap");
  else if (exportType == EXPORT_LANDMESH)
    blk.setStr("hmapExportType", "lmesh");
  else if (exportType == EXPORT_PSEUDO_PLANE)
    blk.setStr("hmapExportType", "plane");

  blk.setReal("meshPreviewDistance", meshPreviewDistance);
  blk.setInt("meshCells", meshCells);
  blk.setReal("meshErrorThreshold", meshErrorThreshold);
  blk.setInt("numMeshVertices", numMeshVertices);
  blk.setInt("lod1TrisDensity", lod1TrisDensity);
  blk.setReal("importanceMaskScale", importanceMaskScale);
  blk.setBool("geomLoftBelowAll", geomLoftBelowAll);

  blk.setBool("hasWaterSurface", hasWaterSurface);
  blk.setStr("waterMatAsset", waterMatAsset);
  blk.setReal("waterSurfaceLevel", waterSurfaceLevel);
  blk.setReal("minUnderwaterBottomDepth", minUnderwaterBottomDepth);
  blk.setBool("hasWorldOcean", hasWorldOcean);
  blk.setReal("worldOceanExpand", worldOceanExpand);
  blk.setReal("worldOceanShorelineTolerance", worldOceanShorelineTolerance);

  if (lastMinHeight[0] != MAX_REAL)
    blk.setReal("lastMinHeight", lastMinHeight[0]);

  if (lastHeightRange[0] != MAX_REAL)
    blk.setReal("lastHeightRange", lastHeightRange[0]);

  if (lastMinHeight[1] != MAX_REAL)
    blk.setReal("lastMinHeightDet", lastMinHeight[1]);

  if (lastHeightRange[1] != MAX_REAL)
    blk.setReal("lastHeightRangeDet", lastHeightRange[1]);

  if (waterHeightMinRangeDet.y > 0.0)
    blk.setPoint2("waterHeightMinRangeDet", waterHeightMinRangeDet);
  if (waterHeightMinRangeMain.y > 0.0)
    blk.setPoint2("waterHeightMinRangeMain", waterHeightMinRangeMain);

  if (!colorGenScriptFilename.empty())
    blk.setStr("colorGenScriptFilename", ::make_path_relative(colorGenScriptFilename, DAGORED2->getSdkDir()));

  blk.setBool("doAutocenter", doAutocenter);

  blk.setReal("gridCellSize", gridCellSize);

  blk.setReal("heightScale", heightMap.heightScale);
  blk.setReal("heightOffset", heightMap.heightOffset);

  blk.setInt("detDivisor", detDivisor);
  blk.setPoint2("detRect0", detRect[0]);
  blk.setPoint2("detRect1", detRect[1]);

  blk.setPoint2("collisionArea_ofs", collisionArea.ofs);
  blk.setPoint2("collisionArea_sz", collisionArea.sz);
  blk.setBool("collisionArea_show", collisionArea.show);

  blk.setPoint2("heightMapOffset", heightMapOffset);

  blk.setReal("shadowBias", shadowBias);
  blk.setReal("shadowTraceDist", shadowTraceDist);
  blk.setReal("shadowDensity", shadowDensity);

  blk.setReal("tileXSize", tileXSize);
  blk.setReal("tileYSize", tileYSize);
  if (requireTileTex)
    blk.setStr("tileTexName", tileTexName);

  blk.setBool("useVertTex", useVertTex);
  blk.setBool("useVertTexForHMAP", useVertTexForHMAP);
  blk.setStr("vertTexName", vertTexName);
  blk.setStr("vertNmTexName", vertNmTexName);
  blk.setStr("vertDetTexName", vertDetTexName);
  blk.setReal("vertTexXZtile", vertTexXZtile);
  blk.setReal("vertTexYtile", vertTexYtile);
  blk.setReal("vertTexYOffset", vertTexYOffset);
  blk.setReal("vertTexAng0", vertTexAng0);
  blk.setReal("vertTexAng1", vertTexAng1);
  blk.setReal("vertTexHorBlend", vertTexHorBlend);
  blk.setReal("vertDetTexXZtile", vertDetTexXZtile);
  blk.setReal("vertDetTexYtile", vertDetTexYtile);
  blk.setReal("vertDetTexYOffset", vertDetTexYOffset);

  DataBlock &newBlk = *blk.addBlock("objects");

  objEd.lastExportPath = ::make_path_relative(objEd.lastExportPath, DAGORED2->getSdkDir());
  objEd.lastImportPath = ::make_path_relative(objEd.lastImportPath, DAGORED2->getSdkDir());
  local_data.setStr("lastExportPath", objEd.lastExportPath);
  local_data.setStr("lastImportPath", objEd.lastImportPath);

  objEd.save(newBlk);

  DataBlock splinesBlk, polysBlk, entBlk, ltBlk;

  objEd.save(splinesBlk, polysBlk, entBlk, ltBlk, -1);

  splinesBlk.saveToTextFile(String(128, "%s/splines.blk", base_path));
  polysBlk.saveToTextFile(String(128, "%s/polys.blk", base_path));
  entBlk.saveToTextFile(String(128, "%s/entities.blk", base_path));
  if (ltBlk.blockCount() || ltBlk.paramCount())
    ltBlk.saveToTextFile(String(128, "%s/lights.blk", base_path));
  else if (dd_file_exist(String(128, "%s/lights.blk", base_path)))
    ltBlk.saveToTextFile(String(128, "%s/lights.blk", base_path));

  for (int i = 0; i < EditLayerProps::layerProps.size(); i++)
  {
    if (EditLayerProps::layerProps[i].nameId == 0)
      continue;
    splinesBlk.clearData(), polysBlk.clearData(), entBlk.clearData();
    ltBlk.clearData();
    objEd.save(splinesBlk, polysBlk, entBlk, ltBlk, i);

    const char *lname = EditLayerProps::layerProps[i].name();
    switch (EditLayerProps::layerProps[i].type)
    {
      case EditLayerProps::SPL: splinesBlk.saveToTextFile(String(128, "%s/splines.%s.blk", base_path, lname)); break;
      case EditLayerProps::PLG: polysBlk.saveToTextFile(String(128, "%s/polys.%s.blk", base_path, lname)); break;
      case EditLayerProps::ENT: entBlk.saveToTextFile(String(128, "%s/entities.%s.blk", base_path, lname)); break;
    }
  }

  DataBlock *brushBlk;

  brushBlk = local_data.addNewBlock("smoothBrush");
  if (brushBlk)
    brushes[SMOOTH_BRUSH]->saveToBlk(*brushBlk);

  brushBlk = local_data.addNewBlock("alignBrush");
  if (brushBlk)
    brushes[ALIGN_BRUSH]->saveToBlk(*brushBlk);

  brushBlk = local_data.addNewBlock("shadowsBrush");
  if (brushBlk)
    brushes[SHADOWS_BRUSH]->saveToBlk(*brushBlk);

  brushBlk = local_data.addNewBlock("scriptBrush");
  if (brushBlk)
    brushes[SCRIPT_BRUSH]->saveToBlk(*brushBlk);

  brushBlk = local_data.addNewBlock("hillUpBrush");
  if (brushBlk)
    brushes[HILLUP_BRUSH]->saveToBlk(*brushBlk);

  brushBlk = local_data.addNewBlock("hillDownBrush");
  if (brushBlk)
    brushes[HILLDOWN_BRUSH]->saveToBlk(*brushBlk);

  DataBlock &renderBlk = *blk.addBlock("render");

  renderBlk.setInt("gridStep", render.gridStep);
  renderBlk.setInt("elemSize", render.elemSize);
  renderBlk.setInt("radiusElems", render.radiusElems);
  renderBlk.setInt("ringElems", render.ringElems);
  renderBlk.setInt("numLods", render.numLods);
  renderBlk.setInt("maxDetailLod", render.maxDetailLod);
  renderBlk.setReal("detailTile", render.detailTile);
  renderBlk.setReal("canyonHorTile", render.canyonHorTile);
  renderBlk.setReal("canyonVertTile", render.canyonVertTile);
  renderBlk.setReal("canyonAngle", render.canyonAngle);
  renderBlk.setReal("canyonFadeAngle", render.canyonFadeAngle);
  renderBlk.setInt("holesLod", render.holesLod);
  renderBlk.setInt("showFinalHM", render.showFinalHM);
  renderBlk.setE3dcolor("monoLand", monochromeLandCol);

  ST_LOCAL_VAR(showMonochromeLand, Bool);

  ST_LOCAL_VAR(showHtLevelCurves, Bool);
  ST_LOCAL_VAR(htLevelCurveStep, Real);
  ST_LOCAL_VAR(htLevelCurveThickness, Real);
  ST_LOCAL_VAR(htLevelCurveOfs, Real);
  ST_LOCAL_VAR(htLevelCurveDarkness, Real);
  ST_LOCAL_VAR(render.hm2YbaseForLod, Real);
#undef ST_LOCAL_VAR

  ldrLight.save("", blk);

  blk.setReal("sunAzimuth", sunAzimuth);
  blk.setReal("sunZenith", sunZenith);

  blk.setInt("landClsMapScaleFactor", lcmScale);
  blk.setInt("lightmapScaleFactor", lightmapScaleFactor);

  blk.setBool("syncLight", syncLight);
  blk.setBool("syncDirLight", syncDirLight);

  for (int i = 0; i < colorGenParams.size(); ++i)
    colorGenParams[i]->save(*colorGenParamsData);

  blk.setBlock(colorGenParamsData, "colorGenParams");

  DataBlock &disShadows = *blk.addNewBlock("disabled_shadows");

  const int colCnt = DAGORED2->getCustomCollidersCount();

  for (int i = 0; i < colCnt; ++i)
  {
    const IDagorEdCustomCollider *collider = DAGORED2->getCustomCollider(i);

    if (!DAGORED2->isCustomShadowEnabled(collider))
      disShadows.addStr("disable", collider->getColliderName());
  }

  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  for (int i = 0; i < scriptImages.size(); ++i)
    if (scriptImages[i]->isImageModified())
      if (!scriptImages[i]->saveImage())
        con.addMessage(ILogWriter::ERROR, "Error saving script image '%s'", scriptImages[i]->getName());
  blk.setBool("showBlueWhiteMask", showBlueWhiteMask);

  if (heightMapDet.isFileOpened())
    if (!flushDataTo(base_path, origBasePath, heightMapDet.getInitialMap()))
      con.addMessage(ILogWriter::ERROR, "Error saving DET heightmap data");
  if (heightMap.isFileOpened() || !landMeshMap.isEmpty())
  {
    if (heightMap.isFileOpened())
      if (!flushDataTo(base_path, origBasePath, heightMap.getInitialMap()))
        con.addMessage(ILogWriter::ERROR, "Error saving heightmap data");

    if (!flushDataTo(base_path, origBasePath, landClsMap))
      con.addMessage(ILogWriter::ERROR, "Error saving landClsMap data");

    if (detTexIdxMap)
      if (!flushDataTo(base_path, origBasePath, *detTexIdxMap))
        DAEDITOR3.conError("Error saving detTexIdx data");
    if (detTexWtMap)
      if (!flushDataTo(base_path, origBasePath, *detTexWtMap))
        DAEDITOR3.conError("Error saving detTexWt data");

    if (hasColorTex)
    {
      if (!colorMap.isFileOpened())
        createColormapFile(con);
      if (!flushDataTo(base_path, origBasePath, colorMap))
        con.addMessage(ILogWriter::ERROR, "Error saving color map data");
    }

    if (hasLightmapTex && exportType != EXPORT_PSEUDO_PLANE)
    {
      if (!lightMapScaled.isFileOpened())
        createLightmapFile(con);
      if (!flushDataTo(base_path, origBasePath, lightMapScaled))
        con.addMessage(ILogWriter::ERROR, "Error saving light map data");
    }
  }


  if (HmapLandPlugin::self->isSnowAvailable() && (snowDynPreview || snowSpherePreview || dynSnowValue != 0 || ambSnowValue != 0))
  {
    if (snowDynPreview)
      blk.addBool("snow_dyn_prewiew", snowDynPreview);
    if (snowSpherePreview)
      blk.addBool("snow_sphere_prewiew", snowSpherePreview);
    blk.addReal("snow_amb_value", ambSnowValue);
  }

  DataBlock *b = blk.addBlock("genLayers");
  saveGenLayers(*b);
  if (!b->blockCount())
    blk.removeBlock("genLayers");


  DataBlock *grassLayersBlk = blk.addBlock("grassLayers");
  if (!grassLayersBlk->blockCount())
    blk.removeBlock("grassLayers");

  if (gpuGrassBlk && !levelBlkFName.empty())
  {
    if (gpuGrassService && gpuGrassService->isGrassEnabled())
    {
      if (!dd_file_exists(levelBlkFName))
        wingw::message_box(wingw::MBS_EXCL, "Error", "Level blk is not valid: %s", levelBlkFName.c_str());
      else if (!gpuGrassPanel.isGrassValid())
        wingw::message_box(wingw::MBS_EXCL, "Error", "Grass properties are not valid and will not be saved");
      else if (!equalDataBlocks(*levelBlk.getBlockByNameEx("grass"), *gpuGrassBlk))
      {
        *levelBlk.addBlock("grass") = *gpuGrassBlk;
        levelBlk.saveToTextFile(levelBlkFName.c_str());
      }
    }
    else
    {
      if (dd_file_exists(levelBlkFName) && levelBlk.getBlockByName("grass"))
      {
        levelBlk.removeBlock("grass");
        levelBlk.saveToTextFile(levelBlkFName.c_str());
      }
      wingw::message_box(wingw::MBS_EXCL, "Warning", "Grass is disabled, properties will not be saved to level blk");
    }
  }
  DataBlock *detail2ColorBlk = blk.addBlock("rendinstDetail2Color");
  saveRendinstDetail2Color(*detail2ColorBlk);

  DataBlock *landModulateColorTexBlk = blk.addBlock("landModulateColorTex");
  saveLandModulateColorTex(*landModulateColorTexBlk);

  DataBlock *horizontalTexBlk = blk.addBlock("horizontalTex");
  saveHorizontalTex(*horizontalTexBlk);


  for (int i = 0; i < MAX_NAVMESHES; ++i)
    *blk.addBlock(i == 0 ? String("navMesh") : String(50, "navMesh%d", i + 1)) = navMeshProps[i];

  local_data.setInt("shownExportedNavMeshIdx", shownExportedNavMeshIdx);
  local_data.setBool("showExportedNavMesh", showExportedNavMesh);
  local_data.setBool("showExportedCovers", showExportedCovers);
  local_data.setBool("showExportedNavMeshContours", showExportedNavMeshContours);
  local_data.setBool("showExpotedObstacles", showExpotedObstacles);
  local_data.setBool("disableZtestForDebugNavMesh", disableZtestForDebugNavMesh);

  /*#if defined(USE_HMAP_ACES)
    // Save tile offsets back to assets blk.
    String text("You should manually change the following detail offset parameters:");
    bool wasChanged = false;
    for (unsigned int tileTexNo = 0;
      tileTexNo < detailTexBlkName.size() && tileTexNo < detailTexOffset.size();
      tileTexNo++)
    {
      DagorAsset *a = DAEDITOR3.getAssetByName(detailTexBlkName[tileTexNo], DAEDITOR3.getAssetTypeId("land"));
      if (a)
      {
        const DataBlock *detailBlk = a->props.getBlockByNameEx("detail");
        Point2 prevOffset = detailBlk->getPoint2("offset", Point2(0.f, 0.f));
        if (!is_equal_float(prevOffset.x, detailTexOffset[tileTexNo].x)
          || !is_equal_float(prevOffset.y, detailTexOffset[tileTexNo].y))
        {
          text += String(200,
            "\nin '%s': from 'detail { offset:p2 = %g, %g }' to 'detail { offset:p2 = %g, %g }'",
            a->getSrcFilePath(),
            prevOffset.x,
            prevOffset.y,
            detailTexOffset[tileTexNo].x,
            detailTexOffset[tileTexNo].y);

          wasChanged = true;
        }
      }
      else if (detailTexBlkName[tileTexNo].length() > 0)
        DAEDITOR3.conError("detailTex[%d] asset <%s> not found", tileTexNo, detailTexBlkName[tileTexNo].str());
    }

    if (wasChanged)
      DAEDITOR3.conWarning("%s", text.str());
    if (wasChanged && !DAGORED2->isInBatchOp())
      wingw::message_box(wingw::MBS_EXCL, "Tile offset changed", text);
  #endif*/

  con.endLog();
  if (strcmp(origBasePath, base_path) != 0)
    origBasePath = base_path;
}

bool HmapLandPlugin::hmCommitChanges()
{
  if (heightMap.isFileOpened())
  {
    heightMap.getInitialMap().flushData();
    heightMap.getInitialMap().closeFile(true);
    heightMap.getInitialMap().openFile(DAGORED2->getPluginFilePath(this, HEIGHTMAP_FILENAME));
  }
  if (landClsMap.isFileOpened())
  {
    landClsMap.flushData();
    landClsMap.closeFile(true);
    landClsMap.openFile(DAGORED2->getPluginFilePath(this, LANDCLSMAP_FILENAME));
  }
  if (colorMap.isFileOpened())
  {
    colorMap.flushData();
    colorMap.closeFile(true);
    colorMap.openFile(DAGORED2->getPluginFilePath(this, COLORMAP_FILENAME));
  }
  if (lightMapScaled.isFileOpened())
  {
    lightMapScaled.flushData();
    lightMapScaled.closeFile(true);
    lightMapScaled.openFile(DAGORED2->getPluginFilePath(this, LIGHTMAP_FILENAME));
  }

  if (detTexIdxMap && detTexIdxMap->isFileOpened())
  {
    detTexIdxMap->flushData();
    detTexIdxMap->closeFile(true);
    detTexIdxMap->openFile(DAGORED2->getPluginFilePath(this, DETTEXMAP_FILENAME));
  }
  if (detTexWtMap && detTexWtMap->isFileOpened())
  {
    detTexWtMap->flushData();
    detTexWtMap->closeFile(true);
    detTexWtMap->openFile(DAGORED2->getPluginFilePath(this, DETTEXWMAP_FILENAME));
  }
  if (detDivisor && heightMapDet.isFileOpened())
  {
    heightMapDet.getInitialMap().flushData();
    heightMapDet.getInitialMap().closeFile(true);
    heightMapDet.getInitialMap().openFile(DAGORED2->getPluginFilePath(this, "det-" HEIGHTMAP_FILENAME));
  }
  {
    const char *names[2] = {WATER_HEIGHTMAP_DET_FILENAME, WATER_HEIGHTMAP_MAIN_FILENAME};
    HeightMapStorage *storages[2] = {&waterHeightmapDet, &waterHeightmapMain};
    for (int i = 0; i < 2; i++)
    {
      String filename = DAGORED2->getPluginFilePath(this, names[i]);
      if (!storages[i]->isFileOpened())
        storages[i]->getInitialMap().createFile(filename.c_str());
      storages[i]->getInitialMap().flushData();
      storages[i]->getInitialMap().closeFile(true);
      storages[i]->getInitialMap().openFile(filename);
    }
  }
  return true;
}

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void HmapLandPlugin::clearObjects()
{
  origBasePath = NULL;
  hmlService->destroyLandMeshRenderer(landMeshRenderer);
  hmlService->destroyLandMeshManager(landMeshManager);
  debugLmeshCells = false;
  renderAllSplinesAlways = false;
  renderSelSplinesAlways = false;

  showBlueWhiteMask = true;
  numDetailTextures = 0;
  detailTexBlkName.clear();

  doAutocenter = false;
  objEd.removeAllObjects(false);

  editedScriptImage = NULL;
  clear_and_shrink(colorGenParams);
  colorGenParamsData->reset();
  clear_and_shrink(scriptImages);

  heightMap.closeFile(true);
  heightMapDet.closeFile(true);
  landClsMap.closeFile(true);
  colorMap.closeFile(true);
  lightMapScaled.closeFile(true);
  waterHeightmapDet.closeFile(true);
  waterHeightmapMain.closeFile(true);

  if (detTexIdxMap)
    detTexIdxMap->closeFile(true);
  if (detTexWtMap)
    detTexWtMap->closeFile(true);

  currentBrushId = 0;
  brushFullDirtyBox.setEmpty();
  noTraceNow = false;

  gridCellSize = 1.0;
  heightMapOffset = Point2(0, 0);

  detDivisor = 0;
  detRect[0].set(0, 0);
  detRect[1].set(0, 0);
  detRectC.setEmpty();
  if (hmlService)
    hmlService->setupRenderHm2(1, 1, 1, 1, NULL, BAD_TEXTUREID, 0, 0, 1, 1, NULL, BAD_TEXTUREID, 0, 0, 1, 1);
  for (int i = 0; i < 2; i++)
    if (hmapTex[i])
      dagRender->releaseManagedTexVerified(hmapTexId[i], hmapTex[i]);

  collisionArea.ofs = Point2(0, 0);
  collisionArea.sz = Point2(100, 100);
  collisionArea.show = false;
  tileXSize = 32;
  tileYSize = 32;
  tileTexName = "---";
  tileTexId = BAD_TEXTUREID;

  useVertTex = false;
  useVertTexForHMAP = true;
  releaseVertTexRef();
  vertTexName = NULL;
  vertNmTexName = NULL;
  vertDetTexName = NULL;
  vertTexXZtile = vertTexYtile = 1;
  vertTexYOffset = 0.f;
  vertTexAng0 = 30;
  vertTexAng1 = 90;
  vertTexHorBlend = 1;
  vertDetTexXZtile = vertDetTexYtile = 1;
  vertDetTexYOffset = 0.f;

  hasWaterSurface = false;
  waterMatAsset = NULL;
  waterSurfaceLevel = 0;
  minUnderwaterBottomDepth = 2;
  hasWorldOcean = true;
  geomLoftBelowAll = false;
  worldOceanExpand = 100;
  worldOceanShorelineTolerance = 0.3;
  waterMask.resize(0, 0);
  waterMaskScale = 1;

  renderDebugLines = false;

  render.init();
  showMonochromeLand = false;
  monochromeLandCol = E3DCOLOR(20, 120, 20, 255);

  showHtLevelCurves = false;
  htLevelCurveStep = 5.0f, htLevelCurveThickness = 0.3f, htLevelCurveOfs = 0.0f, htLevelCurveDarkness = 0.8f;

  sunAzimuth = DegToRad(135);
  sunZenith = DegToRad(45);

  ldrLight.init();

  shadowBias = 0.01f;
  shadowTraceDist = 100;
  shadowDensity = 0.90f;

  lightmapScaleFactor = 1;
  lcmScale = 1;

  lastHmapImportPath = "";
  lastHmapImportPathDet = "";
  lastChangeDet = {};
  lastHmapImportPathMain = "";
  lastChangeMain = {};
  lastWaterHeightmapImportPath = "";
  lastWaterHeightmapImportPathDet = "";
  lastChangeWaterDet = {};
  lastWaterHeightmapImportPathMain = "";
  lastChangeWaterMain = {};
  lastLandExportPath = "";
  lastHmapExportPath = "";
  lastColormapExportPath = "",
  // colorGenScriptFilename
    lastTexImportPath = "";

  lastExpLoftFolder = "";
  lastExpLoftMain = lastExpLoftDet = true;
  lastExpLoftMainSz = lastExpLoftDetSz = 4096;
  lastExpLoftCreateAreaSubfolders = true;
  lastExpLoftUseRect[0] = lastExpLoftUseRect[1] = false;
  lastExpLoftRect[0][0].set(0, 0);
  lastExpLoftRect[0][1].set(1024, 1024);
  lastExpLoftRect[1] = lastExpLoftRect[0];

  isVisible = true;

  lastMinHeight[0] = lastMinHeight[1] = lastHeightRange[0] = lastHeightRange[1] = MAX_REAL;
  waterHeightMinRangeDet = Point2(0, 0);
  waterHeightMinRangeMain = Point2(0, 0);

  syncLight = false;
  syncDirLight = false;
  pendingResetRenderer = false;
  objEd.clearToDefaultState();

  DataBlock app_blk;
  if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
    DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());
  const DataBlock &hmap_def = *app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("hmap");

  defaultHM.sizePixels = hmap_def.getPoint2("sizePixels", Point2(1024, 1024));
  defaultHM.cellSize = hmap_def.getReal("cellSize", 1.0);
  defaultHM.sizeMeters = hmap_def.getPoint2("sizeMeters", defaultHM.sizePixels * defaultHM.cellSize);
  defaultHM.heightScale = hmap_def.getReal("heightScale", 200.0);
  defaultHM.heightOffset = hmap_def.getReal("heightOffset", 0.0);
  defaultHM.originOffset = hmap_def.getPoint2("originOffset", Point2(0, 0));
  defaultHM.collisionOffset = hmap_def.getPoint2("collisionOffset", Point2(0, 0));
  defaultHM.collisionSize = hmap_def.getPoint2("collisionSize", Point2(100, 100));
  defaultHM.collisionShow = hmap_def.getBool("collisionShow", false);
  defaultHM.doAutocenter = hmap_def.getBool("doAutocenter", true);
  defaultHM.tileTex = hmap_def.getStr("tileTex", "");
  defaultHM.tileTexSz = hmap_def.getIPoint2("tileTexSize", IPoint2(32, 32));
  if (defaultHM.doAutocenter)
    defaultHM.originOffset = -defaultHM.sizePixels * defaultHM.cellSize * 0.5;
  if (hmap_def.getStr("script", NULL))
    defaultHM.scriptPath.printf(260, "%s/%s", DAGORED2->getSdkDir(), hmap_def.getStr("script", ""));
  else
    defaultHM.scriptPath = "";

  LandscapeEntityObject::loadColliders(*app_blk.getBlockByNameEx("projectDefaults"));

  heightMap.closeFile();
  colorMap.closeFile();
  lightMapScaled.closeFile();
  del_it(detTexIdxMap);
  del_it(detTexWtMap);

  heightMap.reset(512, 512, 0);
  landClsMap.reset(512, 512, 0);
  if (hasColorTex)
    colorMap.reset(512, 512, E3DCOLOR(255, 10, 255, 0));
  else
    colorMap.defaultValue = E3DCOLOR(128, 128, 128, 0);
  if (hasLightmapTex)
    lightMapScaled.reset(512 * lightmapScaleFactor, 512 * lightmapScaleFactor, 0xFFFF);
  else
    lightMapScaled.defaultValue = E3DCOLOR(255, 10, 255, 0);

  if (propPanel)
    propPanel->fillPanel();
  onLandSizeChanged();
  missing_tile_reported = false;
}


void HmapLandPlugin::onNewProject() { doAutocenter = true; }


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void HmapLandPlugin::setVisible(bool vis) { isVisible = vis; }


//==============================================================================


void HmapLandPlugin::actObjects(float dt)
{

  if (isVisible)
  {
    if (waterService)
      waterService->act(dt);
    if (DAGORED2->curPlugin() == this)
      objEd.update(dt);
    if (pendingLandmeshRebuild)
    {
      DAEDITOR3.conNote("--- Rebuild landmesh");
      rebuildLandmeshDump();
      rebuildLandmeshManager();
      delayedResetRenderer();
      hmlService->invalidateClipmap(true);
      pendingLandmeshRebuild = false;
    }
    if (pendingResetRenderer)
      delayedResetRenderer();
  }
}


void HmapLandPlugin::renderHeight()
{
  int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
  if (!(st_mask & (hmapSubtypeMask | lmeshSubtypeMask)))
    return;

  bool need_hmap = (st_mask & hmapSubtypeMask) || (detDivisor && (st_mask & lmeshSubtypeMask));
  if (need_hmap && (exportType != EXPORT_PSEUDO_PLANE))
  {
    if (!useVertTexForHMAP)
      dagGeom->shaderGlobalSetTexture(landTileVerticalTexVarId, BAD_TEXTUREID);

    hmlService->renderHm2(dagRender->curView().pos - Point3(0, render.hm2YbaseForLod, 0), false, true);

    if (!useVertTexForHMAP)
      updateVertTex();
  }

  if ((st_mask & lmeshSubtypeMask) && landMeshRenderer && !(editedScriptImage && showBlueWhiteMask))
  {
    hmlService->prepareLandMesh(*landMeshRenderer, *landMeshManager, dagRender->curView().pos);

    dagGeom->shaderGlobalSetBlock(dagGeom->shaderGlobalGetBlockId("global_frame"), ShaderGlobal::LAYER_FRAME);

    hmlService->renderLandMeshHeight(*landMeshRenderer, *landMeshManager);

    dagGeom->shaderGlobalSetBlock(-1, ShaderGlobal::LAYER_FRAME);
  }
}

void HmapLandPlugin::renderGrassMask()
{
  int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
  if (!(st_mask & (hmapSubtypeMask | lmeshSubtypeMask)))
    return;


  if ((st_mask & lmeshSubtypeMask) && landMeshRenderer && !(editedScriptImage && showBlueWhiteMask) && landMeshManager)
  {
    IGenViewportWnd *renderView = DAGORED2->getRenderViewport();
    IGenViewportWnd *curView = DAGORED2->getCurrentViewport();

    DagorCurView saveCurView = dagRender->curView();
    if (renderView == curView)
      hmlService->prepareLandMesh(*landMeshRenderer, *landMeshManager, dagRender->curView().pos);
    dagRender->curView() = saveCurView;
  }

  if (landMeshRenderer && landMeshManager)
  {
    DagorCurView saveCurView = dagRender->curView();
    dagGeom->shaderGlobalSetBlock(dagGeom->shaderGlobalGetBlockId("global_frame"), ShaderGlobal::LAYER_FRAME);

    hmlService->renderLandMeshGrassMask(*landMeshRenderer, *landMeshManager);
    int oldMode = hmlService->setGrassMaskRenderingMode();
    objEd.renderGrassMask();
    hmlService->restoreRenderingMode(oldMode);
    dagRender->curView() = saveCurView;

    dagGeom->shaderGlobalSetBlock(-1, ShaderGlobal::LAYER_FRAME);
  }
}

void HmapLandPlugin::renderGrass(Stage stage)
{
  if (grassService == NULL)
    return;

  if (landMeshRenderer)
    landMeshRenderer->setRenderInBBox(*grassService->getGrassBbox());

  grassService->beforeRender(stage);

  if (landMeshRenderer)
    landMeshRenderer->setRenderInBBox(BBox3());


  //      randomGrass->renderOpaque();
  grassService->renderGeometry(stage);
}

void HmapLandPlugin::renderGPUGrass(Stage stage)
{
  if (gpuGrassService == NULL)
    return;

  if (landMeshRenderer)
    landMeshRenderer->setRenderInBBox(*gpuGrassService->getGrassBbox());

  gpuGrassService->beforeRender(stage);

  if (landMeshRenderer)
    landMeshRenderer->setRenderInBBox(BBox3());

  gpuGrassService->renderGeometry(stage);
}

void HmapLandPlugin::resetGrassMask(const DataBlock &grassBlk) {}

void HmapLandPlugin::replaceGrassMask(int colormapId, const char *newGrassMaskName)
{
  // recreate
  if (grassService)
    grassService->forceUpdate();
}

int HmapLandPlugin::getLandclassIndex(const char *landclass_name)
{
  // find index of landclass
  if (landclass_name == NULL)
    return -1;

  int i;
  for (i = 0; i < detailTexBlkName.size(); i++)
    if (strcmp(detailTexBlkName[i], landclass_name) == 0)
      return i;
  return -1;
}


void HmapLandPlugin::updateWaterProjectedFx()
{
  if (!waterProjectedFxSrv)
    return;

  float waterLevel = getWaterSurfLevel();
  float significantWaveHeight = 2.f; // just a constant here

  dagGeom->shaderGlobalSetBlock(dagGeom->shaderGlobalGetBlockId("global_frame"), ShaderGlobal::LAYER_FRAME);

  waterProjectedFxSrv->render(waterLevel, significantWaveHeight);

  dagGeom->shaderGlobalSetBlock(-1, ShaderGlobal::LAYER_FRAME);
}


void HmapLandPlugin::renderWater(Stage stage)
{
  if (waterService == NULL)
    return;

  updateWaterProjectedFx();

  int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
  if (!(st_mask & HmapLandObjectEditor::waterSurfSubtypeMask))
  {
    if (waterService)
      waterService->hide_water();
    return;
  }
  waterService->beforeRender(stage);
  waterService->renderGeometry(stage);
}

void HmapLandPlugin::renderCables()
{
  if (!cableService)
    return;

  IGenViewportWnd *renderView = DAGORED2->getRenderViewport();
  int w, h;
  renderView->getViewportSize(w, h);
  float pixel_scale = tanf(renderView->getFov() * 0.5) / w;
  cableService->beforeRender(pixel_scale);
  cableService->renderGeometry();
}


void HmapLandPlugin::beforeRenderObjects(IGenViewportWnd *vp)
{
  if (vp) //== no viewport-related update
    return;

  if (landMeshManager)
  {
    if (const IBBox2 *b = getExclCellBBox())
    {
      landMeshManager->cullingState.useExclBox = true;
      landMeshManager->cullingState.exclBox[0] = b->lim[0] + landMeshManager->getCellOrigin();
      landMeshManager->cullingState.exclBox[1] = b->lim[1] + landMeshManager->getCellOrigin();
    }
    else
      landMeshManager->cullingState.useExclBox = false;
  }

  objEd.beforeRender();

  if (distFieldInvalid && waterService && get_time_msec() > distFieldBuiltAt + 1000 && useMeshSurface && !landMeshMap.isEmpty() &&
      landMeshRenderer &&
      (DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER) & HmapLandObjectEditor::waterSurfSubtypeMask))
  {
    invalidateDistanceField();
    BBox2 hmap_area;
    if (detDivisor)
      hmap_area = detRect;
    else
    {
      hmap_area[0].set_xz(landBox[0]);
      hmap_area[1].set_xz(landBox[1]);
    }
    waterService->buildDistanceField(hmap_area.center(), max(hmap_area.width().x, hmap_area.width().y) / 2, -1);
    distFieldInvalid = false;
    distFieldBuiltAt = get_time_msec();
    DEBUG_DUMP_VAR(distFieldBuiltAt);
  }
}


void HmapLandPlugin::renderGeometry(Stage stage)
{
  if (!isVisible && stage != STG_RENDER_TO_CLIPMAP)
    return;

  int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
  bool need_lmap = (st_mask & lmeshSubtypeMask);
  bool need_lmap_det = need_lmap && (st_mask & lmeshDetSubtypeMask);
  bool need_hmap = (st_mask & hmapSubtypeMask) || (detDivisor && need_lmap && !need_lmap_det);

  if (showMonochromeLand && stage == STG_RENDER_TO_CLIPMAP)
  {
    d3d::clearview(CLEAR_TARGET, monochromeLandCol, 0.f, 0);
    return;
  }

  switch (stage)
  {
    case STG_BEFORE_RENDER:
    {
      updateHeightMapConstants();

#if defined(USE_HMAP_ACES)
      Driver3dPerspective p(0, 0, 1, 40000, 0, 0);
      if (!d3d::getpersp(p))
      {
        TMatrix4 tm;
        d3d::gettm(TM_PROJ, &tm);
        p.zf = 1.0f / tm.m[2][2];
        p.zn = -p.zf / (1.0f / tm.m[3][2] - 1.0f);
      }
      setZTransformPersp(p.zn, p.zf);
      if (landMeshRenderer && landMeshManager)
      {
        Point3 p = dagRender->curView().pos;
        float height = p.y;
        if (getHeightmapPointHt(p, NULL))
          height = height - p.y;
        if (!(editedScriptImage && showBlueWhiteMask))
          hmlService->prepareClipmap(*landMeshRenderer, *landMeshManager, height * 1.5);
      }

      if (landMeshRenderer && !(editedScriptImage && showBlueWhiteMask) && landMeshManager)
        hmlService->updateGrassTranslucency(*landMeshRenderer, *landMeshManager);

      if (landMeshManager)
      {
        if (need_lmap_det)
          landMeshManager->cullingState.useExclBox = false;
        else if (const IBBox2 *b = getExclCellBBox())
        {
          landMeshManager->cullingState.useExclBox = true;
          landMeshManager->cullingState.exclBox[0] = b->lim[0] + landMeshManager->getCellOrigin();
          landMeshManager->cullingState.exclBox[1] = b->lim[1] + landMeshManager->getCellOrigin();
        }
        else
          landMeshManager->cullingState.useExclBox = false;
      }
#endif

      break;
    }

    case STG_RENDER_STATIC_OPAQUE:
      if (useMeshSurface && !landMeshMap.isEmpty() && !landMeshRenderer)
        resetRenderer();

      hmlService->startUAVFeedback();

      if (need_hmap && exportType != EXPORT_PSEUDO_PLANE)
      {
        IGenViewportWnd *renderView = DAGORED2->getRenderViewport();
        IGenViewportWnd *curView = DAGORED2->getCurrentViewport();

        if (!useVertTexForHMAP)
          dagGeom->shaderGlobalSetTexture(landTileVerticalTexVarId, BAD_TEXTUREID);
        hmlService->renderHm2(dagRender->curView().pos - Point3(0, render.hm2YbaseForLod, 0), false);
        if (!useVertTexForHMAP)
          updateVertTex();
      }

      if (need_lmap && landMeshRenderer && !(editedScriptImage && showBlueWhiteMask))
      {
        IGenViewportWnd *renderView = DAGORED2->getRenderViewport();
        IGenViewportWnd *curView = DAGORED2->getCurrentViewport();

        if (renderView == curView)
          hmlService->prepareLandMesh(*landMeshRenderer, *landMeshManager, dagRender->curView().pos);

#if defined(USE_HMAP_ACES)
        dagGeom->shaderGlobalSetBlock(dagGeom->shaderGlobalGetBlockId("global_frame"), ShaderGlobal::LAYER_FRAME);
#endif
        hmlService->renderLandMesh(*landMeshRenderer, *landMeshManager);
#if defined(USE_HMAP_ACES)
        dagGeom->shaderGlobalSetBlock(-1, ShaderGlobal::LAYER_FRAME);
#endif
      }

      hmlService->endUAVFeedback();


      // grass
      if (st_mask & grasspSubtypeMask)
      {
#if defined(USE_HMAP_ACES)
        dagGeom->shaderGlobalSetBlock(dagGeom->shaderGlobalGetBlockId("global_frame"), ShaderGlobal::LAYER_FRAME);
#endif

        renderGrass(stage);
        renderGPUGrass(stage);
#if defined(USE_HMAP_ACES)
        dagGeom->shaderGlobalSetBlock(-1, ShaderGlobal::LAYER_FRAME);
#endif
      }

      objEd.renderGeometry(true);
      break;

    case STG_RENDER_TO_CLIPMAP:
      if (useMeshSurface && !landMeshMap.isEmpty() && !landMeshRenderer)
        resetRenderer();

      if (landMeshRenderer && (need_lmap || need_hmap))
      {
        hmlService->renderLandMeshClipmap(*landMeshRenderer, *landMeshManager);
        break;
      }

      if (geomLoftBelowAll)
        objEd.renderGeometry(true);
      break;

    case STG_RENDER_TO_CLIPMAP_LATE:
      if (!geomLoftBelowAll)
        objEd.renderGeometry(true);
      break;

    case STG_RENDER_STATIC_TRANS:
      objEd.renderGeometry(false);
      renderWater(stage);
      renderCables();
      break;

    case STG_RENDER_SHADOWS: objEd.renderGeometry(true); break;

    case STG_RENDER_SHADOWS_VSM:
      if (useMeshSurface && !landMeshMap.isEmpty() && !landMeshRenderer)
        resetRenderer();

      if (need_hmap && (exportType != EXPORT_PSEUDO_PLANE))
      {
        IGenViewportWnd *renderView = DAGORED2->getRenderViewport();
        IGenViewportWnd *curView = DAGORED2->getCurrentViewport();

        if (!useVertTexForHMAP)
          dagGeom->shaderGlobalSetTexture(landTileVerticalTexVarId, BAD_TEXTUREID);
        hmlService->renderHm2VSM(dagRender->curView().pos - Point3(0, render.hm2YbaseForLod, 0));
        if (!useVertTexForHMAP)
          updateVertTex();
      }
      if (need_lmap && landMeshRenderer && !(editedScriptImage && showBlueWhiteMask))
      {
        IGenViewportWnd *renderView = DAGORED2->getRenderViewport();
        IGenViewportWnd *curView = DAGORED2->getCurrentViewport();

        if (renderView == curView)
          hmlService->prepareLandMesh(*landMeshRenderer, *landMeshManager, dagRender->curView().pos);

        hmlService->renderLandMeshVSM(*landMeshRenderer, *landMeshManager);
      }
      break;


    case STG_RENDER_HEIGHT_FIELD:
      if (useMeshSurface && !landMeshMap.isEmpty() && !landMeshRenderer)
        resetRenderer();

      renderHeight();
      break;

    case STG_RENDER_GRASS_MASK: renderGrassMask(); break;

    case STG_RENDER_LAND_DECALS:
      if (need_lmap)
        hmlService->renderDecals(*landMeshRenderer, *landMeshManager);
      break;
  }
}

void HmapLandPlugin::prepare(const Point3 &center_pos, const BBox3 &box)
{
  hmlService->prepare(*landMeshRenderer, *landMeshManager, center_pos, box);
}

int HmapLandPlugin::setSubDiv(int lod) { return hmlService->setLod0SubDiv(lod); }

void HmapLandPlugin::renderObjects()
{
  if (!isVisible)
    return;

  objEd.render();

  if (collisionArea.show)
  {
    // validate collision area settings
    if (collisionArea.ofs.x < 0)
      collisionArea.ofs.x = 0;
    else if (collisionArea.ofs.x > 100)
      collisionArea.ofs.x = 100;

    if (collisionArea.ofs.y < 0)
      collisionArea.ofs.y = 0;
    else if (collisionArea.ofs.y > 100)
      collisionArea.ofs.y = 100;

    if (collisionArea.sz.x < 0)
      collisionArea.sz.x = 0;
    else if (collisionArea.ofs.x + collisionArea.sz.x > 100)
      collisionArea.sz.x = 100 - collisionArea.ofs.x;

    if (collisionArea.sz.y < 0)
      collisionArea.sz.y = 0;
    else if (collisionArea.ofs.y + collisionArea.sz.y > 100)
      collisionArea.sz.y = 100 - collisionArea.ofs.y;

    // draw collision area
    real x0 = gridCellSize * heightMap.getMapSizeX() * collisionArea.ofs.x / 100 + heightMapOffset.x;
    real y0 = gridCellSize * heightMap.getMapSizeY() * collisionArea.ofs.y / 100 + heightMapOffset.y;
    real x1 = gridCellSize * heightMap.getMapSizeX() * (collisionArea.ofs.x + collisionArea.sz.x) / 100 + heightMapOffset.x;
    real y1 = gridCellSize * heightMap.getMapSizeY() * (collisionArea.ofs.y + collisionArea.sz.y) / 100 + heightMapOffset.y;
    E3DCOLOR c = E3DCOLOR(128, 0, 128, 255);

    dagRender->startLinesRender();
    dagRender->renderLine(Point3(x0, -10, y0), Point3(x1, -10, y0), c);
    ::dagRender->renderLine(Point3(x1, -10, y0), Point3(x1, -10, y1), c);
    dagRender->renderLine(Point3(x1, -10, y1), Point3(x0, -10, y1), c);
    dagRender->renderLine(Point3(x0, -10, y1), Point3(x0, -10, y0), c);
    dagRender->renderLine(Point3(x0, 250, y0), Point3(x1, 250, y0), c);
    dagRender->renderLine(Point3(x1, 250, y0), Point3(x1, 250, y1), c);
    dagRender->renderLine(Point3(x1, 250, y1), Point3(x0, 250, y1), c);
    dagRender->renderLine(Point3(x0, 250, y1), Point3(x0, 250, y0), c);

    for (int i = 0; i < 10; i++)
    {
      dagRender->renderLine(Point3(x0 + (x1 - x0) * i / 10, -10, y0), Point3(x0 + (x1 - x0) * i / 10, 250, y0), c);
      dagRender->renderLine(Point3(x0 + (x1 - x0) * i / 10, -10, y1), Point3(x0 + (x1 - x0) * i / 10, 250, y1), c);
      dagRender->renderLine(Point3(x0, -10, y0 + (y1 - y0) * i / 10), Point3(x0, 250, y0 + (y1 - y0) * i / 10), c);
      dagRender->renderLine(Point3(x1, -10, y0 + (y1 - y0) * i / 10), Point3(x1, 250, y0 + (y1 - y0) * i / 10), c);
    }

    dagRender->endLinesRender();
  }
}


//==============================================================================
void HmapLandPlugin::renderTransObjects()
{
  if (!isVisible)
    return;

  objEd.renderTrans();
  if (DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER) & navmeshSubtypeMask)
    renderNavMeshDebug();

  if (DAGORED2->curPlugin() == this)
    DAGORED2->renderBrush();
}


//==============================================================================
IGenEventHandler *HmapLandPlugin::getEventHandler() { return this; }


bool HmapLandPlugin::catchEvent(unsigned ev_huid, void *userData)
{
  if (ev_huid == HUID_AfterD3DReset)
  {
    updateHeightMapTex(false);
    updateHeightMapTex(true);
    regenLayerTex();
    if (editedScriptImage && showEditedMask())
      updateBlueWhiteMask(NULL);
    delayedResetRenderer();
    hmlService->invalidateClipmap(true);
  }
  if (ev_huid == HUID_InvalidateClipmap)
  {
    hmlService->invalidateClipmap((bool)userData);
    if (grassService)
      grassService->forceUpdate();
    if (gpuGrassService)
      gpuGrassService->invalidate();
  }
  if (ev_huid == HUID_AutoSaveDueToCrash)
    onPluginMenuClick(CM_COMMIT_HM_CHANGES);
  return false;
}

//==============================================================================
void *HmapLandPlugin::queryInterfacePtr(unsigned huid)
{
  RETURN_INTERFACE(huid, IBinaryDataBuilder);
  RETURN_INTERFACE(huid, ILightingChangeClient);
  RETURN_INTERFACE(huid, IHeightmap);
  RETURN_INTERFACE(huid, IRoadsProvider);
  RETURN_INTERFACE(huid, IWriteAddLtinputData);
  RETURN_INTERFACE(huid, IRenderingService);
  RETURN_INTERFACE(huid, IGatherStaticGeometry);
  RETURN_INTERFACE(huid, IOnExportNotify);
#if defined(USE_HMAP_ACES)
  RETURN_INTERFACE(huid, IEnvironmentSettings);
#endif
  RETURN_INTERFACE(huid, IExportToDag);
  RETURN_INTERFACE(huid, IPluginAutoSave);
  RETURN_INTERFACE(huid, IPostProcessGeometry);
  return NULL;
}


static float old_rad2 = 0, old_assym = 0, old_drad2 = 0;
static void setSuperEntityRef(ICompositObj *co, const char *nm)
{
  for (int j = 0, je = co->getCompositSubEntityCount(); j < je; j++)
    if (IObjEntity *e = co->getCompositSubEntity(j))
    {
      if (IObjEntityUserDataHolder *oeud = e->queryInterface<IObjEntityUserDataHolder>())
        oeud->setSuperEntityRef(String(0, "%s/%d", nm, j));
      if (ICompositObj *co2 = e->queryInterface<ICompositObj>())
        setSuperEntityRef(co2, String(0, "%s/%d", nm, j));
    }
}

void HmapLandPlugin::onBeforeExport(unsigned target_code)
{
  // update splines geometry if needed
  objEd.updateSplinesGeom();
  rebuildSplinesBitmask();

  // generate all data before export operation
  lcMgr->exportEntityGenDataToFile(landClsMap, target_code);

  // update super entity name and subentity index for composit landscape entities
  for (int i = 0; i < objEd.objectCount(); ++i)
  {
    LandscapeEntityObject *obj = RTTI_cast<LandscapeEntityObject>(objEd.getObject(i));
    if (!obj || !obj->getEntity())
      continue;

    const char *nm = obj->getName();
    if (ICompositObj *co = obj->getEntity()->queryInterface<ICompositObj>())
      setSuperEntityRef(co, nm);
    else if (IObjEntityUserDataHolder *oeud = obj->getEntity()->queryInterface<IObjEntityUserDataHolder>())
      oeud->setSuperEntityRef(nm);
  }

  for (uint32_t i = 0; i < EditLayerProps::layerProps.size(); ++i)
    if (EditLayerProps::layerProps[i].exp)
      DAEDITOR3.setEntityLayerHiddenMask(DAEDITOR3.getEntityLayerHiddenMask() & ~(1ull << i));
    else
      DAEDITOR3.setEntityLayerHiddenMask(DAEDITOR3.getEntityLayerHiddenMask() | (1ull << i));
}

void HmapLandPlugin::onAfterExport(unsigned target_code)
{
  if (!DAGORED2->getCurrentViewport())
    return;

  TMatrix itm;
  DAGORED2->getCurrentViewport()->getCameraTransform(itm);

  for (uint32_t i = 0; i < EditLayerProps::layerProps.size(); ++i)
    if (EditLayerProps::layerProps[i].hide)
      DAEDITOR3.setEntityLayerHiddenMask(DAEDITOR3.getEntityLayerHiddenMask() | (1ull << i));
    else
      DAEDITOR3.setEntityLayerHiddenMask(DAEDITOR3.getEntityLayerHiddenMask() & ~(1ull << i));
}

void HmapLandPlugin::selectLayerObjects(int lidx, bool sel)
{
  objEd.getUndoSystem()->begin();
  if (EditLayerProps::layerProps[lidx].type == EditLayerProps::ENT)
  {
    for (int i = 0; i < objEd.objectCount(); i++)
      if (LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(objEd.getObject(i)))
        if (o->getEditLayerIdx() == lidx)
          o->selectObject(sel);
  }
  else if (EditLayerProps::layerProps[lidx].type == EditLayerProps::SPL)
  {
    for (int i = 0; i < objEd.objectCount(); i++)
      if (SplineObject *o = RTTI_cast<SplineObject>(objEd.getObject(i)))
        if (!o->isPoly() && o->getEditLayerIdx() == lidx)
        {
          o->selectObject(sel);
          if (!sel)
            for (int j = 0; j < o->points.size(); j++)
              o->points[j]->selectObject(false);
        }
  }
  else if (EditLayerProps::layerProps[lidx].type == EditLayerProps::PLG)
  {
    for (int i = 0; i < objEd.objectCount(); i++)
      if (SplineObject *o = RTTI_cast<SplineObject>(objEd.getObject(i)))
        if (o->isPoly() && o->getEditLayerIdx() == lidx)
        {
          o->selectObject(sel);
          if (!sel)
            for (int j = 0; j < o->points.size(); j++)
              o->points[j]->selectObject(false);
        }
  }
  objEd.getUndoSystem()->accept(
    String(0, "%s layer \"%s\" objects", sel ? "Select" : "Deselect", EditLayerProps::layerProps[lidx].name()));
}
void HmapLandPlugin::moveObjectsToLayer(int lidx)
{
  class UndoLayerChange : public UndoRedoObject
  {
    Ptr<RenderableEditableObject> obj;
    int oldLayerIdx = -1, newLayerIdx = -1;

  public:
    UndoLayerChange(RenderableEditableObject *o, int old_l, int new_l) : obj(o), oldLayerIdx(old_l), newLayerIdx(new_l) {}

    virtual void restore(bool /*save_redo*/)
    {
      if (LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(obj))
        o->setEditLayerIdx(oldLayerIdx);
      else if (SplineObject *o = RTTI_cast<SplineObject>(obj))
        o->setEditLayerIdx(oldLayerIdx);
    }
    virtual void redo()
    {
      if (LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(obj))
        o->setEditLayerIdx(newLayerIdx);
      else if (SplineObject *o = RTTI_cast<SplineObject>(obj))
        o->setEditLayerIdx(newLayerIdx);
    }

    virtual size_t size() { return sizeof(*this); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoEditLayerIdxChange"; }
  };

  objEd.getUndoSystem()->begin();
  if (EditLayerProps::layerProps[lidx].type == EditLayerProps::ENT)
  {
    for (int i = 0; i < objEd.selectedCount(); i++)
      if (LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(objEd.getSelected(i)))
      {
        objEd.getUndoSystem()->put(new UndoLayerChange(o, o->getEditLayerIdx(), lidx));
        o->setEditLayerIdx(lidx);
      }
  }
  else if (EditLayerProps::layerProps[lidx].type == EditLayerProps::SPL)
  {
    for (int i = 0; i < objEd.selectedCount(); i++)
      if (SplineObject *o = RTTI_cast<SplineObject>(objEd.getSelected(i)))
        if (!o->isPoly())
        {
          objEd.getUndoSystem()->put(new UndoLayerChange(o, o->getEditLayerIdx(), lidx));
          o->setEditLayerIdx(lidx);
        }
  }
  else if (EditLayerProps::layerProps[lidx].type == EditLayerProps::PLG)
  {
    for (int i = 0; i < objEd.selectedCount(); i++)
      if (SplineObject *o = RTTI_cast<SplineObject>(objEd.getSelected(i)))
        if (o->isPoly())
        {
          objEd.getUndoSystem()->put(new UndoLayerChange(o, o->getEditLayerIdx(), lidx));
          o->setEditLayerIdx(lidx);
        }
  }
  objEd.getUndoSystem()->accept(String(0, "Move objects to layer \"%s\"", EditLayerProps::layerProps[lidx].name()));
}
