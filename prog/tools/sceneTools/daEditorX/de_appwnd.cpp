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

#include <de3_interface.h>
#include <de3_lightService.h>
#include <de3_huid.h>
#include <de3_editorEvents.h>
#include <de3_dxpFactory.h>

#include "pluginService/de_IDagorPhysImpl.h"

#include <oldEditor/iClipping.h>


#include <oldEditor/de_cm.h>
#include <oldEditor/de_util.h>
#include <oldEditor/de_common_interface.h>
#include <oldEditor/de_workspace.h>
#include <oldEditor/de_clipping.h>
#include <de3_interface.h>
#include <de3_dynRenderService.h>

#include <propPanel2/comWnd/tool_window.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>
#include <winGuiWrapper/wgw_busy.h>

#include <perfMon/dag_cpuFreq.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_workCycle.h>
#include <gui/dag_baseCursor.h>

#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_gizmofilter.h>
#include <EditorCore/ec_selwindow.h>
#include <EditorCore/ec_newProjDlg.h>
#include <EditorCore/ec_camera_dlg.h>
#include <EditorCore/ec_status_bar.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <EditorCore/ec_IEditorCore.h>
#include <EditorCore/ec_startup.h>

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
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_miscApi.h>

#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>

#include <3d/dag_drv3d.h>

#include <debug/dag_log.h>
#include <debug/dag_debug.h>

#include <windows.h>
#include <htmlhelp.h>

#include <de3_entityFilter.h>

#include <sepGui/wndGlobal.h>

#include <eventLog/eventLog.h>
#include <eventLog/errorLog.h>
#include <util/dag_delayedAction.h>

#ifdef ERROR
#undef ERROR
#endif

using hdpi::_pxActual;
using hdpi::_pxScaled;

#define WSP_FILE_PATH (::make_full_path(sgg::get_exe_path_full(), "../.local/workspace.blk"))

#define LEVEL_FILE_EXTENSION_WO_DOT "level.blk"
#define LEVEL_FILE_EXTENSION        "." LEVEL_FILE_EXTENSION_WO_DOT
enum
{
  CM_NAV_COMPASS = CM_PLUGIN_BASE - 10,
  CM_DISCARD_TEX_MODE,
};

extern void init3d();

namespace environment
{
void show_environment_settings(void *phandle);
void save_settings(DataBlock &blk);
void load_settings(DataBlock &blk);
}; // namespace environment

static bool blockCloseMessage = false;
static bool (*prev_fatal_handler)(const char *msg, const char *call_stack, const char *file, int line) = NULL;

// services implementations
static IEditorCore &editorCoreImpl = IEditorCore::make_instance();
static DeDagorPhys deDagorPhysImpl;
IEditorCoreEngine *IEditorCoreEngine::__global_instance = NULL;


// DLL plugin typedefs
typedef int(__fastcall *GetPluginVersionFunc)(void);
typedef IGenEditorPlugin *(__fastcall *RegisterPluginFunc)(IDagorEd2Engine &);


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
extern FastNameMap cmdline_include_dll_re, cmdline_exclude_dll_re;

IDagorEd2Engine *IDagorEd2Engine::__dagored_global_instance = NULL;

IDagorRender *dagRender = NULL;
IDagorGeom *dagGeom = NULL;
IDagorConsole *dagConsole = NULL;
IDagorTools *dagTools = NULL;
IDagorScene *dagScene = NULL;
IEditorCore *editorCore = NULL;

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
  clipping(NULL),
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

  ::dagRender = editorCoreImpl.getRender();
  G_ASSERT(::dagRender);
  ::dagGeom = editorCoreImpl.getGeom();
  G_ASSERT(::dagGeom);
  ::dagConsole = editorCoreImpl.getConsole();
  G_ASSERT(::dagConsole);
  ::dagTools = editorCoreImpl.getTools();
  G_ASSERT(::dagTools);
  ::dagScene = editorCoreImpl.getScene();
  G_ASSERT(::dagScene);

  // ViewportWindow::showDagorUiCursor = false;

  console->setCaption("DaEditorX console");

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
}


//==============================================================================
DagorEdAppWindow::~DagorEdAppWindow()
{
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

  saveScreenshotSettings(editorBlk);

  String pathName = ::make_full_path(sgg::get_exe_path_full(), "../.local/de3_settings.blk");
  editorBlk.saveToTextFile(pathName);

  wsp->save();

  // switchToPlugin(-1);

  terminateInterface();

  del_it(aboutDlg);
  del_it(wsp);
  del_it(appEH);
}


void DagorEdAppWindow::init()
{
  String imgPath(512, "%s%s", sgg::get_exe_path_full(), "../commonData/gui16x16/");
  ::dd_simplify_fname_c(imgPath.str());
  p2util::set_icon_path(imgPath.str());
  p2util::set_main_parent_handle(mManager->getMainWindow());

  makeDafaultLayout();

  GenericEditorAppWindow::init();

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

  mManager->addAccelerator(CM_FILE_SAVE, 'S', true);

  mManager->addAccelerator(CM_UNDO, 'Z', true);
  mManager->addAccelerator(CM_REDO, 'Y', true);
  mManager->addAccelerator(CM_SELECT_ALL, 'A', true);
  mManager->addAccelerator(CM_DESELECT_ALL, 'D', true);

  mManager->addAccelerator(CM_OPTIONS_TOTAL, wingw::V_F11);
  mManager->addAccelerator(CM_CUR_PLUGIN_VIS_CHANGE, wingw::V_F12);

  mManager->addAccelerator(CM_CREATE_SCREENSHOT, 'P', true);
  mManager->addAccelerator(CM_CREATE_CUBE_SCREENSHOT, 'P', true, false, true);

  mManager->addAccelerator(CM_VIEW_GRID_MOVE_SNAP, 'S', false, false, false);
  mManager->addAccelerator(CM_VIEW_GRID_ANGLE_SNAP, 'A', false, false, false);
  mManager->addAccelerator(CM_VIEW_GRID_SCALE_SNAP, '5', false, false, true);

  mManager->addAccelerator(CM_SWITCH_PLUGIN_F5, wingw::V_F5);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_F6, wingw::V_F6);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_F7, wingw::V_F7);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_F8, wingw::V_F8);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF1, wingw::V_F1, false, false, true);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF2, wingw::V_F2, false, false, true);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF3, wingw::V_F3, false, false, true);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF4, wingw::V_F4, false, false, true);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF5, wingw::V_F5, false, false, true);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF6, wingw::V_F6, false, false, true);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF7, wingw::V_F7, false, false, true);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF8, wingw::V_F8, false, false, true);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF9, wingw::V_F9, false, false, true);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF10, wingw::V_F10, false, false, true);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF11, wingw::V_F11, false, false, true);
  mManager->addAccelerator(CM_SWITCH_PLUGIN_SF12, wingw::V_F12, false, false, true);


  mManager->addAccelerator(CM_NEXT_PLUGIN, wingw::V_TAB, true);
  mManager->addAccelerator(CM_PREV_PLUGIN, wingw::V_TAB, true, false, true);

  mManager->addAccelerator(CM_TAB_PRESS, wingw::V_TAB);
  mManager->addAcceleratorUp(CM_TAB_RELEASE, wingw::V_TAB);

  mManager->addAccelerator(CM_CHANGE_VIEWPORT, 'W', true);
  if (auto *p = curPlugin())
    p->registerMenuAccelerators();
}


void DagorEdAppWindow::addCameraAccelerators()
{
  mManager->clearAccelerators();

  mManager->addAccelerator(CM_CAMERAS_FREE, ' ');
  mManager->addAccelerator(CM_CAMERAS_FPS, ' ', true);
  mManager->addAccelerator(CM_CAMERAS_TPS, ' ', true, false, true);
  // mManager->addAccelerator(CM_CAMERAS_CAR, ' ', false, false, true);

  mManager->addAccelerator(CM_DEBUG_FLUSH, 'F', true, true, false);
  mManager->addAccelerator(CM_DEBUG_FLUSH, 'F', true, false, true);
}


void DagorEdAppWindow::makeDafaultLayout()
{
  mManager->reset();

  void *hVp4 = mManager->getFirstWindow();
  void *hVp2 = mManager->splitWindowF(0, 0, 0.5f, WA_TOP);
  void *hVp3 = mManager->splitWindowF(0, 0, 0.5f, WA_LEFT);
  void *hVp1 = mManager->splitWindowF(hVp3, 0, 0.5f, WA_TOP);

  void *hPlugTools = mManager->splitWindow(0, 0, _pxScaled(TOOLBAR_HEIGHT), WA_TOP);
  mManager->fixWindow(hPlugTools, true);

  void *hTabbar = mManager->splitWindow(0, 0, _pxScaled(TABBAR_HEIGHT), WA_TOP);
  mManager->fixWindow(hTabbar, true);

  void *hToolbar = mManager->splitWindow(0, 0, _pxScaled(TOOLBAR_HEIGHT), WA_TOP);
  mManager->fixWindow(hToolbar, true);

  mManager->setWindowType(hVp1, VIEWPORT_TYPE);
  mManager->setWindowType(hVp2, VIEWPORT_TYPE);
  mManager->setWindowType(hVp3, VIEWPORT_TYPE);
  mManager->setWindowType(hVp4, VIEWPORT_TYPE);
  mManager->setWindowType(hPlugTools, PLUGTOOLS_TYPE);
  mManager->setWindowType(hTabbar, TABBAR_TYPE);
  mManager->setWindowType(hToolbar, TOOLBAR_TYPE);
}


IWndEmbeddedWindow *DagorEdAppWindow::onWmCreateWindow(void *handle, int type)
{
  switch (type)
  {
    case TOOLBAR_TYPE:
    {
      if (mToolPanel)
        return NULL;

      getMainMenu()->setEnabledById(CM_WINDOW_TOOLBAR, false);

      mManager->setHeader(handle, HEADER_LEFT);
      mManager->setCaption(handle, "Main Toolbar");

      unsigned w, h;
      mManager->getWindowClientSize(handle, w, h);

      mToolPanel = new CToolWindow(this, handle, 0, 0, _pxActual(w), _pxActual(h), "");

      fillMainToolBar();

      return mToolPanel;
    }
    break;

    case TABBAR_TYPE:
    {
      if (mTabWindow)
        return NULL;

      getMainMenu()->setEnabledById(CM_WINDOW_TABBAR, false);

      mManager->setCaption(handle, "Plugin Tabs");

      unsigned w, h;
      mManager->getWindowClientSize(handle, w, h);
      mTabWindow = new CToolWindow(this, handle, 0, 0, _pxActual(w), _pxActual(h), "");

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
        return NULL;

      getMainMenu()->setEnabledById(CM_WINDOW_PLUGTOOLS, false);

      mManager->setHeader(handle, HEADER_LEFT);
      mManager->setCaption(handle, "Plugin Tools");

      unsigned w, h;
      mManager->getWindowClientSize(handle, w, h);

      mPlugTools = new CToolWindow(this, handle, 0, 0, _pxActual(w), _pxActual(h), "");

      return mPlugTools;
    }
    break;

    case VIEWPORT_TYPE:
    {
      getMainMenu()->setEnabledById(CM_WINDOW_VIEWPORT, false);

      mManager->setCaption(handle, "Viewport");

      unsigned w, h;
      mManager->getWindowClientSize(handle, w, h);
      DagorEdViewportWindow *v = new DagorEdViewportWindow(handle, 0, 0, w, h);
      ged.addViewport(handle, appEH, mManager, v);
      return v;
    }
    break;
  }

  return NULL;
}


bool DagorEdAppWindow::onWmDestroyWindow(void *handle)
{
  if (mToolPanel && mToolPanel->getParentWindowHandle() == handle)
  {
    del_it(mToolPanel);
    mManager->getMainMenu()->setEnabledById(CM_WINDOW_TOOLBAR, true);
    return true;
  }

  if (mTabWindow && mTabWindow->getParentWindowHandle() == handle)
  {
    del_it(mTabWindow);
    mManager->getMainMenu()->setEnabledById(CM_WINDOW_TABBAR, true);
    return true;
  }

  if (mPlugTools && mPlugTools->getParentWindowHandle() == handle)
  {
    del_it(mPlugTools);
    mManager->getMainMenu()->setEnabledById(CM_WINDOW_PLUGTOOLS, true);
    return true;
  }

  for (int i = 0; i < ged.getViewportCount(); ++i)
  {
    ViewportWindow *vport = ged.getViewport(i);
    if (vport && vport->getParentHandle() == handle)
    {
      ged.deleteViewport(i);
      if (!ged.getViewportCount())
        mManager->getMainMenu()->setEnabledById(CM_WINDOW_VIEWPORT, true);
      return true;
    }
  }

  return false;
}


void DagorEdAppWindow::fillMainToolBar()
{
  ged.tbManager->init(GUI_MAIN_TOOLBAR_ID);

  PropertyContainerControlBase *tb2 = EDITORCORE->getCustomPanel(GUI_MAIN_TOOLBAR_ID)->createToolbarPanel(0, "");
  tb2->createSeparator();
  tb2->createCheckBox(CM_DISCARD_TEX_MODE, "Discard textures (show stub tex)");
  tb2->setButtonPictures(CM_DISCARD_TEX_MODE, "discard_tex");
  tb2->createSeparator();
  tb2->createCheckBox(CM_NAV_COMPASS, "Show compass");
  tb2->setButtonPictures(CM_NAV_COMPASS, "compass");
  tb2->setBool(CM_NAV_COMPASS, static_cast<DagorEdAppEventHandler *>(appEH)->showCompass);
}


void DagorEdAppWindow::fillMenu(IMenu *menu)
{
  menu->clearMenu(ROOT_MENU_ITEM);

  menu->addSubMenu(ROOT_MENU_ITEM, CM_FILE, "Project");
  // currently creating and opening projects is not supported
  //  menu->addItem(CM_FILE, CM_FILE_NEW, "Create new...");
  //  menu->addItem(CM_FILE, CM_FILE_OPEN, "Open project...");
  //  menu->addSubMenu(CM_FILE, CM_FILE_RECENT_LAST, "Open recent");
  //  menu->addItem(CM_FILE, CM_FILE_REOPEN, "Reopen current project");
  //  menu->addSeparator(CM_FILE);

  menu->addItem(CM_FILE, CM_FILE_SAVE, "Save project\tCtrl+S");
  menu->addItem(CM_FILE, CM_FILE_SAVE_AS, "Save as...");

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

  menu->addSeparator(CM_VIEW, 0);

  menu->addItem(CM_VIEW, CM_LOAD_DEFAULT_LAYOUT, "Load default layout");
  menu->addSeparator(CM_VIEW, 0);
  menu->addItem(CM_VIEW, CM_LOAD_LAYOUT, "Load layout ...");
  menu->addItem(CM_VIEW, CM_SAVE_LAYOUT, "Save layout ...");
  menu->addSeparator(CM_VIEW, 0);

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

  menu->setEnabledById(CM_WINDOW_TOOLBAR, false);
  menu->setEnabledById(CM_WINDOW_TABBAR, false);
  menu->setEnabledById(CM_WINDOW_PLUGTOOLS, false);
  menu->setEnabledById(CM_WINDOW_VIEWPORT, false);


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

  // Settings
  menu->addSubMenu(ROOT_MENU_ITEM, CM_OPTIONS, "Settings");

  menu->addItem(CM_OPTIONS, CM_OPTIONS_CAMERAS, "Cameras...");
  menu->addItem(CM_OPTIONS, CM_OPTIONS_SCREENSHOT, "Screenshot...");
  menu->addItem(CM_OPTIONS, CM_OPTIONS_STAT_DISPLAY_SETTINGS, "Stat display...");

  menu->addSeparator(CM_OPTIONS);
  menu->addItem(CM_OPTIONS, CM_ENVIRONMENT_SETTINGS, "Environment settings...");

  menu->addSeparator(CM_OPTIONS);
  menu->addItem(CM_OPTIONS, CM_FILE_SETTINGS, "Project settings...");

  // Help
  menu->addSubMenu(ROOT_MENU_ITEM, CM_HELP, "Help");

  menu->addItem(CM_HELP, CM_EDITOR_HELP, "Editor help");
  menu->addItem(CM_HELP, CM_PLUGIN_HELP, "Plugin help");

  menu->addSeparator(CM_HELP);

  menu->addItem(CM_HELP, CM_TUTORIALS, "Tutorials");

  menu->addSeparator(CM_HELP);

  menu->addItem(CM_HELP, CM_ABOUT, "About DaEditorX");

  addExitCommand(menu);
}


void DagorEdAppWindow::updateMenu(IMenu *menu) {}


void DagorEdAppWindow::updateExportMenuItems()
{
  IMenu *menu = getMainMenu();
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
      {
        strcpy(sceneFname, fileName);
        loadProject(sceneFname);
        setDocTitle();
        queryEditorInterface<IDynRenderService>()->enableRender(!d3d::is_stub_driver());
      }

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
          mManager->removeWindow(vport->getParentHandle());
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
        void *hVp1 = curVp->getParentHandle();
        void *hVp2 = mManager->splitWindowF(hVp1, 0, 0.5f, WA_BOTTOM);
        void *hVp3 = mManager->splitWindowF(hVp1, 0, 0.5f, WA_RIGHT);
        void *hVp4 = mManager->splitWindowF(hVp2, 0, 0.5f, WA_RIGHT);

        mManager->setWindowType(hVp2, VIEWPORT_TYPE);
        mManager->setWindowType(hVp3, VIEWPORT_TYPE);
        mManager->setWindowType(hVp4, VIEWPORT_TYPE);
      }

      return 1;
    }

    case CM_LOAD_DEFAULT_LAYOUT:
      mManager->show(WSI_MAXIMIZED);
      makeDafaultLayout();
      return 1;

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
        plugDlg.autoSize();

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
        void *hToolbar = mManager->splitWindow(0, 0, _pxScaled(TOOLBAR_HEIGHT), WA_TOP);
        mManager->fixWindow(hToolbar, true);
        mManager->setWindowType(hToolbar, TOOLBAR_TYPE);
      }
      return 0;

    case CM_WINDOW_TABBAR:
      if (!mTabWindow)
      {
        void *hTabbar = mManager->splitWindow(0, 0, _pxScaled(TABBAR_HEIGHT), WA_TOP);
        mManager->fixWindow(hTabbar, true);
        mManager->setWindowType(hTabbar, TABBAR_TYPE);
      }
      return 0;

    case CM_WINDOW_PLUGTOOLS:
      if (!mPlugTools)
      {
        void *hPlugTools = mManager->splitWindow(0, 0, _pxScaled(TOOLBAR_HEIGHT), WA_TOP);
        mManager->fixWindow(hPlugTools, true);
        mManager->setWindowType(hPlugTools, PLUGTOOLS_TYPE);
      }
      return 0;

    case CM_WINDOW_VIEWPORT:
      if (!ged.getViewportCount())
      {
        void *hwndViewPort = mManager->splitWindow(0, 0, _pxScaled(VIEWPORT_HEIGHT), WA_BOTTOM);
        mManager->setWindowType(hwndViewPort, VIEWPORT_TYPE);
      }
      return 0;

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

    case CM_ENVIRONMENT_SETTINGS:
      environment::show_environment_settings(0);
      // break;
      return 1;

    case CM_OPTIONS_CAMERAS: show_camera_objects_config_dialog(0); return 1;

    case CM_OPTIONS_SCREENSHOT: setScreenshotOptions(); return 1;

    case CM_OPTIONS_STAT_DISPLAY_SETTINGS:
    {
      ViewportWindow *viewport = ged.getCurrentViewport();
      if (viewport)
        viewport->showStatSettingsDialog();

      return 1;
    }

    case CM_FILE_OPEN_AND_LOCK: handleOpenProject(true); return 1;

    case CM_FILE_REOPEN:
      if (sceneFname[0] && canCloseScene("Reopen project"))
        loadProject(sceneFname);
      queryEditorInterface<IDynRenderService>()->enableRender(!d3d::is_stub_driver());
      // break;
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
      GridEditDialog dlg(0, EDITORCORE->getGrid(), "", ged.getCurrentViewportId());
      dlg.showDialog();
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
      mManager->getMainMenu()->setCheckById(id, discardTexMode);
      mToolPanel->setBool(id, discardTexMode);
      wingw::set_busy(true);
      dxp_factory_force_discard(discardTexMode);
      spawnEvent(HUID_InvalidateClipmap, (void *)true);
      wingw::set_busy(false);
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

        if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
        {
          mToolPanel->setBool(CM_CAMERAS_FREE, false);
          mToolPanel->setBool(CM_CAMERAS_FPS, false);
          mToolPanel->setBool(CM_CAMERAS_TPS, false);
          mToolPanel->setBool(CM_CAMERAS_CAR, false);
          addEditorAccelerators();
        }
        else
        {
          mToolPanel->setBool(id, true);
          addCameraAccelerators();
        }

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

  return GenericEditorAppWindow::onMenuItemClick(id);
}


void DagorEdAppWindow::onClick(int pcb_id, PropPanel2 *panel)
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
    case CM_CONSOLE: console->showConsole(true); break;

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


void DagorEdAppWindow::onChange(int pcb_id, PropPanel2 *panel)
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
  alefind_t ff;
  bool ok;
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

  for (ok = ::dd_find_first(mask, DA_FILE, &ff); ok; ok = ::dd_find_next(&ff))
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

    HMODULE dllHandle = ::LoadLibrary((const char *)libPath);

    if (dllHandle)
    {
      ::symhlp_load(libPath);
      GetPluginVersionFunc getVersionFunc = (GetPluginVersionFunc)::GetProcAddress(dllHandle, "get_plugin_version");

      if (getVersionFunc)
      {
        if ((*getVersionFunc)() != IGenEditorPlugin::CURRENT_VERSION)
        {
          ::symhlp_unload(libPath);
          ::FreeLibrary(dllHandle);
          console->addMessage(ILogWriter::WARNING, "Plugin version mismatch. It can make Editor unstable.");
          continue;
        }

        RegisterPluginFunc regFunc = (RegisterPluginFunc)::GetProcAddress(dllHandle, "register_plugin");

        if (regFunc)
        {
          DAGOR_TRY
          {
            IGenEditorPlugin *plug = (*regFunc)(*IDagorEd2Engine::get());

            if (plug)
            {
              if (registerDllPlugin(plug, dllHandle))
                console->addMessage(ILogWriter::REMARK, "Plugin \"%s\" (%s) succesfully registered", ff.name,
                  plug->getMenuCommandName());
              else
              {
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
      ::FreeLibrary(dllHandle);
      console->addMessage((ILogWriter::MessageType)errorType, errorMess);

      errorMess.clear();
    }
  }
  clear_all_ptr_items(reExcludeDll);

  ::dd_find_close(&ff);
}


//==============================================================================
void DagorEdAppWindow::startWithWorkspace(const char *wspName)
{
  String settingsPath = ::make_full_path(sgg::get_exe_path_full(), "../.local/de3_settings.blk");
  DataBlock settingsBlk(settingsPath);

  loadScreenshotSettings(settingsBlk);

  if (!wspName)
    wspName = settingsBlk.getBlockByNameEx("workspace")->getStr("currentName", NULL);


  for (bool handled = false; !handled;)
  {
    StartupDlg dlg(mManager->getMainWindow(), "DaEditorX start", *getWorkspace(), WSP_FILE_PATH, wspName);

    int result = shouldLoadFile ? DIALOG_ID_OK : dlg.showDialog();
    int sel = shouldLoadFile ? ID_RECENT_FIRST : dlg.getSelected();
    dagor_idle_cycle();

    handled = true;

    if (result == DIALOG_ID_CANCEL || sel == -1)
    {
      mManager->close();
      return;
    }

    const char *wspName = wsp->getName();
    if (!wspName || !*wspName)
      continue;

    wingw::set_busy(true);
    resetCore();
    DataBlock app_blk;
    if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
      DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());

    init3d();
    setDocTitle();
    initPlugins();

    const Tab<String> &recents = wsp->getRecents();

    // to mirror in menu workspace specific data
    //::appWnd.recreateMenu();

    const DataBlock &projDef = *app_blk.getBlockByNameEx("projectDefaults");
    StaticSceneBuilder::check_degenerates = projDef.getBool("check_degenerates", true);
    StaticSceneBuilder::check_no_smoothing = projDef.getBool("check_no_smoothing", true);

    updateExportMenuItems();

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

      default: // load recent
        if (recents.size() || shouldLoadFile)
        {
          String fileName = ::make_full_path(wsp->getSdkDir(), shouldLoadFile ? sceneFname : recents[sel - ID_RECENT_FIRST]);
          ::strcpy(sceneFname, fileName);

          if (!shouldLoadFile)
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
    wingw::set_busy(false);
  }
  shouldLoadFile = false;

  wsp->freeWorkspaceBlk();
  updateRecentMenu();

  wingw::set_busy(true);
  spawnEvent(HUID_AfterProjectLoad, NULL);

  queryEditorInterface<IDynRenderService>()->enableRender(!d3d::is_stub_driver());
  repaint();
  dagor_work_cycle_flush_pending_frame();
  dagor_draw_scene_and_gui(true, true);
  d3d::update_screen();
  dagor_idle_cycle();
  debug_cp();
  wingw::set_busy(false);
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

  // if (huid == HUID_IFmodService)
  //   return get_generic_fmod_service();

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
  /*
    IMenu *_menu = getMainMenu();
    _menu->clearMenu(CM_FILE_RECENT_LAST);

    int n = 0, i;
    for (i = wsp->getRecents().size()-1; i >= 0 && n < CM_FILE_RECENT_LAST - CM_FILE_RECENT_BASE; i --)
    {
      _menu->addItem (CM_FILE_RECENT_LAST, CM_FILE_RECENT_BASE + i, wsp->getRecents()[i]);
      n ++;
    }

    if (!n)
    {
      _menu->addItem (CM_FILE_RECENT_LAST, CM_FILE_RECENT_BASE, "Empty");
      _menu->setEnabledById(CM_FILE_RECENT_BASE, false);
    }
  */
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
  alefind_t ff;
  bool ok;

  for (ok = dd_find_first(FilePathName(from, "*.*"), DA_SUBDIR, &ff); ok; ok = dd_find_next(&ff))
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

  dd_find_close(&ff);
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
  if (dlg.showDialog() != DIALOG_ID_OK)
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
  String helpPath(260, "%s/daeditor3.chm::/html/Overview/index.htm", sgg::get_exe_path_full());

  ::HtmlHelp(NULL, helpPath.str(), HH_DISPLAY_TOPIC, 0);
}


//==============================================================================
void DagorEdAppWindow::pluginHelp(const char *url) const
{
  if (!url || !*url)
    return;

  String help(512, "%s/daeditor3.chm::%s", sgg::get_exe_path_full(), url);

  ::HtmlHelp(NULL, help.str(), HH_DISPLAY_TOPIC, 0);
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
  if (!sceneFname[0])
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
  wingw::set_busy(false);
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
  if (dlg.showDialog() != DIALOG_ID_OK)
    return false;

  wingw::set_busy(true);
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
    debug_ctx("Error loading %s", filename);
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

  IClipping *collision = getInterfaceEx<IClipping>();

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
  debug_cp();

  // perform initial pre-frame-render update
  beforeRenderObjects();
  debug_cp();

  repaint();

  dagor_work_cycle_flush_pending_frame();
  dagor_draw_scene_and_gui(true, true);
  d3d::update_screen();
  dagor_idle_cycle();
  debug_cp();

  prev_fatal_handler = dgs_fatal_handler;
  dgs_fatal_handler = DagorEdAppWindow::gracefulFatalExit;

  return true;
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
IClipping *DagorEdAppWindow::getClipping()
{
  if (!clipping)
  {
    IClipping *tc = NULL;

    for (int i = 0; i < getPluginCount(); ++i)
    {
      IGenEditorPlugin *plugin = getPlugin(i);

      if (plugin)
        tc = plugin->queryInterface<IClipping>();

      if (tc)
      {
        clipping = tc;
        break;
      }
    }
  }

  G_ASSERT(clipping);

  return clipping;
}


//==============================================================================

void DagorEdAppWindow::showSelectWindow(IObjectsList *obj_list, const char *obj_list_owner_name)
{
  SelWindow selWnd(0, obj_list, obj_list_owner_name);

  if (selWnd.showDialog() == DIALOG_ID_OK)
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


//==================================================================================================
Point3 DagorEdAppWindow::snapToGrid(const Point3 &p) const { return grid.snapToGrid(p); }


//==================================================================================================
Point3 DagorEdAppWindow::snapToAngle(const Point3 &p) const { return grid.snapToAngle(p); }


//==================================================================================================
Point3 DagorEdAppWindow::snapToScale(const Point3 &p) const { return grid.snapToScale(p); }


//==================================================================================================
String DagorEdAppWindow::getScreenshotNameMask(bool cube) const
{
  if (!cube && screenshotSaveJpeg)
    return ::make_full_path(getSdkDir(), "de_scrn%03i.jpg");
  return ::make_full_path(getSdkDir(), cube ? "de_cube%03i.tga" : "de_scrn%03i.tga");
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

const char *daeditor3_get_appblk_fname()
{
  static String fn;
  fn.printf(260, "%s/application.blk", DAGORED2->getWorkspace().getAppDir());
  return fn;
}
