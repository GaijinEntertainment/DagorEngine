// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <windows.h>
#undef ERROR

#define IMGUI_DEFINE_MATH_OPERATORS

#include "av_appwnd.h"
#include "av_plugin.h"
#include "av_cm.h"
#include "av_mainAssetSelector.h"
#include "av_viewportWindow.h"

#include "assetUserFlags.h"
#include "assetBuildCache.h"
#include "badRefFinder.h"
#include "compositeAssetCreator.h"

#include <assets/asset.h>
#include <assets/assetExporter.h>
#include <assets/assetMsgPipe.h>
#include <assets/assetHlp.h>
#include <assets/assetRefs.h>
#include <assetsGui/av_allAssetsTree.h>
#include <assetsGui/av_globalState.h>
#include <generic/dag_sort.h>

#include <util/dag_texMetaData.h>
#include <assets/assetUtils.h>
#include <libTools/util/strUtil.h>
#include <assets/assetExpCache.h>
#include <de3_lightService.h>
#include <de3_skiesService.h>
#include <de3_windService.h>
#include <de3_entityFilter.h>
#include <de3_interface.h>
#include <de3_rendInstGen.h>
#include <de3_dynRenderService.h>
#include <de3_editorEvents.h>
#include <render/debugTexOverlay.h>
#include <de3_dxpFactory.h>

#include <debug/dag_debug.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <assets/assetFolder.h>
#include <math/dag_Point2.h>
#include <startup/dag_globalSettings.h>

#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_startup.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_rect.h>
#include <EditorCore/ec_genapp_ehfilter.h>
#include <EditorCore/ec_selwindow.h>
#include <EditorCore/ec_viewportSplitter.h>

#include <daECS/core/entityManager.h>
#include <daECS/core/event.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/dag_vr.h>
#include <3d/dag_texIdSet.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_clipboard.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_shellExecute.h>
#include <propPanel/commonWindow/dialogManager.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/container.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/propPanel.h>
#include <propPanel/c_util.h>
#include <propPanel/constants.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>
#include <winGuiWrapper/wgw_busy.h>
#include <sepGui/wndGlobal.h>
#include <workCycle/dag_workCycle.h>
#include <stdio.h>
#include <ioSys/dag_memIo.h>

#include <gui/dag_imgui.h>
#include <gui/dag_stdGuiRenderEx.h>
#include <gameRes/dag_collisionResource.h>
#include <rendinst/rendinstGen.h>
#include <libTools/shaderResBuilder/matSubst.h>
#include <util/dag_delayedAction.h>
#include <util/dag_fastPtrList.h>
#include <render/dynmodelRenderer.h>

#include <impostorUtil/impostorBaker.h>
#include <impostorUtil/impostorGenerator.h>
#include <impostorUtil/options.h>

#include <pointCloudGen.h>

#include "Entity/colorDlgAppMat.h"
#include "Entity/compositeEditorPanel.h"
#include "Entity/compositeEditorTree.h"

#include <imgui/imgui.h>

#if _TARGET_PC_WIN
#include <osApiWrappers/dag_unicode.h>
#include <Shlobj.h>
#endif

using hdpi::_pxActual;
using hdpi::_pxScaled;

ECS_BROADCAST_EVENT_TYPE(ImGuiStage);
ECS_REGISTER_EVENT(ImGuiStage)

extern String fx_devres_base_path;

static String assetFile;
static String autoSavedWindowLayoutFilePath;
static unsigned collisionMask = 0;
static unsigned rendGeomMask = 0;
String a2d_last_ref_model;

extern void init_interface_de3();
extern bool de3_spawn_event(unsigned event_huid, void *user_data);
extern void init_rimgr_service(const DataBlock &appblk);
extern void init_prefabmgr_service(const DataBlock &appblk);
extern void init_fxmgr_service(const DataBlock &appblk);
extern void init_efxmgr_service();
extern void init_physobj_mgr_service();
extern void init_gameobj_mgr_service();
extern void init_composit_mgr_service();
extern void init_invalid_entity_service();
extern void init_dynmodel_mgr_service(const DataBlock &app_blk);
extern void init_animchar_mgr_service(const DataBlock &app_blk);
extern void init_csg_mgr_service();
extern void init_spline_gen_mgr_service();
extern void init_ecs_mgr_service(const DataBlock &app_blk, const char *app_dir);
extern void init_webui_service();
extern void init_das_mgr_service(const DataBlock &app_blk);

extern void mount_efx_assets(DagorAssetMgr &aMgr, const char *fx_blk_fname);

extern void register_phys_car_gameres_factory();
extern bool src_assets_scan_allowed;

static void init3d_early();

IWindService *av_wind_srv = NULL;
ISkiesService *av_skies_srv = NULL;
SimpleString av_skies_preset, av_skies_env, av_skies_wtype;
InitOnDemand<DebugTexOverlay> av_show_tex_helper;
static FastPtrList changedAssetsList;

enum
{
  PPANEL_WIDTH = 320,
  TREEVIEW_WIDTH = 280,
  TOOLBAR_HEIGHT = 26,
  ADDITIONAL_PROP_WINDOW_WIDTH = 280,
};


enum
{
  TREEVIEW_TYPE,
  PPANEL_TYPE,
  TOOLBAR_TYPE,
  VIEWPORT_TYPE,
  TOOLBAR_PLUGIN_TYPE,
  COMPOSITE_EDITOR_TREEVIEW_TYPE,
  COMPOSITE_EDITOR_PPANEL_TYPE,
};


AssetViewerApp &get_app() { return *((AssetViewerApp *)IEditorCoreEngine::get()); }


static inline unsigned getRendSubTypeMask() { return IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER); }


static void setMaskRend(unsigned ch_mask, bool set_or_unset)
{
  const unsigned maskType = IObjEntityFilter::STMASK_TYPE_RENDER;
  unsigned mask = IObjEntityFilter::getSubTypeMask(maskType);

  if (set_or_unset)
    mask |= ch_mask;
  else
    mask &= ~ch_mask;

  IObjEntityFilter::setSubTypeMask(maskType, mask);
}

static bool hasExternalEditor(const char *type_str)
{
  G_UNUSED(type_str);
  return false;
}

static void runExternalEditor(DagorAsset &a)
{
  const char *type_str = a.getTypeStr();
  String cmdLine;
  String rootDir(sgg::get_exe_path());
  if (rootDir.length() > 0 && rootDir[rootDir.length() - 2] == '/')
    erase_items(rootDir, rootDir.length() - 2, 1);

  G_UNUSED(type_str);
  if (cmdLine.empty())
    return;

  STARTUPINFO cif;
  PROCESS_INFORMATION pi;
  memset(&cif, 0, sizeof(cif));
  memset(&pi, 0, sizeof(pi));

  cif.wShowWindow = SW_SHOW;

  if (!CreateProcess(NULL, cmdLine, NULL, NULL, 0, CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP, NULL, NULL, &cif, &pi))
  {
    wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Error running external editor", "Application start failed:\n%s\n",
      cmdLine.str());
    return;
  }

  HWND mainWnd = (HWND)get_app().getWndManager()->getMainWindow();
  ::SendMessage(mainWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
  ::WaitForSingleObject(pi.hProcess, INFINITE);
  ::ShowWindow(mainWnd, SW_RESTORE);
  ::CloseHandle(pi.hProcess);
  ::CloseHandle(pi.hThread);
}


//=============================================================================


AssetViewerApp::AssetViewerApp(IWndManager *manager, const char *open_fname) :
  GenericEditorAppWindow(open_fname, manager),
  plugin(midmem),
  curPluginId(-1),
  curAsset(NULL),
  blockSave(false),
  scriptChangeFlag(false),
  autoZoomAndCenter(true),
  discardAssetTexMode(false)
{
  grid.setVisible(true, 0);

  IEditorCoreEngine::set(this);

  console->setCaption("AssetViewer console");

  appEH = new (inimem) AppEventHandler(*this);

  curAssetPackName = NULL;

  mTreeView = NULL;
  mPropPanel = NULL;
  mToolPanel = NULL;
  mPluginTool = NULL;

  rendGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
  collisionMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");

  mManager->registerWindowHandler(this);

#define REGISTER_COMMAND(cmd_name)               \
  if (!console->registerCommand(cmd_name, this)) \
    console->addMessage(ILogWriter::ERROR, "[AssetViewer2] Couldn't register command '" cmd_name "'");

  REGISTER_COMMAND("shaders.list");
  REGISTER_COMMAND("shaders.set");
  REGISTER_COMMAND("shaders.reload");

  REGISTER_COMMAND("perf.on");
  REGISTER_COMMAND("perf.off");
  REGISTER_COMMAND("perf.dump");
  REGISTER_COMMAND("tex.info");
  REGISTER_COMMAND("tex.refs");
  REGISTER_COMMAND("tex.show");
  REGISTER_COMMAND("tex.hide");

  REGISTER_COMMAND("camera.pos");
  REGISTER_COMMAND("camera.dir");

  REGISTER_COMMAND("driver.reset");
#undef REGISTER_COMMAND
}


AssetViewerApp::~AssetViewerApp()
{
  switchToPlugin(-1);
  if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
    rigenSrv->clearRtRIGenData();
  ged.curEH = NULL;
  ged.setEH(appEH);

  // Do not show the VR controls window at the next application start.
  if (imgui_window_is_visible("VR", "VR controls"))
    imgui_window_set_visible("VR", "VR controls", false);

  mManager->unregisterWindowHandler(this);
  editor_core_save_imgui_settings();
  PropPanel::remove_delayed_callback(*this);
  PropPanel::release();

  ::set_global_tex_name_resolver(NULL);
  assetMgr.enableChangesTracker(false);
  assetMgr.clear();

  terminateInterface();
  ::term_dabuild_cache();
  extern void console_close();
  console_close();
  imgui_shutdown();

  del_it(appEH);
}


void AssetViewerApp::init()
{
  DEBUG_CP();
  rendinst::rendinstClipmapShadows = false;
  ::init_dabuild_cache(sgg::get_exe_path_full());

  String imgPath(512, "%s%s", sgg::get_exe_path_full(), "../commonData/gui16x16/");
  ::dd_simplify_fname_c(imgPath.str());
  PropPanel::p2util::set_icon_path(imgPath.str());
  PropPanel::p2util::set_main_parent_handle(mManager->getMainWindow());
  mManager->initCustomMouseCursors(imgPath);

  init3d_early();
  editor_core_initialize_imgui(nullptr); // The path is not set to avoid saving the startup screen's settings.
  GenericEditorAppWindow::init();

  IGenViewportWnd *curVP = ged.getViewport(0);
  if (curVP)
  {
    curVP->setCameraPos(Point3(0, 0, 0));
    curVP->setCameraDirection(Point3(0, -0.36, 0.64), Point3(0, 0.64, 0.36));
    curVP->activate();
    ged.setCurrentViewportId(0);
    ged.setViewportCacheMode(VCM_NoCacheAll);
    zoomAndCenter();
  }

  addAccelerators();
}


void AssetViewerApp::addAccelerators()
{
  mManager->clearAccelerators();

  mManager->addAccelerator(CM_CAMERAS_FREE, ' ');
  mManager->addAccelerator(CM_CAMERAS_FPS, ' ', true);
  mManager->addAccelerator(CM_CAMERAS_TPS, ' ', true, false, true);
  mManager->addAccelerator(CM_ZOOM_AND_CENTER, 'Z', true, false, true); // For fly mode.

  mManager->addAccelerator(CM_DEBUG_FLUSH, 'F', true, true, false);
  mManager->addAccelerator(CM_DEBUG_FLUSH, 'F', true, false, true);

  if (CCameraElem::getCamera() != CCameraElem::MAX_CAMERA)
    return;

  mManager->addAccelerator(CM_SAVE_CUR_ASSET, 'S', true);
  mManager->addAccelerator(CM_SAVE_ASSET_BASE, 'S', true, false, true);
  mManager->addAccelerator(CM_RELOAD_SHADERS, 'R', true);

  mManager->addAccelerator(CM_ZOOM_AND_CENTER, 'Z'); // For normal mode.
  mManager->addAccelerator(CM_WINDOW_PPANEL_ACCELERATOR, 'P');
  mManager->addAccelerator(CM_WINDOW_COMPOSITE_EDITOR_ACCELERATOR, 'C');

  mManager->addAccelerator(CM_CREATE_SCREENSHOT, 'P', true);
  mManager->addAccelerator(CM_CREATE_CUBE_SCREENSHOT, 'P', true, false, true);

  mManager->addAccelerator(CM_NEXT_ASSET, wingw::V_ENTER, true);
  mManager->addAccelerator(CM_PREV_ASSET, wingw::V_ENTER, true, false, true);

  mManager->addAccelerator(CM_UNDO, 'Z', true);
  mManager->addAccelerator(CM_REDO, 'Y', true);

  if (IGenEditorPlugin *p = curPlugin())
    p->registerMenuAccelerators();

  ViewportWindow *viewport = ged.getCurrentViewport();
  if (viewport)
    viewport->registerViewportAccelerators(*mManager);
}


void AssetViewerApp::createAssetsTree() { mManager->setWindowType(nullptr, TREEVIEW_TYPE); }


void AssetViewerApp::createToolbar() { mManager->setWindowType(nullptr, TOOLBAR_TYPE); }


void AssetViewerApp::makeDefaultLayout()
{
  mManager->reset();
  // mManager->show(WSI_MAXIMIZED);

  mManager->setWindowType(nullptr, VIEWPORT_TYPE);
  mManager->setWindowType(nullptr, TREEVIEW_TYPE);
  mManager->setWindowType(nullptr, PPANEL_TYPE);

  createToolbar();
}


void AssetViewerApp::refillTree() { mTreeView->getAllAssetsTab().refillTree(); }

void AssetViewerApp::fillTree()
{
  mTreeView->setAssetMgr(assetMgr);

  DataBlock assetDataBlk(assetFile);
  mTreeView->getAllAssetsTab().fillTree(assetMgr, &assetDataBlk);
  ::dagor_idle_cycle();
  ::check_assets_base_up_to_date({}, true, true);
  AssetExportCache::saveSharedData();
  ::dagor_idle_cycle();
}


void AssetViewerApp::selectAsset(const DagorAsset &asset) { mTreeView->goToAsset(asset); }


void *AssetViewerApp::onWmCreateWindow(int type)
{
  switch (type)
  {
    case TREEVIEW_TYPE:
    {
      if (mTreeView)
        return nullptr;

      getMainMenu()->setEnabledById(CM_WINDOW_TREE, false);

      mTreeView = new MainAssetSelector(*this, *this);

      return mTreeView;
    }
    break;

    case PPANEL_TYPE:
    {
      if (mPropPanel)
        return nullptr;

      getMainMenu()->setCheckById(CM_WINDOW_PPANEL, true);

      mPropPanel = IEditorCoreEngine::get()->createPropPanel(this, "Properties");
      fillPropPanel();

      return mPropPanel;
    }
    break;

    case TOOLBAR_TYPE:
    {
      if (mToolPanel)
        return nullptr;

      getMainMenu()->setEnabledById(CM_WINDOW_TOOLBAR, false);

      mToolPanel = new PropPanel::ContainerPropertyControl(0, this, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0));
      fillToolBar();

      return mToolPanel;
    }
    break;

    case TOOLBAR_PLUGIN_TYPE:
    {
      if (mPluginTool)
        return nullptr;

      mPluginTool = new PropPanel::ContainerPropertyControl(0, this, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0));

      return mPluginTool;
    }
    break;

    case VIEWPORT_TYPE:
    {
      getMainMenu()->setEnabledById(CM_WINDOW_VIEWPORT, false);

      AssetViewerViewportWindow *v = new AssetViewerViewportWindow(nullptr, 0, 0, 0, 0);
      ged.addViewport(nullptr, appEH, mManager, v);
      return v;
    }
    break;

    case COMPOSITE_EDITOR_TREEVIEW_TYPE:
    {
      if (compositeEditor.compositeTreeView)
        return nullptr;

      compositeEditor.compositeTreeView =
        eastl::make_unique<CompositeEditorTree>(&compositeEditor, nullptr, 0, 0, 0, 0, "CompositeEditorTree");

      getMainMenu()->setCheckById(CM_WINDOW_COMPOSITE_EDITOR, isCompositeEditorShown());
      return compositeEditor.compositeTreeView.get();
    }
    break;

    case COMPOSITE_EDITOR_PPANEL_TYPE:
    {
      if (compositeEditor.compositePropPanel)
        return nullptr;

      compositeEditor.compositePropPanel = eastl::make_unique<CompositeEditorPanel>(&compositeEditor, 0, 0, 0, 0);

      return compositeEditor.compositePropPanel.get();
    }
    break;

    default: return nullptr;
  }
}

static void onDelayedRemoveCompositeEditorWindow(void *)
{
  CompositeEditor &compositeEditor = get_app().getCompositeEditor();
  if (compositeEditor.compositeTreeView)
    get_app().getWndManager()->removeWindow(compositeEditor.compositeTreeView.get());
  else if (compositeEditor.compositePropPanel)
    get_app().getWndManager()->removeWindow(compositeEditor.compositePropPanel.get());
}

bool AssetViewerApp::onWmDestroyWindow(void *window)
{
  if (mTreeView && mTreeView == window)
  {
    switchToPlugin(-1);

    del_it(mTreeView);
    getMainMenu()->setEnabledById(CM_WINDOW_TREE, true);
    return true;
  }

  if (mPropPanel && mPropPanel == window)
  {
    IGenEditorPlugin *current = curPlugin();
    if (current)
      current->onPropPanelClear(*mPropPanel);

    del_it(mPropPanel);
    getMainMenu()->setCheckById(CM_WINDOW_PPANEL, false);
    return true;
  }

  if (mToolPanel && mToolPanel == window)
  {
    del_it(mToolPanel);
    getMainMenu()->setEnabledById(CM_WINDOW_TOOLBAR, true);
    return true;
  }

  if (mPluginTool && mPluginTool == window)
  {
    del_it(mPluginTool);
    return true;
  }

  if (compositeEditor.compositeTreeView && compositeEditor.compositePropPanel &&
      (compositeEditor.compositeTreeView.get() == window || compositeEditor.compositePropPanel.get() == window))
  {
    getMainMenu()->setCheckById(CM_WINDOW_COMPOSITE_EDITOR, false);

    // We can't call removeWindow directly here on the other window because it would cause a crash in two cases:
    //   - when closing the tree window when it's movable (undocked)
    //   - when closing the application (in the destructor of VirtualWindow)
    add_delayed_callback((delayed_callback)&onDelayedRemoveCompositeEditorWindow, 0);

    // Let the two ifs below handle the actual releasing of the windows.
  }

  if (compositeEditor.compositeTreeView && compositeEditor.compositeTreeView.get() == window)
  {
    compositeEditor.compositeTreeView.reset();
    return true;
  }

  if (compositeEditor.compositePropPanel && compositeEditor.compositePropPanel.get() == window)
  {
    compositeEditor.compositePropPanel.reset();
    return true;
  }

  for (int i = 0; i < ged.getViewportCount(); ++i)
  {
    ViewportWindow *vport = ged.getViewport(i);
    if (vport && vport == window)
    {
      ged.deleteViewport(i);
      if (!ged.getViewportCount())
        getMainMenu()->setEnabledById(CM_WINDOW_VIEWPORT, true);
      return true;
    }
  }

  return false;
}


void AssetViewerApp::showPropWindow(bool is_show)
{
  const bool isCurrentlyShown = mPropPanel != nullptr;
  if (is_show == isCurrentlyShown)
    return;

  if (mPropPanel)
  {
    mManager->removeWindow(mPropPanel);
  }
  else
  {
    mManager->setWindowType(nullptr, PPANEL_TYPE);
  }
}


void AssetViewerApp::showAdditinalToolWindow(bool is_show)
{
  if ((bool)mPluginTool == is_show)
    return;

  if (mPluginTool)
  {
    mManager->removeWindow(mPluginTool);
    mPluginTool = nullptr;
  }
  else
  {
    mManager->setWindowType(nullptr, TOOLBAR_PLUGIN_TYPE);
  }
}

bool AssetViewerApp::isCompositeEditorShown() const { return compositeEditor.compositeTreeView != nullptr; }

void AssetViewerApp::showCompositeEditor(bool show)
{
  if (isCompositeEditorShown() == show)
    return;

  // Ensure that both windows are either closed or shown.
  if (compositeEditor.compositePropPanel)
    mManager->removeWindow(compositeEditor.compositePropPanel.get());
  if (compositeEditor.compositeTreeView)
    mManager->removeWindow(compositeEditor.compositeTreeView.get());

  if (show)
  {
    mManager->setWindowType(nullptr, COMPOSITE_EDITOR_TREEVIEW_TYPE);
    mManager->setWindowType(nullptr, COMPOSITE_EDITOR_PPANEL_TYPE);
  }

  compositeEditor.onCompositeEditorVisibilityChanged(show);
}

void AssetViewerApp::fillPropPanel()
{
  // G_ASSERT(mPropPanel && "AssetViewerApp::fillPropPanel(): mPropPanel == NULL!");
  if (!mPropPanel)
    return;

  if (scriptChangeFlag)
  {
    scriptChangeFlag = false;
    return;
  }

  if (compositeEditor.getPreventUiUpdatesWhileUsingGizmo())
    return;

  IGenEditorPlugin *current = curPlugin();

  propPanelState.reset();
  mPropPanel->saveState(propPanelState);

  if (current)
    current->onPropPanelClear(*mPropPanel);
  mPropPanel->clear();

  if (curAsset)
  {
    PropPanel::ContainerPropertyControl *grpCommon = mPropPanel->createGroup(ID_COMMON_GRP, "Common parameters");

    if (grpCommon)
    {
      grpCommon->createStatic(-1, String(64, "Type: %s", curAsset->getTypeStr()));
      grpCommon->createEditBox(ID_NAME, "Name:", curAsset->getName(), false);
      if (hasExternalEditor(curAsset->getTypeStr()))
        grpCommon->createButton(ID_RUN_EDIT, "Run external editor");
    }

    if (current)
    {

      PropPanel::ContainerPropertyControl *grpSpec = mPropPanel->createGroup(ID_SPEC_GRP, "Object specific parameters");
      current->fillPropPanel(*grpSpec);

      if (current->hasScriptPanel())
      {
        PropPanel::ContainerPropertyControl *grpScript = mPropPanel->createGroup(ID_SCRIPT_GRP, "");
        current->fillScriptPanel(*grpScript);
      }
    }
  }

  if (current)
    current->postFillPropPanel();

  mPropPanel->loadState(propPanelState);

  fillPropPanelHasBeenCalled = true;
}


void AssetViewerApp::fillToolBar()
{
  PropPanel::ContainerPropertyControl *_tb = mToolPanel->createToolbarPanel(CM_TOOLS, "");

  G_ASSERT(_tb && "AssetViewerApp::fillToolBar() Toolbar not created!");

  GenericEditorAppWindow::fillCommonToolbar(*_tb);

  _tb->createSeparator();

  _tb->createCheckBox(CM_COLLISION_PREVIEW, "Entity Collision Preview");
  _tb->setButtonPictures(CM_COLLISION_PREVIEW, "collision_preview");

  _tb->createCheckBox(CM_RENDER_GEOMETRY, "Entity Render Geometry");
  _tb->setButtonPictures(CM_RENDER_GEOMETRY, "render_entity_geom");

  _tb->createSeparator();

  _tb->createCheckBox(CM_AUTO_ZOOM_N_CENTER, "Use auto-zoom-and-center");
  _tb->setButtonPictures(CM_AUTO_ZOOM_N_CENTER, "zoom_and_center");

  _tb->createSeparator();

  _tb->createCheckBox(CM_DISCARD_ASSET_TEX, "Discard textures (show stub tex)");
  _tb->setButtonPictures(CM_DISCARD_ASSET_TEX, "discard_tex");

  _tb->createSeparator();

  _tb->createButton(CM_PALETTE, "Palette");
  _tb->setButtonPictures(CM_PALETTE, "palette");
  _tb->createSeparator();

  _tb->createButton(CM_CONSOLE, "Show/hide console");
  _tb->setButtonPictures(CM_CONSOLE, "console");

  _tb->createSeparator();
  _tb->createButton(CM_VR_ENABLE, "Enable VR view");
  _tb->setButtonPictures(CM_VR_ENABLE, "vr");
}


void AssetViewerApp::fillMenu(PropPanel::IMenu *menu)
{
  menu->clearMenu(ROOT_MENU_ITEM);

  // fill file menu
  menu->addSubMenu(ROOT_MENU_ITEM, CM_FILE_MENU, "File");
  menu->addItem(CM_FILE_MENU, CM_SAVE_CUR_ASSET, "Save current asset\tCrtl+S");
  menu->addItem(CM_FILE_MENU, CM_SAVE_ASSET_BASE, "Save asset base\tCrtl+Shift+S");
  menu->addItem(CM_FILE_MENU, CM_CHECK_ASSET_BASE, "Check asset base");
  menu->addSeparator(CM_FILE_MENU);
  menu->addItem(CM_FILE_MENU, CM_RELOAD_SHADERS, "Reload shaders bindump\tCtrl+R");

  // fill export menu
  menu->addSubMenu(ROOT_MENU_ITEM, CM_EXPORT, "Export");

  // fill settings
  menu->addSubMenu(ROOT_MENU_ITEM, CM_SETTINGS, "Settings");
  menu->addItem(CM_SETTINGS, CM_OPTIONS_SET_ACT_RATE, "Work cycle act rate...");
  menu->addItem(CM_SETTINGS, CM_CAMERAS, "Cameras...");
  menu->addItem(CM_SETTINGS, CM_SCREENSHOT, "Screenshot...");
  menu->addItem(CM_SETTINGS, CM_OPTIONS_STAT_DISPLAY_SETTINGS, "Stats settings...");
  menu->addSeparator(CM_SETTINGS);
  menu->addItem(CM_SETTINGS, CM_ENVIRONMENT_SETTINGS, "Environment settings...");
  menu->addSeparator(CM_SETTINGS);
  menu->addItem(CM_SETTINGS, CM_RENDER_GEOMETRY, "Entity render geometry");
  menu->addItem(CM_SETTINGS, CM_COLLISION_PREVIEW, "Entity collision preview");
  menu->addItem(CM_SETTINGS, CM_AUTO_ZOOM_N_CENTER, "Use auto-zoom-and-center");
  menu->addItem(CM_SETTINGS, CM_AUTO_SHOW_COMPOSITE_EDITOR, "Automatically toggle Composit Editor");

  // fill view menu
  menu->addSubMenu(ROOT_MENU_ITEM, CM_WINDOW, "View");
  menu->addItem(CM_WINDOW, CM_LOAD_DEFAULT_LAYOUT, "Load default layout");
  menu->addSeparator(CM_WINDOW);
  menu->addItem(CM_WINDOW, CM_LOAD_LAYOUT, "Load layout ...");
  menu->addItem(CM_WINDOW, CM_SAVE_LAYOUT, "Save layout ...");
  menu->addSeparator(CM_WINDOW);
  menu->addItem(CM_WINDOW, CM_WINDOW_TOOLBAR, "Toolbar");
  menu->addItem(CM_WINDOW, CM_WINDOW_TREE, "Tree");
  menu->addItem(CM_WINDOW, CM_WINDOW_PPANEL, "Properties\tP");
  menu->addItem(CM_WINDOW, CM_WINDOW_VIEWPORT, "Viewport");
  menu->addItem(CM_WINDOW, CM_WINDOW_COMPOSITE_EDITOR, "Composit Editor\tC");
  menu->addItem(CM_WINDOW, CM_DISCARD_ASSET_TEX, "Discard textures (show stub tex)");

  addExitCommand(menu);
}


void AssetViewerApp::updateMenu(PropPanel::IMenu *menu)
{
  // fill export menu
  if (impostorApp)
  {
    menu->addItem(CM_EXPORT, CM_EXPORT_CURRENT_IMPOSTOR, "Generate current impostor");
    menu->addItem(CM_EXPORT, CM_EXPORT_IMPOSTORS_CURRENT_PACK, "Generate impostors from current pack");
    menu->addItem(CM_EXPORT, CM_EXPORT_ALL_IMPOSTORS, "Generate all impostors");
    menu->addItem(CM_EXPORT, CM_CLEAR_UNUSED_IMPOSTORS, "Clear unused impostors");
  }
  if (pointCloudGen)
    menu->addItem(CM_EXPORT, CM_EXPORT_CURRENT_POINT_CLOUD, "Generate current point cloud");

  menu->addItem(CM_EXPORT, CM_BUILD_RESOURCES, "Export gameRes [pc]");
  menu->addItem(CM_EXPORT, CM_BUILD_TEXTURES, "Export texPack [pc]");
  menu->addSeparator(CM_EXPORT);
  menu->addItem(CM_EXPORT, CM_BUILD_ALL, "Export all [pc]");
  menu->addSeparator(CM_EXPORT);
  menu->addItem(CM_EXPORT, CM_BUILD_CLEAR_CACHE, "Clear cache [pc]");

  int platformCnt = getWorkspace().getAdditionalPlatforms().size();
  if (platformCnt)
  {
    for (int i = 0; i < platformCnt; i++)
    {
      menu->addSubMenu(CM_EXPORT, CM_PLATFORM_SUBMENU + i,
        String(256, "%c%c%c%c", _DUMP4C(getWorkspace().getAdditionalPlatforms()[i])));

      int m_index = 1 + i;

      menu->addItem(CM_PLATFORM_SUBMENU + i, CM_BUILD_RESOURCES + m_index, "Export gameRes");
      menu->addItem(CM_PLATFORM_SUBMENU + i, CM_BUILD_TEXTURES + m_index, "Export texPack");
      menu->addSeparator(CM_PLATFORM_SUBMENU + i);
      menu->addItem(CM_PLATFORM_SUBMENU + i, CM_BUILD_ALL + m_index, "Export all");
      menu->addSeparator(CM_PLATFORM_SUBMENU + i);
      menu->addItem(CM_PLATFORM_SUBMENU + i, CM_BUILD_CLEAR_CACHE + m_index, "Clear cache");
    }
  }

  menu->addSeparator(CM_EXPORT);
  menu->addItem(CM_EXPORT, CM_BUILD_ALL_PLATFORM_RES, "Export gameRes for All platform");
  menu->addItem(CM_EXPORT, CM_BUILD_ALL_PLATFORM_TEX, "Export texPack for All platform");
  menu->addItem(CM_EXPORT, CM_BUILD_ALL_PLATFORM, "Export All for All platform");
  menu->addSeparator(CM_EXPORT);
  menu->addItem(CM_EXPORT, CM_BUILD_CLEAR_CACHE_ALL, "Clear cache for All platform");
}


void AssetViewerApp::drawAssetInformation(IGenViewportWnd *wnd)
{
  using hdpi::_pxS;

  if (!curAsset /* || !getWorkspace()*/)
    return;
  if (curAssetPackName.empty())
    return;

  EcRect r = {0, 0, 0, 0};
  wnd->getViewportSize(r.r, r.b);
  real rc = r.r - r.l;

  StdGuiRender::set_font(0);
  StdGuiRender::set_color(COLOR_BLACK);

  const int textOffset = _pxS(10);
  const Point2 size(StdGuiRender::get_str_bbox(curAssetPackName).size().x + textOffset * 2, _pxS(20));
  const Point2 offset(0, _pxS(17));
  const int pSize = _pxS(60);

  bool is_pc = curAsset->testUserFlags(ASSET_USER_FLG_UP_TO_DATE_PC);
  StdGuiRender::set_color(is_pc ? COLOR_LTGREEN : COLOR_LTRED);

  StdGuiRender::render_box(rc - pSize - offset.x, 0, rc - offset.x, r.t + offset.y + textOffset / 2);
  StdGuiRender::set_color(COLOR_BLACK);
  StdGuiRender::draw_strf_to(rc - pSize - offset.x + textOffset, r.t + offset.y, "PC");
  int sz = pSize;

  for (int i = getWorkspace().getAdditionalPlatforms().size() - 1; i >= 0; i--)
  {
    unsigned p = getWorkspace().getAdditionalPlatforms()[i];
    if (p == _MAKE4C('iOS'))
    {
      bool is_iOS = curAsset->testUserFlags(ASSET_USER_FLG_UP_TO_DATE_iOS);
      StdGuiRender::set_color(is_iOS ? COLOR_LTGREEN : COLOR_LTRED);

      StdGuiRender::render_box(rc - pSize - offset.x - sz, 0, rc - offset.x - sz, r.t + offset.y + textOffset / 2);

      StdGuiRender::set_color(COLOR_BLACK);
      StdGuiRender::draw_strf_to(rc - pSize - offset.x - sz + textOffset, r.t + offset.y, "iOS");
    }
    else if (p == _MAKE4C('and'))
    {
      bool is_AND = curAsset->testUserFlags(ASSET_USER_FLG_UP_TO_DATE_AND);
      StdGuiRender::set_color(is_AND ? COLOR_LTGREEN : COLOR_LTRED);

      StdGuiRender::render_box(rc - pSize - offset.x - sz, 0, rc - offset.x - sz, r.t + offset.y + textOffset / 2);

      StdGuiRender::set_color(COLOR_BLACK);
      StdGuiRender::draw_strf_to(rc - pSize - offset.x - sz + textOffset, r.t + offset.y, "AND");
    }
    else
      continue;

    sz += pSize;
  }

  int boxLeft = rc - size.x - offset.x - sz;

  StdGuiRender::set_color(COLOR_WHITE);
  StdGuiRender::render_box(boxLeft, 0, boxLeft + size.x, r.t + offset.y + textOffset / 2);

  StdGuiRender::set_color(COLOR_BLACK);
  StdGuiRender::draw_strf_to(boxLeft + textOffset, r.t + offset.y, curAssetPackName);

  if (!curAssetPkgName.empty())
  {
    const Point2 pkgSize(StdGuiRender::get_str_bbox(curAssetPkgName).size().x + textOffset * 2, _pxS(20));
    boxLeft -= pkgSize.x;

    StdGuiRender::set_color(E3DCOLOR(192, 192, 192));
    StdGuiRender::render_box(boxLeft, 0, boxLeft + pkgSize.x, r.t + offset.y + textOffset / 2);

    StdGuiRender::set_color(COLOR_BLACK);
    StdGuiRender::draw_strf_to(boxLeft + textOffset, r.t + offset.y, curAssetPkgName);
  }

  if (const char *str = environment::getEnviTitleStr(&assetLtData))
  {
    int tw = StdGuiRender::get_str_bbox(str).size().x;
    r.l = _pxS(128) + _pxS(40);
    r.t = 0;
    r.r = boxLeft - _pxS(40);
    r.b = r.t + offset.y + textOffset / 2;
    int diff = r.r - r.l - _pxS(10) - tw;
    if (diff > 0)
      r.l += diff / 2, r.r -= diff / 2;
    StdGuiRender::set_color(COLOR_YELLOW);
    StdGuiRender::render_box(r.l, r.t, r.r, r.b);

    StdGuiRender::set_color(COLOR_BLACK);
    StdGuiRender::draw_strf_to((r.l + r.r - tw) / 2, r.t + offset.y, str);
  }
}


bool AssetViewerApp::resolveTextureName(const char *src_name, String &out_str)
{
  String tmp_stor;
  out_str = DagorAsset::fpath2asset(TextureMetaData::decodeFileName(src_name, &tmp_stor));
  DagorAsset *tex_a = assetMgr.findAsset(out_str, assetMgr.getTexAssetTypeId());

  if (tex_a)
  {
    out_str.printf(64, "%s*", tex_a->getName());
    if (get_managed_texture_id(out_str) != BAD_TEXTUREID)
      return true;

    TextureMetaData tmd;
    tmd.read(tex_a->props, "PC");
    out_str = tmd.encode(tex_a->getTargetFilePath(), &tmp_stor);
  }
  else if (!out_str.empty())
  {
    getConsole().addMessage(ILogWriter::ERROR, "tex %s for %s not found (src tex is %s)", out_str.str(), getCurrentDagName(),
      src_name);
  }
  return true;
}


void AssetViewerApp::onAssetRemoved(int asset_name_id, int asset_type)
{
  if (curAsset && curAsset->getNameId() == asset_name_id && curAsset->getType() == asset_type)
    switchToPlugin(-1);

  mTreeView->onAssetRemoved();
}

static bool check_asset_depends_on_asset(const DagorAsset *amain, const DagorAsset *a)
{
  if (IDagorAssetRefProvider *refProvider = amain->getMgr().getAssetRefProvider(amain->getType()))
  {
    Tab<IDagorAssetRefProvider::Ref> refs;
    refs = refProvider->getAssetRefs(*const_cast<DagorAsset *>(amain));
    for (int i = 0; i < refs.size(); ++i)
      if (refs[i].getAsset() == a)
        return true;
    for (int i = 0; i < refs.size(); ++i)
      if (refs[i].getAsset() && check_asset_depends_on_asset(refs[i].getAsset(), a))
        return true;
  }
  return false;
}

bool AssetViewerApp::reloadAsset(const DagorAsset &asset, int asset_name_id, int asset_type)
{
  environment::on_asset_changed(asset, assetLtData);

  if (curAsset && curAsset->getNameId() == asset_name_id && curAsset->getType() == asset_type)
  {
    G_ASSERT(curAsset == &asset);
    if (curPlugin() && !curPlugin()->reloadAsset(curAsset))
    {
      int id = curPluginId;
      switchToPlugin(-1);
      if (asset_type != assetMgr.getTexAssetTypeId())
        free_unused_game_resources();
      switchToPlugin(id);
      fillPropPanel();
    }
  }
  else if (curAsset && asset_type != assetMgr.getTexAssetTypeId() &&
           (check_asset_depends_on_asset(curAsset, &asset) || (curPlugin() && curPlugin()->reloadOnAssetChanged(&asset))))
  {
    if (asset.getType() == DAEDITOR3.getAssetTypeId("rendInst"))
      if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
        rigenSrv->createRtRIGenData(-1024, -1024, 32, 8, 8, 8, {}, -1024, -1024);
    if (curPlugin() && !curPlugin()->reloadAsset(curAsset))
    {
      int id = curPluginId;
      switchToPlugin(-1);
      free_unused_game_resources();
      switchToPlugin(id);
      fillPropPanel();
    }
  }
  else
  {
    return false;
  }
  return true;
}

void AssetViewerApp::onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type)
{
  if (!reloadAsset(asset, asset_name_id, asset_type))
    // We need to be able here to reload an asset which we don't view currently.
    // But changing const logic for all onAssetChanged() functions looks bad.
    const_cast<DagorAsset &>(asset).setUserFlags(ASSET_USER_FLG_NEEDS_RELOAD);
}

static void gatherTexRefs(DagorAsset *a, TextureIdSet &reftex)
{
  if (a->getType() == a->getMgr().getTexAssetTypeId())
  {
    TEXTUREID id = get_managed_texture_id(String(0, "%s*", a->getName()));
    if (id != BAD_TEXTUREID)
      reftex.add(id);
    return;
  }

  IDagorAssetRefProvider *refProvider = a->getMgr().getAssetRefProvider(a->getType());
  if (!refProvider)
    return;

  Tab<IDagorAssetRefProvider::Ref> refs;
  refs = refProvider->getAssetRefs(*a);
  for (int i = 0; i < refs.size(); ++i)
    if (DagorAsset *ra = refs[i].getAsset())
      gatherTexRefs(ra, reftex);
}
void AssetViewerApp::applyDiscardAssetTexMode()
{
  if (!discardAssetTexMode)
    dxp_factory_force_discard(false);
  else if (curAsset)
  {
    TextureIdSet reftex;
    gatherTexRefs(curAsset, reftex);
    dxp_factory_force_discard(reftex);
  }
}


IGenEditorPlugin *AssetViewerApp::getTypeSupporter(const DagorAsset *asset) const
{
  if (!asset)
    return NULL;

  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i]->supportAssetType(*asset))
      return plugin[i];

  return NULL;
}


void AssetViewerApp::splitProjectFilename(const char *filename, String &path, String &name) { ::split_path(filename, path, name); }


int AssetViewerApp::findPlugin(IGenEditorPlugin *p)
{
  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i] == p)
      return i;

  return -1;
}


String AssetViewerApp::getScreenshotNameMask(bool cube) const
{
  const char *sdk_dir = ((AssetViewerApp *)this)->getWorkspace().getSdkDir();
  if (!cube && screenshotObjOnTranspBkg && curAsset)
    return ::make_full_path(sdk_dir, String(0, "av_%s_%%03i.tga", curAsset->getName()));
  if (!cube && screenshotSaveJpeg)
    return ::make_full_path(sdk_dir, "av_scrn%03i.jpg");
  return ::make_full_path(sdk_dir, cube ? "av_cube%03i.tga" : "av_scrn%03i.tga");
}


void AssetViewerApp::showSelectWindow(IObjectsList *obj_list, const char *obj_list_owner_name)
{
  SelWindow selWnd(0, obj_list, obj_list_owner_name);

  if (selWnd.showDialog() == PropPanel::DIALOG_ID_OK)
  {
    Tab<String> names(tmpmem);
    selWnd.getSelectedNames(names);

    obj_list->onSelectedNames(names);
  }
}


void AssetViewerApp::zoomAndCenter()
{
  IGenEditorPlugin *current = curPlugin();
  BBox3 bounds;

  if (current && current->getSelectionBox(bounds))
  {
    ViewportWindow *curVP = ged.getActiveViewport();

    if (curVP)
      curVP->zoomAndCenter(bounds);
    else
      for (int i = 0; i < ged.getViewportCount(); ++i)
        ged.getViewport(i)->zoomAndCenter(bounds);
  }
}


Point3 AssetViewerApp::snapToGrid(const Point3 &p) const { return grid.snapToGrid(p); }


Point3 AssetViewerApp::snapToAngle(const Point3 &p) const { return grid.snapToAngle(p); }


Point3 AssetViewerApp::snapToScale(const Point3 &p) const { return grid.snapToScale(p); }


void AssetViewerApp::getDocTitleText(String &text)
{
  unsigned drv = d3d::get_driver_code().asFourCC();
  if (sceneFname[0])
    text.printf(300, "Asset viewer2  [%c%c%c%c]  - %s", _DUMP4C(drv), sceneFname);
  else
    text.printf(300, "Asset viewer2  %s  [%c%c%c%c]", build_version, _DUMP4C(drv));
}


void AssetViewerApp::saveTreeState()
{
  DEBUG_CP();
  if (!mTreeView || assetFile.empty())
    return;

  MainAssetSelector::MainAllAssetsTab &allAssetsTab = mTreeView->getAllAssetsTab();

  Tab<int> types(midmem);
  types = allAssetsTab.getShownAssetTypeIndexes();
  SimpleString filterAssetString(allAssetsTab.getFilterStr());

  DataBlock assetDataBlk;
  allAssetsTab.saveTreeData(assetDataBlk);
  if (const DagorAsset *a = allAssetsTab.getSelectedAsset())
  {
    assetDataBlk.setStr("lastSelAsset", a->getName());
    assetDataBlk.setStr("lastSelAssetType", a->getTypeStr());
  }
  assetDataBlk.setStr("a2d_lastUsedModelAsset", a2d_last_ref_model);

  DataBlock &setBlk = *assetDataBlk.addBlock("setting");
  setBlk.setBool("CollisionPreview", (getRendSubTypeMask() & collisionMask) == collisionMask);
  setBlk.setBool("autoZoomAndCenter", autoZoomAndCenter);
  setBlk.setBool("AutoShowCompositeEditor", compositeEditor.autoShow);

  DataBlock &assetsFiltersBlk = *assetDataBlk.addBlock("assets_filter");

  DataBlock *types_data = assetsFiltersBlk.addBlock("types");
  for (int i = 0; i < types.size(); ++i)
    types_data->addStr("type", assetMgr.getAssetTypeName(types[i]));

  DataBlock *all_types_data = assetsFiltersBlk.addBlock("all_types");
  for (int i = 0; i < assetMgr.getAssetTypesCount(); ++i)
    all_types_data->addStr("type", assetMgr.getAssetTypeName(i));

  assetsFiltersBlk.setStr("string", filterAssetString.str());

  bool needRenderGrid = grid.isVisible(0);
  environment::save_settings(assetDataBlk, &assetLtData, &assetDefaultLtData, needRenderGrid);
  if (av_skies_srv)
    environment::save_skies_settings(*assetDataBlk.addBlock("skies"));

  // DataBlock &layoutBlk = *assetDataBlk.addBlock("layout");
  // mManager->saveLayoutToDataBlock(layoutBlk);

  ged.save(assetDataBlk);
  grid.save(assetDataBlk);
  saveScreenshotSettings(assetDataBlk);
  getConsole().saveCmdHistory(assetDataBlk);
  AssetSelectorGlobalState::save(assetDataBlk);

  DataBlock &pluginsRootSettings = *setBlk.addBlock("plugins");
  for (int i = 0; i < plugin.size(); ++i)
  {
    DataBlock pluginSettings;
    plugin[i]->saveSettings(pluginSettings);
    if (!pluginSettings.isEmpty())
      pluginsRootSettings.addNewBlock(&pluginSettings, plugin[i]->getInternalName());
  }

  assetDataBlk.saveToTextFile(assetFile);
}

void AssetViewerApp::onClosing()
{
  saveTreeState();

  if (!autoSavedWindowLayoutFilePath.empty())
    mManager->saveLayout(autoSavedWindowLayoutFilePath);
}

bool AssetViewerApp::canCloseScene(const char *title)
{
  ::dagor_idle_cycle();
  IGenEditorPlugin *sup = getTypeSupporter(curAsset);
  if (curAsset)
    switchToPlugin(-1);

  ::dagor_idle_cycle();

  if (curAsset && sup && sup->supportEditing())
  {
    if (curAsset->isMetadataChanged())
      changedAssetsList.addPtr(curAsset);
    else
      changedAssetsList.delPtr(curAsset);
  }

  String list_of_changes;
  for (int i = 0; i < changedAssetsList.getList().size(); i++)
    if (const DagorAsset *a = reinterpret_cast<DagorAsset *>(changedAssetsList.getList()[i]))
      if (a->isMetadataChanged())
        list_of_changes.aprintf(0, "\n  %s", a->getNameTypified());
  if (list_of_changes.empty())
  {
    onClosing();
    return true;
  }

  ::dagor_idle_cycle();
  int ret =
    wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNOCANCEL, title, "Asset base changed, save changes?\n" + list_of_changes);

  if (ret == PropPanel::DIALOG_ID_CANCEL)
  {
    if (sup)
    {
      switchToPlugin(getPluginIdx(sup));
      fillPropPanel();
    }
    return false;
  }

  if (ret == PropPanel::DIALOG_ID_YES)
  {
    assetMgr.enableChangesTracker(false);
    onMenuItemClick(CM_SAVE_ASSET_BASE);
  }

  ::dagor_idle_cycle();
  onClosing();
  return true;
}


//-----------------------------------------------------------------

class ConsoleMsgPipe : public NullMsgPipe
{
public:
  virtual void onAssetMgrMessage(int msg_type, const char *msg, DagorAsset *asset, const char *asset_src_fpath)
  {
    G_ASSERT(msg_type >= NOTE && msg_type <= REMARK);
    ILogWriter::MessageType t = (ILogWriter::MessageType)msg_type;
    updateErrCount(msg_type);

    if (asset_src_fpath)
      get_app().getConsole().addMessage(t, "%s (file %s)", msg, asset_src_fpath);
    else if (asset)
      get_app().getConsole().addMessage(t, "%s (file %s)", msg, asset->getSrcFilePath());
    else
      get_app().getConsole().addMessage(t, msg);
  }
};
static ConsoleMsgPipe conPipe;


//-----------------------------------------------------------------

#include <libTools/util/setupTexStreaming.h>
#include <scene/dag_visibility.h>
#include <scene/dag_physMat.h>
#include <gui/dag_guiStartup.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <startup/dag_restart.h>
#include <startup/dag_startupTex.h>
#include <osApiWrappers/dag_basePath.h>

extern const char *av2_drv_name;

static void init3d_early()
{
  dgs_all_shader_vars_optionals = true;
  EDITORCORE->getWndManager()->init3d(av2_drv_name, nullptr);

  ::startup_shaders(String(260, "%s/../commonData/guiShaders", sgg::get_exe_path_full()));
  ::startup_game(RESTART_ALL);
  ShaderGlobal::enableAutoBlockChange(true);

  DataBlock fontsBlk;
  fontsBlk.addBlock("fontbins")->addStr("name", String(260, "%s/../commonData/default", sgg::get_exe_path_full()));
  if (auto *b = fontsBlk.addBlock("dynamicGen"))
  {
    b->setInt("texCount", 2);
    b->setInt("texSz", 256);
    b->setStr("prefix", String(260, "%s/../commonData", sgg::get_exe_path_full()));
  }
  StdGuiRender::init_dynamic_buffers(32 << 10, 8 << 10);
  StdGuiRender::init_fonts(fontsBlk);
  StdGuiRender::init_render();
  StdGuiRender::set_def_font_ht(0, hdpi::_pxS(StdGuiRender::get_initial_font_ht(0)));

  ::register_common_tool_tex_factories();
  ::register_png_tex_load_factory();
  ::register_avif_tex_load_factory();
}

namespace tools3d
{
extern void destroy();
}
static void init3d(const DataBlock &appblk)
{
  // shutdown imGui, render, shaders and d3d before reiniting
  imgui_shutdown();
  StdGuiRender::close_fonts();
  StdGuiRender::close_render();
  enable_res_mgr_mt(false, 0);
  ::startup_shaders(appblk.getStr("shadersAbs", "compiledShaders/tools")); // set new name before restarting
  shutdown_game(RESTART_VIDEODRV | RESTART_VIDEO);
  tools3d::destroy();

  // reinit 3d with actual shaders and texStreaming/texMgr setup for a project
  dgs_all_shader_vars_optionals = false;
  const DataBlock &blk_game = *appblk.getBlockByNameEx("game");
  const char *appdir = appblk.getStr("appDir");
  for (int i = 0; i < blk_game.blockCount(); i++)
    if (strcmp(blk_game.getBlock(i)->getBlockName(), "vromfs") == 0)
    {
      const DataBlock &blk_vrom = *blk_game.getBlock(i);
      VirtualRomFsData *vrom = ::load_vromfs_dump(String(0, "%s/%s", appdir, blk_vrom.getStr("file")), tmpmem);
      String mnt(blk_vrom.getStr("mnt"));
      if (mnt.empty())
      {
        ::add_vromfs(vrom);
        mnt.printf(0, "%s/%s/", appdir, blk_game.getStr("game_folder", "."));
        vrom = ::load_vromfs_dump(String(0, "%s/%s", appdir, blk_vrom.getStr("file")), tmpmem);
      }
      ::add_vromfs(vrom, false, str_dup(mnt, tmpmem));
    }

  DataBlock texStreamingBlk;
  ::load_tex_streaming_settings(String(0, "%s/application.blk", appblk.getStr("appDir")), &texStreamingBlk);
  texStreamingBlk.setBool("texLoadOnDemand", false);
  EDITORCORE->getWndManager()->init3d(av2_drv_name, &texStreamingBlk);
  if (int resv_tid_count = appblk.getInt("texMgrReserveTIDcount", 128 << 10))
  {
    enable_res_mgr_mt(false, 0);
    enable_res_mgr_mt(true, resv_tid_count);
  }

  extern void console_init();
  console_init();
  ::shaders_register_console(true);
  ::startup_game(RESTART_ALL);
  ShaderGlobal::enableAutoBlockChange(true);

  if (const char *gp_file = appblk.getBlockByNameEx("game")->getStr("params", NULL))
  {
    DataBlock *b = const_cast<DataBlock *>(dgs_get_game_params());
    b->load(String(0, "%s/%s", appdir, gp_file));
    b->removeBlock("rendinstExtra");
    b->removeBlock("rendinstOpt");
  }
  const_cast<DataBlock *>(dgs_get_game_params())->setBool("rendinstExtraAutoSubst", false);
  const_cast<DataBlock *>(dgs_get_game_params())->setInt("rendinstExtraMaxCnt", 32);

  DataBlock fontsBlk;
  fontsBlk.addBlock("fontbins")->addStr("name", String(260, "%s/../commonData/default", sgg::get_exe_path_full()));
  if (auto *b = fontsBlk.addBlock("dynamicGen"))
  {
    b->setInt("texCount", 2);
    b->setInt("texSz", 256);
    b->setStr("prefix", String(260, "%s/../commonData", sgg::get_exe_path_full()));
  }
  StdGuiRender::init_dynamic_buffers(appblk.getInt("guiMaxQuad", 256 << 10), appblk.getInt("guiMaxVert", 64 << 10));
  StdGuiRender::init_fonts(fontsBlk);
  StdGuiRender::init_render();
  StdGuiRender::set_def_font_ht(0, hdpi::_pxS(StdGuiRender::get_initial_font_ht(0)));

  if (appblk.getBool("useDynrend", false))
    dynrend::init();

  ::register_dynmodel_gameres_factory();
  ::register_rendinst_gameres_factory();
  ::register_geom_node_tree_gameres_factory();
  ::register_phys_sys_gameres_factory();
  ::register_phys_obj_gameres_factory();
  ::register_phys_car_gameres_factory();
  CollisionResource::registerFactory();

  ShaderGlobal::set_int(get_shader_variable_id("in_editor"), 1);
  ShaderGlobal::set_int(get_shader_variable_id("fake_lighting_computations", true), 1);
}

static ImpostorGenerator::GenerateResponse impostor_export_callback(DagorAsset *asset, unsigned int ind, unsigned int count)
{
  if (ImpostorGenerator::interrupted || !::get_app().getImpostorGenerator())
    return ImpostorGenerator::GenerateResponse::ABORT;
  if (!::get_app().getImpostorGenerator()->hasAssetChanged(asset))
    return ImpostorGenerator::GenerateResponse::SKIP;
  ::get_app().trackChangesContinuous(-1);
  ::get_app().getConsole().addMessage(ILogWriter::NOTE, "[%d/%d] Generating impostor %s", ind + 1, count, asset->getName());
  return ImpostorGenerator::GenerateResponse::PROCESS;
}

void read_mount_points(const DataBlock &appblk)
{
  const bool useAddonVromSrc = true;
  if (const DataBlock *mnt = appblk.getBlockByName("mountPoints"))
  {
    dblk::iterate_child_blocks(*mnt, [p = useAddonVromSrc ? "forSource" : "forVromfs"](const DataBlock &b) {
      dd_set_named_mount_path(b.getBlockName(), b.getStr(p));
    });
    dd_dump_named_mounts();
  }
}
bool AssetViewerApp::loadProject(const char *app_dir)
{
  wingw::set_busy(true);
  ::dagor_idle_cycle();
  char fpath_buf[512];
  app_dir = ::_fullpath(fpath_buf, app_dir, 512);
  ::dd_append_slash_c(fpath_buf);

  String fname(260, "%sapplication.blk", app_dir);

  DataBlock appblk;

  assetMgr.clear();
  assetMgr.setMsgPipe(&conPipe);

  if (!appblk.load(fname))
  {
    debug("cannot load %s", fname.str());
    wingw::set_busy(false);
    return false;
  }
  matParamsPath = appblk.getBlockByNameEx("game")->getStr("mat_params", "");
  appblk.setStr("appDir", app_dir);
  if (appblk.getStr("shaders", NULL))
    appblk.setStr("shadersAbs", String(260, "%s/%s", app_dir, appblk.getStr("shaders", NULL)));
  else
    appblk.setStr("shadersAbs", String(260, "%s/../commonData/compiledShaders/classic/tools", sgg::get_exe_path()));

  read_mount_points(appblk);


  init3d(appblk);
  assetlocalprops::init(app_dir, "develop/.asset-local");
  editor_core_initialize_imgui(assetlocalprops::makePath("_av_imgui.ini"));
  DataBlock::setRootIncludeResolver(app_dir);

  ::dagor_idle_cycle();
  ::dagor_set_game_act_rate(appblk.getInt("work_cycle_act_rate", 100));

  strcpy(sceneFname, fname);
  setDocTitle();

  const char *fx_nut = appblk.getBlockByNameEx("assets")->getStr("fxScriptsDir", NULL);
  if (fx_nut)
    fx_devres_base_path.printf(260, "%s/%s/", app_dir, fx_nut);

  // detect new gameres system model and setup engine support for it
  const DataBlock *exp_blk = appblk.getBlockByNameEx("assets")->getBlockByName("export");
  bool loadGameResPacks = appblk.getBlockByNameEx("assets")->getStr("gameRes", NULL) != NULL;
  bool loadDDSxPacks = appblk.getBlockByNameEx("assets")->getStr("ddsxPacks", NULL) != NULL;
  if (exp_blk)
  {
    ::set_gameres_sys_ver(2);
    if (!loadDDSxPacks)
      ::init_dxp_factory_service();
  }

  allUpToDateFlags = ASSET_USER_FLG_UP_TO_DATE_PC;

  int platformCnt = getWorkspace().getAdditionalPlatforms().size();
  for (int i = 0; i < platformCnt; i++)
  {
    unsigned p = getWorkspace().getAdditionalPlatforms()[i];
    if (p == _MAKE4C('iOS'))
      allUpToDateFlags |= ASSET_USER_FLG_UP_TO_DATE_iOS;
    else if (p == _MAKE4C('and'))
      allUpToDateFlags |= ASSET_USER_FLG_UP_TO_DATE_AND;
  }


  // scan game resources
  const DataBlock &blk = *appblk.getBlockByNameEx("assets");

  ::reset_game_resources();
  DataBlock scannedResBlk;
  set_gameres_scan_recorder(&scannedResBlk, blk.getStr("gameRes", NULL), blk.getStr("ddsxPacks", NULL));

  // read game resource from list
  if ((!blk.getStr("base", NULL) || blk.getBool("allowMixedMode", false)) && exp_blk && (loadGameResPacks || loadDDSxPacks))
  {
    const DataBlock &expBlk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");
    const char *packlist = expBlk.getStr("packlist", NULL);
    const char *v_dest_fname = expBlk.getBlockByNameEx("destGrpHdrCache")->getStr("PC", NULL);
    const char *v_game_relpath = expBlk.getBlockByNameEx("destGrpHdrCache")->getStr("gameRelatedPath", NULL);

    String ri_desc_fn, dm_desc_fn;
    if (const char *fn = blk.getBlockByNameEx("build")->getBlockByNameEx("rendInst")->getStr("descListOutPath", nullptr))
      ri_desc_fn.setStrCat(fn, ".bin");
    if (const char *fn = blk.getBlockByNameEx("build")->getBlockByNameEx("dynModel")->getStr("descListOutPath", nullptr))
      dm_desc_fn.setStrCat(fn, ".bin");

    String patch_dir;
    if (const char *dir = appblk.getBlockByNameEx("game")->getStr("res_patch_folder", NULL))
    {
      patch_dir.printf(0, "%s/%s", app_dir, dir);
      simplify_fname(patch_dir);
      DAEDITOR3.conNote("resource patches location: %s", patch_dir);
    }
    String game_dir(0, "%s/%s", app_dir, appblk.getBlockByNameEx("game")->getStr("game_folder", "game"));
    simplify_fname(game_dir);
    int game_dir_prefix_len = i_strlen(game_dir);

    if (packlist && expBlk.getBlockByNameEx("destination")->getStr("PC", NULL))
    {
      String mntPoint;
      assethlp::build_package_dest(mntPoint, expBlk, nullptr, app_dir, "PC", nullptr);

      String vrom_name;
      VirtualRomFsData *vrom = NULL;
      String plname(260, "%s/%s", mntPoint, packlist);
      simplify_fname(plname);

      if (v_dest_fname)
      {
        vrom_name.printf(260, "%s/%s", app_dir, v_dest_fname);
        simplify_fname(vrom_name);
        vrom = ::load_vromfs_dump(vrom_name, tmpmem);
        plname.printf(260, "%s/%s", v_game_relpath, packlist);
        if (vrom)
          ::add_vromfs(vrom);
        DAEDITOR3.conNote("loading main resources: %s (from VROMFS %s, %p)", plname.str(), vrom_name.str(), vrom);
      }
      else
        DAEDITOR3.conNote("loading main resources: %s", plname.str());

      ::load_res_packs_from_list(plname, loadGameResPacks, loadDDSxPacks, mntPoint);
      if (vrom)
      {
        ::remove_vromfs(vrom);
        tmpmem->free(vrom);
      }
      if (!ri_desc_fn.empty())
        gameres_append_desc(gameres_rendinst_desc, mntPoint + ri_desc_fn, "*");
      if (!dm_desc_fn.empty())
        gameres_append_desc(gameres_dynmodel_desc, mntPoint + dm_desc_fn, "*");

      String patch_fn(0, "%s/res/%s", patch_dir, dd_get_fname(vrom_name));
      const char *base_pkg_res_dir = nullptr;
      if (const char *add_folder_c = expBlk.getBlockByNameEx("destination")->getBlockByNameEx("additional")->getStr("PC", NULL))
      {
        String add_folder(0, "%s/%s", app_dir, add_folder_c);
        simplify_fname(add_folder);
        if (const char *pstr = strstr(mntPoint, add_folder))
          if (const char *add_folder_last = strrchr(add_folder, '/'))
            base_pkg_res_dir = pstr + strlen(add_folder) - strlen(add_folder_last) + 1;
      }
      if (base_pkg_res_dir)
        patch_fn.printf(0, "%s/%s/%s", patch_dir, base_pkg_res_dir, dd_get_fname(vrom_name));

      simplify_fname(patch_fn);
      if (!patch_dir.empty() && dd_file_exists(patch_fn) && (vrom = load_vromfs_dump(patch_fn, tmpmem)) != NULL)
      {
        DAEDITOR3.conNote("loading main resources (patch)");
        String mnt(0, "%s/res/", patch_dir);
        if (base_pkg_res_dir)
        {
          mnt.printf(0, "%s/%s/", patch_dir, base_pkg_res_dir);
          simplify_fname(mnt);
        }

        ::add_vromfs(vrom, true, mnt);
        load_res_packs_patch(mnt + packlist, vrom_name, loadGameResPacks, loadDDSxPacks);
        ::remove_vromfs(vrom);
        tmpmem->free(vrom);
      }
      if (!patch_dir.empty() && base_pkg_res_dir)
      {
        if (!ri_desc_fn.empty())
          gameres_patch_desc(gameres_rendinst_desc, patch_dir + "/" + base_pkg_res_dir + ri_desc_fn, "*", mntPoint + ri_desc_fn);
        if (!dm_desc_fn.empty())
          gameres_patch_desc(gameres_dynmodel_desc, patch_dir + "/" + base_pkg_res_dir + dm_desc_fn, "*", mntPoint + dm_desc_fn);
      }

      if (const char *add_pkg_folder = expBlk.getBlockByNameEx("destination")->getBlockByNameEx("additional")->getStr("PC", NULL))
      {
        // loading additional packages
        String base(260, "%s/%s/", app_dir, add_pkg_folder);
        simplify_fname(base);
        const char *v_name = dd_get_fname(v_dest_fname);
        alefind_t ff;
        FastNameMapEx pkg_list;

        const DataBlock &pkgBlk = *expBlk.getBlockByNameEx("packages");
        const DataBlock &skipBuilt = *expBlk.getBlockByNameEx("skipBuilt");
        bool no_pkg_scan = pkgBlk.getBool("dontScanOtherFolders", false);

        for (int i = 0; i < pkgBlk.blockCount(); i++)
          pkg_list.addNameId(pkgBlk.getBlock(i)->getBlockName());

        for (bool ok = ::dd_find_first(base + "*.*", DA_SUBDIR, &ff); ok; ok = ::dd_find_next(&ff))
        {
          if (!(ff.attr & DA_SUBDIR))
            continue;
          if (stricmp(ff.name, "CVS") == 0 || stricmp(ff.name, ".svn") == 0)
            continue;
          if (stricmp(patch_dir, String(0, "%s%s", base.str(), ff.name)) == 0)
            continue;
          if (no_pkg_scan)
          {
            if (pkg_list.getNameId(ff.name) < 0)
              DAEDITOR3.conNote("skip: %s (due to dontScanOtherFolders:b=yes)", ff.name);
            continue;
          }
          pkg_list.addNameId(ff.name);
        }
        ::dd_find_close(&ff);

        for (int i = 0; i < pkg_list.nameCount(); i++)
        {
          const char *ff_name = pkg_list.getName(i);
          if (skipBuilt.getBool(ff_name, false) && src_assets_scan_allowed)
          {
            DAEDITOR3.conNote("skip: %s", ff_name);
            continue;
          }

          assethlp::build_package_dest(mntPoint, expBlk, ff_name, app_dir, "PC", nullptr);
          vrom = ::load_vromfs_dump(mntPoint + v_name, tmpmem);
          if (vrom)
          {
            DAEDITOR3.conNote("loading resource package: %s", ff_name);
            ::add_vromfs(vrom, false, mntPoint.str());
            mntPoint.resize(strlen(mntPoint) + 1); // required to compensate \0-pos changes

            ::load_res_packs_from_list(mntPoint + packlist, loadGameResPacks, loadDDSxPacks, mntPoint);
            G_VERIFY(::remove_vromfs(vrom) == mntPoint.str());
            tmpmem->free(vrom);

            if (!ri_desc_fn.empty())
              gameres_append_desc(gameres_rendinst_desc, mntPoint + ri_desc_fn, ff_name);
            if (!dm_desc_fn.empty())
              gameres_append_desc(gameres_dynmodel_desc, mntPoint + dm_desc_fn, ff_name);
          }

          patch_fn.printf(0, "%s/%s/%s", patch_dir, mntPoint.str() + game_dir_prefix_len, v_name);
          simplify_fname(patch_fn);
          if (!patch_dir.empty() && dd_file_exists(patch_fn) && (vrom = load_vromfs_dump(patch_fn, tmpmem)) != NULL)
          {
            DAEDITOR3.conNote("loading resource package: %s (patch)", ff_name);
            String mnt(0, "%s/%s/", patch_dir, mntPoint.str() + game_dir_prefix_len);
            simplify_fname(mnt);
            ::add_vromfs(vrom, true, mnt);
            load_res_packs_patch(mnt + packlist, mntPoint + v_name, loadGameResPacks, loadDDSxPacks);
            ::remove_vromfs(vrom);
            tmpmem->free(vrom);
          }
          if (!patch_dir.empty())
          {
            if (!ri_desc_fn.empty())
              gameres_patch_desc(gameres_rendinst_desc, patch_dir + "/" + (mntPoint.str() + game_dir_prefix_len) + ri_desc_fn, ff_name,
                mntPoint + ri_desc_fn);
            if (!dm_desc_fn.empty())
              gameres_patch_desc(gameres_dynmodel_desc, patch_dir + "/" + (mntPoint.str() + game_dir_prefix_len) + dm_desc_fn, ff_name,
                mntPoint + dm_desc_fn);
          }
        }
      }
      gameres_final_optimize_desc(gameres_rendinst_desc, "riDesc");
      gameres_final_optimize_desc(gameres_dynmodel_desc, "dynModelDesc");
    }
  }

  // load asset base
  int base_nid = blk.getNameId("base");

  console->showConsole();
  console->startLog();
  console->startProgress();

  assetMgr.setupAllowedTypes(*blk.getBlockByNameEx("types"), blk.getBlockByName("export"));
  if (const DataBlock *remap =
        appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("prefab")->getBlockByName("remapShaders"))
    static_geom_mat_subst.setupMatSubst(*remap);
  static_geom_mat_subst.setMatProcessor(new GenericTexSubstProcessMaterialData(nullptr, DataBlock::emptyBlock, &assetMgr, console));


  for (int i = 0; src_assets_scan_allowed && i < blk.paramCount(); i++)
    if (blk.getParamNameId(i) == base_nid && blk.getParamType(i) == DataBlock::TYPE_STRING)
    {
      fname.printf(260, "%s%s", app_dir, blk.getStr(i));
      assetMgr.loadAssetsBase(fname, "global");
    }
  if (true)
  {
    dag::ConstSpan<DagorAsset *> assets = assetMgr.getAssets();
    for (int i = 0; i < assets.size(); i++)
      if (assets[i]->getType() == assetMgr.getTexAssetTypeId())
      {
        TEXTUREID tid = get_managed_texture_id(String(0, "%s*", assets[i]->getName()));
        if (tid != BAD_TEXTUREID)
        {
          DAEDITOR3.conWarning("using texture %s (tid=%d) from asset, not from *.DxP.bin", assets[i]->getName(), tid);
          evict_managed_tex_id(tid);
        }
      }
  }

  const DataBlock *skipTypes = blk.getBlockByName("ignorePrebuiltAssetTypes");
  if (!skipTypes)
    skipTypes = blk.getBlockByNameEx("export")->getBlockByNameEx("types");

  if (blk.getStr("gameRes", NULL) || blk.getStr("ddsxPacks", NULL))
  {
    int nid = blk.getNameId("prebuiltGameResFolder");
    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamType(i) == DataBlock::TYPE_STRING && blk.getParamNameId(i) == nid)
      {
        String resDir(260, "%s/%s/", app_dir, blk.getStr(i));
        ::scan_for_game_resources(resDir, true, blk.getStr("ddsxPacks", NULL));
      }
  }
  set_gameres_scan_recorder(NULL, NULL, NULL);
  // scannedResBlk.saveToTextFile("res_list_AV.blk");

  if (blk.getStr("gameRes", NULL) || blk.getStr("ddsxPacks", NULL))
    assetMgr.mountBuiltGameResEx(scannedResBlk, *skipTypes);

  if (const char *efx_fname = appblk.getBlockByNameEx("assets")->getStr("efxBlk", NULL))
    mount_efx_assets(assetMgr, String(0, "%s/%s", app_dir, efx_fname));

  if (exp_blk && loadDDSxPacks)
    ::init_dxp_factory_service();

  if (get_max_managed_texture_id())
    debug("tex/res registered before AssetBase: %d", get_max_managed_texture_id().index());
  if (::get_gameres_sys_ver() == 2)
  {
    dag::ConstSpan<DagorAsset *> assets = assetMgr.getAssets();
    int hqtex_ns = assetMgr.getAssetNameSpaceId("texHQ");
    int tqtex_ns = assetMgr.getAssetNameSpaceId("texTQ");
    for (int i = 0; i < assets.size(); i++)
      if (assets[i]->getType() == assetMgr.getTexAssetTypeId() && assets[i]->getNameSpaceId() != hqtex_ns &&
          assets[i]->getNameSpaceId() != tqtex_ns)
      {
        TextureMetaData tmd;
        tmd.read(assets[i]->props, "PC");
        SimpleString nm(tmd.encode(String(128, "%s*", assets[i]->getName())));
        if (get_managed_texture_id(nm) == BAD_TEXTUREID)
          add_managed_texture(nm);
      }
    if (get_max_managed_texture_id())
      debug("tex/res registered with AssetBase:   %d", get_max_managed_texture_id().index());
  }

  int pc = ::bind_dabuild_cache_with_mgr(assetMgr, appblk, app_dir);
  if (pc < 0)
    console->addMessage(console->ERROR, "cannot init daBuild");
  else if (pc == 0)
    console->addMessage(console->WARNING, "daBuild inited but is useless, due to there are no asset types for export");

  autoSavedWindowLayoutFilePath = assetlocalprops::makePath("_av_window_layout.blk");
  if (dd_file_exists(autoSavedWindowLayoutFilePath.c_str()))
  {
    if (!mManager->loadLayout(autoSavedWindowLayoutFilePath.c_str()))
      makeDefaultLayout();

    if (!mTreeView)
      createAssetsTree();

    if (!mToolPanel)
      createToolbar();
  }
  else
    makeDefaultLayout();
  ::dagor_idle_cycle();
  G_ASSERTF_RETURN(mTreeView, false, "Failed to create AssetViewer windows.\nTry removing .asset-local/_av_window_layout.blk");

  // init dynamic or classic renderer
  EDITORCORE->queryEditorInterface<IDynRenderService>()->setup(app_dir, appblk);
  EDITORCORE->queryEditorInterface<IDynRenderService>()->init();
  EDITORCORE->queryEditorInterface<IDynRenderService>()->setRenderOptEnabled(IDynRenderService::ROPT_SHADOWS_VSM, false);

  // apply AV setting
  DataBlock assetDataBlk;
  assetFile = assetlocalprops::makePath("_av.blk");
  assetDataBlk.load(assetFile);

  ged.load(assetDataBlk);
  grid.load(assetDataBlk);
  loadScreenshotSettings(assetDataBlk);
  getConsole().loadCmdHistory(assetDataBlk);
  AssetSelectorGlobalState::load(assetDataBlk);

  fillTree();
  ::dagor_idle_cycle();

  if (0)
  {
    dag::ConstSpan<DagorAsset *> assets = assetMgr.getAssets();
    debug("%d assets", assets.size());

    for (int i = 0; i < assets.size(); i++)
    {
      const DagorAsset &a = *assets[i];
      debug("  [%d] <%s> name=%s nspace=%s(%d) isVirtual=%d folder=%d srcPath=%s", i, assetMgr.getAssetTypeName(a.getType()),
        a.getName(), a.getNameSpace(), a.isGloballyUnique(), a.isVirtual(), a.getFolderIndex(), a.getSrcFilePath());
    }
  }

  ::set_global_tex_name_resolver(this);
  assetMgr.enableChangesTracker(true);
  assetMgr.subscribeUpdateNotify(this, -1, -1);

  ::dagor_idle_cycle();

  const DataBlock &setBlk = *assetDataBlk.getBlockByNameEx("setting");

  bool previewCollision = setBlk.getBool("CollisionPreview", true);
  setMaskRend(collisionMask, previewCollision);
  setMaskRend(1 << DAEDITOR3.registerEntitySubTypeId("hidden_geom"), false);

  bool collision_check = (getRendSubTypeMask() & collisionMask) == collisionMask;
  getMainMenu()->setCheckById(CM_COLLISION_PREVIEW, collision_check);
  mToolPanel->setBool(CM_COLLISION_PREVIEW, collision_check);

  bool rend_check = (getRendSubTypeMask() & rendGeomMask) == rendGeomMask;
  getMainMenu()->setCheckById(CM_RENDER_GEOMETRY, rend_check);
  mToolPanel->setBool(CM_RENDER_GEOMETRY, rend_check);

  autoZoomAndCenter = setBlk.getBool("autoZoomAndCenter", true);
  getMainMenu()->setCheckById(CM_AUTO_ZOOM_N_CENTER, autoZoomAndCenter);
  mToolPanel->setBool(CM_AUTO_ZOOM_N_CENTER, autoZoomAndCenter);

  compositeEditor.autoShow = setBlk.getBool("AutoShowCompositeEditor", compositeEditor.autoShow);
  getMainMenu()->setCheckById(CM_AUTO_SHOW_COMPOSITE_EDITOR, compositeEditor.autoShow);

  for (int i = 0; i < plugin.size(); ++i)
    plugin[i]->onLoadLibrary();
  ::dagor_idle_cycle();

  bool showCon = (console->hasErrors() || console->hasWarnings());

  console->endLogAndProgress();

  queryEditorInterface<ISceneLightService>()->reset();

  ShaderGlobal::set_texture_fast(get_shader_glob_var_id("lightmap_tex", true),
    add_managed_texture(String(260, "%s%s", sgg::get_exe_path_full(), "../commonData/tex/lmesh_ltmap.tga")));

  const DataBlock &b = *appblk.getBlockByNameEx("tex_shader_globvar");
  for (int i = 0; i < b.paramCount(); i++)
    if (b.getParamType(i) == DataBlock::TYPE_STRING)
    {
      int gvid = get_shader_glob_var_id(b.getParamName(i), true);
      if (gvid < 0)
        DAEDITOR3.conWarning("cannot set tex <%s> to var <%s> - shader globvar is missing", b.getStr(i), b.getParamName(i));
      else
      {
        TEXTUREID tid = add_managed_texture(b.getStr(i));

        if (!acquire_managed_tex(tid))
          DAEDITOR3.conWarning("cannot set tex <%s> to var <%s> - texture is missing", b.getStr(i), b.getParamName(i));
        else
        {
          ShaderGlobal::set_texture_fast(gvid, tid);
          DAEDITOR3.conNote("set tex <%s> to globvar <%s>", b.getStr(i), b.getParamName(i));
          release_managed_tex(tid);
        }
      }
    }

  String phmat_fn(getWorkspace().getPhysmatPath());
  if (::dd_file_exist(phmat_fn))
    PhysMat::init(phmat_fn);
  else if (!phmat_fn.empty())
    DAEDITOR3.conError("PhysMat: can't find physmat file: '%s'", phmat_fn);

  if (!showCon && !(console->hasErrors() || console->hasWarnings()))
  {
    // SetActiveWindow is needed otherwise the application gets deactivated for some reason when hiding the console.
    SetActiveWindow((HWND)mManager->getMainWindow());
    console->hideConsole();
  }

#define INIT_SERVICE(TYPE_NAME, EXPR)          \
  if (assetMgr.getAssetTypeId(TYPE_NAME) >= 0) \
    EXPR;                                      \
  else                                         \
    debug("skipped initing service for \"%s\", asset manager did not declare such type", TYPE_NAME);

  INIT_SERVICE("rendInst", ::init_rimgr_service(appblk));
  INIT_SERVICE("prefab", ::init_prefabmgr_service(appblk));
  INIT_SERVICE("fx", ::init_fxmgr_service(appblk));
  INIT_SERVICE("efx", ::init_efxmgr_service());
  INIT_SERVICE("composit", ::init_composit_mgr_service());
  INIT_SERVICE("physObj", ::init_physobj_mgr_service());
  INIT_SERVICE("gameObj", ::init_gameobj_mgr_service());
  ::init_invalid_entity_service();
  INIT_SERVICE("animChar", ::init_animchar_mgr_service(appblk));
  INIT_SERVICE("dynModel", ::init_dynmodel_mgr_service(appblk));
  INIT_SERVICE("csg", ::init_csg_mgr_service());
  INIT_SERVICE("spline", ::init_spline_gen_mgr_service());
  ::init_ecs_mgr_service(appblk, getWorkspace().getAppDir());
  if (appblk.getBlockByNameEx("game")->getBool("enable_webui_av2", true))
    ::init_webui_service();
  else
    debug("webUi disabled with game{ enable_webui_av2:b=no;");

  init_das_mgr_service(appblk);

  ::init_interface_de3();
  ::init_all_editor_plugins();

#undef INIT_SERVICE

  assetLtData.createSun();
  assetDefaultLtData.loadDefaultSettings(appblk);

  bool needRenderGrid = false;
  environment::load_settings(assetDataBlk, &assetLtData, &assetDefaultLtData, needRenderGrid);
  grid.setVisible(needRenderGrid, 0);

  assetLtData.setReflectionTexture();
  assetLtData.setEnvironmentTexture();
  assetLtData.applyMicrodetailFromLevelBlk();

  const DataBlock &pluginsRootSettings = *setBlk.getBlockByNameEx("plugins");
  for (int i = 0; i < plugin.size(); ++i)
  {
    const DataBlock &pluginSettings = *pluginsRootSettings.getBlockByNameEx(plugin[i]->getInternalName());
    plugin[i]->loadSettings(pluginSettings);
  }

  DataBlock *assetsFiltersBlk = assetDataBlk.getBlockByName("assets_filter");
  if (assetsFiltersBlk)
  {
    DataBlock *typesData = assetsFiltersBlk->getBlockByName("types");
    DataBlock *all_types_data = assetsFiltersBlk->getBlockByName("all_types");
    OAHashNameMap<true> allTypesCurrent;

    for (int i = 0; i < assetMgr.getAssetTypesCount(); ++i)
      allTypesCurrent.addNameId(assetMgr.getAssetTypeName(i));

    if (typesData)
    {
      Tab<int> types(midmem);
      for (int i = 0; i < typesData->paramCount(); ++i)
      {
        int type_id = assetMgr.getAssetTypeId(typesData->getStr(i));
        if (type_id != -1)
          types.push_back(type_id);
      }

      if (all_types_data)
      {
        OAHashNameMap<true> allTypesSaved;
        for (int i = 0; i < all_types_data->paramCount(); ++i)
          allTypesSaved.addNameId(all_types_data->getStr(i));

        iterate_names(allTypesCurrent, [&](int, const char *name) {
          if (allTypesSaved.getNameId(name) == -1)
            types.push_back(assetMgr.getAssetTypeId(name));
        });
        mTreeView->getAllAssetsTab().setShownAssetTypeIndexes(types);
      }
    }

    const char *str = assetsFiltersBlk->getStr("string");
    if (str && *str)
      mTreeView->getAllAssetsTab().setFilterStr(str);
  }

  de3_spawn_event(HUID_BeforeMainLoop, nullptr);
  a2d_last_ref_model = assetDataBlk.getStr("a2d_lastUsedModelAsset", "");
  if (const char *asset_nm = assetDataBlk.getStr("lastSelAsset", NULL))
    if (*asset_nm)
    {
      int atype = assetMgr.getAssetTypeId(assetDataBlk.getStr("lastSelAssetType", ""));
      if (DagorAsset *a = atype < 0 ? assetMgr.findAsset(asset_nm) : assetMgr.findAsset(asset_nm, atype))
        assetToInitiallySelect = a->getNameTypified();
    }

  const DataBlock &envi_def = *appblk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("envi");
  const char *type = envi_def.getStr("type", "classic");
  if (strcmp(type, "skies") == 0)
  {
    av_skies_srv = queryEditorInterface<ISkiesService>();
    if (av_skies_srv)
    {
      av_skies_srv->setup(app_dir, envi_def);
      av_skies_srv->init();

      Tab<String> preset(midmem), env(midmem), wtype(midmem);
      av_skies_srv->fillPresets(preset, env, wtype);

      if (preset.size() && env.size() && wtype.size())
        environment::load_skies_settings(*assetDataBlk.getBlockByNameEx("skies"));
    }
    av_wind_srv = queryEditorInterface<IWindService>();
    if (av_wind_srv)
      av_wind_srv->init(app_dir, envi_def);
  }
  float zn = 0.1f, zf = 10000.0f;
  setViewportZnearZfar(envi_def.getReal("znear", zn), envi_def.getReal("zfar", zf));
  getViewport(0)->getZnearZfar(zn, zf);
  setViewportZnearZfar(assetDataBlk.getReal("av_znear", zn), assetDataBlk.getReal("av_zfar", zf));

  if (assetMgr.getAssetTypeId("rendInst") >= 0 && assetMgr.getAssetTypeId("impostorData") >= 0)
    impostorApp = eastl::make_unique<ImpostorGenerator>(app_dir, appblk, &assetMgr, console, false, impostor_export_callback);

  colorPaletteDlg = eastl::make_unique<ColorDialogAppMat>(PropPanel::p2util::get_main_parent_handle(), "Palette");

  if (assetMgr.getAssetTypeId("rendInst") >= 0)
  {
    pointCloudGen = eastl::make_unique<plod::PointCloudGenerator>(app_dir, appblk, &assetMgr, console);
    if (!pointCloudGen->isInitialized())
      pointCloudGen.reset();
  }

  wingw::set_busy(false);
  queryEditorInterface<IDynRenderService>()->enableRender(true);
  return true;
}


bool AssetViewerApp::saveProject(const char *filename)
{
  if (blockSave)
    return true;

  console->showConsole();
  console->startLog();
  console->startProgress();

  for (int i = 0; i < plugin.size(); ++i)
    plugin[i]->onSaveLibrary();

  assetMgr.saveAssetsMetadata();

  bool showCon = (console->hasErrors() || console->hasWarnings());

  console->endLogAndProgress();

  if (showCon)
    console->showConsole();

  return true;
}


void AssetViewerApp::blockModifyRoutine(bool block) { blockSave = block; }


void AssetViewerApp::afterUpToDateCheck(bool changed)
{
  DEBUG_CP();
  mTreeView->getAllAssetsTab().markExportedTree(allUpToDateFlags);
  DEBUG_CP();
}

bool AssetViewerApp::trackChangesContinuous(int assets_to_check) { return assetMgr.trackChangesContinuous(assets_to_check); }

void AssetViewerApp::invalidateAssetIfChanged(DagorAsset &a)
{
  // if (assetMgr.isAssetMetadataChanged(a))
  dabuildcache::invalidate_asset(a, true);
}

void AssetViewerApp::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  // Clicking on the toolbar button removes the focus from the viewport, so restore it here.
  switch (pcb_id)
  {
    case CM_CAMERAS_FREE:
    case CM_CAMERAS_FPS:
    case CM_CAMERAS_TPS:
    case CM_CAMERAS_CAR: ged.activateCurrentViewport(); break;
  }

  if (onMenuItemClick(pcb_id))
    return;

  switch (pcb_id)
  {
    case CM_NAVIGATE:
      wingw::message_box(0, "Navigation info",
        "Rotate:\t\t Alt + MiddleMouseButton\n"
        "Pan:\t\t MiddleMouseButton\n"
        "Zoom:\t\t MouseWhell\n"
        "Zoom extents:\t Z\n"
        "Free camera:\t Space\n"
        "FPS camera:\t Ctrl+Space\n"
        "Accelerate:\t Ctrl");
      break;

      // CM_STATS

    case ID_RUN_EDIT:
      if (curAsset)
        runExternalEditor(*curAsset);
      return;

    case CM_CHANGE_FOV: onChangeFov(); break;

    case CM_CONSOLE: console->showConsole(true); break;

    case CM_VR_ENABLE: EDITORCORE->queryEditorInterface<IDynRenderService>()->toggleVrMode(); break;

    case CM_PALETTE:
      if (colorPaletteDlg->isVisible())
        colorPaletteDlg->hide();
      else
        colorPaletteDlg->show();
      break;
    default: break;
  }
}

static void build_currently_assets(AllAssetsTree *tree, dag::ConstSpan<unsigned> tc)
{
  PropPanel::TLeafHandle sel = tree->getSelectedItem();

  void *data = tree->getItemData(sel);
  if (!get_dagor_asset_folder(data))
    ::build_assets(tc, make_span_const((DagorAsset **)&data, 1));
}


static void build_currently_assets(AllAssetsTree *tree)
{
  Tab<unsigned> tc(get_app().getWorkspace().getAdditionalPlatforms(), tmpmem);
  tc.push_back(_MAKE4C('PC'));

  ::build_currently_assets(tree, tc);
}


static inline void make_hierarchical_list(AllAssetsTree *tree, PropPanel::TLeafHandle item, Tab<PropPanel::TLeafHandle> &items)
{
  int count = tree->getChildrenCount(item);
  for (int i = 0; i < count; ++i)
  {
    PropPanel::TLeafHandle child = tree->getChild(item, i);
    items.push_back(child);
    ::make_hierarchical_list(tree, child, items);
  }
}

static int find_folder_idx(const DagorAssetFolder *f)
{
  if (!f)
    return -1;
  const DagorAssetMgr &mgr = ::get_app().getAssetMgr();
  for (int i = 0;; ++i)
  {
    const DagorAssetFolder *ff = mgr.getFolderPtr(i);
    if (!ff)
      return -1;
    if (ff == f)
      return i;
  }
  return -1;
}


static void create_folder_idx_list(AllAssetsTree *tree, bool hierarchical, Tab<int> &fld_idx)
{
  PropPanel::TLeafHandle selItem = tree->getSelectedItem();

  Tab<PropPanel::TLeafHandle> sel(tmpmem);

  sel.push_back(selItem);
  ::make_hierarchical_list(tree, selItem, sel);

  int selCnt = sel.size();
  for (int i = 0; i < selCnt; i++) // O(N*M)
  {
    int idx = find_folder_idx(get_dagor_asset_folder(tree->getItemData(sel[i])));
    if (idx >= 0)
      fld_idx.push_back(idx);
  }
}

void AssetViewerApp::generate_impostors(const ImpostorOptions &options)
{
  if (!impostorApp)
    return;
  getConsole().showConsole();
  impostorApp->run(options);
  getConsole().hideConsole();
  ::post_base_update_notify_dabuild();
}

void AssetViewerApp::clear_impostors(const ImpostorOptions &options)
{
  if (!impostorApp)
    return;
  getConsole().showConsole();
  impostorApp->cleanUp(options);
  getConsole().hideConsole();
}

void AssetViewerApp::generate_point_cloud(DagorAsset *asset)
{
  if (!pointCloudGen)
    return;
  getConsole().showConsole();
  pointCloudGen->generate(asset);
  getConsole().hideConsole();
  ::post_base_update_notify_dabuild();
}

static void build_folder_for_platform(AllAssetsTree *tree, bool hierarchical, bool tex, bool res, unsigned platform)
{
  Tab<int> fldIdx;
  ::create_folder_idx_list(tree, hierarchical, fldIdx);

  ::rebuild_assets_in_folders(make_span_const(&platform, 1), fldIdx, tex, res);
}


static void build_folder_all_platform(AllAssetsTree *tree, bool hierarchical, bool tex, bool res)
{
  Tab<unsigned> p(get_app().getWorkspace().getAdditionalPlatforms());
  p.push_back(_MAKE4C('PC'));

  Tab<int> fldIdx;
  ::create_folder_idx_list(tree, hierarchical, fldIdx);

  ::rebuild_assets_in_folders(p, fldIdx, tex, res);
}

static void copy_asset_props_to_stream(DagorAsset *a, IGenSave &cwr, bool add_asset_closure)
{
  String tmp;
  if (add_asset_closure)
  {
    tmp.printf(0, "%s {\n", a->getName());
    cwr.write(tmp.str(), i_strlen(tmp));
  }

  if (a->isVirtual())
  {
    tmp.printf(0, "// %s", a->getTargetFilePath());
    if (a->getSrcFileName())
      tmp.aprintf(0, "\nname:t=\"%s\"", a->getSrcFileName());
  }
  else
    tmp.printf(0, "// %s", a->getSrcFilePath());
  tmp.append("\n");
  cwr.write(tmp.str(), i_strlen(tmp));
  a->props.saveToTextStream(cwr);
  if (add_asset_closure)
    cwr.write("}\n\n", 3);
}
static void copy_terse_lod_asset_props_to_stream(DagorAsset *a, IGenSave &cwr)
{
  String tmp(0, "%s { ", a->getName());
  cwr.write(tmp.str(), i_strlen(tmp));
  int lodIndex = 0;
  for (int i = 0, nid = a->props.getNameId("lod"); i < a->props.blockCount(); i++)
    if (a->props.getBlock(i)->getBlockNameId() == nid)
    {
      tmp.printf(0, "lod%d:r=%g; ", lodIndex, a->props.getBlock(i)->getReal("range", 0));
      cwr.write(tmp.str(), i_strlen(tmp));
      ++lodIndex;
    }
  cwr.write("}\n", 2);
}

static void showAssetDeps(DagorAsset &asset, CoolConsole &console)
{
  console.showConsole(true);

  IDagorAssetExporter *exporter = asset.getMgr().getAssetExporter(asset.getType());
  if (!exporter)
  {
    DAEDITOR3.conError("\nCan't get exporter plugin for %s (type=%s).", asset.getName(), asset.getTypeStr());
    return;
  }

  Tab<SimpleString> files;
  exporter->gatherSrcDataFiles(asset, files);
  DAEDITOR3.conNote("\nAsset <%s> depends on %d file(s):", asset.getNameTypified().c_str(), (int)files.size());
  for (const SimpleString &file : files)
    DAEDITOR3.conNote("  %s", file.c_str());
}

static void showAssetRefs(DagorAsset &asset, CoolConsole &console)
{
  console.showConsole(true);

  IDagorAssetRefProvider *refProvider = asset.getMgr().getAssetRefProvider(asset.getType());
  if (!refProvider)
  {
    DAEDITOR3.conError("\nAsset <%s> refs: no refs provider.", asset.getNameTypified().c_str());
    return;
  }

  dag::ConstSpan<IDagorAssetRefProvider::Ref> refs = refProvider->getAssetRefs(asset);
  DAEDITOR3.conNote("\nAsset <%s> references %d asset(s):", asset.getNameTypified().c_str(), (int)refs.size());
  for (const IDagorAssetRefProvider::Ref &ref : refs)
  {
    String refName;
    const DagorAsset *refAsset = ref.getAsset();
    if (refAsset)
      refName = refAsset->getNameTypified();
    else if (ref.flags & IDagorAssetRefProvider::RFLG_BROKEN)
      refName = ref.getBrokenRef();

    DAEDITOR3.conNote("  %c[%c%c] %s", ref.flags & IDagorAssetRefProvider::RFLG_BROKEN ? '-' : ' ',
      ref.flags & IDagorAssetRefProvider::RFLG_EXTERNAL ? 'X' : ' ', ref.flags & IDagorAssetRefProvider::RFLG_OPTIONAL ? 'o' : ' ',
      refName.c_str());
  }
}

static bool assetCompareFunction(const DagorAsset *a, const DagorAsset *b)
{
  const int result = stricmp(a->getNameTypified().c_str(), b->getNameTypified().c_str());
  if (result == 0)
    return a < b;
  return result < 0;
}

static void showAssetBackRefs(DagorAsset &asset, CoolConsole &console, DagorAssetMgr &assetMgr)
{
  struct AllAssetRefsElement
  {
    dag::Vector<IDagorAssetRefProvider::Ref> refs;
  };

  typedef eastl::hash_map<const DagorAsset *, AllAssetRefsElement> AllAssetRefsHashType;
  static AllAssetRefsHashType allAssetRefs; // Getting references for rendInsts is slow, so we cache the results.

  console.showConsole(true);

  if (allAssetRefs.empty())
  {
    wingw::set_busy(true);

    int assetsWithoutRefProvider = 0;

    dag::ConstSpan<DagorAsset *> assets = assetMgr.getAssets();
    for (DagorAsset *currentAsset : assets)
    {
      G_ASSERT(allAssetRefs.find(currentAsset) == allAssetRefs.end());

      IDagorAssetRefProvider *refProvider = currentAsset->getMgr().getAssetRefProvider(currentAsset->getType());
      if (!refProvider)
      {
        ++assetsWithoutRefProvider;
        continue;
      }

      AllAssetRefsElement assetRefs;
      assetRefs.refs = refProvider->getAssetRefs(*currentAsset);
      allAssetRefs.emplace(currentAsset, assetRefs);
    }

    if (assetsWithoutRefProvider > 0)
      DAEDITOR3.conError("\nFound %d assets without ref provider. The results won't be 100%% accurate.", assetsWithoutRefProvider);

    wingw::set_busy(false);
  }

  dag::Vector<const DagorAsset *> referencingAssets;

  for (AllAssetRefsHashType::const_iterator i = allAssetRefs.begin(); i != allAssetRefs.end(); ++i)
    for (const IDagorAssetRefProvider::Ref &ref : i->second.refs)
      if (ref.getAsset() == &asset)
        referencingAssets.emplace_back(i->first);

  eastl::sort(referencingAssets.begin(), referencingAssets.end(), assetCompareFunction);

  DAEDITOR3.conNote("\nAssets referencing <%s>:", asset.getNameTypified().c_str());
  for (const DagorAsset *currentAsset : referencingAssets)
    DAEDITOR3.conNote("  %s", currentAsset->getNameTypified().c_str());
}

static void createNewCompositeAsset(AllAssetsTree &tree)
{
  PropPanel::TLeafHandle selection = tree.getSelectedItem();
  const DagorAssetFolder *folder = get_dagor_asset_folder(tree.getItemData(selection));
  if (!folder)
    return;

  const String assetName = CompositeAssetCreator::pickName();
  if (assetName.empty())
    return;

  String assetFullPath(folder->folderPath);
  append_slash(assetFullPath);
  assetFullPath += assetName;
  assetFullPath += ".composit.blk";

  DagorAsset *newAsset = CompositeAssetCreator::create(assetName, assetFullPath);
  if (newAsset)
    get_app().selectAsset(*newAsset);
  else
    wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Error", "Failed to create new asset file '%s'.", assetFullPath.str());
}

int AssetViewerApp::onMenuItemClick(unsigned id)
{
  int ind = -1;

  // context menu
  if ((id >= CM_INC_BUILD_CUR_PACK) && (id < CM_INC_BUILD_CUR_PACK + CM_PLATFORM_COUNT))
  {
    ind = id - CM_INC_BUILD_CUR_PACK;
    id = CM_INC_BUILD_CUR_PACK;
  }
  else if ((id >= CM_INC_BUILD_RESOURCES) && (id < CM_INC_BUILD_RESOURCES + CM_PLATFORM_COUNT))
  {
    ind = id - CM_INC_BUILD_RESOURCES;
    id = CM_INC_BUILD_RESOURCES;
  }
  else if ((id >= CM_INC_BUILD_TEXTURES) && (id < CM_INC_BUILD_TEXTURES + CM_PLATFORM_COUNT))
  {
    ind = id - CM_INC_BUILD_TEXTURES;
    id = CM_INC_BUILD_TEXTURES;
  }
  else if ((id >= CM_INC_BUILD_RESOURCES_H) && (id < CM_INC_BUILD_RESOURCES_H + CM_PLATFORM_COUNT))
  {
    ind = id - CM_INC_BUILD_RESOURCES_H;
    id = CM_INC_BUILD_RESOURCES_H;
  }
  else if ((id >= CM_INC_BUILD_TEXTURES_H) && (id < CM_INC_BUILD_TEXTURES_H + CM_PLATFORM_COUNT))
  {
    ind = id - CM_INC_BUILD_TEXTURES_H;
    id = CM_INC_BUILD_TEXTURES_H;
  }
  else if ((id >= CM_INC_BUILD_ALL) && (id < CM_INC_BUILD_ALL + CM_PLATFORM_COUNT))
  {
    ind = id - CM_INC_BUILD_ALL;
    id = CM_INC_BUILD_ALL;
  }
  else if ((id >= CM_INC_BUILD_ALL_H) && (id < CM_INC_BUILD_ALL_H + CM_PLATFORM_COUNT))
  {
    ind = id - CM_INC_BUILD_ALL_H;
    id = CM_INC_BUILD_ALL_H;
  }

  // main menu
  if ((id >= CM_BUILD_RESOURCES) && ((id < CM_BUILD_RESOURCES + CM_PLATFORM_COUNT)))
  {
    ind = id - CM_BUILD_RESOURCES;
    id = CM_BUILD_RESOURCES;
  }
  else if ((id >= CM_BUILD_TEXTURES) && ((id < CM_BUILD_TEXTURES + CM_PLATFORM_COUNT)))
  {
    ind = id - CM_BUILD_TEXTURES;
    id = CM_BUILD_TEXTURES;
  }
  else if ((id >= CM_BUILD_ALL) && ((id < CM_BUILD_ALL + CM_PLATFORM_COUNT)))
  {
    ind = id - CM_BUILD_ALL;
    id = CM_BUILD_ALL;
  }
  else if ((id >= CM_BUILD_CLEAR_CACHE) && ((id < CM_BUILD_CLEAR_CACHE + CM_PLATFORM_COUNT)))
  {
    ind = id - CM_BUILD_CLEAR_CACHE;
    id = CM_BUILD_CLEAR_CACHE;
  }

  unsigned p = 0;
  if (ind > -1)
  {
    G_ASSERT((ind >= 0) && (getWorkspace().getAdditionalPlatforms().size() + 1 > ind));
    p = (ind == 0) ? _MAKE4C('PC') : getWorkspace().getAdditionalPlatforms()[ind - 1];
  }

  switch (id)
  {
      // file

    case CM_SAVE_CUR_ASSET:
      if (curAsset)
      {
        getConsole().addMessage(ILogWriter::NOTE, "Saving asset <%s>", curAsset->getNameTypified());
        if (IGenEditorPlugin *p = curPlugin())
          p->onSaveLibrary();
        assetMgr.saveAssetMetadata(*curAsset);
      }
      return 1;

    case CM_SAVE_ASSET_BASE:
      getConsole().addMessage(ILogWriter::NOTE, "Saving asset base");
      saveProject(sceneFname);
      return 1;

    case CM_CHECK_ASSET_BASE:
    {
      BadRefFinder refChecker(&assetMgr);
    }
      return 1;

    case CM_RELOAD_SHADERS: runShadersReload({}); return 1;

    case CM_UNDO: undoSystem->undo(); return 1;

    case CM_REDO:
      undoSystem->redo();
      return 1;

      // export

    case CM_BUILD_RESOURCES: ::rebuild_assets_in_root_single(p, false, true); return 1;

    case CM_BUILD_TEXTURES: ::rebuild_assets_in_root_single(p, true, false); return 1;

    case CM_BUILD_ALL: ::rebuild_assets_in_root_single(p, true, true); return 1;

    case CM_BUILD_CLEAR_CACHE: ::destroy_assets_cache_single(p); return 1;

    case CM_BUILD_ALL_PLATFORM_TEX:
    case CM_BUILD_ALL_PLATFORM_RES:
    case CM_BUILD_ALL_PLATFORM:
    {
      Tab<unsigned> targetCodes(getWorkspace().getAdditionalPlatforms());
      targetCodes.push_back(_MAKE4C('PC'));
      bool tex, res;
      if (id == CM_BUILD_ALL_PLATFORM)
        tex = res = true;
      else
      {
        tex = (id == CM_BUILD_ALL_PLATFORM_TEX);
        res = (id == CM_BUILD_ALL_PLATFORM_RES);
      }

      ::rebuild_assets_in_root(targetCodes, tex, res);
      return 1;
    }
    case CM_CLEAR_UNUSED_IMPOSTORS:
    {
      ImpostorOptions options;
      clear_impostors(options);
      return 1;
    }
    case CM_EXPORT_ALL_IMPOSTORS:
    {
      ImpostorOptions options;
      generate_impostors(options);
      return 1;
    }
    case CM_EXPORT_CURRENT_IMPOSTOR:
    {
      if (curAsset && impostorApp && impostorApp->getImpostorBaker()->isSupported(curAsset))
      {
        ImpostorOptions options;
        options.assetsToBuild.push_back(eastl::string{curAsset->getName()});
        generate_impostors(options);
      }
      return 1;
    }
    case CM_EXPORT_IMPOSTORS_CURRENT_PACK:
    {
      if (curAsset)
      {
        ImpostorOptions options;
        options.packsToBuild.push_back(eastl::string{curAsset->getDestPackName()});
        generate_impostors(options);
      }
      return 1;
    }
    case CM_EXPORT_CURRENT_POINT_CLOUD:
    {
      if (curAsset && pointCloudGen)
        generate_point_cloud(curAsset);
      return 1;
    }
    case CM_BUILD_CLEAR_CACHE_ALL:
    {
      Tab<unsigned> targetCodes(getWorkspace().getAdditionalPlatforms());
      targetCodes.push_back(_MAKE4C('PC'));
      ::destroy_assets_cache(targetCodes);
      return 1;
    }

      // tree context menu

    case CM_INC_BUILD_RESOURCES: ::build_folder_for_platform(&mTreeView->getAllAssetsTab().getTree(), false, false, true, p); return 1;

    case CM_INC_BUILD_TEXTURES: ::build_folder_for_platform(&mTreeView->getAllAssetsTab().getTree(), false, true, false, p); return 1;

    case CM_INC_BUILD_RESOURCES_H:
      ::build_folder_for_platform(&mTreeView->getAllAssetsTab().getTree(), true, false, true, p);
      return 1;

    case CM_INC_BUILD_TEXTURES_H: ::build_folder_for_platform(&mTreeView->getAllAssetsTab().getTree(), true, true, false, p); return 1;

    case CM_INC_BUILD_ALL: ::build_folder_for_platform(&mTreeView->getAllAssetsTab().getTree(), false, true, true, p); return 1;

    case CM_INC_BUILD_ALL_H: ::build_folder_for_platform(&mTreeView->getAllAssetsTab().getTree(), true, true, true, p); return 1;

    case CM_INC_BUILD_ALL_PLATFORM_TEX:
    case CM_INC_BUILD_ALL_PLATFORM_RES:
    case CM_INC_BUILD_ALL_PLATFORM:
    {
      bool tex, res;
      if (id == CM_INC_BUILD_ALL_PLATFORM)
        tex = res = true;
      else
      {
        tex = (id == CM_INC_BUILD_ALL_PLATFORM_TEX);
        res = (id == CM_INC_BUILD_ALL_PLATFORM_RES);
      }

      ::build_folder_all_platform(&mTreeView->getAllAssetsTab().getTree(), false, tex, res);
      return 1;
    }
    case CM_INC_BUILD_ALL_PLATFORM_TEX_H:
    case CM_INC_BUILD_ALL_PLATFORM_RES_H:
    case CM_INC_BUILD_ALL_PLATFORM_H:
    {
      bool tex, res;
      if (id == CM_INC_BUILD_ALL_PLATFORM_H)
        tex = res = true;
      else
      {
        tex = (id == CM_INC_BUILD_ALL_PLATFORM_TEX_H);
        res = (id == CM_INC_BUILD_ALL_PLATFORM_RES_H);
      }

      ::build_folder_all_platform(&mTreeView->getAllAssetsTab().getTree(), true, tex, res);
      return 1;
    }
    case CM_INC_BUILD_CUR_PACK:
    {
      Tab<unsigned> tc(tmpmem);
      tc.push_back(p);

      ::build_currently_assets(&mTreeView->getAllAssetsTab().getTree(), tc);
      return 1;
    }

    case CM_INC_BUILD_CUR_PACK_BQ_HQ_UHQ_PC:
      if (DagorAsset *asset = mTreeView->getAllAssetsTab().getSelectedAsset())
      {
        unsigned tc = _MAKE4C('PC');
        StaticTab<DagorAsset *, 3> a;
        a.push_back(asset);
        DagorAssetMgr &mgr = a[0]->getMgr();
        if (DagorAsset *a_hq = mgr.findAsset(String(0, "%s$hq", a[0]->getName()), a[0]->getType()))
          a.push_back(a_hq);
        if (DagorAsset *a_uhq = mgr.findAsset(String(0, "%s$uhq", a[0]->getName()), a[0]->getType()))
          a.push_back(a_uhq);
        ::build_assets(make_span_const(&tc, 1), a);
      }
      return 1;
    case CM_INC_BUILD_CUR_PACK_TQ_PC:
      if (DagorAsset *a = mTreeView->getAllAssetsTab().getSelectedAsset())
      {
        unsigned tc = _MAKE4C('PC');
        DagorAssetMgr &mgr = a->getMgr();
        if (DagorAsset *a_tq = mgr.findAsset(String(0, "%s$tq", a->getName()), a->getType()))
          ::build_assets(make_span_const(&tc, 1), make_span(&a_tq, 1));
      }
      return 1;


    case CM_INC_BUILD_ALL_PLATFORM_CUR_PACK:
      ::build_currently_assets(&mTreeView->getAllAssetsTab().getTree());
      return 1;

      // settings

    case CM_CAMERAS: show_camera_objects_config_dialog(0); return 1;

    case CM_SCREENSHOT: setScreenshotOptions(); return 1;

    case CM_OPTIONS_STAT_DISPLAY_SETTINGS:
    {
      ViewportWindow *viewport = ged.getCurrentViewport();
      if (viewport)
        viewport->showStatSettingsDialog();

      return 1;
    }

    case CM_ENVIRONMENT_SETTINGS: environment::show_environment_settings(0, &assetLtData); return 1;

    case CM_COLLISION_PREVIEW:
    {
      bool check = (getRendSubTypeMask() & collisionMask) != collisionMask;
      setMaskRend(collisionMask, check);

      getMainMenu()->setCheckById(id, check);
      mToolPanel->setBool(id, check);
      return 1;
    }

    case CM_RENDER_GEOMETRY:
    {
      bool check = (getRendSubTypeMask() & rendGeomMask) != rendGeomMask;
      setMaskRend(rendGeomMask, check);

      getMainMenu()->setCheckById(id, check);
      mToolPanel->setBool(id, check);
      return 1;
    }

    case CM_AUTO_ZOOM_N_CENTER:
    {
      autoZoomAndCenter = !autoZoomAndCenter;
      getMainMenu()->setCheckById(id, autoZoomAndCenter);
      mToolPanel->setBool(id, autoZoomAndCenter);
      if (autoZoomAndCenter)
        zoomAndCenter();
      return 1;
    }

    case CM_AUTO_SHOW_COMPOSITE_EDITOR:
    {
      compositeEditor.autoShow = !compositeEditor.autoShow;
      getMainMenu()->setCheckById(id, compositeEditor.autoShow);
      return 1;
    }

    case CM_DISCARD_ASSET_TEX:
      discardAssetTexMode = !discardAssetTexMode;
      getMainMenu()->setCheckById(id, discardAssetTexMode);
      mToolPanel->setBool(id, discardAssetTexMode);
      wingw::set_busy(true);
      applyDiscardAssetTexMode();
      wingw::set_busy(false);
      return 1;

      // window

    case CM_LOAD_DEFAULT_LAYOUT:
    {
      makeDefaultLayout();

      ViewportWindow *curVp = ged.getViewport(0);
      if (curVp)
      {
        curVp->activate();
        ged.setCurrentViewportId(0);
      }

      fillTree();
    }
      return 1;

    case CM_LOAD_LAYOUT:
    {
      String result(wingw::file_open_dlg(mManager->getMainWindow(), "Load layout from...", "Layout|*.blk", "blk"));
      if (!result.empty())
      {
        mManager->loadLayout(result.str());
        if (mTreeView)
          fillTree();
      }
    }
      return 1;

    case CM_SAVE_LAYOUT:
    {
      String result(wingw::file_save_dlg(mManager->getMainWindow(), "Save layout as...", "Layout|*.blk", "blk"));
      if (!result.empty())
      {
        mManager->saveLayout(result.str());
      }
    }
      return 1;


    case CM_WINDOW_TOOLBAR:
      if (!mToolPanel)
        createToolbar();
      return 1;

    case CM_WINDOW_TREE:
      if (!mTreeView)
      {
        createAssetsTree();
        fillTree();
      }
      return 1;

    case CM_WINDOW_PPANEL: showPropWindow(mPropPanel == nullptr); return 1;

    case CM_WINDOW_PPANEL_ACCELERATOR:
    {
      // The hotkey is simply 'P', so only handle it if the viewport is active.
      IGenViewportWnd *vp = getCurrentViewport();
      if (vp && vp->isActive())
      {
        showPropWindow(mPropPanel == nullptr);
        return 1;
      }
    }
    break;

    case CM_WINDOW_VIEWPORT:
      if (!ged.getViewportCount())
      {
        mManager->setWindowType(nullptr, VIEWPORT_TYPE);
      }
      return 1;

    case CM_WINDOW_COMPOSITE_EDITOR: showCompositeEditor(!isCompositeEditorShown()); return 1;

    case CM_WINDOW_COMPOSITE_EDITOR_ACCELERATOR:
    {
      // The hotkey is simply 'C', so only handle it if the viewport is active, and the current camera mode does not
      // need it to move the camera down.
      IGenViewportWnd *vp = getCurrentViewport();
      if (vp && vp->isActive() && !vp->isFlyMode())
      {
        showCompositeEditor(!isCompositeEditorShown());
        return 1;
      }
    }
    break;

    case CM_CAMERAS_FREE:
    case CM_CAMERAS_FPS:
    case CM_CAMERAS_TPS:
    {
      IGenViewportWnd *vp = IEditorCoreEngine::get()->getCurrentViewport();

      if (vp && vp->isActive())
      {
        vp->activate();
        vp->handleCommand(id);

        mToolPanel->setBool(CM_CAMERAS_FREE, CCameraElem::getCamera() == CCameraElem::FREE_CAMERA);
        mToolPanel->setBool(CM_CAMERAS_FPS, CCameraElem::getCamera() == CCameraElem::FPS_CAMERA);
        mToolPanel->setBool(CM_CAMERAS_TPS, CCameraElem::getCamera() == CCameraElem::TPS_CAMERA);

        // Refill the accelerators to limit them in fly mode.
        addAccelerators();

        return 1;
      }
    }
    break;

    case CM_DEBUG_FLUSH:
    {
      debug_flush(false);
      return 1;
    }

    case CM_OPTIONS_SET_ACT_RATE:
    {
      PropPanel::DialogWindow *dialog = new PropPanel::DialogWindow(NULL, _pxScaled(250), _pxScaled(100), "Set work cycle act rate");
      dialog->getPanel()->createEditInt(0, "Acts per second:", ::dagor_game_act_rate);

      if (dialog->showDialog() == PropPanel::DIALOG_ID_OK)
        ::dagor_set_game_act_rate(dialog->getPanel()->getInt(0));
      delete dialog;
    }
      return 1;

    case CM_ZOOM_AND_CENTER: zoomAndCenter(); return 1;

    case CM_COPY_ASSET_FILEPATH:
      if (curAsset)
      {
        String path(curAsset->isVirtual() ? curAsset->getTargetFilePath() : curAsset->getSrcFilePath());
        clipboard::set_clipboard_ansi_text(make_ms_slashes(path));
      }
      return 1;

    case CM_COPY_ASSET_NAME:
      if (curAsset)
      {
        clipboard::set_clipboard_ansi_text(curAsset->getName());
      }
      return 1;

    case CM_COPY_ASSET_PROPS_BLK:
      if (curAsset)
      {
        DynamicMemGeneralSaveCB cwr(tmpmem, 0, 4 << 10);
        copy_asset_props_to_stream(curAsset, cwr, false);
        cwr.write("\0", 1);
        clipboard::set_clipboard_ansi_text((const char *)cwr.data());
      }
      return 1;
    case CM_COPY_LOD_ASSET_PROPS_BLK:
      if (curAsset && curAsset->props.getBlockByName("lod"))
      {
        DynamicMemGeneralSaveCB cwr(tmpmem, 0, 4 << 10);
        copy_terse_lod_asset_props_to_stream(curAsset, cwr);
        cwr.write("\0", 1);
        clipboard::set_clipboard_ansi_text((const char *)cwr.data());
      }
      return 1;
    case CM_SHOW_ASSET_DEPS:
      if (curAsset)
        showAssetDeps(*curAsset, getConsole());
      return 1;
    case CM_SHOW_ASSET_REFS:
      if (curAsset)
        showAssetRefs(*curAsset, getConsole());
      return 1;
    case CM_SHOW_ASSET_BACK_REFS:
      if (curAsset)
        showAssetBackRefs(*curAsset, getConsole(), assetMgr);
      return 1;
    case CM_COPY_FOLDER_ASSETS_PROPS_BLK:
    case CM_COPY_FOLDER_LOD_ASSETS_PROPS_BLK:
      if (!curAsset)
      {
        if (const DagorAssetFolder *f = mTreeView->getAllAssetsTab().getSelectedAssetFolder())
        {
          for (int fidx = 0; assetMgr.getFolderPtr(fidx); fidx++)
            if (assetMgr.getFolderPtr(fidx) == f)
            {
              int out_start_idx = 0, out_end_idx = -1;
              assetMgr.getFolderAssetIdxRange(fidx, out_start_idx, out_end_idx);

              DynamicMemGeneralSaveCB cwr(tmpmem, 0, 4 << 10);
              String tmp(0, "// folder %s (%s)\n", f->folderName, f->folderPath);
              cwr.write(tmp, i_strlen(tmp));
              for (int j = out_start_idx; j < out_end_idx; j++)
              {
                DagorAsset &a = assetMgr.getAsset(j);
                if (id == CM_COPY_FOLDER_ASSETS_PROPS_BLK)
                  copy_asset_props_to_stream(&a, cwr, true);
                else if (id == CM_COPY_FOLDER_LOD_ASSETS_PROPS_BLK)
                {
                  if (a.props.getBlockByName("lod"))
                    copy_terse_lod_asset_props_to_stream(&a, cwr);
                }
              }
              cwr.write("\0", 1);
              clipboard::set_clipboard_ansi_text((const char *)cwr.data());
              break;
            }
        }
      }
      return 1;

    case CM_COPY_FOLDERPATH:
      if (curAsset)
      {
        String path(curAsset->getFolderPath());
        clipboard::set_clipboard_ansi_text(make_ms_slashes(path));
      }
      else
      {
        const DagorAssetFolder *f = mTreeView->getAllAssetsTab().getSelectedAssetFolder();
        if (f)
        {
          String path(f->folderPath);
          clipboard::set_clipboard_ansi_text(make_ms_slashes(path));
        }
      }
      return 1;

    case CM_REVEAL_IN_EXPLORER:
      if (curAsset)
      {
        dag_reveal_in_explorer(curAsset);
      }
      else
      {
        const DagorAssetFolder *f = mTreeView->getAllAssetsTab().getSelectedAssetFolder();
        if (f)
          dag_reveal_in_explorer(f);
      }
      return 1;

    case CM_CREATE_NEW_COMPOSITE_ASSET:
      if (mTreeView)
        createNewCompositeAsset(mTreeView->getAllAssetsTab().getTree());
      return 1;

    case CM_ADD_ASSET_TO_FAVORITES:
      if (const DagorAsset *asset = mTreeView->getAllAssetsTab().getSelectedAsset())
        mTreeView->addAssetToFavorites(*asset);
      return 1;

    case CM_CREATE_SCREENSHOT: createScreenshot(); return 1;

    case CM_CREATE_CUBE_SCREENSHOT: createCubeScreenshot(); return 1;

    case CM_PREV_ASSET: mTreeView->selectNextItemInActiveTree(false); return 1;

    case CM_NEXT_ASSET: mTreeView->selectNextItemInActiveTree(true); return 1;

    case CM_EXPAND_CHILDREN:
    case CM_COLLAPSE_CHILDREN:
    {
      AllAssetsTree &tree = mTreeView->getAllAssetsTab().getTree();
      const PropPanel::TLeafHandle selectedItem = tree.getSelectedItem();
      if (tree.isFolder(selectedItem))
      {
        const bool expand = id == CM_EXPAND_CHILDREN;
        const int childCount = tree.getChildrenCount(selectedItem);
        for (int i = 0; i < childCount; ++i)
          tree.expandRecursive(tree.getChild(selectedItem, i), expand);
      }

      return 1;
    }

    default: break;
  }

  return GenericEditorAppWindow::onMenuItemClick(id);
}


void AssetViewerApp::onAssetSelectionChanged(DagorAsset *asset, DagorAssetFolder *asset_folder)
{
  DagorAsset *prev = curAsset;
  if (prev && getTypeSupporter(prev) && getTypeSupporter(prev)->supportEditing())
  {
    if (prev->isMetadataChanged())
      changedAssetsList.addPtr(prev);
    else
      changedAssetsList.delPtr(prev);
  }

  curAsset = asset;
  fillPropPanelHasBeenCalled = false;

  wingw::set_busy(true);
  if (discardAssetTexMode && curAsset != prev)
    applyDiscardAssetTexMode();
  if (curAsset)
  {
    if (curAsset != prev)
    {
      curAssetPackName = ::get_asset_pack_name(curAsset);
      curAssetPkgName = get_asset_pkg_name(curAsset);
      IGenEditorPlugin *sup = getTypeSupporter(curAsset);
      scriptChangeFlag = false;

      if (sup)
        switchToPlugin(getPluginIdx(sup));
      else
        switchToPlugin(-1);
    }
  }
  else
  {
    scriptChangeFlag = false;
    curAssetPackName = NULL;
    curAssetPkgName.clear();
    switchToPlugin(-1);
  }

  if (!fillPropPanelHasBeenCalled)
    fillPropPanel();
  repaint();
  wingw::set_busy(false);
}


void AssetViewerApp::onAvSelectAsset(DagorAsset *asset, const char *asset_name) { onAssetSelectionChanged(asset, nullptr); }


void AssetViewerApp::onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name)
{
  onAssetSelectionChanged(nullptr, asset_folder);
}


static void create_aditional_platform_popup_menu(PropPanel::IMenu &menu, unsigned pid, int menu_inc_idx, bool exported)
{
  if (exported)
  {
    menu.addItem(pid, CM_INC_BUILD_RESOURCES + menu_inc_idx, "Export gameRes");
    menu.addItem(pid, CM_INC_BUILD_TEXTURES + menu_inc_idx, "Export texPack");
    menu.addItem(pid, CM_INC_BUILD_ALL + menu_inc_idx, "Export All");
    menu.addSeparator(pid);
  }

  menu.addItem(pid, CM_INC_BUILD_RESOURCES_H + menu_inc_idx, "Export gameRes with sub folders");
  menu.addItem(pid, CM_INC_BUILD_TEXTURES_H + menu_inc_idx, "Export texPack with sub folders");
  menu.addItem(pid, CM_INC_BUILD_ALL_H + menu_inc_idx, "Export All with sub folders");
}


static inline void create_popup_build_menu(PropPanel::IMenu &menu, bool exported)
{
  if (exported)
  {
    menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_RESOURCES, "Export gameRes [PC]");
    menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_TEXTURES, "Export texPack [PC]");
    menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_ALL, "Export All [PC]");
    menu.addSeparator(ROOT_MENU_ITEM);
  }

  menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_RESOURCES_H, "Export gameRes with sub folders [PC]");
  menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_TEXTURES_H, "Export texPack with sub folders [PC]");
  menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_ALL_H, "Export All with sub folders [PC]");

  int platformCnt = get_app().getWorkspace().getAdditionalPlatforms().size();

  if (!platformCnt)
    return;

  menu.addSeparator(ROOT_MENU_ITEM);

  for (int i = 0; i < platformCnt; i++)
  {
    String group(256, "%c%c%c%c", _DUMP4C(get_app().getWorkspace().getAdditionalPlatforms()[i]));

    menu.addSubMenu(ROOT_MENU_ITEM, CM_INC_BUILD_GROUP_ADDITIONAL + (i + 1), group);

    ::create_aditional_platform_popup_menu(menu, CM_INC_BUILD_GROUP_ADDITIONAL + (i + 1), i + 1, exported);
  }

  menu.addSeparator(ROOT_MENU_ITEM);

  if (exported)
  {
    menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_ALL_PLATFORM_TEX, "Export texPack for All platform");
    menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_ALL_PLATFORM_RES, "Export gameRes for All platform");
    menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_ALL_PLATFORM, "Export All for All platform");

    menu.addSeparator(ROOT_MENU_ITEM);
  }

  menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_ALL_PLATFORM_TEX_H, "Export texPack for All platform with sub folders");
  menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_ALL_PLATFORM_RES_H, "Export gameRes for All platform with sub folders");
  menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_ALL_PLATFORM_H, "Export All for All platform with sub folders");
}


static inline void create_popup_build_assets_menu(PropPanel::IMenu &menu, DagorAsset *e)
{
  const char *pack_name = ::get_asset_pack_name(e);
  menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_CUR_PACK, String(256, "Export pack '%s' [PC]", pack_name));
  if (e && e->getType() == e->getMgr().getTexAssetTypeId() && !strchr(e->getName(), '$'))
  {
    DagorAsset *e_hq = e->getMgr().findAsset(String(0, "%s$hq", e->getName()), e->getType());
    DagorAsset *e_uhq = e->getMgr().findAsset(String(0, "%s$uhq", e->getName()), e->getType());
    if (e_hq || e_uhq)
    {
      String pack_names(0, "'%s'", pack_name);
      if (e_hq)
        pack_names.aprintf(0, ", '%s'", ::get_asset_pack_name(e_hq));
      if (e_uhq)
        pack_names.aprintf(0, ", '%s'", ::get_asset_pack_name(e_uhq));
      menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_CUR_PACK_BQ_HQ_UHQ_PC, String(256, "Export HQ/UHQ packs %s [PC]", pack_names));
    }

    if (DagorAsset *e_tq = e->getMgr().findAsset(String(0, "%s$tq", e->getName()), e->getType()))
      menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_CUR_PACK_TQ_PC, String(256, "Export TQ pack %s [PC]", ::get_asset_pack_name(e_tq)));
  }


  int platformCnt = get_app().getWorkspace().getAdditionalPlatforms().size();

  if (!platformCnt)
    return;

  menu.addSeparator(ROOT_MENU_ITEM);

  String menuStr(256, "Export pack '%s'", pack_name);
  for (int i = 0; i < platformCnt; i++)
  {
    String group(256, "%c%c%c%c", _DUMP4C(get_app().getWorkspace().getAdditionalPlatforms()[i]));

    menu.addSubMenu(ROOT_MENU_ITEM, CM_INC_BUILD_GROUP_PACK + (i + 1), group);
    menu.addItem(CM_INC_BUILD_GROUP_PACK + (i + 1), CM_INC_BUILD_CUR_PACK + (i + 1), menuStr);
  }

  menu.addSeparator(ROOT_MENU_ITEM);

  menu.addItem(ROOT_MENU_ITEM, CM_INC_BUILD_ALL_PLATFORM_CUR_PACK, "Export pack for All platform");
}


bool AssetViewerApp::onAssetSelectorContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree)
{
  void *data = tree_base_window.getItemData(tree_base_window.getSelectedItem());
  const DagorAssetFolder *g = get_dagor_asset_folder(data);
  if (g == assetMgr.getRootFolder())
    return false;

  PropPanel::IMenu &menu = tree.createContextMenu();
  menu.setEventHandler(this);

  if (!g)
  {
    DagorAsset *e = (DagorAsset *)data;

    menu.addItem(ROOT_MENU_ITEM, CM_ADD_ASSET_TO_FAVORITES, "Add to favorites");
    menu.addSeparator(ROOT_MENU_ITEM);

    // curAssetPackName
    const int oldMenuItemCount = menu.getItemCount(ROOT_MENU_ITEM);
    if (::is_asset_exportable(e))
      ::create_popup_build_assets_menu(menu, e);

    if (menu.getItemCount(ROOT_MENU_ITEM) != oldMenuItemCount)
      menu.addSeparator(ROOT_MENU_ITEM);

    menu.addItem(ROOT_MENU_ITEM, CM_COPY_ASSET_FILEPATH, "Copy filepath");
    menu.addItem(ROOT_MENU_ITEM, CM_COPY_ASSET_PROPS_BLK, "Copy asset BLK data");
    menu.addItem(ROOT_MENU_ITEM, CM_COPY_FOLDERPATH, "Copy folder path");
    menu.addItem(ROOT_MENU_ITEM, CM_COPY_ASSET_NAME, "Copy name");
    menu.addItem(ROOT_MENU_ITEM, CM_COPY_LOD_ASSET_PROPS_BLK, "Copy terse LOD asset summary");
    menu.addItem(ROOT_MENU_ITEM, CM_SHOW_ASSET_DEPS, "Show asset dependencies in console");
    menu.addItem(ROOT_MENU_ITEM, CM_SHOW_ASSET_REFS, "Show asset references in console");
    menu.addItem(ROOT_MENU_ITEM, CM_SHOW_ASSET_BACK_REFS, "Show references to asset in console");
    menu.addItem(ROOT_MENU_ITEM, CM_REVEAL_IN_EXPLORER, "Reveal in Explorer");
  }
  else
  {
    menu.addItem(ROOT_MENU_ITEM, CM_EXPAND_CHILDREN, "Expand children");
    menu.addItem(ROOT_MENU_ITEM, CM_COLLAPSE_CHILDREN, "Collapse children");
    menu.addSeparator(ROOT_MENU_ITEM);

    bool exported = false;
    if (mTreeView->getAllAssetsTab().isSelectedFolderExportable(&exported))
    {
      ::create_popup_build_menu(menu, exported);
      menu.addSeparator(ROOT_MENU_ITEM);
    }

    menu.addItem(ROOT_MENU_ITEM, CM_COPY_FOLDERPATH, "Copy folder path");
    menu.addItem(ROOT_MENU_ITEM, CM_COPY_FOLDER_ASSETS_PROPS_BLK, "Copy assets BLK data (whole folder)");
    menu.addItem(ROOT_MENU_ITEM, CM_COPY_FOLDER_LOD_ASSETS_PROPS_BLK, "Copy terse LOD assets summary (whole folder)");
    menu.addItem(ROOT_MENU_ITEM, CM_REVEAL_IN_EXPLORER, "Reveal in Explorer");
    menu.addSeparator(ROOT_MENU_ITEM);
    menu.addItem(ROOT_MENU_ITEM, CM_CREATE_NEW_COMPOSITE_ASSET, "Create new composit asset");
  }

  return true;
}

static int sort_texid_by_refc(const TEXTUREID *a, const TEXTUREID *b)
{
  if (int rc_diff = get_managed_texture_refcount(*a) - get_managed_texture_refcount(*b))
    return -rc_diff;
  return strcmp(get_managed_texture_name(*a), get_managed_texture_name(*b));
}

bool AssetViewerApp::onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params)
{
  if (!stricmp(cmd, "shaders.list"))
    return runShadersListVars(params);
  if (!stricmp(cmd, "shaders.set"))
    return runShadersSetVar(params);
  if (!stricmp(cmd, "shaders.reload"))
    return runShadersReload(params);
  if (!stricmp(cmd, "perf.on"))
  {
    queryEditorInterface<IDynRenderService>()->enableFrameProfiler(true);
    return true;
  }
  if (!stricmp(cmd, "perf.off"))
  {
    queryEditorInterface<IDynRenderService>()->enableFrameProfiler(false);
    return true;
  }
  if (!stricmp(cmd, "perf.dump"))
  {
    queryEditorInterface<IDynRenderService>()->profilerDumpFrame();
    repaint();
    return true;
  }
  if (!stricmp(cmd, "tex.info"))
  {
    if (params.size() == 0 || (params.size() == 1 && strcmp(params[0], "full") == 0))
    {
      uint32_t num_textures = 0;
      uint64_t total_mem = 0;
      String texStat;
      d3d::get_texture_statistics(&num_textures, &total_mem, &texStat);
      char *str = texStat.str();
      if (params.size() == 1)
      {
        while (char *p = strchr(str, '\n'))
        {
          *p = 0;
          DAEDITOR3.conNote(str);
          str = p + 1;
        }
        DAEDITOR3.conNote(str);
      }
      else
        DAEDITOR3.conNote("%d tex use %dM of GPU memory", num_textures, total_mem >> 20);
      return true;
    }
    return false;
  }
  if (!stricmp(cmd, "tex.refs"))
  {
    Tab<TEXTUREID> ids;
    ids.reserve(32 << 10);
    for (TEXTUREID i = first_managed_texture(1); i != BAD_TEXTUREID; i = next_managed_texture(i, 1))
      ids.push_back(i);
    sort(ids, &sort_texid_by_refc);

    for (TEXTUREID texId : ids)
      DAEDITOR3.conNote("  [%5d] refc=%-3d %s", texId, get_managed_texture_refcount(texId), get_managed_texture_name(texId));
    DAEDITOR3.conNote("total %d referenced textures", ids.size());
  }
  if (!stricmp(cmd, "tex.show"))
  {
    av_show_tex_helper.demandInit();

    Tab<const char *> argv;
    argv.push_back("tex.show");
    append_items(argv, params.size(), params.data());
    String str = av_show_tex_helper->processConsoleCmd(argv.data(), argv.size());
    if (!str.empty())
      DAEDITOR3.conNote(str);
    return true;
  }
  if (!stricmp(cmd, "tex.hide"))
  {
    if (av_show_tex_helper.get())
      av_show_tex_helper->hideTex();
    return true;
  }

  if (!stricmp(cmd, "camera.pos"))
  {
    IGenViewportWnd *vpw = getCurrentViewport();
    if (!vpw)
      return false;

    if (!params.size())
    {
      TMatrix tm;
      vpw->getCameraTransform(tm);

      console->addMessage(ILogWriter::NOTE, "Current camera position: %g, %g, %g", P3D(tm.getcol(3)));

      return true;
    }
    else if (params.size() >= 3)
    {
      vpw->setCameraPos(strToPoint3(params, 0));
      return true;
    }
    return false;
  }

  if (!stricmp(cmd, "camera.dir"))
  {
    IGenViewportWnd *vpw = getCurrentViewport();
    if (!vpw)
      return false;

    if (params.size() >= 3)
    {
      Point3 p3 = strToPoint3(params, 0);
      Point3 up = params.size() >= 6 ? strToPoint3(params, 3) : Point3(0, 1, 0);

      vpw->setCameraDirection(::normalize(p3), ::normalize(up));
      return true;
    }
    return false;
  }
  if (!stricmp(cmd, "driver.reset"))
  {
    dagor_d3d_force_driver_reset = true;
    return true;
  }
  return false;
}

const char *AssetViewerApp::onConsoleCommandHelp(const char *cmd)
{
  if (!stricmp(cmd, "shaders.list"))
    return "shaders.list [name_substr]";
  if (!stricmp(cmd, "shaders.set"))
    return "shaders.set <shader-var-name> <value>";
  if (!stricmp(cmd, "shaders.reload"))
    return "shaders.reload [shaders-binary-dump-fname]";
  if (!stricmp(cmd, "tex.info"))
    return "tex.info [full]";
  if (!stricmp(cmd, "tex.refs"))
    return "tex.refs";
  if (!stricmp(cmd, "driver.reset"))
    return "driver.reset";
  if (!stricmp(cmd, "tex.show"))
  {
    static String str;
    av_show_tex_helper.demandInit();
    str = av_show_tex_helper->processConsoleCmd(NULL, 0);
    if (!str.empty())
      return str;
  }
  if (!stricmp(cmd, "tex.hide"))
    return "tex.hide";

  return NULL;
}

bool AssetViewerApp::runShadersListVars(dag::ConstSpan<const char *> params)
{
  for (int i = 0, ie = VariableMap::getGlobalVariablesCount(); i < ie; i++)
  {
    const char *name = VariableMap::getGlobalVariableName(i);
    if (!name)
      continue;
    if (params.size() && !strstr(String(name).toLower(), String(params[0]).toLower()))
      continue;

    int varId = VariableMap::getVariableId(name);
    int type = ShaderGlobal::get_var_type(varId);
    if (type < 0)
      continue;

    Color4 c;
    TEXTUREID tid;
    switch (type)
    {
      case SHVT_INT: console->addMessage(ILogWriter::NOTE, "[int]   %-40s %d", name, ShaderGlobal::get_int_fast(varId)); break;
      case SHVT_REAL: console->addMessage(ILogWriter::NOTE, "[real]  %-40s %.3f", name, ShaderGlobal::get_real_fast(varId)); break;
      case SHVT_COLOR4:
        c = ShaderGlobal::get_color4_fast(varId);
        console->addMessage(ILogWriter::NOTE, "[color] %-40s %.3f,%.3f,%.3f,%.3f", name, c.r, c.g, c.b, c.a);
        break;
      case SHVT_TEXTURE:
        tid = ShaderGlobal::get_tex_fast(varId);
        console->addMessage(ILogWriter::NOTE, "[tex]   %-40s %s", name, tid == BAD_TEXTUREID ? "---" : get_managed_texture_name(tid));
        break;
    }
  }
  return true;
}
bool AssetViewerApp::runShadersSetVar(dag::ConstSpan<const char *> params)
{
  if (params.size() < 1)
  {
    console->addMessage(ILogWriter::ERROR, "shaders.set requires <name> and <value>");
    return false;
  }
  int varId = VariableMap::getVariableId(params[0]);
  int type = ShaderGlobal::get_var_type(varId);
  if (type < 0)
  {
    console->addMessage(ILogWriter::ERROR, "<%s> is not global shader var", params[0]);
    return false;
  }
  if (params.size() > 1)
  {
    switch (type)
    {
      case SHVT_INT: ShaderGlobal::set_int(varId, atoi(params[1])); break;
      case SHVT_REAL: ShaderGlobal::set_real(varId, atof(params[1])); break;
      case SHVT_COLOR4:
        if (params.size() < 5)
        {
          console->addMessage(ILogWriter::ERROR, "shaders.set requires <name> <R> <G> <B> <A> for color var");
          return false;
        }
        ShaderGlobal::set_color4(varId, atof(params[1]), atof(params[2]), atof(params[3]), atof(params[4]));
        break;
      case SHVT_TEXTURE:
        TEXTUREID tex_id = get_managed_texture_id(params[1]);
        if (tex_id == BAD_TEXTUREID && !strchr(params[1], '*') && dd_file_exist(params[1]))
          tex_id = add_managed_texture(params[1]);
        if (tex_id != BAD_TEXTUREID)
          ShaderGlobal::set_texture(varId, tex_id);
        else
          console->addMessage(ILogWriter::ERROR, "<%s> is not valid texture", params[1]);
        break;
    }
  }
  switch (type)
  {
    case SHVT_INT:
      console->addMessage(ILogWriter::NOTE, "globalvar <%s> [int] set to %d", params[0], ShaderGlobal::get_int_fast(varId));
      break;
    case SHVT_REAL:
      console->addMessage(ILogWriter::NOTE, "globalvar <%s> [real] set to %.3f", params[0], ShaderGlobal::get_real_fast(varId));
      break;
    case SHVT_COLOR4:
    {
      Color4 val = ShaderGlobal::get_color4_fast(varId);
      console->addMessage(ILogWriter::NOTE, "globalvar <%s> [color] set to %.3f,%.3f,%.3f,%.3f", params[0], val.r, val.g, val.b,
        val.a);
    }
    break;
    case SHVT_TEXTURE:
    {
      TEXTUREID tex_id = ShaderGlobal::get_tex_fast(varId);
      console->addMessage(ILogWriter::NOTE, "globalvar <%s> [tex] set to %s [0x%x]", params[0], get_managed_texture_name(tex_id),
        tex_id);
    }
    break;
  }
  return true;
}
bool AssetViewerApp::runShadersReload(dag::ConstSpan<const char *> params)
{
  G_ASSERT(d3d::is_inited());
  const Driver3dDesc &dsc = d3d::get_driver_desc();

  DataBlock appblk(::get_app().getWorkspace().getAppPath());
  String sh_file;
  if (appblk.getStr("shaders", NULL))
    sh_file.printf(260, "%s/%s", ::get_app().getWorkspace().getAppDir(), appblk.getStr("shaders", NULL));
  else
    sh_file.printf(260, "%s/../commonData/compiledShaders/classic/tools", sgg::get_exe_path_full());
  simplify_fname(sh_file);

  const char *shname = params.size() > 0 ? params[0] : sh_file.c_str();

  String fileName;
  for (auto version : d3d::smAll)
  {
    if (dsc.shaderModel < version)
      continue;

    fileName.printf(260, "%s.%s.shdump.bin", shname, version.psName);

    if (!dd_file_exist(fileName))
      continue;

    if (load_shaders_bindump(shname, version))
    {
      console->addMessage(ILogWriter::NOTE, "reloaded %s, ver=%s", shname, version.psName);
      return true;
    }
    console->addMessage(ILogWriter::FATAL, "failed to reload %s, ver=%s", shname, version.psName);
    return false;
  }
  console->addMessage(ILogWriter::FATAL, "failed to reload %s, dsc.shaderModel=%s", shname, d3d::as_string(dsc.shaderModel));
  return false;
}

void AssetViewerApp::onImguiDelayedCallback(void *user_data)
{
  const unsigned highestCommandBit = 1 << ((sizeof(unsigned) * 8) - 1);
  const unsigned commandId = (unsigned)(uintptr_t)user_data;

  if ((commandId & highestCommandBit) == 0)
  {
    onMenuItemClick(commandId);
  }
  else
  {
    ViewportWindow *viewport = ged.getCurrentViewport();
    if (!viewport || !viewport->isActive())
      return;

    const unsigned viewportCommandId = commandId & ~highestCommandBit;
    if (viewport->handleViewportAcceleratorCommand(viewportCommandId))
      return;

    IGenEditorPlugin *currentPlugin = curPlugin();
    if (currentPlugin)
      currentPlugin->handleViewportAcceleratorCommand(*viewport, viewportCommandId);
  }
}

void AssetViewerApp::renderUIViewports(bool vr_mode)
{
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("Viewport", nullptr, vr_mode ? ImGuiWindowFlags_NoBackground : ImGuiWindowFlags_None);
  ImGui::PopStyleVar();

  const float itemSpacing = ImGui::GetStyle().ItemSpacing.y; // Use the same spacing in both directions.
  const Point2 regionAvailable = Point2(ImGui::GetContentRegionAvail()) - Point2(itemSpacing, itemSpacing);

  if (ged.getViewportCount() == 1)
  {
    ViewportWindow *viewport0 = ged.getViewport(0);
    if (viewport0)
      viewport0->updateImgui(regionAvailable, itemSpacing, vr_mode);
  }
  else if (ged.getViewportCount() == 4)
  {
    ViewportWindow *viewport0 = ged.getViewport(0);
    ViewportWindow *viewport1 = ged.getViewport(1);
    ViewportWindow *viewport2 = ged.getViewport(2);
    ViewportWindow *viewport3 = ged.getViewport(3);
    if (viewport0 && viewport1 && viewport2 && viewport3)
    {
      int currentLeftWidth;
      int currentRightWidth;
      int currentTopHeight;
      int currentBottomHeight;
      viewport0->getViewportSize(currentLeftWidth, currentTopHeight);
      viewport3->getViewportSize(currentRightWidth, currentBottomHeight);

      render_imgui_viewport_splitter(viewportSplitRatio, regionAvailable, currentLeftWidth, currentRightWidth, currentTopHeight,
        currentBottomHeight, itemSpacing);

      const float leftWidth = regionAvailable.x * viewportSplitRatio.x;
      const float rightWidth = regionAvailable.x - leftWidth;
      const float topHeight = regionAvailable.y * viewportSplitRatio.y;
      const float bottomHeight = regionAvailable.y - topHeight;

      viewport0->updateImgui(Point2(leftWidth, topHeight), itemSpacing, vr_mode);
      ImGui::SameLine(0.0f, itemSpacing);
      viewport1->updateImgui(Point2(rightWidth, topHeight), itemSpacing, vr_mode);

      viewport2->updateImgui(Point2(leftWidth, bottomHeight), itemSpacing, vr_mode);
      ImGui::SameLine(0.0f, itemSpacing);
      viewport3->updateImgui(Point2(rightWidth, bottomHeight), itemSpacing, vr_mode);
    }
  }

  ImGui::End();
}

void AssetViewerApp::renderUI()
{
  unsigned clientWidth = 0;
  unsigned clientHeight = 0;
  getWndManager()->getWindowClientSize(getWndManager()->getMainWindow(), clientWidth, clientHeight);

  // They can be zero when toggling the window between minimized and maximized state.
  if (clientWidth == 0 || clientHeight == 0)
    return;

  PropPanel::IMenu *mainMenu = getMainMenu();
  if (mainMenu)
    PropPanel::render_menu(*mainMenu);

  // If VR mode is active then we do not draw backgrounds and viewports, so the viewport area will be transparent.
  const bool vrMode = VRDevice::getInstance() != nullptr;

  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  const ImGuiWindowFlags rootWindowFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration |
                                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav |
                                           (vrMode ? ImGuiWindowFlags_NoBackground : ImGuiWindowFlags_None);
  ImGui::Begin("Root", nullptr, rootWindowFlags);
  ImGui::PopStyleVar(3);

  if (mToolPanel || mPluginTool)
  {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_PopupBg));
    ImGui::PushStyleColor(ImGuiCol_Border, PropPanel::Constants::TOOLBAR_BORDER_COLOR);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, PropPanel::Constants::TOOLBAR_WINDOW_PADDING);

    ImGui::BeginChild("Toolbar", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border,
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    if (mToolPanel)
      mToolPanel->updateImgui();

    if (mPluginTool)
    {
      if (mToolPanel)
      {
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, 1.0f);
      }

      mPluginTool->updateImgui();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
    ImGui::EndChild();
    ImGui::PopStyleVar();
  }

  ImGui::BeginChild("DockHolder", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking);

  ImGuiID rootDockSpaceId = ImGui::GetID("RootDockSpace");

  if (!dockPositionsInitialized)
  {
    dockPositionsInitialized = true;

    const ImVec2 viewportSize = ImGui::GetMainViewport()->Size;

    ImGui::DockBuilderRemoveNode(rootDockSpaceId);
    ImGui::DockBuilderAddNode(rootDockSpaceId, ImGuiDockNodeFlags_CentralNode);
    ImGui::DockBuilderSetNodeSize(rootDockSpaceId, ImGui::GetMainViewport()->Size);

    ImGuiID dockSpaceViewport;
    ImGuiID dockSpaceRightBottom = ImGui::DockBuilderSplitNode(rootDockSpaceId, ImGuiDir_Right, 0.15f, nullptr, &dockSpaceViewport);
    ImGuiID dockSpaceRightTop = ImGui::DockBuilderSplitNode(dockSpaceRightBottom, ImGuiDir_Up, 0.5f, nullptr, &dockSpaceRightBottom);
    ImGuiID dockSpaceLeft1 = ImGui::DockBuilderSplitNode(dockSpaceViewport, ImGuiDir_Left, 0.25f, nullptr, &dockSpaceViewport);
    ImGuiID dockSpaceLeft2 = ImGui::DockBuilderSplitNode(dockSpaceViewport, ImGuiDir_Left, 0.25f, nullptr, &dockSpaceViewport);

    ImGuiDockNode *viewportNode = ImGui::DockBuilderGetNode(dockSpaceViewport);
    if (viewportNode)
      viewportNode->SetLocalFlags(viewportNode->LocalFlags | ImGuiDockNodeFlags_HiddenTabBar);

    // TODO: ImGui porting: view: AV: extensibility?
    ImGui::DockBuilderDockWindow("Assets Tree", dockSpaceLeft1);
    ImGui::DockBuilderDockWindow("Actions Tree", dockSpaceLeft2);
    ImGui::DockBuilderDockWindow("Effects Tools", dockSpaceLeft2);
    ImGui::DockBuilderDockWindow("Viewport", dockSpaceViewport);
    ImGui::DockBuilderDockWindow("Properties", dockSpaceRightBottom);
    ImGui::DockBuilderDockWindow("Composit Outliner", dockSpaceRightTop);

    ImGui::DockBuilderFinish(rootDockSpaceId);
  }

  // Change the close button's color for the daEditorX Classic style.
  ImGui::PushStyleColor(ImGuiCol_Text, PropPanel::Constants::DIALOG_TITLE_COLOR);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
  if (vrMode)
    ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
  ImGui::DockSpace(rootDockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
  ImGui::PopStyleColor(vrMode ? 3 : 2);

  renderUIViewports(vrMode);

  if (mTreeView)
  {
    DAEDITOR3.imguiBegin("Assets Tree");
    mTreeView->updateImgui();
    DAEDITOR3.imguiEnd();
  }

  if (mPropPanel)
  {
    bool open = true;
    DAEDITOR3.imguiBegin(*mPropPanel, &open);
    mPropPanel->updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open)
      showPropWindow(false);
  }

  if (compositeEditor.compositeTreeView && compositeEditor.compositePropPanel)
  {
    bool open = true;
    DAEDITOR3.imguiBegin("Composit Outliner", &open);

    if (ImGui::BeginChild("tree", ImVec2(0.0f, 250.0f), ImGuiChildFlags_ResizeY, ImGuiWindowFlags_NoBackground))
      compositeEditor.compositeTreeView->updateImgui();
    ImGui::EndChild();

    compositeEditor.compositePropPanel->updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open)
      showCompositeEditor(false);
  }

  IGenEditorPlugin *currentPlugin = curPlugin();
  if (currentPlugin)
    currentPlugin->updateImgui();

  g_entity_mgr->broadcastEventImmediate(ImGuiStage());

  PropPanel::render_dialogs();

  ImGui::EndChild();

  ImGui::End();
}

void AssetViewerApp::beforeUpdateImgui() { mManager->updateImguiMouseCursor(); }

void AssetViewerApp::updateImgui()
{
  static bool renderingImGui = false;

  // Most likely cause if this assert fails: a modal dialog is being shown in response to an event handled in an
  // updateImgui. See the notes at the PropPanel::MessageQueue class.
  G_ASSERT(!renderingImGui);
  renderingImGui = true;

  PropPanel::after_new_frame();

  // The keyboard press checking itself cannot be in actObject because actObject could be called more often than
  // renderUI (that calls ImGui's NewFrame that updates ImGui's key states), and onMenuItemClick could fire more than
  // once.

  const unsigned highestCommandBit = 1 << ((sizeof(unsigned) * 8) - 1);
  ViewportWindow *viewport = ged.getCurrentViewport();
  unsigned commandId = 0;
  if (viewport && viewport->isActive())
  {
    commandId = mManager->processImguiViewportAccelerator();
    if (commandId != 0)
    {
      G_ASSERT((commandId & highestCommandBit) == 0);
      commandId |= highestCommandBit;
    }
  }

  if (commandId == 0)
  {
    commandId = mManager->processImguiAccelerator();
    G_ASSERT((commandId & highestCommandBit) == 0);
  }

  if (commandId != 0)
    PropPanel::request_delayed_callback(*this, (void *)commandId);

  if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_F12))
    imguiDebugWindowsVisible = !imguiDebugWindowsVisible;

  if (imguiDebugWindowsVisible)
    ImGui::ShowMetricsWindow(&imguiDebugWindowsVisible);

  editor_core_update_imgui_style_editor();

  renderUI();

  PropPanel::before_end_frame();

  renderingImGui = false;
}

const char *daeditor3_get_appblk_fname()
{
  static String fn;
  fn = ::get_app().getWorkspace().getAppPath();
  return fn;
}
