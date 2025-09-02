// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"

#include "hmlCm.h"

#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include "hmlPanel.h"
#include "recastNavMesh.h"

#include "hmlSelTexDlg.h"
#include "landClassSlotsMgr.h"
#include "roadsSnapshot.h"

#include "Brushes/hmlBrush.h"

#include <math/dag_adjpow2.h>
#include <math/dag_bits.h>
#include <image/dag_tga.h>
#include <image/dag_texPixel.h>

#include <osApiWrappers/dag_direct.h>


#include <generic/dag_tab.h>

#include <coolConsole/coolConsole.h>

#include <EditorCore/ec_IEditorCore.h>
#include <oldEditor/de_workspace.h>
#include <oldEditor/pluginService/de_IDagorPhys.h>

#include <de3_hmapService.h>
#include <de3_genHmapData.h>
#include <de3_interface.h>
#include <de3_entityFilter.h>
#include <de3_splineGenSrv.h>
#include <assets/asset.h>

#include <util/dag_texMetaData.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_createTex.h>
#include <drv/3d/dag_tex3d.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_driver.h>
#include <pathFinder/pathFinder.h>

#include <debug/dag_debug.h>

#include <ctype.h>

#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/panelWindow.h>
#include <stdio.h> // snprintf

using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;
using editorcore_extapi::dagTools;

using hdpi::_pxScaled;

G_STATIC_ASSERT(MAX_NAVMESHES == MAX_UI_NAVMESHES);

#define MIN_HEIGHT_SCALE 0.01

#if !_TARGET_STATIC_LIB
size_t dagormem_max_crt_pool_sz = ~0u;
#endif

IHmapService *HmapLandPlugin::hmlService = NULL;
IBitMaskImageMgr *HmapLandPlugin::bitMaskImgMgr = NULL;
IDagorPhys *HmapLandPlugin::dagPhys = NULL;
int HmapLandPlugin::hmapSubtypeMask = -1;
int HmapLandPlugin::lmeshSubtypeMask = -1;
int HmapLandPlugin::lmeshDetSubtypeMask = -1;
int HmapLandPlugin::grasspSubtypeMask = -1;
int HmapLandPlugin::navmeshSubtypeMask = -1;
extern bool game_res_sys_v2;
bool allow_debug_bitmap_dump = false;

IGrassService *HmapLandPlugin::grassService = NULL;
IGPUGrassService *HmapLandPlugin::gpuGrassService = nullptr;
IWaterService *HmapLandPlugin::waterService = NULL;
ICableService *HmapLandPlugin::cableService = NULL;
IWaterProjFxService *HmapLandPlugin::waterProjectedFxSrv = NULL;

inline bool is_pow_2(int num) { return (num && !(num & (num - 1))); }

// returns the exponent of the greatest power of two equal-to or less than num
// ie GetPower(2) returns 1, GetPower(16) returns 4.
static inline unsigned get_power(int num)
{
  unsigned long power = 0;
  __bit_scan_reverse(power, num);
  return power;
}

//==============================================================================


HmapLandPlugin *HmapLandPlugin::self = NULL;
bool HmapLandPlugin::prepareRequiredServices()
{
  CoolConsole &con = DAGORED2->getConsole();

  hmlService = DAGORED2->queryEditorInterface<IHmapService>();
  if (!hmlService)
  {
    con.addMessage(ILogWriter::FATAL, "missing IHmapService interface");
    return false;
  }

  bitMaskImgMgr = DAGORED2->queryEditorInterface<IBitMaskImageMgr>();
  if (!bitMaskImgMgr)
  {
    con.addMessage(ILogWriter::FATAL, "missing IBitMaskImageMgr interface");
    return false;
  }

  dagPhys = DAGORED2->queryEditorInterface<IDagorPhys>();
  if (!dagPhys)
  {
    con.addMessage(ILogWriter::FATAL, "missing IDagorPhys interface");
    return false;
  }

  return true;
}


HmapLandPlugin::HmapLandPlugin() :
  hasColorTex(false),
  propPanel(NULL),
  currentBrushId(0),
  landClsMap(*hmlService->createUint32MapStorage(512, 512, 0)),
  lcmScale(1),
  colorMap(*hmlService->createColorMapStorage(512, 512, 0)),
  lightMapScaled(*hmlService->createUint32MapStorage(512, 512, 0)),
  colorGenParamsData(NULL),
  lightmapScaleFactor(1),
  detDivisor(0),
  detRect(0, 0, 0, 0),
  geomLoftBelowAll(false),
  hasWaterSurface(false),
  waterSurfaceLevel(0),
  minUnderwaterBottomDepth(2),
  hasWorldOcean(true),
  worldOceanExpand(100),
  worldOceanShorelineTolerance(0.3),
  waterMaskScale(1),
  landMeshRenderer(NULL),
  landMeshManager(NULL),
  lmDump(NULL),
  pendingLandmeshRebuild(false),
  gridCellSize(1),
  doAutocenter(false),
  sunAzimuth(DegToRad(135)),
  sunZenith(DegToRad(45)),
  numDetailTextures(0),
  // detailTexOffset(midmem),
  calculating_shadows(false),
  detailTexBlkName(midmem),
  colorGenParams(midmem_ptr()),
  scriptImages(midmem_ptr()),
  shadowBias(0.01f),
  shadowTraceDist(100),
  shadowDensity(0.90f),
  heightMapOffset(0, 0),
  brushDlg(NULL),
  noTraceNow(false),
  syncLight(true),
  syncDirLight(false),
  meshPreviewDistance(20000.f),
  meshCells(16),
  meshErrorThreshold(1.f),
  numMeshVertices(90000),
  lod1TrisDensity(30),
  importanceMaskScale(10.f),
  lightmapTexId(BAD_TEXTUREID),
  lmlightmapTexId(BAD_TEXTUREID),
  lmlightmapTex(NULL),
  bluewhiteTexId(BAD_TEXTUREID),
  bluewhiteTex(NULL),
  exportType(EXPORT_HMAP),
  lcMgr(NULL),
  debugLmeshCells(false),
  renderAllSplinesAlways(false),
  renderSelSplinesAlways(false),
  distFieldInvalid(true),
  distFieldBuiltAt(0),
  snowDynPreview(false),
  snowSpherePreview(false),
  ambSnowValue(0),
  dynSnowValue(0),
  snowPrevievSVId(-1),
  snowValSVId(-1)
{
  lastMinHeight[0] = lastMinHeight[1] = lastHeightRange[0] = lastHeightRange[1] = MAX_REAL;
  detTexIdxMap = detTexWtMap = NULL;
  hmapTex[0] = hmapTex[1] = NULL;
  hmapTexId[0] = hmapTexId[1] = BAD_TEXTUREID;
  waterHeightMinRangeDet = Point2(0, 0);
  waterHeightMinRangeMain = Point2(0, 0);

  lastExpLoftRect[0][0].set(0, 0);
  lastExpLoftRect[0][1].set(1024, 1024);
  lastExpLoftRect[1] = lastExpLoftRect[0];

  pendingResetRenderer = false;
  editedScriptImageIdx = -1;
  esiGridW = esiGridH = 1;
  esiGridStep = 1;
  esiOrigin.set(0, 0);
  showBlueWhiteMask = true;
  showMonochromeLand = false;

  shownExportedNavMeshIdx = 0;
  showExportedNavMesh = false;
  showExportedCovers = false;
  showExportedNavMeshContours = false;
  showExpotedObstacles = false;
  disableZtestForDebugNavMesh = false;

  monochromeLandCol = E3DCOLOR(20, 120, 20, 255);
  showHtLevelCurves = false;
  htLevelCurveStep = 5.0f, htLevelCurveThickness = 0.3f, htLevelCurveOfs = 0.0f, htLevelCurveDarkness = 0.8f;

  G_ASSERT(sizeof(detLayerDescStorage) >= sizeof(HmapBitLayerDesc));
  G_ASSERT(sizeof(impLayerDescStorage) >= sizeof(HmapBitLayerDesc));

  heightMap.heightScale = 1;
  heightMap.heightOffset = 1;

  hmlService->createStorage(heightMap, 512, 512, true);
  hmlService->createStorage(heightMapDet, 1, 1, true);
  hmlService->createStorage(waterHeightmapDet, 0, 0, false);
  hmlService->createStorage(waterHeightmapMain, 0, 0, false);
  DAGORED2->getWorkspace().getHmapSettings(requireTileTex, hasColorTex, hasLightmapTex, useMeshSurface, useNormalMap);

  DataBlock app_blk;
  if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
    DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());

  ::game_res_sys_v2 = app_blk.getBlockByNameEx("assets")->getBlockByName("export") != NULL;
  allow_debug_bitmap_dump = app_blk.getBlockByNameEx("SDK")->getBool("allow_debug_bitmap_dump", false);

  storeNxzInLtmapTex = app_blk.getBlockByNameEx("heightMap")->getBool("storeNxzInLtmapTex", false);

  landClsLayersHandle = hmlService->createBitLayersList(DAGORED2->getWorkspace().getHmapLandClsLayersDesc());
  landClsLayer = hmlService->getBitLayersList(landClsLayersHandle);

  genHmap = hmlService->registerGenHmap("hmap", &heightMap, &landClsMap, landClsLayer, hasColorTex ? &colorMap : NULL,
    hasLightmapTex ? &lightMapScaled : NULL, heightMapOffset, gridCellSize);
  G_ASSERT(genHmap && "can't register as <hmap>");

  genHmap->sunColor = ldrLight.getSunLightColor();
  genHmap->skyColor = ldrLight.getSkyLightColor();

  detLayerIdx = hmlService->findBitLayerByAttr(landClsLayersHandle, hmlService->getBitLayerAttrId(landClsLayersHandle, "detTexSlot"));
  impLayerIdx =
    hmlService->findBitLayerByAttr(landClsLayersHandle, hmlService->getBitLayerAttrId(landClsLayersHandle, "importanceMaskSlot"));

  String ed_cls_fn(0, "%s/assets/editable_landclass_layers.blk", DAGORED2->getWorkspace().getSdkDir());
  if (dd_file_exist(ed_cls_fn))
  {
    DAEDITOR3.conNote("loading <%s>", ed_cls_fn);
    DataBlock blk(ed_cls_fn);
    hmlService->createDetLayerClassList(blk);
  }

  if (detLayerIdx < 0)
    memset(detLayerDescStorage, 0, sizeof(HmapBitLayerDesc));
  else
    memcpy(detLayerDescStorage, &landClsLayer[detLayerIdx], sizeof(HmapBitLayerDesc));

  if (impLayerIdx < 0)
    memset(impLayerDescStorage, 0, sizeof(HmapBitLayerDesc));
  else
    memcpy(impLayerDescStorage, &landClsLayer[impLayerIdx], sizeof(HmapBitLayerDesc));

  debug("using HMAP settings: requireTileTex=%d, hasColorTex=%d, hasLightmapTex=%d "
        "detTexLayer=%d impLayerIdx=%d",
    requireTileTex, hasColorTex, hasLightmapTex, detLayerIdx, impLayerIdx);

  lcMgr = new LandClassSlotsManager(landClsLayer.size());
  onLandSizeChanged();

  self = this;

  colorGenParamsData = new (midmem) DataBlock;
  collisionArea.ofs = Point2(0, 0);
  collisionArea.sz = Point2(100, 100);
  collisionArea.show = false;
  tileXSize = 32;
  tileYSize = 32;
  tileTexName = "---";
  tileTexId = BAD_TEXTUREID;

  useVertTex = false;
  useVertTexForHMAP = true;
  vertTexXZtile = 1;
  vertTexYtile = 1;
  vertTexYOffset = 0.f;
  vertTexAng0 = 30;
  vertTexAng1 = 90;
  vertTexHorBlend = 1;
  vertDetTexXZtile = 1.f;
  vertDetTexYtile = 1.f;
  vertDetTexYOffset = 0.f;


  useHorizontalTex = false;
  horizontalTex_TileSizeX = 128.f;
  horizontalTex_TileSizeY = 128.f;
  horizontalTex_OffsetX = 0.f;
  horizontalTex_OffsetY = 0.f;
  horizontalTex_DetailTexSizeX = 128.f;
  horizontalTex_DetailTexSizeY = 128.f;

  useLandModulateColorTex = false;
  landModulateColorEdge0 = 0.f;
  landModulateColorEdge1 = 1.f;

  useRendinstDetail2Modulation = false;
  rendinstDetail2ColorFrom = E3DCOLOR(128, 128, 128, 128);
  rendinstDetail2ColorTo = E3DCOLOR(128, 128, 128, 128);

  enableGrass = false;
  grassBlk = NULL;
  gpuGrassBlk = nullptr;
  grass.masks.clear();
  grass.defaultMinDensity = 0.2f;
  grass.defaultMaxDensity = 0.75f;
  grass.maxRadius = 400.f;
  grass.texSize = 256;

  grass.sowPerlinFreq = 0.1;
  grass.fadeDelta = 100;
  grass.lodFadeDelta = 30;
  grass.blendToLandDelta = 100;

  grass.noise_first_rnd = 23.1406926327792690;
  grass.noise_first_val_mul = 1;
  grass.noise_first_val_add = 0;

  grass.noise_second_rnd = 0.05;
  grass.noise_second_val_mul = 1;
  grass.noise_second_val_add = 0;

  grass.directWindMul = 1;
  grass.noiseWindMul = 1;
  grass.windPerlinFreq = 50000;
  grass.directWindFreq = 1;
  grass.directWindModulationFreq = 1;
  grass.windWaveLength = 100;
  grass.windToColor = 0.2;
  grass.helicopterHorMul = 2;
  grass.helicopterVerMul = 1;
  grass.helicopterToDirectWindMul = 2;
  grass.helicopterFalloff = 0.001;

  renderDebugLines = false;
  isVisible = true;

  if (useNormalMap && !hasLightmapTex)
  {
    useNormalMap = false;
    DAEDITOR3.conError("hasNormalMap=true requires hasLightmapTex to be also true!");
  }
  if (storeNxzInLtmapTex && !hasLightmapTex)
  {
    storeNxzInLtmapTex = false;
    DAEDITOR3.conError("storeNxzInLtmapTex=true requires hasLightmapTex to be also true!");
  }
  if (storeNxzInLtmapTex && useNormalMap)
  {
    useNormalMap = false;
    DAEDITOR3.conError("storeNxzInLtmapTex=true conflicts with useNormalMap=true!");
  }

  objEd.requireTileTex = requireTileTex;
  objEd.hasColorTex = hasColorTex;
  objEd.hasLightmapTex = hasLightmapTex;
  objEd.useMeshSurface = useMeshSurface;

  navMeshBuf = dagRender->newDebugPrimitivesVbuffer("navmesh_lines_vbuf", midmem);
  coversBuf = dagRender->newDebugPrimitivesVbuffer("covers_lines_vbuf", midmem);
  contoursBuf = dagRender->newDebugPrimitivesVbuffer("contours_lines_vbuf", midmem);
  obstaclesBuf = dagRender->newDebugPrimitivesVbuffer("obstacles_lines_vbuf", midmem);
  EditLayerProps::resetLayersToDefauls();

  IAssetService *assetSrv = DAGORED2->queryEditorInterface<IAssetService>();
  if (assetSrv)
    assetSrv->subscribeUpdateNotify(this, true, false);
}

void HmapLandPlugin::RenderParams::init()
{
  gridStep = 1;
  elemSize = 8;
  radiusElems = 4;
  ringElems = 3;
  numLods = 4;
  maxDetailLod = 2;
  detailTile = 4.0;
  canyonAngle = 60;
  canyonFadeAngle = 10;
  canyonHorTile = 10.0f;
  canyonVertTile = 10.0f;
  hm2YbaseForLod = 0;
  showFinalHM = true;
  hm2displacementQ = 1;
}


HmapLandPlugin::~HmapLandPlugin()
{
  if (ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>())
    splSrv->setSweepMask(nullptr, 0, 0, 1);

  detailedLandMask = NULL;
  editedScriptImage = NULL;
  clear_and_shrink(colorGenParams);
  clear_and_shrink(genLayers);
  clear_and_shrink(grassLayers);

  del_it(propPanel);
  for (int i = 0; i < 2; i++)
    if (hmapTex[i])
      dagRender->releaseManagedTexVerified(hmapTexId[i], hmapTex[i]);

  clearTexCache();
  hmlService->unregisterGenHmap("hmap");
  del_it(lcMgr);
  del_it(detTexIdxMap);
  del_it(detTexWtMap);
  delete &landClsMap;
  delete &colorMap;
  delete &lightMapScaled;
  hmlService->destroyStorage(heightMap);
  hmlService->destroyStorage(heightMapDet);
  hmlService->destroyStorage(waterHeightmapDet);
  hmlService->destroyStorage(waterHeightmapMain);
  hmlService->destroyBitLayersList(landClsLayersHandle);
  landClsLayersHandle = NULL;
  del_it(brushDlg);
  delete colorGenParamsData;
  clear_all_ptr_items(scriptImages);

  self = NULL;

  dagRender->deleteDebugPrimitivesVbuffer(navMeshBuf);
  dagRender->deleteDebugPrimitivesVbuffer(coversBuf);
  dagRender->deleteDebugPrimitivesVbuffer(contoursBuf);
  dagRender->deleteDebugPrimitivesVbuffer(obstaclesBuf);
  del_it(grassBlk);
  del_it(gpuGrassBlk);

  IAssetService *assetSrv = DAGORED2->queryEditorInterface<IAssetService>();
  if (assetSrv)
    assetSrv->unsubscribeUpdateNotify(this, true, false);
  for (int i = 0; i < physMaps.size(); ++i)
    del_it(physMaps[i]);
  dagRender->releaseManagedTex(lightmapTexId);
}


//==============================================================================

void HmapLandPlugin::createPropPanel()
{
  if (!propPanel)
    propPanel = new (inimem) HmapLandPanel(*this);

  G_ASSERT(propPanel);

  // propPanel->fillToolBar();
}

PropPanel::ContainerPropertyControl *HmapLandPlugin::getPropPanel() const { return propPanel ? propPanel->getPanelWindow() : NULL; }


//==============================================================================
void HmapLandPlugin::registerMenuAccelerators()
{
  IWndManager &wndManager = *DAGORED2->getWndManager();

  wndManager.addAccelerator(CM_REIMPORT, ImGuiKey_F4);
  wndManager.addAccelerator(CM_EXPORT_LOFT_MASKS, ImGuiKey_F9);

  // ObjectEditor has an accelerator with the same hotkey but because this is registered first, this will "win".
  wndManager.addViewportAccelerator(CM_TOGGLE_PROPERTIES_AND_OBJECT_PROPERTIES, ImGuiKey_P);

  wndManager.addViewportAccelerator(CM_DECREASE_BRUSH_SIZE, ImGuiKey_LeftBracket, true);
  wndManager.addViewportAccelerator(CM_INCREASE_BRUSH_SIZE, ImGuiKey_RightBracket, true);
  wndManager.addViewportAccelerator(CM_BUILD_COLORMAP, ImGuiMod_Ctrl | ImGuiKey_G);
  wndManager.addViewportAccelerator(CM_COMMIT_HM_CHANGES, ImGuiMod_Ctrl | ImGuiKey_H);
  wndManager.addViewportAccelerator(CM_RESTORE_HM_BACKUP, ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_H);
  wndManager.addViewportAccelerator(CM_HIDE_UNSELECTED_SPLINES, ImGuiMod_Ctrl | ImGuiKey_E);
  wndManager.addViewportAccelerator(CM_UNHIDE_ALL_SPLINES, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_E);
  wndManager.addViewportAccelerator(CM_COLLAPSE_MODIFIERS, ImGuiMod_Ctrl | ImGuiKey_M);
  wndManager.addViewportAccelerator(CM_REBUILD, ImGuiMod_Ctrl | ImGuiKey_R);
  wndManager.addViewportAccelerator(CM_HILL_UP, ImGuiKey_1);
  wndManager.addViewportAccelerator(CM_HILL_DOWN, ImGuiKey_2);
  wndManager.addViewportAccelerator(CM_ALIGN, ImGuiKey_3);
  wndManager.addViewportAccelerator(CM_SMOOTH, ImGuiKey_4);
  wndManager.addViewportAccelerator(CM_SHADOWS, ImGuiKey_5);
  wndManager.addViewportAccelerator(CM_SCRIPT, ImGuiKey_6);

  objEd.registerViewportAccelerators(wndManager);
}

void HmapLandPlugin::registerMenuSettings(unsigned menu_id, int base_id)
{
  settingsBaseId = base_id;

  PropPanel::IMenu *mainMenu = DAGORED2->getMainMenu();

  // settings
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, base_id + CM_PREFERENCES_PLACE_TYPE, "Object Properties: place type");
  mainMenu->setEnabledById(base_id + CM_PREFERENCES_PLACE_TYPE, false);
  const unsigned int cmPrefPlaceTypeDropdown = base_id + CM_PREFERENCES_PLACE_TYPE_DROPDOWN;
  mainMenu->addItem(menu_id, cmPrefPlaceTypeDropdown, "Dropdown");
  const unsigned int cmPrefPlaceTypeRadio = base_id + CM_PREFERENCES_PLACE_TYPE_RADIO;
  mainMenu->addItem(menu_id, cmPrefPlaceTypeRadio, "Radiobuttons");

  const bool placeTypeRadio = ObjectEditor::getPlaceTypeRadio();
  mainMenu->setRadioById(placeTypeRadio ? cmPrefPlaceTypeRadio : cmPrefPlaceTypeDropdown, cmPrefPlaceTypeDropdown,
    cmPrefPlaceTypeRadio);
}

void HmapLandPlugin::createMenu(unsigned menu_id)
{
  // menu

  PropPanel::IMenu *mainMenu = DAGORED2->getMainMenu();

  mainMenu->addItem(menu_id, CM_CREATE_HEIGHTMAP, "Create heightmap");
  mainMenu->addItem(menu_id, CM_IMPORT_HEIGHTMAP, "Import heightmap");
  mainMenu->addItem(menu_id, CM_ERASE_HEIGHTMAP, "Erase heightmap");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_IMPORT_WATER_DET_HMAP, "Import water det heightmap");
  mainMenu->addItem(menu_id, CM_IMPORT_WATER_MAIN_HMAP, "Import water main heightmap");
  mainMenu->addItem(menu_id, CM_ERASE_WATER_HEIGHTMAPS, "Erase water heightmaps");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_REIMPORT, "Re-import changed heightmaps\tF4");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_RESCALE_HMAP, "Rescale heightmap...");
  mainMenu->addSeparator(menu_id);

  mainMenu->addItem(menu_id, CM_RESTORE_HM_BACKUP, "Revert heightmap changes\tCtrl+Alt+H");
  mainMenu->addItem(menu_id, CM_COMMIT_HM_CHANGES, "Save heightmap changes\tCtrl+H");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_MOVE_OBJECTS, "Move objects...");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_BUILD_COLORMAP, "Rebuild colors\tCtrl+G");
  mainMenu->addItem(menu_id, CM_BUILD_LIGHTMAP, "Rebuild ligting");
  mainMenu->addItem(menu_id, CM_REBUILD, "Rebuild colors and lighting\tCtrl+R");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_REBUILD_RIVERS, "Rebuild rivers");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_EXPORT_AS_COMPOSIT, "Export as composit...");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_SPLIT_COMPOSIT, "Split composits into separate objects...");
  mainMenu->addItem(menu_id, CM_INSTANTIATE_GENOBJ_INTO_ENTITIES, "Instantiate gen. objects into separate entities...");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_EXPORT_LAND_TO_GAME, "Export to game...");

  mainMenu->addItem(menu_id, CM_EXPORT_HEIGHTMAP, "Export heightmap...");
  if (hasColorTex)
    mainMenu->addItem(menu_id, CM_EXPORT_COLORMAP, "Export colormap...");
  mainMenu->addItem(menu_id, CM_EXPORT_LAYERS, "Export landClass layers...");
  mainMenu->addItem(menu_id, CM_EXPORT_LOFT_MASKS, "Export loft masks...\tF9");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_SPLINE_IMPORT_FROM_DAG, "Import from DAG...");
  mainMenu->addItem(menu_id, CM_SPLINE_EXPORT_TO_DAG, "Export to DAG...");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_COLLAPSE_MODIFIERS, "Collapse modifier(s)\tCtrl+M");
  mainMenu->addItem(menu_id, CM_UNIFY_OBJ_NAMES, "Unify object names...");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_SET_PT_VIS_DIST, "Set spline points vis. range...");
  mainMenu->addItem(menu_id, CM_HIDE_UNSELECTED_SPLINES, "Hide (inactivate) all unselected splines\tCtrl+E");
  mainMenu->addItem(menu_id, CM_UNHIDE_ALL_SPLINES, "Unhide (activate) all splines\tCtrl+Shift+E");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_AUTO_ATACH, "Auto attach splines...");
  mainMenu->addItem(menu_id, CM_MAKE_SPLINES_CROSSES, "Make crosspoints for splines...");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, CM_MAKE_BOTTOM_SPLINES, "Make bottom splines for polygons...");
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

bool HmapLandPlugin::getTexEntry(const char *s, String *path, String *name) const
{
  if (!s || !*s)
  {
    if (path)
      *path = "";
    return false;
  }

  String asset(DagorAsset::fpath2asset(s)), tmp_stor;
  if (name)
    *name = asset;

  DagorAsset *a = DAEDITOR3.getAssetByName(asset, DAEDITOR3.getAssetTypeId("tex"));

  if (a)
  {
    if (path && game_res_sys_v2)
      path->printf(128, "%s*", a->getName());
    else if (path)
    {
      TextureMetaData tmd;
      tmd.read(a->props, "PC");
      *path = tmd.encode(a->getTargetFilePath(), &tmp_stor);
    }
    return true;
  }
  debug("false: %s", s);
  return false;
}


void HmapLandPlugin::refillPanel(bool schedule_regen)
{
  if (propPanel)
    propPanel->fillPanel(true, schedule_regen, true);
}

void HmapLandPlugin::fillPanel(PropPanel::ContainerPropertyControl &panel)
{
  panel.clear();
  panel.disableFillAutoResize();

  PropPanel::ContainerPropertyControl *exportGroup = panel.createGroup(PID_HM_EXPORT_GRP, "Export Parameters");

  Tab<String> typesExport(tmpmem);
  typesExport.push_back() = "Pseudo plane, no export";    // EXPORT_PSEUDO_PLANE
  typesExport.push_back() = "Regular height map, 'hmap'"; // EXPORT_HMAP
  if (useMeshSurface)
    typesExport.push_back() = "Optimized land mesh, 'lmap'"; // EXPORT_LANDMESH

  exportGroup->createCombo(PID_HM_EXPORT_TYPE, "Export type:", typesExport, exportType);
  exportGroup->createCheckBox(PID_HM_LOFT_BELOW_ALL, "Geom: render loft below all", geomLoftBelowAll);
  exportGroup->createEditFloat(PID_RI_MAX_CELL_SIZE, "Max cell size for RendInst", riMaxCellSz);
  exportGroup->createEditInt(PID_RI_MAX_GEN_LAYER_CELL_DIVISOR, "Limit genLayer cell divisor for RI", riMaxGenLayerCellDivisor);
  panel.setBool(PID_HM_EXPORT_GRP, true);


  //---Mesh
  if (useMeshSurface)
  {
    PropPanel::ContainerPropertyControl *meshGroup = panel.createGroup(PID_MESH_GRP, "Mesh");

    meshGroup->createEditFloat(PID_MESH_PREVIEW_DISTANCE, "Preview distance", meshPreviewDistance);
    if (exportType != EXPORT_PSEUDO_PLANE)
      meshGroup->createButton(PID_REBUILD_MESH, "Rebuild mesh");

    meshGroup->createCheckBox(PID_APPLY_HEIGHTBAKE, "Apply HeightBake Splines", applyHeightBakeSplines);
    meshGroup->createCheckBox(PID_APPLY_HEIGHTBAKE_ON_EDIT, "Apply HeightBake Splines On Edit", applyHeightBakeSplinesOnEdit);

    meshGroup->createButton(PID_REBAKE_HMAP_MODIFIERS, "Rebake hmap modifiers");
    meshGroup->createButton(PID_CONVERT_DELAUNAY_SPLINES_TO_HEIGHTBAKE, "Convert Delaunay Splines To HeightBake");
    meshGroup->createSeparator(0);
    // meshGroup->createEditFloat(PID_MESH_CELL_SIZE, "Cell size", meshCellSize);
    meshGroup->createEditInt(PID_MESH_CELLS, "Cell Grid", meshCells);
    meshGroup->createEditFloat(PID_MESH_ERROR_THRESHOLD, "Error threshold", meshErrorThreshold);
    meshGroup->createEditInt(PID_NUM_MESH_VERTICES, "Vertices", numMeshVertices);
    meshGroup->createTrackInt(PID_LOD1_DENSITY, "LOD1 density (%)", lod1TrisDensity, 1, 100, 1);
    meshGroup->createEditFloat(PID_IMPORTANCE_MASK_SCALE, "Importance scale", importanceMaskScale);
    meshGroup->createSeparator(0);
    meshGroup->createCheckBox(PID_WATER_SURF, "Has water surface", hasWaterSurface);
    meshGroup->createButton(PID_WATER_SET_MAT, String(128, "Water mat: <%s>", waterMatAsset.str()));
    meshGroup->createEditFloat(PID_WATER_SURF_LEV, "Water surface level (m)", waterSurfaceLevel);
    meshGroup->createEditFloat(PID_MIN_UNDERWATER_BOTOM_DEPTH, "Min underwater depth (m)", minUnderwaterBottomDepth);
    meshGroup->createCheckBox(PID_WATER_OCEAN, "Add world ocean", hasWorldOcean);
    meshGroup->createEditFloat(PID_WATER_OCEAN_EXPAND, "World ocean expand (m)", worldOceanExpand);
    meshGroup->createEditFloat(PID_WATER_OCEAN_SL_TOL, "Shoreline tolerance (m)", worldOceanShorelineTolerance);

    meshGroup->createIndent();
    meshGroup->createButton(PID_WATER_SURF_REBUILD, "Rebuild water");

    if (dagGeom->getShaderVariableId("landmesh_debug_cells_scale") >= 0)
    {
      meshGroup->createSeparator(0);
      meshGroup->createCheckBox(PID_DEBUG_CELLS, "Highlight cells for debug", debugLmeshCells);
    }

    panel.setBool(PID_MESH_GRP, true);
  }

  //---Brush
  PropPanel::ContainerPropertyControl *maxGrp = panel.createGroup(PID_BRUSH_GRP, "Brush");
  brushes[currentBrushId]->fillParams(*maxGrp);
  panel.setBool(PID_BRUSH_GRP, true);

  // Following block is not used in Warthunder version of DaEditor
  /*  if (exportType!=EXPORT_PSEUDO_PLANE)
    {
      maxGrp = panel.createGroup(PID_DETAIL_TEX_LIST_GRP, "Canyon settings");
      maxGrp->createEditFloat(PID_CANYON_ANGLE, "Canyon angle", render.canyonAngle);
      maxGrp->createEditFloat(PID_CANYON_FADE_ANGLE, "Canyon fade angle", render.canyonFadeAngle);

      panel.setBool(PID_DETAIL_TEX_LIST_GRP, true);
    } */

  // grass options
  if (DAGORED2->queryEditorInterface<IGrassService>())
  {
    const int PID_GRASS_GRP = 0;
    grass_panel = panel.createGroup(PID_GRASS_GRP, "Grass");

    grass_panel->createCheckBox(PID_ENABLE_GRASS, "Enable grass", enableGrass, true);

    grass_panel->createButton(PID_GRASS_LOADFROM_LEVELBLK, "Load from level.blk");
    grass_panel->createButton(PID_GRASS_ADDLAYER, "Add new grass layer");

    // base options
    int i = 0;
    for (const auto &mask : grass.masks)
      grass_panel->createFileButton(PID_GRASS_MASK_START + i++, mask.first, mask.second);
    grass_panel->createEditFloat(PID_GRASS_DEFAULT_MIN_DENSITY, "Min density", grass.defaultMinDensity);
    grass_panel->setMinMaxStep(PID_GRASS_DEFAULT_MIN_DENSITY, 0.f, 10.f, 0.01);

    grass_panel->createEditFloat(PID_GRASS_DEFAULT_MAX_DENSITY, "Max density", grass.defaultMaxDensity);
    grass_panel->setMinMaxStep(PID_GRASS_DEFAULT_MAX_DENSITY, 0.f, 10.f, 0.01);

    grass_panel->createEditFloat(PID_GRASS_MAX_RADIUS, "Max radius", grass.maxRadius);
    grass_panel->setMinMaxStep(PID_GRASS_MAX_RADIUS, 0.f, 10000.f, 0.5);

    grass_panel->createEditInt(PID_GRASS_TEX_SIZE, "Texture size", grass.texSize);

    grass_panel->createEditFloat(PID_GRASS_SOW_PER_LIN_FREQ, "Sow per lin. freq.", grass.sowPerlinFreq);
    grass_panel->createEditFloat(PID_GRASS_FADE_DELTA, "Fade delta", grass.fadeDelta);
    grass_panel->createEditFloat(PID_GRASS_LOD_FADE_DELTA, "LOD fade delta", grass.lodFadeDelta);
    grass_panel->createEditFloat(PID_GRASS_BLEND_TO_LAND_DELTA, "Blend to land delta", grass.blendToLandDelta);

    grass_panel->createEditFloat(PID_GRASS_NOISE_FIRST_RND, "Noise first rnd.", grass.noise_first_rnd, 16);
    grass_panel->createEditFloat(PID_GRASS_NOISE_FIRST_VAL_MUL, "Noise first val. mul.", grass.noise_first_val_mul);
    grass_panel->createEditFloat(PID_GRASS_NOISE_FIRST_VAL_ADD, "Noise first val. add", grass.noise_first_val_add);

    grass_panel->createEditFloat(PID_GRASS_NOISE_SECOND_RND, "Noise second rnd.", grass.noise_second_rnd);
    grass_panel->createEditFloat(PID_GRASS_NOISE_SECOND_VAL_MUL, "Noise second val. mul.", grass.noise_second_val_mul);
    grass_panel->createEditFloat(PID_GRASS_NOISE_SECOND_VAL_ADD, "Noise second val. add", grass.noise_second_val_add);

    grass_panel->createEditFloat(PID_GRASS_DIRECT_WIND_MUL, "Direct wind mul.", grass.directWindMul);
    grass_panel->createEditFloat(PID_GRASS_NOISE_WIND_MUL, "Noise wind mul.", grass.noiseWindMul);
    grass_panel->createEditFloat(PID_GRASS_WIND_PER_LIN_FREQ, "Wind per lin. freq.", grass.windPerlinFreq);
    grass_panel->createEditFloat(PID_GRASS_DIRECT_WIND_FREQ, "Direct wind freq.", grass.directWindFreq);
    grass_panel->createEditFloat(PID_GRASS_DIRECT_WIND_MODULATION_FREQ, "Wind modulation freq.", grass.directWindModulationFreq);
    grass_panel->createEditFloat(PID_GRASS_WIND_WAVE_LENGTH, "Wind wave length", grass.windWaveLength);
    grass_panel->createEditFloat(PID_GRASS_WIND_TO_COLOR, "Wind to color", grass.windToColor);
    grass_panel->createEditFloat(PID_GRASS_HELICOPTER_HOR_MUL, "Helicopter hor. mul.", grass.helicopterHorMul);
    grass_panel->createEditFloat(PID_GRASS_HELICOPTER_VER_MUL, "Helicopter ver. mul.", grass.helicopterVerMul);
    grass_panel->createEditFloat(PID_GRASS_HELICOPTER_TO_DIRECT_WIND_MUL, "Helicopter to direct wind mul.",
      grass.helicopterToDirectWindMul);
    grass_panel->createEditFloat(PID_GRASS_HELICOPTER_FALLOFF, "Helicopter falloff", grass.helicopterFalloff, 3);

    grass_panel->createSeparator(0);

    // grass layers
    int grass_pid = PID_GRASS_PARAM_START;
    for (int i = 0; i < grassLayers.size(); ++i)
      grassLayers[i]->fillParams(grass_panel, grass_pid);

    panel.setBool(PID_GRASS_GRP, true);
  }

  if (gpuGrassService && gpuGrassBlk)
    gpuGrassPanel.fillPanel(gpuGrassService, gpuGrassBlk, panel);
  //---Script Parameters
  maxGrp = panel.createGroup(PID_SCRIPT_PARAMS_GRP, "Script Parameters");

  maxGrp->createButton(PID_SCRIPT_FILE, String("Script: ") + ::get_file_name_wo_ext(colorGenScriptFilename));
  maxGrp->createButton(PID_RESET_SCRIPT, "Reset script");
  maxGrp->createButton(PID_RELOAD_SCRIPT, "Reload script");
  // maxGrp->createButton(PID_IMPORT_SCRIPT_IMAGE, "Import image...");
  // maxGrp->createButton(PID_CREATE_MASK, "Create mask...");
  maxGrp->createButton(PID_GENERATE_COLORMAP, "Generate color map");

  maxGrp->createIndent();

  int pid = PID_SCRIPT_PARAM_START;
  for (int i = 0; i < colorGenParams.size(); ++i)
    colorGenParams[i]->fillParams(*maxGrp, pid);

  maxGrp->createCheckBox(PID_SHOWMASK, "Show blue-white mask", showBlueWhiteMask);

  for (int i = 0; i < genLayers.size(); ++i)
    genLayers[i]->fillParams(*maxGrp, pid);
  G_ASSERT(pid <= PID_LAST_SCRIPT_PARAM);
  maxGrp->createSeparator(0);
  maxGrp->createButton(PID_ADDLAYER, "Add new gen layer");

  panel.setBool(PID_SCRIPT_PARAMS_GRP, true);

#if defined(USE_HMAP_ACES)
/*  maxGrp = panel.createGroup(PID_TILE_OFFSET_GRP, "Tile offset");
  panel.setBool(PID_TILE_OFFSET_GRP, false);
  for (unsigned int tileTexNo = 0;
    tileTexNo < detailTexBlkName.size() && tileTexNo < detailTexOffset.size();
    tileTexNo++)
  {
    if (detailTexBlkName[tileTexNo].length() > 0)
      maxGrp->createPoint2(PID_DETAIL_TEX_OFFSET_0 + tileTexNo,
        ::dd_get_fname(detailTexBlkName[tileTexNo]), detailTexOffset[tileTexNo]);
  }*/
#endif


  //---Heightmap Parameters
  maxGrp = panel.createGroup(PID_HMAP_PARAMS_GRP, "Heightmap Parameters");
  maxGrp->createEditFloat(PID_GRID_CELL_SIZE, "Cell size", gridCellSize);
  if (exportType != EXPORT_PSEUDO_PLANE)
  {
    maxGrp->createEditFloat(PID_HEIGHT_SCALE, "Height scale", heightMap.heightScale);
    maxGrp->setMinMaxStep(PID_HEIGHT_SCALE, MIN_HEIGHT_SCALE, 10e6, 0.01);
    maxGrp->createEditFloat(PID_HEIGHT_OFFSET, "Height offset", heightMap.heightOffset);
  }
  maxGrp->createPoint2(PID_HEIGHTMAP_OFFSET, "Origin offset", heightMapOffset, 2, !doAutocenter);
  maxGrp->createCheckBox(PID_HEIGHTMAP_AUTOCENTER, "Automatic center", doAutocenter);

  PropPanel::ContainerPropertyControl *grp;

  if (exportType != EXPORT_PSEUDO_PLANE)
  {
    PropPanel::ContainerPropertyControl *rg1 = maxGrp->createRadioGroup(PID_LCMAP_SUBDIV, "Land class map detail");
    rg1->createRadio(PID_LCMAP_1, "1x one-to-one");
    rg1->createRadio(PID_LCMAP_2, "2x");
    rg1->createRadio(PID_LCMAP_4, "4x");
    switch (lcmScale)
    {
      case 1: maxGrp->setInt(PID_LCMAP_SUBDIV, PID_LCMAP_1); break;
      case 2: maxGrp->setInt(PID_LCMAP_SUBDIV, PID_LCMAP_2); break;
      case 4: maxGrp->setInt(PID_LCMAP_SUBDIV, PID_LCMAP_4); break;
    }

    PropPanel::ContainerPropertyControl *subGrpH2 = maxGrp->createGroupBox(-1, "Detailed Heightmap area");
    subGrpH2->createEditFloat(PID_GRID_H2_CELL_SIZE, "Cell size:", detDivisor ? gridCellSize / detDivisor : 0);
    subGrpH2->createPoint2(PID_GRID_H2_BBOX_OFS, "Area origin:", detRect[0]);
    subGrpH2->createPoint2(PID_GRID_H2_BBOX_SZ, "Area Size:", detRect.width());
    subGrpH2->createButton(PID_GRID_H2_APPLY, "Apply!");
    subGrpH2->setEnabledById(PID_GRID_H2_APPLY, false);

    PropPanel::ContainerPropertyControl *rg2 = maxGrp->createRadioGroup(PID_HMAP_UPSCALE, "[Slow] Upscale Heightmap");
    rg2->createRadio(PID_HMAP_UPSCALE_1, "1x (disabled)");
    rg2->createRadio(PID_HMAP_UPSCALE_2, "2x");
    rg2->createRadio(PID_HMAP_UPSCALE_4, "4x");
    int desiredHmapUpscale = hmlService ? hmlService->getDesiredHmapUpscale() : 1;
    switch (desiredHmapUpscale)
    {
      case 1: maxGrp->setInt(PID_HMAP_UPSCALE, PID_HMAP_UPSCALE_1); break;
      case 2: maxGrp->setInt(PID_HMAP_UPSCALE, PID_HMAP_UPSCALE_2); break;
      case 4: maxGrp->setInt(PID_HMAP_UPSCALE, PID_HMAP_UPSCALE_4); break;
    }
    Tab<String> upscaleAlgos(tmpmem);
    upscaleAlgos.push_back() = "Bicubic";
    upscaleAlgos.push_back() = "Lanczos";
    upscaleAlgos.push_back() = "Steffen Hermite";

    int hmapUpscaleAlgo = hmlService ? hmlService->getHmapUpscaleAlgo() : 0;
    maxGrp->createCombo(PID_HMAP_UPSCALE_ALGO, "Upscale Algorithm:", upscaleAlgos, hmapUpscaleAlgo);

    // Following block is not used in Warthunder version of DaEditor
    /*    grp = maxGrp->createGroupBox(0, "Preload collision area");

        grp->createPoint2(PID_HM_COLLISION_OFFSET, "Offset (in %):", collisionArea.ofs);
        grp->createPoint2(PID_HM_COLLISION_SIZE, "Size (in %):", collisionArea.sz);
        grp->createCheckBox(PID_HM_COLLISION_SHOW, "Show area", collisionArea.show);*/


    //---Renderer
    grp = panel.createGroup(PID_RENDERER_GRP, "Renderer");
    grp->createEditInt(PID_RENDER_GRID_STEP, "Mesh grid step", render.gridStep);
    grp->createEditInt(PID_RENDER_RADIUS_ELEMS, "1st LoD radius", render.radiusElems);
    grp->createEditInt(PID_RENDER_RING_ELEMS, "LoD ring size", render.ringElems);
    grp->createEditInt(PID_RENDER_NUM_LODS, "Number of LoDs", render.numLods);
    grp->createEditInt(PID_RENDER_MAX_DETAIL_LOD, "Detail tex. range", render.maxDetailLod);
    grp->createEditFloat(PID_RENDER_DETAIL_TILE, "Detail tex. tile", render.detailTile);
    grp->createEditFloat(PID_RENDER_CANYON_HOR_TILE, "Canyon hor. tile", render.canyonHorTile);
    grp->createEditFloat(PID_RENDER_CANYON_VERT_TILE, "Canyon vert. tile", render.canyonVertTile);
    grp->createEditInt(PID_RENDER_ELEM_SIZE, "Mesh element size", render.elemSize);


    grp->createSeparator(0);
    PropPanel::ContainerPropertyControl *rg = grp->createRadioGroup(PID_RENDER_RADIOGROUP_HM, "Show heightmap");

    rg->createRadio(PID_RENDER_INITIAL_HM, "initial");
    rg->createRadio(PID_RENDER_FINAL_HM, "final (after modifiers)");
    grp->setInt(PID_RENDER_RADIOGROUP_HM, (render.showFinalHM) ? PID_RENDER_FINAL_HM : PID_RENDER_INITIAL_HM);

    grp->createSeparator(0);
    grp->createEditInt(PID_HM2_DISPLACEMENT_Q, "HMAP displacement quality", render.hm2displacementQ);
    grp->setMinMaxStep(PID_HM2_DISPLACEMENT_Q, 0, 5, 1);
    grp->createSeparator(0);

    grp->createCheckBox(PID_RENDER_DEBUG_LINES, "Render polygon debug lines", renderDebugLines);
    grp->createCheckBox(PID_RENDER_ALL_SPLINES_ALWAYS, "Render all splines always", renderAllSplinesAlways);
    grp->createCheckBox(PID_RENDER_SEL_SPLINES_ALWAYS, "Render sel splines always", renderSelSplinesAlways);

    grp->createSeparator(0);
    grp->createCheckBox(PID_MONOLAND, "Show monochrome land", showMonochromeLand);
    grp->createColorBox(PID_MONOLAND_COL, "Monochrome land color", monochromeLandCol);

    grp->createSeparator(0);
    grp->createColorBox(PID_GROUND_OBJECTS_COL, "Ground objects color");

    if (dagGeom->getShaderVariableId("heightmap_show_level_curves") >= 0)
    {
      grp->createSeparator(0);
      grp->createCheckBox(PID_HTLEVELS, "Show height level curves", showHtLevelCurves);
      grp->createEditFloat(PID_HTLEVELS_STEP, "Level step, m", htLevelCurveStep);
      grp->setMinMaxStep(PID_HTLEVELS_STEP, 0.01f, 100.f, 0.5);
      grp->createEditFloat(PID_HTLEVELS_OFS, "Height base, m", htLevelCurveOfs);
      grp->setMinMaxStep(PID_HTLEVELS_OFS, -8000, 8000.f, 1);
      grp->createEditFloat(PID_HTLEVELS_THICK, "Curve thickness, m", htLevelCurveThickness);
      grp->createEditFloat(PID_HTLEVELS_DARK, "Curve darkness, %", htLevelCurveDarkness * 100);
      grp->setMinMaxStep(PID_HTLEVELS_DARK, 5, 100, 10);
    }
    grp->createEditFloat(PID_RENDER_HM2_YBASE_FOR_LOD, "Base height for LOD", render.hm2YbaseForLod);

    panel.setBool(PID_RENDERER_GRP, true);
  }

  panel.setBool(PID_HMAP_PARAMS_GRP, true);


  //---Textures (groups several rarely used panels):
  PropPanel::ContainerPropertyControl *landOptionsGrp = panel.createGroup(PID_LAND_TEXTURES_OPTIONS_GRP, "Textures:");

  //---Tile detail texture
  if (requireTileTex)
  {
    maxGrp = landOptionsGrp->createGroup(PID_TILE_TEX_GRP, "Tile detail texture");

    maxGrp->createEditFloat(PID_TILE_SIZE_X, "Tile X size", tileXSize);
    maxGrp->setMinMaxStep(PID_TILE_SIZE_X, 10e-3, 10e6, 0.001);
    maxGrp->createEditFloat(PID_TILE_SIZE_Y, "Tile Y size", tileYSize);
    maxGrp->setMinMaxStep(PID_TILE_SIZE_Y, 10e-3, 10e6, 0.001);

    const char *name = tileTexName;

    if (!name || !*name)
      name = "---";

    maxGrp->createButton(PID_TILE_TEX, String(128, "tex: %s", name));

    landOptionsGrp->setBool(PID_TILE_TEX_GRP, true);
  }


  //---Land color modulate tex
  // if ( requireLandColorModulateTex )
  {
    maxGrp = landOptionsGrp->createGroup(PID_LAND_MODULATE_TEX_TEX_GRP, "Land color modulate tex");

    maxGrp->createCheckBox(PID_USE_LAND_MODULATE_TEX, "Use modulate texture", useLandModulateColorTex);

    maxGrp->createButton(PID_LAND_MODULATE_TEX_NAME, String(128, "tex: %s", landModulateColorTexName.str()));

    maxGrp->createEditFloat(PID_LAND_MODULATE_EDGE0_X, "0 weight at:", landModulateColorEdge0);
    maxGrp->setMinMaxStep(PID_LAND_MODULATE_EDGE0_X, 0.f, 1.f, 0.001);
    maxGrp->createEditFloat(PID_LAND_MODULATE_EDGE1_X, "1 weight at:", landModulateColorEdge1);
    maxGrp->setMinMaxStep(PID_LAND_MODULATE_EDGE1_X, 0.001f, 1.f, 0.001);

    landOptionsGrp->setBool(PID_LAND_MODULATE_TEX_TEX_GRP, true);
  }


  //---Landscape geometry horizontal texture
  // if ( requireHorizontalTex)
  {
    maxGrp = landOptionsGrp->createGroup(PID_HORIZONTAL_TEX_GRP, "Horizontal texture");

    maxGrp->createCheckBox(PID_USE_HORIZONTAL_TEX, "Use horizontal texture", useHorizontalTex);

    maxGrp->createButton(PID_HORIZONTAL_TEX_NAME, String(128, "tex: %s", horizontalTexName.str()));
    maxGrp->createButton(PID_HORIZONTAL_DET_TEX_NAME, String(128, "det.tex: %s", horizontalDetailTexName.str()));

    maxGrp->createEditFloat(PID_HORIZONTAL_TILE_SIZE_X, "Tile X size", horizontalTex_TileSizeX);
    maxGrp->setMinMaxStep(PID_HORIZONTAL_TILE_SIZE_X, 10e-3, 10e6, 0.1);
    maxGrp->createEditFloat(PID_HORIZONTAL_TILE_SIZE_Y, "Tile Y size", horizontalTex_TileSizeY);
    maxGrp->setMinMaxStep(PID_HORIZONTAL_TILE_SIZE_Y, 10e-3, 10e6, 0.1);

    maxGrp->createEditFloat(PID_HORIZONTAL_OFFSET_X, "X offset", horizontalTex_OffsetX);
    maxGrp->setMinMaxStep(PID_HORIZONTAL_OFFSET_X, 10e-3, 10e6, 0.01);
    maxGrp->createEditFloat(PID_HORIZONTAL_OFFSET_Y, "Y offset", horizontalTex_OffsetY);
    maxGrp->setMinMaxStep(PID_HORIZONTAL_OFFSET_Y, 10e-3, 10e6, 0.01);

    maxGrp->createEditFloat(PID_HORIZONTAL_DETAIL_TILE_SIZE_X, "Detail Tile X size", horizontalTex_DetailTexSizeX);
    maxGrp->setMinMaxStep(PID_HORIZONTAL_DETAIL_TILE_SIZE_X, 10e-3, 10e6, 0.1);
    maxGrp->createEditFloat(PID_HORIZONTAL_DETAIL_TILE_SIZE_Y, "Detail Tile Y size", horizontalTex_DetailTexSizeY);
    maxGrp->setMinMaxStep(PID_HORIZONTAL_DETAIL_TILE_SIZE_Y, 10e-3, 10e6, 0.1);

    landOptionsGrp->setBool(PID_HORIZONTAL_TEX_GRP, true);
  }


//---Vertical texture
#if defined(USE_LMESH_ACES)
  maxGrp = landOptionsGrp->createGroup(PID_VERT_TEX_GRP, "Vertical texture");

  maxGrp->createCheckBox(PID_VERT_TEX_USE, "Use vertical texture", useVertTex);
  maxGrp->createCheckBox(PID_VERT_TEX_USE_HMAP, "Use VT for detailed HMAP", useVertTexForHMAP);
  maxGrp->createButton(PID_VERT_TEX_NAME, String(128, "tex: %s", vertTexName.str()));
  maxGrp->createButton(PID_VERT_NM_TEX_NAME, String(128, "nm tex: %s", vertNmTexName.str()));
  maxGrp->createButton(PID_VERT_DET_TEX_NAME, String(128, "det tex: %s", vertDetTexName.str()));
  maxGrp->createEditFloat(PID_VERT_TEX_TILE_XZ, "XZ Tile, m", vertTexXZtile);
  maxGrp->setMinMaxStep(PID_VERT_TEX_TILE_XZ, 10e-3, 10e6, 0.001);
  maxGrp->createEditFloat(PID_VERT_TEX_TILE_Y, "Y Tile, m", vertTexYtile);
  maxGrp->setMinMaxStep(PID_VERT_TEX_TILE_Y, 10e-3, 10e6, 0.001);
  maxGrp->createEditFloat(PID_VERT_TEX_Y_OFS, "Y Offset, m", vertTexYOffset);
  maxGrp->setMinMaxStep(PID_VERT_TEX_Y_OFS, -10e6, 10e6, 0.001);
  maxGrp->createEditFloat(PID_VERT_TEX_ANG0, "Start angle, deg", vertTexAng0);
  maxGrp->setMinMaxStep(PID_VERT_TEX_ANG0, 0, 90, 0.1);
  maxGrp->createEditFloat(PID_VERT_TEX_ANG1, "Full angle, deg", vertTexAng1);
  maxGrp->setMinMaxStep(PID_VERT_TEX_ANG1, 0, 90, 0.1);
  maxGrp->createEditFloat(PID_VERT_TEX_HBLEND, "Horizontal blend", vertTexHorBlend);
  maxGrp->setMinMaxStep(PID_VERT_TEX_HBLEND, 0, 100, 0.1);
  maxGrp->createEditFloat(PID_VERT_DET_TEX_TILE_XZ, "Det XZ Tile, m", vertDetTexXZtile);
  maxGrp->setMinMaxStep(PID_VERT_DET_TEX_TILE_XZ, 10e-3, 10e6, 0.001);
  maxGrp->createEditFloat(PID_VERT_DET_TEX_TILE_Y, "Det Y Tile, m", vertDetTexYtile);
  maxGrp->setMinMaxStep(PID_VERT_DET_TEX_TILE_Y, 10e-3, 10e6, 0.001);
  maxGrp->createEditFloat(PID_VERT_DET_TEX_Y_OFS, "Det Y Offset, m", vertDetTexYOffset);
  maxGrp->setMinMaxStep(PID_VERT_DET_TEX_Y_OFS, -10e6, 10e6, 0.001);
  landOptionsGrp->setBool(PID_VERT_TEX_GRP, true);
#endif


  //---Normalmap
  if (exportType != EXPORT_PSEUDO_PLANE)
  {
    grp = landOptionsGrp->createGroup(PID_LIGHTING_GRP, "Normalmap");
    // Following block is not used in Warthunder version of DaEditor
    /*
        grp->createCheckBox(PID_GET_SUN_SETTINGS_AUTOMAT, "Automatic sync sun dir and color",
          syncLight);
        grp->createCheckBox(PID_GET_SUN_DIR_SETTINGS_AUTOMAT, "Automatic sync sun dir",
          syncDirLight);
        grp->createButton(PID_GET_SUN_SETTINGS, "Get sun settings", !syncLight);
        grp->createColorBox(PID_SUN_LIGHT_COLOR, "Sun light color", ldrLight.sunColor, !syncLight);
        grp->createEditFloat(PID_SUN_LIGHT_POWER, "Sun light power", ldrLight.sunPower, 2, !syncLight);
        grp->createColorBox(PID_SPECULAR_COLOR, "Specular color", ldrLight.specularColor, !syncLight);
        grp->createEditFloat(PID_SPECULAR_MUL, "Specular multiplier", ldrLight.specularMul);
        grp->createEditFloat(PID_SPECULAR_POWER, "Shininess", ldrLight.specularPower);
        grp->createColorBox(PID_SKY_LIGHT_COLOR, "Sky light color", ldrLight.skyColor, !syncLight);
        grp->createEditFloat(PID_SKY_LIGHT_POWER, "Sky light power",
          ldrLight.skyPower * 4, 2, !syncLight);

        grp->createTrackFloat(PID_SUN_AZIMUTH, "Sun azimuth",
          RadToDeg(HALFPI - sunAzimuth), -180, +180,  0.01f, !syncLight && !syncDirLight);
        grp->createTrackFloat(PID_SUN_ZENITH, "Sun zenith angle",
          RadToDeg(sunZenith), 5, +90, 0.01f, !syncLight && !syncDirLight);
    */

    if (hasLightmapTex)
    {
      grp->createIndent();

      // Following block is not used, we have normalmap only
      /*      if (useNormalMap)
            {
              grp->createButton(PID_CALC_FAST_LIGHTING, "Calculate normalmap");
            }
            else*/
      {
        //        grp->createButton(PID_CALC_FAST_LIGHTING, "Calculate lighting (no shadows)");
        grp->createButton(PID_CALC_GOOD_LIGHTING, "Calculate normalmap");
      }

      // Following block is not used in Warthunder version of DaEditor
      /*     PropPanel::ContainerPropertyControl *rg = grp->createRadioGroup(PID_LIGHTING_SUBDIV,
             useNormalMap ? "NormalmapDetail" : "LightmapDetail");

           rg->createRadio(PID_LIGHTING_1, "1x one-to-one");
           rg->createRadio(PID_LIGHTING_2, "2x");
           rg->createRadio(PID_LIGHTING_4, "4x");

           int rgid = -1;
           switch (lightmapScaleFactor)
           {
             case 1:
               rgid = PID_LIGHTING_1;
               break;

             case 2:
               rgid = PID_LIGHTING_2;
               break;

             case 4:
               rgid = PID_LIGHTING_4;
               break;
           }

           grp->setInt(PID_LIGHTING_SUBDIV, rgid);

           if (!useNormalMap)
           {
             PropPanel::ContainerPropertyControl* subGrp =
               grp->createGroup(PID_SHADOW_GRP, "Shadows");
             subGrp->createEditFloat(PID_SHADOW_BIAS, "Shadow bias", shadowBias, 3);
             subGrp->createEditFloat(PID_SHADOW_TRACE_DIST, "Shadow trace distance", shadowTraceDist);
             subGrp->createEditFloat(PID_SHADOW_DENSITY, "Shadow density %", shadowDensity*100);

             panel.setBool(PID_SHADOW_GRP, true);

             subGrp = grp->createGroup(PID_SHADOW_CASTERS_GRP, "Shadow casters");

             const int colCnt = DAGORED2->getCustomCollidersCount();

             G_ASSERT(colCnt < PID_SHADOW_CASTER_LAST - PID_SHADOW_CASTER_FIRST);

             for (int i = 0; i < colCnt; ++i)
             {
               const IDagorEdCustomCollider* collider = DAGORED2->getCustomCollider(i);

               if (collider)
                 subGrp->createCheckBox(PID_SHADOW_CASTER_FIRST + i, collider->getColliderName(),
                   DAGORED2->isCustomShadowEnabled(collider));
             }
             panel.setBool(PID_SHADOW_CASTERS_GRP, true);
           }*/
    }
    landOptionsGrp->setBool(PID_LIGHTING_GRP, true);
  }


  //---Rendinst color
  {
    grp = landOptionsGrp->createGroup(PID_RENDIST_DETAIL2_COLOR_GRP, "Rendinst  2-nd detail color");

    grp->createCheckBox(PID_USE_RENDIST_DETAIL2_COLOR, "Use detail2 color", useRendinstDetail2Modulation);

    grp->createColorBox(PID_RENDIST_DETAIL2_COLOR_FROM, "From color:", rendinstDetail2ColorFrom);
    grp->createColorBox(PID_RENDIST_DETAIL2_COLOR_TO, "From to:", rendinstDetail2ColorTo); // e3dcolor(

    landOptionsGrp->setBool(PID_RENDIST_DETAIL2_COLOR_GRP, true);
  }

  panel.setBool(PID_LAND_TEXTURES_OPTIONS_GRP, true);

  //---Navigation meshes
  {
    grp = panel.createGroup(PID_NAVMESH_COMMON_GRP, "Navigation meshes");

    for (int pcb_id = PID_NAVMESH_EXPORT_START; pcb_id <= PID_NAVMESH_EXPORT_END; ++pcb_id)
    {
      const int navMeshIdx = pcb_id - PID_NAVMESH_EXPORT_START;
      grp->createCheckBox(pcb_id, String(50, "Export nav mesh #%d", navMeshIdx + 1),
        navMeshProps[navMeshIdx].getBool("export", false));
    }
    grp->createSeparator(0);

    Tab<String> navMeshes(tmpmem);
    for (int navMeshIdx = 0; navMeshIdx < MAX_UI_NAVMESHES; ++navMeshIdx)
      navMeshes.push_back() = String(50, "Nav mesh #%d", navMeshIdx + 1);

    grp->createCombo(PID_NAVMESH_SHOWN_NAVMESH, "Displayed nav mesh:", navMeshes, shownExportedNavMeshIdx);
    grp->createCheckBox(PID_NAVMESH_EXP_SHOW, "Show exported nav mesh", showExportedNavMesh);
    grp->createCheckBox(PID_NAVMESH_COVERS_SHOW, "Show exported covers", showExportedCovers, /*enabled*/ shownExportedNavMeshIdx == 0);
    grp->createCheckBox(PID_NAVMESH_CONTOURS_SHOW, "Show exported contours", showExportedNavMeshContours,
      /*enabled*/ shownExportedNavMeshIdx == 0);
    grp->createCheckBox(PID_NAVMESH_OBSTACLES_SHOW, "Show exported obstacles", showExpotedObstacles);
    grp->createCheckBox(PID_NAVMESH_DISABLE_ZTEST, "Disable ztest for debug geometry", disableZtestForDebugNavMesh);
    grp->createButton(PID_NAVMESH_BUILD, "Build nav mesh!");
  }

  for (int navMeshIdx = 0; navMeshIdx < MAX_UI_NAVMESHES; ++navMeshIdx)
  {
    //---Nav mesh jump links and covers
    const int baseOfs = PID_NAVMESH_START + NM_PARAMS_COUNT * navMeshIdx;

    if (navMeshIdx == 0)
    {
      char groupName[100];
      SNPRINTF(groupName, sizeof(groupName), "Nav mesh #%d jump links / covers", navMeshIdx + 1);
      grp = panel.createGroup(baseOfs + NM_PARAM_JLK_AND_CVRS_GRP, groupName);
      grp->createEditInt(baseOfs + NM_PARAM_JLK_CVRS_EXTRA_CELLS, "extra cells", navMeshProps[navMeshIdx].getInt("jlkExtraCells", 32));
      grp->createCheckBox(baseOfs + NM_PARAM_EMRG_ENABLED, "edge simplification",
        navMeshProps[navMeshIdx].getBool("simplificationEdgeEnabled", true));
      grp->createPoint2(baseOfs + NM_PARAM_EMRG_WALK_PRECISION, "walk check precision, m",
        navMeshProps[navMeshIdx].getPoint2("walkPrecision",
          Point2(navMeshProps[navMeshIdx].getReal("agentRadius", 2.0f), navMeshProps[navMeshIdx].getReal("agentMaxClimb", 1.5f))));
      grp->createEditFloat(baseOfs + NM_PARAM_EMRG_EXTRUDE_LIMIT, "extrude line limit, m",
        navMeshProps[navMeshIdx].getReal("extrudeLimit", 0.3f));
      grp->createEditFloat(baseOfs + NM_PARAM_EMRG_EXTRUDE_MISTAKE, "extrude line error, m",
        navMeshProps[navMeshIdx].getReal("extrudeError", 0.4f));
      grp->createEditFloat(baseOfs + NM_PARAM_EMRG_SAFE_CUT_LIMIT, "line safe cut limit, m",
        navMeshProps[navMeshIdx].getReal("safeCutLimit", 0.5f));
      grp->createEditFloat(baseOfs + NM_PARAM_EMRG_UNSAFE_CUT_LIMIT, "line unsafe cut limit, m",
        navMeshProps[navMeshIdx].getReal("unsafeCutLimit", 0.2f));
      grp->createEditFloat(baseOfs + NM_PARAM_EMRG_UNSAFE_MAX_CUT_SPACE, "max unsafe cut space, m2",
        navMeshProps[navMeshIdx].getReal("maxUnsafeCutSpace", 1.f));

      grp->createSeparator(0);
      grp->createCheckBox(baseOfs + NM_PARAM_JLK_ENABLED, "Export jump links mesh",
        navMeshProps[navMeshIdx].getBool("jumpLinksEnabled", false));
      Tab<String> jlkTypes(tmpmem);
      jlkTypes.push_back() = "Original";
      jlkTypes.push_back() = "(WIP) New 2024";
      int jlkType = navMeshProps[navMeshIdx].getInt("jumpLinksTypeGen", 0);
      const bool jlkEnableOrig = (jlkType == 0);
      const bool jlkEnableNew2024 = (jlkType == 1);
      grp->createCombo(baseOfs + NM_PARAM_JLK_TYPEGEN, "Jump links generation:", jlkTypes, jlkType);
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_JUMPOFF_MIN_HEIGHT, "jumpoff min height, m",
        navMeshProps[navMeshIdx].getReal("jumpLinksJumpoffMinHeight", 1.0f), 2, jlkEnableNew2024);
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_JUMPOFF_MAX_HEIGHT, "jumpoff max height, m",
        navMeshProps[navMeshIdx].getReal("jumpLinksJumpoffMaxHeight", 4.0f), 2, jlkEnableNew2024);
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_JUMPOFF_MIN_XZ_LENGTH, "jumpoff min XZ length, m",
        navMeshProps[navMeshIdx].getReal("jumpLinksJumpoffMinLinkLength", 0.3f), 2, jlkEnableNew2024);
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_EDGE_MERGE_ANGLE, "jlk merge angle, deg",
        navMeshProps[navMeshIdx].getReal("jumpLinksEdgeMergeAngle", 10.0f), 2, jlkEnableNew2024);
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_EDGE_MERGE_DIST, "jlk merge dist, m",
        navMeshProps[navMeshIdx].getReal("jumpLinksEdgeMergeDist", 0.1f), 2, jlkEnableNew2024);
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_JUMP_OVER_HEIGHT, "jump over height, m",
        navMeshProps[navMeshIdx].getReal("jumpLinksHeight", 2.0f));
      const float typoDefJumpLength = navMeshProps[navMeshIdx].getReal("jumpLinksLenght", 2.5f);
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_JUMP_LENGTH, "jump length, m",
        navMeshProps[navMeshIdx].getReal("jumpLinksLength", typoDefJumpLength));
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_MIN_WIDTH, "min width, m", navMeshProps[navMeshIdx].getReal("jumpLinksWidth", 1.0f));
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_AGENT_HEIGHT, "agent height, m",
        navMeshProps[navMeshIdx].getReal("jumpLinksAgentHeight", 1.5f));
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_AGENT_MIN_SPACE, "agent min space, m",
        navMeshProps[navMeshIdx].getReal("jumpLinksAgentMinSpace", 1.0f));
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_DH_THRESHOLD, "delta height threshold, m",
        navMeshProps[navMeshIdx].getReal("jumpLinksDeltaHeightTreshold", 0.5f), 2, jlkEnableOrig);
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_MAX_OBSTRUCTION_ANGLE, "max obstruction angle, deg",
        navMeshProps[navMeshIdx].getReal("jumpLinksMaxObstructionAngle", 25.0f), 2, jlkEnableOrig);
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_MERGE_ANGLE, "merge delta angle, deg",
        navMeshProps[navMeshIdx].getReal("jumpLinksMergeAngle", 15.0f), 2, jlkEnableOrig);
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_MERGE_DIST, "max angle between edges, deg",
        navMeshProps[navMeshIdx].getReal("jumpLinksMergeDist", 5.0f), 2, jlkEnableOrig);
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_CPLX_THRESHOLD, "complex link threshold",
        navMeshProps[navMeshIdx].getReal("complexJumpTheshold", 0.0f));
      grp->createCheckBox(baseOfs + NM_PARAM_JLK_CROSS_OBSTACLES_WITH_JUMPLINKS, "cross obstacles with jlinks",
        navMeshProps[navMeshIdx].getBool("crossObstaclesWithJumplinks", false));
      grp->createCheckBox(baseOfs + NM_PARAM_JLK_GENERATE_CUSTOM_JUMPLINKS, "enable custom jumplinks",
        navMeshProps[navMeshIdx].getBool("enableCustomJumplinks", false));
      grp->createEditFloat(baseOfs + NM_PARAM_JLK_EDGE_MERGE_DIST_V1, "jlk merge dist (old), m",
        navMeshProps[navMeshIdx].getReal("jumpLinksEdgeMergeDistV1", 99999.f), 2, !jlkEnableNew2024);

      grp->createSeparator(0);
      grp->createCheckBox(baseOfs + NM_PARAM_CVRS_ENABLED, "Export covers", navMeshProps[navMeshIdx].getBool("coversEnabled", false));

      Tab<String> coversType(tmpmem);
      coversType.push_back() = "Standard";
      coversType.push_back() = "Alternative 1";
      int covType = navMeshProps[navMeshIdx].getInt("coversTypeGen", 0);
      const bool covEnable1 = (covType == 0);
      grp->createCombo(baseOfs + NM_PARAM_CVRS_TYPEGEN, "Covers generation:", coversType, covType);
      grp->createEditFloat(baseOfs + NM_PARAM_CVRS_MAX_HEIGHT, "max height, m",
        navMeshProps[navMeshIdx].getReal("coversMaxHeight", 2.f), 2, covEnable1);
      grp->createEditFloat(baseOfs + NM_PARAM_CVRS_MIN_HEIGHT, "min height, m",
        navMeshProps[navMeshIdx].getReal("coversMinHeight", 1.f), 2, covEnable1);
      grp->createEditFloat(baseOfs + NM_PARAM_CVRS_MIN_WIDTH, "min width, m", navMeshProps[navMeshIdx].getReal("coversWidth", 1.f), 2,
        covEnable1);
      grp->createEditFloat(baseOfs + NM_PARAM_CVRS_MIN_DEPTH, "min depth, m", navMeshProps[navMeshIdx].getReal("coversDepth", 1.f), 2,
        covEnable1);
      grp->createEditFloat(baseOfs + NM_PARAM_CVRS_SHOOT_WINDOW_HEIGHT, "min window for shoot, m",
        navMeshProps[navMeshIdx].getReal("coversShootWindow", 0.4f), 2, covEnable1);
      grp->createEditFloat(baseOfs + NM_PARAM_CVRS_COLLISION_TARNSP_THRESHOLD, "collision transparent, 0 - 1",
        navMeshProps[navMeshIdx].getReal("coversTransparent", 0.8f), 2, covEnable1);
      grp->createEditFloat(baseOfs + NM_PARAM_CVRS_MERGE_DIST, "covers merge distance, m",
        navMeshProps[navMeshIdx].getReal("coversMergeDist", 0.3f), 2, covEnable1);
      grp->createEditFloat(baseOfs + NM_PARAM_CVRS_DH_THRESHOLD, "delta height threshold, m",
        navMeshProps[navMeshIdx].getReal("coversDeltaHeightTreshold", 0.2f), 2, covEnable1);
      grp->createEditFloat(baseOfs + NM_PARAM_CVRS_SHOOT_DEPTH_CHECK, "checked shoot depth, m",
        navMeshProps[navMeshIdx].getReal("coversShootDepth", 3.f), 2, covEnable1);
      grp->createEditFloat(baseOfs + NM_PARAM_CVRS_SHOOT_HEIGHT, "shoot height, m",
        navMeshProps[navMeshIdx].getReal("coversShootHeight", 1.5f), 2, covEnable1);
      grp->createEditFloat(baseOfs + NM_PARAM_CVRS_OPENING_TRESHOLD, "open air treshold, 0-1",
        navMeshProps[navMeshIdx].getReal("openingTreshold", 0.9f));
      grp->createEditFloat(baseOfs + NM_PARAM_CVRS_BOX_OFFSET, "visible box offset, m",
        navMeshProps[navMeshIdx].getReal("visibleBoxOffset", 1.f));

      // NOTE: uncomment only for tests
      // grp->createEditFloat(baseOfs + NM_PARAM_CVRS_TEST_X, "test X", navMeshProps[navMeshIdx].getReal("coversTestX", 0.0f));
      // grp->createEditFloat(baseOfs + NM_PARAM_CVRS_TEST_Y, "test Y", navMeshProps[navMeshIdx].getReal("coversTestY", 0.0f));
      // grp->createEditFloat(baseOfs + NM_PARAM_CVRS_TEST_Z, "test Z", navMeshProps[navMeshIdx].getReal("coversTestZ", 0.0f));

      panel.setBool(baseOfs + NM_PARAM_JLK_AND_CVRS_GRP, true);
    }


    //---Nav mesh
    {
      char groupName[100];
      SNPRINTF(groupName, sizeof(groupName), "Nav mesh #%d setup", navMeshIdx + 1);
      grp = panel.createGroup(baseOfs + NM_PARAM_GRP, groupName);
      grp->createCheckBox(baseOfs + NM_PARAM_EXP, String(50, "Export nav mesh #%d", navMeshIdx + 1),
        navMeshProps[navMeshIdx].getBool("export", false));
      grp->createEditBox(baseOfs + NM_PARAM_KIND, "Nav mesh kind", navMeshProps[navMeshIdx].getStr("kind", ""));

      PropPanel::ContainerPropertyControl *rg = grp->createRadioGroup(baseOfs + NM_PARAM_AREATYPE, "Navigation area");
      rg->createRadio(baseOfs + NM_PARAM_AREATYPE_MAIN, "Full main HMAP area");
      rg->createRadio(baseOfs + NM_PARAM_AREATYPE_DET, "Detailed HMAP area");
      rg->createRadio(baseOfs + NM_PARAM_AREATYPE_RECT, "Explicit rectangle");
      rg->createRadio(baseOfs + NM_PARAM_AREATYPE_POLY, "Specified area");

      if (navMeshProps[navMeshIdx].getInt("navArea", 0) <= NM_PARAM_AREATYPE_POLY - NM_PARAM_AREATYPE_MAIN)
        grp->setInt(baseOfs + NM_PARAM_AREATYPE, baseOfs + NM_PARAM_AREATYPE_MAIN + navMeshProps[navMeshIdx].getInt("navArea", 0));
      else
        grp->setInt(baseOfs + NM_PARAM_AREATYPE, baseOfs + NM_PARAM_AREATYPE_MAIN);

      PropPanel::ContainerPropertyControl *areatypeInputContainer =
        grp->createContainer(baseOfs + NM_PARAM_AREAS_INPUT_CONTAINER, true, _pxScaled(2));
      navmeshAreasProcessing[navMeshIdx].setPropPanel(areatypeInputContainer);
      if (panel.getInt(baseOfs + NM_PARAM_AREATYPE) == baseOfs + NM_PARAM_AREATYPE_RECT)
      {
        fillAreatypeRectPanel(*areatypeInputContainer, navMeshIdx, baseOfs);
      }
      else if (panel.getInt(baseOfs + NM_PARAM_AREATYPE) == baseOfs + NM_PARAM_AREATYPE_POLY)
      {
        navmeshAreasProcessing[navMeshIdx].fillNavmeshAreasPanel();
      }

      grp->createSeparator(0);

      PropPanel::ContainerPropertyControl *rgType = grp->createRadioGroup(baseOfs + NM_PARAM_EXPORT_TYPE, "Navmesh type");
      rgType->createRadio(baseOfs + NM_PARAM_EXPORT_TYPE_WATER, "Water navigation");
      rgType->createRadio(baseOfs + NM_PARAM_EXPORT_TYPE_SPLINES, "Splines navigation");
      rgType->createRadio(baseOfs + NM_PARAM_EXPORT_TYPE_HEIGHT_FROM_ABOVE, "Height from above");
      rgType->createRadio(baseOfs + NM_PARAM_EXPORT_TYPE_GEOMETRY, "Detailed geometry");
      rgType->createRadio(baseOfs + NM_PARAM_EXPORT_TYPE_WATER_AND_GEOMETRY, "Water and geometry");

      NavmeshExportType navmeshExportType =
        navmesh_export_type_name_to_enum(navMeshProps[navMeshIdx].getStr("navmeshExportType", "invalid"));
      if (navmeshExportType != NavmeshExportType::INVALID)
        grp->setInt(baseOfs + NM_PARAM_EXPORT_TYPE, baseOfs + NM_PARAM_EXPORT_TYPE_WATER + int(navmeshExportType));

      grp->createSeparator(0);

      grp->createCheckBox(baseOfs + NM_PARAM_WATER, "Use water surface", navMeshProps[navMeshIdx].getBool("hasWater", false));
      panel.setEnabledById(baseOfs + NM_PARAM_WATER, navmeshExportType != NavmeshExportType::WATER);

      grp->createEditFloat(baseOfs + NM_PARAM_WATER_LEV, "Water surface level, m", navMeshProps[navMeshIdx].getReal("waterLev", 0.0f));
      grp->createSeparator(0);

      grp->createEditFloat(baseOfs + NM_PARAM_CELL_DXZ, "Cell size (XZ), m", navMeshProps[navMeshIdx].getReal("cellSize", 2.0f));
      grp->createEditFloat(baseOfs + NM_PARAM_CELL_DY, "Cell size (Y), m", navMeshProps[navMeshIdx].getReal("cellHeight", 0.125f));
      grp->createEditFloat(baseOfs + NM_PARAM_TRACE_STEP, "Trace step (XZ), m",
        navMeshProps[navMeshIdx].getReal("traceStep", navMeshProps[navMeshIdx].getReal("cellSize", 2.f)));
      grp->createEditFloat(baseOfs + NM_PARAM_CELL_A_HT, "Agent height, m", navMeshProps[navMeshIdx].getReal("agentHeight", 3.0f));
      grp->createEditFloat(baseOfs + NM_PARAM_CELL_A_RAD, "Agent radius, m", navMeshProps[navMeshIdx].getReal("agentRadius", 2.0f));
      grp->createEditFloat(baseOfs + NM_PARAM_CELL_A_SLOPE, "Max agent slope, deg",
        navMeshProps[navMeshIdx].getReal("agentMaxSlope", 30.0f));
      grp->createEditFloat(baseOfs + NM_PARAM_CELL_A_CLIMB, "Max agent climb, m",
        navMeshProps[navMeshIdx].getReal("agentMaxClimb", 1.5f));
      grp->createEditFloat(baseOfs + NM_PARAM_CELL_A_WALKABLE_CLIMB, "Walkable climb, m", // this is its actual effect
        navMeshProps[navMeshIdx].getReal("agentClimbAfterGluingMeshes", 0.1f));
      grp->createEditFloat(baseOfs + NM_PARAM_CELL_EDGE_MAX_LEN, "Edge max len, m",
        navMeshProps[navMeshIdx].getReal("edgeMaxLen", 128.0f));
      grp->createEditFloat(baseOfs + NM_PARAM_CELL_EDGE_MAX_ERR, "Edge max error, m",
        navMeshProps[navMeshIdx].getReal("edgeMaxError", 1.5f));
      grp->createEditFloat(baseOfs + NM_PARAM_CELL_REG_MIN_SZ, "Region min size, m2",
        navMeshProps[navMeshIdx].getReal("regionMinSize", 9.0f));
      grp->createEditFloat(baseOfs + NM_PARAM_CELL_REG_MERGE_SZ, "Region merge size, m2",
        navMeshProps[navMeshIdx].getReal("regionMergeSize", 100.0f));
      grp->createEditInt(baseOfs + NM_PARAM_CELL_VERTS_PER_POLY, "Vertices per polygon",
        navMeshProps[navMeshIdx].getInt("vertsPerPoly", 3));
      grp->createEditFloat(baseOfs + NM_PARAM_CELL_DET_SAMPLE, "Detail sample dist, m",
        navMeshProps[navMeshIdx].getReal("detailSampleDist", 3.0f));
      grp->createEditFloat(baseOfs + NM_PARAM_CELL_DET_MAX_ERR, "Detail sample max error, m",
        navMeshProps[navMeshIdx].getReal("detailSampleMaxError", 0.25f));
      grp->createEditFloat(baseOfs + NM_PARAM_CROSSING_WATER_DEPTH, "Crossing water depth, m",
        navMeshProps[navMeshIdx].getReal("crossingWaterDepth", 0.f));
      grp->createCheckBox(baseOfs + NM_PARAM_PREFAB_COLLISION, "Use prefab collision",
        navMeshProps[navMeshIdx].getBool("usePrefabCollision", true));
      grp->createSeparator(0);
      Tab<String> typesNavMesh(tmpmem);
      typesNavMesh.push_back() = "Simple";
      typesNavMesh.push_back() = "Tiled";
      typesNavMesh.push_back() = "TileCached";
      int navMeshType = navMeshProps[navMeshIdx].getInt("navMeshType",
        navMeshProps[navMeshIdx].getBool("tiled", false) ? pathfinder::NMT_TILED : pathfinder::NMT_SIMPLE);
      grp->createCombo(baseOfs + NM_PARAM_TYPE, "Nav mesh type:", typesNavMesh, navMeshType);
      grp->createEditInt(baseOfs + NM_PARAM_TILE_SZ, "Tile size (XZ), cells", navMeshProps[navMeshIdx].getInt("tileCells", 128));
      grp->createEditFloat(baseOfs + NM_PARAM_BUCKET_SZ, "Bucket size, m", navMeshProps[navMeshIdx].getReal("bucketSize", 200.0f));
      panel.setEnabledById(baseOfs + NM_PARAM_BUCKET_SZ, navMeshType == pathfinder::NMT_TILECACHED);
      grp->createSeparator(0);
      grp->createCheckBox(baseOfs + NM_PARAM_DROP_RESULT_ON_COLLISION, "Drop navmesh on collision",
        navMeshProps[navMeshIdx].getBool("dropNavmeshOnCollision", false));
      grp->createEditFloat(baseOfs + NM_PARAM_DROP_RESULT_ON_COLLISION_TRACE_OFFSET, "Trace offset, m", 0.3f, 2,
        navMeshProps[navMeshIdx].getBool("dropNavmeshOnCollision", false));
      grp->createEditFloat(baseOfs + NM_PARAM_DROP_RESULT_ON_COLLISION_TRACE_LEN, "Trace length, m", 0.3f, 2,
        navMeshProps[navMeshIdx].getBool("dropNavmeshOnCollision", false));
      grp->createSeparator(0);
      grp->createEditInt(baseOfs + NM_PARAM_COMPRESS_RATIO, "Compress ratio", 19, true);
      grp->createIndent();
      panel.setBool(baseOfs + NM_PARAM_GRP, true);
    }
  }

  //---Snow
  if (isSnowAvailable())
  {
    grp = panel.createGroup(PID_SNOW_GRP, "Snow");
    grp->createCheckBox(PID_SNOW_PREVIEW, "Dynamic snow preview", snowDynPreview);
    grp->createCheckBox(PID_SNOW_SPHERE_PREVIEW, "Snow sources preview", snowSpherePreview);
    grp->createTrackFloat(PID_DYN_SNOW, "Dynamic snow coverage", dynSnowValue, 0, 1, 0.001);
    grp->createButton(PID_COPY_SNOW, "Copy Dyn to Amb");
    grp->createTrackFloat(PID_AMB_SNOW, "Ambient snow coverage", ambSnowValue, 0, 1, 0.001);
    grp->createStatic(0, "Color: Environment/Shader global vars");

    panel.setEnabledById(PID_SNOW_SPHERE_PREVIEW, snowDynPreview);
    panel.setEnabledById(PID_DYN_SNOW, snowDynPreview);
    panel.setEnabledById(PID_COPY_SNOW, snowDynPreview);
    panel.setBool(PID_SNOW_GRP, true);
  }

  panel.restoreFillAutoResize();
  panel.loadState(mainPanelState);
}


void HmapLandPlugin::updateLightGroup(PropPanel::ContainerPropertyControl &panel)
{
  panel.setColor(PID_SUN_LIGHT_COLOR, ldrLight.sunColor);
  panel.setFloat(PID_SUN_LIGHT_POWER, ldrLight.sunPower);
  panel.setColor(PID_SPECULAR_COLOR, ldrLight.specularColor);
  panel.setFloat(PID_SPECULAR_MUL, ldrLight.specularMul);
  panel.setFloat(PID_SPECULAR_POWER, ldrLight.specularPower);
  panel.setColor(PID_SKY_LIGHT_COLOR, ldrLight.skyColor);
  panel.setFloat(PID_SKY_LIGHT_POWER, ldrLight.skyPower * 4);
  panel.setFloat(PID_SUN_AZIMUTH, (RadToDeg(HALFPI - sunAzimuth)));
  panel.setFloat(PID_SUN_ZENITH, (RadToDeg(sunZenith)));
}

void HmapLandPlugin::fillAreatypeRectPanel(PropPanel::ContainerPropertyControl &panel, int navmesh_idx, int base_ofs)
{
  panel.createPoint2(base_ofs + NM_PARAM_AREATYPE_RECT0, "  rectangle min pos",
    navMeshProps[navmesh_idx].getPoint2("rect0", Point2(0, 0)));
  panel.createPoint2(base_ofs + NM_PARAM_AREATYPE_RECT1, "  rectangle max pos",
    navMeshProps[navmesh_idx].getPoint2("rect1", Point2(1000, 1000)));
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

void HmapLandPlugin::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{

  if (pcb_id >= PID_SCRIPT_PARAM_START && pcb_id <= PID_LAST_SCRIPT_PARAM)
  {
    for (int i = 0; i < colorGenParams.size(); ++i)
      colorGenParams[i]->onPPChange(pcb_id, *panel);
    for (int i = 0; i < genLayers.size(); ++i)
      if (genLayers[i]->onPPChangeEx(pcb_id, *panel))
        break;
  }

  if (pcb_id >= PID_GRASS_PARAM_START && pcb_id <= PID_LAST_GRASS_PARAM)
  {
    for (int i = 0; i < grassLayers.size(); ++i)
      if (grassLayers[i]->onPPChangeEx(pcb_id, *panel))
      {
        HmapLandPlugin::grassService->resetLayersVB();
        break;
      }
  }

  // if (!brushDlg->isDialogVisible() && !brushDlg->isDialogClosed())
  //   return;

  // PropPanel::ContainerPropertyControl &dlgPanel = brushDlg->getPropPanel();
  // PropPanel::ContainerPropertyControl &plugPanel = *propPanel->getPanel();
  // PropPanel::ContainerPropertyControl &panel = brushDlg->isDialogVisible() ? dlgPanel : plugPanel;


  switch (pcb_id)
  {
    case PID_SUN_LIGHT_COLOR:
      ldrLight.sunColor = panel->getColor(pcb_id);
      updateRendererLighting();
      return;

    case PID_SKY_LIGHT_COLOR:
      ldrLight.skyColor = panel->getColor(pcb_id);
      updateRendererLighting();
      return;

    case PID_SPECULAR_COLOR:
      ldrLight.specularColor = panel->getColor(pcb_id);
      updateRendererLighting();
      return;

    case PID_SUN_LIGHT_POWER:
      ldrLight.sunPower = panel->getFloat(pcb_id);
      updateRendererLighting();
      return;

    case PID_SKY_LIGHT_POWER:
      ldrLight.skyPower = panel->getFloat(pcb_id) / 4;
      updateRendererLighting();
      return;

    case PID_SPECULAR_MUL:
      ldrLight.specularMul = panel->getFloat(pcb_id);
      updateRendererLighting();
      return;

    case PID_SPECULAR_POWER:
      ldrLight.specularPower = panel->getFloat(pcb_id);
      updateRendererLighting();
      return;

    case PID_GET_SUN_SETTINGS_AUTOMAT:
      syncLight = panel->getBool(pcb_id);
      panel->setEnabledById(PID_GET_SUN_SETTINGS, !syncLight);

      panel->setEnabledById(PID_SUN_LIGHT_COLOR, !syncLight);
      panel->setEnabledById(PID_SUN_LIGHT_POWER, !syncLight);
      panel->setEnabledById(PID_SKY_LIGHT_COLOR, !syncLight);
      panel->setEnabledById(PID_SKY_LIGHT_POWER, !syncLight);
      panel->setEnabledById(PID_SPECULAR_COLOR, !syncLight);

      panel->setEnabledById(PID_SUN_AZIMUTH, !syncLight && !syncDirLight);
      panel->setEnabledById(PID_SUN_ZENITH, !syncLight && !syncDirLight);
      ldrLight.specularMul = panel->getFloat(PID_SPECULAR_MUL);
      ldrLight.specularPower = panel->getFloat(PID_SPECULAR_POWER);

      if (!syncLight)
      {
        ldrLight.sunColor = panel->getColor(PID_SUN_LIGHT_COLOR);
        ldrLight.skyColor = panel->getColor(PID_SKY_LIGHT_COLOR);
        ldrLight.specularColor = panel->getColor(PID_SPECULAR_COLOR);
        ldrLight.sunPower = panel->getFloat(PID_SUN_LIGHT_POWER);
        ldrLight.skyPower = panel->getFloat(PID_SKY_LIGHT_POWER) / 4;
        if (!syncDirLight)
        {
          sunAzimuth = HALFPI - DegToRad(panel->getFloat(PID_SUN_AZIMUTH));
          sunZenith = DegToRad(panel->getFloat(PID_SUN_ZENITH));
        }
      }
      else
      {
        getAllSunSettings();
        updateLightGroup(*panel);
      }

      updateRendererLighting();

      return;

    case PID_GET_SUN_DIR_SETTINGS_AUTOMAT:
      syncDirLight = panel->getBool(pcb_id);
      panel->setEnabledById(PID_SUN_AZIMUTH, !syncLight && !syncDirLight);
      panel->setEnabledById(PID_SUN_ZENITH, !syncLight && !syncDirLight);

      if (!syncLight && !syncDirLight)
      {
        sunAzimuth = HALFPI - DegToRad(panel->getFloat(PID_SUN_AZIMUTH));
        sunZenith = DegToRad(panel->getFloat(PID_SUN_ZENITH));
      }
      else
      {
        getDirectionSunSettings();
        panel->setFloat(PID_SUN_AZIMUTH, (RadToDeg(HALFPI - sunAzimuth)));
        panel->setFloat(PID_SUN_ZENITH, (RadToDeg(sunZenith)));
      }

      updateRendererLighting();

      return;

    case PID_RI_MAX_CELL_SIZE:
      riMaxCellSz = panel->getFloat(pcb_id);
      lcMgr->reinitRIGen();
      break;
    case PID_RI_MAX_GEN_LAYER_CELL_DIVISOR: riMaxGenLayerCellDivisor = panel->getInt(pcb_id); break;

    case PID_MESH_PREVIEW_DISTANCE:
      meshPreviewDistance = panel->getFloat(pcb_id);
      resetRenderer();
      return;

    case PID_MESH_CELLS:
      meshCells = panel->getInt(pcb_id);
      meshCells = get_closest_pow2(meshCells);
      meshCells = clamp(meshCells, 1, 256);
      return;

    case PID_MESH_ERROR_THRESHOLD: meshErrorThreshold = panel->getFloat(pcb_id); return;

    case PID_NUM_MESH_VERTICES: numMeshVertices = panel->getInt(pcb_id); return;

    case PID_LOD1_DENSITY: lod1TrisDensity = panel->getInt(pcb_id); return;

    case PID_IMPORTANCE_MASK_SCALE: importanceMaskScale = panel->getFloat(pcb_id); return;

    case PID_WATER_SURF: hasWaterSurface = panel->getBool(pcb_id); return;
    case PID_WATER_SURF_LEV: waterSurfaceLevel = panel->getFloat(pcb_id); return;
    case PID_MIN_UNDERWATER_BOTOM_DEPTH: minUnderwaterBottomDepth = panel->getFloat(pcb_id); return;
    case PID_WATER_OCEAN: hasWorldOcean = panel->getBool(pcb_id); return;
    case PID_WATER_OCEAN_EXPAND: worldOceanExpand = panel->getFloat(pcb_id); return;
    case PID_WATER_OCEAN_SL_TOL: worldOceanShorelineTolerance = panel->getFloat(pcb_id); return;

    case PID_DEBUG_CELLS:
      debugLmeshCells = panel->getBool(pcb_id);
      if (landMeshRenderer)
        landMeshRenderer->setCellsDebug(debugLmeshCells);
      break;

    case PID_HM_EXPORT_TYPE:
      exportType = panel->getInt(pcb_id);
      genHmap->altCollider = (exportType == EXPORT_LANDMESH) ? this : NULL;
      onLandSizeChanged();
      onWholeLandChanged();
      resetRenderer();
      return;

    case PID_HMAP_UPSCALE_ALGO:
      if (hmlService)
        hmlService->setHmapUpscaleAlgo(panel->getInt(pcb_id));
      return;

    case PID_HM_LOFT_BELOW_ALL:
      geomLoftBelowAll = panel->getBool(pcb_id);
      if (hmlService)
        hmlService->invalidateClipmap(false);
      DAGORED2->invalidateViewportCache();
      break;

    case PID_APPLY_HEIGHTBAKE: applyHeightBakeSplines = panel->getBool(pcb_id); break;
    case PID_APPLY_HEIGHTBAKE_ON_EDIT: applyHeightBakeSplinesOnEdit = panel->getBool(pcb_id); break;

    case PID_SNOW_PREVIEW:
    case PID_DYN_SNOW:
    case PID_SNOW_SPHERE_PREVIEW:
      snowDynPreview = panel->getBool(PID_SNOW_PREVIEW);
      snowSpherePreview = panel->getBool(PID_SNOW_SPHERE_PREVIEW);
      dynSnowValue = panel->getFloat(PID_DYN_SNOW);

      if (isSnowAvailable())
      {
        int p_val = (snowDynPreview && snowSpherePreview) ? 2 : (snowDynPreview ? 1 : 0);

        dagGeom->shaderGlobalSetInt(snowPrevievSVId, p_val);
        dagGeom->shaderGlobalSetReal(snowValSVId, dynSnowValue);
        if (snowSpherePreview)
          updateSnowSources();
      }

      panel->setEnabledById(PID_SNOW_SPHERE_PREVIEW, snowDynPreview);
      panel->setEnabledById(PID_DYN_SNOW, snowDynPreview);
      panel->setEnabledById(PID_COPY_SNOW, snowDynPreview);
      break;

    case PID_AMB_SNOW: ambSnowValue = panel->getFloat(PID_AMB_SNOW); break;

    case PID_GRID_H2_CELL_SIZE:
      if (detDivisor != panel->getFloat(pcb_id))
        panel->setEnabledById(PID_GRID_H2_APPLY, true);
      break;
    case PID_GRID_H2_BBOX_OFS:
      if (detRect[0] != panel->getPoint2(pcb_id))
        panel->setEnabledById(PID_GRID_H2_APPLY, true);
      break;
    case PID_GRID_H2_BBOX_SZ:
      if (detRect[1] - detRect[0] != panel->getPoint2(pcb_id))
        panel->setEnabledById(PID_GRID_H2_APPLY, true);
      break;

    case PID_HM2_DISPLACEMENT_Q:
      render.hm2displacementQ = panel->getInt(pcb_id);
      updateHmap2Tesselation();
      break;

    case PID_NAVMESH_SHOWN_NAVMESH:
      shownExportedNavMeshIdx = panel->getInt(pcb_id);
      panel->setEnabledById(PID_NAVMESH_COVERS_SHOW, shownExportedNavMeshIdx == 0);
      panel->setEnabledById(PID_NAVMESH_CONTOURS_SHOW, shownExportedNavMeshIdx == 0);
      clearNavMesh();
      break;

    case PID_NAVMESH_EXP_SHOW: showExportedNavMesh = panel->getBool(pcb_id); break;
    case PID_NAVMESH_COVERS_SHOW: showExportedCovers = panel->getBool(pcb_id); break;
    case PID_NAVMESH_CONTOURS_SHOW: showExportedNavMeshContours = panel->getBool(pcb_id); break;
    case PID_NAVMESH_OBSTACLES_SHOW: showExpotedObstacles = panel->getBool(pcb_id); break;
    case PID_NAVMESH_DISABLE_ZTEST: disableZtestForDebugNavMesh = panel->getBool(pcb_id); break;
  }

  if (pcb_id >= PID_NAVMESH_EXPORT_START && pcb_id <= PID_NAVMESH_EXPORT_END)
  {
    const int navMeshIdx = pcb_id - PID_NAVMESH_EXPORT_START;
    navMeshProps[navMeshIdx].setBool("export", panel->getBool(pcb_id));
    panel->setBool(PID_NAVMESH_START + navMeshIdx * NM_PARAMS_COUNT + NM_PARAM_EXP, panel->getBool(pcb_id));
  }

  if ((pcb_id == PID_NAVMESH_EXP_SHOW || pcb_id == PID_NAVMESH_COVERS_SHOW || pcb_id == PID_NAVMESH_CONTOURS_SHOW ||
        pcb_id == PID_NAVMESH_OBSTACLES_SHOW) &&
      (panel->getBool(pcb_id) || !(showExportedNavMesh || showExportedCovers || showExportedNavMeshContours || showExpotedObstacles)))
    if (IGenEditorPlugin *p = DAGORED2->getPluginByName("_navmesh"))
      p->setVisible(panel->getBool(pcb_id));

  if (pcb_id >= PID_NAVMESH_START && pcb_id <= PID_NAVMESH_END)
  {
    const int navMeshIdx = (pcb_id - PID_NAVMESH_START) / NM_PARAMS_COUNT;
    const int navMeshParam = (pcb_id - PID_NAVMESH_START) % NM_PARAMS_COUNT;
    const int baseOfs = PID_NAVMESH_START + NM_PARAMS_COUNT * navMeshIdx;
    switch (navMeshParam)
    {
      case NM_PARAM_EXP:
        navMeshProps[navMeshIdx].setBool("export", panel->getBool(pcb_id));
        panel->setBool(PID_NAVMESH_EXPORT_START + navMeshIdx, panel->getBool(pcb_id));
        break;
      case NM_PARAM_KIND: navMeshProps[navMeshIdx].setStr("kind", panel->getText(pcb_id)); break;
      case NM_PARAM_CELL_DXZ: navMeshProps[navMeshIdx].setReal("cellSize", panel->getFloat(pcb_id)); break;
      case NM_PARAM_TRACE_STEP: navMeshProps[navMeshIdx].setReal("traceStep", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CELL_DY: navMeshProps[navMeshIdx].setReal("cellHeight", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CELL_A_HT: navMeshProps[navMeshIdx].setReal("agentHeight", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CELL_A_RAD: navMeshProps[navMeshIdx].setReal("agentRadius", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CELL_A_SLOPE: navMeshProps[navMeshIdx].setReal("agentMaxSlope", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CELL_A_CLIMB: navMeshProps[navMeshIdx].setReal("agentMaxClimb", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CELL_A_WALKABLE_CLIMB:
        navMeshProps[navMeshIdx].setReal("agentClimbAfterGluingMeshes", panel->getFloat(pcb_id));
        break;
      case NM_PARAM_CELL_EDGE_MAX_LEN: navMeshProps[navMeshIdx].setReal("edgeMaxLen", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CELL_EDGE_MAX_ERR: navMeshProps[navMeshIdx].setReal("edgeMaxError", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CELL_REG_MIN_SZ: navMeshProps[navMeshIdx].setReal("regionMinSize", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CELL_REG_MERGE_SZ: navMeshProps[navMeshIdx].setReal("regionMergeSize", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CELL_VERTS_PER_POLY: navMeshProps[navMeshIdx].setInt("vertsPerPoly", panel->getInt(pcb_id)); break;
      case NM_PARAM_CELL_DET_SAMPLE: navMeshProps[navMeshIdx].setReal("detailSampleDist", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CELL_DET_MAX_ERR: navMeshProps[navMeshIdx].setReal("detailSampleMaxError", panel->getFloat(pcb_id)); break;
      case NM_PARAM_AREATYPE:
      {
        int areaType = panel->getInt(baseOfs + NM_PARAM_AREATYPE) - (baseOfs + NM_PARAM_AREATYPE_MAIN);
        if (areaType != navMeshProps[navMeshIdx].getInt("navArea", 0))
        {
          navMeshProps[navMeshIdx].setInt("navArea", areaType);
        }

        if (areaType >= NM_PARAM_AREATYPE_RECT - NM_PARAM_AREATYPE_MAIN && areaType <= NM_PARAM_AREATYPE_POLY - NM_PARAM_AREATYPE_MAIN)
        {
          panel->removeById(baseOfs + NM_PARAM_AREAS_INPUT_CONTAINER);
          PropPanel::ContainerPropertyControl *grp = panel->getById(baseOfs + NM_PARAM_GRP)->getContainer();
          PropPanel::ContainerPropertyControl *areatypeInputContainer =
            grp->createContainer(baseOfs + NM_PARAM_AREAS_INPUT_CONTAINER, true, _pxScaled(2));
          if (areaType == NM_PARAM_AREATYPE_RECT - NM_PARAM_AREATYPE_MAIN)
          {
            fillAreatypeRectPanel(*areatypeInputContainer, navMeshIdx, baseOfs);
          }
          else
          {
            navmeshAreasProcessing[navMeshIdx].fillNavmeshAreasPanel();
          }
          grp->moveById(baseOfs + NM_PARAM_AREAS_INPUT_CONTAINER, baseOfs + NM_PARAM_AREATYPE, true);
        }

        break;
      }
      case NM_PARAM_AREATYPE_RECT0: navMeshProps[navMeshIdx].setPoint2("rect0", panel->getPoint2(pcb_id)); break;
      case NM_PARAM_AREATYPE_RECT1: navMeshProps[navMeshIdx].setPoint2("rect1", panel->getPoint2(pcb_id)); break;
      case NM_PARAM_WATER: navMeshProps[navMeshIdx].setBool("hasWater", panel->getBool(pcb_id)); break;
      case NM_PARAM_WATER_LEV: navMeshProps[navMeshIdx].setReal("waterLev", panel->getFloat(pcb_id)); break;
      case NM_PARAM_TYPE:
        navMeshProps[navMeshIdx].setInt("navMeshType", panel->getInt(pcb_id));
        panel->setEnabledById(baseOfs + NM_PARAM_BUCKET_SZ, panel->getInt(pcb_id) == pathfinder::NMT_TILECACHED);
        break;
      case NM_PARAM_PREFAB_COLLISION: navMeshProps[navMeshIdx].setBool("usePrefabCollision", panel->getBool(pcb_id)); break;
      case NM_PARAM_TILE_SZ: navMeshProps[navMeshIdx].setInt("tileCells", panel->getInt(pcb_id)); break;
      case NM_PARAM_BUCKET_SZ: navMeshProps[navMeshIdx].setReal("bucketSize", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CROSSING_WATER_DEPTH: navMeshProps[navMeshIdx].setReal("crossingWaterDepth", panel->getFloat(pcb_id)); break;

      case NM_PARAM_JLK_CVRS_EXTRA_CELLS: navMeshProps[navMeshIdx].setInt("jlkExtraCells", panel->getInt(pcb_id)); break;
      case NM_PARAM_EMRG_ENABLED: navMeshProps[navMeshIdx].setBool("simplificationEdgeEnabled", panel->getBool(pcb_id)); break;
      case NM_PARAM_EMRG_WALK_PRECISION: navMeshProps[navMeshIdx].setPoint2("walkPrecision", panel->getPoint2(pcb_id)); break;
      case NM_PARAM_EMRG_EXTRUDE_MISTAKE: navMeshProps[navMeshIdx].setReal("extrudeError", panel->getFloat(pcb_id)); break;
      case NM_PARAM_EMRG_EXTRUDE_LIMIT: navMeshProps[navMeshIdx].setReal("extrudeLimit", panel->getFloat(pcb_id)); break;
      case NM_PARAM_EMRG_SAFE_CUT_LIMIT: navMeshProps[navMeshIdx].setReal("safeCutLimit", panel->getFloat(pcb_id)); break;
      case NM_PARAM_EMRG_UNSAFE_CUT_LIMIT: navMeshProps[navMeshIdx].setReal("unsafeCutLimit", panel->getFloat(pcb_id)); break;
      case NM_PARAM_EMRG_UNSAFE_MAX_CUT_SPACE: navMeshProps[navMeshIdx].setReal("maxUnsafeCutSpace", panel->getFloat(pcb_id)); break;

      case NM_PARAM_JLK_ENABLED: navMeshProps[navMeshIdx].setBool("jumpLinksEnabled", panel->getBool(pcb_id)); break;
      case NM_PARAM_JLK_TYPEGEN:
      {
        const int typegen = panel->getInt(pcb_id);
        navMeshProps[navMeshIdx].setInt("jumpLinksTypeGen", typegen);
        const bool jlkEnableOrig = (typegen == 0);
        const bool jlkEnableNew2024 = (typegen == 1);
        panel->setEnabledById(baseOfs + NM_PARAM_JLK_JUMPOFF_MIN_HEIGHT, jlkEnableNew2024);
        panel->setEnabledById(baseOfs + NM_PARAM_JLK_JUMPOFF_MAX_HEIGHT, jlkEnableNew2024);
        panel->setEnabledById(baseOfs + NM_PARAM_JLK_JUMPOFF_MIN_XZ_LENGTH, jlkEnableNew2024);
        panel->setEnabledById(baseOfs + NM_PARAM_JLK_EDGE_MERGE_ANGLE, jlkEnableNew2024);
        panel->setEnabledById(baseOfs + NM_PARAM_JLK_EDGE_MERGE_DIST, jlkEnableNew2024);
        panel->setEnabledById(baseOfs + NM_PARAM_JLK_DH_THRESHOLD, jlkEnableOrig);
        panel->setEnabledById(baseOfs + NM_PARAM_JLK_MERGE_ANGLE, jlkEnableOrig);
        panel->setEnabledById(baseOfs + NM_PARAM_JLK_MERGE_DIST, jlkEnableOrig);
        panel->setEnabledById(baseOfs + NM_PARAM_JLK_MAX_OBSTRUCTION_ANGLE, jlkEnableOrig);
        break;
      }
      case NM_PARAM_JLK_JUMPOFF_MIN_HEIGHT:
        navMeshProps[navMeshIdx].setReal("jumpLinksJumpoffMinHeight", panel->getFloat(pcb_id));
        break;
      case NM_PARAM_JLK_JUMPOFF_MAX_HEIGHT:
        navMeshProps[navMeshIdx].setReal("jumpLinksJumpoffMaxHeight", panel->getFloat(pcb_id));
        break;
      case NM_PARAM_JLK_JUMPOFF_MIN_XZ_LENGTH:
        navMeshProps[navMeshIdx].setReal("jumpLinksJumpoffMinLinkLength", panel->getFloat(pcb_id));
        break;
      case NM_PARAM_JLK_EDGE_MERGE_ANGLE: navMeshProps[navMeshIdx].setReal("jumpLinksEdgeMergeAngle", panel->getFloat(pcb_id)); break;
      case NM_PARAM_JLK_EDGE_MERGE_DIST: navMeshProps[navMeshIdx].setReal("jumpLinksEdgeMergeDist", panel->getFloat(pcb_id)); break;
      case NM_PARAM_JLK_EDGE_MERGE_DIST_V1:
        navMeshProps[navMeshIdx].setReal("jumpLinksEdgeMergeDistV1", panel->getFloat(pcb_id));
        break;
      case NM_PARAM_JLK_JUMP_OVER_HEIGHT: navMeshProps[navMeshIdx].setReal("jumpLinksHeight", panel->getFloat(pcb_id)); break;
      case NM_PARAM_JLK_JUMP_LENGTH: navMeshProps[navMeshIdx].setReal("jumpLinksLength", panel->getFloat(pcb_id)); break;
      case NM_PARAM_JLK_MIN_WIDTH: navMeshProps[navMeshIdx].setReal("jumpLinksWidth", panel->getFloat(pcb_id)); break;
      case NM_PARAM_JLK_AGENT_HEIGHT: navMeshProps[navMeshIdx].setReal("jumpLinksAgentHeight", panel->getFloat(pcb_id)); break;
      case NM_PARAM_JLK_AGENT_MIN_SPACE: navMeshProps[navMeshIdx].setReal("jumpLinksAgentMinSpace", panel->getFloat(pcb_id)); break;
      case NM_PARAM_JLK_DH_THRESHOLD: navMeshProps[navMeshIdx].setReal("jumpLinksDeltaHeightTreshold", panel->getFloat(pcb_id)); break;
      case NM_PARAM_JLK_MAX_OBSTRUCTION_ANGLE:
        navMeshProps[navMeshIdx].setReal("jumpLinksMaxObstructionAngle", panel->getFloat(pcb_id));
        break;
      case NM_PARAM_JLK_MERGE_ANGLE: navMeshProps[navMeshIdx].setReal("jumpLinksMergeAngle", panel->getFloat(pcb_id)); break;
      case NM_PARAM_JLK_MERGE_DIST: navMeshProps[navMeshIdx].setReal("jumpLinksMergeDist", panel->getFloat(pcb_id)); break;
      case NM_PARAM_JLK_CPLX_THRESHOLD: navMeshProps[navMeshIdx].setReal("complexJumpTheshold", panel->getFloat(pcb_id)); break;
      case NM_PARAM_JLK_CROSS_OBSTACLES_WITH_JUMPLINKS:
        navMeshProps[navMeshIdx].setBool("crossObstaclesWithJumplinks", panel->getBool(pcb_id));
        break;
      case NM_PARAM_JLK_GENERATE_CUSTOM_JUMPLINKS:
        navMeshProps[navMeshIdx].setBool("enableCustomJumplinks", panel->getBool(pcb_id));
        break;

      case NM_PARAM_CVRS_ENABLED: navMeshProps[navMeshIdx].setBool("coversEnabled", panel->getBool(pcb_id)); break;
      case NM_PARAM_CVRS_TYPEGEN:
        navMeshProps[navMeshIdx].setInt("coversTypeGen", panel->getInt(pcb_id));
        {
          const bool covEnable1 = (panel->getInt(pcb_id) == 0);
          panel->setEnabledById(baseOfs + NM_PARAM_CVRS_MAX_HEIGHT, covEnable1);
          panel->setEnabledById(baseOfs + NM_PARAM_CVRS_MIN_HEIGHT, covEnable1);
          panel->setEnabledById(baseOfs + NM_PARAM_CVRS_MIN_WIDTH, covEnable1);
          panel->setEnabledById(baseOfs + NM_PARAM_CVRS_MIN_DEPTH, covEnable1);
          panel->setEnabledById(baseOfs + NM_PARAM_CVRS_SHOOT_WINDOW_HEIGHT, covEnable1);
          panel->setEnabledById(baseOfs + NM_PARAM_CVRS_COLLISION_TARNSP_THRESHOLD, covEnable1);
          panel->setEnabledById(baseOfs + NM_PARAM_CVRS_MERGE_DIST, covEnable1);
          panel->setEnabledById(baseOfs + NM_PARAM_CVRS_DH_THRESHOLD, covEnable1);
          panel->setEnabledById(baseOfs + NM_PARAM_CVRS_SHOOT_DEPTH_CHECK, covEnable1);
          panel->setEnabledById(baseOfs + NM_PARAM_CVRS_SHOOT_HEIGHT, covEnable1);
          panel->setEnabledById(baseOfs + NM_PARAM_CVRS_MAX_HEIGHT, covEnable1);
        }
        break;
      case NM_PARAM_CVRS_MAX_HEIGHT: navMeshProps[navMeshIdx].setReal("coversMaxHeight", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CVRS_MIN_HEIGHT: navMeshProps[navMeshIdx].setReal("coversMinHeight", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CVRS_MIN_WIDTH: navMeshProps[navMeshIdx].setReal("coversWidth", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CVRS_MIN_DEPTH: navMeshProps[navMeshIdx].setReal("coversDepth", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CVRS_SHOOT_WINDOW_HEIGHT: navMeshProps[navMeshIdx].setReal("coversShootWindow", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CVRS_COLLISION_TARNSP_THRESHOLD:
        navMeshProps[navMeshIdx].setReal("coversTransparent", panel->getFloat(pcb_id));
        break;
      case NM_PARAM_CVRS_MERGE_DIST: navMeshProps[navMeshIdx].setReal("coversMergeDist", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CVRS_DH_THRESHOLD: navMeshProps[navMeshIdx].setReal("coversDeltaHeightTreshold", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CVRS_SHOOT_DEPTH_CHECK: navMeshProps[navMeshIdx].setReal("coversShootDepth", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CVRS_SHOOT_HEIGHT: navMeshProps[navMeshIdx].setReal("coversShootHeight", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CVRS_OPENING_TRESHOLD: navMeshProps[navMeshIdx].setReal("openingTreshold", panel->getFloat(pcb_id)); break;
      case NM_PARAM_CVRS_BOX_OFFSET: navMeshProps[navMeshIdx].setReal("visibleBoxOffset", panel->getFloat(pcb_id)); break;

      case NM_PARAM_CVRS_TEST_X:
        if (panel->getFloat(pcb_id) != 0.0f || navMeshProps[navMeshIdx].findParam("coversTestX") >= 0)
          navMeshProps[navMeshIdx].setReal("coversTestX", panel->getFloat(pcb_id));
        break;
      case NM_PARAM_CVRS_TEST_Y:
        if (panel->getFloat(pcb_id) != 0.0f || navMeshProps[navMeshIdx].findParam("coversTestY") >= 0)
          navMeshProps[navMeshIdx].setReal("coversTestY", panel->getFloat(pcb_id));
        break;
      case NM_PARAM_CVRS_TEST_Z:
        if (panel->getFloat(pcb_id) != 0.0f || navMeshProps[navMeshIdx].findParam("coversTestZ") >= 0)
          navMeshProps[navMeshIdx].setReal("coversTestZ", panel->getFloat(pcb_id));
        break;

      case NM_PARAM_DROP_RESULT_ON_COLLISION:
        navMeshProps[navMeshIdx].setBool("dropNavmeshOnCollision", panel->getBool(pcb_id));
        panel->setEnabledById(baseOfs + NM_PARAM_DROP_RESULT_ON_COLLISION_TRACE_OFFSET, panel->getBool(pcb_id));
        panel->setEnabledById(baseOfs + NM_PARAM_DROP_RESULT_ON_COLLISION_TRACE_LEN, panel->getBool(pcb_id));
        break;
      case NM_PARAM_DROP_RESULT_ON_COLLISION_TRACE_OFFSET:
        navMeshProps[navMeshIdx].setReal("dropNavmeshOffset", panel->getFloat(pcb_id));
        break;
      case NM_PARAM_DROP_RESULT_ON_COLLISION_TRACE_LEN:
        navMeshProps[navMeshIdx].setReal("dropNavmeshTraceLen", panel->getFloat(pcb_id));
        break;
      case NM_PARAM_COMPRESS_RATIO: navMeshProps[navMeshIdx].setInt("navmeshCompressRatio", panel->getInt(pcb_id)); break;
    }

    int navmeshExportType = panel->getInt(baseOfs + NM_PARAM_EXPORT_TYPE);
    if (navmeshExportType >= baseOfs + NM_PARAM_EXPORT_TYPE_WATER &&
        navmeshExportType <= baseOfs + NM_PARAM_EXPORT_TYPE_WATER_AND_GEOMETRY)
    {
      navMeshProps[navMeshIdx].setStr("navmeshExportType",
        navmesh_export_type_to_string(
          NavmeshExportType(int(NavmeshExportType::WATER) + (navmeshExportType - (baseOfs + NM_PARAM_EXPORT_TYPE_WATER)))));
      panel->setEnabledById(baseOfs + NM_PARAM_WATER, navmeshExportType != baseOfs + NM_PARAM_EXPORT_TYPE_WATER &&
                                                        navmeshExportType != baseOfs + NM_PARAM_EXPORT_TYPE_WATER_AND_GEOMETRY);
    }

    navmeshAreasProcessing[navMeshIdx].onChange(pcb_id);
  }


  bool newShowHm = panel->getInt(PID_RENDER_RADIOGROUP_HM) == PID_RENDER_FINAL_HM;
  if (newShowHm != render.showFinalHM)
  {
    render.showFinalHM = newShowHm;
    if (render.showFinalHM)
      applyHmModifiers(false);
    // generateLandColors(NULL); not my comment
    // calcFastLandLighting(); not my comment
    updateHeightMapTex(false);
    updateHeightMapTex(true);
  }

  int newLightingDetails = 0;
  switch (panel->getInt(PID_LIGHTING_SUBDIV))
  {
    case PID_LIGHTING_1: newLightingDetails = 1; break;

    case PID_LIGHTING_2: newLightingDetails = 2; break;

    case PID_LIGHTING_4: newLightingDetails = 4; break;
  }

  if (newLightingDetails && newLightingDetails != lightmapScaleFactor)
  {
    lightmapScaleFactor = newLightingDetails;
    lightMapScaled.reset(heightMap.getMapSizeX() * lightmapScaleFactor, heightMap.getMapSizeY() * lightmapScaleFactor, 0xFFFF);
    createLightmapFile(DAGORED2->getConsole());
    resetRenderer();
  }


  int newLcmDetails = 0;
  switch (panel->getInt(PID_LCMAP_SUBDIV))
  {
    case PID_LCMAP_1: newLcmDetails = 1; break;
    case PID_LCMAP_2: newLcmDetails = 2; break;
    case PID_LCMAP_4: newLcmDetails = 4; break;
  }
  if (newLcmDetails && newLcmDetails != lcmScale)
  {
    lcmScale = newLcmDetails;
    resizeLandClassMapFile(DAGORED2->getConsole());
    lcMgr->reinitRIGen();
    onLandSizeChanged();
    generateLandColors();
  }

  int newDesiredHmapUpscale = 0;
  switch (panel->getInt(PID_HMAP_UPSCALE))
  {
    case PID_HMAP_UPSCALE_1: newDesiredHmapUpscale = 1; break;
    case PID_HMAP_UPSCALE_2: newDesiredHmapUpscale = 2; break;
    case PID_HMAP_UPSCALE_4: newDesiredHmapUpscale = 4; break;
  }
  if (hmlService && newDesiredHmapUpscale && newDesiredHmapUpscale != hmlService->getDesiredHmapUpscale())
  {
    hmlService->setDesiredHmapUpscale(newDesiredHmapUpscale);
  }

  // if (!edit_finished)
  //   return;

  if (pcb_id < PID_BRUSHES_PIDS)
  {
    brushes[currentBrushId]->updateFromPanelRef(*panel, pcb_id);
    if (brushDlg->isVisible())
    {
      if (propPanel->getPanelWindow())
        brushes[currentBrushId]->updateToPanel(*propPanel->getPanelWindow());

      return;
    }
    DAGORED2->repaint();
  }

  /*if (pcb_id>=PID_DETAIL_TEX_OFFSET_0 && pcb_id<=PID_DETAIL_TEX_OFFSET_LAST)
  {
    int index = pcb_id-PID_DETAIL_TEX_OFFSET_0;
    if (index >= detailTexOffset.size())
      detailTexOffset.resize(index + 1);

    detailTexOffset[index] = panel->getPoint2(pcb_id);
  #if defined(USE_HMAP_ACES)
    //panel->setCaption(pcb_id, ::dd_get_fname(detailTexBlkName[index]));
  #endif
    resetRenderer();
    return;
  }*/

  switch (pcb_id)
  {
    case PID_GRID_CELL_SIZE:
      gridCellSize = panel->getFloat(pcb_id);
      if (gridCellSize <= 0)
        gridCellSize = 1;
      autocenterHeightmap(panel);
      resetRenderer();
      onLandSizeChanged();
      onWholeLandChanged();
      break;

    case PID_HEIGHT_SCALE:
    case PID_HEIGHT_OFFSET:
    case PID_HEIGHTMAP_OFFSET: onChangeFinished(pcb_id, panel); break;

    case PID_HEIGHTMAP_AUTOCENTER:
      doAutocenter = panel->getBool(pcb_id);
      autocenterHeightmap(panel);
      onLandSizeChanged();
      onWholeLandChanged();
      break;

    case PID_HM_COLLISION_OFFSET:
      collisionArea.ofs = panel->getPoint2(pcb_id);
      DAGORED2->invalidateViewportCache();
      break;

    case PID_HM_COLLISION_SIZE:
      collisionArea.sz = panel->getPoint2(pcb_id);
      DAGORED2->invalidateViewportCache();
      break;

    case PID_HM_COLLISION_SHOW:
      collisionArea.show = panel->getBool(pcb_id);
      DAGORED2->invalidateViewportCache();
      break;

    case PID_RENDER_GRID_STEP:
      render.gridStep = panel->getInt(pcb_id);
      resetRenderer();
      break;

    case PID_RENDER_RADIUS_ELEMS:
      render.radiusElems = panel->getInt(pcb_id);
      resetRenderer();
      break;

    case PID_RENDER_RING_ELEMS:
      render.ringElems = panel->getInt(pcb_id);
      resetRenderer();
      break;

    case PID_RENDER_NUM_LODS:
      render.numLods = panel->getInt(pcb_id);
      resetRenderer();
      break;

    case PID_RENDER_MAX_DETAIL_LOD:
      render.maxDetailLod = panel->getInt(pcb_id);
      resetRenderer();
      break;

    case PID_RENDER_DETAIL_TILE:
      render.detailTile = panel->getFloat(pcb_id);
      resetRenderer();
      break;

    case PID_RENDER_CANYON_HOR_TILE:
      render.canyonHorTile = panel->getFloat(pcb_id);
      resetRenderer();
      break;

    case PID_RENDER_CANYON_VERT_TILE:
      render.canyonVertTile = panel->getFloat(pcb_id);
      resetRenderer();
      break;

    case PID_RENDER_HM2_YBASE_FOR_LOD:
      render.hm2YbaseForLod = panel->getFloat(pcb_id);
      DAGORED2->repaint();
      break;

    case PID_RENDER_ELEM_SIZE:
      render.elemSize = panel->getInt(pcb_id);
      if (render.elemSize < 1)
        render.elemSize = 1;
      if (render.elemSize > 16)
        render.elemSize = 16;
      resetRenderer();
      break;

    case PID_RENDER_DEBUG_LINES:
      renderDebugLines = panel->getBool(pcb_id);
      DAGORED2->repaint();
      break;
    case PID_RENDER_ALL_SPLINES_ALWAYS: renderAllSplinesAlways = panel->getBool(pcb_id); break;
    case PID_RENDER_SEL_SPLINES_ALWAYS: renderSelSplinesAlways = panel->getBool(pcb_id); break;

    case PID_CANYON_ANGLE:
      render.canyonAngle = panel->getFloat(pcb_id);
      resetRenderer();
      break;

    case PID_CANYON_FADE_ANGLE:
      render.canyonFadeAngle = panel->getFloat(pcb_id);
      resetRenderer();
      break;

    case PID_SUN_AZIMUTH:
      sunAzimuth = HALFPI - DegToRad(panel->getFloat(pcb_id));
      updateRendererLighting();
      break;

    case PID_SUN_ZENITH:
      sunZenith = DegToRad(panel->getFloat(pcb_id));
      updateRendererLighting();
      break;

    case PID_SHADOW_BIAS: shadowBias = panel->getFloat(pcb_id); break;

    case PID_SHADOW_TRACE_DIST: shadowTraceDist = panel->getFloat(pcb_id); break;

    case PID_SHADOW_DENSITY: shadowDensity = panel->getFloat(pcb_id) / 100; break;

    case PID_SHOWMASK:
      showBlueWhiteMask = panel->getBool(pcb_id);
      setShowBlueWhiteMask();
      if (editedScriptImage)
      {
        if (showBlueWhiteMask)
          updateBlueWhiteMask(NULL);
        DAGORED2->invalidateViewportCache();
      }
      break;

    case PID_MONOLAND:
      showMonochromeLand = panel->getBool(pcb_id);
      if (hmlService)
        hmlService->invalidateClipmap(showMonochromeLand);
      DAGORED2->invalidateViewportCache();
      break;

    case PID_MONOLAND_COL:
      monochromeLandCol = panel->getColor(pcb_id);
      if (showMonochromeLand)
        if (hmlService)
          hmlService->invalidateClipmap(true);
      DAGORED2->invalidateViewportCache();
      break;

    case PID_GROUND_OBJECTS_COL:
      dagGeom->shaderGlobalSetColor4(dagGeom->getShaderVariableId("prefabs_color"), Color4(panel->getColor(pcb_id)));
      DAGORED2->invalidateViewportCache();
      break;

    case PID_HTLEVELS:
      showHtLevelCurves = panel->getBool(pcb_id);
      updateHtLevelCurves();
      break;
    case PID_HTLEVELS_STEP:
      htLevelCurveStep = panel->getFloat(pcb_id);
      updateHtLevelCurves();
      break;
    case PID_HTLEVELS_OFS:
      htLevelCurveOfs = panel->getFloat(pcb_id);
      updateHtLevelCurves();
      break;
    case PID_HTLEVELS_THICK:
      htLevelCurveThickness = panel->getFloat(pcb_id);
      updateHtLevelCurves();
      break;
    case PID_HTLEVELS_DARK:
      htLevelCurveDarkness = panel->getFloat(pcb_id) / 100;
      updateHtLevelCurves();
      break;

    case PID_TILE_SIZE_X:
      tileXSize = panel->getFloat(pcb_id);
      resetRenderer();
      break;

    case PID_TILE_SIZE_Y:
      tileYSize = panel->getFloat(pcb_id);
      resetRenderer();
      break;


    case PID_HORIZONTAL_TILE_SIZE_X:
      horizontalTex_TileSizeX = panel->getFloat(pcb_id);
      updateHorizontalTex();
      break;

    case PID_HORIZONTAL_TILE_SIZE_Y:
      horizontalTex_TileSizeY = panel->getFloat(pcb_id);
      updateHorizontalTex();
      break;

    case PID_HORIZONTAL_OFFSET_X:
      horizontalTex_OffsetX = panel->getFloat(pcb_id);
      updateHorizontalTex();
      break;

    case PID_HORIZONTAL_OFFSET_Y:
      horizontalTex_OffsetY = panel->getFloat(pcb_id);
      updateHorizontalTex();
      break;

    case PID_HORIZONTAL_DETAIL_TILE_SIZE_X:
      horizontalTex_DetailTexSizeX = panel->getFloat(pcb_id);
      updateHorizontalTex();
      break;

    case PID_HORIZONTAL_DETAIL_TILE_SIZE_Y:
      horizontalTex_DetailTexSizeY = panel->getFloat(pcb_id);
      updateHorizontalTex();
      break;

    case PID_USE_HORIZONTAL_TEX:
      useHorizontalTex = panel->getBool(pcb_id);
      updateHorizontalTex();
      break;


    case PID_LAND_MODULATE_EDGE0_X:
      landModulateColorEdge0 = min(panel->getFloat(pcb_id), landModulateColorEdge1 - 0.001f);
      panel->setFloat(PID_LAND_MODULATE_EDGE0_X, landModulateColorEdge0);
      updateLandModulateColorTex();
      break;

    case PID_LAND_MODULATE_EDGE1_X:
      landModulateColorEdge1 = panel->getFloat(pcb_id);
      landModulateColorEdge0 = min(landModulateColorEdge0, landModulateColorEdge1 - 0.001f);
      panel->setFloat(PID_LAND_MODULATE_EDGE0_X, landModulateColorEdge0);
      updateLandModulateColorTex();
      break;

    case PID_USE_LAND_MODULATE_TEX:
      useLandModulateColorTex = panel->getBool(pcb_id);
      updateLandModulateColorTex();
      if (hmlService)
        hmlService->invalidateClipmap(true);
      break;

    case PID_USE_RENDIST_DETAIL2_COLOR:
      useRendinstDetail2Modulation = panel->getBool(pcb_id);
      setRendinstlayeredDetailColor();
      break;
    case PID_RENDIST_DETAIL2_COLOR_FROM:
      rendinstDetail2ColorFrom = panel->getColor(pcb_id);
      setRendinstlayeredDetailColor();
      break;
    case PID_RENDIST_DETAIL2_COLOR_TO:
      rendinstDetail2ColorTo = panel->getColor(pcb_id);
      setRendinstlayeredDetailColor();
      break;


    case PID_ENABLE_GRASS:
      enableGrass = panel->getBool(pcb_id);
      if (grassService)
        grassService->enableGrass(enableGrass);
      if (gpuGrassService)
        gpuGrassService->enableGrass(enableGrass);
      break;


    case PID_VERT_TEX_USE:
      useVertTex = panel->getBool(pcb_id);
      updateVertTex();
      break;
    case PID_VERT_TEX_USE_HMAP:
      useVertTexForHMAP = panel->getBool(pcb_id);
      updateVertTex();
      break;

    case PID_VERT_TEX_TILE_XZ:
      vertTexXZtile = panel->getFloat(pcb_id);
      updateVertTex();
      break;
    case PID_VERT_TEX_TILE_Y:
      vertTexYtile = panel->getFloat(pcb_id);
      updateVertTex();
      break;
    case PID_VERT_TEX_Y_OFS:
      vertTexYOffset = panel->getFloat(pcb_id);
      updateVertTex();
      break;
    case PID_VERT_TEX_ANG0:
      vertTexAng0 = panel->getFloat(pcb_id);
      updateVertTex();
      break;
    case PID_VERT_TEX_ANG1:
      vertTexAng1 = panel->getFloat(pcb_id);
      updateVertTex();
      break;
    case PID_VERT_TEX_HBLEND:
      vertTexHorBlend = panel->getFloat(pcb_id);
      updateVertTex();
      break;
    case PID_VERT_DET_TEX_TILE_XZ:
      vertDetTexXZtile = panel->getFloat(pcb_id);
      updateVertTex();
      break;
    case PID_VERT_DET_TEX_TILE_Y:
      vertDetTexYtile = panel->getFloat(pcb_id);
      updateVertTex();
      break;
    case PID_VERT_DET_TEX_Y_OFS:
      vertDetTexYOffset = panel->getFloat(pcb_id);
      updateVertTex();
      break;
  }

  if (pcb_id >= PID_SHADOW_CASTER_FIRST && pcb_id < PID_SHADOW_CASTER_LAST)
  {
    const int shadowIdx = pcb_id - PID_SHADOW_CASTER_FIRST;
    const IDagorEdCustomCollider *collider = DAGORED2->getCustomCollider(shadowIdx);

    if (collider)
    {
      if (panel->getBool(pcb_id))
        DAGORED2->enableCustomShadow(collider->getColliderName());
      else
        DAGORED2->disableCustomShadow(collider->getColliderName());
    }
  }

  if (pcb_id >= PID_GRASS_MASK_START && pcb_id <= PID_GRASS_MASK_END)
  {
    SimpleString fname = panel->getText(pcb_id);
    int index = pcb_id - PID_GRASS_MASK_START;
    String maskName = ::get_file_name_wo_ext(fname);
    panel->setText(pcb_id, maskName);
    grassBlk->setStr(grass.masks[index].first, maskName.c_str());
    grassService->resetGrassMask(*landMeshRenderer, index, *grassBlk, grass.masks[index].first, maskName);
    // This texture is released in landMeshRenderer
    Tab<LandClassDetailTextures> &landClasses = landMeshManager->getLandClasses();
    landClasses[index].grassMaskTexId = BAD_TEXTUREID;
    HmapLandPlugin::grassService->forceUpdate();
  }
  else if (pcb_id >= PID_GRASS_DEFAULT_MIN_DENSITY && pcb_id <= PID_GRASS_HELICOPTER_FALLOFF)
  {
    switch (pcb_id)
    {
      case PID_GRASS_DEFAULT_MIN_DENSITY:
        grass.defaultMinDensity = panel->getFloat(pcb_id);
        grassBlk->setReal("minLodDensity", grass.defaultMinDensity);
        break;
      case PID_GRASS_DEFAULT_MAX_DENSITY:
        grass.defaultMaxDensity = panel->getFloat(pcb_id);
        grassBlk->setReal("density", grass.defaultMaxDensity);
        break;
      case PID_GRASS_MAX_RADIUS:
        grass.maxRadius = panel->getFloat(pcb_id);
        grassBlk->setReal("maxRadius", grass.maxRadius);
        break;
      case PID_GRASS_TEX_SIZE:
        grass.texSize = panel->getInt(pcb_id);
        grassBlk->setReal("texSize", grass.texSize);
        break;
      case PID_GRASS_SOW_PER_LIN_FREQ:
        grass.sowPerlinFreq = panel->getFloat(pcb_id);
        grassBlk->setReal("sowPerlinFreq", grass.sowPerlinFreq);
        break;
      case PID_GRASS_FADE_DELTA:
        grass.fadeDelta = panel->getFloat(pcb_id);
        grassBlk->setReal("fadeDelta", grass.fadeDelta);
        break;
      case PID_GRASS_LOD_FADE_DELTA:
        grass.lodFadeDelta = panel->getFloat(pcb_id);
        grassBlk->setReal("lodFadeDelta", grass.lodFadeDelta);
        break;
      case PID_GRASS_BLEND_TO_LAND_DELTA:
        grass.blendToLandDelta = panel->getFloat(pcb_id);
        grassBlk->setReal("blendToLandDelta", grass.blendToLandDelta);
        break;
      case PID_GRASS_NOISE_FIRST_RND:
        grass.noise_first_rnd = panel->getFloat(pcb_id);
        grassBlk->setReal("noise_first_rnd", grass.noise_first_rnd);
        break;
      case PID_GRASS_NOISE_FIRST_VAL_MUL:
        grass.noise_first_val_mul = panel->getFloat(pcb_id);
        grassBlk->setReal("noise_first_val_mul", grass.noise_first_val_mul);
        break;
      case PID_GRASS_NOISE_FIRST_VAL_ADD:
        grass.noise_first_val_add = panel->getFloat(pcb_id);
        grassBlk->setReal("noise_first_val_add", grass.noise_first_val_add);
        break;
      case PID_GRASS_NOISE_SECOND_RND:
        grass.noise_second_rnd = panel->getFloat(pcb_id);
        grassBlk->setReal("noise_second_rnd", grass.noise_second_rnd);
        break;
      case PID_GRASS_NOISE_SECOND_VAL_MUL:
        grass.noise_second_val_mul = panel->getFloat(pcb_id);
        grassBlk->setReal("noise_second_val_mul", grass.noise_second_val_mul);
        break;
      case PID_GRASS_NOISE_SECOND_VAL_ADD:
        grass.noise_second_val_add = panel->getFloat(pcb_id);
        grassBlk->setReal("noise_second_val_add", grass.noise_second_val_add);
        break;
      case PID_GRASS_DIRECT_WIND_MUL:
        grass.directWindMul = panel->getFloat(pcb_id);
        grassBlk->setReal("directWindMul", grass.directWindMul);
        break;
      case PID_GRASS_NOISE_WIND_MUL:
        grass.noiseWindMul = panel->getFloat(pcb_id);
        grassBlk->setReal("noiseWindMul", grass.noiseWindMul);
        break;
      case PID_GRASS_WIND_PER_LIN_FREQ:
        grass.windPerlinFreq = panel->getFloat(pcb_id);
        grassBlk->setReal("windPerlinFreq", grass.windPerlinFreq);
        break;
      case PID_GRASS_DIRECT_WIND_FREQ:
        grass.directWindFreq = panel->getFloat(pcb_id);
        grassBlk->setReal("directWindFreq", grass.directWindFreq);
        break;
      case PID_GRASS_DIRECT_WIND_MODULATION_FREQ:
        grass.directWindModulationFreq = panel->getFloat(pcb_id);
        grassBlk->setReal("directWindModulationFreq", grass.directWindModulationFreq);
        break;
      case PID_GRASS_WIND_WAVE_LENGTH:
        grass.windWaveLength = panel->getFloat(pcb_id);
        grassBlk->setReal("windWaveLength", grass.windWaveLength);
        break;
      case PID_GRASS_WIND_TO_COLOR:
        grass.windToColor = panel->getFloat(pcb_id);
        grassBlk->setReal("windToColor", grass.windToColor);
        break;
      case PID_GRASS_HELICOPTER_HOR_MUL:
        grass.helicopterHorMul = panel->getFloat(pcb_id);
        grassBlk->setReal("helicopterHorMul", grass.helicopterHorMul);
        break;
      case PID_GRASS_HELICOPTER_VER_MUL:
        grass.helicopterVerMul = panel->getFloat(pcb_id);
        grassBlk->setReal("helicopterVerMul", grass.helicopterVerMul);
        break;
      case PID_GRASS_HELICOPTER_TO_DIRECT_WIND_MUL:
        grass.helicopterToDirectWindMul = panel->getFloat(pcb_id);
        grassBlk->setReal("helicopterToDirectWindMul", grass.helicopterToDirectWindMul);
        break;
      case PID_GRASS_HELICOPTER_FALLOFF:
        grass.helicopterFalloff = panel->getFloat(pcb_id);
        grassBlk->setReal("helicopterFalloff", grass.helicopterFalloff);
        break;
    }
    HmapLandPlugin::grassService->reloadAll(*grassBlk, levelBlk);
    bool savedEnableGrass = enableGrass;
    loadGrassLayers(*grassBlk, false);
    enableGrass = savedEnableGrass; //-V1048
    hmlService->setGrassBlk(grassBlk);
  }
  gpuGrassPanel.onChange(pcb_id, panel);
}
void HmapLandPlugin::onChangeFinished(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case PID_HEIGHT_SCALE:
      heightMap.heightScale = panel->getFloat(pcb_id);
      resetRenderer();
      heightMap.resetFinal();
      applyHmModifiers(true, true, false);
      updateHeightMapTex(false);
      // onLandSizeChanged();
      // onWholeLandChanged();
      break;

    case PID_HEIGHT_OFFSET:
      heightMap.heightOffset = panel->getFloat(pcb_id);
      resetRenderer();
      heightMap.resetFinal();
      applyHmModifiers(true, true, false);
      updateHeightMapTex(false);
      // onLandSizeChanged();
      // onWholeLandChanged();
      break;

    case PID_HEIGHTMAP_OFFSET:
      heightMapOffset = panel->getPoint2(pcb_id);
      resetRenderer();
      heightMap.resetFinal();
      applyHmModifiers(true, true, false);
      // onLandSizeChanged();
      // onWholeLandChanged();
      break;
  }
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

bool HmapLandPlugin::importScriptImage(String *name)
{
  String fname =
    wingw::file_open_dlg(NULL, "Import image for generator script", "TGA files (*.tga)|*.tga", "tga", DAGORED2->getSdkDir());

  if (!fname.length())
    return false;

  String texName = ::get_file_name_wo_ext(fname);
  String destFile(DAGORED2->getPluginFilePath(this, texName + ".tga"));

  if (::dd_file_exist(destFile))
  {
    if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Overwrite old image?") != wingw::MB_ID_YES)
      return false;
  }

  if (!dagTools->copyFile(fname, destFile))
  {
    wingw::message_box(wingw::MBS_EXCL, "Import error", "Unable to copy image\n\"%s\"\nto\n\"%s\"", (const char *)fname,
      (const char *)destFile);

    return false;
  }

  if (name)
    *name = texName;

  return true;
}


bool HmapLandPlugin::importTileTex()
{
  const char *tex = DAEDITOR3.selectAssetX(tileTexName, "Select tile texture", "tex");

  if (tex)
  {
    tileTexName = tex;
    if (propPanel)
      propPanel->fillPanel();
    resetRenderer();
  }
  return true;
}


bool HmapLandPlugin::createMask(int bpp, String *name)
{
  static const int INPUT_STRING_EDIT_ID = 222;
  PropPanel::DialogWindow *dialog = DAGORED2->createDialog(_pxScaled(250), _pxScaled(125), "Create mask");
  dialog->setInitialFocus(PropPanel::DIALOG_ID_NONE);
  PropPanel::ContainerPropertyControl *panel = dialog->getPanel();
  panel->createEditBox(INPUT_STRING_EDIT_ID, "Enter mask name:");
  panel->setFocusById(INPUT_STRING_EDIT_ID);

  String result;

  for (bool nameValid = false; !nameValid;)
  {
    nameValid = true;
    int ret = dialog->showDialog();
    if (ret == PropPanel::DIALOG_ID_OK)
    {
      result = panel->getText(INPUT_STRING_EDIT_ID);
    }

    if (result.length())
    {
      for (const char *c = result; *c; ++c)
      {
        if (!isalnum(*c) && *c != '_')
        {
          wingw::message_box(wingw::MBS_EXCL, "Name error", "Mask name can contain only latin letters, digits and '_' symbol.");

          nameValid = false;
          break;
        }
      }
    }
  }

  DAGORED2->deleteDialog(dialog);

  if (result.length())
  {
    IBitMaskImageMgr::BitmapMask img;
    if (bpp == 32)
      bitMaskImgMgr->createImage32(img, getHeightmapSizeX(), getHeightmapSizeY());
    else
      bitMaskImgMgr->createBitMask(img, getHeightmapSizeX(), getHeightmapSizeY(), bpp);

    if (bitMaskImgMgr->saveImage(img, HmapLandPlugin::self->getInternalName(), result))
      if (*name)
        *name = result;
    bitMaskImgMgr->destroyImage(img);
    return true;
  }

  return false;
}

void HmapLandPlugin::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id >= PID_GRASS_PARAM_START && pcb_id <= PID_LAST_GRASS_PARAM)
  {
    bool layer_removed = false;
    for (int i = 0; i < grassLayers.size(); i++)
    {
      grassLayers[i]->onClick(pcb_id, *panel);
      if (grassLayers[i]->remove_this_layer)
      {
        HmapLandPlugin::grassService->removeLayer(grassLayers[i]->grass_layer_i);
        safe_erase_items(grassLayers, i, 1);
        layer_removed = true;
        break;
      }
    }

    if (layer_removed)
    {
      // reset layer ids
      for (int i = 0; i < grassLayers.size(); i++)
        grassLayers[i]->grass_layer_i = i;
      if (propPanel)
        propPanel->fillPanel();
    }
  }

  if (pcb_id == PID_IMPORT_SCRIPT_IMAGE)
  {
    importScriptImage();
  }
  else if (pcb_id == PID_TILE_TEX)
  {
    importTileTex();
  }


  else if (pcb_id == PID_HORIZONTAL_TEX_NAME)
  {
    const char *tex = DAEDITOR3.selectAssetX(horizontalTexName, "Select rendinst vertical texture", "tex");

    if (tex)
    {
      releaseHorizontalTexRef();
      horizontalTexName = tex;
      if (propPanel)
        propPanel->fillPanel();
      acquireHorizontalTexRef();
      updateHorizontalTex();
    }
  }
  else if (pcb_id == PID_HORIZONTAL_DET_TEX_NAME)
  {
    const char *tex = DAEDITOR3.selectAssetX(horizontalDetailTexName, "Select rendinst vertical detail texture", "tex");

    if (tex)
    {
      releaseHorizontalTexRef();
      horizontalDetailTexName = tex;
      if (propPanel)
        propPanel->fillPanel();
      acquireHorizontalTexRef();
      updateHorizontalTex();
    }
  }

  else if (pcb_id == PID_LAND_MODULATE_TEX_NAME)
  {
    const char *tex = DAEDITOR3.selectAssetX(landModulateColorTexName, "Select land modulate texture", "tex");

    if (tex)
    {
      releaseLandModulateColorTexRef();
      landModulateColorTexName = tex;
      if (propPanel)
        propPanel->fillPanel();
      acquireLandModulateColorTexRef();
      updateLandModulateColorTex();
    }
  }

  else if (pcb_id == PID_VERT_TEX_NAME)
  {
    const char *tex = DAEDITOR3.selectAssetX(vertTexName, "Select vertical texture", "tex");

    if (tex)
    {
      releaseVertTexRef();
      vertTexName = tex;
      if (propPanel)
        propPanel->fillPanel();
      acquireVertTexRef();
      updateVertTex();
    }
  }
  else if (pcb_id == PID_VERT_NM_TEX_NAME)
  {
    const char *tex = DAEDITOR3.selectAssetX(vertNmTexName, "Select vertical NM texture", "tex");

    if (tex)
    {
      releaseVertTexRef();
      vertNmTexName = tex;
      if (propPanel)
        propPanel->fillPanel();
      acquireVertTexRef();
      updateVertTex();
    }
  }
  else if (pcb_id == PID_VERT_DET_TEX_NAME)
  {
    const char *tex = DAEDITOR3.selectAssetX(vertDetTexName, "Select vertical detail texture", "tex");

    if (tex)
    {
      releaseVertTexRef();
      vertDetTexName = tex;
      if (propPanel)
        propPanel->fillPanel();
      acquireVertTexRef();
      updateVertTex();
    }
  }
  else if (pcb_id == PID_CREATE_MASK)
  {
    createMask(1);
  }
  else if (pcb_id == PID_RESET_SCRIPT)
  {
    if (colorGenScriptFilename.empty())
      return;
    if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation",
          "Do you really want to reset generation script?\n\n"
          "(generation layers, if any, can do processing without script)") != wingw::MB_ID_YES)
      return;

    colorGenScriptFilename = "";
    getColorGenVarsFromScript();
    onWholeLandChanged();
    if (propPanel)
      propPanel->fillPanel();
  }
  else if (pcb_id == PID_RELOAD_SCRIPT)
  {
    getColorGenVarsFromScript();
    onWholeLandChanged();
    if (propPanel)
      propPanel->fillPanel();
  }
  else if (pcb_id == PID_GENERATE_COLORMAP)
  {
    generateLandColors();
    resetRenderer();
  }
  else if (pcb_id == PID_CALC_FAST_LIGHTING)
  {
    calcFastLandLighting();
    resetRenderer();
  }
  else if (pcb_id == PID_GET_SUN_SETTINGS)
  {
    getAllSunSettings();
    updateLightGroup(*panel);
  }
  else if (pcb_id == PID_CALC_GOOD_LIGHTING)
  {
    calcGoodLandLighting();
    resetRenderer();
  }
  else if (pcb_id == PID_SAVE_LIGHTING_PC || pcb_id == PID_SAVE_LIGHTING_X360 || pcb_id == PID_SAVE_LIGHTING_PS3 ||
           pcb_id == PID_SAVE_LIGHTING_PS4 || pcb_id == PID_SAVE_LIGHTING_iOS || pcb_id == PID_SAVE_LIGHTING_AND)
  {
    String fileName = wingw::file_open_dlg(NULL, "Save lightmap", "Lightmap (*.ddsx)|*.ddsx", ".dds", "lightmap.ddsx");

    if (fileName.length())
    {
      if (pcb_id == PID_SAVE_LIGHTING_PC)
        exportLightmapToFile(fileName, _MAKE4C('PC'), true);
      else if (pcb_id == PID_SAVE_LIGHTING_X360)
        exportLightmapToFile(fileName, _MAKE4C('PS4'), true);
      else if (pcb_id == PID_SAVE_LIGHTING_iOS)
        exportLightmapToFile(fileName, _MAKE4C('iOS'), true);
      else if (pcb_id == PID_SAVE_LIGHTING_AND)
        exportLightmapToFile(fileName, _MAKE4C('and'), true);
    }
  }
  else if (pcb_id == PID_SAVE_LIGHTING_DDS)
  {
    String fileName = wingw::file_open_dlg(NULL, "Save lightmap", "Lightmap (*.dds)|*.dds", ".dds", "lightmap.dds");

    exportLightmapToFile(fileName, _MAKE4C('DDS'), true);
  }
  else if (pcb_id >= PID_SCRIPT_PARAM_START && pcb_id <= PID_LAST_SCRIPT_PARAM)
  {
    for (int i = 0; i < colorGenParams.size(); ++i)
      colorGenParams[i]->onPPBtnPressed(pcb_id, *panel);
    for (int i = 0; i < genLayers.size(); ++i)
      if (genLayers[i]->onPPBtnPressedEx(pcb_id, *panel))
        break;
  }
  else if (pcb_id == PID_SCRIPT_FILE)
  {
    String fname = wingw::file_open_dlg(NULL, "Select color map generator script", "Squirrel files (*.nut)|*.nut|All files (*.*)|*.*",
      "nut", colorGenScriptFilename);

    if (!fname.length())
      return;

    colorGenScriptFilename = fname;
    getColorGenVarsFromScript();
    onWholeLandChanged();
    if (propPanel)
      propPanel->fillPanel();
  }
  else if (pcb_id == PID_ADDLAYER)
  {
    PropPanel::DialogWindow *dialog = DAGORED2->createDialog(_pxScaled(250), _pxScaled(125), "Create generation layer");
    dialog->setInitialFocus(PropPanel::DIALOG_ID_NONE);
    PropPanel::ContainerPropertyControl *panel = dialog->getPanel();
    panel->createEditBox(0, "Enter generation layer name:");
    panel->setFocusById(0);

    int ret = dialog->showDialog();
    if (ret == PropPanel::DIALOG_ID_OK)
    {
      addGenLayer(panel->getText(0));
      if (propPanel)
        propPanel->fillPanel();
    }
    DAGORED2->deleteDialog(dialog);
  }
  else if (pcb_id == PID_GRASS_LOADFROM_LEVELBLK)
  {
    loadGrassFromLevelBlk();
    if (propPanel)
      propPanel->fillPanel();
  }
  else if (pcb_id == PID_GRASS_ADDLAYER)
  {
    PropPanel::DialogWindow *dialog = DAGORED2->createDialog(_pxScaled(250), _pxScaled(125), "Create grass layer");
    dialog->setInitialFocus(PropPanel::DIALOG_ID_NONE);
    PropPanel::ContainerPropertyControl *panel = dialog->getPanel();
    panel->createEditBox(0, "Enter grass layer name:");
    panel->setFocusById(0);

    int ret = dialog->showDialog();
    if (ret == PropPanel::DIALOG_ID_OK)
    {
      addGrassLayer(panel->getText(0));
      if (propPanel)
        propPanel->fillPanel();
    }
    DAGORED2->deleteDialog(dialog);
  }
  else if (pcb_id == PID_REBUILD_MESH)
  {
    dd_erase(DAGORED2->getPluginFilePath(HmapLandPlugin::self, "delaunayGen.cache.bin"));
    generateLandMeshMap(landMeshMap, DAGORED2->getConsole(), false, NULL);
    onWholeLandChanged(); // Place objects on current hmap.
    resetLandmesh();
  }
  else if (pcb_id == PID_REBAKE_HMAP_MODIFIERS)
  {
    heightmapChanged();
    applyHmModifiers(false, true, false);
  }
  else if (pcb_id == PID_CONVERT_DELAUNAY_SPLINES_TO_HEIGHTBAKE)
  {
    convertDelaunaySplinesToHeightBake();
  }
  else if (pcb_id == PID_WATER_SURF_REBUILD)
  {
    rebuildWaterSurface();
  }
  else if (pcb_id == PID_WATER_SET_MAT)
  {
    const char *mat = DAEDITOR3.selectAssetX(waterMatAsset, "Select water material", "mat");
    if (mat)
    {
      waterMatAsset = mat;
      panel->setCaption(pcb_id, String(128, "Water mat: <%s>", waterMatAsset.str()));
    }
  }
  else if (pcb_id == PID_COPY_SNOW)
  {
    ambSnowValue = panel->getFloat(PID_DYN_SNOW);
    panel->setFloat(PID_AMB_SNOW, ambSnowValue);
  }
  else if (pcb_id == PID_GRID_H2_APPLY)
  {
    float csz = panel->getFloat(PID_GRID_H2_CELL_SIZE);
    Point2 b0 = panel->getPoint2(PID_GRID_H2_BBOX_OFS);
    Point2 b1 = panel->getPoint2(PID_GRID_H2_BBOX_SZ) + b0;
    int old_detDivisor = detDivisor;
    BBox2 old_detRect = detRect;
    IBBox2 old_detRectC = detRectC;
    float *heights = NULL;

    if (!csz)
      detDivisor = 0;
    else
    {
      float land_cell_sz = getLandCellSize();
      b0.x = floor((b0.x - heightMapOffset.x) / land_cell_sz) * land_cell_sz + heightMapOffset.x;
      b0.y = floor((b0.y - heightMapOffset.y) / land_cell_sz) * land_cell_sz + heightMapOffset.y;
      b1.x = floor((b1.x - heightMapOffset.x) / land_cell_sz) * land_cell_sz + heightMapOffset.x;
      b1.y = floor((b1.y - heightMapOffset.y) / land_cell_sz) * land_cell_sz + heightMapOffset.y;
      if (b0.x == b1.x)
        b1.x += land_cell_sz;
      if (b0.y == b1.y)
        b1.y += land_cell_sz;
      detDivisor = gridCellSize / csz;
      detRect[0] = b0;
      detRect[1] = b1;
      detRectC[0].set_xy((detRect[0] - heightMapOffset) / gridCellSize);
      detRectC[0] *= detDivisor;
      detRectC[1].set_xy((detRect[1] - heightMapOffset) / gridCellSize);
      detRectC[1] *= detDivisor;

      heights = (float *)midmem->tryAlloc(sizeof(float) * (detRectC[1].x - detRectC[0].x) * (detRectC[1].y - detRectC[0].y));
      if (heights)
      {
        memset(heights, 0, sizeof(float) * (detRectC[1].x - detRectC[0].x) * (detRectC[1].y - detRectC[0].y));

        int new_detDivisor = detDivisor;
        BBox2 new_detRect = detRect;
        IBBox2 new_detRectC = detRectC;

        detDivisor = old_detDivisor;
        detRect = old_detRect;
        detRectC = old_detRectC;

        float *hp = heights;
        DEBUG_DUMP_VAR(old_detRectC);
        DEBUG_DUMP_VAR(detRectC);
        DEBUG_DUMP_VAR(new_detRectC);
        for (int y = new_detRectC[0].y; y < new_detRectC[1].y; y++)
          for (int x = new_detRectC[0].x; x < new_detRectC[1].x; x++, hp++)
          {
            Point3 p(heightMapOffset.x + x * gridCellSize / new_detDivisor, 100,
              heightMapOffset.y + y * gridCellSize / new_detDivisor);
            if (getHeightmapOnlyPointHt(p, NULL))
              *hp = p.y;
            else
              debug("failed %d,%d: %@", x, y, p);
          }

        detDivisor = new_detDivisor;
        detRect = new_detRect;
        detRectC = new_detRectC;
      }
    }
    panel->setFloat(PID_GRID_H2_CELL_SIZE, detDivisor ? gridCellSize / detDivisor : 0);
    panel->setPoint2(PID_GRID_H2_BBOX_OFS, detRect[0]);
    panel->setPoint2(PID_GRID_H2_BBOX_SZ, detRect.width());

    if (detDivisor)
    {
      int dw = heightMap.getMapSizeX() * detDivisor, dh = heightMap.getMapSizeY() * detDivisor;
      bool created_anew = !heightMapDet.isFileOpened();

      if (created_anew || heightMapDet.getMapSizeX() != dw || heightMapDet.getMapSizeY() != dh)
      {
        resizeHeightMapDet(DAGORED2->getConsole());

        if (heights)
        {
          float *hp = heights;
          for (int y = detRectC[0].y; y < detRectC[1].y; y++)
            for (int x = detRectC[0].x; x < detRectC[1].x; x++, hp++)
            {
              heightMapDet.setInitialData(x, y, *hp);
              heightMapDet.setFinalData(x, y, *hp);
            }
        }
        onLandRegionChanged(detRectC[0].x / detDivisor, detRectC[0].y / detDivisor, detRectC[1].x / detDivisor,
          detRectC[1].y / detDivisor);
        applyHmModifiers();
        heightMapDet.flushData();
      }
    }
    else
      heightMapDet.closeFile();

    if (heights)
      midmem->free(heights);

    updateHeightMapTex(true);
    updateHeightMapConstants();
    panel->setEnabledById(PID_GRID_H2_APPLY, false);
  }
  else if (pcb_id == PID_NAVMESH_BUILD)
  {
    BinDumpSaveCB cwr(1 << 10, _MAKE4C('PC'), false);
    buildAndWriteSingleNavMesh(cwr, shownExportedNavMeshIdx, false);
  }
  gpuGrassPanel.onClick(
    pcb_id, panel, [this]() { loadGPUGrassFromLevelBlk(); },
    [this]() {
      if (propPanel)
        propPanel->fillPanel();
    });

  if (pcb_id >= PID_NAVMESH_START && pcb_id <= PID_NAVMESH_END)
  {
    const int navMeshIdx = (pcb_id - PID_NAVMESH_START) / NM_PARAMS_COUNT;
    navmeshAreasProcessing[navMeshIdx].onClick(pcb_id);
  }
}

void HmapLandPlugin::processTexName(SimpleString &out, const char *in)
{
  if (!in || !in[0])
  {
    out = NULL;
    return;
  }

  String asset(DagorAsset::fpath2asset(in));
  DagorAsset *a = DAEDITOR3.getAssetByName(asset, DAEDITOR3.getAssetTypeId("tex"));

  if (a)
    out = a->getTargetFilePath();
  else
  {
    DAEDITOR3.conError("cant find det texture asset: %s", asset);
    out = in;
  }
}

bool HmapLandPlugin::setDetailTexSlot(int slot, const char *blk_name)
{
  // debug("setDetailTexSlot %d. %s. %s. %s. %s %.3f", slot, det, det_n, side, side_n, tile);
  if (slot < 0 || getDetLayerDesc().checkOverflow(slot))
    return false;

  if (numDetailTextures < slot + 1)
    numDetailTextures = slot + 1;

  while (detailTexBlkName.size() < slot + 1)
    detailTexBlkName.push_back() = "";

  detailTexBlkName[slot] = blk_name ? blk_name : "";
  resetRenderer();

  // debug("setDetailTexSlot[%d]: <%s>", slot, (char*)det);
  return true;
}
// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


HmapLandPlugin::ScriptImage::ScriptImage(const char *n) : isModified(false), isSaved(false), name(n), fileNotifyId(-1)
{
  memset(bitsTile, 0, sizeof(bitsTile));
  w = h = 1;
  bpp = bmStride = 0;
  tileSz = 1;
  wTiles = 1;

  IFileChangeTracker *ftracker = DAGORED2->queryEditorInterface<IFileChangeTracker>();
  if (ftracker) // register file notify
  {
    char fullPath[260];
    String projPath;
    DAGORED2->getProjectFolderPath(projPath);
    if (n[0] != '*')
      SNPRINTF(fullPath, sizeof(fullPath), "%s/%s/%s.tif", projPath.str(), HmapLandPlugin::self->getInternalName(), n);
    else
      SNPRINTF(fullPath, sizeof(fullPath), "%s/%s.tif", projPath.str(), n + 1);
    G_ASSERT(fileNotifyId < 0);
    fileNotifyId = ftracker->addFileChangeTracking(fullPath);
    if (fileNotifyId >= 0)
      ftracker->subscribeUpdateNotify(fileNotifyId, this);
    else
      debug("can't find file '%s' to track changes from", fullPath);
  }
}


HmapLandPlugin::ScriptImage::~ScriptImage()
{
  if (fileNotifyId >= 0)
  {
    IFileChangeTracker *ftracker = DAGORED2->queryEditorInterface<IFileChangeTracker>();
    if (ftracker)
      ftracker->unsubscribeUpdateNotify(fileNotifyId, this);
  }
  bitMaskImgMgr->destroyImage(*this);
}


bool HmapLandPlugin::ScriptImage::loadImage()
{
  if (!name.length())
    return false;

  if (getBitsPerPixel() && isModified)
    return false;

  bitMaskImgMgr->destroyImage(*this);
  if (name[0] == '*')
    return bitMaskImgMgr->loadImage(*this, NULL, &name[1]);
  return bitMaskImgMgr->loadImage(*this, HmapLandPlugin::self->getInternalName(), name);
}


bool HmapLandPlugin::ScriptImage::saveImage()
{
  if (!name.length())
    return true;

  if (!getBitsPerPixel() || !isModified)
    return true;

  if (!bitMaskImgMgr->saveImage(*this, HmapLandPlugin::self->getInternalName(), name))
    return false;

  isModified = false;
  isSaved = true;
  return true;
}

void HmapLandPlugin::ScriptImage::onFileChanged(int)
{
  if (isSaved)
    isSaved = false; // image was saved by editor, ignore it
  else
  {
    debug("file '%s' changed, reloading...", getName());
    if (isImageModified())
      DAEDITOR3.conWarning("overwrite modified image '%s' from disk", getName());
    isModified = false;
    loadImage();
  }
}

E3DCOLOR HmapLandPlugin::ScriptImage::sampleImageUV(real fx, real fy, bool clamp_x, bool clamp_y)
{
  if (!getBitsPerPixel() || !isImage32())
    return E3DCOLOR(255, 255, 255, 0);

  fy = 1 - fy;

  fx *= getWidth();
  fy *= getHeight();

  int ix = int(floorf(fx));
  int iy = int(floorf(fy));

  fx -= ix;
  fy -= iy;

  if (clamp_x)
  {
    if (ix < 0)
    {
      ix = 0;
      fx = 0;
    }
    else if (ix >= getWidth() - 1)
    {
      ix = getWidth() - 1;
      fx = 0;
    }
  }
  else
  {
    ix %= getWidth();
    if (ix < 0)
      ix += getWidth();
  }

  if (clamp_y)
  {
    if (iy < 0)
    {
      iy = 0;
      fy = 0;
    }
    else if (iy >= getHeight() - 1)
    {
      iy = getHeight() - 1;
      fy = 0;
    }
  }
  else
  {
    iy %= getHeight();
    if (iy < 0)
      iy += getHeight();
  }

  int nx = ix + 1;
  if (nx >= getWidth())
    nx = 0;
  int ny = iy + 1;
  if (ny >= getHeight())
    ny = 0;

  E3DCOLOR c00 = getImagePixel(ix, iy);
  E3DCOLOR c01 = getImagePixel(nx, iy);
  E3DCOLOR c10 = getImagePixel(ix, ny);
  E3DCOLOR c11 = getImagePixel(nx, ny);

  int r = real2int((c00.r * (1 - fx) + c01.r * fx) * (1 - fy) + (c10.r * (1 - fx) + c11.r * fx) * fy);
  int g = real2int((c00.g * (1 - fx) + c01.g * fx) * (1 - fy) + (c10.g * (1 - fx) + c11.g * fx) * fy);
  int b = real2int((c00.b * (1 - fx) + c01.b * fx) * (1 - fy) + (c10.b * (1 - fx) + c11.b * fx) * fy);

  if (r < 0)
    r = 0;
  else if (r > 255)
    r = 255;

  if (g < 0)
    g = 0;
  else if (g > 255)
    g = 255;

  if (b < 0)
    b = 0;
  else if (b > 255)
    b = 255;

  return E3DCOLOR(r, g, b, 0);
}


E3DCOLOR HmapLandPlugin::ScriptImage::sampleImagePixelUV(real fx, real fy, bool clamp_x, bool clamp_y)
{
  if (!getBitsPerPixel() || !isImage32())
    return E3DCOLOR(255, 255, 255, 0);

  fy = 1 - fy;

  fx *= getWidth();
  fy *= getHeight();

  int ix = real2int(fx);
  int iy = real2int(fy);

  fx -= ix;
  fy -= iy;

  if (clamp_x)
  {
    if (ix < 0)
    {
      ix = 0;
      fx = 0;
    }
    else if (ix >= getWidth() - 1)
    {
      ix = getWidth() - 1;
      fx = 0;
    }
  }
  else
  {
    ix %= getWidth();
    if (ix < 0)
      ix += getWidth();
  }

  if (clamp_y)
  {
    if (iy < 0)
    {
      iy = 0;
      fy = 0;
    }
    else if (iy >= getHeight() - 1)
    {
      iy = getHeight() - 1;
      fy = 0;
    }
  }
  else
  {
    iy %= getHeight();
    if (iy < 0)
      iy += getHeight();
  }

  return getImagePixel(ix, iy);
}

inline void HmapLandPlugin::ScriptImage::calcClampMapping(float &fx, float &fy, int &ix, int &iy, int &nx, int &ny)
{
  fy = 1 - fy;

  fx *= getWidth();
  fy *= getHeight();

  ix = int(floorf(fx));
  iy = int(floorf(fy));

  fx -= ix;
  fy -= iy;

  if (ix < 0)
  {
    ix = 0;
    fx = 0;
  }
  else if (ix >= getWidth() - 1)
  {
    ix = getWidth() - 1;
    fx = 0;
  }

  if (iy < 0)
  {
    iy = 0;
    fy = 0;
  }
  else if (iy >= getHeight() - 1)
  {
    iy = getHeight() - 1;
    fy = 0;
  }

  nx = ix + 1;
  if (nx >= getWidth())
    nx = 0;
  ny = iy + 1;
  if (ny >= getHeight())
    ny = 0;
}
inline void HmapLandPlugin::ScriptImage::calcMapping(float &fx, float &fy, int &ix, int &iy, int &nx, int &ny, bool clamp_u,
  bool clamp_v)
{
  fy = 1 - fy;

  fx *= getWidth();
  fy *= getHeight();

  ix = int(floorf(fx));
  iy = int(floorf(fy));

  fx -= ix;
  fy -= iy;

  if (clamp_u)
  {
    if (ix < 0)
    {
      ix = 0;
      fx = 0;
    }
    else if (ix >= getWidth() - 1)
    {
      ix = getWidth() - 1;
      fx = 0;
    }
  }
  else
  {
    ix %= getWidth();
    if (ix < 0)
      ix += getWidth();
  }

  if (clamp_v)
  {
    if (iy < 0)
    {
      iy = 0;
      fy = 0;
    }
    else if (iy >= getHeight() - 1)
    {
      iy = getHeight() - 1;
      fy = 0;
    }
  }
  else
  {
    iy %= getHeight();
    if (iy < 0)
      iy += getHeight();
  }

  nx = ix + 1;
  if (nx >= getWidth())
    nx = 0;
  ny = iy + 1;
  if (ny >= getHeight())
    ny = 0;
}
inline void HmapLandPlugin::ScriptImage::calcClampMapping(float fx0, float fy0, int &ix0, int &iy0)
{
  fy0 = 1 - fy0;

  fx0 *= getWidth();
  fy0 *= getHeight();

  ix0 = real2int(fx0);
  iy0 = real2int(fy0);

  if (ix0 < 0)
    ix0 = 0;
  else if (ix0 >= getWidth() - 1)
    ix0 = getWidth() - 1;

  if (iy0 < 0)
    iy0 = 0;
  else if (iy0 >= getHeight() - 1)
    iy0 = getHeight() - 1;
}


float HmapLandPlugin::ScriptImage::sampleMask1UV(real fx, real fy)
{
  if (getBitsPerPixel() != 1)
    return 1.0;

  int ix, iy, nx, ny;
  calcClampMapping(fx, fy, ix, iy, nx, ny);

  float c00 = getMaskPixel1(ix, iy) ? 1.0 : 0.0;
  float c01 = getMaskPixel1(nx, iy) ? 1.0 : 0.0;
  float c10 = getMaskPixel1(ix, ny) ? 1.0 : 0.0;
  float c11 = getMaskPixel1(nx, ny) ? 1.0 : 0.0;

  return (c00 * (1 - fx) + c01 * fx) * (1 - fy) + (c10 * (1 - fx) + c11 * fx) * fy;
}

float HmapLandPlugin::ScriptImage::sampleMask1UV(real fx, real fy, bool clamp_u, bool clamp_v)
{
  if (getBitsPerPixel() != 1)
    return 1.0;

  int ix, iy, nx, ny;
  calcMapping(fx, fy, ix, iy, nx, ny, clamp_u, clamp_v);

  float c00 = getMaskPixel1(ix, iy) ? 1.0 : 0.0;
  float c01 = getMaskPixel1(nx, iy) ? 1.0 : 0.0;
  float c10 = getMaskPixel1(ix, ny) ? 1.0 : 0.0;
  float c11 = getMaskPixel1(nx, ny) ? 1.0 : 0.0;

  return (c00 * (1 - fx) + c01 * fx) * (1 - fy) + (c10 * (1 - fx) + c11 * fx) * fy;
}

float HmapLandPlugin::ScriptImage::sampleMask8UV(real fx, real fy)
{
  if (getBitsPerPixel() != 8)
    return 1.0;

  int ix, iy, nx, ny;
  calcClampMapping(fx, fy, ix, iy, nx, ny);

  float c00 = getMaskPixel8(ix, iy) / 255.0;
  float c01 = getMaskPixel8(nx, iy) / 255.0;
  float c10 = getMaskPixel8(ix, ny) / 255.0;
  float c11 = getMaskPixel8(nx, ny) / 255.0;

  return (c00 * (1 - fx) + c01 * fx) * (1 - fy) + (c10 * (1 - fx) + c11 * fx) * fy;
}

float HmapLandPlugin::ScriptImage::sampleMask8UV(real fx, real fy, bool clamp_u, bool clamp_v)
{
  if (getBitsPerPixel() != 8)
    return 1.0;

  int ix, iy, nx, ny;
  calcMapping(fx, fy, ix, iy, nx, ny, clamp_u, clamp_v);

  float c00 = getMaskPixel8(ix, iy) / 255.0;
  float c01 = getMaskPixel8(nx, iy) / 255.0;
  float c10 = getMaskPixel8(ix, ny) / 255.0;
  float c11 = getMaskPixel8(nx, ny) / 255.0;

  return (c00 * (1 - fx) + c01 * fx) * (1 - fy) + (c10 * (1 - fx) + c11 * fx) * fy;
}

float HmapLandPlugin::ScriptImage::sampleMask1PixelUV(real fx, real fy)
{
  if (getBitsPerPixel() != 1)
    return 1.0;

  int ix, iy;
  calcClampMapping(fx, fy, ix, iy);
  return getMaskPixel1(ix, iy) ? 1.0 : 0.0;
}

float HmapLandPlugin::ScriptImage::sampleMask8PixelUV(real fx, real fy)
{
  if (getBitsPerPixel() != 8)
    return 1.0;

  int ix, iy;
  calcClampMapping(fx, fy, ix, iy);
  return getMaskPixel8(ix, iy) / 255.0;
}

void HmapLandPlugin::ScriptImage::paintImageUV(real fx0, real fy0, real fx1, real fy1, bool clamp_x, bool clamp_y, E3DCOLOR color)
{
  G_UNUSED(fx1);
  G_UNUSED(fy1);

  if (!getBitsPerPixel() || !isImage32())
    return;

  fy0 = 1 - fy0;

  fx0 *= getWidth();
  fy0 *= getHeight();

  int ix0 = real2int(fx0);
  int iy0 = real2int(fy0);

  if (clamp_x)
  {
    if (ix0 < 0)
    {
      ix0 = 0;
    }
    else if (ix0 >= getWidth() - 1)
      ix0 = getWidth() - 1;
  }
  else
  {
    ix0 %= getWidth();
    if (ix0 < 0)
      ix0 += getWidth();
  }

  if (clamp_y)
  {
    if (iy0 < 0)
    {
      iy0 = 0;
    }
    else if (iy0 >= getHeight() - 1)
      iy0 = getHeight() - 1;
  }
  else
  {
    iy0 %= getHeight();
    if (iy0 < 0)
      iy0 += getHeight();
  }

  setImagePixel(ix0, iy0, color);

  isModified = true;
}

void HmapLandPlugin::ScriptImage::paintMask1UV(real fx0, real fy0, bool val)
{
  if (getBitsPerPixel() != 1)
    return;

  int ix0, iy0;
  calcClampMapping(fx0, fy0, ix0, iy0);
  setMaskPixel1(ix0, iy0, val ? 0x80 : 0);

  isModified = true;
}
void HmapLandPlugin::ScriptImage::paintMask8UV(real fx0, real fy0, char val)
{
  if (getBitsPerPixel() != 8)
    return;

  int ix0, iy0;
  calcClampMapping(fx0, fy0, ix0, iy0);
  setMaskPixel8(ix0, iy0, val);

  isModified = true;
}

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void HmapLandPlugin::updateScriptImageList()
{
  String fname;

  for (const alefind_t &ff : dd_find_iterator(DAGORED2->getPluginFilePath(this, "*.tif"), DA_FILE))
  {
    fname = ::get_file_name_wo_ext(ff.name);
    if (stricmp(fname, "importanceMask_lc") == 0) // skip temporary files
      continue;

    int i;
    for (i = 0; i < scriptImages.size(); ++i)
      if (stricmp(scriptImages[i]->getName(), fname) == 0)
        break;

    if (i >= scriptImages.size())
      scriptImages.push_back(new ScriptImage(fname));
  }

  for (const alefind_t &ff : dd_find_iterator(DAGORED2->getPluginFilePath(this, "*.tga"), DA_FILE))
  {
    fname = ::get_file_name_wo_ext(ff.name);

    int i;
    for (i = 0; i < scriptImages.size(); ++i)
      if (stricmp(scriptImages[i]->getName(), fname) == 0)
        break;

    if (i >= scriptImages.size())
      scriptImages.push_back(new ScriptImage(fname));
  }
}


const char *HmapLandPlugin::pickScriptImage(const char *current_name, int bpp)
{
  static String stor;
  HmlSelTexDlg dlg(*this, current_name, bpp);
  if (dlg.execute())
    return stor = dlg.getSetTex();
  return NULL;
}


E3DCOLOR HmapLandPlugin::sampleScriptImageUV(int id, real x, real y, bool clamp_x, bool clamp_y)
{
  if (id < 0 || id >= scriptImages.size())
    return E3DCOLOR(255, 255, 255, 0);
  return scriptImages[id]->sampleImageUV(x, y, clamp_x, clamp_y);
}


E3DCOLOR HmapLandPlugin::sampleScriptImagePixelUV(int id, real x, real y, bool clamp_x, bool clamp_y)
{
  if (id < 0 || id >= scriptImages.size())
    return E3DCOLOR(255, 255, 255, 0);
  return scriptImages[id]->sampleImagePixelUV(x, y, clamp_x, clamp_y);
}


void HmapLandPlugin::paintScriptImageUV(int id, real x0, real y0, real x1, real y1, bool clamp_x, bool clamp_y, E3DCOLOR color)
{
  if (id < 0 || id >= scriptImages.size())
    return;
  return scriptImages[id]->paintImageUV(x0, y0, x1, y1, clamp_x, clamp_y, color);
}

float HmapLandPlugin::sampleMask1UV(int id, real x, real y)
{
  if (id < 0 || id >= scriptImages.size())
    return 1.0;
  return scriptImages[id]->sampleMask1UV(x, y);
}
float HmapLandPlugin::sampleMask1UV(int id, real x, real y, bool clamp_u, bool clamp_v)
{
  if (id < 0 || id >= scriptImages.size())
    return 1.0;
  return scriptImages[id]->sampleMask1UV(x, y, clamp_u, clamp_v);
}
float HmapLandPlugin::sampleMask8UV(int id, real x, real y)
{
  if (id < 0 || id >= scriptImages.size())
    return 1.0;
  return scriptImages[id]->sampleMask8UV(x, y);
}
float HmapLandPlugin::sampleMask8UV(int id, real x, real y, bool clamp_u, bool clamp_v)
{
  if (id < 0 || id >= scriptImages.size())
    return 1.0;
  return scriptImages[id]->sampleMask8UV(x, y, clamp_u, clamp_v);
}
float HmapLandPlugin::sampleMask1PixelUV(int id, real x, real y)
{
  if (id < 0 || id >= scriptImages.size())
    return 1.0;
  return scriptImages[id]->sampleMask1PixelUV(x, y);
}
float HmapLandPlugin::sampleMask8PixelUV(int id, real x, real y)
{
  if (id < 0 || id >= scriptImages.size())
    return 1.0;
  return scriptImages[id]->sampleMask8PixelUV(x, y);
}
void HmapLandPlugin::paintMask1UV(int id, real x0, real y0, bool color)
{
  if (id < 0 || id >= scriptImages.size())
    return;
  scriptImages[id]->paintMask1UV(x0, y0, color);
}
void HmapLandPlugin::paintMask8UV(int id, real x0, real y0, char color)
{
  if (id < 0 || id >= scriptImages.size())
    return;
  scriptImages[id]->paintMask8UV(x0, y0, color);
}

bool HmapLandPlugin::saveImage(int id)
{
  if (id < 0 || id >= scriptImages.size())
    return false;
  return scriptImages[id]->saveImage();
}

int HmapLandPlugin::getScriptImageWidth(int id)
{
  if (id < 0 || id >= scriptImages.size())
    return 1;
  return scriptImages[id]->getWidth();
}


int HmapLandPlugin::getScriptImageHeight(int id)
{
  if (id < 0 || id >= scriptImages.size())
    return 1;
  return scriptImages[id]->getHeight();
}

int HmapLandPlugin::getScriptImageBpp(int id)
{
  if (id < 0 || id >= scriptImages.size())
    return 1;
  if (int bpp = scriptImages[id]->getBitsPerPixel())
    return bpp;

  int w, h, bpp;
  if (bitMaskImgMgr->getBitMaskProps(HmapLandPlugin::self->getInternalName(), scriptImages[id]->getName(), w, h, bpp))
    return bpp;
  return 0;
}

void HmapLandPlugin::prepareScriptImage(const char *name, int img_size_mul, int img_bpp)
{
  if (!name || !*name)
    return;

  for (int i = 0; i < scriptImages.size(); ++i)
    if (stricmp(name, scriptImages[i]->getName()) == 0)
      return;

  if (img_size_mul && img_bpp)
  {
    const char *dirname = NULL;
    const char *fname = name;

    if (fname[0] == '*')
      fname++;
    else
      dirname = HmapLandPlugin::self->getInternalName();

    if (!bitMaskImgMgr->checkBitMask(dirname, fname, getHeightmapSizeX() * img_size_mul, getHeightmapSizeY() * img_size_mul, img_bpp))
      getScriptImage(name, img_size_mul, img_bpp);
  }
}
int HmapLandPlugin::getScriptImage(const char *name, int img_size_mul, int img_bpp)
{
  if (!name || !*name)
    return -1;

  int id = -1;
  for (int i = 0; i < scriptImages.size(); ++i)
  {
    if (stricmp(name, scriptImages[i]->getName()) != 0)
      continue;

    scriptImages[i]->loadImage();
    id = i;
  }

  if (img_size_mul && img_bpp)
  {
    int dest_w = getHeightmapSizeX();
    int dest_h = getHeightmapSizeY();
    if (trail_stricmp(name, "_lc"))
      dest_w *= lcmScale, dest_h *= lcmScale;
    else if (trail_stricmp(name, "_dr") && detDivisor)
      dest_w = detRectC[1].x - detRectC[0].x, dest_h = detRectC[1].y - detRectC[0].y;

    if (id == -1)
    {
      id = scriptImages.size();
      scriptImages.push_back(new ScriptImage(name));
    }

    ScriptImage &img = *scriptImages[id];

    if (!img.loadImage() && img_bpp < 32 && img_bpp > 0)
    {
      bitMaskImgMgr->createBitMask(img, dest_w * img_size_mul, dest_h * img_size_mul, img_bpp);
      bitMaskImgMgr->saveImage(img, HmapLandPlugin::self->getInternalName(), name);
    }
    else if (img_bpp < 32)
    {
      int sizeX = dest_w * img_size_mul;
      int sizeY = dest_h * img_size_mul;
      if (img_bpp < 0)
        img_bpp = scriptImages[id]->getBitsPerPixel();

      if (scriptImages[id]->getWidth() != sizeX || scriptImages[id]->getHeight() != sizeY ||
          scriptImages[id]->getBitsPerPixel() != img_bpp)
      {
        bool needToResample = true;

        if (sizeX < scriptImages[id]->getWidth() || sizeY < scriptImages[id]->getHeight() ||
            img_bpp < scriptImages[id]->getBitsPerPixel())
        {
          String msg(512,
            "Mask <%s>, %dx%d %d bpp  requires downsapling.\n"
            "Continue resampling to %dx%d %d bpp?",
            name, scriptImages[id]->getWidth(), scriptImages[id]->getHeight(), scriptImages[id]->getBitsPerPixel(), sizeX, sizeY,
            img_bpp);

          if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_EXCL, "Are you sure?", msg) != wingw::MB_ID_YES)
            needToResample = false;
        }

        if (needToResample)
        {
          if (bitMaskImgMgr->resampleBitMask(img, sizeX, sizeY, img_bpp))
            bitMaskImgMgr->saveImage(img, HmapLandPlugin::self->getInternalName(), name);
        }
      }
    }
  }

  return id;
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void HmapLandPlugin::onBrushPaint(Brush *brush, const Point3 &center, const Point3 &prev_center, const Point3 &normal, int buttons,
  int key_modif)
{
  Point3 c = center - Point3::x0y(editedScriptImage ? esiOrigin : heightMapOffset);
  float cell_sz = editedScriptImage ? esiGridStep : (detDivisor ? gridCellSize / detDivisor : gridCellSize);
  ((HmapLandBrush *)brush)->setHeightmapOffset(editedScriptImage ? esiOrigin : heightMapOffset);
  noTraceNow = true;
  ((HmapLandBrush *)brush)->onBrushPaint(c, prev_center, normal, buttons, key_modif, cell_sz);
  noTraceNow = false;
  if (propPanel && propPanel->getPanelWindow())
    ((HmapLandBrush *)brush)->dynamicItemChange(*propPanel->getPanelWindow());
  updateLandOnPaint(brush, false);
}


void HmapLandPlugin::onRBBrushPaint(Brush *brush, const Point3 &center, const Point3 &prev_center, const Point3 &normal, int buttons,
  int key_modif)
{
  Point3 c = center - Point3::x0y(editedScriptImage ? esiOrigin : heightMapOffset);
  float cell_sz = editedScriptImage ? esiGridStep : (detDivisor ? gridCellSize / detDivisor : gridCellSize);
  ((HmapLandBrush *)brush)->setHeightmapOffset(editedScriptImage ? esiOrigin : heightMapOffset);
  noTraceNow = true;
  ((HmapLandBrush *)brush)->onRBBrushPaint(c, prev_center, normal, buttons, key_modif, cell_sz);
  noTraceNow = false;
  if (propPanel && propPanel->getPanelWindow())
    ((HmapLandBrush *)brush)->dynamicItemChange(*propPanel->getPanelWindow());
  updateLandOnPaint(brush, false);
}


IRoadsProvider::Roads *HmapLandPlugin::getRoadsSnapshot()
{
  if (!roadsSnapshot.get() || roadsSnapshot->getGenTimeStamp() != objEd.generationCount)
  {
    roadsSnapshot = NULL;
    roadsSnapshot = new RoadsSnapshot(objEd);
  }

  roadsSnapshot.addRef();
  return roadsSnapshot.get();
}


void HmapLandPlugin::updateLandOnPaint(Brush *brush, bool finished)
{
  IBBox2 rect;

  if (finished)
    rect = brushFullDirtyBox;
  else
  {
    rect = ((HmapLandBrush *)brush)->getDirtyBox();
    if (detDivisor && !editedScriptImage)
      rect[0] /= detDivisor, rect[1].x = (rect[1].x + detDivisor - 1) / detDivisor,
                             rect[1].y = (rect[1].y + detDivisor - 1) / detDivisor;
    else if (editedScriptImage)
    {
      rect[0].x = (rect[0].x * esiGridStep + esiOrigin.x - heightMapOffset.x) / gridCellSize;
      rect[0].y = (rect[0].y * esiGridStep + esiOrigin.y - heightMapOffset.y) / gridCellSize;
      rect[1].x = (rect[1].x * esiGridStep + esiOrigin.x - heightMapOffset.x - 1) / gridCellSize + 1;
      rect[1].y = (rect[1].y * esiGridStep + esiOrigin.y - heightMapOffset.y - 1) / gridCellSize + 1;
    }
  }

  if (rect.isEmpty())
    return;

  rect[0] -= IPoint2(1, 1);
  rect[1] += IPoint2(1, 1);

  if (editedScriptImage && editedScriptImageIdx >= 0)
  {
    if (!finished)
      updateBlueWhiteMask(&rect);
    return;
  }

  if (finished)
  {
    if (!editedScriptImage && ((HmapLandBrush *)brush) != brushes[SCRIPT_BRUSH])
      recalcLightingInRect(rect);
    generateLandColors(&rect, true);

    updateHeightMapTex(false, &rect);
    if (detDivisor)
    {
      rect[0] *= detDivisor;
      rect[1] *= detDivisor;
    }
    updateHeightMapTex(true, &rect);
    resetTexCacheRect(rect);
  }
  else
  {
    if (editedScriptImage)
      updateBlueWhiteMask(&rect);

    brushFullDirtyBox += rect;
    ((HmapLandBrush *)brush)->resetDirtyBox();

    applyHmModifiers(false, true, false);
    if (!editedScriptImage)
    {
      updateHeightMapTex(false, &rect);
      rect[0] *= detDivisor;
      rect[1] *= detDivisor;
      updateHeightMapTex(true, &rect);
    }
  }
}


void HmapLandPlugin::onBrushPaintEnd(Brush *brush, int buttons, int key_modif)
{
  ((HmapLandBrush *)brush)->onBrushPaintEnd(buttons, key_modif);
  updateLandOnPaint(brush, true);
  objEd.onBrushPaintEnd();
}


void HmapLandPlugin::onRBBrushPaintEnd(Brush *brush, int buttons, int key_modif)
{
  ((HmapLandBrush *)brush)->onRBBrushPaintEnd(buttons, key_modif);
  updateLandOnPaint(brush, true);
}


void HmapLandPlugin::onBrushPaintStart(Brush *brush, int buttons, int key_modif)
{
  brushFullDirtyBox.setEmpty();
  ((HmapLandBrush *)brush)->onBrushPaintStart(buttons, key_modif);
}


void HmapLandPlugin::onRBBrushPaintStart(Brush *brush, int buttons, int key_modif)
{
  brushFullDirtyBox.setEmpty();
  ((HmapLandBrush *)brush)->onRBBrushPaintEnd(buttons, key_modif);
}


void HmapLandPlugin::onLightingSettingsChanged()
{
  if (syncLight)
  {
    getAllSunSettings();
    updateRendererLighting();
  }
  else if (syncDirLight)
  {
    getDirectionSunSettings();
    updateRendererLighting();
  }
  DataBlock level_blk;
  if (waterService && loadLevelSettingsBlk(level_blk))
    updateWaterSettings(level_blk);
}


void HmapLandPlugin::onLandRegionChanged(int x0, int y0, int x1, int y1, bool recalc_all, bool finished)
{
  if (x0 < 0)
    x0 = 0;
  if (y0 < 0)
    y0 = 0;
  if (x1 > landClsMap.getMapSizeX())
    x1 = landClsMap.getMapSizeX();
  if (y1 > landClsMap.getMapSizeY())
    y1 = landClsMap.getMapSizeY();

  if (x0 > x1 || y0 > y1)
    return;

  float fx0, fz0, fx1, fz1;
  fx0 = heightMapOffset.x + gridCellSize * x0 * heightMap.getMapSizeX() / landClsMap.getMapSizeX();
  fz0 = heightMapOffset.y + gridCellSize * y0 * heightMap.getMapSizeY() / landClsMap.getMapSizeY();
  fx1 = heightMapOffset.x + gridCellSize * x1 * heightMap.getMapSizeX() / landClsMap.getMapSizeX();
  fz1 = heightMapOffset.y + gridCellSize * y1 * heightMap.getMapSizeY() / landClsMap.getMapSizeY();
  objEd.onLandRegionChanged(fx0, fz0, fx1, fz1, recalc_all);
  lcMgr->onLandRegionChanged(x0, y0, x1, y1, landClsMap, finished);
  if (hmlService)
    hmlService->invalidateClipmap(false);
  invalidateDistanceField();
}
void HmapLandPlugin::onWholeLandChanged() { onLandRegionChanged(0, 0, landClsMap.getMapSizeX(), landClsMap.getMapSizeY()); }
void HmapLandPlugin::onLandSizeChanged()
{
  genHmap->offset = heightMapOffset;
  genHmap->cellSize = gridCellSize;
  if (brushes.size())
    brushes.back()->setMaxRadius(1000 * gridCellSize);
  lcMgr->onLandSizeChanged(gridCellSize * heightMap.getMapSizeX() / landClsMap.getMapSizeX(), heightMapOffset.x, heightMapOffset.y,
    landClsLayer);
  invalidateDistanceField();
}

/*void HmapLandPlugin::prepareDetailTexTileAndOffset(Tab<float> &out_tile, Tab<Point2> &out_offset)
{
  out_tile = detailTexTile;
  out_offset = detailTexOffset;

  for ( int i = 0; i < out_tile.size(); i ++ )
  {
    if ( out_tile[i] < 0)
      out_tile[i] = -render.detailTile/out_tile[i];
    //else
    //  out_tile[i] = -render.detailTile;
  }
}*/

#if defined(USE_HMAP_ACES)
void HmapLandPlugin::getEnvironmentSettings(DataBlock &blk)
{
  blk.clearData();
  ldrLight.save("", blk);
  blk.setReal("sunAzimuth", sunAzimuth);
  blk.setReal("sunZenith", sunZenith);
}

void HmapLandPlugin::setEnvironmentSettings(DataBlock &blk)
{
  ldrLight.load("", blk);
  sunAzimuth = blk.getReal("sunAzimuth", sunAzimuth);
  sunZenith = blk.getReal("sunZenith", sunZenith);
  resetRenderer();
  if (DAGORED2->curPlugin() == this && propPanel)
    propPanel->updateLightGroup();
}
#endif


void HmapLandPlugin::setSelectMode() { objEd.setEditMode(CM_OBJED_MODE_SELECT); }
// IPostProcessGeometry
void HmapLandPlugin::processGeometry(StaticGeometryContainer &container)
{
  DataBlock app_blk(DAGORED2->getWorkspace().getAppPath());
  const char *mgr_type = app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("hmap")->getStr("type", NULL);
  bool snow = (!mgr_type || strcmp(mgr_type, "aces") != 0);
  if (snow)
    objEd.calcSnow(container);
  else
    removeInvisibleFaces(container);
}


//------------------------------------------------------------------------------------
//---------------------------------------------------------GRASS
//------------------------------------------------------------------------------------

// Layers manipulations

void HmapLandPlugin::addGrassLayer(const char *name)
{
  int layerId = HmapLandPlugin::grassService->addDefaultLayer();
  if (layerId != -1)
  {
    GrassLayerPanel *grass_layer = new GrassLayerPanel(name, layerId);
    grassLayers.push_back(grass_layer);
  }
}

GrassLayerPanel::GrassLayerPanel(const char *name, int layer_i) :
  grass_layer_i(layer_i), layerName(name), newLayerGrp(NULL), pidBase(0), remove_this_layer(false)
{}


// Grass interface

void GrassLayerPanel::fillParams(PropPanel::ContainerPropertyControl *parent_panel, int &pid)
{
  GrassLayerInfo *layerProps = HmapLandPlugin::grassService->getLayerInfo(grass_layer_i);
  if (!layerProps)
    return;

  pidBase = pid;
  newLayerGrp = parent_panel->createGroup(PID_GRASS_LAYER_GRP + grass_layer_i, layerName.str());

  newLayerGrp->createButton(GL_PID_ASSET_NAME + pid, String(128, "AssetName: %s", layerProps->resName.str()));

  newLayerGrp->createStatic(0, "Mask:");
  newLayerGrp->createCheckBox(GL_PID_MASK_R + pid, "r", (layerProps->bitMask & 1) ? true : false, true, false);
  newLayerGrp->createCheckBox(GL_PID_MASK_G + pid, "g", (layerProps->bitMask & 2) ? true : false, true, false);
  newLayerGrp->createCheckBox(GL_PID_MASK_B + pid, "b", (layerProps->bitMask & 4) ? true : false, true, false);

  newLayerGrp->createEditFloat(GL_PID_DENSITY + pid, "Density", layerProps->density);
  newLayerGrp->setMinMaxStep(GL_PID_DENSITY + pid, 0.f, 5.f, 0.01);

  newLayerGrp->createStatic(0, "Horizontal scale:");
  newLayerGrp->createEditFloat(GL_PID_HORIZONTAL_SCALE_MIN + pid, "min", layerProps->minScale.x);
  newLayerGrp->setMinMaxStep(GL_PID_HORIZONTAL_SCALE_MIN + pid, 0.f, 10.f, 0.01);
  newLayerGrp->createEditFloat(GL_PID_HORIZONTAL_SCALE_MAX + pid, "max", layerProps->maxScale.x, 2, true, false);
  newLayerGrp->setMinMaxStep(GL_PID_HORIZONTAL_SCALE_MAX + pid, 0.f, 10.f, 0.01);

  newLayerGrp->createStatic(0, "Vertical scale:");
  newLayerGrp->createEditFloat(GL_PID_VERTICAL_SCALE_MIN + pid, "min", layerProps->minScale.y);
  newLayerGrp->setMinMaxStep(GL_PID_VERTICAL_SCALE_MIN + pid, 0.f, 10.f, 0.01);
  newLayerGrp->createEditFloat(GL_PID_VERTICAL_SCALE_MAX + pid, "max", layerProps->maxScale.y, 2, true, false);
  newLayerGrp->setMinMaxStep(GL_PID_VERTICAL_SCALE_MAX + pid, 0.f, 10.f, 0.01);
  newLayerGrp->createEditFloat(GL_PID_WIND_MUL + pid, "Wind mul.", layerProps->windMul);
  newLayerGrp->createEditFloat(GL_PID_RADIUS_MUL + pid, "Radius mul.", layerProps->radiusMul);


  newLayerGrp->createStatic(0, "");

  newLayerGrp->createColorBox(GL_PID_GRASS_RED_COLOR_FROM + pid, "Red color (from):", e3dcolor(layerProps->colors[0].start));
  newLayerGrp->createColorBox(GL_PID_GRASS_RED_COLOR_TO + pid, "Red color (to):", e3dcolor(layerProps->colors[0].end));

  newLayerGrp->createColorBox(GL_PID_GRASS_GREEN_COLOR_FROM + pid, "Green color (from):", e3dcolor(layerProps->colors[1].start));
  newLayerGrp->createColorBox(GL_PID_GRASS_GREEN_COLOR_TO + pid, "Green color (to):", e3dcolor(layerProps->colors[1].end));

  newLayerGrp->createColorBox(GL_PID_GRASS_BLUE_COLOR_FROM + pid, "Blue color (from):", e3dcolor(layerProps->colors[2].start));
  newLayerGrp->createColorBox(GL_PID_GRASS_BLUE_COLOR_TO + pid, "Blue color (to):", e3dcolor(layerProps->colors[2].end));

  newLayerGrp->createButton(GL_PID_REMOVE_LAYER + pid, "Remove layer");

  pid += GL_PID_ELEM_COUNT;

  parent_panel->setBool(PID_GRASS_LAYER_GRP + grass_layer_i, true);
}

void GrassLayerPanel::onClick(int pid, PropPanel::ContainerPropertyControl &p)
{
  int detailTypePid = pid - pidBase;

  if (detailTypePid == GL_PID_ASSET_NAME)
  {
    String fname =
      wingw::file_open_dlg(NULL, "Select grass layer resource", "Grass blades (*.dag)|*.dag|All files (*.*)|*.*", "dag", "");

    if (!fname.length())
      return;

    String resName = ::get_file_name_wo_ext(fname);
    String corrected_resName;
    corrected_resName.setStr(resName.str(), resName.length() - 6); // cut off file extention

    p.setCaption(pid, *corrected_resName.str() ? corrected_resName.str() : "AssetName:");

    HmapLandPlugin::grassService->changeLayerResource(grass_layer_i, corrected_resName.str());
  }
  else if (detailTypePid == GL_PID_REMOVE_LAYER)
  {
    remove_this_layer = true;
  }
}

bool GrassLayerPanel::onPPChangeEx(int pid, PropPanel::ContainerPropertyControl &p)
{
  if (pid < pidBase || pid >= pidBase + GL_PID_ELEM_COUNT)
    return false;
  onPPChange(pid, p);
  return true;
}

void GrassLayerPanel::onPPChange(int pid, PropPanel::ContainerPropertyControl &panel)
{
  GrassLayerInfo *layerProps = HmapLandPlugin::grassService->getLayerInfo(grass_layer_i);
  if (!layerProps)
    return;

  int detailTypePid = pid - pidBase;

  switch (detailTypePid)
  {
    case GL_PID_MASK_R:
      layerProps->bitMask = panel.getBool(pid) ? (layerProps->bitMask | 1ULL) : (layerProps->bitMask & (~1ULL));
      break;
    case GL_PID_MASK_G:
      layerProps->bitMask = panel.getBool(pid) ? (layerProps->bitMask | 2ULL) : (layerProps->bitMask & (~2ULL));
      break;
    case GL_PID_MASK_B:
      layerProps->bitMask = panel.getBool(pid) ? (layerProps->bitMask | 4ULL) : (layerProps->bitMask & (~4ULL));
      break;

    case GL_PID_DENSITY:
      layerProps->density = panel.getFloat(pid);
      layerProps->resetLayerVB = true;
      HmapLandPlugin::grassService->setLayerDensity(grass_layer_i, layerProps->density);
      break;

    case GL_PID_HORIZONTAL_SCALE_MIN:
      layerProps->minScale.x = panel.getFloat(pid);
      layerProps->resetLayerVB = true;
      HmapLandPlugin::grassService->updateLayerVbo(grass_layer_i);
      break;
    case GL_PID_HORIZONTAL_SCALE_MAX:
      layerProps->maxScale.x = panel.getFloat(pid);
      layerProps->resetLayerVB = true;
      HmapLandPlugin::grassService->updateLayerVbo(grass_layer_i);
      break;


    case GL_PID_VERTICAL_SCALE_MIN:
      layerProps->minScale.y = panel.getFloat(pid);
      layerProps->resetLayerVB = true;
      HmapLandPlugin::grassService->updateLayerVbo(grass_layer_i);
      break;
    case GL_PID_VERTICAL_SCALE_MAX:
      layerProps->maxScale.y = panel.getFloat(pid);
      layerProps->resetLayerVB = true;
      HmapLandPlugin::grassService->updateLayerVbo(grass_layer_i);
      break;


    case GL_PID_GRASS_RED_COLOR_FROM: layerProps->colors[0].start = color4(panel.getColor(pid)); break;
    case GL_PID_GRASS_RED_COLOR_TO: layerProps->colors[0].end = color4(panel.getColor(pid)); break;

    case GL_PID_GRASS_GREEN_COLOR_FROM: layerProps->colors[1].start = color4(panel.getColor(pid)); break;
    case GL_PID_GRASS_GREEN_COLOR_TO: layerProps->colors[1].end = color4(panel.getColor(pid)); break;

    case GL_PID_GRASS_BLUE_COLOR_FROM: layerProps->colors[2].start = color4(panel.getColor(pid)); break;
    case GL_PID_GRASS_BLUE_COLOR_TO: layerProps->colors[2].end = color4(panel.getColor(pid)); break;

    case GL_PID_WIND_MUL: layerProps->windMul = panel.getFloat(pid); break;
    case GL_PID_RADIUS_MUL: layerProps->radiusMul = panel.getFloat(pid); break;
  };
}


// Save & load grass

bool HmapLandPlugin::loadGrassLayers(const DataBlock &blk, bool update_grass_blk)
{
  enableGrass = blk.getBool("enableGrass", enableGrass);
  const Tab<LandClassDetailTextures> &landClasses = landMeshRenderer->getLandClasses();
  grass.masks.clear();
  for (int i = 0; i < landClasses.size(); ++i)
  {
    String detailName(grassService->getResName(landClasses[i].colormapId));
    if (detailName.length() > 0 && detailName[detailName.length() - 1] == '*')
      detailName[detailName.length() - 1] = 0;
    grass.masks.emplace_back(detailName, String{blk.getStr(detailName, "grass_mask_black")});
  }
  grass.defaultMinDensity = blk.getReal("minLodDensity", 0.2f);
  grass.defaultMaxDensity = blk.getReal("density", 0.75f);
  grass.maxRadius = blk.getReal("maxRadius", 400.f);

  grass.texSize = blk.getInt("texSize", 256);
  grass.sowPerlinFreq = blk.getReal("sowPerlinFreq", 0.1);
  grass.fadeDelta = blk.getReal("fadeDelta", 100);
  grass.lodFadeDelta = blk.getReal("lodFadeDelta", 30);
  grass.blendToLandDelta = blk.getReal("blendToLandDelta", 100);

  grass.noise_first_rnd = blk.getReal("noise_first_rnd", 23.1406926327792690);
  grass.noise_first_val_mul = blk.getReal("noise_first_val_mul", 1);
  grass.noise_first_val_add = blk.getReal("noise_first_val_add", 0);

  grass.noise_second_rnd = blk.getReal("noise_second_rnd", 0.05);
  grass.noise_second_val_mul = blk.getReal("noise_second_val_mul", 1);
  grass.noise_second_val_add = blk.getReal("noise_second_val_add", 0);

  grass.directWindMul = blk.getReal("directWindMul", 1);
  grass.noiseWindMul = blk.getReal("noiseWindMul", 1);
  grass.windPerlinFreq = blk.getReal("windPerlinFreq", 50000);
  grass.directWindFreq = blk.getReal("directWindFreq", 1);
  grass.directWindModulationFreq = blk.getReal("directWindModulationFreq", 1);
  grass.windWaveLength = blk.getReal("windWaveLength", 100);
  grass.windToColor = blk.getReal("windToColor", 0.2);
  grass.helicopterHorMul = blk.getReal("helicopterHorMul", 2);
  grass.helicopterVerMul = blk.getReal("helicopterVerMul", 1);
  grass.helicopterToDirectWindMul = blk.getReal("helicopterToDirectWindMul", 2);
  grass.helicopterFalloff = blk.getReal("helicopterFalloff", 0.001);

  int nid = blk.getNameId("layer");
  grassLayers.clear();

  for (int i = 0, idx = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      String defLayerName(16, "layer_%i", i);
      const char *layerName = blk.getBlock(i)->getStr("layerName", defLayerName.str());
      GrassLayerPanel *l = new GrassLayerPanel(layerName, i);
      grassLayers.push_back(l);
    }

  // set grass mask
  resetGrassMask(blk);

  // create copy from blk
  if (update_grass_blk)
  {
    delete grassBlk;
    grassBlk = new DataBlock(blk);
  }
  hmlService->setGrassBlk(grassBlk);
  return true;
}

bool HmapLandPlugin::loadGrassFromBlk(const DataBlock &level_blk)
{
  const DataBlock *blk = level_blk.getBlockByName("randomGrass");
  if (!blk)
    return false;
  enableGrass = true;
  delete grassBlk;
  grassBlk = new DataBlock(*blk);
  return true;
}

void HmapLandPlugin::loadGrassFromLevelBlk()
{
  if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Overwrite grass settings?") != wingw::MB_ID_YES)
    return;

  DataBlock level_blk;
  if (loadLevelSettingsBlk(level_blk))
    if (loadGrassFromBlk(level_blk))
    {
      HmapLandPlugin::grassService->reloadAll(*grassBlk, level_blk);
      bool savedEnableGrass = enableGrass; // assume that this flag always false in level.blk - save it
      loadGrassLayers(*grassBlk, false);
      enableGrass = savedEnableGrass;
    }
}

bool HmapLandPlugin::loadGPUGrassFromBlk(const DataBlock &level_blk)
{
  const DataBlock *gpuGrassBlock = level_blk.getBlockByNameEx("grass", nullptr);
  if (!gpuGrassBlock)
    return false;
  enableGrass = true;
  delete gpuGrassBlk;
  gpuGrassBlk = new DataBlock(*gpuGrassBlock);
  return true;
}

void HmapLandPlugin::loadGPUGrassFromLevelBlk()
{
  DataBlock level_blk;
  bool savedEnableGrass = gpuGrassService->isGrassEnabled();
  if (loadLevelSettingsBlk(level_blk))
  {
    loadGPUGrassFromBlk(level_blk);
    gpuGrassService->enableGrass(savedEnableGrass);
    if (propPanel)
      propPanel->fillPanel();
  }
}

void HmapLandPlugin::onLandClassAssetTexturesChanged(landclass::AssetData *data) { resetLandmesh(); }

void HmapLandPlugin::onObjectSelectionChanged(RenderableEditableObject *obj)
{
  if (SplineObject *spline = RTTI_cast<SplineObject>(obj))
  {
    int navmeshIdx = spline->getProps().navmeshIdx;
    if (navmeshIdx >= 0)
      navmeshAreasProcessing[navmeshIdx].onSplineSelectionChanged(spline);
  }
}

void HmapLandPlugin::onObjectsRemove()
{
  for (int i = 0; i < navmeshAreasProcessing.size(); i++)
  {
    int areaType = navMeshProps[i].getInt("navArea", 0);
    if (areaType == NM_AREATYPE_POLY)
      navmeshAreasProcessing[i].onObjectsRemove();
  }
}
