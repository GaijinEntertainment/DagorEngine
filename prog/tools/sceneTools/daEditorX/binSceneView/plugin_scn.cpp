// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "plugin_scn.h"
#include "strmlevel.h"
#include <sceneRay/dag_sceneRay.h>
#include <rendInst/rendInstGen.h>
#include <heightmap/heightmapHandler.h>
#include <pathFinder/pathFinder.h>
#include <workCycle/dag_delayedAction.h>

#include <oldEditor/de_cm.h>
#include <debug/dag_debug.h>
#include <de3_interface.h>
#include <de3_editorEvents.h>
#include <de3_entityFilter.h>
#include <de3_hmapDebugShadingService.h>

#include <libTools/util/strUtil.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/panelWindow.h>
#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_wndPublic.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <perfMon/dag_visClipMesh.h>
#include <scene/dag_physMat.h>
#include <osApiWrappers/dag_direct.h>
#include <drv/3d/dag_info.h>

using hdpi::_pxScaled;

extern bool de3_late_scene_select;
static int navmeshSubtypeMask = -1;

enum
{
  CM_TOOL = CM_PLUGIN_BASE + 0,
  CM_SHOW_PANEL,
  PID_SET_BIN_DUMP,
  PID_SHOW_SPLINES,
  PID_SHOW_SPLINE_PTS,
  PID_SHOW_NAVMESH,
  PID_SHOW_COLLISION,
  PID_SHOW_STATIC_GEOM,
  PID_SHOW_LANDMESH,

  PID_PANEL_VISIBILITY_SETTINGS_GROUP,
  PID_PANEL_SHOW_STATIC_GEOM,
  PID_PANEL_SHOW_LANDMESH,
  PID_PANEL_USE_LAND_MIRRORING,
  PID_PANEL_SHOW_NAVMESH,
  PID_PANEL_SHOW_SPLINES,
  PID_PANEL_SHOW_SPLINE_PTS,
  PID_PANEL_SPLINE_POINT_VISIBILITY_RANGE,
  PID_PANEL_SHOW_STATIC_COLLISION,
  PID_PANEL_SHOW_LAND_COLLISION,
  PID_PANEL_COLLISION_VISIBILITY_RANGE,
  PID_PANEL_SHOW_WIREFRAME_COLLISION,

  PID_PANEL_LANDSCAPE_DEBUG_SHADING_GROUP,
  PID_PANEL_LANDSCAPE_DEBUG_SHADING_START,
  PID_PANEL_LANDSCAPE_DEBUG_SHADING_END = PID_PANEL_LANDSCAPE_DEBUG_SHADING_START + IHmapDebugShadingService::REQUIRED_PROPERTY_IDS,
};

namespace EditorCommandIds
{

static constexpr const char *SHOW_PANEL = "Plugin.SceneView.TogglePropertiesPanel";

} // namespace EditorCommandIds

enum
{
  PROPBAR_WIDTH = 280,
  PROPBAR_SCENE_VIEW_WTYPE = 190,
};

BinSceneViewPlugin::BinSceneViewPlugin() : isVisible(false), streamingScene(NULL)
{
  isDebugVisible = false;
  showSplines = true;
  showSplinePoints = true;
  maxPointVisDist = 5000;
  showNavMesh = true;
  showFrt = false;
  showFrtWire = false;
  showLrt = false;
  maxFrtVisDist = 500;
  showStaticGeom = true;
}

BinSceneViewPlugin::~BinSceneViewPlugin()
{
  if (streamingScene)
    streamingScene->clear();
  del_it(streamingScene);
}


void BinSceneViewPlugin::registered()
{
  if (!streamingScene)
    streamingScene = new (inimem) AcesScene;
  if (streamingScene && !streamingScene->isInited())
  {
    delete streamingScene;
    streamingScene = NULL;
  }

  ::register_custom_collider(this);
  rendinst::initRIGen(/*render*/ !d3d::is_stub_driver(), 8 * 8 + 8, 8000, 0, 0, -1);
  streamingScene->skipEnviData = DAGORED2->getPluginByName("environment") != NULL;

  if (navmeshSubtypeMask == -1)
    navmeshSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("navmesh");
}


void BinSceneViewPlugin::unregistered()
{
  ::unregister_custom_collider(this);

  if (streamingScene)
    streamingScene->clear();
  del_it(streamingScene);
  rendinst::termRIGen();
}


void BinSceneViewPlugin::setVisible(bool vis)
{
  if (isVisible != vis && streamingScene && !de3_late_scene_select)
  {
    if (vis)
    {
      loadScene(streamingFolder);
    }
    else
    {
      if (streamingScene->getSsm() && streamingScene->getSsm()->isLoading())
      {
        DAEDITOR3.conNote("Waiting for streaming to shutdown...");

        // streaming only needs an approximate observer position to finish loading
        Point3 camPos(0, 0, 0);
        if (IGenViewportWnd *viewport = DAGORED2->getCurrentViewport())
        {
          TMatrix cameraTm;
          viewport->getCameraTransform(cameraTm);
          camPos = cameraTm.getcol(3);
        }

        while (streamingScene->getSsm()->isLoading())
        {
          streamingScene->update(camPos, 0);
          perform_delayed_actions();
        }

        DAEDITOR3.conNote("Done.");
      }

      streamingScene->clear();
    }
  }
  isVisible = vis;
}


void BinSceneViewPlugin::registerEditorCommands(IEditorCommandSystem &command_system)
{
  command_system.addCommand(EditorCommandIds::SHOW_PANEL, ImGuiKey_P);
}


void BinSceneViewPlugin::registerMenuAccelerators()
{
  IWndManager &wndManager = *DAGORED2->getWndManager();

  wndManager.addAccelerator(CM_SHOW_PANEL, EditorCommandIds::SHOW_PANEL);
}


void BinSceneViewPlugin::fillToolbar()
{
  IEditorCommandSystem *commandSystem = DAGORED2->queryEditorInterface<IEditorCommandSystem>();
  G_ASSERT(commandSystem);

  PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolBarId);
  G_ASSERT(toolbar);
  PropPanel::ContainerPropertyControl *tool = toolbar->getContainerById(CM_TOOL);
  G_ASSERT(tool);

  tool->clear();

  tool->createButton(PID_SET_BIN_DUMP, "Set level binary dump");
  tool->setButtonPictures(PID_SET_BIN_DUMP, "import_hm");
  tool->createCheckBox(PID_SHOW_SPLINES, "Show splines");
  tool->setButtonPictures(PID_SHOW_SPLINES, "select_spline");
  tool->setBool(PID_SHOW_SPLINES, showSplines);
  tool->createCheckBox(PID_SHOW_SPLINE_PTS, "Show spline points");
  tool->setButtonPictures(PID_SHOW_SPLINE_PTS, "select_points");
  tool->setBool(PID_SHOW_SPLINE_PTS, showSplinePoints);
  tool->createCheckBox(PID_SHOW_NAVMESH, "Show nav mesh");
  tool->setButtonPictures(PID_SHOW_NAVMESH, "navigate");
  tool->setBool(PID_SHOW_NAVMESH, showNavMesh);
  tool->createCheckBox(PID_SHOW_COLLISION, "Static Collision Preview");
  tool->setButtonPictures(PID_SHOW_COLLISION, "collision_preview");
  tool->setBool(PID_SHOW_COLLISION, showFrt);
  tool->createCheckBox(PID_SHOW_STATIC_GEOM, "Render Static Geom");
  tool->setButtonPictures(PID_SHOW_STATIC_GEOM, "render_entity_geom");
  tool->setBool(PID_SHOW_STATIC_GEOM, showStaticGeom);
  tool->createCheckBox(PID_SHOW_LANDMESH, "Render Land Geom");
  tool->setButtonPictures(PID_SHOW_LANDMESH, "asset_land");
  tool->setBool(PID_SHOW_LANDMESH, streamingScene && streamingScene->getLandscapeVis());
  tool->createSeparator(0);

  commandSystem->createToolbarToggleButton(*tool, CM_SHOW_PANEL, EditorCommandIds::SHOW_PANEL, "Show settings");
  tool->setButtonPictures(CM_SHOW_PANEL, "show_panel");
  tool->setBool(CM_SHOW_PANEL, propPanel != nullptr);
}


bool BinSceneViewPlugin::begin(int toolbar_id, unsigned menu_id)
{
  // menu
  PropPanel::IMenu *mainMenu = DAGORED2->getMainMenu();
  mainMenu->addItem(menu_id, PID_SET_BIN_DUMP, "Set level binary dump");

  // toolbar
  toolBarId = toolbar_id;
  PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolbar_id);
  G_ASSERT(toolbar);
  toolbar->setEventHandler(this);

  PropPanel::ContainerPropertyControl *tool = toolbar->createToolbarPanel(CM_TOOL);
  fillToolbar();

  IWndManager *manager = DAGORED2->getWndManager();
  manager->registerWindowHandler(this);

  if (propPanelVisible)
    showPanel();

  return true;
}


bool BinSceneViewPlugin::end()
{
  propPanelVisible = propPanel != nullptr;

  if (propPanelVisible)
    showPanel();

  IWndManager *manager = DAGORED2->getWndManager();
  manager->unregisterWindowHandler(this);

  return true;
}


void BinSceneViewPlugin::loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path)
{
  if (!streamingScene)
    return;

  isDebugVisible = local_data.getBool("isDebugVisible", true);
  showSplines = local_data.getBool("showSplines", true);
  showSplinePoints = local_data.getBool("showSplinePoints", true);
  maxPointVisDist = local_data.getReal("maxPointVisDist", 5000.0);
  showNavMesh = local_data.getBool("showNavMesh", true);
  showFrt = local_data.getBool("showFrt", false);
  showLrt = local_data.getBool("showLrt", false);
  showFrtWire = local_data.getBool("showFrtWire", false);
  maxFrtVisDist = local_data.getReal("maxFrtVisDist", 500.0);
  streamingScene->setLandscapeMirroring(local_data.getBool("useLandMirroring", true));

  recentFn.clear();
  if (const DataBlock *b = local_data.getBlockByName("recents"))
  {
    String fn;
    recentFn.reserve(b->paramCount());
    for (int i = 0, nid = b->getNameId("fn"); i < b->paramCount(); i++)
      if (b->getParamNameId(i) == nid && b->getParamType(i) == b->TYPE_STRING)
      {
        fn = ::make_full_path(DAGORED2->getGameDir(), "levels/") + b->getStr(i);
        if (dd_file_exists(fn))
          recentFn.push_back() = b->getStr(i);
        else
          DAEDITOR3.conWarning("skip missing %s (recent files)", fn);
      }

    if (recentFn.size())
      binFn = recentFn.back();
  }
  else
    binFn = local_data.getStr("bin_dump_file", "");

  streamingFolder = ::make_full_path(DAGORED2->getGameDir(), "levels/");
  if (binFn.empty())
  {
    binFn = DAGORED2->getProjectFileName();
    remove_trailing_string(binFn, ".level.blk");
    binFn += ".bin";
  }
  strmBlk.setStr("maindump", binFn);

  const DataBlock *panelStateBlk = local_data.getBlockByName("panel_state");
  if (panelStateBlk)
    mainPanelState.setFrom(panelStateBlk);

  if (IHmapDebugShadingService *debugShadingService = DAGORED2->queryEditorInterface<IHmapDebugShadingService>())
    debugShadingService->loadSettings(*local_data.getBlockByNameEx("debugShading"));

  fillPanel();
}


void BinSceneViewPlugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) {}


void BinSceneViewPlugin::autoSaveObjects(DataBlock &local_data)
{
  DataBlock &autoBlk = *local_data.addBlock("panel_state");
  autoBlk.setFrom(&mainPanelState);

  local_data.setBool("isDebugVisible", isDebugVisible);
  local_data.setBool("showSplines", showSplines);
  local_data.setBool("showSplinePoints", showSplinePoints);
  local_data.setReal("maxPointVisDist", maxPointVisDist);
  local_data.setBool("showNavMesh", showNavMesh);
  local_data.setBool("showFrt", showFrt);
  local_data.setBool("showLrt", showLrt);
  local_data.setBool("showFrtWire", showFrtWire);
  local_data.setReal("maxFrtVisDist", maxFrtVisDist);
  local_data.setBool("useLandMirroring", streamingScene ? streamingScene->getLandscapeMirroring() : true);

  local_data.removeBlock("recents");
  if (recentFn.size())
  {
    DataBlock *b = local_data.addBlock("recents");
    for (int i = 0; i < recentFn.size(); i++)
      b->addStr("fn", recentFn[i]);
    local_data.removeParam("bin_dump_file");
  }
  else
    local_data.setStr("bin_dump_file", binFn);

  if (IHmapDebugShadingService *debugShadingService = DAGORED2->queryEditorInterface<IHmapDebugShadingService>())
    debugShadingService->saveSettings(*local_data.addBlock("debugShading"));
}


void BinSceneViewPlugin::clearObjects()
{
  streamingFolder = "";
  if (streamingScene)
    streamingScene->clear();
}


void BinSceneViewPlugin::actObjects(float dt)
{
  if (isVisible && streamingScene)
  {
    IGenViewportWnd *viewport = DAGORED2->getCurrentViewport();
    if (!viewport)
      return;

    TMatrix cameraTm;
    viewport->getCameraTransform(cameraTm);
    streamingScene->update(cameraTm.getcol(3), dt);
  }
  if (streamingScene)
  {
    PropPanel::ContainerPropertyControl *tb = DAGORED2->getCustomPanel(toolBarId);
    if (tb && tb->getBool(PID_SHOW_LANDMESH) != streamingScene->getLandscapeVis())
      tb->setBool(PID_SHOW_LANDMESH, streamingScene->getLandscapeVis());
    if (propPanel && propPanel->getBool(PID_PANEL_SHOW_LANDMESH) != streamingScene->getLandscapeVis())
      propPanel->setBool(PID_PANEL_SHOW_LANDMESH, streamingScene->getLandscapeVis());
  }
}


void BinSceneViewPlugin::beforeRenderObjects(IGenViewportWnd *vp) {}


void BinSceneViewPlugin::renderObjects()
{
  if (isVisible && isDebugVisible && streamingScene && streamingScene->getSsmCtrl())
    streamingScene->getSsmCtrl()->renderDbg();
  if (isVisible && streamingScene && showLrt)
  {
    begin_draw_cached_debug_lines();
    collisionpreview::drawCollisionPreview(streamingScene->lrtCollision, TMatrix::IDENT, E3DCOLOR(128, 255, 0));
    end_draw_cached_debug_lines();
  }
}


void BinSceneViewPlugin::renderTransObjects()
{
  if (showSplines)
  {
    streamingScene->renderSplines();
    if (showSplinePoints)
      streamingScene->renderSplinePoints(maxPointVisDist);
  }
  if (showNavMesh && (DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER) & navmeshSubtypeMask))
    pathfinder::renderDebug();
}


void *BinSceneViewPlugin::queryInterfacePtr(unsigned huid)
{
  RETURN_INTERFACE(huid, IRenderingService);
  RETURN_INTERFACE(huid, IPluginAutoSave);
  RETURN_INTERFACE(huid, ILevelBinLoader);
  RETURN_INTERFACE(huid, IEnvironmentSettings);
  return NULL;
}

void BinSceneViewPlugin::onBeforeReset3dDevice()
{
  if (streamingScene)
    streamingScene->beforeD3DReset();
}

bool BinSceneViewPlugin::catchEvent(unsigned ev_huid, void *userData)
{
  if (ev_huid == HUID_InvalidateClipmap || ev_huid == HUID_AfterD3DReset)
  {
    if (streamingScene)
    {
      if (streamingScene->lmeshMgr && streamingScene->lmeshMgr->getHmapHandler())
        streamingScene->lmeshMgr->getHmapHandler()->fillHmapTextures();

      if (ev_huid == HUID_AfterD3DReset)
        streamingScene->afterD3DReset((bool)(uintptr_t)userData);
      else
      {
        auto flags = (uintptr_t)userData;
        streamingScene->invalidateClipmap(flags & INVALIDATE_CLIPMAP_FORCE_REDRAW, !(flags & INVALIDATE_CLIPMAP_SKIP_LAST_CLIP));
      }
    }
  }
  else if (ev_huid == HUID_PostRenderObjects && isVisible && showFrt && PhysMat::physMatCount() && streamingScene &&
           streamingScene->frtDump.isDataValid())
  {
    float prev_rad = get_vcm_rad();
    bool prev_vis = is_vcm_visible();
    int prev_type = set_vcm_draw_type(showFrtWire);
    set_vcm_rad(maxFrtVisDist);
    set_vcm_visible(true);
    if (IGenViewportWnd *vp = DAGORED2->getRenderViewport())
    {
      TMatrix cameraTm;
      vp->getCameraTransform(cameraTm);
      ::render_visclipmesh(streamingScene->frtDump, cameraTm.getcol(3));
    }
    set_vcm_rad(prev_rad);
    set_vcm_visible(prev_vis);
    set_vcm_draw_type(prev_type);
  }
  else if (ev_huid == HUID_AfterProjectLoad && isVisible)
  {
    if (streamingFolder.length() && !de3_late_scene_select)
      loadScene(streamingFolder);
    else if (de3_late_scene_select)
      onPluginMenuClick(PID_SET_BIN_DUMP);
  }
  return false;
}

void BinSceneViewPlugin::renderGeometry(Stage stage)
{
  if (!isVisible || !streamingScene)
    return;

  switch (stage)
  {
    case STG_BEFORE_RENDER: streamingScene->beforeRender(); break;

    case STG_RENDER_STATIC_OPAQUE: streamingScene->render(showStaticGeom); break;

    case STG_RENDER_STATIC_TRANS:
      streamingScene->renderTrans(showStaticGeom);
      streamingScene->renderWater(stage);
      break;

    case STG_RENDER_SHADOWS: streamingScene->renderShadows(); break;

    case STG_RENDER_SHADOWS_VSM:
      if (streamingScene->getLandscapeVis())
        streamingScene->renderShadowsVsm();
      break;

    case STG_RENDER_HEIGHT_FIELD:
      if (streamingScene->getLandscapeVis())
        streamingScene->renderHeight();
      break;

    default: break;
  }
}


bool BinSceneViewPlugin::loadScene(const char *streaming_folder)
{
  if (!streamingScene || !streaming_folder || !streaming_folder[0])
    return false;

  static const int MAX_RECENTS = 16;
  int idx = find_value_idx(recentFn, SimpleString(binFn));
  if (idx < 0)
    recentFn.push_back() = binFn;
  else if (idx != recentFn.size() - 1)
  {
    erase_items(recentFn, idx, 1);
    recentFn.push_back() = binFn;
  }
  if (recentFn.size() > MAX_RECENTS)
    erase_items(recentFn, 0, recentFn.size() - MAX_RECENTS);

  DAEDITOR3.setFatalHandler(true);
  streamingScene->loadLevel(::make_full_path(streaming_folder, binFn));
  DAEDITOR3.popFatalHandler();
  return true;
}


void BinSceneViewPlugin::changeLevelBinary(const char *bin_fn)
{
  if (bin_fn && strlen(bin_fn) && (binFn != bin_fn || de3_late_scene_select))
  {
    de3_late_scene_select = false;
    binFn = bin_fn;
    if (isVisible)
    {
      setVisible(false);
      setVisible(true);
    }
  }
}

void BinSceneViewPlugin::setEnvironmentSettings(DataBlock &blk) { streamingScene->setEnvironmentSettings(blk); }


void *BinSceneViewPlugin::onWmCreateWindow(int type)
{
  switch (type)
  {
    case PROPBAR_SCENE_VIEW_WTYPE:
    {
      if (propPanel)
        return nullptr;

      propPanel = IEditorCoreEngine::get()->createPropPanel(this, "Properties");
      fillPanel();

      if (PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolBarId))
        toolbar->setBool(CM_SHOW_PANEL, true);

      return propPanel;
    }
    break;
  }

  return nullptr;
}


bool BinSceneViewPlugin::onWmDestroyWindow(void *window)
{
  if (window == propPanel)
  {
    mainPanelState.reset();
    propPanel->saveState(mainPanelState);
    del_it(propPanel);

    if (PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolBarId))
      toolbar->setBool(CM_SHOW_PANEL, false);

    return true;
  }

  return false;
}


void BinSceneViewPlugin::showPanel()
{
  if (propPanel)
    EDITORCORE->removePropPanel(propPanel);
  else
    EDITORCORE->addPropPanel(PROPBAR_SCENE_VIEW_WTYPE, hdpi::_pxScaled(PROPBAR_WIDTH));
}


void BinSceneViewPlugin::fillPanel()
{
  if (!propPanel)
    return;

  if (propPanel->getChildCount() > 0)
  {
    mainPanelState.reset();
    propPanel->saveState(mainPanelState);
  }

  propPanel->clear();
  propPanel->disableFillAutoResize();

  {
    PropPanel::ContainerPropertyControl *group = propPanel->createGroup(PID_PANEL_VISIBILITY_SETTINGS_GROUP, "Visibility settings");

    group->createCheckBox(PID_PANEL_SHOW_STATIC_GEOM, "Render static geom", showStaticGeom);
    if (streamingScene)
    {
      group->createCheckBox(PID_PANEL_SHOW_LANDMESH, "Render land geom", streamingScene->getLandscapeVis());
      group->createCheckBox(PID_PANEL_USE_LAND_MIRRORING, "Use Land mirroring", streamingScene->getLandscapeMirroring());
    }
    group->createCheckBox(PID_PANEL_SHOW_NAVMESH, "Show navigation mesh", showNavMesh);
    group->createSeparator(0);
    group->createCheckBox(PID_PANEL_SHOW_SPLINES, "Show splines", showSplines);
    group->createCheckBox(PID_PANEL_SHOW_SPLINE_PTS, "Show spline points", showSplinePoints);
    group->createEditFloat(PID_PANEL_SPLINE_POINT_VISIBILITY_RANGE, "Spline points vis. range", maxPointVisDist);
    group->createSeparator(0);
    group->createCheckBox(PID_PANEL_SHOW_STATIC_COLLISION, "Show static collision", showFrt);
    group->createCheckBox(PID_PANEL_SHOW_LAND_COLLISION, "Show land collision", showLrt);
    group->createEditFloat(PID_PANEL_COLLISION_VISIBILITY_RANGE, "Collision  vis. range", maxFrtVisDist);
    group->createCheckBox(PID_PANEL_SHOW_WIREFRAME_COLLISION, "Wireframe collision", showFrtWire);
  }

  if (IHmapDebugShadingService *debugShadingService = DAGORED2->queryEditorInterface<IHmapDebugShadingService>())
  {
    PropPanel::ContainerPropertyControl *group =
      propPanel->createGroup(PID_PANEL_LANDSCAPE_DEBUG_SHADING_GROUP, "Landscape debug shading");
    debugShadingService->fillPropertyPanel(*group, PID_PANEL_LANDSCAPE_DEBUG_SHADING_START);
  }

  propPanel->restoreFillAutoResize();
  propPanel->loadState(mainPanelState);
}


void BinSceneViewPlugin::updateImgui()
{
  if (DAGORED2->curPlugin() == this)
  {
    if (propPanel)
    {
      bool open = true;
      DAEDITOR3.imguiBegin(*propPanel, &open);
      propPanel->updateImgui();
      DAEDITOR3.imguiEnd();

      if (!open && propPanel)
      {
        showPanel();
        EDITORCORE->managePropPanels();
      }
    }
  }
}


void BinSceneViewPlugin::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case PID_PANEL_SHOW_STATIC_GEOM:
      showStaticGeom = panel->getBool(pcb_id);
      fillToolbar();
      break;

    case PID_PANEL_SHOW_LANDMESH:
      if (streamingScene)
      {
        streamingScene->setLandscapeVis(panel->getBool(PID_PANEL_SHOW_LANDMESH));
        fillToolbar();
      }
      break;

    case PID_PANEL_USE_LAND_MIRRORING:
      if (streamingScene)
      {
        const bool useLandMirroring = panel->getBool(PID_PANEL_USE_LAND_MIRRORING);
        if (useLandMirroring != streamingScene->getLandscapeMirroring())
        {
          streamingScene->setLandscapeMirroring(useLandMirroring);
          if (isVisible)
          {
            setVisible(false);
            setVisible(true);
          }
        }
      }
      break;

    case PID_PANEL_SHOW_NAVMESH:
      showNavMesh = panel->getBool(pcb_id);
      if (IGenEditorPlugin *p = DAGORED2->getPluginByName("_navmesh"))
        p->setVisible(showNavMesh);
      fillToolbar();
      break;

    case PID_PANEL_SHOW_SPLINES:
      showSplines = panel->getBool(pcb_id);
      fillToolbar();
      break;

    case PID_PANEL_SHOW_SPLINE_PTS:
      showSplinePoints = panel->getBool(PID_PANEL_SHOW_SPLINE_PTS);
      fillToolbar();
      break;

    case PID_PANEL_SPLINE_POINT_VISIBILITY_RANGE: maxPointVisDist = panel->getFloat(pcb_id); break;

    case PID_PANEL_SHOW_STATIC_COLLISION:
      showFrt = panel->getBool(pcb_id);
      fillToolbar();
      break;

    case PID_PANEL_SHOW_LAND_COLLISION: showLrt = panel->getBool(pcb_id); break;

    case PID_PANEL_COLLISION_VISIBILITY_RANGE: maxFrtVisDist = panel->getFloat(pcb_id); break;

    case PID_PANEL_SHOW_WIREFRAME_COLLISION: showFrtWire = panel->getBool(pcb_id); break;
  }

  if (IHmapDebugShadingService *debugShadingService = DAGORED2->queryEditorInterface<IHmapDebugShadingService>())
    debugShadingService->onChange(pcb_id, *panel, PID_PANEL_LANDSCAPE_DEBUG_SHADING_START);
}


void BinSceneViewPlugin::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  onPluginMenuClick(pcb_id);
  switch (pcb_id)
  {
    case PID_SHOW_SPLINES:
      panel->setBool(pcb_id, showSplines = !showSplines);
      fillPanel();
      DAGORED2->repaint();
      break;
    case PID_SHOW_SPLINE_PTS:
      panel->setBool(pcb_id, showSplinePoints = !showSplinePoints);
      fillPanel();
      DAGORED2->repaint();
      break;
    case PID_SHOW_NAVMESH:
      panel->setBool(pcb_id, showNavMesh = !showNavMesh);
      if (IGenEditorPlugin *p = DAGORED2->getPluginByName("_navmesh"))
        p->setVisible(showNavMesh);
      fillPanel();
      DAGORED2->repaint();
      break;
    case PID_SHOW_COLLISION:
      panel->setBool(pcb_id, showFrt = !showFrt);
      fillPanel();
      DAGORED2->repaint();
      break;
    case PID_SHOW_STATIC_GEOM:
      panel->setBool(pcb_id, showStaticGeom = !showStaticGeom);
      fillPanel();
      DAGORED2->repaint();
      break;
    case PID_SHOW_LANDMESH:
      if (streamingScene)
        streamingScene->setLandscapeVis(panel->getBool(pcb_id));
      else
        panel->setBool(pcb_id, false);
      fillPanel();
      break;
  }

  if (IHmapDebugShadingService *debugShadingService = DAGORED2->queryEditorInterface<IHmapDebugShadingService>())
    debugShadingService->onClick(pcb_id, *panel, PID_PANEL_LANDSCAPE_DEBUG_SHADING_START);
}


bool BinSceneViewPlugin::onPluginMenuClick(unsigned id)
{
  enum
  {
    PID_RECENT_RG = 1,
    PID_BIN_FILE = 100
  };

  switch (id)
  {
    case CM_SHOW_PANEL:
      showPanel();
      EDITORCORE->managePropPanels();
      return true;

    case PID_SET_BIN_DUMP:
    {
      struct SelectBinDumpDlg : public PropPanel::DialogWindow
      {
        SelectBinDumpDlg(dag::ConstSpan<SimpleString> recentFn, const char *strm_folder) :
          DialogWindow(0, _pxScaled(450), _pxScaled(160), "Select binary dump file")
        {
          Tab<String> filter(midmem);
          filter.push_back() = "Binary dump (*.bin)|*.bin";

          PropPanel::ContainerPropertyControl *recentsRG = getPanel()->createRadioGroup(PID_RECENT_RG, "Recently used files");

          for (int i = recentFn.size() - 1; i >= 0; i--)
          {
            String fn(recentFn[i]);
            simplify_fname(fn);
            recentsRG->createRadio(i, fn);
          }
          if (recentFn.size())
          {
            recentsRG->createSeparator(-1);
            recentsRG->createSeparator(-1);
            recentsRG->createSeparator(-1);
            recentsRG->createSeparator(-1);
          }

          recentsRG->createRadio(-1, "Open new file");
          getPanel()->createFileEditBox(PID_BIN_FILE, "", strm_folder);
          getPanel()->setStrings(PID_BIN_FILE, filter);
          getPanel()->setInt(PID_BIN_FILE, PropPanel::FS_DIALOG_OPEN_FILE);

          getPanel()->setInt(PID_RECENT_RG, recentFn.size() - 1);

          // ImGui::GetMainViewport()->GetCenter() is still zero at this point, let ImGui auto center it for us.
          autoSize(/*auto_center = */ false);
        }
        void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
        {
          if (pcb_id == PID_BIN_FILE)
            panel->setInt(PID_RECENT_RG, -1);
        }
        void onDoubleClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
        {
          if (pcb_id == PID_RECENT_RG && panel->getInt(PID_RECENT_RG) != -1)
            clickDialogButton(PropPanel::DIALOG_ID_OK);
        }
      };

      SelectBinDumpDlg *dialog = new SelectBinDumpDlg(recentFn, streamingFolder);

      if (dialog->showDialog() == PropPanel::DIALOG_ID_OK)
      {
        int sel_idx = dialog->getPanel()->getInt(PID_RECENT_RG);
        if (sel_idx == -1)
        {
          String fn(make_path_relative(dialog->getPanel()->getText(PID_BIN_FILE), streamingFolder));
          while (fn.empty() || strcmp(fn, "./.") == 0 || strcmp(fn, ".") == 0)
          {
            fn = wingw::file_open_dlg(NULL, "Select scene binary to load", "Binary dump (*.bin)|*.bin", "bin",
              dialog->getPanel()->getText(PID_BIN_FILE));
            if (fn.empty())
            {
              del_it(dialog);
              return true;
            }
            fn = make_path_relative(fn, streamingFolder);
          }
          changeLevelBinary(fn);
        }
        else if (sel_idx >= 0 && sel_idx < recentFn.size())
          changeLevelBinary(recentFn[sel_idx]);
      }
      del_it(dialog);
    }
      return true;
  }

  return false;
}


bool BinSceneViewPlugin::traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm)
{
  if (!streamingScene)
    return false;
  bool _result = false;
  if (streamingScene->lmeshMgr)
    _result = streamingScene->lmeshMgr->traceray(p, dir, maxt, norm);
  int pmid;
  if (streamingScene->frtDump.isDataValid() && streamingScene->frtDump.traceray(p, dir, maxt, pmid) >= 0)
    _result = true;
  return _result;
}


bool BinSceneViewPlugin::shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt)
{
  if (!streamingScene)
    return false;

  Point3 tmp_dir = dir;
  tmp_dir.normalize();

  if (streamingScene->lmeshMgr && streamingScene->lmeshMgr->rayhitNormalized(p, tmp_dir, maxt))
    return true;
  if (streamingScene->frtDump.isDataValid() && streamingScene->frtDump.rayhitNormalized(p, tmp_dir, maxt))
    return true;
  return false;
}


//===============================================================================

void init_plugin_bin_scn_view()
{
  if (!DAGORED2->checkVersion())
  {
    DEBUG_CTX("Incorrect version!");
    return;
  }
  if (DAGORED2->getPluginByName("heightmapLand"))
  {
    DAEDITOR3.conError("\"%s\" is not compatible with already inited \"%s\", skipped", "Scene view", "Landscape");
    return;
  }

  plugin = new (inimem) BinSceneViewPlugin;

  if (!DAGORED2->registerPlugin(::plugin))
    del_it(plugin);
}
