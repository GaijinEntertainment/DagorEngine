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

#include <libTools/util/strUtil.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/menu.h>
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
  PID_SET_BIN_DUMP,
  PID_SHOW_SPLINES,
  PID_SHOW_SPLINE_PTS,
  PID_SET_VIS_SETTS,
  PID_SHOW_NAVMESH,
  PID_SHOW_COLLISION,
  PID_SHOW_STATIC_GEOM,
  PID_SHOW_LANDMESH,
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

        while (streamingScene->getSsm()->isLoading())
        {
          streamingScene->update(grs_cur_view.tm.getcol(3), 0);
          perform_delayed_actions();
        }

        DAEDITOR3.conNote("Done.");
      }

      streamingScene->clear();
    }
  }
  isVisible = vis;
}


bool BinSceneViewPlugin::begin(int toolbar_id, unsigned menu_id)
{
  // menu
  PropPanel::IMenu *mainMenu = DAGORED2->getMainMenu();
  mainMenu->addItem(menu_id, PID_SET_BIN_DUMP, "Set level binary dump");
  mainMenu->addSeparator(menu_id);
  mainMenu->addItem(menu_id, PID_SET_VIS_SETTS, "Edit visibility settings...");

  // toolbar
  toolBarId = toolbar_id;
  PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolbar_id);
  G_ASSERT(toolbar);
  toolbar->setEventHandler(this);

  PropPanel::ContainerPropertyControl *tool = toolbar->createToolbarPanel(CM_TOOL, "");

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
  tool->setBool(PID_SHOW_LANDMESH, true);
  tool->createSeparator(0);
  tool->createButton(PID_SET_VIS_SETTS, "Edit visibility settings...");
  tool->setButtonPictures(PID_SET_VIS_SETTS, "show_panel");

  return true;
}


bool BinSceneViewPlugin::end() { return true; }


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
}


void BinSceneViewPlugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) {}


void BinSceneViewPlugin::autoSaveObjects(DataBlock &local_data)
{
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

    viewport->getCameraTransform(grs_cur_view.tm);
    streamingScene->update(grs_cur_view.tm.getcol(3), dt);
  }
  if (streamingScene)
  {
    PropPanel::ContainerPropertyControl *tb = DAGORED2->getCustomPanel(toolBarId);
    if (tb && tb->getBool(PID_SHOW_LANDMESH) != streamingScene->getLandscapeVis())
      tb->setBool(PID_SHOW_LANDMESH, streamingScene->getLandscapeVis());
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

bool BinSceneViewPlugin::catchEvent(unsigned ev_huid, void *userData)
{
  if (ev_huid == HUID_InvalidateClipmap || ev_huid == HUID_AfterD3DReset)
  {
    if (streamingScene)
    {
      if (streamingScene->lmeshMgr && streamingScene->lmeshMgr->getHmapHandler())
        streamingScene->lmeshMgr->getHmapHandler()->fillHmapTextures();
      streamingScene->invalidateClipmap((bool)userData);
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
    ::render_visclipmesh(streamingScene->frtDump, ::grs_cur_view.pos);
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

void BinSceneViewPlugin::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  onPluginMenuClick(pcb_id);
  switch (pcb_id)
  {
    case PID_SHOW_SPLINES:
      panel->setBool(pcb_id, showSplines = !showSplines);
      DAGORED2->repaint();
      break;
    case PID_SHOW_SPLINE_PTS:
      panel->setBool(pcb_id, showSplinePoints = !showSplinePoints);
      DAGORED2->repaint();
      break;
    case PID_SHOW_NAVMESH:
      panel->setBool(pcb_id, showNavMesh = !showNavMesh);
      if (IGenEditorPlugin *p = DAGORED2->getPluginByName("_navmesh"))
        p->setVisible(showNavMesh);
      DAGORED2->repaint();
      break;
    case PID_SHOW_COLLISION:
      panel->setBool(pcb_id, showFrt = !showFrt);
      DAGORED2->repaint();
      break;
    case PID_SHOW_STATIC_GEOM:
      panel->setBool(pcb_id, showStaticGeom = !showStaticGeom);
      DAGORED2->repaint();
      break;
    case PID_SHOW_LANDMESH:
      if (streamingScene)
        streamingScene->setLandscapeVis(panel->getBool(pcb_id));
      else
        panel->setBool(pcb_id, false);
      break;
  }
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

    case PID_SET_VIS_SETTS:
    {
      PropPanel::DialogWindow *dlg = DAGORED2->createDialog(_pxScaled(250), _pxScaled(380), "Set visibility settings");
      PropPanel::ContainerPropertyControl &panel = *dlg->getPanel();
      panel.createCheckBox(101, "Render static geom", showStaticGeom);
      if (streamingScene)
      {
        panel.createCheckBox(102, "Render land geom", streamingScene->getLandscapeVis());
        panel.createCheckBox(1021, "Use Land mirroring", streamingScene->getLandscapeMirroring());
      }
      panel.createCheckBox(103, "Show navigation mesh", showNavMesh);
      panel.createSeparator(0);
      panel.createCheckBox(201, "Show splines", showSplines);
      panel.createCheckBox(202, "Show spline points", showSplinePoints);
      panel.createEditFloat(203, "Spline points vis. range", maxPointVisDist);
      panel.createSeparator(0);
      panel.createCheckBox(301, "Show static collision", showFrt);
      panel.createCheckBox(302, "Show land collision", showLrt);
      panel.createEditFloat(303, "Collision  vis. range", maxFrtVisDist);
      panel.createCheckBox(304, "Wireframe collision", showFrtWire);
      int ret = dlg->showDialog();

      if (ret == PropPanel::DIALOG_ID_OK)
      {
        showStaticGeom = panel.getBool(101);
        showNavMesh = panel.getBool(103);

        showSplines = panel.getBool(201);
        showSplinePoints = panel.getBool(202);
        maxPointVisDist = panel.getFloat(203);

        showFrt = panel.getBool(301);
        showLrt = panel.getBool(302);
        maxFrtVisDist = panel.getFloat(303);
        showFrtWire = panel.getBool(304);

        PropPanel::ContainerPropertyControl *tb = DAGORED2->getCustomPanel(toolBarId);
        tb->setBool(PID_SHOW_SPLINES, showSplines);
        tb->setBool(PID_SHOW_SPLINE_PTS, showSplinePoints);
        tb->setBool(PID_SHOW_NAVMESH, showNavMesh);
        if (IGenEditorPlugin *p = DAGORED2->getPluginByName("_navmesh"))
          p->setVisible(showNavMesh);
        tb->setBool(PID_SHOW_COLLISION, showFrt);
        tb->setBool(PID_SHOW_STATIC_GEOM, showStaticGeom);
        if (streamingScene)
        {
          tb->setBool(PID_SHOW_LANDMESH, panel.getBool(102));
          streamingScene->setLandscapeVis(panel.getBool(102));
          if (panel.getBool(1021) != streamingScene->getLandscapeMirroring())
          {
            streamingScene->setLandscapeMirroring(panel.getBool(1021));
            if (isVisible)
            {
              setVisible(false);
              setVisible(true);
            }
          }
        }

        DAGORED2->invalidateViewportCache();
      }
      del_it(dlg);
      break;
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
  if (streamingScene->frtDump.isDataValid() && streamingScene->frtDump.traceray(p, dir, maxt, pmid))
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
