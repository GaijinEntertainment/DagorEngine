// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_appwnd.h"
#include "av_mainAssetSelector.h"
#include "av_plugin.h"
#include "assetBuildCache.h"

#include <de3_lightService.h>
#include <de3_lightProps.h>
#include <de3_skiesService.h>
#include <de3_dynRenderService.h>
#include <de3_interface.h>
#include <de3_rendInstGen.h>
#include <render/debugTexOverlay.h>

#include <de3_huid.h>

#include <3d/dag_render.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_texPackMgr2.h>
#include <3d/dag_stereoIndex.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderBlock.h>
#include <render/dag_cur_view.h>

#include <EditorCore/ec_gizmofilter.h>

#include <debug/dag_debug.h>
#include <debug/dag_textMarks.h>
#include <perfMon/dag_perfMonStat.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <assets/asset.h>
#include <assets/assetHlp.h>
#include <assets/assetRefs.h>
#include <assets/daBuildInterface.h>
#include <gameRes/dag_gameResSystem.h>

#include <propPanel/control/panelWindow.h>

#include <EASTL/vector_set.h>

#include "assetUserFlags.h"

extern void *get_generic_render_helper_service();
extern void *get_generic_dyn_render_service();
extern void *get_dng_based_render_service();
extern void *get_dng_based_skies_service();
extern void *get_tiff_bit_mask_image_mgr();
extern void *get_generic_asset_service();
extern void *get_gen_light_service();
extern void *get_generic_skies_service();
extern void *get_generic_color_range_service();
extern void *get_generic_rendinstgen_service();
extern void *get_generic_spline_gen_service();
extern void *get_generic_wind_service();
extern void *get_pixel_perfect_selection_service();
extern void *get_visibility_finder_service();

extern void terminate_interface_de3();
extern InitOnDemand<DebugTexOverlay> av_show_tex_helper;


static int worldViewPosVarId = -2;

static void getAssetReferencesRecursively(DagorAsset &asset, eastl::vector_set<DagorAsset *> &references,
  Tab<IDagorAssetRefProvider::Ref> &tmp_refs)
{
  IDagorAssetRefProvider *refProvider = asset.getMgr().getAssetRefProvider(asset.getType());
  if (!refProvider)
    return;

  refProvider->getAssetRefs(asset, tmp_refs);
  Tab<DagorAsset *> new_assets;
  for (const IDagorAssetRefProvider::Ref &ref : tmp_refs)
  {
    if (DagorAsset *refAsset = ref.getAsset())
    {
      if (references.insert(refAsset).second) // If it was inserted.
        new_assets.push_back(refAsset);
    }
    else if (ref.flags & IDagorAssetRefProvider::RFLG_BROKEN)
    {
      DAEDITOR3.conWarning("Asset \"%s\" has broken reference to \"%s\".", asset.getNameTypified().c_str(), ref.getBrokenRef());
    }
  }
  for (DagorAsset *refAsset : new_assets)
    getAssetReferencesRecursively(*refAsset, references, tmp_refs);
}

static bool hasAssetBeenLoadedPreviouslyInThisSession(DagorAsset &asset)
{
  const int resId = dabuildcache::get_asset_res_id(asset);
  const int classId = get_game_res_class_id(resId);
  return classId != 0 && is_game_resource_loaded(GAMERES_HANDLE_FROM_STRING(asset.getName()), classId);
}

static void logAssetBuildWarningsForASingleAsset(DagorAsset &asset)
{
  IDaBuildInterface *dabuild = dabuildcache::get_dabuild();
  if (!dabuild)
    return;

  IDagorAssetExporter *exporter = asset.getMgr().getAssetExporter(asset.getType());
  if (!exporter)
    return;

  dag::Vector<IDaBuildInterface::BuildWarning> warnings;
  bool cacheUpToDate;
  if (!dabuild->getBuiltResWarnings(asset, *exporter, assetlocalprops::makePath("cache"), warnings, cacheUpToDate) || !cacheUpToDate)
    return;

  for (const IDaBuildInterface::BuildWarning &warning : warnings)
    DAEDITOR3.getCon().addMessage(warning.messageType, "%s", warning.message.c_str());
}

static void logAssetBuildWarnings(DagorAsset &asset, bool show_once_per_session)
{
  // If we only need to show the build warnings once per session and the asset has been loaded previously then its
  // references has been loaded too.
  if (show_once_per_session && hasAssetBeenLoadedPreviouslyInThisSession(asset))
    return;

  eastl::vector_set<DagorAsset *> assetReferences;
  Tab<IDagorAssetRefProvider::Ref> tmp_refs;
  getAssetReferencesRecursively(asset, assetReferences, tmp_refs);

  // Log the warnings from the dependencies first.
  for (DagorAsset *assetReference : assetReferences)
    if (assetReference != &asset && (!show_once_per_session || !hasAssetBeenLoadedPreviouslyInThisSession(*assetReference)))
      logAssetBuildWarningsForASingleAsset(*assetReference);

  // Log the warnings from the main asset last.
  logAssetBuildWarningsForASingleAsset(asset);
}

//=============================================================================
void AssetViewerApp::terminateInterface()
{
  av_show_tex_helper.demandDestroy();
  environment::clear();

  for (int i = 0; i < plugin.size(); ++i)
  {
    plugin[i]->unregistered();
    del_it(plugin[i]);
  }

  terminate_interface_de3();
  IEditorCoreEngine::set(NULL);
}


//=============================================================================
void *AssetViewerApp::queryEditorInterfacePtr(unsigned huid)
{
  if (huid == HUID_IBitMaskImageMgr)
    return ::get_tiff_bit_mask_image_mgr();

  if (huid == HUID_IAssetService)
    return ::get_generic_asset_service();

  if (huid == HUID_ISceneLightService)
    return get_gen_light_service();

  if (huid == HUID_ISkiesService)
    return useDngBasedSceneRender ? get_dng_based_skies_service() : get_generic_skies_service();

  if (huid == HUID_IColorRangeService)
    return get_generic_color_range_service();

  if (huid == HUID_IRendInstGenService)
    return get_generic_rendinstgen_service();

  if (huid == HUID_IDynRenderService)
    return useDngBasedSceneRender ? get_dng_based_render_service() : get_generic_dyn_render_service();

  if (huid == HUID_IRenderHelperService)
    return get_generic_render_helper_service();

  if (huid == HUID_ISplineGenService)
    return get_generic_spline_gen_service();

  if (huid == HUID_IWindService)
    return !useDngBasedSceneRender ? get_generic_wind_service() : nullptr;

  if (huid == HUID_IPixelPerfectSelectionService)
    return get_pixel_perfect_selection_service();

  if (huid == HUID_IVisibilityFinderProvider)
    return get_visibility_finder_service();

  RETURN_INTERFACE(huid, IMainWindowImguiRenderingService);

  return NULL;
}

void AssetViewerApp::screenshotRender(bool skip_debug_objects)
{
  const bool lastSkipDebugObjects = skipRenderObjects;

  skipRenderObjects = skip_debug_objects;
  skipSetViewProj = true;

  queryEditorInterface<IDynRenderService>()->renderScreenshot();

  skipSetViewProj = false;
  skipRenderObjects = lastSkipDebugObjects;
}

//=============================================================================
bool AssetViewerApp::registerPlugin(IGenEditorPlugin *p)
{
  for (int i = plugin.size() - 1; i >= 0; --i)
  {
    if (plugin[i] == p)
    {
      debug("multiple register %p/%s!", p, p->getInternalName());
      return true;
    }
    else if (!stricmp(plugin[i]->getInternalName(), p->getInternalName()))
    {
      debug("another plugin with the same internal name is already "
            "registered: %p/%s!",
        p, p->getInternalName());
      return false;
    }
  }

  debug("== register plugin %p/%s", p, p->getInternalName());

  plugin.push_back(p);
  sortPlugins();
  p->registered();

  return true;
}


//=============================================================================
bool AssetViewerApp::unregisterPlugin(IGenEditorPlugin *p)
{
  for (int i = plugin.size() - 1; i >= 0; --i)
    if (plugin[i] == p)
    {
      debug("== unregister plugin %p/%s", p, p->getInternalName());
      plugin[i]->unregistered();
      erase_items(plugin, i, 1);
      return true;
    }

  return false;
}


//=============================================================================
void AssetViewerApp::switchToPlugin(int id)
{
  class InfoDrawStub : public IGenEventHandler
  {
    bool handleMouseMove(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
    bool handleMouseLBPress(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
    bool handleMouseLBRelease(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
    bool handleMouseRBPress(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
    bool handleMouseRBRelease(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
    bool handleMouseCBPress(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
    bool handleMouseCBRelease(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
    bool handleMouseWheel(IGenViewportWnd *, int, int, int, int) override { return false; }
    bool handleMouseDoubleClick(IGenViewportWnd *, int, int, int) override { return false; }
    void handleViewChange(IGenViewportWnd *) override {}

    void handleViewportPaint(IGenViewportWnd *wnd) override { ::get_app().drawAssetInformation(wnd); }
  };
  static InfoDrawStub infoDrawStub;

  if (curAsset && curAsset->testUserFlags(ASSET_USER_FLG_NEEDS_RELOAD))
  {
    curAsset->clrUserFlags(ASSET_USER_FLG_NEEDS_RELOAD);
    switchToPlugin(id);
    reloadAsset(*curAsset, curAsset->getNameId(), curAsset->getType());
    return;
  }

  IGenEditorPlugin *cur = curPlugin();

  const int oldCur = curPluginId;
  curPluginId = id;

  IGenEditorPlugin *next = (curPluginId == -1) ? NULL : plugin[curPluginId];

  if (cur && !cur->end())
  {
    curPluginId = oldCur;
    return;
  }

  if (curAsset && assetBuildWarningDisplay != AssetBuildWarningDisplay::ShowWhenBuilding)
    logAssetBuildWarnings(*curAsset, assetBuildWarningDisplay == AssetBuildWarningDisplay::ShowOncePerSession);

  showAdditinalToolWindow(next ? next->haveToolPanel() : false);

  if (next && !next->begin(curAsset))
  {
    curPluginId = oldCur;
    showAdditinalToolWindow(cur->haveToolPanel());
    cur->begin(curAsset);
    return;
  }

  ged.curEH = next ? next->getEventHandler() : (curAssetPackName.empty() ? NULL : &infoDrawStub);
  ged.setEH(appEH);

  addAccelerators();

  if (autoZoomAndCenter)
    zoomAndCenter();
}


IGenEditorPluginBase *AssetViewerApp::getPluginBase(int idx) { return getPlugin(idx); }
IGenEditorPluginBase *AssetViewerApp::curPluginBase() { return curPlugin(); }
bool IGenEditorPlugin::getVisible() const { return this == get_app().curPlugin(); }

//=============================================================================
int AssetViewerApp::getPluginCount() { return plugin.size(); }


//=============================================================================
IGenEditorPlugin *AssetViewerApp::getPlugin(int idx) { return (idx >= 0 && idx < plugin.size()) ? plugin[idx] : NULL; }


//=============================================================================
int AssetViewerApp::getPluginIdx(IGenEditorPlugin *plug) const
{
  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i] == plug)
      return i;

  return -1;
}


//=============================================================================
void *AssetViewerApp::getInterface(int interface_uid)
{
  for (int i = 0; i < plugin.size(); ++i)
  {
    void *iface = plugin[i]->queryInterfacePtr(interface_uid);
    if (iface)
      return iface;
  }

  return NULL;
}


//=============================================================================
void AssetViewerApp::getInterfaces(int interface_uid, Tab<void *> &interfaces)
{
  for (int i = 0; i < plugin.size(); ++i)
  {
    void *iface = plugin[i]->queryInterfacePtr(interface_uid);
    if (iface)
      interfaces.push_back(iface);
  }
}


//==================================================================================================
IWndManager *AssetViewerApp::getWndManager() const
{
  G_ASSERT(mManager && "AssetViewerApp::getWndManager(): window manager == NULL!");
  return mManager;
}


//==================================================================================================
PropPanel::ContainerPropertyControl *AssetViewerApp::getCustomPanel(int id) const
{
  switch (id)
  {
    case GUI_PLUGIN_TOOLBAR_ID: return mPluginTool;
  }

  return NULL;
}


//==================================================================================================
void AssetViewerApp::addPropPanel(int type, hdpi::Px) { mManager->setWindowType(nullptr, type); }


void AssetViewerApp::removePropPanel(void *hwnd) { mManager->removeWindow(hwnd); }


PropPanel::PanelWindowPropertyControl *AssetViewerApp::createPropPanel(ControlEventHandler *eh, const char *caption)
{
  return new PropPanel::PanelWindowPropertyControl(0, eh, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0), caption);
}


PropPanel::IMenu *AssetViewerApp::getMainMenu() { return GenericEditorAppWindow::getMainMenu(); }


PropPanel::DialogWindow *AssetViewerApp::createDialog(hdpi::Px w, hdpi::Px h, const char *title)
{
  return new PropPanel::DialogWindow(0, w, h, title);
}


void AssetViewerApp::deleteDialog(PropPanel::DialogWindow *dlg) { delete dlg; }


//==============================================================================
void AssetViewerApp::updateViewports() { shouldUpdateViewports = true; }


//==============================================================================
void AssetViewerApp::setViewportCacheMode(ViewportCacheMode mode) { ged.setViewportCacheMode(mode); }


//==============================================================================
void AssetViewerApp::invalidateViewportCache() { ged.invalidateCache(); }


//==============================================================================
int AssetViewerApp::getViewportCount() { return ged.getViewportCount(); }


//==============================================================================
IGenViewportWnd *AssetViewerApp::getViewport(int n) { return ged.getViewport(n); }


//==============================================================================
IGenViewportWnd *AssetViewerApp::getRenderViewport() { return ged.getRenderViewport(); }


//==============================================================================
IGenViewportWnd *AssetViewerApp::getCurrentViewport() { return ged.getCurrentViewport(); }


//==============================================================================
void AssetViewerApp::setViewportZnearZfar(real zn, real zf) { ged.setZnearZfar(zn, zf); }


//==============================================================================
IGenViewportWnd *AssetViewerApp::screenToViewport(int &x, int &y) { return ged.screenToViewport(x, y); }


//==============================================================================
bool AssetViewerApp::getSelectionBox(BBox3 &box)
{
  box.setempty();

  IGenEditorPlugin *current = curPlugin();

  return current ? current->getSelectionBox(box) : false;
}

//==============================================================================
void AssetViewerApp::setupColliderParams(int, const BBox3 &) {}

//==================================================================================================
bool AssetViewerApp::traceRay(const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm, bool /*use_zero_plane*/)
{
  if (fabs(dir.y) < 0.0001)
    return false;

  real t = -src.y / dir.y;
  if (t > 0 && t < dist)
  {
    dist = t;

    if (out_norm)
    {
      if (dir.y < 0)
        *out_norm = Point3(0, 1, 0);
      else
        *out_norm = Point3(0, -1, 0);
    }

    return true;
  }

  return false;
}


//==============================================================================
void AssetViewerApp::actObjects(real dt)
{
  cpujobs::release_done_jobs();

  PropPanel::process_message_queue();

  ged.act(dt);

  static float timeToTrackFiles = 0;
  timeToTrackFiles -= dt;

  environment::renderEnviEntity(assetLtData);
  act_camera_objects_config_dialog();

  if (timeToTrackFiles < 0)
  {
    timeToTrackFiles = 0.5;
    if (assetMgr.trackChangesContinuous(-1))
      ::post_base_update_notify_dabuild();
  }

  for (int i = 0; i < plugin.size(); ++i)
    plugin[i]->actObjects(dt);

  // NOTE: ImGui porting: delay the selection of the last used asset (from the previous application run) till we have a
  // valid viewport. This is needed to make sure that when ViewportWindow::zoomAndCenter executes the viewport's size is
  // set. This is ugly but probably still more robust than handling it in ViewportWindow would be.
  if (!assetToInitiallySelect.empty())
  {
    IGenViewportWnd *viewport = ged.getViewport(0);
    if (viewport)
    {
      int viewportWidth = 0;
      int viewportHeight = 0;
      viewport->getViewportSize(viewportWidth, viewportHeight);
      if (viewportWidth > 0 && viewportHeight > 0)
      {
        const DagorAsset *asset = DAEDITOR3.getAssetByName(assetToInitiallySelect);
        assetToInitiallySelect.clear();
        if (asset)
          selectAsset(*asset);

        if (mPropPanel)
          mPropPanel->loadState(propPanelStateOfTheAssetToInitiallySelect, true);
        propPanelStateOfTheAssetToInitiallySelect.reset();
      }
    }
  }

  static unsigned last_t = 0;
  if (last_t + 1000 < get_time_msec())
  {
    perfmonstat::dump_stat();
    last_t = get_time_msec();
  }
}


//==============================================================================
void AssetViewerApp::beforeRenderObjects()
{
  ddsx::tex_pack2_perform_delayed_data_loading();
  ViewportWindow *vpw = ged.getRenderViewport();
  if (vpw)
  {
    if (!skipSetViewProj)
      vpw->setViewProj();

    int vp_w, vp_h;
    vpw->getViewportSize(vp_w, vp_h);
    TMatrix4 globTm = TMatrix4(vpw->getViewTm()) * vpw->getProjTm();
    prepare_debug_text_marks(globTm, vp_w, vp_h);
  }

  IGenEditorPlugin *curPlug = curPlugin();

  if (worldViewPosVarId == -2)
    worldViewPosVarId = ::get_shader_variable_id("world_view_pos");
  if (worldViewPosVarId >= 0)
    ShaderGlobal::set_color4(worldViewPosVarId, Color4(::grs_cur_view.pos.x, ::grs_cur_view.pos.y, ::grs_cur_view.pos.z, 1.f));

  plugin[0]->beforeRenderObjects();
  if (curPlug)
    curPlug->beforeRenderObjects();

  if (shouldUpdateViewports)
  {
    shouldUpdateViewports = false;
    ged.redrawClientRect();
  }
}


//==============================================================================
void AssetViewerApp::renderObjects()
{
  if (skipRenderObjects)
    return;
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  renderGrid();

  IGenEditorPlugin *curPlug = curPlugin();

  if (curPlug)
  {
    plugin[0]->renderObjects();
    curPlug->renderObjects();
  }
}


//==============================================================================
void AssetViewerApp::renderTransObjects()
{
  if (skipRenderObjects)
    return;
  IGenEditorPlugin *curPlug = curPlugin();

  if (curPlug)
  {
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
    plugin[0]->renderTransObjects();
    curPlug->renderTransObjects();
  }

  if (av_show_tex_helper.get())
  {
    int w = 1, h = 1;
    EDITORCORE->getRenderViewport()->getViewportSize(w, h);
    av_show_tex_helper->setTargetSize(Point2(w, h));
    av_show_tex_helper->render();
  }

  render_debug_text_marks();
}


//==============================================================================
void AssetViewerApp::renderGrid()
{
  if (DAEDITOR3.getStereoIndex() != StereoIndex::Mono)
    return;

  Tab<Point3> pt(tmpmem);
  Tab<Point3> dirs(tmpmem);

  const int gridConersNum = 4;
  const int gridDirectionsNum = 2;
  const real fakeGridSize = 200.f;
  const real perspectiveZoom = 600.f;

  pt.resize(gridConersNum);
  dirs.resize(gridDirectionsNum);

  TMatrix camera;
  ViewportWindow *vpw = ged.getRenderViewport();

  if (vpw->isOrthogonal() && grid.getUseInfiniteGrid())
    return;

  vpw->clientRectToWorld(pt.data(), dirs.data(), grid.getStep() * fakeGridSize);
  vpw->getCameraTransform(camera);

  float dh = fabs(camera.getcol(3).y - grid.getGridHeight()) + 1;
  grid.render(pt.data(), dirs.data(), fabs(vpw->isOrthogonal() ? vpw->getOrthogonalZoom() : perspectiveZoom / dh),
    ged.findViewportIndex(vpw));
}

//==============================================================================

void AssetViewerApp::setGizmo(IGizmoClient *gc, ModeType type)
{
  // set a new gizmoclient
  ged.tbManager->setGizmoClient(gc, type);

  if (gc && gizmoEH->getGizmoClient() == gc)
  {
    gizmoEH->setGizmoType(type);
    repaint();
    return;
  }

  if (gizmoEH->getGizmoClient())
    gizmoEH->getGizmoClient()->release();

  gizmoEH->setGizmoType(type);
  gizmoEH->setGizmoClient(gc);
  gizmoEH->zeroOverAndSelected();

  ged.setEH(appEH);
  repaint();
}


//==============================================================================

void AssetViewerApp::startGizmo(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (gizmoEH->getGizmoClient() && gizmoEH->getGizmoType() == MODE_None)
  {
    IGenEventHandler *eh = ged.curEH;
    ged.curEH = NULL;
    gizmoEH->handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);
    ged.curEH = eh;
  }
}

//==============================================================================

void AssetViewerApp::endGizmo(bool apply) { gizmoEH->endGizmo(apply); }

//==============================================================================

IEditorCoreEngine::ModeType AssetViewerApp::getGizmoModeType() { return gizmoEH->getGizmoType(); }

//==============================================================================

bool AssetViewerApp::isGizmoOperationStarted() const { return gizmoEH->isStarted(); }

//==============================================================================