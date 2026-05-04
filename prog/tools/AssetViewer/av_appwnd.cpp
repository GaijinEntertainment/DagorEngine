// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "av_appwnd.h"
#include "av_assetSearchResultsWindow.h"
#include "av_consoleCommandProcessor.h"
#include "av_plugin.h"
#include "av_cm.h"
#include "av_mainAssetSelector.h"
#include "av_preferencesDialog.h"
#include "av_viewportWindow.h"
#include "av_workCycleActRateDialog.h"

#include "assetUserFlags.h"
#include "assetBackReferenceViewer.h"
#include "assetBuildCache.h"
#include "assetReferenceViewer.h"
#include "badRefFinder.h"
#include "compositeAssetCreator.h"
#include "suboptimalAssetCollector.h"

#include <assets/daBuildInterface.h>
#include <assets/asset.h>
#include <assets/assetExporter.h>
#include <assets/assetMsgPipe.h>
#include <assets/assetHlp.h>
#include <assets/assetRefs.h>
#include <assets/texAssetBuilderTextureFactory.h>
#include <assetsGui/av_allAssetsTree.h>
#include <assetsGui/av_assetSelectorCommon.h>
#include <assetsGui/av_globalState.h>
#include <generic/dag_sort.h>

#include <util/dag_texMetaData.h>
#include <assets/assetUtils.h>
#include <libTools/util/fileUtils.h>
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
#include <render/wind/ambientWind.h>
#include <render/debugTexOverlay.h>
#include <render/texDebug.h>
#include <render/daFrameGraph/debug.h>

#include <debug/dag_debug.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResHooks.h>
#include <assets/assetFolder.h>
#include <math/dag_Point2.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_restart.h>

#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_IEditorCore.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_input.h>
#include <EditorCore/ec_startup.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_rect.h>
#include <EditorCore/ec_genapp_ehfilter.h>
#include <EditorCore/ec_selwindow.h>
#include <EditorCore/ec_viewportSplitter.h>
#include <EditorCore/ec_wndGlobal.h>
#include <EditorCore/ec_shaders.h>
#include <EditorCore/ec_workspace.h>

#include <consoleWindow/dag_consoleWindow.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/event.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
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
#include <propPanel/control/toolbarContainer.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/propPanel.h>
#include <propPanel/toastManager.h>
#include <propPanel/c_util.h>
#include <propPanel/colors.h>
#include <propPanel/constants.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>
#include <workCycle/dag_startupModules.h>
#include <workCycle/dag_workCycle.h>
#include <perfMon/dag_daProfilerSettings.h>
#include <stdio.h>
#include <ioSys/dag_memIo.h>

#include <gui/dag_imgui.h>
#include <gui/dag_stdGuiRenderEx.h>
#include <gameRes/dag_collisionResource.h>
#include <rendInst/rendInstGen.h>
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
#include <ska_hash_map/flat_hash_map2.hpp>

#ifdef _TARGET_PC_WIN
#define PCRE_STATIC
#endif
#include <pcre/pcre/pcre.h>

#if _TARGET_PC_WIN
#include <windows.h>
#undef ERROR
#include <osApiWrappers/dag_unicode.h>
#include <Shlobj.h>
#endif

using hdpi::_pxActual;
using hdpi::_pxScaled;
using PropPanel::ROOT_MENU_ITEM;

namespace workcycle_internal
{
extern bool window_initing;
}

ECS_BROADCAST_EVENT_TYPE(ImGuiStage);
ECS_REGISTER_EVENT(ImGuiStage)

static constexpr const char *INITIAL_CAPTION = "AssetViewer2";
static constexpr const char *INITIAL_ICON = "AssetViewerIcon";

extern String fx_devres_base_path;

// services implementations
static IEditorCore &editorCoreImpl = IEditorCore::make_instance();

static String assetFile;
static unsigned collisionMask = 0;
static unsigned rendGeomMask = 0;
bool av_perform_uptodate_check = true;
bool dabuildWindowVisible = true;
bool daBuildWindowAutoOpen = true;
bool daBuildWindowAutoClose = false;
String a2d_last_ref_model;
DataBlock asset_browser_modal_settings;

extern void init_interface_de3();
extern bool de3_spawn_event(unsigned event_huid, void *user_data);
extern void init_rimgr_service(const DataBlock &appblk);
extern void init_prefabmgr_service(const DataBlock &appblk);
extern void init_fxmgr_service(const DataBlock &appblk);
extern void init_efxmgr_service();
extern void init_physobj_mgr_service();
extern void init_gameobj_mgr_service();
extern void init_composit_mgr_service(const DataBlock &app_blk);
extern void init_invalid_entity_service();
extern void init_dynmodel_mgr_service(const DataBlock &app_blk);
extern void init_animchar_mgr_service(const DataBlock &app_blk);
extern void init_ecs_animchar_mgr_service(const DataBlock &app_blk);
extern void init_csg_mgr_service();
extern void init_spline_gen_mgr_service();
extern void init_ecs_mgr_service(const DataBlock &app_blk, const char *app_dir);
extern void init_webui_service();
extern void init_das_mgr_service(const DataBlock &app_blk, bool dng_based_render_used);

extern void mount_efx_assets(DagorAssetMgr &aMgr, const char *fx_blk_fname);
extern void mount_ecs_template_assets(DagorAssetMgr &aMgr);

extern void register_phys_car_gameres_factory();
extern bool src_assets_scan_allowed;

static void init3d_early();
static bool disable_gameres_loading(const char *, bool) { return false; }

IDagorInput *editorcore_extapi::dagInput = nullptr;

IWindService *av_wind_srv = NULL;
ISkiesService *av_skies_srv = NULL;
SimpleString av_skies_preset, av_skies_env, av_skies_wtype;
EditorCommandSystem editor_command_system;
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
  TAGMANAGER_TYPE,
  PPANEL_TYPE,
  TOOLBAR_TYPE,
  VIEWPORT_TYPE,
  TOOLBAR_PLUGIN_TYPE,
  TOOLBAR_THEME_SWITCHER_TYPE,
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
#if _TARGET_PC_WIN // TODO: tools Linux porting: runExternalEditor -- unused
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
#else
  G_UNUSED(a);
  LOGERR_CTX("TODO: tools Linux porting: runExternalEditor");
#endif
}

String get_global_av_settings_file_path() { return make_full_path(sgg::get_exe_path_full(), "../.local/av_settings.blk"); }

static String get_global_av_hotkey_settings_file_path()
{
  return make_full_path(sgg::get_exe_path_full(), "../.local/av_hotkeys.blk");
}

static E3DCOLOR load_window_background_color()
{
  DataBlock settingsBlock;
  dblk::load(settingsBlock, get_global_av_settings_file_path(), dblk::ReadFlag::ROBUST);
  const DataBlock *settingsThemeBlock = settingsBlock.getBlockByNameEx("theme");
  const char *themeName = settingsThemeBlock->getStr("name", "light");
  return editor_core_load_window_background_color(String::mk_str_cat(themeName, ".blk"));
}

static void log_invalid_asset_names(const DataBlock &app_blk, dag::ConstSpan<DagorAsset *> assets)
{
  static constexpr const char *BLOCK_NAME = "asset_viewer";
  static constexpr const char *PARAM_NAME = "assetNameValidationRegExp";

  const DataBlock *assetViewerBlk = app_blk.getBlockByNameEx(BLOCK_NAME);
  const char *regExpString = assetViewerBlk->getStr(PARAM_NAME, "");
  if (*regExpString == 0)
    return;

  const char *error = nullptr;
  int errorOffset = 0;
  pcre *regExp = pcre_compile(regExpString, 0, &error, &errorOffset, nullptr);
  if (!regExp)
  {
    DAEDITOR3.conError("Wrong regular expression string at %s/%s: \"%s\". Error: %s", BLOCK_NAME, PARAM_NAME, regExpString, error);
    return;
  }

  for (DagorAsset *asset : assets)
  {
    const char *name = asset->getName();
    const int result = pcre_exec(regExp, nullptr, name, strlen(name), 0, 0, nullptr, 0);
    if (result < 0)
      DAEDITOR3.conError("Invalid asset name: \"%s\". Asset type: \"%s\", path: \"%s\".", name, asset->getTypeStr(),
        asset->getTargetFilePath().c_str());
  }

  pcre_free(regExp);
}

static inline float deg_to_fov(float deg) { return 1.f / tanf(DEG_TO_RAD * 0.5f * deg); }

static inline float fov_to_deg(float fov) { return RAD_TO_DEG * 2.f * atan(1.f / fov); }

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

  editorcore_extapi::dagInput = editorCoreImpl.getInput();
  G_ASSERT(editorcore_extapi::dagInput);

  appEH = new (inimem) AppEventHandler(*this);

  curAssetPackName = NULL;

  mTreeView = NULL;
  mTagManager = NULL;
  mPropPanel = NULL;
  mToolPanel = NULL;
  mPluginTool = NULL;

  rendGeomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
  collisionMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("collision");

  mManager->registerWindowHandler(this);
}


AssetViewerApp::~AssetViewerApp()
{
  const DataBlock &debugSettings = *dgs_get_settings()->getBlockByNameEx("debug");
  const DataBlock *profiler = debugSettings.getBlockByName("profiler");
  if (profiler && profiler->getBool("auto_dump", false))
  {
    da_profiler::request_dump();
    da_profiler::tick_frame();
  }

  texconvcache::build_on_demand_tex_factory_cease_loading(false);
  environment::destroyEnviEntity(assetLtData);
  switchToPlugin(-1);
  if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
    rigenSrv->clearRtRIGenData();
  ged.curEH = NULL;
  ged.setEH(appEH);

  // Do not show the VR controls window at the next application start.
  if (imgui_window_is_visible("VR", "VR controls"))
    imgui_window_set_visible("VR", "VR controls", false);

  console->destroyConsole(); // Prevent the console from issuing draw commands during shut down.
  asset_search_results_window.reset();
  getModelessWindowControllers().releaseAllWindows();
  mManager->unregisterWindowHandler(this);
  PropPanel::remove_delayed_callback(*this);
  PropPanel::release();

  ShaderGlobal::set_texture_fast(get_shader_glob_var_id("lightmap_tex", true), BAD_TEXTUREID);
  if (av_skies_srv)
    av_skies_srv->term();
  if (av_wind_srv)
    av_wind_srv->term();
  if (auto *srv = queryEditorInterface<IDynRenderService>())
    srv->term();

  ::set_global_tex_name_resolver(NULL);
  assetMgr.enableChangesTracker(false);

  terminateInterface();
  ::term_dabuild_cache();
  assetMgr.clear();
  if (!useDngBasedSceneRender)
    dynrend::close();
  close_ambient_wind();
  extern void console_close();
  console_close();
  imgui_shutdown();

  // These do not survive multiple closes, and shut down by GuiBaseRestartProc too when using the daNetGame-based renderer.
  if (!useDngBasedSceneRender)
  {
    StdGuiRender::close_fonts();
    StdGuiRender::close_render();
  }

  del_it(appEH);
}


void AssetViewerApp::init(const char *select_workspace)
{
  DEBUG_CP();
  rendinst::rendinstClipmapShadows = false;
  ::init_dabuild_cache(sgg::get_exe_path_full());

  PropPanel::p2util::set_main_parent_handle(mManager->getMainWindow());

  String imgPath(512, "%s%s", sgg::get_exe_path_full(), "../commonData/icons/");
  ::dd_simplify_fname_c(imgPath.str());
  mManager->initCustomMouseCursors(imgPath);

  loadGlobalSettings();

  // Initialize the startup scene before init3d(), so VideoRestartProc can use it to draw the first frame.
  startup_editor_core_select_startup_scene(load_window_background_color());

  init3d_early();

  dagor_init_keyboard_win();
  dagor_init_mouse_win();
  editor_core_initialize_input_handler();
  startup_game(RESTART_ALL);

  editor_core_initialize_imgui();
  editor_core_load_imgui_theme(getThemeFileName());
  GenericEditorAppWindow::init(select_workspace);

  PropPanel::load_toast_message_icons();

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

  registerEditorCommands();

  // Normally it would be enough to just call addAccelerators() here but because both fillMenu() and fillToolbar() ran
  // before registerEditorCommands() so we have to update the menu and toolbar button shortcuts too. So we simply call
  // onEditorCommandKeyChordChanged() to do all these.
  onEditorCommandKeyChordChanged();
}

void AssetViewerApp::onEditorCommandKeyChordChanged()
{
  addAccelerators();

  editor_command_system.updateMenuItemShortcuts(*getMainMenu());

  for (int i = 0; i < ged.getViewportCount(); ++i)
    if (ViewportWindow *viewport = ged.getViewport(i))
      if (PropPanel::IMenu *menu = viewport->getPopupMenu())
        editor_command_system.updateMenuItemShortcuts(*menu);

  if (mToolPanel)
    editor_command_system.updateToolbarButtons(*mToolPanel);
  if (mPluginTool)
    editor_command_system.updateToolbarButtons(*mPluginTool);
}

void AssetViewerApp::registerEditorCommands()
{
  registerCommonEditorCommands(editor_command_system);
  ViewportWindow::registerEditorCommands(editor_command_system);

  for (int i = 0; i < getPluginCount(); ++i)
    if (IGenEditorPlugin *p = getPlugin(i))
      p->registerEditorCommands(editor_command_system);

  editor_command_system.addCommand(EditorCommandIds::DEBUG_FLUSH, ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_F,
    ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_F);

  editor_command_system.addCommand(EditorCommandIds::SAVE_CUR_ASSET, ImGuiMod_Ctrl | ImGuiKey_S);
  editor_command_system.addCommand(EditorCommandIds::SAVE_ASSET_BASE, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S);
  editor_command_system.addCommand(EditorCommandIds::CHECK_ASSET_BASE);
  editor_command_system.addCommand(EditorCommandIds::RELOAD_SHADERS, ImGuiMod_Ctrl | ImGuiKey_R);

  editor_command_system.addCommand(EditorCommandIds::LOAD_DEFAULT_LAYOUT);
  editor_command_system.addCommand(EditorCommandIds::LOAD_LAYOUT);
  editor_command_system.addCommand(EditorCommandIds::SAVE_LAYOUT);
  editor_command_system.addCommand(EditorCommandIds::TOGGLE_TAG_MANAGER);
  editor_command_system.addCommand(EditorCommandIds::TOGGLE_PROPERTY_PANEL, ImGuiKey_P);
  editor_command_system.addCommand(EditorCommandIds::COMPOSITE_EDITOR, ImGuiKey_C);

  editor_command_system.addCommand(EditorCommandIds::NEXT_ASSET, ImGuiMod_Ctrl | ImGuiKey_Enter);
  editor_command_system.addCommand(EditorCommandIds::PREV_ASSET, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Enter);

  editor_command_system.addCommand(EditorCommandIds::UNDO, ImGuiMod_Ctrl | ImGuiKey_Z);
  editor_command_system.addCommand(EditorCommandIds::REDO, ImGuiMod_Ctrl | ImGuiKey_Y);
  editor_command_system.addCommand(EditorCommandIds::CONSOLE, ImGuiMod_Ctrl | ImGuiKey_GraveAccent); // Ctrl+`

  editor_command_system.addCommand(EditorCommandIds::COLLISION_PREVIEW);
  editor_command_system.addCommand(EditorCommandIds::RENDER_GEOMETRY);
  editor_command_system.addCommand(EditorCommandIds::AUTO_ZOOM_N_CENTER);
  editor_command_system.addCommand(EditorCommandIds::DISCARD_ASSET_TEX);
  editor_command_system.addCommand(EditorCommandIds::PALETTE);
  editor_command_system.addCommand(EditorCommandIds::VR_ENABLE);
  editor_command_system.addCommand(EditorCommandIds::THEME_TOGGLE);

  if (impostorApp)
  {
    editor_command_system.addCommand(EditorCommandIds::EXPORT_CURRENT_IMPOSTOR);
    editor_command_system.addCommand(EditorCommandIds::EXPORT_IMPOSTORS_CURRENT_PACK);
    editor_command_system.addCommand(EditorCommandIds::EXPORT_ALL_IMPOSTORS);
    editor_command_system.addCommand(EditorCommandIds::CLEAR_UNUSED_IMPOSTORS);
  }

  if (pointCloudGen)
    editor_command_system.addCommand(EditorCommandIds::EXPORT_CURRENT_POINT_CLOUD);

  editor_command_system.addCommand(EditorCommandIds::BUILD_RESOURCES);
  editor_command_system.addCommand(EditorCommandIds::BUILD_TEXTURES);
  editor_command_system.addCommand(EditorCommandIds::BUILD_ALL);
  editor_command_system.addCommand(EditorCommandIds::BUILD_CLEAR_CACHE);

  editor_command_system.addCommand(EditorCommandIds::BUILD_ALL_PLATFORM_RES);
  editor_command_system.addCommand(EditorCommandIds::BUILD_ALL_PLATFORM_TEX);
  editor_command_system.addCommand(EditorCommandIds::BUILD_ALL_PLATFORM);
  editor_command_system.addCommand(EditorCommandIds::BUILD_CLEAR_CACHE_ALL);

  editor_command_system.addCommand(EditorCommandIds::OPTIONS_SET_ACT_RATE);
  editor_command_system.addCommand(EditorCommandIds::CAMERAS);
  editor_command_system.addCommand(EditorCommandIds::SCREENSHOT);
  editor_command_system.addCommand(EditorCommandIds::OPTIONS_STAT_DISPLAY_SETTINGS);
  editor_command_system.addCommand(EditorCommandIds::OPTIONS_EDIT_PREFERENCES);

  editor_command_system.addCommand(EditorCommandIds::VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES);
  editor_command_system.addCommand(EditorCommandIds::VIEW_DEVELOPER_TOOLS_IMGUI_DEBUGGER, ImGuiMod_Ctrl | ImGuiKey_F12);
  editor_command_system.addCommand(EditorCommandIds::VIEW_DEVELOPER_TOOLS_TOAST_MANAGER);
  editor_command_system.addCommand(EditorCommandIds::VIEW_DEVELOPER_TOOLS_TEXTURE_DEBUG);
  editor_command_system.addCommand(EditorCommandIds::VIEW_DEVELOPER_TOOLS_NODE_DEPS);

  // All commands have been registered, enable logging access to missing commands.
  editor_command_system.enableMissingCommandLogging();

  const String hotkeySettingsPath = get_global_av_hotkey_settings_file_path();
  if (dd_file_exists(hotkeySettingsPath))
  {
    const DataBlock hotkeysBlk(hotkeySettingsPath);
    editor_command_system.loadChangedHotkeys(hotkeysBlk);
  }
}

void AssetViewerApp::addAccelerators()
{
  mManager->clearAccelerators();

  { // camera controls that always work
    mManager->addViewportAccelerator(CM_CAMERAS_FREE, EditorCommandIds::CAMERAS_FREE);
    mManager->addViewportAccelerator(CM_CAMERAS_FPS, EditorCommandIds::CAMERAS_FPS);
    mManager->addViewportAccelerator(CM_CAMERAS_TPS, EditorCommandIds::CAMERAS_TPS);
    mManager->addAccelerator(CM_ZOOM_AND_CENTER, EditorCommandIds::ZOOM_AND_CENTER_IN_FLY_MODE); // For fly mode.

    mManager->addAccelerator(CM_DEBUG_FLUSH, EditorCommandIds::DEBUG_FLUSH);
  }

  if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
  {
    mManager->addAccelerator(CM_SAVE_CUR_ASSET, EditorCommandIds::SAVE_CUR_ASSET);
    mManager->addAccelerator(CM_SAVE_ASSET_BASE, EditorCommandIds::SAVE_ASSET_BASE);
    mManager->addAccelerator(CM_CHECK_ASSET_BASE, EditorCommandIds::CHECK_ASSET_BASE);
    mManager->addAccelerator(CM_RELOAD_SHADERS, EditorCommandIds::RELOAD_SHADERS);

    mManager->addAccelerator(CM_ZOOM_AND_CENTER, EditorCommandIds::ZOOM_AND_CENTER); // For normal mode.
    mManager->addAccelerator(CM_LOAD_DEFAULT_LAYOUT, EditorCommandIds::LOAD_DEFAULT_LAYOUT);
    mManager->addAccelerator(CM_LOAD_LAYOUT, EditorCommandIds::LOAD_LAYOUT);
    mManager->addAccelerator(CM_SAVE_LAYOUT, EditorCommandIds::SAVE_LAYOUT);
    mManager->addAccelerator(CM_WINDOW_TAGMANAGER, EditorCommandIds::TOGGLE_TAG_MANAGER);
    mManager->addAccelerator(CM_WINDOW_PPANEL_ACCELERATOR, EditorCommandIds::TOGGLE_PROPERTY_PANEL);
    mManager->addAccelerator(CM_WINDOW_COMPOSITE_EDITOR_ACCELERATOR, EditorCommandIds::COMPOSITE_EDITOR);

    mManager->addAccelerator(CM_CREATE_SCREENSHOT, EditorCommandIds::CREATE_SCREENSHOT);
    mManager->addAccelerator(CM_CREATE_CUBE_SCREENSHOT, EditorCommandIds::CREATE_CUBE_SCREENSHOT);

    mManager->addAccelerator(CM_NEXT_ASSET, EditorCommandIds::NEXT_ASSET);
    mManager->addAccelerator(CM_PREV_ASSET, EditorCommandIds::PREV_ASSET);

    mManager->addAccelerator(CM_UNDO, EditorCommandIds::UNDO);
    mManager->addAccelerator(CM_REDO, EditorCommandIds::REDO);
    mManager->addAccelerator(CM_CONSOLE, EditorCommandIds::CONSOLE);

    mManager->addAccelerator(CM_NAVIGATE, EditorCommandIds::NAVIGATE);
    mManager->addAccelerator(CM_ENVIRONMENT_SETTINGS, EditorCommandIds::ENVIRONMENT_SETTINGS);
    mManager->addAccelerator(CM_CHANGE_FOV, EditorCommandIds::CHANGE_FOV);
    mManager->addAccelerator(CM_COLLISION_PREVIEW, EditorCommandIds::COLLISION_PREVIEW);
    mManager->addAccelerator(CM_RENDER_GEOMETRY, EditorCommandIds::RENDER_GEOMETRY);
    mManager->addAccelerator(CM_AUTO_ZOOM_N_CENTER, EditorCommandIds::AUTO_ZOOM_N_CENTER);
    mManager->addAccelerator(CM_DISCARD_ASSET_TEX, EditorCommandIds::DISCARD_ASSET_TEX);
    mManager->addAccelerator(CM_PALETTE, EditorCommandIds::PALETTE);
    mManager->addAccelerator(CM_VR_ENABLE, EditorCommandIds::VR_ENABLE);
    mManager->addAccelerator(CM_THEME_TOGGLE, EditorCommandIds::THEME_TOGGLE);

    if (impostorApp)
    {
      mManager->addAccelerator(CM_EXPORT_CURRENT_IMPOSTOR, EditorCommandIds::EXPORT_CURRENT_IMPOSTOR);
      mManager->addAccelerator(CM_EXPORT_IMPOSTORS_CURRENT_PACK, EditorCommandIds::EXPORT_IMPOSTORS_CURRENT_PACK);
      mManager->addAccelerator(CM_EXPORT_ALL_IMPOSTORS, EditorCommandIds::EXPORT_ALL_IMPOSTORS);
      mManager->addAccelerator(CM_CLEAR_UNUSED_IMPOSTORS, EditorCommandIds::CLEAR_UNUSED_IMPOSTORS);
    }

    if (pointCloudGen)
      mManager->addAccelerator(CM_EXPORT_CURRENT_POINT_CLOUD, EditorCommandIds::EXPORT_CURRENT_POINT_CLOUD);

    mManager->addAccelerator(CM_BUILD_RESOURCES, EditorCommandIds::BUILD_RESOURCES);
    mManager->addAccelerator(CM_BUILD_TEXTURES, EditorCommandIds::BUILD_TEXTURES);
    mManager->addAccelerator(CM_BUILD_ALL, EditorCommandIds::BUILD_ALL);
    mManager->addAccelerator(CM_BUILD_CLEAR_CACHE, EditorCommandIds::BUILD_CLEAR_CACHE);

    mManager->addAccelerator(CM_BUILD_ALL_PLATFORM_RES, EditorCommandIds::BUILD_ALL_PLATFORM_RES);
    mManager->addAccelerator(CM_BUILD_ALL_PLATFORM_TEX, EditorCommandIds::BUILD_ALL_PLATFORM_TEX);
    mManager->addAccelerator(CM_BUILD_ALL_PLATFORM, EditorCommandIds::BUILD_ALL_PLATFORM);
    mManager->addAccelerator(CM_BUILD_CLEAR_CACHE_ALL, EditorCommandIds::BUILD_CLEAR_CACHE_ALL);

    mManager->addAccelerator(CM_OPTIONS_SET_ACT_RATE, EditorCommandIds::OPTIONS_SET_ACT_RATE);
    mManager->addAccelerator(CM_CAMERAS, EditorCommandIds::CAMERAS);
    mManager->addAccelerator(CM_SCREENSHOT, EditorCommandIds::SCREENSHOT);
    mManager->addAccelerator(CM_OPTIONS_STAT_DISPLAY_SETTINGS, EditorCommandIds::OPTIONS_STAT_DISPLAY_SETTINGS);
    mManager->addAccelerator(CM_OPTIONS_EDIT_PREFERENCES, EditorCommandIds::OPTIONS_EDIT_PREFERENCES);

    mManager->addAccelerator(CM_VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES,
      EditorCommandIds::VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES);
    mManager->addAccelerator(CM_VIEW_DEVELOPER_TOOLS_IMGUI_DEBUGGER, EditorCommandIds::VIEW_DEVELOPER_TOOLS_IMGUI_DEBUGGER);
    mManager->addAccelerator(CM_VIEW_DEVELOPER_TOOLS_TOAST_MANAGER, EditorCommandIds::VIEW_DEVELOPER_TOOLS_TOAST_MANAGER);
    mManager->addAccelerator(CM_VIEW_DEVELOPER_TOOLS_TEXTURE_DEBUG, EditorCommandIds::VIEW_DEVELOPER_TOOLS_TEXTURE_DEBUG);
    mManager->addAccelerator(CM_VIEW_DEVELOPER_TOOLS_NODE_DEPS, EditorCommandIds::VIEW_DEVELOPER_TOOLS_NODE_DEPS);

    if (IGenEditorPlugin *p = curPlugin())
      p->registerMenuAccelerators();
  }

  ViewportWindow *viewport = ged.getCurrentViewport();
  if (viewport)
    viewport->registerViewportAccelerators(*mManager);
}


void AssetViewerApp::createAssetsTree() { mManager->setWindowType(nullptr, TREEVIEW_TYPE); }


void AssetViewerApp::createToolbar() { mManager->setWindowType(nullptr, TOOLBAR_TYPE); }


void AssetViewerApp::createThemeSwitcherToolbar() { mManager->setWindowType(nullptr, TOOLBAR_THEME_SWITCHER_TYPE); }


ModelessWindowControllerList AssetViewerApp::getModelessWindowControllers()
{
  ModelessWindowControllerList controllers;

  GenericEditorAppWindow::getModelessWindowControllers(controllers, ScreenshotDlgMode::ASSET_VIEWER);

  for (int i = 0; i < ged.getViewportCount(); ++i)
    ged.getViewport(i)->getModelessWindowControllers(controllers);

  controllers.addWindowController(*get_camera_object_config_modeless_dialog_handler());
  controllers.addWindowController(*environment::get_environment_settings_dialog_controller());
  controllers.addWindowController(*get_preferences_dialog_controller());
  controllers.addWindowController(*get_work_cycle_act_rate_dialog_controller());

  return controllers;
}


void AssetViewerApp::makeDefaultLayout(bool for_initial_layout)
{
  G_ASSERT(!makingDefaultLayout);
  makingDefaultLayout = true;

  dockPositionsInitialized = false;

  if (!for_initial_layout)
  {
    PropPanel::ImguiHelper::resetWindowLayout();
    getModelessWindowControllers().setDefaultState();
    editor_core_close_dag_imgui_windows();

    consoleCommandsAndVariableWindowsVisible = imgui_window_is_visible("", "Console commands and variables");
    getMainMenu()->setCheckById(CM_VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES, consoleCommandsAndVariableWindowsVisible);
  }

  asset_search_results_window.reset();

  mManager->reset();
  mManager->setWindowType(nullptr, VIEWPORT_TYPE);
  mManager->setWindowType(nullptr, TREEVIEW_TYPE);
  mManager->setWindowType(nullptr, PPANEL_TYPE);

  createToolbar();
  createThemeSwitcherToolbar();

  G_ASSERT(makingDefaultLayout);
  makingDefaultLayout = false;
}

void AssetViewerApp::saveLayout(const char *path)
{
  DataBlock blk;

  DataBlock *modelessWindowsBlk = blk.addBlock("modelessWindows");
  getModelessWindowControllers().saveState(*modelessWindowsBlk);

  editor_core_save_dag_imgui_windows(blk);

  DataBlock *imguiBlk = blk.addBlock("imgui");
  imguiBlk->setInt("dockSettingsVersion", LATEST_DOCK_SETTINGS_VERSION);

  const char *imguiSettings = ImGui::SaveIniSettingsToMemory();
  imguiBlk->setStr("imguiSettings", imguiSettings);

  if (!blk.saveToTextFile(path))
    logerr("Saving the window layout to \"%s\" has failed.", path);
}

bool AssetViewerApp::loadLayout(const DataBlock &blk)
{
  const DataBlock *imguiBlk = blk.getBlockByNameEx("imgui");
  const int loadedDockSettingsVersion = imguiBlk->getInt("dockSettingsVersion", 0);
  bool succeeded = false;
  if (loadedDockSettingsVersion == LATEST_DOCK_SETTINGS_VERSION)
  {
    const char *imguiSettings = imguiBlk->getStr("imguiSettings", nullptr);
    if (imguiSettings != nullptr && *imguiSettings != 0)
    {
      PropPanel::ImguiHelper::resetWindowLayout();
      ImGui::LoadIniSettingsFromMemory(imguiSettings);
      succeeded = true;
    }
  }
  else
  {
    logwarn("The saved window layout contains old dock setting version, the layout will not be restored.");
  }

  getModelessWindowControllers().loadState(*blk.getBlockByNameEx("modelessWindows"));
  editor_core_load_dag_imgui_windows(blk);

  consoleCommandsAndVariableWindowsVisible = imgui_window_is_visible("", "Console commands and variables");
  getMainMenu()->setCheckById(CM_VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES, consoleCommandsAndVariableWindowsVisible);

  return succeeded;
}

void AssetViewerApp::loadLastUsedLayout()
{
  const String windowLayoutBlkPath(assetlocalprops::makePath("_av_window_layout.blk"));
  DataBlock windowLayoutBlk;
  if (dd_file_exist(windowLayoutBlkPath) && windowLayoutBlk.load(windowLayoutBlkPath))
  {
    logdbg("Loading last used window layout from \"%s\".", windowLayoutBlkPath.c_str());
    dockPositionsInitialized = loadLayout(windowLayoutBlk);
  }
  else // TODO: remove the entire else block after a while. This code just loads layout from its old location.
  {
    const String imguiIniPath(assetlocalprops::makePath("_av_imgui.ini"));
    String imguiIniContents;
    if (dag_read_file_to_mem_str(imguiIniPath, imguiIniContents) &&
        strstr(imguiIniContents, String(0, "DockSettingsVersion=%d", LATEST_DOCK_SETTINGS_VERSION)))
    {
      logdbg("Loading layout from \"%s\".", imguiIniPath.c_str());
      windowLayoutBlk.clearData();
      windowLayoutBlk.addBlock("imgui")->setInt("dockSettingsVersion", LATEST_DOCK_SETTINGS_VERSION);
      windowLayoutBlk.addBlock("imgui")->setStr("imguiSettings", imguiIniContents);
      dockPositionsInitialized = loadLayout(windowLayoutBlk);
    }
  }

  lastUsedLayoutLoaded = true;
}

void AssetViewerApp::refillTree() { mTreeView->getAllAssetsTab().refillTree(); }

void AssetViewerApp::fillTree()
{
  mTreeView->setAssetMgr(assetMgr);

  DataBlock assetDataBlk(assetFile);
  mTreeView->getAllAssetsTab().fillTree(assetMgr, &assetDataBlk);
  ::dagor_idle_cycle();
  if (av_perform_uptodate_check)
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

    case TAGMANAGER_TYPE:
    {
      mTagManager = DAEDITOR3.getVisibleTagManagerWindow();

      getMainMenu()->setCheckById(CM_WINDOW_TAGMANAGER, (mTagManager != nullptr));

      return mTagManager;
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
        return makingDefaultLayout ? mToolPanel : nullptr;

      getMainMenu()->setEnabledById(CM_WINDOW_TOOLBAR, false);

      mToolPanel = new PropPanel::ToolbarContainerPropertyControl(0, this, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0));
      fillToolBar();

      return mToolPanel;
    }
    break;

    case TOOLBAR_PLUGIN_TYPE:
    {
      if (mPluginTool)
        return nullptr;

      mPluginTool = new PropPanel::ToolbarContainerPropertyControl(0, this, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0));

      return mPluginTool;
    }
    break;

    case TOOLBAR_THEME_SWITCHER_TYPE:
    {
      if (themeSwitcherToolPanel)
        return nullptr;

      themeSwitcherToolPanel = new PropPanel::ToolbarContainerPropertyControl(0, this, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0));
      PropPanel::ContainerPropertyControl *tb = themeSwitcherToolPanel->createToolbarPanel(0, "");
      tb->setAlignRightFromChild(tb->getChildCount());
      editor_command_system.createToolbarButton(*tb, CM_THEME_TOGGLE, EditorCommandIds::THEME_TOGGLE, "Toggle theme");
      updateThemeItems();
      return themeSwitcherToolPanel;
    }
    break;

    case VIEWPORT_TYPE:
    {
      getMainMenu()->setEnabledById(CM_WINDOW_VIEWPORT, false);

      AssetViewerViewportWindow *v = new AssetViewerViewportWindow();
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

  if (mTagManager && mTagManager == window)
  {
    mTagManager = nullptr;
    getMainMenu()->setCheckById(CM_WINDOW_TAGMANAGER, false);
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
    // Prevent recreating the toolbar when resetting the layout because we cannot easily restore the state of the
    // toolbar buttons.
    if (!makingDefaultLayout)
    {
      del_it(mToolPanel);
      getMainMenu()->setEnabledById(CM_WINDOW_TOOLBAR, true);
    }
    return true;
  }

  if (mPluginTool && mPluginTool == window)
  {
    del_it(mPluginTool);
    return true;
  }

  if (themeSwitcherToolPanel && themeSwitcherToolPanel == window)
  {
    del_it(themeSwitcherToolPanel);
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

void AssetViewerApp::showTagManager(bool show)
{
  if (show)
  {
    mManager->setWindowType(nullptr, TAGMANAGER_TYPE);
  }
  else
  {
    mManager->removeWindow(mTagManager);
  }
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
      grpCommon->createButton(ID_COPY_NAME, "Copy name to clipboard");
      grpCommon->createButton(ID_REVEAL_IN_EXPLORER, "Reveal in Explorer");
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

  GenericEditorAppWindow::fillCommonToolbar(*_tb, editor_command_system);

  _tb->createSeparator();

  editor_command_system.createToolbarToggleButton(*_tb, CM_COLLISION_PREVIEW, EditorCommandIds::COLLISION_PREVIEW,
    "Entity Collision Preview");
  _tb->setButtonPictures(CM_COLLISION_PREVIEW, "collision_preview");

  editor_command_system.createToolbarToggleButton(*_tb, CM_RENDER_GEOMETRY, EditorCommandIds::RENDER_GEOMETRY,
    "Entity Render Geometry");
  _tb->setButtonPictures(CM_RENDER_GEOMETRY, "render_entity_geom");

  _tb->createSeparator();

  editor_command_system.createToolbarToggleButton(*_tb, CM_AUTO_ZOOM_N_CENTER, EditorCommandIds::AUTO_ZOOM_N_CENTER,
    "Use auto-zoom-and-center");
  _tb->setButtonPictures(CM_AUTO_ZOOM_N_CENTER, "zoom_and_center");

  _tb->createSeparator();

  editor_command_system.createToolbarToggleButton(*_tb, CM_DISCARD_ASSET_TEX, EditorCommandIds::DISCARD_ASSET_TEX,
    "Discard textures (show stub tex)");
  _tb->setButtonPictures(CM_DISCARD_ASSET_TEX, "discard_tex");

  _tb->createSeparator();

  editor_command_system.createToolbarButton(*_tb, CM_PALETTE, EditorCommandIds::PALETTE, "Palette");
  _tb->setButtonPictures(CM_PALETTE, "palette");
  _tb->createSeparator();

  editor_command_system.createToolbarToggleButton(*_tb, CM_CONSOLE, EditorCommandIds::CONSOLE, "Show/hide console");
  _tb->setButtonPictures(CM_CONSOLE, "console");

  _tb->createSeparator();
  editor_command_system.createToolbarButton(*_tb, CM_VR_ENABLE, EditorCommandIds::VR_ENABLE, "Enable VR view");
  _tb->setButtonPictures(CM_VR_ENABLE, "vr");
}


void AssetViewerApp::fillMenu(PropPanel::IMenu *menu)
{
  menu->clearMenu(ROOT_MENU_ITEM);

  // fill file menu
  menu->addSubMenu(ROOT_MENU_ITEM, CM_FILE_MENU, "File");
  editor_command_system.addMenuItem(*menu, CM_FILE_MENU, CM_SAVE_CUR_ASSET, EditorCommandIds::SAVE_CUR_ASSET, "Save current asset");
  editor_command_system.addMenuItem(*menu, CM_FILE_MENU, CM_SAVE_ASSET_BASE, EditorCommandIds::SAVE_ASSET_BASE, "Save asset base");
  editor_command_system.addMenuItem(*menu, CM_FILE_MENU, CM_CHECK_ASSET_BASE, EditorCommandIds::CHECK_ASSET_BASE, "Check asset base");
  menu->addSeparator(CM_FILE_MENU);
  editor_command_system.addMenuItem(*menu, CM_FILE_MENU, CM_RELOAD_SHADERS, EditorCommandIds::RELOAD_SHADERS,
    "Reload shaders bindump");
  menu->addSeparator(CM_FILE_MENU);
  menu->addItem(CM_FILE_MENU, CM_EXIT, "Exit\tAlt+F4");

  // fill export menu
  menu->addSubMenu(ROOT_MENU_ITEM, CM_EXPORT, "Export");

  // fill settings
  menu->addSubMenu(ROOT_MENU_ITEM, CM_SETTINGS, "Settings");
  editor_command_system.addMenuItem(*menu, CM_SETTINGS, CM_OPTIONS_SET_ACT_RATE, EditorCommandIds::OPTIONS_SET_ACT_RATE,
    "Work cycle act rate...");
  editor_command_system.addMenuItem(*menu, CM_SETTINGS, CM_CAMERAS, EditorCommandIds::CAMERAS, "Cameras...");
  editor_command_system.addMenuItem(*menu, CM_SETTINGS, CM_SCREENSHOT, EditorCommandIds::SCREENSHOT, "Screenshot...");
  editor_command_system.addMenuItem(*menu, CM_SETTINGS, CM_OPTIONS_STAT_DISPLAY_SETTINGS,
    EditorCommandIds::OPTIONS_STAT_DISPLAY_SETTINGS, "Stats settings...");
  menu->addSeparator(CM_SETTINGS);
  editor_command_system.addMenuItem(*menu, CM_SETTINGS, CM_ENVIRONMENT_SETTINGS, EditorCommandIds::ENVIRONMENT_SETTINGS,
    "Environment settings...");
  menu->addSeparator(CM_SETTINGS);
  menu->addSubMenu(CM_SETTINGS, CM_SETTINGS_ASSET_BUILD_WARNINGS, "Asset build warning display");
  menu->addItem(CM_SETTINGS_ASSET_BUILD_WARNINGS, CM_SETTINGS_ASSET_BUILD_WARNINGS_SHOW_WHEN_BUILDING, "When building asset");
  menu->addItem(CM_SETTINGS_ASSET_BUILD_WARNINGS, CM_SETTINGS_ASSET_BUILD_WARNINGS_SHOW_ONCE_PER_SESSION, "Once per session");
  menu->addItem(CM_SETTINGS_ASSET_BUILD_WARNINGS, CM_SETTINGS_ASSET_BUILD_WARNINGS_SHOW_WHEN_SELECTED, "Every time asset is selected");
  editor_command_system.addMenuItem(*menu, CM_SETTINGS, CM_RENDER_GEOMETRY, EditorCommandIds::RENDER_GEOMETRY,
    "Entity render geometry");
  editor_command_system.addMenuItem(*menu, CM_SETTINGS, CM_COLLISION_PREVIEW, EditorCommandIds::COLLISION_PREVIEW,
    "Entity collision preview");
  editor_command_system.addMenuItem(*menu, CM_SETTINGS, CM_AUTO_ZOOM_N_CENTER, EditorCommandIds::AUTO_ZOOM_N_CENTER,
    "Use auto-zoom-and-center");
  menu->addItem(CM_SETTINGS, CM_AUTO_SHOW_COMPOSITE_EDITOR, "Automatically toggle Composit Editor");
  menu->addItem(CM_SETTINGS, CM_SETTINGS_ENABLE_DEVELOPER_TOOLS, "Enable developer tools");
  menu->addSubMenu(CM_SETTINGS, CM_PREFERENCES, "Preferences");
  menu->addItem(CM_PREFERENCES, CM_PREFERENCES_ASSET_TREE, "Asset Tree");
  menu->setEnabledById(CM_PREFERENCES_ASSET_TREE, false);
  menu->addItem(CM_PREFERENCES, CM_PREFERENCES_ASSET_TREE_COPY_SUBMENU, "Move \"Copy\" to submenu");
  editor_command_system.addMenuItem(*menu, CM_SETTINGS, CM_OPTIONS_EDIT_PREFERENCES, EditorCommandIds::OPTIONS_EDIT_PREFERENCES,
    "Toolbar preferences..."); // The name is temporary, this will be the general preferences window.

  // fill view menu
  menu->addSubMenu(ROOT_MENU_ITEM, CM_WINDOW, "View");
  editor_command_system.addMenuItem(*menu, CM_WINDOW, CM_LOAD_DEFAULT_LAYOUT, EditorCommandIds::LOAD_DEFAULT_LAYOUT,
    "Load default layout");
  menu->addSeparator(CM_WINDOW);
  editor_command_system.addMenuItem(*menu, CM_WINDOW, CM_LOAD_LAYOUT, EditorCommandIds::LOAD_LAYOUT, "Load layout ...");
  editor_command_system.addMenuItem(*menu, CM_WINDOW, CM_SAVE_LAYOUT, EditorCommandIds::SAVE_LAYOUT, "Save layout ...");
  menu->addSeparator(CM_WINDOW);
  menu->addItem(CM_WINDOW, CM_WINDOW_TOOLBAR, "Toolbar");
  menu->addItem(CM_WINDOW, CM_WINDOW_TREE, "Tree");
  editor_command_system.addMenuItem(*menu, CM_WINDOW, CM_WINDOW_TAGMANAGER, EditorCommandIds::TOGGLE_TAG_MANAGER, "Tags");
  editor_command_system.addMenuItem(*menu, CM_WINDOW, CM_WINDOW_PPANEL, EditorCommandIds::TOGGLE_PROPERTY_PANEL, "Properties");
  menu->addItem(CM_WINDOW, CM_WINDOW_VIEWPORT, "Viewport");
  editor_command_system.addMenuItem(*menu, CM_WINDOW, CM_WINDOW_COMPOSITE_EDITOR, EditorCommandIds::COMPOSITE_EDITOR,
    "Composit Editor");
  menu->addItem(CM_WINDOW, CM_WINDOW_DABUILD, "daBuild");
  editor_command_system.addMenuItem(*menu, CM_WINDOW, CM_DISCARD_ASSET_TEX, EditorCommandIds::DISCARD_ASSET_TEX,
    "Discard textures (show stub tex)");
  menu->addSeparator(CM_WINDOW);
  menu->addSubMenu(CM_WINDOW, CM_VIEW_DEVELOPER_TOOLS, "Developer tools");
  editor_command_system.addMenuItem(*menu, CM_VIEW_DEVELOPER_TOOLS, CM_VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES,
    EditorCommandIds::VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES, "Console commands and variables");
  editor_command_system.addMenuItem(*menu, CM_VIEW_DEVELOPER_TOOLS, CM_VIEW_DEVELOPER_TOOLS_IMGUI_DEBUGGER,
    EditorCommandIds::VIEW_DEVELOPER_TOOLS_IMGUI_DEBUGGER, "ImGui debugger");
  editor_command_system.addMenuItem(*menu, CM_VIEW_DEVELOPER_TOOLS, CM_VIEW_DEVELOPER_TOOLS_TOAST_MANAGER,
    EditorCommandIds::VIEW_DEVELOPER_TOOLS_TOAST_MANAGER, "Toast manager");
  editor_command_system.addMenuItem(*menu, CM_VIEW_DEVELOPER_TOOLS, CM_VIEW_DEVELOPER_TOOLS_TEXTURE_DEBUG,
    EditorCommandIds::VIEW_DEVELOPER_TOOLS_TEXTURE_DEBUG, "Texture debug");
  editor_command_system.addMenuItem(*menu, CM_VIEW_DEVELOPER_TOOLS, CM_VIEW_DEVELOPER_TOOLS_NODE_DEPS,
    EditorCommandIds::VIEW_DEVELOPER_TOOLS_NODE_DEPS, "Node dependencies");
  menu->addSeparator(CM_WINDOW);
  menu->addSubMenu(CM_WINDOW, CM_THEME, "Theme");
  menu->addItem(CM_THEME, CM_THEME_LIGHT, "Light");
  menu->addItem(CM_THEME, CM_THEME_DARK, "Dark");
}


void AssetViewerApp::updateMenu(PropPanel::IMenu *menu)
{
  // fill export menu
  if (impostorApp)
  {
    editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_EXPORT_CURRENT_IMPOSTOR, EditorCommandIds::EXPORT_CURRENT_IMPOSTOR,
      "Generate current impostor");
    editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_EXPORT_IMPOSTORS_CURRENT_PACK,
      EditorCommandIds::EXPORT_IMPOSTORS_CURRENT_PACK, "Generate impostors from current pack");
    editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_EXPORT_ALL_IMPOSTORS, EditorCommandIds::EXPORT_ALL_IMPOSTORS,
      "Generate all impostors");
    editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_CLEAR_UNUSED_IMPOSTORS, EditorCommandIds::CLEAR_UNUSED_IMPOSTORS,
      "Clear unused impostors");
  }
  if (pointCloudGen)
    editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_EXPORT_CURRENT_POINT_CLOUD, EditorCommandIds::EXPORT_CURRENT_POINT_CLOUD,
      "Generate current point cloud");

  editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_BUILD_RESOURCES, EditorCommandIds::BUILD_RESOURCES, "Export gameRes [pc]");
  editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_BUILD_TEXTURES, EditorCommandIds::BUILD_TEXTURES, "Export texPack [pc]");
  menu->addSeparator(CM_EXPORT);
  editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_BUILD_ALL, EditorCommandIds::BUILD_ALL, "Export all [pc]");
  menu->addSeparator(CM_EXPORT);
  editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_BUILD_CLEAR_CACHE, EditorCommandIds::BUILD_CLEAR_CACHE, "Clear cache [pc]");

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
  editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_BUILD_ALL_PLATFORM_RES, EditorCommandIds::BUILD_ALL_PLATFORM_RES,
    "Export gameRes for All platform");
  editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_BUILD_ALL_PLATFORM_TEX, EditorCommandIds::BUILD_ALL_PLATFORM_TEX,
    "Export texPack for All platform");
  editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_BUILD_ALL_PLATFORM, EditorCommandIds::BUILD_ALL_PLATFORM,
    "Export All for All platform");
  menu->addSeparator(CM_EXPORT);
  editor_command_system.addMenuItem(*menu, CM_EXPORT, CM_BUILD_CLEAR_CACHE_ALL, EditorCommandIds::BUILD_CLEAR_CACHE_ALL,
    "Clear cache for All platform");
}

void AssetViewerApp::updateAssetBuildWarningsMenu()
{
  getMainMenu()->setCheckById(CM_SETTINGS_ASSET_BUILD_WARNINGS_SHOW_WHEN_BUILDING,
    assetBuildWarningDisplay == AssetBuildWarningDisplay::ShowWhenBuilding);
  getMainMenu()->setCheckById(CM_SETTINGS_ASSET_BUILD_WARNINGS_SHOW_ONCE_PER_SESSION,
    assetBuildWarningDisplay == AssetBuildWarningDisplay::ShowOncePerSession);
  getMainMenu()->setCheckById(CM_SETTINGS_ASSET_BUILD_WARNINGS_SHOW_WHEN_SELECTED,
    assetBuildWarningDisplay == AssetBuildWarningDisplay::ShowWhenSelected);
}

void AssetViewerApp::updateThemeItems()
{
  const bool isDefault = themeName == defaultThemeName;
  const unsigned int id = isDefault ? CM_THEME_LIGHT : CM_THEME_DARK;
  const char *pic = isDefault ? "theme_light" : "theme_dark";
  getMainMenu()->setRadioById(id, CM_THEME_LIGHT, CM_THEME_DARK);
  if (themeSwitcherToolPanel)
    themeSwitcherToolPanel->setButtonPictures(CM_THEME_TOGGLE, pic);
}

void AssetViewerApp::drawAssetInformation(IGenViewportWnd *wnd)
{
  using hdpi::_pxS;

  AsyncTextureLoadingState ld_state;
  static int64_t ld_reft = 0;
  if (texconvcache::get_tex_factory_current_loading_state(ld_state) && //
      (ld_state.pendingLdTotal > 0 || (ld_reft && get_time_usec(ld_reft) < 3000000)))
  {
    using hdpi::_px;
    hdpi::Px rx, ry;
    static_cast<ViewportWindow *>(wnd)->getMenuAreaSize(rx, ry);

    String text;
    if (ld_state.pendingLdTotal > 0)
    {
      const unsigned *c = ld_state.pendingLdCount;
      text.printf(0, "Loading textures: %d pending [%d %s], %d tex loaded, %d tex updated", //
        ld_state.pendingLdTotal, c[0] ? c[0] : (c[1] ? c[1] : (c[2] ? c[2] : c[3])),
        c[0] ? "TQ" : (c[1] ? "BQ" : (c[2] ? "HQ" : "UHQ")), ld_state.totalLoadedCount, ld_state.totalCacheUpdated);
      ld_reft = ref_time_ticks();
    }
    else
      text.printf(0, "All textures loaded: %d tex (%dK read), updated %d tex", //
        ld_state.totalLoadedCount, ld_state.totalLoadedSizeKb, ld_state.totalCacheUpdated);

    StdGuiRender::set_font(0);
    Point2 ts = StdGuiRender::get_str_bbox(text).size();

    StdGuiRender::set_color(ld_state.pendingLdTotal > 0 ? COLOR_RED : COLOR_GREEN);
    StdGuiRender::render_box(_px(rx), 0, _px(rx) + ts.x + _pxS(6), _px(ry));

    StdGuiRender::set_color(COLOR_WHITE);
    StdGuiRender::draw_strf_to(_px(rx) + _pxS(3), _pxS(17), text);
  }

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
  StdGuiRender::set_color(!av_perform_uptodate_check ? COLOR_YELLOW : is_pc ? COLOR_LTGREEN : COLOR_LTRED);

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
      StdGuiRender::set_color(!av_perform_uptodate_check ? COLOR_YELLOW : is_iOS ? COLOR_LTGREEN : COLOR_LTRED);

      StdGuiRender::render_box(rc - pSize - offset.x - sz, 0, rc - offset.x - sz, r.t + offset.y + textOffset / 2);

      StdGuiRender::set_color(COLOR_BLACK);
      StdGuiRender::draw_strf_to(rc - pSize - offset.x - sz + textOffset, r.t + offset.y, "iOS");
    }
    else if (p == _MAKE4C('and'))
    {
      bool is_AND = curAsset->testUserFlags(ASSET_USER_FLG_UP_TO_DATE_AND);
      StdGuiRender::set_color(!av_perform_uptodate_check ? COLOR_YELLOW : is_AND ? COLOR_LTGREEN : COLOR_LTRED);

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
    refProvider->getAssetRefs(*const_cast<DagorAsset *>(amain), refs);
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

static void gatherTexRefs(DagorAsset *a, TextureIdSet &reftex, ska::flat_hash_set<const DagorAsset *> &visited_assets)
{
  if (!visited_assets.emplace(a).second) // second = inserted
    return;

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
  refProvider->getAssetRefs(*a, refs);
  for (int i = 0; i < refs.size(); ++i)
    if (DagorAsset *ra = refs[i].getAsset())
      gatherTexRefs(ra, reftex, visited_assets);
}
void AssetViewerApp::applyDiscardAssetTexMode()
{
  if (!discardAssetTexMode && curMaxTexQL == TQL_uhq)
    texconvcache::build_on_demand_tex_factory_limit_tql(TQL_uhq);
  else if (curAsset)
  {
    TextureIdSet reftex;
    ska::flat_hash_set<const DagorAsset *> visitedAssets;
    gatherTexRefs(curAsset, reftex, visitedAssets);
    texconvcache::build_on_demand_tex_factory_limit_tql(discardAssetTexMode ? TQL_thumb : curMaxTexQL, reftex);
  }
}
void AssetViewerApp::setCurrentTexQualityLimit(TexQL ql)
{
  if (curMaxTexQL == ql)
    return;
  curMaxTexQL = ql;
  applyDiscardAssetTexMode();
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
  const bool nameAsAsset = (cube ? cubeScreenshotCfg.nameAsCurrentAsset : screenshotCfg.nameAsCurrentAsset) && curAsset;
  return ::make_full_path(sdk_dir, String(0, "av_%s_%%03i%s", nameAsAsset ? curAsset->getName() : (cube ? "cube" : "scrn"),
                                     getFileExtFromFormat(cube ? cubeScreenshotCfg.format : screenshotCfg.format)));
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
    text.printf(300, "Asset viewer2  [%s%c%c%c%c]  - %s", useDngBasedSceneRender ? "dngBasedRender | " : "", _DUMP4C(drv), sceneFname);
  else
    text.printf(300, "Asset viewer2  %s  [%s%c%c%c%c]", build_version, useDngBasedSceneRender ? "dngBasedRender | " : "",
      _DUMP4C(drv));
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
  if (curAsset)
  {
    assetDataBlk.setStr("lastSelAsset", curAsset->getName());
    assetDataBlk.setStr("lastSelAssetType", curAsset->getTypeStr());
  }
  assetDataBlk.setStr("a2d_lastUsedModelAsset", a2d_last_ref_model);

  if (mPropPanel)
    mPropPanel->saveState(*assetDataBlk.addBlock("propertyPanelState"), true);

  DataBlock &setBlk = *assetDataBlk.addBlock("setting");
  setBlk.setBool("CollisionPreview", (getRendSubTypeMask() & collisionMask) == collisionMask);
  setBlk.setBool("autoZoomAndCenter", autoZoomAndCenter);
  setBlk.setBool("AutoShowCompositeEditor", compositeEditor.autoShow);
  setBlk.setBool("DeveloperToolsEnabled", developerToolsEnabled);
  setBlk.setBool("DaBuildWindowVisible", dabuildWindowVisible);
  setBlk.setBool("DaBuildWindowAutoOpen", daBuildWindowAutoOpen);
  setBlk.setBool("DaBuildWindowAutoClose", daBuildWindowAutoClose);
  setBlk.setInt("ToolbarScalePercent", getToolbarScalePercent());

  if (assetBuildWarningDisplay == AssetBuildWarningDisplay::ShowOncePerSession)
    setBlk.setStr("AssetBuildWarningDisplay", "ShowOncePerSession");
  else if (assetBuildWarningDisplay == AssetBuildWarningDisplay::ShowWhenSelected)
    setBlk.setStr("AssetBuildWarningDisplay", "ShowWhenSelected");

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

  ged.save(assetDataBlk);
  grid.save(assetDataBlk);
  saveScreenshotSettings(assetDataBlk);
  getConsole().saveCmdHistory(assetDataBlk);
  AssetSelectorGlobalState::save(assetDataBlk);

  DataBlock *assetBrowserBlk = assetDataBlk.addBlock("assetBrowser");
  mTreeView->saveAssetBrowserSettings(*assetBrowserBlk->addBlock("assetsTree"));
  assetBrowserBlk->addNewBlock(&asset_browser_modal_settings, "assetSelectorDialogModal");

  editor_core_save_dag_imgui_blk_settings(assetDataBlk);

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

// global AV settings (that applies to all workspaces)
void AssetViewerApp::loadGlobalSettings()
{
  DataBlock globalSettingsBlk;
  globalSettingsBlk.load(get_global_av_settings_file_path());
  loadThemeSettings(globalSettingsBlk);
  GizmoSettings::load(globalSettingsBlk);
}

// global AV settings (that applies to all workspaces)
void AssetViewerApp::saveGlobalSettings()
{
  DataBlock globalSettingsBlk;
  editor_core_save_imgui_color_editor_options(globalSettingsBlk);
  saveThemeSettings(globalSettingsBlk);
  GizmoSettings::save(globalSettingsBlk);
  globalSettingsBlk.saveToTextFile(get_global_av_settings_file_path());

  DataBlock hotkeysBlk;
  editor_command_system.saveChangedHotkeys(hotkeysBlk);
  hotkeysBlk.saveToTextFile(get_global_av_hotkey_settings_file_path());
}

void AssetViewerApp::onClosing()
{
  if (!assetFile.empty()) // If a workspace has been selected and Asset Viewer has been "fully" started.
  {
    DAEDITOR3.saveAssetsTags();

    if (dockPositionsInitialized)
      saveLayout(assetlocalprops::makePath("_av_window_layout.blk"));
  }

  saveTreeState();
  saveGlobalSettings();
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

  if (ret == wingw::MB_ID_CANCEL)
  {
    if (sup)
    {
      switchToPlugin(getPluginIdx(sup));
      fillPropPanel();
    }
    return false;
  }

  if (ret == wingw::MB_ID_YES)
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
  void onAssetMgrMessage(int msg_type, const char *msg, DagorAsset *asset, const char *asset_src_fpath) override
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
  EDITORCORE->getWndManager()->init3d(av2_drv_name, nullptr, INITIAL_CAPTION, INITIAL_ICON);

  const char *commonData = sgg::get_common_data_dir();
  ::startup_shaders(String(260, "%s/guiShaders", commonData));
  ::startup_game(RESTART_ALL);
  ShaderGlobal::enableAutoBlockChange(true);

  DataBlock fontsBlk;
  fontsBlk.addBlock("fontbins")->addStr("name", String(260, "%s/default", commonData));
  if (auto *b = fontsBlk.addBlock("dynamicGen"))
  {
    b->setInt("texCount", 2);
    b->setInt("texSz", 256);
    b->setStr("prefix", commonData);
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

  // Shut down shaders explicitly because it would restart earlier than the d3d driver, and shaders expect d3d to be inited.
  shutdown_shaders();

  // By setting window_initing to true we prevent the destruction of the main window mark the application inactive in main_wnd_proc().
  // window_initing will be set true when the device is recreated by init3d(), so the application would remain in erroneous inactive
  // state without this prevention.
  workcycle_internal::window_initing = true;
  shutdown_game(RESTART_VIDEODRV | RESTART_VIDEO);
  workcycle_internal::window_initing = false;

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
  ::load_tex_streaming_settings(::get_app().getWorkspace().getAppBlkPath(), &texStreamingBlk);
  EDITORCORE->getWndManager()->init3d(av2_drv_name, &texStreamingBlk, INITIAL_CAPTION, INITIAL_ICON);
  if (int resv_tid_count = appblk.getInt("texMgrReserveTIDcount", 128 << 10))
  {
    enable_res_mgr_mt(false, 0);
    enable_res_mgr_mt(true, resv_tid_count);
  }

  ::startup_shaders(appblk.getStr("shadersAbs", "compiledShaders/tools"));

  extern void console_init();
  console_init();
  add_con_proc(new AssetViewerConsoleCommandProcessor(10000)); // Let Asset Viewer override all other commands.
  ::shaders_register_console(true);
  ::startup_game(RESTART_ALL);
  ShaderGlobal::enableAutoBlockChange(true);

  ShaderGlobal::set_int(get_shader_variable_id("dgs_tex_anisotropy", true), ::dgs_tex_anisotropy);
  ShaderGlobal::set_float(get_shader_variable_id("mip_bias", true), 0.f);

  if (const char *gp_file = appblk.getBlockByNameEx("game")->getStr("params", NULL))
  {
    DataBlock *b = const_cast<DataBlock *>(dgs_get_game_params());
    b->load(String(0, "%s/%s", appdir, gp_file));
    b->removeBlock("rendinstExtra");
    b->removeBlock("rendinstOpt");
  }
  const_cast<DataBlock *>(dgs_get_game_params())->setBool("rendinstExtraAutoSubst", false);
  const_cast<DataBlock *>(dgs_get_game_params())->setInt("rendinstExtraMaxCnt", 4096);

  const char *commonData = sgg::get_common_data_dir();

  DataBlock fontsBlk;
  fontsBlk.addBlock("fontbins")->addStr("name", String(260, "%s/default", commonData));
  if (auto *b = fontsBlk.addBlock("dynamicGen"))
  {
    b->setInt("texCount", 2);
    b->setInt("texSz", 256);
    b->setStr("prefix", commonData);
  }
  StdGuiRender::init_dynamic_buffers(appblk.getInt("guiMaxQuad", 256 << 10), appblk.getInt("guiMaxVert", 64 << 10));
  StdGuiRender::init_fonts(fontsBlk);
  StdGuiRender::init_render();
  StdGuiRender::set_def_font_ht(0, hdpi::_pxS(StdGuiRender::get_initial_font_ht(0)));

  bool useDngBasedSceneRender = get_app().dngBasedSceneRenderUsed();
  if (!useDngBasedSceneRender && appblk.getBool("useDynrend", false))
    dynrend::init();

  ::register_dynmodel_gameres_factory();
  ::register_rendinst_gameres_factory();
  ::register_geom_node_tree_gameres_factory();
  ::register_phys_sys_gameres_factory();
  ::register_phys_obj_gameres_factory();
  ::register_phys_car_gameres_factory();
  CollisionResource::registerFactory();

  if (!useDngBasedSceneRender)
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
      String mnt(0, "%s/%s", ::get_app().getWorkspace().getSdkDir(), b.getStr(p));
      simplify_fname(mnt);
      dd_set_named_mount_path(b.getBlockName(), mnt);
    });
    dd_dump_named_mounts();
  }
}
bool AssetViewerApp::loadProject(const char *app_dir)
{
  ec_set_busy(true);
  ::dagor_idle_cycle();

  String appDirString = make_path_absolute(app_dir);
  G_ASSERT(!appDirString.empty());
  append_slash(appDirString);
  app_dir = appDirString.c_str();

  DataBlock appblk;

  assetMgr.clear();
  assetMgr.setMsgPipe(&conPipe);

  const char *app_blk_path = ::get_app().getWorkspace().getAppBlkPath();
  if (!appblk.load(app_blk_path))
  {
    debug("cannot load %s", app_blk_path);
    ec_set_busy(false);
    return false;
  }

  bool patch_build = appblk.getBool("av_patch_build", true);
  dabuildcache::get_dabuild()->allowPatchBuild(patch_build);
  DAEDITOR3.conNote("patch build is %s", patch_build ? "enabled" : "disabled");
  av_perform_uptodate_check =
    appblk.getBlockByNameEx("assets")->getBool("avPerformUpToDateChecks", true) && !dgs_get_argv("skip_check");

  matParamsPath = appblk.getBlockByNameEx("game")->getStr("mat_params", "");
  appblk.setStr("appDir", app_dir);
  useDngBasedSceneRender = appblk.getBlockByNameEx("game")->getBool("daNetGameRender", false);

  appblk.setStr("shadersAbs", tools3d::get_shaders_path(appblk, app_dir, useDngBasedSceneRender));

  read_mount_points(appblk);


  init3d(appblk);
  assetlocalprops::init(app_dir, "develop/.asset-local");

  const DataBlock &debugSettings = *dgs_get_settings()->getBlockByNameEx("debug");
  const DataBlock *profiler = debugSettings.getBlockByName("profiler");
  da_profiler::set_profiling_settings(debugSettings);
  if (profiler && profiler->getBool("auto_dump", false))
    da_profiler::start_file_dump_server("");

  editor_core_initialize_imgui();
  editor_core_load_imgui_theme(getThemeFileName());

  DataBlock globalSettingsBlk;
  globalSettingsBlk.load(get_global_av_settings_file_path());
  editor_core_load_imgui_color_editor_options(globalSettingsBlk);

  DataBlock::setRootIncludeResolver(app_dir);

  ::dagor_idle_cycle();
  ::dagor_set_game_act_rate(appblk.getInt("work_cycle_act_rate", 100));

  strcpy(sceneFname, app_blk_path);
  setDocTitle();

  const char *fx_nut = appblk.getBlockByNameEx("assets")->getStr("fxScriptsDir", NULL);
  if (fx_nut)
    fx_devres_base_path.printf(260, "%s/%s/", app_dir, fx_nut);

  // detect new gameres system model and setup engine support for it
  const DataBlock *exp_blk = appblk.getBlockByNameEx("assets")->getBlockByName("export");
  bool loadGameResPacks = appblk.getBlockByNameEx("assets")->getStr("gameRes", NULL) != NULL;
  bool loadDDSxPacks = appblk.getBlockByNameEx("assets")->getStr("ddsxPacks", NULL) != NULL;

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
    int game_dir_prefix_len = (int)strlen(game_dir);

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
        FastNameMapEx pkg_list;

        const DataBlock &pkgBlk = *expBlk.getBlockByNameEx("packages");
        const DataBlock &skipBuilt = *expBlk.getBlockByNameEx("skipBuilt");
        bool no_pkg_scan = pkgBlk.getBool("dontScanOtherFolders", false);

        for (int i = 0; i < pkgBlk.blockCount(); i++)
          pkg_list.addNameId(pkgBlk.getBlock(i)->getBlockName());

        for (const alefind_t &ff : dd_find_iterator(base + "*.*", DA_SUBDIR))
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
            String mnt(mntPoint);
            ::add_vromfs(vrom, false, mnt.str());

            ::load_res_packs_from_list(mntPoint + packlist, loadGameResPacks, loadDDSxPacks, mntPoint);
            G_VERIFY(::remove_vromfs(vrom) == mnt.str());
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

  if (exp_blk)
    texconvcache::init_build_on_demand_tex_factory(assetMgr, console);

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
      assetMgr.loadAssetsBase(String::mk_str_cat(app_dir, blk.getStr(i)), "global");
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
  gamereshooks::on_gameres_pack_load_confirm = &disable_gameres_loading;

  if (blk.getStr("gameRes", NULL) || blk.getStr("ddsxPacks", NULL))
    assetMgr.mountBuiltGameResEx(scannedResBlk, *skipTypes);

  if (const char *efx_fname = appblk.getBlockByNameEx("assets")->getStr("efxBlk", NULL))
    mount_efx_assets(assetMgr, String(0, "%s/%s", app_dir, efx_fname));

  if (get_max_managed_texture_id())
    debug("tex/res registered before AssetBase: %d", get_max_managed_texture_id().index());

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

  log_invalid_asset_names(appblk, assetMgr.getAssets());

  int pc = ::bind_dabuild_cache_with_mgr(assetMgr, appblk, app_dir);
  if (pc < 0)
    console->addMessage(console->ERROR, "cannot init daBuild");
  else if (pc == 0)
    console->addMessage(console->WARNING, "daBuild inited but is useless, due to there are no asset types for export");

  makeDefaultLayout(/*for_initial_layout = */ true);
  ::dagor_idle_cycle();
  G_ASSERTF_RETURN(mTreeView, false, "Failed to create AssetViewer windows.\nTry removing .asset-local/_av_window_layout.blk");

  load_suboptimal_asset_default_search_options(appblk);

  // init dynamic, classic or daNetGame-based renderer
  EDITORCORE->queryEditorInterface<IDynRenderService>()->setup(app_dir, appblk);
  EDITORCORE->queryEditorInterface<IDynRenderService>()->init();
  EDITORCORE->queryEditorInterface<IDynRenderService>()->setRenderOptEnabled(IDynRenderService::ROPT_SHADOWS_VSM, false);

  // apply AV setting
  DataBlock assetDataBlk;
  assetFile = assetlocalprops::makePath("_av.blk");
  assetDataBlk.load(assetFile);

  ged.load(assetDataBlk);
  grid.load(assetDataBlk);
  editor_core_load_dag_imgui_blk_settings(assetDataBlk);
  loadScreenshotSettings(assetDataBlk);
  getConsole().loadCmdHistory(assetDataBlk);
  AssetSelectorGlobalState::load(assetDataBlk);
  propPanelStateOfTheAssetToInitiallySelect = *assetDataBlk.getBlockByNameEx("propertyPanelState");

  if (DataBlock *assetBrowserBlk = assetDataBlk.getBlockByName("assetBrowser"))
  {
    if (DataBlock *assetsTreeBlk = assetBrowserBlk->getBlockByName("assetsTree"))
      mTreeView->loadAssetBrowserSettings(*assetsTreeBlk);
    if (DataBlock *assetSelectorDialogModalBlk = assetBrowserBlk->getBlockByName("assetSelectorDialogModal"))
      asset_browser_modal_settings = *assetSelectorDialogModalBlk;
  }

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

  if (!assettags::load(assetMgr))
    console->addMessage(console->WARNING, "Failed to load asset tags!");

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

  developerToolsEnabled = setBlk.getBool("DeveloperToolsEnabled", developerToolsEnabled);
  getMainMenu()->setCheckById(CM_SETTINGS_ENABLE_DEVELOPER_TOOLS, developerToolsEnabled);
  getMainMenu()->setEnabledById(CM_VIEW_DEVELOPER_TOOLS, developerToolsEnabled);

  daBuildWindowAutoOpen = setBlk.getBool("DaBuildWindowAutoOpen", daBuildWindowAutoOpen);
  daBuildWindowAutoClose = setBlk.getBool("DaBuildWindowAutoClose", daBuildWindowAutoClose);
  dabuildWindowVisible = setBlk.getBool("DaBuildWindowVisible", dabuildWindowVisible);
  getMainMenu()->setCheckById(CM_WINDOW_DABUILD, dabuildWindowVisible);

  if (strcmp(setBlk.getStr("AssetBuildWarningDisplay", ""), "ShowOncePerSession") == 0)
    assetBuildWarningDisplay = AssetBuildWarningDisplay::ShowOncePerSession;
  else if (strcmp(setBlk.getStr("AssetBuildWarningDisplay", ""), "ShowWhenSelected") == 0)
    assetBuildWarningDisplay = AssetBuildWarningDisplay::ShowWhenSelected;
  updateAssetBuildWarningsMenu();

  const bool moveCopyToSubmenu = AssetSelectorGlobalState::getMoveCopyToSubmenu();
  getMainMenu()->setCheckById(CM_PREFERENCES_ASSET_TREE_COPY_SUBMENU, moveCopyToSubmenu);

  setToolbarScalePercent(setBlk.getInt("ToolbarScalePercent", 100));

  for (int i = 0; i < plugin.size(); ++i)
    plugin[i]->onLoadLibrary();
  ::dagor_idle_cycle();

  console->endLogAndProgress();

  queryEditorInterface<ISceneLightService>()->reset();

  TEXTUREID lightmapTexId = add_managed_texture(String(260, "%s%s", sgg::get_exe_path_full(), "../commonData/tex/lmesh_ltmap.tga"));
  ShaderGlobal::set_texture_fast(get_shader_glob_var_id("lightmap_tex", true), lightmapTexId);
  ShaderGlobal::set_sampler(get_shader_glob_var_id("lightmap_tex_samplerstate", true), get_texture_separate_sampler(lightmapTexId));

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

  // There could have been multiple startLog()/endLog() calls (that reset the non-global counters) while we reach this
  // point, so we have to check the global counters here.
  if (console->getGlobalFatalsCounter() == 0 && console->getGlobalErrorsCounter() == 0 && console->getGlobalWarningsCounter() == 0)
    console->hideConsole();

#define INIT_SERVICE(TYPE_NAME, EXPR)          \
  if (assetMgr.getAssetTypeId(TYPE_NAME) >= 0) \
    EXPR;                                      \
  else                                         \
    debug("skipped initing service for \"%s\", asset manager did not declare such type", TYPE_NAME);

  INIT_SERVICE("rendInst", ::init_rimgr_service(appblk));
  INIT_SERVICE("prefab", ::init_prefabmgr_service(appblk));
  INIT_SERVICE("fx", ::init_fxmgr_service(appblk));
  INIT_SERVICE("efx", ::init_efxmgr_service());
  INIT_SERVICE("composit", ::init_composit_mgr_service(appblk));
  INIT_SERVICE("physObj", ::init_physobj_mgr_service());
  INIT_SERVICE("gameObj", ::init_gameobj_mgr_service());
  ::init_invalid_entity_service();
  INIT_SERVICE("animChar", useDngBasedSceneRender ? ::init_ecs_animchar_mgr_service(appblk) : ::init_animchar_mgr_service(appblk));
  INIT_SERVICE("dynModel", ::init_dynmodel_mgr_service(appblk));
  INIT_SERVICE("csg", ::init_csg_mgr_service());
  INIT_SERVICE("spline", ::init_spline_gen_mgr_service());
  if (!useDngBasedSceneRender)
    ::init_ecs_mgr_service(appblk, getWorkspace().getAppDir());
  if (appblk.getBlockByNameEx("game")->getBool("enable_webui_av2", true) && !useDngBasedSceneRender)
    ::init_webui_service();
  else
    debug("webUi disabled with game{ enable_webui_av2:b=no;");

  init_das_mgr_service(appblk, useDngBasedSceneRender);

  // scan templates and add them as assets if needed after templates are loaded
  if (useDngBasedSceneRender && assetMgr.getAssetTypeId("ecs_template") >= 0)
  {
    perform_delayed_actions();
    mount_ecs_template_assets(assetMgr);
  }

  ::init_interface_de3();
  ::init_all_editor_plugins(appblk);

#undef INIT_SERVICE

  fillTree();

  assetLtData.createSun();
  assetDefaultLtData.loadDefaultSettings(appblk);

  bool needRenderGrid = false;
  environment::load_settings(assetDataBlk, &assetLtData, &assetDefaultLtData, needRenderGrid);
  grid.setVisible(needRenderGrid, 0);

  assetLtData.setReflectionTexture();
  assetLtData.setEnvironmentTexture();
  assetLtData.applySettingsFromLevelBlk();

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

  ec_set_busy(false);
  queryEditorInterface<IDynRenderService>()->enableRender(true);
  queryEditorInterface<IDynRenderService>()->selectAsGameScene();

  if (useDngBasedSceneRender)
    assetLtData.setPaintDetailsTexture();
  return true;
}


bool AssetViewerApp::saveProject(const char *)
{
  if (blockSave)
    return true;

  console->showConsole();
  console->startLog();
  console->startProgress();

  for (int i = 0; i < plugin.size(); ++i)
    plugin[i]->onSaveLibrary();

  assetMgr.saveAssetsMetadata();
  DAEDITOR3.saveAssetsTags();

  bool showCon = (console->hasErrors() || console->hasWarnings());

  console->endLogAndProgress();

  if (showCon)
    console->showConsole();

  return true;
}


void AssetViewerApp::blockModifyRoutine(bool block) { blockSave = block; }


void AssetViewerApp::afterUpToDateCheck(bool)
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

void AssetViewerApp::onClick(int pcb_id, [[maybe_unused]] PropPanel::ContainerPropertyControl *panel)
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
    case ID_RUN_EDIT:
      if (curAsset)
        runExternalEditor(*curAsset);
      return;
    case ID_COPY_NAME:
      if (curAsset)
        clipboard::set_clipboard_ansi_text(curAsset->getName());
      return;
    case ID_REVEAL_IN_EXPLORER:
      if (curAsset)
        AssetSelectorCommon::revealInExplorer(*curAsset);
      return;

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


static void create_folder_idx_list(AllAssetsTree *tree, bool /*hierarchical*/, Tab<int> &fld_idx)
{
  PropPanel::TLeafHandle selItem = tree->getSelectedItem();

  Tab<PropPanel::TLeafHandle> sel(tmpmem);

  sel.push_back(selItem);
  ::make_hierarchical_list(tree, selItem, sel);

  int selCnt = sel.size();
  for (int i = 0; i < selCnt; i++) // O(N*M)
  {
    if (const DagorAssetFolder *folder = get_dagor_asset_folder(tree->getItemData(sel[i])))
    {
      int idx = get_app().getAssetMgr().getFolderIndex(*folder);
      if (idx >= 0)
        fld_idx.push_back(idx);
    }
  }
}

void AssetViewerApp::generate_impostors(const ImpostorOptions &options)
{
  if (!impostorApp)
    return;
  EcAutoBusy autoBusy;
  const bool consoleWasOpen = getConsole().isVisible();
  getConsole().showConsole();
  impostorApp->run(options);
  if (!consoleWasOpen)
    getConsole().hideConsole();
  ::post_base_update_notify_dabuild();
}

void AssetViewerApp::clear_impostors(const ImpostorOptions &options)
{
  if (!impostorApp)
    return;
  EcAutoBusy autoBusy;
  const bool consoleWasOpen = getConsole().isVisible();
  getConsole().showConsole();
  impostorApp->cleanUp(options);
  if (!consoleWasOpen)
    getConsole().hideConsole();
}

void AssetViewerApp::generate_point_cloud(DagorAsset *asset)
{
  if (!pointCloudGen)
    return;
  const bool consoleWasOpen = getConsole().isVisible();
  getConsole().showConsole();
  pointCloudGen->generate(asset);
  if (!consoleWasOpen)
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
    cwr.write(tmp.str(), (int)strlen(tmp));
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
  cwr.write(tmp.str(), (int)strlen(tmp));
  a->props.saveToTextStream(cwr);
  if (add_asset_closure)
    cwr.write("}\n\n", 3);
}
static void copy_terse_lod_asset_props_to_stream(DagorAsset *a, IGenSave &cwr)
{
  String tmp(0, "%s { ", a->getName());
  cwr.write(tmp.str(), (int)strlen(tmp));
  int lodIndex = 0;
  for (int i = 0, nid = a->props.getNameId("lod"); i < a->props.blockCount(); i++)
    if (a->props.getBlock(i)->getBlockNameId() == nid)
    {
      tmp.printf(0, "lod%d:r=%g; ", lodIndex, a->props.getBlock(i)->getReal("range", 0));
      cwr.write(tmp.str(), (int)strlen(tmp));
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

static void exportCompositToEcs(const DagorAsset &asset, DataBlock &out, const DagorAssetMgr &mgr);

static void addEntityToSelectOne(DataBlock *selectOne, const char *fullName, float weight, DataBlock &out, const DagorAssetMgr &mgr)
{
  const char *colon = strrchr(fullName, ':');
  const int nameLen = colon ? (int)(colon - fullName) : (int)strlen(fullName);
  String assetName(0, "%.*s", nameLen, fullName);

  int typeId = colon ? mgr.getAssetTypeId(colon + 1) : -1;
  const DagorAsset *a = typeId >= 0 ? mgr.findAsset(assetName, typeId) : mgr.findAsset(assetName);
  const char *assetType = a ? mgr.getAssetTypeName(a->getType()) : (colon ? colon + 1 : nullptr);

  DataBlock *entityObj = selectOne->addNewBlock("entity:object");

  if (assetType && strcmp(assetType, "composit") == 0)
  {
    String compositeName(0, "%s_composite", assetName.c_str());
    if (a && !out.getBlockByName(compositeName))
      exportCompositToEcs(*a, out, mgr);
    entityObj->addStr("gen__template", compositeName);
  }
  else if (assetType)
  {
    if (strcmp(assetType, "rendInst") == 0)
    {
      entityObj->addStr("gen__template", "composite_rendinst_child_base");
      entityObj->addStr("ri_extra__name", assetName);
    }
    else if (strcmp(assetType, "fx") == 0)
    {
      entityObj->addStr("gen__template", "composite_efx_child_base");
      entityObj->addStr("effect__name", assetName);
    }
    else if (strcmp(assetType, "animChar") == 0)
    {
      entityObj->addStr("gen__template", "composite_animchar_child_base");
      entityObj->addStr("animchar__res", assetName);
    }
    else if (strcmp(assetType, "dynModel") == 0)
    {
      entityObj->addStr("gen__template", "composite_animchar_child_base");
      String animcharRes(0, "%s_animchar", assetName.c_str());
      entityObj->addStr("animchar__res", animcharRes);
    }
  }
  entityObj->addReal("gen__weight", weight);
}

static void exportCompositNodes(const DataBlock &src, DataBlock *nodesArray, DataBlock &out, const DagorAssetMgr &mgr)
{
  for (int i = 0; i < src.blockCount(); ++i)
  {
    const DataBlock *node = src.getBlock(i);
    if (strcmp(node->getBlockName(), "node") != 0)
      continue;

    DataBlock *nodeObj = nodesArray->addNewBlock("node:object");

    if (node->findParam("tm") >= 0)
    {
      nodeObj->addTm("localTransform", node->getTm("tm", TMatrix::IDENT));
    }
    else
    {
      static const char *transformParams[] = {
        "rot_x", "rot_y", "rot_z", "offset_x", "offset_y", "offset_z", "scale", "xScale", "yScale"};

      for (const char *p : transformParams)
      {
        int idx = node->findParam(p);
        if (idx >= 0 && node->getParamType(idx) == DataBlock::TYPE_POINT2)
          nodeObj->addPoint2(p, node->getPoint2(idx));
      }
    }

    const char *name = node->getStr("name", nullptr);
    if (name && name[0] != '\0')
    {
      DataBlock *selectOne = nodeObj->addNewBlock("select_one:array");
      addEntityToSelectOne(selectOne, name, 1.0f, out, mgr);
    }
    else
    {
      DataBlock *selectOne = nullptr; // avoiding select_one creation if no "ent" found

      for (int j = 0; j < node->blockCount(); ++j)
      {
        const DataBlock *ent = node->getBlock(j);
        if (strcmp(ent->getBlockName(), "ent") != 0)
          continue;
        if (selectOne == nullptr)
          selectOne = nodeObj->addNewBlock("select_one:array");
        addEntityToSelectOne(selectOne, ent->getStr("name", ""), ent->getReal("weight", 1.0f), out, mgr);
      }
    }

    bool hasNested = false;
    for (int j = 0; j < node->blockCount() && !hasNested; ++j)
      hasNested = strcmp(node->getBlock(j)->getBlockName(), "node") == 0;
    if (hasNested)
      exportCompositNodes(*node, nodeObj->addNewBlock("nested:array"), out, mgr);
  }
}

static void exportCompositToEcs(const DagorAsset &asset, DataBlock &out, const DagorAssetMgr &mgr)
{
  String compositeName(0, "%s_composite", asset.getName());
  DataBlock *tpl = out.addNewBlock(compositeName);
  tpl->addStr("_use", "composite_base");
  exportCompositNodes(asset.props, tpl->addNewBlock("nodes:shared:array"), out, mgr);
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

    case CM_RELOAD_SHADERS: AssetViewerConsoleCommandProcessor::runShadersReload({}); return 1;

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

    case CM_CAMERAS: getModelessWindowControllers().toggleShowById(WindowIds::MAIN_SETTINGS_CAMERA); return 1;

    case CM_SCREENSHOT: getModelessWindowControllers().toggleShowById(WindowIds::MAIN_SETTINGS_SCREENSHOT); return 1;

    case CM_OPTIONS_STAT_DISPLAY_SETTINGS:
    {
      ViewportWindow *viewport = ged.getCurrentViewport();
      if (viewport)
        viewport->showStatSettingsDialog();

      return 1;
    }

    case CM_ENVIRONMENT_SETTINGS: getModelessWindowControllers().toggleShowById(WindowIds::MAIN_SETTINGS_ENVIRONMENT); return 1;

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

    case CM_SETTINGS_ENABLE_DEVELOPER_TOOLS:
      if (!developerToolsEnabled)
        wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Warning",
          "Developer tools are not intended to be used by users and may cause bugs and crashes. Use them at your own risk.");

      developerToolsEnabled = !developerToolsEnabled;
      getMainMenu()->setCheckById(id, developerToolsEnabled);
      getMainMenu()->setEnabledById(CM_VIEW_DEVELOPER_TOOLS, developerToolsEnabled);
      return 1;

    case CM_SETTINGS_ASSET_BUILD_WARNINGS_SHOW_WHEN_BUILDING:
    case CM_SETTINGS_ASSET_BUILD_WARNINGS_SHOW_ONCE_PER_SESSION:
    case CM_SETTINGS_ASSET_BUILD_WARNINGS_SHOW_WHEN_SELECTED:
      if (id == CM_SETTINGS_ASSET_BUILD_WARNINGS_SHOW_ONCE_PER_SESSION)
        assetBuildWarningDisplay = AssetBuildWarningDisplay::ShowOncePerSession;
      else if (id == CM_SETTINGS_ASSET_BUILD_WARNINGS_SHOW_WHEN_SELECTED)
        assetBuildWarningDisplay = AssetBuildWarningDisplay::ShowWhenSelected;
      else
        assetBuildWarningDisplay = AssetBuildWarningDisplay::ShowWhenBuilding;

      updateAssetBuildWarningsMenu();
      return 1;

    case CM_PREFERENCES_ASSET_TREE_COPY_SUBMENU:
    {
      bool moveCopyToSubmenu = !AssetSelectorGlobalState::getMoveCopyToSubmenu();
      AssetSelectorGlobalState::setMoveCopyToSubmenu(moveCopyToSubmenu);
      getMainMenu()->setCheckById(id, moveCopyToSubmenu);
      return 1;
    }

    case CM_OPTIONS_EDIT_PREFERENCES:
      getModelessWindowControllers().toggleShowById(WindowIds::MAIN_SETTINGS_EDIT_PREFERENCES);
      return 1;

    case CM_DISCARD_ASSET_TEX:
      discardAssetTexMode = !discardAssetTexMode;
      getMainMenu()->setCheckById(id, discardAssetTexMode);
      mToolPanel->setBool(id, discardAssetTexMode);
      ec_set_busy(true);
      applyDiscardAssetTexMode();
      ec_set_busy(false);
      return 1;

    case CM_THEME_LIGHT:
    case CM_THEME_DARK:
    {
      themeName = (id == CM_THEME_LIGHT) ? defaultThemeName : "dark";
      updateThemeItems();
      editor_core_load_imgui_theme(getThemeFileName());
      return 1;
    }

      // window

    case CM_LOAD_DEFAULT_LAYOUT:
    {
      makeDefaultLayout(/*for_initial_layout = */ false);

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
        DataBlock blk;
        if (blk.load(result))
        {
          loadLayout(blk);

          if (mTreeView)
            fillTree();
        }
        else
          logerr("Loading the window layout from \"%s\" has failed.", result.c_str());
      }
    }
      return 1;

    case CM_SAVE_LAYOUT:
    {
      String result(wingw::file_save_dlg(mManager->getMainWindow(), "Save layout as...", "Layout|*.blk", "blk"));
      if (!result.empty())
        saveLayout(result.str());
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

    case CM_WINDOW_TAGMANAGER:
    {
      const bool show = (DAEDITOR3.getVisibleTagManagerWindow() == nullptr);
      DAEDITOR3.showTagManagerWindow(show);
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

    case CM_WINDOW_DABUILD:
      dabuildWindowVisible = !dabuildWindowVisible;
      getMainMenu()->setCheckById(CM_WINDOW_DABUILD, dabuildWindowVisible);
      return 1;

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

    case CM_VIEW_DEVELOPER_TOOLS_IMGUI_DEBUGGER:
      imguiDebugWindowsVisible = !imguiDebugWindowsVisible;
      getMainMenu()->setCheckById(id, imguiDebugWindowsVisible);
      return 1;

    case CM_VIEW_DEVELOPER_TOOLS_TOAST_MANAGER: PropPanel::show_toast_debug_panel(); return 1;

    case CM_VIEW_DEVELOPER_TOOLS_TEXTURE_DEBUG: texdebug::select_texture("none"); return 1;

    case CM_VIEW_DEVELOPER_TOOLS_NODE_DEPS: dafg::show_dependencies_window(); return 1;

    case CM_VIEW_DEVELOPER_TOOLS_CONSOLE_COMMANDS_AND_VARIABLES:
      consoleCommandsAndVariableWindowsVisible = !consoleCommandsAndVariableWindowsVisible;
      imgui_window_set_visible("General", "Console commands and variables", consoleCommandsAndVariableWindowsVisible);
      getMainMenu()->setCheckById(id, consoleCommandsAndVariableWindowsVisible);
      return 1;

    case CM_CAMERAS_FREE:
    case CM_CAMERAS_FPS:
    case CM_CAMERAS_TPS:
    {
      IGenViewportWnd *vp = IEditorCoreEngine::get()->getCurrentViewport();

      if (vp)
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

    case CM_OPTIONS_SET_ACT_RATE: getModelessWindowControllers().toggleShowById(WindowIds::MAIN_SETTINGS_SET_ACT_RATE); return 1;

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

    case CM_COPY_ASSET_TAGS:
      if (curAsset)
      {
        AssetSelectorCommon::copyAssetTagsToClipboard(*curAsset);
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
        AssetReferenceViewer::show(*curAsset);
      return 1;
    case CM_SHOW_ASSET_BACK_REFS:
      if (curAsset)
        AssetBackReferenceViewer::show(*curAsset, getConsole());
      return 1;
    case CM_COPY_FOLDER_ASSETS_PROPS_BLK:
    case CM_COPY_FOLDER_LOD_ASSETS_PROPS_BLK:
      if (!curAsset)
      {
        if (const DagorAssetFolder *f = mTreeView->getAllAssetsTab().getSelectedAssetFolder())
        {
          const int fidx = assetMgr.getFolderIndex(*f);
          if (fidx >= 0)
          {
            int out_start_idx = 0, out_end_idx = -1;
            assetMgr.getFolderAssetIdxRange(fidx, out_start_idx, out_end_idx);

            DynamicMemGeneralSaveCB cwr(tmpmem, 0, 4 << 10);
            String tmp(0, "// folder %s (%s)\n", f->folderName, f->folderPath);
            cwr.write(tmp, (int)strlen(tmp));
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
          }
        }
      }
      return 1;

    case CM_COPY_FOLDERPATH:
      if (const DagorAsset *asset = mTreeView->getAllAssetsTab().getSelectedAsset())
        AssetSelectorCommon::copyAssetFolderPathToClipboard(*asset);
      else if (const DagorAssetFolder *f = mTreeView->getAllAssetsTab().getSelectedAssetFolder())
        AssetSelectorCommon::copyAssetFolderPathToClipboard(*f);
      return 1;

    case CM_REVEAL_IN_EXPLORER:
      if (const DagorAsset *asset = mTreeView->getAllAssetsTab().getSelectedAsset())
        AssetSelectorCommon::revealInExplorer(*asset);
      else if (const DagorAssetFolder *f = mTreeView->getAllAssetsTab().getSelectedAssetFolder())
        AssetSelectorCommon::revealInExplorer(*f);
      return 1;

    case CM_CREATE_NEW_COMPOSITE_ASSET:
      if (mTreeView)
        createNewCompositeAsset(mTreeView->getAllAssetsTab().getTree());
      return 1;

    case CM_SEARCH_FOR_SUBOPTIMAL_ASSETS:
      if (const DagorAssetFolder *folder = mTreeView->getAllAssetsTab().getSelectedAssetFolder())
        show_suboptimal_asset_collector_dialog(*folder);
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

    case CM_CONSOLE:
      if (console->isVisible())
      {
        if (console->isCanClose() && !console->isProgress())
          console->hideConsole();
      }
      else
        console->showConsole(true);
      return 1;

    case CM_NAVIGATE:
    {
      const SimpleString zoomKey(editor_command_system.getCommandKeyChordsAsText(EditorCommandIds::ZOOM_AND_CENTER));
      const SimpleString zoomKeyFly(editor_command_system.getCommandKeyChordsAsText(EditorCommandIds::ZOOM_AND_CENTER_IN_FLY_MODE));
      const SimpleString freeCamera(editor_command_system.getCommandKeyChordsAsText(EditorCommandIds::CAMERAS_FREE));
      const SimpleString fpsCamera(editor_command_system.getCommandKeyChordsAsText(EditorCommandIds::CAMERAS_FPS));

      wingw::message_box(0, "Navigation info",
        "Rotate:\t\t Alt + MiddleMouseButton\n"
        "Pan:\t\t MiddleMouseButton\n"
        "Zoom:\t\t MouseWhell\n"
        "Zoom extents:\t %s\n"
        "Zoom extents in fly mode: %s\n"
        "Free camera:\t %s\n"
        "FPS camera:\t %s\n"
        "Accelerate:\t Ctrl",
        zoomKey.c_str(), zoomKeyFly.c_str(), freeCamera.c_str(), fpsCamera.c_str());

      return 1;
    }

    case CM_CHANGE_FOV: onChangeFov(); return 1;

    case CM_VR_ENABLE: EDITORCORE->queryEditorInterface<IDynRenderService>()->toggleVrMode(); return 1;

    case CM_THEME_TOGGLE:
      themeName = (themeName != defaultThemeName) ? defaultThemeName : "dark";
      updateThemeItems();
      editor_core_load_imgui_theme(getThemeFileName());
      return 1;

    case CM_PALETTE:
      if (colorPaletteDlg->isVisible())
        colorPaletteDlg->hide();
      else
        colorPaletteDlg->show();
      return 1;

    case CM_EXPORT_AS_COMPOSITE_ENTITY:
    {
      if (!curAsset)
        break;

      DataBlock appBlk(getWorkspace().getAppBlkPath());
      const char *exportFolderRel = appBlk.getBlockByNameEx("asset_viewer")->getStr("compositeEntityExportFolder", "");
      String outPath;
      if (*exportFolderRel)
      {
        String exportFolder(0, "%s%s", getWorkspace().getAppDir(), exportFolderRel);
        dd_simplify_fname_c(exportFolder.str());
        if (!dd_dir_exists(exportFolder) && !dd_mkdir(exportFolder))
        {
          wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Export error", "Failed to create export folder '%s'.",
            exportFolder.str());
          break;
        }
        outPath.printf(0, "%s/%s.entities.blk", exportFolder.str(), curAsset->getName());
      }
      else
      {
        String defaultName(0, "%s.entities.blk", curAsset->getName());
        outPath = wingw::file_save_dlg(mManager->getMainWindow(), "Export as ECS composite template", "BLK|*.blk", "blk",
          curAsset->getFolderPath(), defaultName);
        if (outPath.empty())
          break;
      }

      DataBlock outBlk;
      exportCompositToEcs(*curAsset, outBlk, assetMgr);

      if (!outBlk.saveToTextFile(outPath))
        wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Export error", "Failed to write to '%s'.", outPath.str());

      return 1;
    }

    default: break;
  }

  return GenericEditorAppWindow::onMenuItemClick(id);
}


void AssetViewerApp::onAssetSelectionChanged(DagorAsset *asset, DagorAssetFolder *)
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

  ec_set_busy(true);
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
  ec_set_busy(false);
}


void AssetViewerApp::onAvSelectAsset(DagorAsset *asset, const char *name)
{
  if (asset)
    onAssetSelectionChanged(asset, nullptr);

  if (mTagManager)
    mTagManager->onAvSelectAsset(asset, name);
}


void AssetViewerApp::onAvSelectFolder(DagorAssetFolder *folder, const char *name)
{
  if (mTagManager)
    mTagManager->onAvSelectFolder(folder, name);
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
  String pack_name = ::get_asset_pack_name(e);
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


bool AssetViewerApp::onAssetSelectorContextMenu(PropPanel::IMenu &menu, DagorAsset *asset, DagorAssetFolder *asset_folder)
{
  if (asset_folder == assetMgr.getRootFolder())
    return false;

  menu.setEventHandler(this);

  if (!asset_folder)
  {
    const bool hasTags = (asset != nullptr && assettags::getTagIds(*asset).size() > 0);

    menu.addItem(ROOT_MENU_ITEM, CM_ADD_ASSET_TO_FAVORITES, "Add to favorites");
    menu.addSeparator(ROOT_MENU_ITEM);

    // curAssetPackName
    const int oldMenuItemCount = menu.getItemCount(ROOT_MENU_ITEM);
    if (::is_asset_exportable(asset))
      ::create_popup_build_assets_menu(menu, asset);

    if (asset->getType() == assetMgr.getAssetTypeId("composit"))
      menu.addItem(ROOT_MENU_ITEM, CM_EXPORT_AS_COMPOSITE_ENTITY, "Export as ECS composite entity");

    if (menu.getItemCount(ROOT_MENU_ITEM) != oldMenuItemCount)
      menu.addSeparator(ROOT_MENU_ITEM);

    if (AssetSelectorGlobalState::getMoveCopyToSubmenu())
    {
      menu.addSubMenu(ROOT_MENU_ITEM, CM_COPY_SUBMENU, "Copy");
      menu.addItem(CM_COPY_SUBMENU, CM_COPY_ASSET_FILEPATH, "File path");
      menu.addItem(CM_COPY_SUBMENU, CM_COPY_ASSET_PROPS_BLK, "Asset BLK data");
      menu.addItem(CM_COPY_SUBMENU, CM_COPY_FOLDERPATH, "Folder path");
      menu.addItem(CM_COPY_SUBMENU, CM_COPY_ASSET_NAME, "Name");
      if (hasTags)
        menu.addItem(CM_COPY_SUBMENU, CM_COPY_ASSET_TAGS, "Tags");
      menu.addItem(CM_COPY_SUBMENU, CM_COPY_LOD_ASSET_PROPS_BLK, "Terse LOD asset summary");
    }
    else
    {
      menu.addItem(ROOT_MENU_ITEM, CM_COPY_ASSET_FILEPATH, "Copy filepath");
      menu.addItem(ROOT_MENU_ITEM, CM_COPY_ASSET_PROPS_BLK, "Copy asset BLK data");
      menu.addItem(ROOT_MENU_ITEM, CM_COPY_FOLDERPATH, "Copy folder path");
      menu.addItem(ROOT_MENU_ITEM, CM_COPY_ASSET_NAME, "Copy name");
      if (hasTags)
        menu.addItem(ROOT_MENU_ITEM, CM_COPY_ASSET_TAGS, "Copy tags");
      menu.addItem(ROOT_MENU_ITEM, CM_COPY_LOD_ASSET_PROPS_BLK, "Copy terse LOD asset summary");
    }

    menu.addItem(ROOT_MENU_ITEM, CM_SHOW_ASSET_DEPS, "Show asset dependencies in console");
    menu.addItem(ROOT_MENU_ITEM, CM_SHOW_ASSET_REFS, "Show asset references");
    menu.addItem(ROOT_MENU_ITEM, CM_SHOW_ASSET_BACK_REFS, "Show references to asset");
    menu.addItem(ROOT_MENU_ITEM, CM_REVEAL_IN_EXPLORER, "Reveal in Explorer");
  }
  else
  {
    menu.addItem(ROOT_MENU_ITEM, CM_EXPAND_CHILDREN, "Expand children");
    menu.addItem(ROOT_MENU_ITEM, CM_COLLAPSE_CHILDREN, "Collapse children");
    menu.addSeparator(ROOT_MENU_ITEM);

    bool exported = false;
    if (asset_folder == mTreeView->getAllAssetsTab().getSelectedAssetFolder() &&
        mTreeView->getAllAssetsTab().isSelectedFolderExportable(&exported))
    {
      ::create_popup_build_menu(menu, exported);
      menu.addSeparator(ROOT_MENU_ITEM);
    }

    if (AssetSelectorGlobalState::getMoveCopyToSubmenu())
    {
      menu.addSubMenu(ROOT_MENU_ITEM, CM_COPY_SUBMENU, "Copy");
      menu.addItem(CM_COPY_SUBMENU, CM_COPY_FOLDERPATH, "Folder path");
      menu.addItem(CM_COPY_SUBMENU, CM_COPY_FOLDER_ASSETS_PROPS_BLK, "Assets BLK data (whole folder)");
      menu.addItem(CM_COPY_SUBMENU, CM_COPY_FOLDER_LOD_ASSETS_PROPS_BLK, "Terse LOD assets summary (whole folder)");
    }
    else
    {
      menu.addItem(ROOT_MENU_ITEM, CM_COPY_FOLDERPATH, "Copy folder path");
      menu.addItem(ROOT_MENU_ITEM, CM_COPY_FOLDER_ASSETS_PROPS_BLK, "Copy assets BLK data (whole folder)");
      menu.addItem(ROOT_MENU_ITEM, CM_COPY_FOLDER_LOD_ASSETS_PROPS_BLK, "Copy terse LOD assets summary (whole folder)");
    }

    menu.addItem(ROOT_MENU_ITEM, CM_REVEAL_IN_EXPLORER, "Reveal in Explorer");
    menu.addSeparator(ROOT_MENU_ITEM);
    menu.addItem(ROOT_MENU_ITEM, CM_CREATE_NEW_COMPOSITE_ASSET, "Create new composit asset");
    menu.addItem(ROOT_MENU_ITEM, CM_SEARCH_FOR_SUBOPTIMAL_ASSETS, "Search for suboptimal assets...");
  }

  return true;
}

void AssetViewerApp::onSnapSettingChanged()
{
  GenericEditorAppWindow::onSnapSettingChanged();

  getCompositeEditor().onSnapSettingChanged();
}

void AssetViewerApp::setToolbarScalePercent(int scale_percent)
{
  if (scale_percent < 50 || scale_percent > 400 || scale_percent == toolbarScalePercent)
    return;

  toolbarScalePercent = scale_percent;

  const float toolbarScale = scale_percent / 100.0f;
  const int toolbarControlWidth = (int)(PropPanel::Constants::TOOLBAR_DEFAULT_CONTROL_WIDTH * toolbarScale);

  if (mToolPanel)
    mToolPanel->setToolbarControlWidth(toolbarControlWidth);
  if (mPluginTool)
    mPluginTool->setToolbarControlWidth(toolbarControlWidth);
  if (themeSwitcherToolPanel)
    themeSwitcherToolPanel->setToolbarControlWidth(toolbarControlWidth);
}

void AssetViewerApp::onImguiDelayedCallback(void *user_data)
{
  const unsigned commandId = (unsigned)(uintptr_t)user_data;

  if ((commandId & DELAYED_CALLBACK_VIEWPORT_COMMAND_BIT) == 0)
  {
    onMenuItemClick(commandId);
  }
  else
  {
    ViewportWindow *viewport = ged.getCurrentViewport();
    if (!viewport)
      return;

    const unsigned viewportCommandId = commandId & ~DELAYED_CALLBACK_VIEWPORT_COMMAND_BIT;
    if (viewport->handleViewportAcceleratorCommand(viewportCommandId))
      return;

    IGenEditorPlugin *currentPlugin = curPlugin();
    if (currentPlugin)
      currentPlugin->handleViewportAcceleratorCommand(*viewport, viewportCommandId);
  }
}

void AssetViewerApp::renderUIViewport(ViewportWindow &viewport, const Point2 &size, float item_spacing, bool vr_mode)
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

  viewport.updateImgui(viewportCanvasId, size, item_spacing, vr_mode);

  ImGui::PopID();
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
      renderUIViewport(*viewport0, regionAvailable, itemSpacing, vr_mode);
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

      renderUIViewport(*viewport0, Point2(leftWidth, topHeight), itemSpacing, vr_mode);
      ImGui::SameLine(0.0f, itemSpacing);
      renderUIViewport(*viewport1, Point2(rightWidth, topHeight), itemSpacing, vr_mode);

      renderUIViewport(*viewport2, Point2(leftWidth, bottomHeight), itemSpacing, vr_mode);
      ImGui::SameLine(0.0f, itemSpacing);
      renderUIViewport(*viewport3, Point2(rightWidth, bottomHeight), itemSpacing, vr_mode);
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

  if (PropPanel::IMenu *mm = getMainMenu())
    PropPanel::render_menu(*mm);

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

  if (mToolPanel || mPluginTool || themeSwitcherToolPanel)
  {
    PropPanel::pushToolBarColorOverrides();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, PropPanel::Constants::TOOLBAR_WINDOW_PADDING);

    ImGui::BeginChild("Toolbar", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders,
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking);

    ImGui::PopStyleVar();
    PropPanel::popToolBarColorOverrides();

    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * toolbarScalePercent * 0.01f);

    if (mToolPanel)
      mToolPanel->updateImgui();

    if (mPluginTool)
    {
      if (mToolPanel)
      {
        ImGui::SameLine();
        if (mPluginTool->getChildCount() != 0)
          ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, 1.0f);
      }

      mPluginTool->updateImgui();
    }

    if (themeSwitcherToolPanel)
    {
      if (mToolPanel || mPluginTool)
      {
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, 1.0f);
      }

      themeSwitcherToolPanel->updateImgui();
    }

    ImGui::PopFont();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
    ImGui::EndChild();
    ImGui::PopStyleVar();

    editor_command_system.updateImgui();
  }

  ImGui::BeginChild("DockHolder", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking);

  ImGuiID rootDockSpaceId = ImGui::GetID("RootDockSpace");

  if (!dockPositionsInitialized && !lastUsedLayoutLoaded)
    loadLastUsedLayout();

  // Create the initial dock layout.
  // If you want to the change the initial dock layout you will likely need to increase LATEST_DOCK_SETTINGS_VERSION.
  if (!dockPositionsInitialized)
  {
    dockPositionsInitialized = true;

    ImGui::DockBuilderRemoveNode(rootDockSpaceId);
    ImGui::DockBuilderAddNode(rootDockSpaceId, ImGuiDockNodeFlags_CentralNode);
    ImGui::DockBuilderSetNodeSize(rootDockSpaceId, ImGui::GetMainViewport()->Size);

    ImGuiID dockSpaceViewport;
    ImGuiID dockSpaceRightBottom = ImGui::DockBuilderSplitNode(rootDockSpaceId, ImGuiDir_Right, 0.15f, nullptr, &dockSpaceViewport);
    ImGuiID dockSpaceRightTop = ImGui::DockBuilderSplitNode(dockSpaceRightBottom, ImGuiDir_Up, 0.5f, nullptr, &dockSpaceRightBottom);
    ImGuiID dockSpaceLeft1 = ImGui::DockBuilderSplitNode(dockSpaceViewport, ImGuiDir_Left, 0.25f, nullptr, &dockSpaceViewport);
    ImGuiID dockSpaceLeft1Bottom = ImGui::DockBuilderSplitNode(dockSpaceLeft1, ImGuiDir_Down, 0.15f, nullptr, &dockSpaceLeft1);
    ImGuiID dockSpaceLeft2 = ImGui::DockBuilderSplitNode(dockSpaceViewport, ImGuiDir_Left, 0.25f, nullptr, &dockSpaceViewport);
    ImGuiID dockSpaceLeft3 = ImGui::DockBuilderSplitNode(dockSpaceViewport, ImGuiDir_Left, 0.25f, nullptr, &dockSpaceViewport);
    ImGuiID dockSpaceBottom = ImGui::DockBuilderSplitNode(dockSpaceViewport, ImGuiDir_Down, 0.25f, nullptr, &dockSpaceViewport);

    ImGuiDockNode *viewportNode = ImGui::DockBuilderGetNode(dockSpaceViewport);
    if (viewportNode)
      viewportNode->SetLocalFlags(viewportNode->LocalFlags | ImGuiDockNodeFlags_HiddenTabBar);

    // TODO: ImGui porting: view: AV: extensibility?
    ImGui::DockBuilderDockWindow("Assets Tree", dockSpaceLeft1);
    ImGui::DockBuilderDockWindow("Asset Browser", dockSpaceLeft2);
    ImGui::DockBuilderDockWindow("Tag Management", dockSpaceLeft1Bottom);
    ImGui::DockBuilderDockWindow("Actions Tree", dockSpaceLeft3);
    ImGui::DockBuilderDockWindow("Effects Tools", dockSpaceLeft3);
    ImGui::DockBuilderDockWindow("Viewport", dockSpaceViewport);
    ImGui::DockBuilderDockWindow("Properties", dockSpaceRightBottom);
    ImGui::DockBuilderDockWindow("Composit Outliner", dockSpaceRightTop);
    ImGui::DockBuilderDockWindow("daBuild", dockSpaceBottom);

    ImGui::DockBuilderFinish(rootDockSpaceId);
  }

  // Change the close button's color for the daEditorX Classic style.
  PropPanel::pushDialogTitleBarColorOverrides();
  if (vrMode)
    ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
  ImGui::DockSpace(rootDockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
  ImGui::PopStyleColor(vrMode ? 1 : 0);
  PropPanel::popDialogTitleBarColorOverrides();

  renderUIViewports(vrMode);

  if (mTreeView)
  {
    DAEDITOR3.imguiBegin("Assets Tree");
    mTreeView->updateImgui();
    DAEDITOR3.imguiEnd();
  }

  if (mTagManager)
  {
    bool open = true;
    DAEDITOR3.imguiBegin("Tag Management", &open);
    if (!mTagManager->updateImgui())
      open = false;
    DAEDITOR3.imguiEnd();

    if (!open)
      DAEDITOR3.showTagManagerWindow(false);
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

  if (console && console->isVisible())
  {
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(viewport->Size.x * 0.45f, viewport->Size.y * 0.15f), ImGuiCond_FirstUseEver);

    const bool canClose = console->isCanClose() && !console->isProgress();
    bool open = true;
    DAEDITOR3.imguiBegin("AssetViewer console", canClose ? &open : nullptr);
    console->updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open)
      console->hide();
  }

  g_entity_mgr->broadcastEventImmediate(ImGuiStage());

  PropPanel::render_dialogs();

  PropPanel::render_toast_messages();

  if (dabuildWindowVisible)
    ::render_dabuild_imgui();


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

  if (imguiDebugWindowsVisible)
    ImGui::ShowMetricsWindow(&imguiDebugWindowsVisible);

  editor_core_update_imgui_style_editor();

  ::update_dabuild_background(getMainMenu());

  renderUI();

  PropPanel::before_end_frame();

  renderingImGui = false;
}

const char *daeditor3_get_appblk_fname() { return ::get_app().getWorkspace().getAppBlkPath(); }

REGISTER_IMGUI_WINDOW("General", "Console commands and variables", console::imgui_window);