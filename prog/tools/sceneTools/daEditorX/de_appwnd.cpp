// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "de_appwnd.h"
#include "de_apphandler.h"
#include "plugin_dlg.h"
#include "de_plugindata.h"
#include "de_ProjectSettings.h"
#include "de_aboutdlg.h"
#include "de_startdlg.h"
#include "de_batch.h"
#include "de_screenshotMetaInfoLoader.h"
#include "de_viewportWindow.h"
#include "controlGallery.h"

#include <de3_interface.h>
#include <de3_lightService.h>
#include <de3_hmapService.h>
#include <de3_huid.h>
#include <de3_editorEvents.h>
#include "pluginService/de_IDagorPhysImpl.h"

#include <oldEditor/iCollision.h>


#include <oldEditor/de_cm.h>
#include <oldEditor/de_util.h>
#include <oldEditor/de_common_interface.h>
#include <oldEditor/de_workspace.h>
#include <oldEditor/de_collision.h>
#include <de3_interface.h>
#include <de3_dynRenderService.h>

#include <propPanel/commonWindow/dialogManager.h>
#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>
#include <propPanel/c_util.h>
#include <propPanel/colors.h>
#include <propPanel/constants.h>
#include <propPanel/propPanel.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

#include <perfMon/dag_cpuFreq.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_startupModules.h>
#include <workCycle/dag_workCycle.h>
#include <gui/dag_baseCursor.h>
#include <gui/dag_stdGuiRenderEx.h>

#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_gizmofilter.h>
#include <EditorCore/ec_selwindow.h>
#include <EditorCore/ec_newProjDlg.h>
#include <EditorCore/ec_camera_dlg.h>
#include <EditorCore/ec_status_bar.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <EditorCore/ec_IEditorCore.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_input.h>
#include <EditorCore/ec_startup.h>
#include <EditorCore/ec_viewportSplitter.h>
#include <EditorCore/ec_wndGlobal.h>

#include <libTools/staticGeom/geomObject.h>
#include <libTools/renderViewports/cachedViewports.h>
#include <libTools/util/makeBindump.h>
#include <scene/dag_occlusionMap.h>
#include <sceneBuilder/nsb_StdTonemapper.h>
#include <sceneBuilder/nsb_LightmappedScene.h>

#include <generic/dag_sort.h>
#include <libTools/util/undo.h>
#include <libTools/util/fileUtils.h>
#include <regExp/regExp.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_miscApi.h>

#include <consoleWindow/dag_consoleWindow.h>
#include <assets/texAssetBuilderTextureFactory.h>
#include <assetsGui/av_globalState.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>

#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <render/texDebug.h>
#include <render/daFrameGraph/debug.h>
#include <render/dynmodelRenderer.h>

#include <de3_entityFilter.h>

#include <eventLog/eventLog.h>
#include <eventLog/errorLog.h>
#include <util/dag_delayedAction.h>

#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#if _TARGET_PC_WIN
#include <windows.h>
#include <htmlhelp.h>

#ifdef ERROR
#undef ERROR
#endif
#endif

using hdpi::_pxActual;
using hdpi::_pxScaled;
using PropPanel::ROOT_MENU_ITEM;

#define WSP_FILE_PATH (::make_full_path(sgg::get_exe_path_full(), "../.local/workspace.blk"))

#define LEVEL_FILE_EXTENSION_WO_DOT "level.blk"
#define LEVEL_FILE_EXTENSION        "." LEVEL_FILE_EXTENSION_WO_DOT

static constexpr unsigned DELAYED_CALLBACK_VIEWPORT_COMMAND_BIT = 1 << ((sizeof(unsigned) * 8) - 1);

enum
{
  CM_NAV_COMPASS = CM_PLUGIN_BASE - 10,
  CM_DISCARD_TEX_MODE,
};

extern void init3d_early();
extern void init3d();

namespace environment
{
void show_environment_settings(void *phandle);
void save_settings(DataBlock &blk);
void load_settings(DataBlock &blk);
void save_ui_state(DataBlock &per_app_settings);
void load_ui_state(const DataBlock &per_app_settings);
}; // namespace environment

static bool blockCloseMessage = false;
static bool (*prev_fatal_handler)(const char *msg, const char *call_stack, const char *file, int line) = NULL;

// services implementations
static IEditorCore &editorCoreImpl = IEditorCore::make_instance();
static DeDagorPhys deDagorPhysImpl;
IEditorCoreEngine *IEditorCoreEngine::__global_instance = NULL;


// DLL plugin typedefs
typedef int(__fastcall *GetPluginVersionFunc)(void);
typedef IGenEditorPlugin *(__fastcall *RegisterPluginFunc)(IDagorEd2Engine &, const char *);
typedef void(__fastcall *ReleasePluginFunc)();


extern void *get_generic_render_helper_service();
extern void *get_generic_dyn_render_service();
extern void *get_tiff_bit_mask_image_mgr();
extern void *get_generic_hmap_service();
extern void *get_generic_asset_service();
extern void *get_gen_light_service();
extern void *get_generic_file_change_tracking_service();
extern void *get_generic_fmod_service();
extern void *get_generic_skies_service();
extern void *get_generic_color_range_service();
extern void *get_generic_rendinstgen_service();
extern void *get_generic_grass_service();
extern void *get_gpu_grass_service();
extern void *get_generic_water_service();
extern void *get_generic_water_proj_fx_service();
extern void *get_generic_cable_service();
extern void *get_generic_spline_gen_service();
extern void *get_generic_wind_service();
extern void *get_pixel_perfect_selection_service();
extern void *get_visibility_finder_service();

extern void release_generic_hmap_service();
extern void release_generic_water_proj_fx_service();
extern void release_generic_water_service();
extern void release_generic_cable_service();

extern FastNameMap cmdline_include_dll_re, cmdline_exclude_dll_re;

IDagorEd2Engine *IDagorEd2Engine::__dagored_global_instance = NULL;

IDagorRender *editorcore_extapi::dagRender = nullptr;
IDagorGeom *editorcore_extapi::dagGeom = nullptr;
IDagorConsole *editorcore_extapi::dagConsole = nullptr;
IDagorInput *editorcore_extapi::dagInput = nullptr;
IDagorTools *editorcore_extapi::dagTools = nullptr;
IDagorScene *editorcore_extapi::dagScene = nullptr;

static eastl::unique_ptr<ControlGallery> control_gallery;
static bool discardTexMode = false;

struct PluginMap
{
  Plugin *plugin;
  int idx;
  PluginMap(Plugin *plugin_, int idx_) : plugin(plugin_), idx(idx_) {}
};


static int plugs_compare(PluginMap const *s1, PluginMap const *s2)
{
  return stricmp(s1->plugin->p->getMenuCommandName(), s2->plugin->p->getMenuCommandName());
}

static void force_screen_redraw()
{
  d3d::GpuAutoLock gpuLock;
  dagor_work_cycle_flush_pending_frame();
  dagor_draw_scene_and_gui(true, true);
  d3d::update_screen();
}

enum
{
  VIEWPORT_HEIGHT = 400,
  TABBAR_HEIGHT = 20,
  TOOLBAR_HEIGHT = 26,
};


enum
{
  TOOLBAR_TYPE,
  TABBAR_TYPE,
  PLUGTOOLS_TYPE,
  VIEWPORT_TYPE,
};


static void enable_event_log()
{
  event_log::EventLogInitParams eventInitParams;
  eventInitParams.host = "client-logs.warthunder.com";
  eventInitParams.use_https = true;
  eventInitParams.http_port = 0;
  eventInitParams.udp_port = 20020;
  eventInitParams.user_agent = "daEditor";
  eventInitParams.origin = "client";
  eventInitParams.circuit = "dev";
  eventInitParams.project = "daeditor";

  eventInitParams.version = "3.0.0";

  if (event_log::init(eventInitParams))
    debug("Event log initialized as %s with '%s', udp %d, tcp %d", eventInitParams.origin, eventInitParams.host,
      eventInitParams.udp_port, eventInitParams.http_port);
  else
    logerr("Could not initialize remote event log.");

  event_log::ErrorLogInitParams eparams;
  eparams.collection = "events";
  eparams.game = "daeditor";

  event_log::init_error_log(eparams);
}


void send_event_error(const char *s, const char *callstack)
{
  String buf(0, "%s\n\n%s", s, callstack);
  event_log::ErrorLogSendParams params;
  params.attach_game_log = true;
  params.collection = "assert";
  params.dump_call_stack = false;
  event_log::send_error_log(buf.str(), params);
}


static void set_colliders_to_default_state()
{
  restore_editor_colliders();
  enable_all_custom_shadows();

  const int colliderCount = get_custom_colliders_count();
  for (int i = 0; i < colliderCount; ++i)
  {
    const IDagorEdCustomCollider *collider = get_custom_collider(i);
    if (collider && !is_custom_collider_enabled(collider))
      enable_custom_collider(collider->getColliderName());
  }
}


//==============================================================================
DagorEdAppWindow::DagorEdAppWindow(IWndManager *manager, const char *open_fname) :
  GenericEditorAppWindow(open_fname, manager),
  plugin(midmem),
  panelsToAdd(midmem),
  panelsToAddWidth(midmem),
  panelsToDelete(midmem),
  panelsSkipManage(false),
  curPluginId(-1),
  lastUniqueId(1),
  collision(NULL),
  mNeedSuppress(false),
  mSuppressDlgResult(""),
  mPluginShowDialogIsShow(false),
  toDagPlugNames(midmem),
  wsp(NULL),
  useDirectLight(true),
  animateViewports(false),
  shouldUseOccluders(true),
  activeColliders(midmem),
  backupPluginColliders(tmpmem),
  exportPaths(midmem),
  mToolPanel(NULL),
  mTabWindow(NULL),
  mTabPanel(NULL),
  mPlugTools(NULL),
  waterService(NULL),
  noWaterService(false)
{
  enable_event_log();
  IDagorEd2Engine::set(this);
  IEditorCoreEngine::set(this);

  editorcore_extapi::dagRender = editorCoreImpl.getRender();
  G_ASSERT(editorcore_extapi::dagRender);
  editorcore_extapi::dagGeom = editorCoreImpl.getGeom();
  G_ASSERT(editorcore_extapi::dagGeom);
  editorcore_extapi::dagConsole = editorCoreImpl.getConsole();
  G_ASSERT(editorcore_extapi::dagConsole);
  editorcore_extapi::dagInput = editorCoreImpl.getInput();
  G_ASSERT(editorcore_extapi::dagInput);
  editorcore_extapi::dagTools = editorCoreImpl.getTools();
  G_ASSERT(editorcore_extapi::dagTools);
  editorcore_extapi::dagScene = editorCoreImpl.getScene();
  G_ASSERT(editorcore_extapi::dagScene);

  // ViewportWindow::showDagorUiCursor = false;

  appEH = new (inimem) DagorEdAppEventHandler(*this);

  wsp = new (midmem) DeWorkspace;

  DataBlock editorBlk(::make_full_path(sgg::get_exe_path_full(), "../.local/de3_settings.blk"));
  const DataBlock *toDagPlug = editorBlk.getBlockByName("pluginsToDAG");

  if (toDagPlug)
    for (int i = 0; i < toDagPlug->paramCount(); ++i)
      toDagPlugNames.push_back() = toDagPlug->getStr(i);

  registerConsoleCommands();

  // clearProc.init("isl_clear_shmap");

  mManager->registerWindowHandler(this);

  aboutDlg = new AboutDlg(mManager->getMainWindow());

  loadThemeSettings(editorBlk);
}


//==============================================================================
DagorEdAppWindow::~DagorEdAppWindow()
{
  texconvcache::build_on_demand_tex_factory_cease_loading(false);
  mManager->unregisterWindowHandler(this);

  clearProc.clear();

  // delete_visclipmesh();

  DataBlock editorBlk;

  DataBlock *toDagPlugBlk = editorBlk.addNewBlock("pluginsToDAG");
  DataBlock *workspaceBlk = editorBlk.addNewBlock("workspace");

  if (workspaceBlk && wsp)
    workspaceBlk->setStr("currentName", wsp->getName());

  if (toDagPlugBlk)
    for (int i = 0; i < toDagPlugNames.size(); ++i)
      toDagPlugBlk->addStr("plug_name", toDagPlugNames[i]);

  DataBlock &globalPluginsRootSettings = *editorBlk.addNewBlock("plugins");
  DataBlock perAppSettingsBlk;
  DataBlock &perAppPluginsRootSettings = *perAppSettingsBlk.addNewBlock("plugins");
  for (int i = 0; i < getPluginCount(); ++i)
    if (IGenEditorPlugin *plugin = getPlugin(i))
    {
      DataBlock globalPluginSettings;
      DataBlock perAppPluginSettings;
      plugin->saveSettings(globalPluginSettings, perAppPluginSettings);
      if (!globalPluginSettings.isEmpty())
        globalPluginsRootSettings.addNewBlock(&globalPluginSettings, plugin->getInternalName());
      if (!perAppPluginSettings.isEmpty())
        perAppPluginsRootSettings.addNewBlock(&perAppPluginSettings, plugin->getInternalName());
    }

  saveThemeSettings(editorBlk);
  saveScreenshotSettings(editorBlk);

  editorBlk.setBool("DeveloperToolsEnabled", developerToolsEnabled);
  GizmoSettings::save(editorBlk);

  String pathName = ::make_full_path(sgg::get_exe_path_full(), "../.local/de3_settings.blk");
  editorBlk.saveToTextFile(pathName);

  editor_core_save_dag_imgui_blk_settings(perAppSettingsBlk);
  environment::save_ui_state(perAppSettingsBlk);

  // There might be no path if exiting from the workspace selector without creating any workspaces.
  const String perAppSettingsPath = getPerApplicationSettingsBlkPath();
  if (!perAppSettingsPath.empty())
    perAppSettingsBlk.saveToTextFile(perAppSettingsPath);

  wsp->save();

  // switchToPlugin(-1);

  editor_core_save_imgui_settings(dockPositionsInitialized ? LATEST_DOCK_SETTINGS_VERSION : 0);
  PropPanel::remove_delayed_callback(*this);
  PropPanel::release();

  del_it(appEH);
  terminateInterface();
  release_generic_cable_service();
  release_generic_hmap_service();
  release_generic_water_proj_fx_service();
  release_generic_water_service();
  dynrend::close();
  console::shutdown();
  imgui_shutdown();
  StdGuiRender::close_fonts();
  StdGuiRender::close_render();

  del_it(aboutDlg);
  del_it(wsp);
}


void DagorEdAppWindow::init(const char *)
{
  PropPanel::p2util::set_main_parent_handle(mManager->getMainWindow());

  String imgPath(512, "%s%s", sgg::get_exe_path_full(), "../commonData/icons/");
  ::dd_simplify_fname_c(imgPath.str());
  mManager->initCustomMouseCursors(imgPath);

#if _TARGET_PC_LINUX // The Windows version uses its own message loop and the input is handled there.
  dagor_init_keyboard_win();
  dagor_init_mouse_win();
  editor_core_initialize_input_handler();
#endif

  init3d_early();
  editor_core_initialize_imgui(nullptr); // The path is not set to avoid saving the startup screen's settings.
  editor_core_load_imgui_theme(getThemeFileName());
  GenericEditorAppWindow::init();
  makeDefaultLayout();

  IGenViewportWnd *curVP = ged.getViewport(0);
  if (curVP)
  {
    curVP->activate();
    ged.setCurrentViewportId(0);
  }

  // Accelerators
  addEditorAccelerators();
}


void DagorEdAppWindow::addEditorAccelerators()
{
  mManager->clearAccelerators();

  addCameraAccelerators();

  mManager->addAccelerator(CM_FILE_SAVE, ImGuiMod_Ctrl | ImGuiKey_S);

  mManager->addAccelerator(CM_UNDO, ImGuiMod_Ctrl | ImGuiKey_Z);
  mManager->addAccelerator(CM_REDO, ImGuiMod_Ctrl | ImGuiKey_Y);
  mManager->addAccelerator(CM_SELECT_ALL, ImGuiMod_Ctrl | ImGuiKey_A);
  mManager->addAccelerator(CM_DESELECT_ALL, ImGuiMod_Ctrl | ImGuiKey_D);

  mManager->addAccelerator(CM_OPTIONS_TOTAL, ImGuiKey_F11);
  mManager->addAccelerator(CM_CUR_PLUGIN_VIS_CHANGE, ImGuiKey_F12);

  mManager->addAccelerator(CM_CREATE_SCREENSHOT, ImGuiMod_Ctrl | ImGuiKey_P);
  mManager->addAccelerator(CM_CREATE_CUBE_SCREENSHOT, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P);

  mManager->addAccelerator(CM_VIEW_GRID_MOVE_SNAP, ImGuiKey_S);
  mManager->addAccelerator(CM_VIEW_GRID_ANGLE_SNAP, ImGuiKey_A);
  mManager->addAccelerator(CM_VIEW_GRID_SCALE_SNAP, ImGuiMod_Shift | ImGuiKey_5);

  mManager->addAccelerator(CM_SWITCH_PLUGIN_F5, ImGuiKey_F5);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_F6, ImGuiKey_F6);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_F7, ImGuiKey_F7);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_F8, ImGuiKey_F8);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF1, ImGuiMod_Shift | ImGuiKey_F1);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF2, ImGuiMod_Shift | ImGuiKey_F2);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF3, ImGuiMod_Shift | ImGuiKey_F3);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF4, ImGuiMod_Shift | ImGuiKey_F4);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF5, ImGuiMod_Shift | ImGuiKey_F5);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF6, ImGuiMod_Shift | ImGuiKey_F6);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF7, ImGuiMod_Shift | ImGuiKey_F7);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF8, ImGuiMod_Shift | ImGuiKey_F8);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF9, ImGuiMod_Shift | ImGuiKey_F9);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF10, ImGuiMod_Shift | ImGuiKey_F10);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF11, ImGuiMod_Shift | ImGuiKey_F11);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF12, ImGuiMod_Shift | ImGuiKey_F12);

  mManager->addAccelerator(CM_NEXT_PLUGIN, ImGuiMod_Ctrl | ImGuiKey_Tab);
  mManager->addAccelerator(CM_PREV_PLUGIN, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Tab);

  mManager->addAccelerator(CM_TAB_PRESS, ImGuiKey_Tab);
  mManager->addAcceleratorUp(CM_TAB_RELEASE, ImGuiKey_Tab);

  mManager->addAccelerator(CM_CHANGE_VIEWPORT, ImGuiMod_Ctrl | ImGuiKey_W);
  mManager->addAccelerator(CM_ZOOM_AND_CENTER, ImGuiKey_Z);                   // For normal mode.
  mManager->addAccelerator(CM_CONSOLE, ImGuiMod_Ctrl | ImGuiKey_GraveAccent); // Ctrl+`

  if (auto *p = curPlugin())
    p->registerMenuAccelerators();

  ViewportWindow *viewport = ged.getCurrentViewport();
  if (viewport)
    viewport->registerViewportAccelerators(*mManager);
}


void DagorEdAppWindow::addCameraAccelerators()
{
  mManager->clearAccelerators();

  mManager->addAccelerator(CM_CAMERAS_FREE, ImGuiKey_Space);
  mManager->addAccelerator(CM_CAMERAS_FPS, ImGuiMod_Ctrl | ImGuiKey_Space);
  mManager->addAccelerator(CM_CAMERAS_TPS, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Space);
  // mManager->addAccelerator(CM_CAMERAS_CAR, ImGuiMod_Shift | ImGuiKey_Space);

  mManager->addAccelerator(CM_ZOOM_AND_CENTER, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z); // For fly mode.

  mManager->addAccelerator(CM_DEBUG_FLUSH, ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_F);
  mManager->addAccelerator(CM_DEBUG_FLUSH, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_F);
}


void DagorEdAppWindow::makeDefaultLayout()
{
  dockPositionsInitialized = false;
  resetModelessWindows(ModelessWindowResetReason::ResettingLayout);
  mManager->reset();

  mManager->setWindowType(nullptr, VIEWPORT_TYPE);
  mManager->setWindowType(nullptr, PLUGTOOLS_TYPE);
  mManager->setWindowType(nullptr, TABBAR_TYPE);
  mManager->setWindowType(nullptr, TOOLBAR_TYPE);
}


void *DagorEdAppWindow::onWmCreateWindow(int type)
{
  switch (type)
  {
    case TOOLBAR_TYPE:
    {
      if (mToolPanel)
        return nullptr;

      getMainMenu()->setEnabledById(CM_WINDOW_TOOLBAR, false);

      mToolPanel = new PropPanel::ContainerPropertyControl(0, this, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0));

      fillMainToolBar();
      updateThemeItems();

      return mToolPanel;
    }
    break;

    case TABBAR_TYPE:
    {
      if (mTabWindow)
        return nullptr;

      getMainMenu()->setEnabledById(CM_WINDOW_TABBAR, false);

      mTabWindow = new PropPanel::ContainerPropertyControl(0, this, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0));

      mTabPanel = mTabWindow->createTabPanel(TAB_PANEL_ID, "");
      mTabPanel->moveTo(2, 0);
      mTabWindow->setWidthById(TAB_PANEL_ID, _pxScaled(mTabWindow->getWidth() - 4));

      fillPluginTabs();

      return mTabWindow;
    }
    break;

    case PLUGTOOLS_TYPE:
    {
      if (mPlugTools)
        return nullptr;

      getMainMenu()->setEnabledById(CM_WINDOW_PLUGTOOLS, false);

      mPlugTools = new PropPanel::ContainerPropertyControl(0, this, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0));

      return mPlugTools;
    }
    break;

    case VIEWPORT_TYPE:
    {
      getMainMenu()->setEnabledById(CM_WINDOW_VIEWPORT, false);

      DagorEdViewportWindow *v = new DagorEdViewportWindow();
      ged.addViewport(nullptr, appEH, mManager, v);
      return v;
    }
    break;
  }

  return nullptr;
}


bool DagorEdAppWindow::onWmDestroyWindow(void *window)
{
  if (window == mToolPanel)
  {
    del_it(mToolPanel);
    getMainMenu()->setEnabledById(CM_WINDOW_TOOLBAR, true);
    return true;
  }

  if (window == mTabWindow)
  {
    del_it(mTabWindow);
    getMainMenu()->setEnabledById(CM_WINDOW_TABBAR, true);
    return true;
  }

  if (window == mPlugTools)
  {
    del_it(mPlugTools);
    getMainMenu()->setEnabledById(CM_WINDOW_PLUGTOOLS, true);
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


void DagorEdAppWindow::fillMainToolBar()
{
  ged.tbManager->init(GUI_MAIN_TOOLBAR_ID);

  PropPanel::ContainerPropertyControl *tb2 = EDITORCORE->getCustomPanel(GUI_MAIN_TOOLBAR_ID)->createToolbarPanel(0, "");
  tb2->createSeparator();
  tb2->createCheckBox(CM_DISCARD_TEX_MODE, "Discard textures (show stub tex)");
  tb2->setButtonPictures(CM_DISCARD_TEX_MODE, "discard_tex");
  tb2->setBool(CM_DISCARD_TEX_MODE, discardTexMode);
  tb2->createSeparator();
  tb2->createCheckBox(CM_NAV_COMPASS, "Show compass");
  tb2->setButtonPictures(CM_NAV_COMPASS, "compass");
  tb2->setBool(CM_NAV_COMPASS, static_cast<DagorEdAppEventHandler *>(appEH)->showCompass);

  tb2->setAlignRightFromChild(tb2->getChildCount());
  tb2->createButton(CM_THEME_TOGGLE, "Toggle theme");
}


void DagorEdAppWindow::fillMenu(PropPanel::IMenu *menu)
{
  menu->clearMenu(ROOT_MENU_ITEM);

  menu->addSubMenu(ROOT_MENU_ITEM, CM_FILE, "Project");
  menu->addItem(CM_FILE, CM_FILE_SAVE, "Save project\tCtrl+S");
  menu->addItem(CM_FILE, CM_FILE_SAVE_AS, "Save as...");
  menu->addSeparator(CM_FILE);

  // currently creating projects is not supported
  //  menu->addItem(CM_FILE, CM_FILE_NEW, "Create new...");
  menu->addItem(CM_FILE, CM_FILE_OPEN, "Open project...");
  menu->addSubMenu(CM_FILE, CM_FILE_RECENT_LAST, "Open recent");
  menu->addItem(CM_FILE, CM_FILE_REOPEN, "Reopen current project");
  menu->addSeparator(CM_FILE);

  menu->addItem(CM_FILE, CM_FILE_EXPORT_TO_GAME_PC, "Export to game (PC format)");

  // Edit
  menu->addSubMenu(ROOT_MENU_ITEM, CM_EDIT, "Edit");
  menu->addItem(CM_EDIT, CM_UNDO, "Undo\tCtrl+Z");
  menu->addItem(CM_EDIT, CM_REDO, "Redo\tCtrl+Y");
  updateUndoRedoMenu();

  menu->addSeparator(CM_EDIT);

  menu->addItem(CM_EDIT, CM_SELECT_ALL, "Select all\tCtrl+A");
  menu->addItem(CM_EDIT, CM_DESELECT_ALL, "Deselect all\tCtrl+D");

  // View

  menu->addSubMenu(ROOT_MENU_ITEM, CM_VIEW, "View");

  menu->addItem(CM_VIEW, CM_VIEWPORT_LAYOUT_4, "4 quarters");
  menu->addItem(CM_VIEW, CM_VIEWPORT_LAYOUT_1, "1 viewport");

  menu->addSeparator(CM_VIEW);

  menu->addItem(CM_VIEW, CM_LOAD_DEFAULT_LAYOUT, "Load default layout");
  menu->addSeparator(CM_VIEW);
  menu->addItem(CM_VIEW, CM_LOAD_LAYOUT, "Load layout ...");
  menu->addItem(CM_VIEW, CM_SAVE_LAYOUT, "Save layout ...");
  menu->addSeparator(CM_VIEW);

  menu->addItem(CM_VIEW, CM_OPTIONS_TOTAL, "Plugins visibility...\tF11");
  menu->addItem(CM_VIEW, CM_OPTIONS_GRID, "Edit grid options...");

  menu->addSeparator(CM_VIEW);

  menu->addItem(CM_VIEW, CM_ANIMATE_VIEWPORTS, "Animate viewports");
  // viewMenu.checkMenuItem(CM_ANIMATE_VIEWPORTS, deAppWnd->animateViewports);

  menu->addItem(CM_VIEW, CM_USE_OCCLUDERS, "Use occluders");
  // viewMenu.checkMenuItem(CM_USE_OCCLUDERS, deAppWnd->shouldUseOccluders);
  // menu->setCheckById(CM_USE_OCCLUDERS, shouldUseOccluders);
  menu->addItem(CM_VIEW, CM_NAV_COMPASS, "Show compass");
  menu->addItem(CM_VIEW, CM_DISCARD_TEX_MODE, "Discard textures (show stub tex)");

  menu->addSeparator(CM_VIEW);
  menu->addItem(CM_VIEW, CM_WINDOW_TOOLBAR, "Main toolbar");
  menu->addItem(CM_VIEW, CM_WINDOW_TABBAR, "Plugin tabs");
  menu->addItem(CM_VIEW, CM_WINDOW_PLUGTOOLS, "Plugin toolbar");
  menu->addItem(CM_VIEW, CM_WINDOW_VIEWPORT, "Viewport");
  menu->addSeparator(CM_VIEW);
  menu->addSubMenu(CM_VIEW, CM_VIEW_DEVELOPER_TOOLS, "Developer tools");
  menu->addItem(CM_VIEW_DEVELOPER_TOOLS, CM_VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES, "Console commands and variables");
  menu->addItem(CM_VIEW_DEVELOPER_TOOLS, CM_VIEW_DEVELOPER_TOOLS_CONTROL_GALLERY, "Control gallery");
  menu->addItem(CM_VIEW_DEVELOPER_TOOLS, CM_VIEW_DEVELOPER_TOOLS_IMGUI_DEBUGGER, "ImGui debugger");
  menu->addItem(CM_VIEW_DEVELOPER_TOOLS, CM_VIEW_DEVELOPER_TOOLS_TEXTURE_DEBUG, "Texture debug");
  menu->addItem(CM_VIEW_DEVELOPER_TOOLS, CM_VIEW_DEVELOPER_TOOLS_NODE_DEPS, "Node dependencies");

  menu->setEnabledById(CM_WINDOW_TOOLBAR, false);
  menu->setEnabledById(CM_WINDOW_TABBAR, false);
  menu->setEnabledById(CM_WINDOW_PLUGTOOLS, false);
  menu->setEnabledById(CM_WINDOW_VIEWPORT, false);

  menu->addSeparator(CM_VIEW);
  menu->addSubMenu(CM_VIEW, CM_THEME, "Theme");
  menu->addItem(CM_THEME, CM_THEME_LIGHT, "Light");
  menu->addItem(CM_THEME, CM_THEME_DARK, "Dark");


  // Modes
  menu->addSubMenu(ROOT_MENU_ITEM, CM_PLUGINS_MENU, "Modes");

  // Plugin
  menu->addSubMenu(ROOT_MENU_ITEM, CM_PLUGIN_PRIVATE_MENU, "Plugin");

  // Tools
  menu->addSubMenu(ROOT_MENU_ITEM, CM_TOOLS, "Tools");

  menu->addItem(CM_TOOLS, CM_BATCH_RUN, "Run batch file...");

  menu->addSeparator(CM_TOOLS);
  menu->addItem(CM_TOOLS, CM_FILE_EXPORT_TO_DAG, "Export scene geometry to DAG...");
  menu->addItem(CM_TOOLS, CM_FILE_EXPORT_COLLISION_TO_DAG, "Export scene collision geometry to DAG...");
  menu->addSeparator(CM_TOOLS);

  menu->addItem(CM_TOOLS, CM_CREATE_SCREENSHOT, "Create screenshot\tCtrl+P");
  menu->addItem(CM_TOOLS, CM_CREATE_CUBE_SCREENSHOT, "Create cube screenshot\tCtrl+Shift+P");
  menu->addItem(CM_TOOLS, CM_CREATE_ORTHOGONAL_SCREENSHOT, "Create orthogonal screenshot");
  menu->addItem(CM_TOOLS, CM_UPSCALE_HEIGHTMAP, "[SLOW] Upscale heightmap & rebake");

  // Settings
  menu->addSubMenu(ROOT_MENU_ITEM, CM_OPTIONS, "Settings");

  menu->addItem(CM_OPTIONS, CM_OPTIONS_CAMERAS, "Cameras...");
  menu->addItem(CM_OPTIONS, CM_OPTIONS_SCREENSHOT, "Screenshot...");
  menu->addItem(CM_OPTIONS, CM_OPTIONS_STAT_DISPLAY_SETTINGS, "Stats settings...");

  menu->addSeparator(CM_OPTIONS);
  menu->addItem(CM_OPTIONS, CM_ENVIRONMENT_SETTINGS, "Environment settings...");

  menu->addSeparator(CM_OPTIONS);
  menu->addItem(CM_OPTIONS, CM_FILE_SETTINGS, "Project settings...");

  menu->addSeparator(CM_OPTIONS);
  menu->addItem(CM_OPTIONS, CM_OPTIONS_ENABLE_DEVELOPER_TOOLS, "Enable developer tools");

  menu->addSeparator(CM_OPTIONS);
  menu->addSubMenu(CM_OPTIONS, CM_OPTIONS_PREFERENCES, "Preferences");
  menu->addItem(CM_OPTIONS_PREFERENCES, CM_OPTIONS_PREFERENCES_ASSET_TREE, "Asset Tree");
  menu->setEnabledById(CM_OPTIONS_PREFERENCES_ASSET_TREE, false);
  menu->addItem(CM_OPTIONS_PREFERENCES, CM_OPTIONS_PREFERENCES_ASSET_TREE_COPY_SUBMENU, "Move \"Copy\" to submenu");

  // Help
  menu->addSubMenu(ROOT_MENU_ITEM, CM_HELP, "Help");

  menu->addItem(CM_HELP, CM_EDITOR_HELP, "Editor help");
  menu->addItem(CM_HELP, CM_PLUGIN_HELP, "Plugin help");

  menu->addSeparator(CM_HELP);

  menu->addItem(CM_HELP, CM_TUTORIALS, "Tutorials");

  menu->addSeparator(CM_HELP);

  menu->addItem(CM_HELP, CM_ABOUT, "About DaEditorX");
}


void DagorEdAppWindow::updateMenu(PropPanel::IMenu *menu) {}


void DagorEdAppWindow::updateExportMenuItems()
{
  PropPanel::IMenu *menu = getMainMenu();
  bool has_all = false;

  if (wsp)
  {
    DAGORED2->addExportPath(_MAKE4C('PC'));
    dag::ConstSpan<unsigned> platf = DAGORED2->getAdditionalPlatforms();
    for (int i = 0; i < platf.size(); ++i)
    {
      if (platf[i] == _MAKE4C('PS4'))
        menu->addItem(CM_FILE, CM_FILE_EXPORT_TO_GAME_PS4, "Export to game (PS 4 format)");
      else if (platf[i] == _MAKE4C('iOS'))
        menu->addItem(CM_FILE, CM_FILE_EXPORT_TO_GAME_iOS, "Export to game (iOS format)");
      else if (platf[i] == _MAKE4C('and'))
        menu->addItem(CM_FILE, CM_FILE_EXPORT_TO_GAME_AND, "Export to game (Android format)");
      else
        continue;
      DAGORED2->addExportPath(platf[i]);
      has_all = true;
    }
#if !_TARGET_64BIT
    menu->setEnabledById(CM_FILE_EXPORT_TO_GAME_PS4, false);
#endif
  }

  if (has_all)
  {
    menu->addSeparator(CM_FILE);
    menu->addItem(CM_FILE, CM_FILE_EXPORT_TO_GAME_ALL, "Export to game (all formats)");
  }
}


void DagorEdAppWindow::updateThemeItems()
{
  const bool isDefault = themeName == defaultThemeName;
  const unsigned int id = isDefault ? CM_THEME_LIGHT : CM_THEME_DARK;
  const char *pic = isDefault ? "theme_light" : "theme_dark";
  getMainMenu()->setRadioById(id, CM_THEME_LIGHT, CM_THEME_DARK);
  mToolPanel->setButtonPictures(CM_THEME_TOGGLE, pic);
}


int DagorEdAppWindow::onMenuItemClick(unsigned id)
{
  if (wsp)
  {
    const Tab<String> &recents = wsp->getRecents();
    const int menuResentCnt = CM_FILE_RECENT_LAST - CM_FILE_RECENT_BASE;
    const int minRecentRemainder = (recents.size() <= menuResentCnt) ? 0 : recents.size() - menuResentCnt;

    if (id >= CM_FILE_RECENT_BASE && id < CM_FILE_RECENT_LAST && id - CM_FILE_RECENT_BASE < recents.size())
    {
      String fileName = ::make_full_path(DAGORED2->getSdkDir(), recents[id - CM_FILE_RECENT_BASE + minRecentRemainder]);

      simplify_fname(fileName);

      if (!stricmp(fileName, sceneFname))
        return 1;

      if (canCloseScene("Open recent project"))
        loadProjectFromEditor(fileName);

      return 1;
    }
  }

  switch (id)
  {
    case CM_FILE_NEW:
    {
      undoSystem->clear();
      handleNewProject();
      return 1;
    }

    case CM_FILE_OPEN:
    {
      handleOpenProject(false);
      return 1;
    }

    case CM_FILE_RECENT_LAST:
    {
      return 1;
    }

    case CM_FILE_SETTINGS:
    {
      bool useDirLight = false;
      ProjectSettingsDlg projSettings(0, useDirLight);
      projSettings.showDialog();
      return 1;
    }

    case CM_CHANGE_VIEWPORT:
    {
      if (ged.getViewportLayout() == CM_VIEWPORT_LAYOUT_1)
        getMainMenu()->click(CM_VIEWPORT_LAYOUT_4);
      else
        getMainMenu()->click(CM_VIEWPORT_LAYOUT_1);
      return 1;
    }

    case CM_VIEWPORT_LAYOUT_1:
    case CM_VIEWPORT_LAYOUT_4:
    {
      ViewportWindow *cur_vport = ged.getCurrentViewport();

      int _id = 0;
      while (ged.getViewportCount() > 1)
      {
        ViewportWindow *vport = ged.getViewport(_id);
        if (vport && vport != cur_vport)
          mManager->removeWindow(vport);
        else
          _id++;
      }

      getMainMenu()->setRadioById(id, CM_VIEWPORT_LAYOUT_4, CM_VIEWPORT_LAYOUT_1);
      ged.setViewportLayout(id);

      ViewportWindow *curVp = ged.getViewport(0);
      if (curVp)
      {
        ged.setCurrentViewportId(0);
        curVp->activate();
      }

      if (id == CM_VIEWPORT_LAYOUT_1)
        return 1;

      if (curVp)
      {
        mManager->setWindowType(nullptr, VIEWPORT_TYPE);
        mManager->setWindowType(nullptr, VIEWPORT_TYPE);
        mManager->setWindowType(nullptr, VIEWPORT_TYPE);
      }

      return 1;
    }

    case CM_LOAD_DEFAULT_LAYOUT:
    {
      mManager->show(WSI_MAXIMIZED);

      ImGuiContext *context = ImGui::GetCurrentContext();
      for (ImGuiWindow *window : context->Windows)
        ImGui::ClearWindowSettings(window->Name);

      makeDefaultLayout();
      getMainMenu()->click(CM_VIEWPORT_LAYOUT_1);

      const int oldPluginId = curPluginId;
      switchToPlugin(-1);
      switchToPlugin(oldPluginId);
      return 1;
    }

    case CM_LOAD_LAYOUT:
    {
      String result = wingw::file_open_dlg(mManager->getMainWindow(), "Load layout from...", "Layout|*.blk", "blk");
      if (!result.empty())
        mManager->loadLayout(result.str());
    }
      return 1;

    case CM_SAVE_LAYOUT:
    {
      String result = wingw::file_save_dlg(mManager->getMainWindow(), "Save layout as...", "Layout|*.blk", "blk");
      if (!result.empty())
        mManager->saveLayout(result.str());
    }
      return 1;

    case CM_OPTIONS_TOTAL:
    {
      if (mPluginShowDialogIsShow)
        return 1;
      mPluginShowDialogIsShow = true;

      ::delayed_call([&]() {
        PluginShowDialog plugDlg(0, wsp->getF11Tags(), wsp->getPluginTags());

        int i;
        Tab<PluginMap> visPlugins(tmpmem);
        for (i = 0; i < plugin.size(); ++i)
          visPlugins.push_back(PluginMap(&plugin[i], i));

        sort(visPlugins, &plugs_compare);

        for (i = 0; i < visPlugins.size(); ++i)
          if (visPlugins[i].plugin->p)
            plugDlg.addPlugin(visPlugins[i].plugin->p, &visPlugins[i].plugin->data->hotkey, visPlugins[i].idx);
        plugDlg.autoSize(!plugDlg.hasEverBeenShown());

        plugDlg.showDialog();
        mPluginShowDialogIsShow = false;
      });

      return 1;
    }

    case CM_CUR_PLUGIN_VIS_CHANGE:
    {
      IGenEditorPlugin *plug = curPlugin();
      if (plug)
      {
        plug->setVisible(!plug->getVisible());
        repaint();
      }

      return 1;
    }

    case CM_WINDOW_TOOLBAR:
      if (!mToolPanel)
      {
        mManager->setWindowType(nullptr, TOOLBAR_TYPE);
      }
      return 0;

    case CM_WINDOW_TABBAR:
      if (!mTabWindow)
      {
        mManager->setWindowType(nullptr, TABBAR_TYPE);
      }
      return 0;

    case CM_WINDOW_PLUGTOOLS:
      if (!mPlugTools)
      {
        mManager->setWindowType(nullptr, PLUGTOOLS_TYPE);
      }
      return 0;

    case CM_WINDOW_VIEWPORT:
      if (!ged.getViewportCount())
      {
        mManager->setWindowType(nullptr, VIEWPORT_TYPE);
      }
      return 0;

    case CM_THEME_LIGHT:
    case CM_THEME_DARK:
    {
      themeName = (id == CM_THEME_LIGHT) ? defaultThemeName : "dark";
      updateThemeItems();
      editor_core_load_imgui_theme(getThemeFileName());
      return 1;
    }

    case CM_FILE_EXPORT_TO_GAME_PC:
    {
      exportLevelToGame(_MAKE4C('PC'));
      return 1;
    }
    case CM_FILE_EXPORT_TO_GAME_PS4: exportLevelToGame(_MAKE4C('PS4')); return 1;

    case CM_FILE_EXPORT_TO_GAME_iOS: exportLevelToGame(_MAKE4C('iOS')); return 1;

    case CM_FILE_EXPORT_TO_GAME_AND: exportLevelToGame(_MAKE4C('and')); return 1;

    case CM_FILE_EXPORT_TO_GAME_ALL:
    {
      Tab<const char *> params(tmpmem);
      runProjectExportCmd(params);
    }
      return 1;

    case CM_FILE_EXPORT_TO_DAG: exportLevelToDag(true); return 1;

    case CM_FILE_EXPORT_COLLISION_TO_DAG: exportLevelToDag(false); return 1;

    case CM_UNDO:
      undoSystem->undo();
      updateUndoRedoMenu();
      IEditorCoreEngine::get()->updateViewports();
      IEditorCoreEngine::get()->invalidateViewportCache();
      return 1;

    case CM_REDO:
      undoSystem->redo();
      updateUndoRedoMenu();
      IEditorCoreEngine::get()->updateViewports();
      IEditorCoreEngine::get()->invalidateViewportCache();
      return 1;

    case CM_CREATE_SCREENSHOT: createScreenshot(); return 1;

    case CM_CREATE_CUBE_SCREENSHOT:
    {
      IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>();
      bool old_no_postfx = drSrv->getRenderOptEnabled(drSrv->ROPT_NO_POSTFX);
      drSrv->setRenderOptEnabled(drSrv->ROPT_NO_POSTFX, true);

      createCubeScreenshot();

      drSrv->setRenderOptEnabled(drSrv->ROPT_NO_POSTFX, old_no_postfx);
      return 1;
    }

    case CM_CREATE_ORTHOGONAL_SCREENSHOT:
    {
      String dest;
      getProjectFolderPath(dest);
      createOrthogonalScreenshot(dest + "/map2d");
    }
      return 1;

    case CM_UPSCALE_HEIGHTMAP:
    {
      if (sceneFname[0] && canCloseScene("Upscale hmap and reopen project"))
      {
        int desiredHmapUpscale = EDITORCORE->queryEditorInterface<IHmapService>()->getDesiredHmapUpscale();
        EDITORCORE->queryEditorInterface<IHmapService>()->setHmapUpscale(desiredHmapUpscale);
        loadProjectFromEditor(String(sceneFname));
        EDITORCORE->queryEditorInterface<IHmapService>()->setHmapUpscale(1);
        EDITORCORE->queryEditorInterface<IHmapService>()->setDesiredHmapUpscale(1);
        queryEditorInterface<IDynRenderService>()->enableRender(!d3d::is_stub_driver());
      }

      return 1;
    }

    case CM_ENVIRONMENT_SETTINGS:
      environment::show_environment_settings(0);
      // break;
      return 1;

    case CM_OPTIONS_CAMERAS: show_camera_objects_config_dialog(0); return 1;

    case CM_OPTIONS_SCREENSHOT: setScreenshotOptions(ScreenshotDlgMode::DA_EDITOR); return 1;

    case CM_OPTIONS_STAT_DISPLAY_SETTINGS:
    {
      ViewportWindow *viewport = ged.getCurrentViewport();
      if (viewport)
        viewport->showStatSettingsDialog();

      return 1;
    }

    case CM_OPTIONS_ENABLE_DEVELOPER_TOOLS:
    {
      if (!developerToolsEnabled)
        wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Warning",
          "Developer tools are not intended to be used by users and may cause bugs and crashes. Use them at your own risk.");

      developerToolsEnabled = !developerToolsEnabled;
      getMainMenu()->setCheckById(id, developerToolsEnabled);
      getMainMenu()->setEnabledById(CM_VIEW_DEVELOPER_TOOLS, developerToolsEnabled);
      return 1;
    }

    case CM_OPTIONS_PREFERENCES_ASSET_TREE_COPY_SUBMENU:
    {
      bool moveCopyToSubmenu = !AssetSelectorGlobalState::getMoveCopyToSubmenu();
      AssetSelectorGlobalState::setMoveCopyToSubmenu(moveCopyToSubmenu);
      getMainMenu()->setCheckById(id, moveCopyToSubmenu);
      return 1;
    }

    case CM_FILE_OPEN_AND_LOCK: handleOpenProject(true); return 1;

    case CM_FILE_REOPEN:
      if (sceneFname[0] && canCloseScene("Reopen project"))
        loadProjectFromEditor(String(sceneFname));
      return 1;

    case CM_FILE_SAVE:
      if (sceneFname[0])
      {
        saveProject(sceneFname);
        addToRecentList(sceneFname);
      }
      return 1;

    case CM_FILE_SAVE_AS: handleSaveAsProject(); return 1;

    case CM_SELECT_ALL:
      if (curPlugin() && curPlugin()->showSelectAll())
        curPlugin()->selectAll();
      return 1;

    case CM_DESELECT_ALL:
      if (curPlugin())
        curPlugin()->deselectAll();
      return 1;

    case CM_OPTIONS_GRID:
    {
      ViewportWindow *viewport = ged.getCurrentViewport();
      if (viewport)
        viewport->showGridSettingsDialog();

      return 1;
    }
    case CM_ANIMATE_VIEWPORTS:
    {
      animateViewports = !animateViewports;
      ged.setViewportCacheMode(animateViewports ? IDagorEd2Engine::VCM_NoCacheAll : IDagorEd2Engine::VCM_CacheAll);

      getMainMenu()->setCheckById(id, animateViewports);
      return 1;
    }
    case CM_NAV_COMPASS:
    {
      bool show = static_cast<DagorEdAppEventHandler *>(appEH)->showCompass;
      static_cast<DagorEdAppEventHandler *>(appEH)->showCompass = !show;
      getMainMenu()->setCheckById(id, !show);
      EDITORCORE->getCustomPanel(GUI_MAIN_TOOLBAR_ID)->setBool(CM_NAV_COMPASS, !show);
      IEditorCoreEngine::get()->updateViewports();
      IEditorCoreEngine::get()->invalidateViewportCache();
      return 1;
    }

    case CM_DISCARD_TEX_MODE:
      discardTexMode = !discardTexMode;
      getMainMenu()->setCheckById(id, discardTexMode);
      mToolPanel->setBool(id, discardTexMode);
      ec_set_busy(true);
      texconvcache::build_on_demand_tex_factory_limit_tql(discardTexMode ? TQL_thumb : TQL_uhq);
      spawnEvent(HUID_InvalidateClipmap, (void *)true);
      ec_set_busy(false);
      break;

    case CM_VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES:
      consoleCommandsAndVariableWindowsVisible = !consoleCommandsAndVariableWindowsVisible;
      imgui_window_set_visible("General", "Console commands and variables", consoleCommandsAndVariableWindowsVisible);
      getMainMenu()->setCheckById(id, consoleCommandsAndVariableWindowsVisible);
      return 1;

    case CM_VIEW_DEVELOPER_TOOLS_CONTROL_GALLERY:
    {
      if (control_gallery)
        control_gallery.reset();
      else
        control_gallery.reset(new ControlGallery());

      return 1;
    }

    case CM_VIEW_DEVELOPER_TOOLS_IMGUI_DEBUGGER:
      imguiDebugWindowsVisible = !imguiDebugWindowsVisible;
      getMainMenu()->setCheckById(id, imguiDebugWindowsVisible);
      return 1;

    case CM_VIEW_DEVELOPER_TOOLS_TEXTURE_DEBUG: texdebug::select_texture("none"); return 1;

    case CM_VIEW_DEVELOPER_TOOLS_NODE_DEPS: dafg::show_dependencies_window(); return 1;

    case CM_THEME_TOGGLE:
      themeName = (themeName != defaultThemeName) ? defaultThemeName : "dark";
      updateThemeItems();
      editor_core_load_imgui_theme(getThemeFileName());
      break;

    case CM_USE_OCCLUDERS:
      shouldUseOccluders = !shouldUseOccluders;
      getMainMenu()->setCheckById(id, shouldUseOccluders);
      updateUseOccluders();
      return 1;

    case CM_BATCH_RUN:
    {
      DeBatch batch;
      batch.openAndRunBatch();
      return 1;
    }

    case CM_EDITOR_HELP: editorHelp(); return 1;

    case CM_PLUGIN_HELP:
    {
      IGenEditorPlugin *plug = curPlugin();
      if (plug)
        pluginHelp(plug->getHelpUrl());

      return 1;
    }

    case CM_ABOUT:
    {
      aboutDlg->show();
      return 1;
    }
      /*
          case CM_VIEW_EDGED:
            GeomObject::isEdgedFaceView = !GeomObject::isEdgedFaceView;
            GeomObject::edgedFaceColor = E3DCOLOR(200, 200, 200, 255);
            repaint();
            return 1;
      */

    case CM_ZOOM_AND_CENTER:
    {
      zoomAndCenter();
      return 1;
    }

    case CM_VIEW_GRID_MOVE_SNAP:
    case CM_VIEW_GRID_ANGLE_SNAP:
    case CM_VIEW_GRID_SCALE_SNAP:
      if (IGenViewportWnd *vpw = getCurrentViewport())
        if (vpw->isActive())
          switch (id)
          {
            case CM_VIEW_GRID_MOVE_SNAP: ged.tbManager->setMoveSnap(); break;
            case CM_VIEW_GRID_ANGLE_SNAP: ged.tbManager->setRotateSnap(); break;
            case CM_VIEW_GRID_SCALE_SNAP: ged.tbManager->setScaleSnap(); break;
          }
      break;

    case CM_CAMERAS_FREE:
    case CM_CAMERAS_FPS:
    case CM_CAMERAS_TPS:
    case CM_CAMERAS_CAR:
    {
      IGenViewportWnd *vpw = getCurrentViewport();

      if ((!vpw) || (!vpw->isActive()))
        return 0;

      if (vpw)
      {
        vpw->activate();
        vpw->handleCommand(id);

        mToolPanel->setBool(CM_CAMERAS_FREE, CCameraElem::getCamera() == CCameraElem::FREE_CAMERA);
        mToolPanel->setBool(CM_CAMERAS_FPS, CCameraElem::getCamera() == CCameraElem::FPS_CAMERA);
        mToolPanel->setBool(CM_CAMERAS_TPS, CCameraElem::getCamera() == CCameraElem::TPS_CAMERA);
        mToolPanel->setBool(CM_CAMERAS_CAR, CCameraElem::getCamera() == CCameraElem::CAR_CAMERA);

        if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
          addEditorAccelerators();
        else
          addCameraAccelerators();

        if (id == CM_CAMERAS_FREE)
        {
          if (!vpw->isFlyMode())
            setEnabledColliders(backupPluginColliders);
          else
            getEnabledColliders(backupPluginColliders);
        }
      }

      if (id == CM_CAMERAS_FPS || id == CM_CAMERAS_TPS || id == CM_CAMERAS_CAR)
      {
        getEnabledColliders(backupPluginColliders);
        setEnabledColliders(activeColliders);
      }
      return 1;
    }

    case CM_DEBUG_FLUSH:
    {
      debug_flush(false);
      return 1;
    }

    case CM_SWITCH_PLUGIN_F5:
    case CM_SWITCH_PLUGIN_F6:
    case CM_SWITCH_PLUGIN_F7:
    case CM_SWITCH_PLUGIN_F8:
    case CM_SWITCH_PLUGIN_SF1:
    case CM_SWITCH_PLUGIN_SF2:
    case CM_SWITCH_PLUGIN_SF3:
    case CM_SWITCH_PLUGIN_SF4:
    case CM_SWITCH_PLUGIN_SF5:
    case CM_SWITCH_PLUGIN_SF6:
    case CM_SWITCH_PLUGIN_SF7:
    case CM_SWITCH_PLUGIN_SF8:
    case CM_SWITCH_PLUGIN_SF9:
    case CM_SWITCH_PLUGIN_SF10:
    case CM_SWITCH_PLUGIN_SF11:
    case CM_SWITCH_PLUGIN_SF12:
    {
      for (int pi = 0; pi < plugin.size(); ++pi)
        if (plugin[pi].data && plugin[pi].data->hotkey == id)
          plugin[pi].p->setVisible(!plugin[pi].p->getVisible());

      return 1;
    }

    case CM_NEXT_PLUGIN: switchPluginTab(true); return 1;

    case CM_PREV_PLUGIN: switchPluginTab(false); return 1;

    case CM_CONSOLE:
      if (console->isVisible())
      {
        if (console->isCanClose() && !console->isProgress())
          console->hideConsole();
      }
      else
        console->showConsole(true);
      return 1;
  }

  // change plugin

  if (id >= CM_PLUGINS_MENU_1ST && id <= CM_PLUGINS_MENU__LAST)
    switchToPlugin(id - CM_PLUGINS_MENU_1ST);

  // plugin's menu processing

  if (id >= CM_PLUGIN_BASE && id <= CM_PLUGIN__LAST && curPluginId != -1 && plugin[curPluginId].p)
  {
    if ((CCameraElem::getCamera() != CCameraElem::MAX_CAMERA) && (id == CM_TAB_PRESS || id == CM_TAB_RELEASE))
      return 0;

    if (plugin[curPluginId].p->onPluginMenuClick(id))
      return 1;
  }

  // plugin's settings processing

  if (id >= CM_PLUGIN_SETTINGS_BASE)
  {
    for (int i = 0; i < plugin.size(); ++i)
    {
      if (plugin[i].p && plugin[i].p->onSettingsMenuClick(id))
        return 1;
    }
  }

  return GenericEditorAppWindow::onMenuItemClick(id);
}


void DagorEdAppWindow::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
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
    case CM_VIEW_GRID_MOVE_SNAP: ged.tbManager->setMoveSnap(); break;

    case CM_VIEW_GRID_ANGLE_SNAP: ged.tbManager->setRotateSnap(); break;

    case CM_VIEW_GRID_SCALE_SNAP: ged.tbManager->setScaleSnap(); break;

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

    case CM_STATS: showStats(); break;

    case CM_CHANGE_FOV: onChangeFov(); break;
  }
}


void DagorEdAppWindow::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  // ged.activateCurrentViewport();

  if (ged.tbManager->onChange(pcb_id, panel))
    return;

  switch (pcb_id)
  {
    case TAB_PANEL_ID: onTabSelChange(mTabWindow->getInt(TAB_PANEL_ID)); break;
  }
}


bool DagorEdAppWindow::canClose()
{
  if (!::blockCloseMessage)
  {
    const bool ret = GenericEditorAppWindow::canClose();

    if (ret)
      ::blockCloseMessage = true;

    return ret;
  }


  switchToPlugin(-1);

  return true;
}


//==================================================================================================

void DagorEdAppWindow::initDllPlugins(const char *plug_dir)
{
  String libPath;

#if DAGOR_DBGLEVEL > 1
  String mask = ::make_full_path(plug_dir, "*-dbg.dll");
#elif DAGOR_DBGLEVEL > 0
  String mask = ::make_full_path(plug_dir, "*-dev.dll");
#else
  String mask("");
#endif
  String errorMess;

  DataBlock appblk(String(260, "%s/application.blk", wsp->getAppDir()));
  const DataBlock &sdk_blk = *appblk.getBlockByNameEx("SDK");
  Tab<RegExp *> reExcludeDll(tmpmem);
  Tab<RegExp *> reIncludeDll(tmpmem);
  int nid = sdk_blk.getNameId("exclude_dll");
  for (int j = 0; j < sdk_blk.paramCount(); j++)
    if (sdk_blk.getParamNameId(j) == nid && sdk_blk.getParamType(j) == DataBlock::TYPE_STRING)
    {
      const char *pattern = sdk_blk.getStr(j);
      RegExp *re = new RegExp;
      if (re->compile(pattern, "i"))
        reExcludeDll.push_back(re);
      else
      {
        DAEDITOR3.conError("SDK/exclude_dll: bad regexp /%s/", pattern);
        delete re;
      }
    }
  iterate_names(cmdline_include_dll_re, [&](int, const char *pattern) {
    RegExp *re = new RegExp;
    if (re->compile(pattern, "i"))
      reIncludeDll.push_back(re);
    else
    {
      DAEDITOR3.conError("include_dll_re: bad regexp /%s/", pattern);
      delete re;
    }
  });
  iterate_names(cmdline_exclude_dll_re, [&](int, const char *pattern) {
    RegExp *re = new RegExp;
    if (re->compile(pattern, "i"))
      reExcludeDll.push_back(re);
    else
    {
      DAEDITOR3.conError("exclude_dll_re: bad regexp /%s/", pattern);
      delete re;
    }
  });

  for (const alefind_t &ff : dd_find_iterator(mask, DA_FILE))
  {
    int errorType = ILogWriter::ERROR;
    libPath = ::make_full_path(plug_dir, ff.name);

    for (int i = 0; i < reExcludeDll.size(); i++)
      if (reExcludeDll[i]->test(libPath))
      {
        bool req_incl = false;
        for (int j = 0; j < reIncludeDll.size(); j++)
          if (reIncludeDll[j]->test(libPath))
          {
            req_incl = true;
            break;
          }
        if (req_incl)
          break;

        DAEDITOR3.conNote("Plugin %s skipped", libPath.str());
        libPath = "";
        break;
      }
    if (libPath.empty())
      continue;

    console->addMessage(ILogWriter::REMARK, "Loading DLL \"%s\"...", ff.name);

    void *dllHandle = os_dll_load_deep_bind(libPath);

    if (dllHandle)
    {
      ::symhlp_load(libPath);
      GetPluginVersionFunc getVersionFunc = (GetPluginVersionFunc)os_dll_get_symbol(dllHandle, "get_plugin_version");

      if (getVersionFunc)
      {
        if ((*getVersionFunc)() != IGenEditorPlugin::CURRENT_VERSION)
        {
          ::symhlp_unload(libPath);
          os_dll_close(dllHandle);
          console->addMessage(ILogWriter::WARNING, "Plugin version mismatch. It can make Editor unstable.");
          continue;
        }

        RegisterPluginFunc regFunc = (RegisterPluginFunc)os_dll_get_symbol(dllHandle, "register_plugin");

        if (regFunc)
        {
          DAGOR_TRY
          {
            IGenEditorPlugin *plug = (*regFunc)(*IDagorEd2Engine::get(), libPath);

            if (plug)
            {
              if (registerDllPlugin(plug, dllHandle, libPath))
                console->addMessage(ILogWriter::REMARK, "Plugin \"%s\" (%s) succesfully registered", ff.name,
                  plug->getMenuCommandName());
              else
              {
                ReleasePluginFunc releaseFunc = (ReleasePluginFunc)os_dll_get_symbol(dllHandle, "release_plugin");
                if (releaseFunc)
                  (*releaseFunc)();

                errorMess = String(256, "Plugin from DLL \"%s\" not registered", ff.name);
                errorType = ILogWriter::NOTE;
              }
            }
            else
              errorMess = String(256, "Couldn't register plugin from DLL \"%s\"", ff.name);
          }
          DAGOR_CATCH(...) { errorMess = String(256, "Exception - couldn't register plugin from DLL \"%s\"", ff.name); }
        }
        else
          errorMess = String(256,
            "Couldn't locate \"register_plugin()\" function in DLL "
            "\"%s\"",
            ff.name);
      }
      else
        errorMess = String(256,
          "Couldn't locate \"get_plugin_version()\" function in DLL "
          "\"%s\"",
          ff.name);
    }
    else
    {
      errorType = ILogWriter::NOTE;
      errorMess = String(256, "DLL \"%s\" not loaded", ff.name);
    }

    if (errorMess.length())
    {
      ::symhlp_unload(libPath);
      os_dll_close(dllHandle);
      console->addMessage((ILogWriter::MessageType)errorType, errorMess);

      errorMess.clear();
    }
  }
  clear_all_ptr_items(reExcludeDll);
}


//==============================================================================
void DagorEdAppWindow::startWithWorkspace(const char *wspName)
{
  String settingsPath = ::make_full_path(sgg::get_exe_path_full(), "../.local/de3_settings.blk");
  DataBlock settingsBlk(settingsPath);

  loadScreenshotSettings(settingsBlk);

  developerToolsEnabled = settingsBlk.getBool("DeveloperToolsEnabled", developerToolsEnabled);
  GizmoSettings::load(settingsBlk);

  if (!wspName)
    wspName = settingsBlk.getBlockByNameEx("workspace")->getStr("currentName", NULL);

  // Asset Viewer and daEditorX work differently, if daEditorX does not find the specified workspace then
  // EditorStartDialog's ctor calls EditorStartDialog::onAddWorkspace() which shows a dialog, so the startup scene is
  // always needed.
  startup_editor_core_select_startup_scene();

  for (bool handled = false; !handled;)
  {
    StartupDlg dlg("DaEditorX start", *getWorkspace(), WSP_FILE_PATH, wspName, shouldLoadFile);

    int result = PropPanel::DIALOG_ID_OK;
    int sel = ID_RECENT_FIRST;
    if (!shouldLoadFile || dlg.getWorkspaceIndex(wspName) < 0 || !dd_file_exist(DAGORED2->getWorkspace().getAppPath()))
    {
      startupDlgShown = &dlg;
      result = dlg.showDialog();
      sel = dlg.getSelected();
      startupDlgShown = nullptr;

      if (result == PropPanel::DIALOG_ID_CANCEL || sel == -1 || stricmp(wsp->getName(), dlg.getStartWorkspace()) != 0)
      {
        sceneFname[0] = 0; // Prevent canCloseScene() from asking to save the project, nothing has changed yet.
        shouldLoadFile = false;
      }

      startup_editor_core_force_startup_scene_draw();
    }

    dagor_idle_cycle();

    handled = true;

    if (result == PropPanel::DIALOG_ID_CANCEL || sel == -1)
    {
      mManager->close();
      return;
    }

    const char *wspName = wsp->getName();
    if (!wspName || !*wspName)
      continue;

    ec_set_busy(true);
    resetCore();
    DataBlock app_blk;
    if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
    {
      DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());
      handled = false;
      continue;
    }

    init3d();

    const String imguiIniPath(::make_full_path(sgg::get_exe_path_full(), "../.local/de3_imgui.ini"));
    int loadedDockSettingsVersion = 0;
    editor_core_initialize_imgui(imguiIniPath, &loadedDockSettingsVersion);
    dockPositionsInitialized = loadedDockSettingsVersion == LATEST_DOCK_SETTINGS_VERSION;

    editor_core_load_imgui_theme(getThemeFileName());
    setDocTitle();

    DataBlock perAppSettingsBlk;
    const String perAppSettingsPath = getPerApplicationSettingsBlkPath();
    if (!perAppSettingsPath.empty() && dd_file_exists(perAppSettingsPath))
      perAppSettingsBlk.load(perAppSettingsPath);

    editor_core_load_dag_imgui_blk_settings(perAppSettingsBlk);
    environment::load_ui_state(perAppSettingsBlk);
    initPlugins(settingsBlk, perAppSettingsBlk);

    const Tab<String> &recents = wsp->getRecents();

    // to mirror in menu workspace specific data
    //::appWnd.recreateMenu();

    const DataBlock &projDef = *app_blk.getBlockByNameEx("projectDefaults");
    StaticSceneBuilder::check_degenerates = projDef.getBool("check_degenerates", true);
    StaticSceneBuilder::check_no_smoothing = projDef.getBool("check_no_smoothing", true);

    updateExportMenuItems();
    getMainMenu()->addSeparator(CM_FILE);
    getMainMenu()->addItem(CM_FILE, CM_EXIT, "Exit\tAlt+F4");

    switch (sel)
    {
      case ID_CREATE_NEW: // new
        handled = handleNewProject(false);
        break;

      case ID_OPEN_PROJECT: // open
        handled = handleOpenProject(false);
        break;

      case ID_OPEN_DRAG_AND_DROPPED_SCREENSHOT:
      {
        ::strcpy(sceneFname, dlg.getFilePathFromScreenshotMetaInfo());
        screenshotMetaInfoToApply = dlg.getScreenshotMetaInfo();

        addToRecentList(sceneFname);

        if (loadProject(sceneFname))
        {
          DagorEdAppWindow::setDocTitle();
          handled = true;
        }
        else
        {
          wingw::message_box(wingw::MBS_EXCL, "Fatal error", "Unable to load project from file\n%s", (const char *)sceneFname);
          sceneFname[0] = 0;
          quit_game(-1);
        }

        screenshotMetaInfoToApply.reset();
      }
      break;

      case ID_OPEN_FROM_COMMAND_LINE:
      default: // load recent
        if (recents.size() || shouldLoadFile)
        {
          String fileName = ::make_full_path(wsp->getSdkDir(), shouldLoadFile ? sceneFname : recents[sel - ID_RECENT_FIRST]);
          ::strcpy(sceneFname, fileName);

          addToRecentList(sceneFname);

          if (loadProject(sceneFname))
          {
            DagorEdAppWindow::setDocTitle();
            handled = true;
          }
          else
          {
            wingw::message_box(wingw::MBS_EXCL, "Fatal error", "Unable to load project from file\n%s", (const char *)sceneFname);
            sceneFname[0] = 0;
            quit_game(-1);
          }
        }
        break;
    }
    if (!handled) //== we init d3d for workspace only once!
      quit_game(-1);
    ec_set_busy(false);
  }
  shouldLoadFile = false;

  wsp->freeWorkspaceBlk();
  updateRecentMenu();

  ec_set_busy(true);

  // Rendering must be enabled before the HUID_AfterProjectLoad event because in response to this event the "Select
  // binary dump file" modal dialog is shown by the BinSceneViewPlugin in War Thunder. (It is drawn by ImGui, so if
  // the rendering is not enabled at that point then the application appears to be frozen.)
  queryEditorInterface<IDynRenderService>()->enableRender(!d3d::is_stub_driver());
  queryEditorInterface<IDynRenderService>()->selectAsGameScene();
  spawnEvent(HUID_AfterProjectLoad, NULL);

  repaint();
  force_screen_redraw();
  dagor_idle_cycle();
  DEBUG_CP();
  ec_set_busy(false);

  DAGORED2->getConsole().hideConsole();
}


void *DagorEdAppWindow::queryEditorInterfacePtr(unsigned huid)
{
  if (huid == HUID_IDaEditor3Engine)
    return static_cast<IDaEditor3Engine *>(&IDaEditor3Engine::get());

  if (huid == HUID_IEditorCore)
    return static_cast<IEditorCore *>(&editorCoreImpl);

  if (huid == HUID_IDagorPhys)
    return static_cast<IDagorPhys *>(&deDagorPhysImpl);

  if (huid == HUID_IBitMaskImageMgr)
    return ::get_tiff_bit_mask_image_mgr();

  if (huid == HUID_IHmapService)
    return ::get_generic_hmap_service();

  if (huid == HUID_IAssetService)
    return ::get_generic_asset_service();

  if (huid == HUID_ISceneLightService)
    return get_gen_light_service();

  if (huid == HUID_IFileChangeTracker)
    return get_generic_file_change_tracking_service();

  if (huid == HUID_ISkiesService)
    return get_generic_skies_service();

  if (huid == HUID_IColorRangeService)
    return get_generic_color_range_service();

  if (huid == HUID_IRendInstGenService)
    return get_generic_rendinstgen_service();

  if (huid == HUID_IGrassService)
    return get_generic_grass_service();

  if (huid == HUID_IGPUGrassService)
    return get_gpu_grass_service();

  if (huid == HUID_IWaterService)
    return get_generic_water_service();

  if (huid == HUID_IDynRenderService)
    return get_generic_dyn_render_service();

  if (huid == HUID_IRenderHelperService)
    return get_generic_render_helper_service();

  if (huid == HUID_IWaterProjFxService)
    return get_generic_water_proj_fx_service();

  if (huid == HUID_ICableService)
    return get_generic_cable_service();

  if (huid == HUID_ISplineGenService)
    return get_generic_spline_gen_service();

  if (huid == HUID_IWindService)
    return get_generic_wind_service();

  if (huid == HUID_IPixelPerfectSelectionService)
    return get_pixel_perfect_selection_service();

  if (huid == HUID_IVisibilityFinderProvider)
    return get_visibility_finder_service();

  RETURN_INTERFACE(huid, IMainWindowImguiRenderingService);

  return NULL;
}


void *DagorEdAppWindow::getInterface(int huid)
{
  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i].p)
    {
      void *iface = plugin[i].p->queryInterfacePtr(huid);
      if (iface)
        return iface;
    }

  return NULL;
}


void DagorEdAppWindow::getInterfaces(int huid, Tab<void *> &interfaces)
{
  for (int i = 0; i < plugin.size(); ++i)
  {
    if (plugin[i].p)
    {
      void *iface = plugin[i].p->queryInterfacePtr(huid);
      if (iface)
        interfaces.push_back(iface);
    }
  }
}


//==============================================================================
void DagorEdAppWindow::addToRecentList(const char *fname, bool menu_update)
{
  if (!fname || !*fname)
    return;

  String fileName;

  if (::is_full_path(fname))
    fileName = ::make_path_relative(fname, wsp->getSdkDir());

  Tab<String> &recents = wsp->getRecents();

  for (int i = 0; i < recents.size(); ++i)
    if (!::stricmp(recents[i], fileName))
    {
      if (!i)
        return;

      erase_items(recents, i, 1);
      break;
    }

  insert_item_at(recents, 0, fileName);
  updateRecentMenu();

  wsp->save();
}


void DagorEdAppWindow::updateRecentMenu()
{
  PropPanel::IMenu *_menu = getMainMenu();
  _menu->clearMenu(CM_FILE_RECENT_LAST);

  const int recentCount = min((int)wsp->getRecents().size(), CM_FILE_RECENT_LAST - CM_FILE_RECENT_BASE);
  for (int i = 0; i < recentCount; ++i)
    _menu->addItem(CM_FILE_RECENT_LAST, CM_FILE_RECENT_BASE + i, String(0, "%d. %s", i + 1, wsp->getRecents().at(i).c_str()));

  if (recentCount == 0)
  {
    _menu->addItem(CM_FILE_RECENT_LAST, CM_FILE_RECENT_BASE, "Empty");
    _menu->setEnabledById(CM_FILE_RECENT_BASE, false);
  }
}

//==============================================================================

/*
void DagorEdAppWindow::handleNotifyEvent(CtlWndObject *w, CtlEventData &ev)
{
  switch (ev.id)
  {
    case CTLWM_CLICK:
    {
      const int btnId = w->getId();
      switch (btnId)
      {
        case 0: //==
          break;
      }
      if ((btnId >= CM_OBJED_MODE_SELECT && btnId <= CM_OBJED_DELETE) ||
        (btnId >= CM_PLUGIN_BASE && btnId <= CM_PLUGIN__LAST && ged.curEH))
          appEH->handleButtonClick(btnId, (CtlBtnTemplate*)w,
            ((CtlBtnTemplate*)w)->getState());

      break;
    }
  }

  GenericEditorAppWindow::handleNotifyEvent(w, ev);
}
*/

//==============================================================================

void DagorEdAppWindow::copyFiles(const FilePathName &from, const FilePathName &to, bool overwrite)
{
  for (const alefind_t &ff : dd_find_iterator(FilePathName(from, "*.*"), DA_SUBDIR))
  {
    FilePathName src(from, ff.name);
    FilePathName dst(to, ff.name);

    if (ff.name[0] == '.' && ff.name[1] == '#')
      continue;

    if (stricmp(ff.name, "cvs") == 0)
      continue;

    if (ff.attr & DA_SUBDIR)
    {
      if (!::dd_dir_exist(dst))
        dd_mkdir(dst);

      copyFiles(src, dst, overwrite);
    }
    else
      dag_copy_file(src, dst, overwrite);
  }
}

//==============================================================================
String DagorEdAppWindow::getApplicationName() const
{
  if (!wsp || !wsp->getAppPath() || wsp->getAppPath()[0] == '\0')
    return String();

  String fileName;
  String absoluteAppPath = make_path_absolute(wsp->getAppPath());
  if (!absoluteAppPath.empty())
  {
    String dirPath;
    split_path(absoluteAppPath, dirPath, fileName);

    fileName = get_file_name(dirPath);
    trim(fileName);
  }

  return fileName;
}

//==============================================================================
String DagorEdAppWindow::getPerApplicationSettingsBlkPath() const
{
  const String name = getApplicationName();
  if (name.empty())
    return name;
  else
    return ::make_full_path(sgg::get_exe_path_full(), String(0, "../.local/de3_settings_%s.blk", name.c_str()));
}

//==============================================================================
void DagorEdAppWindow::handleSaveAsProject()
{
  NewProjectDialog dlg(mManager->getMainWindow(), "Save project as", "New project name", "Project will be saved to:");

  String location(wsp->getLevelsDir());

  if (location == "")
    location = sgg::get_exe_path_full();

  dlg.setLocation(location);
  dlg.setName("");
  if (dlg.showDialog() != PropPanel::DIALOG_ID_OK)
    return;

  String fileName(260, "%s/%s/%s" LEVEL_FILE_EXTENSION, dlg.getLocation(), dlg.getName(), dlg.getName());

  ::simplify_fname(fileName);

  if (!canCreateNewProject(fileName))
    return;

  // recurcive copy files
  copyFiles(FilePathName(sceneFname).getPath(), FilePathName(fileName).getPath(), false);

  // remove old project file from copy
  String projBlk(::get_file_name(sceneFname));
  String localBlk = ::get_file_name_wo_ext(projBlk) + ".local.blk";
  String projDir(fileName);
  ::location_from_path(projDir);

  ::dd_erase(::make_full_path(projDir, projBlk));
  ::dd_erase(::make_full_path(projDir, localBlk));

  strcpy(sceneFname, fileName);
  addToRecentList(sceneFname);

  if (!saveProject(fileName))
    return;

  DagorEdAppWindow::setDocTitle();
}


//==============================================================================
void DagorEdAppWindow::editorHelp() const
{
#if _TARGET_PC_WIN // TODO: tools Linux porting: DagorEdAppWindow::editorHelp
  String helpPath(260, "%s/daeditor3.chm::/html/Overview/index.htm", sgg::get_exe_path_full());

  ::HtmlHelp(NULL, helpPath.str(), HH_DISPLAY_TOPIC, 0);
#else
  LOGERR_CTX("TODO: tools Linux porting: DagorEdAppWindow::editorHelp");
#endif
}


//==============================================================================
void DagorEdAppWindow::pluginHelp(const char *url) const
{
#if _TARGET_PC_WIN // TODO: tools Linux porting: DagorEdAppWindow::pluginHelp
  if (!url || !*url)
    return;

  String help(512, "%s/daeditor3.chm::%s", sgg::get_exe_path_full(), url);

  ::HtmlHelp(NULL, help.str(), HH_DISPLAY_TOPIC, 0);
#else
  LOGERR_CTX("TODO: tools Linux porting: DagorEdAppWindow::pluginHelp");
#endif
}


//==============================================================================
void DagorEdAppWindow::getDocTitleText(String &text)
{
  unsigned drv = d3d::get_driver_code().asFourCC();
  if (sceneFname[0])
    text.printf(300, "DaEditorX  [%c%c%c%c]  - %s", _DUMP4C(drv), sceneFname);
  else
    text.printf(300, "%s  [%c%c%c%c]", IDagorEd2Engine::getBuildVersion(), _DUMP4C(drv));
}


bool DagorEdAppWindow::canCloseScene(const char *title)
{
  if (!sceneFname[0] || on_batch_exit)
    return true;

  int ret;
  if (mNeedSuppress && mMsgBoxResult != -1)
    ret = mMsgBoxResult;
  else
    ret = wingw::message_box(wingw::MBS_YESNOCANCEL, title, "Project changed:\n\"%s\"\n\nSave changes?", sceneFname);

  if (ret == wingw::MB_ID_CANCEL)
    return false;

  if (ret == wingw::MB_ID_YES)
    onMenuItemClick(CM_FILE_SAVE);

  if (sceneFname[0])
    autoSaveProject(sceneFname);

  switchToPlugin(-1);

  return true;
}


bool DagorEdAppWindow::handleNewProject(bool edit)
{
  ec_set_busy(false);
  if (!canCloseScene("Create new project"))
    return false;
  if (dgs_fatal_handler == DagorEdAppWindow::gracefulFatalExit)
    dgs_fatal_handler = prev_fatal_handler;

  NewProjectDialog dlg(mManager->getMainWindow(), "New project", "Project name", "Project will be created in:");
  String location(wsp->getLevelsDir());

  if (!location.length())
    location = sgg::get_exe_path_full();

  dlg.setLocation(location);
  dlg.setName("");
  if (dlg.showDialog() != PropPanel::DIALOG_ID_OK)
    return false;

  ec_set_busy(true);
  String fileName(260, "%s/%s/%s" LEVEL_FILE_EXTENSION, dlg.getLocation(), dlg.getName(), dlg.getName());

  ::simplify_fname(fileName);

  if (!canCreateNewProject(fileName))
    return false;

  strcpy(sceneFname, fileName);

  if (createNewProject(sceneFname))
  {
    grid.resetToDefault();
    addToRecentList(sceneFname);
  }
  else
  {
    sceneFname[0] = 0;
    return false;
  }

  String projectDir(260, "%s/%s/", dlg.getLocation(), dlg.getName());

  DagorEdAppWindow::setDocTitle();

  return true;
}


bool DagorEdAppWindow::handleOpenProject(bool edit)
{
  if (!canCloseScene("Open project"))
    return false;

  String result;
  if (mNeedSuppress)
    result = mSuppressDlgResult;
  else
    result = wingw::file_open_dlg(mManager->getMainWindow(), "Open location to edit...", "Level BLK|*" LEVEL_FILE_EXTENSION,
      LEVEL_FILE_EXTENSION_WO_DOT, wsp->getLevelsDir());

  if (result.empty())
    return false;

  String fn(result);
  simplify_fname(fn);

  strcpy(sceneFname, fn);
  addToRecentList(sceneFname);

  bool success = loadProject(sceneFname);
  if (success)
  {
    DagorEdAppWindow::setDocTitle();
  }
  queryEditorInterface<IDynRenderService>()->enableRender(!d3d::is_stub_driver());

  return success;
}


bool DagorEdAppWindow::canCreateNewProject(const char *filename)
{
  if (::dd_file_exist(filename))
  {
    wingw::message_box(wingw::MBS_EXCL, "New Project", "Project already exists in specified location:\n%s", (const char *)filename);

    return false;
  }
  // get dir containing project's dir
  const char *end = filename + strlen(filename) - 1;

  while (end > filename && !(*end == '\\' || *end == '/'))
    --end;

  if (end <= filename)
    return false;

  --end;
  while (end > filename && !(*end == '\\' || *end == '/'))
    --end;

  if (end < filename)
    return false;

  return true;
}


bool DagorEdAppWindow::createNewProject(const char *filename)
{
  String basePath, projectName;
  splitProjectFilename(filename, basePath, projectName);


  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i].p)
    {
      debug("clearing %s...", plugin[i].p->getInternalName());
      plugin[i].p->clearObjects();
    }
  undoSystem->clear();

  DataBlock appblk(String(260, "%s/application.blk", wsp->getAppDir()));
  ged.load(*appblk.getBlockByNameEx("defProjectLocal"));

  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i].p)
    {
      const char *name = plugin[i].p->getInternalName();
      if (plugin[i].p->acceptSaveLoad())
        dd_mkdir(String(256, "%s/%s", (const char *)basePath, name));

      plugin[i].p->onNewProject();
    }

  //== reset editor project settings
  lastUniqueId = 1;

  Tab<String> enColliders(tmpmem);
  wsp->getEnabledColliders(enColliders);

  const int colCnt = ::get_custom_colliders_count();

  for (int i = 0; i < colCnt; ++i)
  {
    const IDagorEdCustomCollider *collider = ::get_custom_collider(i);

    if (collider)
    {
      const char *colName = collider->getColliderName();
      for (int j = 0; j < enColliders.size(); j++)
        if (strcmp(enColliders[j], colName) == 0)
        {
          activeColliders.push_back() = colName;
          break;
        }
    }
  }

  return saveProject(filename);
}


void DagorEdAppWindow::splitProjectFilename(const char *filename, String &path, String &name)
{
  path = filename;

  int i;
  for (i = path.length() - 1; i >= 0; --i)
    if (path[i] == '/' || path[i] == '\\')
      break;

  name = &path[i + 1];

  if (i < 0)
    ++i;
  safe_erase_items(path, i, path.length() - i);
}


//==============================================================================
bool DagorEdAppWindow::loadWorkspace(const char *wsp_name)
{
  if (!wsp_name || !*wsp_name)
    return false;

  if (!wsp->load(wsp_name))
    return false;

  return true;
}

extern bool de3_default_fatal_handler(const char *msg, const char *call_stack, const char *file, int line);
bool DagorEdAppWindow::gracefulFatalExit(const char *msg, const char *call_stack, const char *file, int line)
{
  send_event_error(msg, call_stack);

  static bool processing = false;
  static int last_save_time = -1;
  static bool processing_prev_fatal = false;
  if (!is_main_thread())
    return de3_default_fatal_handler(msg, call_stack, file, line);

  if (processing)
  {
    if (processing_prev_fatal)
      return true;
    processing_prev_fatal = true;
    bool ret = prev_fatal_handler ? prev_fatal_handler(msg, call_stack, file, line) : true;
    processing_prev_fatal = false;
    if (get_time_msec() > last_save_time + 10000)
      processing = false;
    return ret;
  }
  processing = true;

  if (DAGORED2)
  {
    if (wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNO, "Editor is about to crash",
          "Editor is going to crash on fatal error\nDo you want to SAVE YOUR PROJECT?") == wingw::MB_ID_YES)
    {
      ((DagorEdAppWindow *)DAGORED2)->onMenuItemClick(CM_FILE_SAVE); // save immediately
      DAGORED2->spawnEvent(HUID_AutoSaveDueToCrash, NULL);
      ((DagorEdAppWindow *)DAGORED2)->onMenuItemClick(CM_FILE_SAVE); // save once more to handle changes done by plugins during event
                                                                     // catch
      ((DagorEdAppWindow *)DAGORED2)->autoSaveProject(DAGORED2->getSceneFileName());
    }
    last_save_time = get_time_msec();
  }
  processing_prev_fatal = true;
  bool ret = prev_fatal_handler ? prev_fatal_handler(msg, call_stack, file, line) : true;
  processing_prev_fatal = false;
  return ret;
}

//==============================================================================
bool DagorEdAppWindow::loadProject(const char *filename)
{
  bool real_3d = !d3d::is_stub_driver();
  if (!real_3d)
  {
    ::dgs_dont_use_cpu_in_background = false;
    dagor_enable_idle_priority(false);
  }
  queryEditorInterface<IDynRenderService>()->enableRender(false);
  undoSystem->clear();

  CoolConsole &con = DAGORED2->getConsole();
  con.startProgress();
  con.setTotal(plugin.size() + 2);
  con.setActionDesc("Clearing scene...");

  String basePath, projectName;
  String lastplugin;
  splitProjectFilename(filename, basePath, projectName);

  resetModelessWindows(ModelessWindowResetReason::LoadingProject);
  set_colliders_to_default_state();

  int i;
  for (i = 0; i < plugin.size(); ++i)
    if (plugin[i].p)
    {
      debug("clearing %s...", plugin[i].p->getInternalName());
      plugin[i].p->clearObjects();
    }
  undoSystem->clear();

  DataBlock blk, b1, b2;
  DataBlock localBlk;

  {
    String localSetPath(filename);
    String localSetFname = ::get_file_name_wo_ext(filename);

    ::location_from_path(localSetPath);

    localSetPath = ::make_full_path(localSetPath, localSetFname + ".local.blk");

    localBlk.load(localSetPath);
  }

  if (!blk.load(filename))
  {
    DEBUG_CTX("Error loading %s", filename);
    queryEditorInterface<IDynRenderService>()->enableRender(real_3d);
    return false;
  }

  if (localBlk.blockCount() + localBlk.paramCount() == 0)
  {
    DataBlock appblk(String(260, "%s/application.blk", wsp->getAppDir()));
    ged.load(*appblk.getBlockByNameEx("defProjectLocal"));
  }
  else
    ged.load(localBlk);

  if (IGenViewportWnd *viewport = EDITORCORE->getViewport(0))
    ScreenshotMetaInfoLoader::applyCameraSettings(screenshotMetaInfoToApply, *viewport);

  float new_z_far = getMaxTraceDistance();
  for (int i = 0; i < getViewportCount(); ++i)
  {
    ViewportWindow *vport = ged.getViewport(i);
    if (!vport->isVisible())
      continue;
    float zn, zf;
    vport->getZnearZfar(zn, zf);
    if (new_z_far > zf)
      vport->setZnearZfar(zn, new_z_far, true);
  }

  //::appWnd.checkMenuRadioItem(CM_VIEWPORT_LAYOUT_4, CM_VIEWPORT_LAYOUT_1, ged.viewportLayout);
  int layout_id = ged.getViewportLayout();
  if ((layout_id >= CM_VIEWPORT_LAYOUT_4) && (layout_id <= CM_VIEWPORT_LAYOUT_1))
    getMainMenu()->click(layout_id);

  // adjustNonClient();
  updateViewports();

  lastUniqueId = blk.getInt("lastUniqueId", 1);
  lastplugin = localBlk.getStr("lastPlugin", "");
  grid.load(localBlk);
  mToolPanel->setBool(CM_VIEW_GRID_MOVE_SNAP, grid.getMoveSnap());
  mToolPanel->setBool(CM_VIEW_GRID_ANGLE_SNAP, grid.getRotateSnap());
  mToolPanel->setBool(CM_VIEW_GRID_SCALE_SNAP, grid.getScaleSnap());

  E3DCOLOR color;
  real mult;

  GeomObject::getSkyLight(color, mult);
  color = blk.getE3dcolor("ambient_light_color", color);
  mult = blk.getReal("ambient_light_mult", mult);
  GeomObject::setAmbientLight(color, mult, 0, 0);

  GeomObject::getDirectLight(color, mult);
  color = blk.getE3dcolor("direct_light_color", color);
  mult = blk.getReal("direct_light_mult", mult);
  GeomObject::setDirectLight(color, mult);

  real zenith, azimuth;
  GeomObject::getLightAngles(zenith, azimuth);

  zenith = blk.getReal("light_direction_alpha", zenith);
  azimuth = blk.getReal("light_direction_beta", azimuth);

  GeomObject::setLightAngles(zenith, azimuth);

  useDirectLight = blk.getBool("use_direct_light", useDirectLight);
  ::dgs_tex_quality = blk.getInt("tex_quality", ::dgs_tex_quality);

  for (int i = 0; i < exportPaths.size(); i++)
  {
    exportPaths[i].path = blk.getStr(String(32, "exportPath%s", wsp->getPlatformNameFromId(exportPaths[i].platfId)), "");

    String fullPath(wsp->getGameDir());
    fullPath += "/" + exportPaths[i].path;
    dd_simplify_fname_c(fullPath.str());
    if (dd_file_exist(fullPath))
      exportPaths[i].path = fullPath;
    else
      exportPaths[i].path = "";
  }

  const DataBlock *redirBlk = blk.getBlockByNameEx("pluginsRedirect");
  redirectPathTo = redirBlk->getStr("referTo", NULL);
  for (int i = 0; i < redirBlk->paramCount(); i++)
    if (redirBlk->getParamType(i) == DataBlock::TYPE_BOOL)
    {
      IGenEditorPlugin *p = getPluginByName(redirBlk->getParamName(i));
      if (!p)
        DAEDITOR3.conWarning("unrecohnized internal plugin name <%s> in <%s> block", redirBlk->getParamName(i),
          redirBlk->getBlockName());
      else if (redirBlk->getBool(i))
        redirectPluginList.addPtr(p);
    }

  const DataBlock *exportBlk = blk.getBlockByName("pluginsExport");
  const DataBlock *extBlk = blk.getBlockByName("pluginsExternal");
  const DataBlock *extSourceBlk = blk.getBlockByName("externalSource");
  const DataBlock *extPathesBlk = blk.getBlockByNameEx("externalPathes");

  for (i = 0; i < getPluginCount(); i++)
    plugin[i].data->externalPath = extPathesBlk->getStr(plugin[i].p->getInternalName(), "");

  String blk_fn(260, "%s/../.local/de3_editor.blk", sgg::get_exe_path_full());
  DataBlock editorBlk;
  if (dd_file_exist(blk_fn))
    editorBlk.load(blk_fn);
  const DataBlock *hkBlk = editorBlk.getBlockByName("plugins_hotkeys");

  loadViewportsParams(localBlk);
  con.loadCmdHistory(editorBlk);
  dagor_idle_cycle();

  animateViewports = localBlk.getBool("animateViewports", false);
  ged.setViewportCacheMode(animateViewports ? IDagorEd2Engine::VCM_NoCacheAll : IDagorEd2Engine::VCM_CacheAll);
  getMainMenu()->setCheckById(CM_ANIMATE_VIEWPORTS, animateViewports);

  bool show_compass = localBlk.getBool("showCompass", true);
  static_cast<DagorEdAppEventHandler *>(appEH)->showCompass = show_compass;
  getMainMenu()->setCheckById(CM_NAV_COMPASS, show_compass);
  EDITORCORE->getCustomPanel(GUI_MAIN_TOOLBAR_ID)->setBool(CM_NAV_COMPASS, show_compass);

  shouldUseOccluders = localBlk.getBool("useOccludersAndPortals", false);

  getMainMenu()->setCheckById(CM_USE_OCCLUDERS, shouldUseOccluders);
  getMainMenu()->setCheckById(CM_OPTIONS_ENABLE_DEVELOPER_TOOLS, developerToolsEnabled);
  getMainMenu()->setEnabledById(CM_VIEW_DEVELOPER_TOOLS, developerToolsEnabled);
  const bool moveCopyToSubmenu = AssetSelectorGlobalState::getMoveCopyToSubmenu();
  getMainMenu()->setCheckById(CM_OPTIONS_PREFERENCES_ASSET_TREE_COPY_SUBMENU, moveCopyToSubmenu);

  const DataBlock *disCollidersBlk = blk.getBlockByName("disabled_colliders");

  if (disCollidersBlk)
  {
    for (i = 0; i < disCollidersBlk->paramCount(); ++i)
    {
      const char *collName = disCollidersBlk->getStr(i);
      disable_custom_collider(collName);
    }
  }
  getEnabledColliders(activeColliders);

  DataBlock *localPlugData = localBlk.getBlockByName("local_plugin_data");
  DataBlock *exportFltrBlk = blk.getBlockByName("filters_export");
  if (!exportFltrBlk)
    exportFltrBlk = localBlk.getBlockByName("filters_export");

  con.incDone();

  for (i = 0; i < plugin.size(); ++i, con.incDone())
  {
    if (!plugin[i].p)
      continue;

    const char *name = plugin[i].p->getInternalName();
    const char *menuName = plugin[i].p->getMenuCommandName();

    DataBlock *plugLocalBlk = localPlugData ? localPlugData->getBlockByName(name) : NULL;
    String path(256, "%s/%s/", (const char *)basePath, name);

    DAGOR_TRY
    {
      if (plugLocalBlk)
        plugin[i].p->setVisible(d3d::get_driver_code().is(d3d::stub) ? false : plugLocalBlk->getBool("visible", true));
    }
    DAGOR_CATCH(...)
    {
      console->addMessage(ILogWriter::ERROR, "Plugin \"%s\" loading failed, plugin disabled.", menuName);
      wingw::message_box(wingw::MBS_EXCL, "Plugin fatal error", "Plugin \"%s\" loading failed, plugin disabled.", menuName);
      debug("Plugin \"%s\" loading failed, plugin disabled.", menuName);

      DataBlock::setRootIncludeResolver(wsp->getAppDir()); // reset DataBlock::setIncludeResolver() that may be installed in plugin to
                                                           // be removed
      unregisterPlugin(plugin[i].p);
      --i;

      continue;
    }

    if (exportBlk)
      plugin[i].data->doExport = exportBlk->getBool(name, true);

    if (exportFltrBlk)
      plugin[i].data->doExportFilter = exportFltrBlk->getBool(name, true);

    if (extBlk)
      plugin[i].data->externalExport = extBlk->getBool(name, false);

    if (extSourceBlk)
      plugin[i].data->externalSource = extSourceBlk->getBool(name, false);

    if (plugin[i].data->externalSource && plugin[i].data->externalPath.length())
    {
      String dir, fname;
      ::split_path(plugin[i].data->externalPath, dir, fname);
      path = ::make_full_path(::make_full_path(de_get_sdk_dir(), dir), name) + "/";
    }

    if (plugin[i].p->acceptSaveLoad())
    {
      con.setActionDesc("Loading data: %s", menuName);

      if (!::dd_dir_exist(path))
        ::dd_mkdir(path);

      String plugin_fn(0, "%s.plugin.blk", getPluginFilePath(plugin[i].p, name));
      DataBlock blk;
      if (dd_file_exists(plugin_fn) && !blk.load(plugin_fn))
      {
        wingw::message_box(wingw::MBS_EXCL, "Fatal error", "DaEditorX failed to start:\n%s is broken", plugin_fn);
        quit_game(-1);
      }

      DataBlock &locBlk = plugLocalBlk ? *plugLocalBlk : blk;

      DAGOR_TRY
      {
        if (!plugLocalBlk)
          plugin[i].p->setVisible(d3d::get_driver_code().is(d3d::stub) ? false : locBlk.getBool("visible", true));
        plugin[i].p->loadObjects(blk, locBlk, getPluginFilePath(plugin[i].p, "."));
      }
      DAGOR_CATCH(...)
      {
        console->addMessage(ILogWriter::ERROR, "Plugin \"%s\" loading failed, plugin disabled.", menuName);
        if (!mNeedSuppress)
          wingw::message_box(wingw::MBS_EXCL, "Plugin fatal error", "Plugin \"%s\" loading failed, plugin disabled.", menuName);
        debug("Plugin \"%s\" loading failed, plugin disabled.", menuName);

        DataBlock::setRootIncludeResolver(wsp->getAppDir()); // reset DataBlock::setIncludeResolver() that may be installed in plugin
                                                             // to be removed
        unregisterPlugin(plugin[i].p);
        --i;

        continue;
      }
    }
    if (hkBlk)
      plugin[i].data->hotkey = hkBlk->getInt(name, -1);
  }
  environment::load_settings(localBlk);

  String fname(260, "%sapplication.blk", wsp->getAppDir());
  DataBlock appBlk(fname);
  queryEditorInterface<IDynRenderService>()->reloadCharacterMicroDetails(*appBlk.getBlockByNameEx("dynamicDeferred"), localBlk);

  con.setActionDesc("Preparing to show...");

  // activate last used plugin
  int plugId = -1;

  for (i = 0; i < plugin.size(); ++i)
    if (plugin[i].p && !strcmp(plugin[i].p->getInternalName(), lastplugin))
    {
      plugId = i;
      break;
    }

  con.incDone();
  con.endProgress();
  dagor_idle_cycle();

  updateUseOccluders();
  updateViewports();

  // if Editor have to use lighting from Light plugin
  ISceneLightService *srv = EDITORCORE->queryEditorInterface<ISceneLightService>();
  if (srv && !useDirectLight)
    srv->applyLightingToGeometry();

  // apply lighting settings from project on all static geometry
  con.startProgress();
  GeomObject::recompileAllGeomObjects(&con);

  ICollision *collision = getInterfaceEx<ICollision>();

  if (collision)
    collision->manageCustomColliders();

  // notify all plugins about loading finish
  spawnEvent(HUID_BeforeMainLoop, nullptr);
  for (i = 0; i < plugin.size(); ++i)
    if (plugin[i].p)
    {
      debug("before main loop of %s", plugin[i].p->getMenuCommandName());
      DAGOR_TRY { plugin[i].p->beforeMainLoop(); }
      DAGOR_CATCH(...)
      {
        console->addMessage(ILogWriter::ERROR, "Plugin \"%s\" failed during beforeMainLoop()", plugin[i].p->getMenuCommandName());
      }
    }

  switchToPlugin(plugId);
  ged.activateCurrentViewport();
  con.endProgress();
  DEBUG_CP();

  // perform initial pre-frame-render update
  {
    d3d::GpuAutoLock gpuLock;
    beforeRenderObjects();
  }
  DEBUG_CP();

  repaint();
  force_screen_redraw();
  dagor_idle_cycle();
  DEBUG_CP();

  prev_fatal_handler = dgs_fatal_handler;
  dgs_fatal_handler = DagorEdAppWindow::gracefulFatalExit;

  return true;
}


void DagorEdAppWindow::loadProjectFromEditor(const char *path)
{
  const bool oldBusy = ec_set_busy(true);
  CoolConsole &con = DAGORED2->getConsole();
  con.showConsole();

  strcpy(sceneFname, path);
  addToRecentList(sceneFname);

  startup_editor_core_select_startup_scene(); // Show the full screen console as the loading screen.
  loadProject(sceneFname);
  queryEditorInterface<IDynRenderService>()->enableRender(!d3d::is_stub_driver()); // loadProject disables rendering.
  queryEditorInterface<IDynRenderService>()->selectAsGameScene();
  setDocTitle();
  spawnEvent(HUID_AfterProjectLoad, nullptr);

  repaint();
  force_screen_redraw();
  dagor_idle_cycle();

  con.hideConsole();
  ec_set_busy(oldBusy);
}


void DagorEdAppWindow::prepareLocalLevelBlk(const char *filename, String &localSetPath, DataBlock &localBlk)
{
  {
    String localSetDir(filename);
    ::location_from_path(localSetDir);

    String localSetFname = ::get_file_name_wo_ext(filename) + ".local.blk";
    localSetPath = ::make_full_path(localSetDir, localSetFname);
  }

  localBlk.load(localSetPath);

  localBlk.setStr("noteForCuriousHumanBeings", "HANDS OFF!!!  This file and those in sub-folders here are used by "
                                               "DagorEditor, don't touch them!!!");
}


//==============================================================================
bool DagorEdAppWindow::saveProject(const char *filename)
{
  int i;
  String basePath, projectName;
  splitProjectFilename(filename, basePath, projectName);

  bool ok = true;

  CoolConsole &con = DAGORED2->getConsole();
  con.startProgress();
  con.setTotal(plugin.size() + 1);
  con.setActionDesc("Saving editor data...");

  String localSetPath;
  DataBlock localBlk;
  prepareLocalLevelBlk(filename, localSetPath, localBlk);

  {
    environment::save_settings(localBlk);

    DataBlock blk;

    blk.setStr("noteForCuriousHumanBeings", "HANDS OFF!!!  This file and those in sub-folders here are used by "
                                            "DagorEditor, don't touch them!!!");

    blk.setInt("lastUniqueId", lastUniqueId);

    for (i = 0; i < exportPaths.size(); i++)
      blk.setStr(String(32, "exportPath%s", wsp->getPlatformNameFromId(exportPaths[i].platfId)),
        ::make_path_relative(exportPaths[i].path, wsp->getGameDir()));

    ged.save(localBlk);
    grid.save(localBlk);

    E3DCOLOR color;
    real mult;

    GeomObject::getSkyLight(color, mult);

    blk.setE3dcolor("ambient_light_color", color);
    blk.setReal("ambient_light_mult", mult);

    GeomObject::getDirectLight(color, mult);

    blk.setE3dcolor("direct_light_color", color);
    blk.setReal("direct_light_mult", mult);

    real zenith, azimuth;
    GeomObject::getLightAngles(zenith, azimuth);

    blk.setReal("light_direction_alpha", zenith);
    blk.setReal("light_direction_beta", azimuth);

    blk.setBool("use_direct_light", useDirectLight);
    blk.setInt("tex_quality", ::dgs_tex_quality);

    if (!redirectPathTo.empty())
    {
      DataBlock *redirBlk = blk.addNewBlock("pluginsRedirect");
      redirBlk->setStr("referTo", redirectPathTo);

      for (i = 0; i < plugin.size(); ++i)
        if (plugin[i].p && redirectPluginList.hasPtr(plugin[i].p))
          redirBlk->setBool(plugin[i].p->getInternalName(), true);
    }

    DataBlock *exportBlk = blk.addNewBlock("pluginsExport");
    DataBlock *extBlk = blk.addNewBlock("pluginsExternal");
    DataBlock *extSourceBlk = blk.addNewBlock("externalSource");
    DataBlock *extPathesBlk = blk.addNewBlock("externalPathes");

    for (i = 0; i < plugin.size(); ++i)
    {
      if (!plugin[i].p)
        continue;

      const char *name = plugin[i].p->getInternalName();
      exportBlk->addBool(name, plugin[i].data->doExport);
      extBlk->addBool(name, plugin[i].data->externalExport);
      extSourceBlk->addBool(name, plugin[i].data->externalSource);
      extPathesBlk->setStr(plugin[i].p->getInternalName(), plugin[i].data->externalPath);
    }

    saveViewportsParams(localBlk);

    localBlk.setBool("animateViewports", animateViewports);
    localBlk.setBool("useOccludersAndPortals", shouldUseOccluders);
    localBlk.setBool("use_dyn_lighting", GeomObject::useDynLighting);

    DataBlock *disCollidersBlk = blk.addNewBlock("disabled_colliders");
    restoreEditorColliders();

    if (disCollidersBlk)
    {
      FastNameMap map;
      for (i = 0; i < ::get_custom_colliders_count(); ++i)
      {
        IDagorEdCustomCollider *coll = ::get_custom_collider(i);
        const bool enabled = ::is_custom_collider_enabled(coll);
        if (!enabled)
          map.addNameId(coll->getColliderName());
      }
      for (int i = 0; i < getPluginCount(); ++i)
      {
        IGenEditorPlugin *plugin = getPlugin(i);
        IObjEntityFilter *filter = plugin->queryInterface<IObjEntityFilter>();

        if (filter && filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION))
          if (!filter->isFilteringActive(IObjEntityFilter::STMASK_TYPE_COLLISION))
            map.addNameId(plugin->getMenuCommandName());
      }

      iterate_names(map, [&](int, const char *name) { disCollidersBlk->addStr("name", name); });
    }

    DataBlock &exportFltrBlk = *blk.addBlock("filters_export");
    for (i = 0; i < plugin.size(); ++i)
      if (plugin[i].p)
      {
        IObjEntityFilter *filter = plugin[i].p->queryInterface<IObjEntityFilter>();
        if (filter && filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_EXPORT))
          exportFltrBlk.setBool(plugin[i].p->getInternalName(), plugin[i].data->doExportFilter);
      }

    dd_mkdir(basePath);

    if (!blk.saveToTextFile(filename))
    {
      console->addMessage(ILogWriter::FATAL, "Error saving \"%s\"", (const char *)filename);
      ok = false;
    }
  }

  con.incDone();

  if (!ok)
  {
    con.endProgress();
    return false;
  }

  DataBlock &localPlugData = *localBlk.addBlock("local_plugin_data");
  localBlk.removeBlock("filters_export");

  for (i = 0; i < plugin.size(); ++i, con.incDone())
  {
    if (!plugin[i].p)
      continue;

    if (!plugin[i].data->externalSource || plugin[i].data->externalPath == "")
    {
      const char *name = plugin[i].p->getInternalName();
      DataBlock &locBlk = *localPlugData.addBlock(name);

      locBlk.clearData();
      locBlk.setBool("visible", plugin[i].p->getVisible());

      if (plugin[i].p->acceptSaveLoad())
      {
        con.setActionDesc(String(1024, "Saving data: %s", plugin[i].p->getMenuCommandName()));

        dd_mkdir(String(256, "%s/%s", (const char *)basePath, name));

        String path(256, "%s/%s/", (const char *)basePath, name);

        DataBlock blk;

        plugin[i].p->saveObjects(blk, locBlk, getPluginFilePath(plugin[i].p, "."));

        String blkFilename(getPluginFilePath(plugin[i].p, name) + ".plugin.blk");

        if (!blk.saveToTextFile(blkFilename))
        {
          con.addMessage(ILogWriter::FATAL, "Error saving \"%s\"", (const char *)blkFilename);
          ok = false;
        }
      }
    }
  }

  if (d3d::get_driver_code().is(!d3d::stub))
  {
    String localSetDir(filename);
    ::location_from_path(localSetDir);

    String localSetFname = ::get_file_name_wo_ext(filename) + ".local.blk";
    String localSetPath = ::make_full_path(localSetDir, localSetFname);

    if (!localBlk.saveToTextFile(localSetPath))
    {
      console->addMessage(ILogWriter::FATAL, "Error saving \"%s\"", (const char *)localSetPath);
      ok = false;
    }

    file_ptr_t cvsignore = ::df_open(::make_full_path(localSetDir, ".cvsignore"), DF_RDWR);
    if (!cvsignore)
      cvsignore = ::df_open(::make_full_path(localSetDir, ".cvsignore"), DF_WRITE);

    if (cvsignore)
    {
      char buf[1024];
      bool str_found = false;
      memset(buf, 0, sizeof(buf));
      while (df_gets(buf, sizeof(buf) - 1, cvsignore))
        if (strnicmp(buf, localSetFname, localSetFname.length()) == 0)
          if (buf[localSetFname.length()] == '\0' || strchr("\n\r ", buf[localSetFname.length()]))
            str_found = true;

      if (!str_found)
      {
        if (!strchr("\n\r", buf[strlen(buf) - 1]))
          ::df_write(cvsignore, "\n", 1);
        ::df_write(cvsignore, localSetFname, localSetFname.length());
      }
      ::df_close(cvsignore);
    }
    else
      console->addMessage(ILogWriter::WARNING, "Error creating \".cvsignore\" file");
  }

  con.endProgress();

  return ok;
}


void DagorEdAppWindow::autoSaveProject(const char *filename)
{
  String localSetPath;
  DataBlock localBlk;
  prepareLocalLevelBlk(filename, localSetPath, localBlk);

  DataBlock editorBlk;
  DataBlock *hkBlk = editorBlk.addNewBlock("plugins_hotkeys");

  localBlk.setStr("lastPlugin", curPluginId == -1 ? "" : plugin[curPluginId].p->getInternalName());

  DataBlock &localPlugData = *localBlk.addBlock("local_plugin_data");

  bool isSaved = false;
  for (int i = 0; i < plugin.size(); ++i)
  {
    if (!plugin[i].p)
      continue;

    hkBlk->addInt(plugin[i].p->getInternalName(), plugin[i].data->hotkey);
    if (!plugin[i].p->acceptSaveLoad())
      continue;

    IPluginAutoSave *autoSave = plugin[i].p->queryInterface<IPluginAutoSave>();
    if (!autoSave)
      continue;

    const char *name = plugin[i].p->getInternalName();
    DataBlock &locBlk = *localPlugData.addBlock(name);

    autoSave->autoSaveObjects(locBlk);
    isSaved = true;
  }

  if (isSaved && d3d::get_driver_code().is(!d3d::stub))
    localBlk.saveToTextFile(localSetPath);

  DAGORED2->getConsole().saveCmdHistory(editorBlk);
  String pathName(260, "%s/../.local/de3_editor.blk", sgg::get_exe_path_full());
  if (!editorBlk.saveToTextFile(pathName))
    logerr("error saving '%s'", pathName.str());
}


//==============================================================================
ICollision *DagorEdAppWindow::getCollision()
{
  if (!collision)
  {
    ICollision *tc = NULL;

    for (int i = 0; i < getPluginCount(); ++i)
    {
      IGenEditorPlugin *plugin = getPlugin(i);

      if (plugin)
        tc = plugin->queryInterface<ICollision>();

      if (tc)
      {
        collision = tc;
        break;
      }
    }
  }

  G_ASSERT(collision);

  return collision;
}


//==============================================================================

void DagorEdAppWindow::showSelectWindow(IObjectsList *obj_list, const char *obj_list_owner_name)
{
  SelWindow selWnd(0, obj_list, obj_list_owner_name);

  if (selWnd.showDialog() == PropPanel::DIALOG_ID_OK)
  {
    Tab<String> names(tmpmem);
    selWnd.getSelectedNames(names);

    obj_list->onSelectedNames(names);
  }
}

//==============================================================================
void DagorEdAppWindow::zoomAndCenter()
{
  IGenEditorPlugin *current = curPlugin();
  ViewportWindow *curVP = NULL;
  BBox3 bounds;

  if (current && current->getSelectionBox(bounds))
  {
    curVP = ged.getActiveViewport();

    if (curVP)
      curVP->zoomAndCenter(bounds);
    else
      for (int i = 0; i < ged.getViewportCount(); ++i)
        ged.getViewport(i)->zoomAndCenter(bounds);
  }
}


//==============================================================================
void DagorEdAppWindow::repaint()
{
  updateViewports();
  invalidateViewportCache();
}


//==============================================================================
void DagorEdAppWindow::correctCursorInSurfMove(const Point3 &delta) { gizmoEH->correctCursorInSurfMove(delta); }


//==============================================================================

void DagorEdAppWindow::updateUndoRedoMenu()
{
  if (gizmoEH->isStarted())
    return;

  String caption;
  const char *undoName = undoSystem->get_undo_name(0);
  bool enable = true;

  if (undoName)
    caption = String(128, "Undo \"%s\"\tCtrl+Z", undoName);
  else
  {
    enable = false;
    caption = "Undo\tCtrl+Z";
  }

  getMainMenu()->setEnabledById(CM_UNDO, enable);
  getMainMenu()->setCaptionById(CM_UNDO, caption);

  const char *redoName = undoSystem->get_redo_name(0);

  if (redoName)
  {
    enable = true;
    caption = String(128, "Redo \"%s\"\tCtrl+Y", redoName);
  }
  else
  {
    enable = false;
    caption = "Redo\tCtrl+Y";
  }

  getMainMenu()->setEnabledById(CM_REDO, enable);
  getMainMenu()->setCaptionById(CM_REDO, caption);
}


void DagorEdAppWindow::updateUseOccluders()
{
  del_it(occlusion_map);

  if (shouldUseOccluders)
    if (IGenEditorPlugin *occlPlugin = getPluginByName("occluders"))
    {
      IBinaryDataBuilder *b = occlPlugin->queryInterface<IBinaryDataBuilder>();
      if (b)
      {
        getConsole().addMessage(ILogWriter::NOTE, "found occluder build interface");
        mkbindump::BinDumpSaveCB cwr(1 << 20, _MAKE4C('PC'), false);

        TextureRemapHelper trh(cwr.getTarget());
        b->validateBuild(cwr.getTarget(), getConsole(), NULL);
        b->buildAndWrite(cwr, trh, NULL);

        MemoryLoadCB crd(cwr.getRawWriter().getMem(), false);
        while (crd.tell() < cwr.getSize())
        {
          switch (crd.beginTaggedBlock())
          {
            case _MAKE4C('OCCL'):
              if (crd.readInt() == 0x20080514)
              {
                if (!occlusion_map)
                  occlusion_map = new OcclusionMap();
                occlusion_map->init(crd);
              }
              break;
          }
          crd.endBlock();
        }
      }
    }

  invalidateViewportCache();
}

void DagorEdAppWindow::setShowMessageAt(int x, int y, const SimpleString &msg)
{
  msgX = x;
  msgY = y;
  messageAt = msg;
}

void DagorEdAppWindow::showMessageAt()
{
  if (messageAt.empty())
    return;
  StdGuiRender::set_font(0);
  Point2 ts = StdGuiRender::get_str_bbox(messageAt.c_str()).size();
  int ascent = StdGuiRender::get_font_ascent();
  int descent = StdGuiRender::get_font_descent();
  int fontHeight = ascent + descent;
  int padding = 2;
  int width = ts.x;
  int height = padding + fontHeight + padding;
  int left = msgX;
  int top = msgY - height;
  StdGuiRender::set_color(COLOR_BLACK);
  StdGuiRender::render_box(left, top, left + width, top + height);

  StdGuiRender::set_color(COLOR_WHITE);
  StdGuiRender::draw_strf_to(left + (width - ts.x) / 2, top + padding + ascent, messageAt.c_str());
};
//==================================================================================================
Point3 DagorEdAppWindow::snapToGrid(const Point3 &p) const { return grid.snapToGrid(p); }


//==================================================================================================
Point3 DagorEdAppWindow::snapToAngle(const Point3 &p) const { return grid.snapToAngle(p); }


//==================================================================================================
Point3 DagorEdAppWindow::snapToScale(const Point3 &p) const { return grid.snapToScale(p); }


//==================================================================================================
String DagorEdAppWindow::getScreenshotNameMask(bool cube) const
{
  return ::make_full_path(getSdkDir(),
    String(0, "de_%s_%%03i%s", cube ? "cube" : "scrn", getFileExtFromFormat(cube ? cubeScreenshotCfg.format : screenshotCfg.format)));
}

//==================================================================================================
void DagorEdAppWindow::setAnimateViewports(bool animate)
{
  /*
  animateViewports = animate;
  ged.setViewportCacheMode(animateViewports ?
    IDagorEd2Engine::VCM_NoCacheAll : IDagorEd2Engine::VCM_CacheAll);

  ::appWnd.checkMenuItem(CM_ANIMATE_VIEWPORTS, animateViewports);
  */
}

void DagorEdAppWindow::onImguiDelayedCallback(void *user_data)
{
  const unsigned commandId = (unsigned)(uintptr_t)user_data;

  if ((commandId & DELAYED_CALLBACK_VIEWPORT_COMMAND_BIT) == 0)
  {
    onMenuItemClick(commandId);
  }
  else
  {
    ViewportWindow *viewport = ged.getCurrentViewport();
    if (!viewport || !viewport->isActive())
      return;

    const unsigned viewportCommandId = commandId & ~DELAYED_CALLBACK_VIEWPORT_COMMAND_BIT;
    if (viewport->handleViewportAcceleratorCommand(viewportCommandId))
      return;

    IGenEditorPlugin *currentPlugin = curPlugin();
    if (currentPlugin)
      currentPlugin->handleViewportAcceleratorCommand(viewportCommandId);
  }
}

void DagorEdAppWindow::renderUIViewport(ViewportWindow &viewport, const Point2 &size, float item_spacing)
{
  ImGui::PushID(&viewport);

  const ImGuiID viewportCanvasId = ImGui::GetCurrentWindow()->GetID("canvas");

  if (viewport.isActive())
  {
    unsigned commandId = mManager->processImguiViewportAccelerator(viewportCanvasId);
    if (commandId != 0)
    {
      G_ASSERT((commandId & DELAYED_CALLBACK_VIEWPORT_COMMAND_BIT) == 0);
      commandId |= DELAYED_CALLBACK_VIEWPORT_COMMAND_BIT;
      PropPanel::request_delayed_callback(*this, (void *)((uintptr_t)commandId));
    }
  }

  viewport.updateImgui(viewportCanvasId, size, item_spacing);

  ImGui::PopID();
}

void DagorEdAppWindow::renderUIViewports()
{
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("Viewport");
  ImGui::PopStyleVar();

  const float itemSpacing = ImGui::GetStyle().ItemSpacing.y; // Use the same spacing in both directions.
  const Point2 regionAvailable = Point2(ImGui::GetContentRegionAvail()) - Point2(itemSpacing, itemSpacing);

  if (ged.getViewportCount() == 1)
  {
    ViewportWindow *viewport0 = ged.getViewport(0);
    if (viewport0)
      renderUIViewport(*viewport0, regionAvailable, itemSpacing);
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

      renderUIViewport(*viewport0, Point2(leftWidth, topHeight), itemSpacing);
      ImGui::SameLine(0.0f, itemSpacing);
      renderUIViewport(*viewport1, Point2(rightWidth, topHeight), itemSpacing);

      renderUIViewport(*viewport2, Point2(leftWidth, bottomHeight), itemSpacing);
      ImGui::SameLine(0.0f, itemSpacing);
      renderUIViewport(*viewport3, Point2(rightWidth, bottomHeight), itemSpacing);
    }
  }

  ImGui::End();
}

void DagorEdAppWindow::renderUI()
{
  unsigned clientWidth = 0;
  unsigned clientHeight = 0;
  getWndManager()->getWindowClientSize(getWndManager()->getMainWindow(), clientWidth, clientHeight);

  // They can be zero when toggling the application's window between minimized and maximized state.
  if (clientWidth == 0 || clientHeight == 0)
    return;

  PropPanel::IMenu *mainMenu = getMainMenu();
  if (mainMenu)
    PropPanel::render_menu(*mainMenu);

  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("Root", nullptr,
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
  ImGui::PopStyleVar(3);

  if (mToolPanel || mTabWindow || mPlugTools)
  {
    PropPanel::pushToolBarColorOverrides();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, PropPanel::Constants::TOOLBAR_WINDOW_PADDING);

    ImGui::BeginChild("Toolbar", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border,
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking);

    ImGui::PopStyleVar();
    PropPanel::popToolBarColorOverrides();

    if (mToolPanel)
      mToolPanel->updateImgui();
    if (mTabWindow)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, PropPanel::getOverriddenColor(PropPanel::ColorOverride::DIALOG_TITLE));
      mTabWindow->updateImgui();
      ImGui::PopStyleColor();
      if (mPlugTools)
        ImGui::NewLine();
    }
    if (mPlugTools)
      mPlugTools->updateImgui();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
    ImGui::EndChild();
    ImGui::PopStyleVar();
  }

  ImGui::BeginChild("DockHolder", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking);

  ImGuiID rootDockSpaceId = ImGui::GetID("RootDockSpace");

  // Create the initial dock layout.
  // If you want to the change the initial dock layout you will likely need to increase LATEST_DOCK_SETTINGS_VERSION.
  if (!dockPositionsInitialized)
  {
    dockPositionsInitialized = true;

    const ImVec2 viewportSize = ImGui::GetMainViewport()->Size;

    ImGui::DockBuilderRemoveNode(rootDockSpaceId);
    ImGui::DockBuilderAddNode(rootDockSpaceId, ImGuiDockNodeFlags_CentralNode);
    ImGui::DockBuilderSetNodeSize(rootDockSpaceId, ImGui::GetMainViewport()->Size);

    ImGuiID dockSpaceLeft, dockSpaceViewport;
    ImGuiID dockSpaceRight4 = ImGui::DockBuilderSplitNode(rootDockSpaceId, ImGuiDir_Right, 0.15f, nullptr, &dockSpaceLeft);
    ImGuiID dockSpaceRight3 = ImGui::DockBuilderSplitNode(dockSpaceLeft, ImGuiDir_Right, 0.25f, nullptr, &dockSpaceLeft);
    ImGuiID dockSpaceRight2 = ImGui::DockBuilderSplitNode(dockSpaceLeft, ImGuiDir_Right, 0.25f, nullptr, &dockSpaceLeft);
    ImGuiID dockSpaceRight1 = ImGui::DockBuilderSplitNode(dockSpaceLeft, ImGuiDir_Right, 0.33f, nullptr, &dockSpaceViewport);
    dockSpaceLeft = ImGui::DockBuilderSplitNode(dockSpaceViewport, ImGuiDir_Left, 0.4f, nullptr, &dockSpaceViewport);

    ImGuiDockNode *viewportNode = ImGui::DockBuilderGetNode(dockSpaceViewport);
    if (viewportNode)
      viewportNode->SetLocalFlags(viewportNode->LocalFlags | ImGuiDockNodeFlags_HiddenTabBar);

    // TODO: ImGui porting: view: extensibility?
    ImGui::DockBuilderDockWindow("###AssetSelectorPanel", dockSpaceLeft);
    ImGui::DockBuilderDockWindow("Viewport", dockSpaceViewport);
    ImGui::DockBuilderDockWindow("Outliner", dockSpaceRight1);
    ImGui::DockBuilderDockWindow("Properties", dockSpaceRight2);
    ImGui::DockBuilderDockWindow("Landscape properties", dockSpaceRight2);
    ImGui::DockBuilderDockWindow("Trigger / Mission Obj. Info", dockSpaceRight3);
    ImGui::DockBuilderDockWindow("Object Properties", dockSpaceRight4);

    ImGui::DockBuilderFinish(rootDockSpaceId);
  }

  // Change the close button's color for the daEditorX Classic style.
  PropPanel::pushDialogTitleBarColorOverrides();
  ImGui::DockSpace(rootDockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
  PropPanel::popDialogTitleBarColorOverrides();

  renderUIViewports();

  IGenEditorPlugin *currentPlugin = curPlugin();
  if (currentPlugin)
    currentPlugin->updateImgui();

  if (console && console->isVisible())
  {
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(viewport->Size.x * 0.45f, viewport->Size.y * 0.15f), ImGuiCond_FirstUseEver);

    const bool canClose = console->isCanClose() && !console->isProgress();
    bool open = true;
    DAEDITOR3.imguiBegin("DaEditorX console", canClose ? &open : nullptr);
    console->updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open)
      console->hide();
  }

  if (control_gallery)
  {
    bool open = true;
    DAEDITOR3.imguiBegin(control_gallery->getPanel(), &open);
    control_gallery->getPanel().updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open)
      control_gallery.reset();
  }

  PropPanel::render_dialogs();

  ImGui::EndChild();

  ImGui::End();
}

void DagorEdAppWindow::beforeUpdateImgui() { mManager->updateImguiMouseCursor(); }

void DagorEdAppWindow::updateImgui()
{
  static bool renderingImgui = false;

  // Most likely cause if this assert fails: a modal dialog is being shown in response to an event handled in an
  // updateImgui. See the notes at the PropPanel::MessageQueue class.
  G_ASSERT(!renderingImgui);
  renderingImgui = true;

  PropPanel::after_new_frame();

  // The keyboard press checking itself cannot be in actObject because actObject could be called more often than
  // renderUI (that calls ImGui's NewFrame that updates ImGui's key states), and onMenuItemClick could fire more than
  // once.

  const unsigned commandId = mManager->processImguiAccelerator();
  if (commandId != 0)
  {
    G_ASSERT((commandId & DELAYED_CALLBACK_VIEWPORT_COMMAND_BIT) == 0);
    PropPanel::request_delayed_callback(*this, (void *)((uintptr_t)commandId));
  }

  if (consoleCommandsAndVariableWindowsVisible && !imgui_window_is_visible("General", "Console commands and variables"))
  {
    consoleCommandsAndVariableWindowsVisible = false;
    getMainMenu()->setCheckById(CM_VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES, consoleCommandsAndVariableWindowsVisible);
  }

  if (consoleWindowVisible != console->isVisible())
  {
    consoleWindowVisible = console->isVisible();
    mToolPanel->setBool(CM_CONSOLE, consoleWindowVisible);
  }

  const bool oldImguiDebugWindowsVisible = imguiDebugWindowsVisible;
  if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_F12))
    imguiDebugWindowsVisible = !imguiDebugWindowsVisible;

  if (imguiDebugWindowsVisible)
    ImGui::ShowMetricsWindow(&imguiDebugWindowsVisible);

  if (imguiDebugWindowsVisible != oldImguiDebugWindowsVisible)
    getMainMenu()->setCheckById(CM_VIEW_DEVELOPER_TOOLS_IMGUI_DEBUGGER, imguiDebugWindowsVisible);

  editor_core_update_imgui_style_editor();

  renderUI();

  PropPanel::before_end_frame();

  renderingImgui = false;
}

const char *daeditor3_get_appblk_fname()
{
  static String fn;
  fn.printf(260, "%s/application.blk", DAGORED2->getWorkspace().getAppDir());
  return fn;
}

REGISTER_IMGUI_WINDOW("General", "Console commands and variables", console::imgui_window);