// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_appwnd.h"
#include <de3_lightService.h>
#include <de3_interface.h>
#include <de3_editorEvents.h>
#include <de3_dynRenderService.h>
#include <de3_skiesService.h>
#include <de3_waterSrv.h>

#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_startup.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/setupTexStreaming.h>
#include <oldEditor/de_workspace.h>

#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_startupModules.h>

#include <shaders/dag_dynSceneRes.h>

#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_collisionResource.h>

#include <startup/dag_startupTex.h>
#include <startup/dag_restart.h>
#include <gui/dag_stdGuiRender.h>

#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_vromfs.h>
#include <animChar/dag_animCharacter2.h>
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_visClipMesh.h>
#include <obsolete/dag_cfg.h>

#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_texPackMgr2.h>
#include <libTools/util/strUtil.h>
#include <render/dynmodelRenderer.h>

#include <scene/dag_visibility.h>
#include <gui/dag_imgui.h>

namespace workcycle_internal
{
extern bool window_initing;
}

bool is_generic_water_service_initialized();

static constexpr const char *INITIAL_CAPTION = "DaEditorX";
static constexpr const char *INITIAL_ICON = "RES_DE_ICON";

static struct DagorEdReset3DCallback : public IDrv3DResetCB
{
  void beforeReset(bool full_reset) override
  {
    DAEDITOR3.conNote("notifying services (onBeforeReset3dDevice)...");
    if (IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>())
      drSrv->beforeD3DReset(full_reset);
    ((DagorEdAppWindow *)DAGORED2)->onBeforeReset3dDevice();
  }

  void afterReset(bool full_reset) override
  {
    extern void rebuild_shaders_stateblocks();

    DAEDITOR3.conNote("notifying services (afterReset)...");
    rebuild_shaders_stateblocks();
    StdGuiRender::after_device_reset();

    if (IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>())
      drSrv->afterD3DReset(full_reset);
    if (ISkiesService *skiesSrv = EDITORCORE->queryEditorInterface<ISkiesService>())
      skiesSrv->afterD3DReset(full_reset);

    // Querying IWaterService will initialize it. The initialization in WaterService::initSrv() disables the service if
    // the "fft_source_texture_no" shader variable does not exist, so prevent calling it too early (for example from the
    // project selector).
    if (is_generic_water_service_initialized())
      if (IWaterService *waterSrv = EDITORCORE->queryEditorInterface<IWaterService>())
        waterSrv->afterD3DReset(full_reset);

    if (full_reset)
    {
      DAEDITOR3.conNote("reloading textures...");
      ddsx::reload_active_textures(0);
    }
    DAEDITOR3.conNote("notifying services (reset finished)...");
    DAGORED2->spawnEvent(HUID_AfterD3DReset, NULL);
  }
} drv_3d_reset_cb;


#include <scene/dag_visibility.h>
#include <gui/dag_guiStartup.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <startup/dag_restart.h>
#include <startup/dag_startupTex.h>
#include <osApiWrappers/dag_basePath.h>
#include <EditorCore/ec_wndGlobal.h>

extern const char *de3_drv_name;

static SimpleString shaderBinFname;
struct DabuildIntStrPair
{
  unsigned code;
  SimpleString fn;
};
static Tab<DabuildIntStrPair> shaderBinFnameAlt(inimem);
static unsigned curLoadedShaderTarget = 0;

void load_exp_shaders_for_target(unsigned tc)
{
  if (tc == curLoadedShaderTarget)
    return;
  if (curLoadedShaderTarget)
    ::unload_shaders_bindump(true);

  curLoadedShaderTarget = tc;
  const char *fn = shaderBinFname;
  for (int i = 0; i < shaderBinFnameAlt.size(); i++)
    if (shaderBinFnameAlt[i].code == tc)
    {
      fn = shaderBinFnameAlt[i].fn;
      break;
    }

  d3d::shadermodel::Version ver = d3d::smAny;
  size_t suffixLen = strlen(".psXX.shdump.bin");
  if (trail_strcmp(fn, "SpirV.ps50.shdump.bin"))
  {
    ver = 5.0_sm;
    suffixLen = strlen("SpirV.ps50.shdump.bin");
  }
  else if (trail_strcmp(fn, "SpirV.bindless.ps50.shdump.bin"))
  {
    ver = 5.0_sm;
    suffixLen = strlen("SpirV.bindless.ps50.shdump.bin");
  }
  else if (trail_strcmp(fn, ".ps50.shdump.bin"))
    ver = 5.0_sm;
  else if (trail_strcmp(fn, ".ps40.shdump.bin"))
    ver = 4.0_sm;
  else if (trail_strcmp(fn, ".ps30.shdump.bin"))
    ver = 3.0_sm;
  if (::load_shaders_bindump(String(0, "%.*s", strlen(fn) - suffixLen, fn), ver, true))
    debug("loaded export-specific shader dump (%c%c%c%c): %s", _DUMP4C(tc), fn);
  else
    DAG_FATAL("failed to load shaders: %s", fn);
}

static E3DCOLOR load_window_background_color()
{
  const String settingsPath = make_full_path(sgg::get_exe_path_full(), "../.local/de3_settings.blk");
  DataBlock settingsBlock;
  dblk::load(settingsBlock, settingsPath, dblk::ReadFlag::ROBUST);
  const DataBlock *settingsThemeBlock = settingsBlock.getBlockByNameEx("theme");
  const char *themeName = settingsThemeBlock->getStr("name", "light");
  return editor_core_load_window_background_color(String::mk_str_cat(themeName, ".blk"));
}

void init3d_early()
{
  // Initialize the startup scene before init3d(), so VideoRestartProc can use it to draw the first frame.
  startup_editor_core_select_startup_scene(load_window_background_color());

  dgs_all_shader_vars_optionals = true;
  EDITORCORE->getWndManager()->init3d(de3_drv_name, nullptr, INITIAL_CAPTION, INITIAL_ICON);

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

  ::set_3d_device_reset_callback(&drv_3d_reset_cb);
}

namespace tools3d
{
extern void destroy();
}
void init3d()
{
  const char *appdir = DAGORED2->getWorkspace().getAppDir();
  DataBlock appblk(DAGORED2->getWorkspace().getAppBlkPath());
  String sh_file;
  if (appblk.getStr("shaders", NULL))
    sh_file.printf(260, "%s/%s", appdir, appblk.getStr("shaders", NULL));
  else
    sh_file.printf(260, "%s/compiledShaders/classic/tools", sgg::get_common_data_dir());

  // shutdown imGui, render, shaders, d3d before reiniting
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
  ::load_tex_streaming_settings(DAGORED2->getWorkspace().getAppBlkPath(), &texStreamingBlk);
  EDITORCORE->getWndManager()->init3d(de3_drv_name, &texStreamingBlk, INITIAL_CAPTION, INITIAL_ICON);
  if (int resv_tid_count = appblk.getInt("texMgrReserveTIDcount", 128 << 10))
  {
    enable_res_mgr_mt(false, 0);
    enable_res_mgr_mt(true, resv_tid_count);
  }

  ::startup_shaders(sh_file);

  if (const char *gp_file = appblk.getBlockByNameEx("game")->getStr("params", NULL))
  {
    DataBlock *b = const_cast<DataBlock *>(dgs_get_game_params());
    b->load(String(0, "%s/%s", appdir, gp_file));
    b->removeBlock("rendinstExtra");
    b->removeBlock("rendinstOpt");
  }

  bool forceRiExtra =
    appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("rendInst")->getBool("forceRiExtra", false);
  const_cast<DataBlock *>(dgs_get_game_params())->setBool("rendinstExtraAutoSubst", forceRiExtra && !d3d::is_stub_driver());
  const_cast<DataBlock *>(dgs_get_game_params())->setInt("rendinstExtraMaxCnt", 4096);

  const char *ver_suffix = "ps50.shdump.bin";
  String full_sh_file;
  full_sh_file.printf(260, "%s.fexp.%s", sh_file, ver_suffix);
  if (!dd_file_exist(full_sh_file))
    full_sh_file.printf(260, "%s.fexp.%s", sh_file, ver_suffix = "ps40.shdump.bin");
  if (!dd_file_exist(full_sh_file))
    full_sh_file.printf(260, "%s.fexp.%s", sh_file, ver_suffix = "ps30.shdump.bin");

  if (dd_file_exist(full_sh_file))
  {
    static unsigned codes[] = {
      _MAKE4C('PC'),
      _MAKE4C('iOS'),
      _MAKE4C('and'),
      _MAKE4C('PS4'),
    };

    shaderBinFname = String(260, "%s.fexp.%s", sh_file, ver_suffix);
    debug("using export-specific shader dump:        %s", shaderBinFname.str());
    for (int i = 0; i < countof(codes); i++)
    {
      uint64_t tc_storage = 0;
      String fn(0, "%s.%s.fexp.%s", sh_file, mkbindump::get_target_str(codes[i], tc_storage), ver_suffix);
      if (dd_file_exist(fn))
      {
        DabuildIntStrPair &pair = shaderBinFnameAlt.push_back();
        pair.code = codes[i];
        pair.fn = fn;
        debug("using export-specific shader dump (%c%c%c%c): %s", _DUMP4C(pair.code), pair.fn);
      }
    }
  }
  else
  {
    if (d3d::get_driver_code() == d3d::vulkan)
    {
      const char *format = d3d::get_driver_desc().caps.hasBindless ? "%sSpirV.bindless.%s" : "%sSpirV.%s";
      full_sh_file.printf(260, format, sh_file, "ps50.shdump.bin");
      if (dd_file_exist(full_sh_file))
        shaderBinFname = full_sh_file;
    }
    else
    {
      full_sh_file.printf(260, "%s.%s", sh_file, ver_suffix = "ps50.shdump.bin");
      if (!dd_file_exist(full_sh_file))
        full_sh_file.printf(260, "%s.%s", sh_file, ver_suffix = "ps40.shdump.bin");
      if (!dd_file_exist(full_sh_file))
        full_sh_file.printf(260, "%s.%s", sh_file, ver_suffix = "ps30.shdump.bin");
      if (dd_file_exist(full_sh_file))
        shaderBinFname = full_sh_file;
    }
  }

  ::shaders_register_console(true);

  ::startup_game(RESTART_ALL);
  ShaderGlobal::enableAutoBlockChange(true);

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

  ShaderGlobal::set_int(get_shader_variable_id("in_editor"), 1);
  ShaderGlobal::set_int(get_shader_variable_id("fake_lighting_computations", true), 1);

  ShaderGlobal::set_int(get_shader_variable_id("dgs_tex_anisotropy", true), ::dgs_tex_anisotropy);
  ShaderGlobal::set_float(get_shader_variable_id("mip_bias", true), 0.f);

  if (appblk.getBool("useDynrend", false))
    dynrend::init();

  ::register_dynmodel_gameres_factory();
  ::register_rendinst_gameres_factory();
  ::register_geom_node_tree_gameres_factory();
  ::register_character_gameres_factory();
  //::register_fast_phys_gameres_factory();
  ::register_phys_sys_gameres_factory();
  ::register_phys_obj_gameres_factory();
  ::register_ragdoll_gameres_factory();
  ::register_animchar_gameres_factory();
  ::register_a2d_gameres_factory();
  CollisionResource::registerFactory();

  startup_game(RESTART_ALL);

  load_exp_shaders_for_target(_MAKE4C('PC'));

  CfgReader cfr;
  ::create_visclipmesh(cfr);
}


extern void setup_water_service(const DataBlock &app_blk);
extern void setup_grass_service(const DataBlock &app_blk);
extern void setup_gpu_grass_service(const DataBlock &app_blk);

extern void init_plugin_ecs_editor();
extern void init_plugin_environment();
extern void init_plugin_collision();
extern void init_plugin_scn_export();
extern void init_plugin_ssview();
extern void init_plugin_bin_scn_view();

extern void init_rimgr_service(const DataBlock &app_blk);
extern void init_prefabmgr_service(const DataBlock &app_blk);
extern void init_fxmgr_service(const DataBlock &app_blk);
extern void init_efxmgr_service();
extern void init_physobj_mgr_service();
extern void init_gameobj_mgr_service();
extern void init_composit_mgr_service(const DataBlock &app_blk);
extern void init_entity_filter_service();
extern void init_built_scene_view_service();
extern void init_invalid_entity_service();
extern void init_animchar_mgr_service(const DataBlock &app_blk);
extern void init_dynmodel_mgr_service(const DataBlock &app_blk);
extern void init_csg_mgr_service();
extern void init_spline_gen_mgr_service();
extern void init_ecs_mgr_service(const DataBlock &app_blk, const char *app_dir);
extern void init_webui_service();

extern void init_plugin_heightmapland();
extern void init_plugin_csg();
extern void init_plugin_ivygen();
extern void init_plugin_occluders();
extern void init_plugin_staticgeom();
#if HAS_PLUGINS_WT
extern void init_plugin_missioned2();
extern void init_plugin_locationscene();
#endif

void dagored_init_all_plugins(const DataBlock &app_blk)
{
  EDITORCORE->queryEditorInterface<ISceneLightService>()->reset();
#define INIT_SERVICE(TYPE_NAME, EXPR)           \
  if (DAEDITOR3.getAssetTypeId(TYPE_NAME) >= 0) \
    EXPR;                                       \
  else                                          \
    debug("skipped initing service for \"%s\", asset manager did not declare such type", TYPE_NAME);

  ::setup_water_service(app_blk);
  ::setup_grass_service(app_blk);
  ::setup_gpu_grass_service(app_blk);
  INIT_SERVICE("rendInst", ::init_rimgr_service(app_blk));
  INIT_SERVICE("prefab", ::init_prefabmgr_service(app_blk));
  INIT_SERVICE("fx", ::init_fxmgr_service(app_blk));
  INIT_SERVICE("efx", ::init_efxmgr_service());
  INIT_SERVICE("composit", ::init_composit_mgr_service(app_blk));
  ::init_entity_filter_service();
  INIT_SERVICE("physObj", ::init_physobj_mgr_service());
  INIT_SERVICE("gameObj", ::init_gameobj_mgr_service());
  ::init_built_scene_view_service();
  ::init_invalid_entity_service();
  INIT_SERVICE("animChar", ::init_animchar_mgr_service(app_blk));
  INIT_SERVICE("dynModel", ::init_dynmodel_mgr_service(app_blk));
  INIT_SERVICE("csg", ::init_csg_mgr_service());
  INIT_SERVICE("spline", ::init_spline_gen_mgr_service());
#undef INIT_SERVICE

  ::init_ecs_mgr_service(app_blk, DAGORED2->getWorkspace().getAppDir());
  if (app_blk.getBlockByNameEx("game")->getBool("enable_webui_de3", true))
    ::init_webui_service();
  else
    debug("webUi disabled with game{ enable_webui_de3:b=no;");

  ::init_plugin_ecs_editor();
  ::init_plugin_environment();
  ::init_plugin_collision();
  ::init_plugin_scn_export();
  ::init_plugin_ssview();
  ::init_plugin_bin_scn_view();
#if _TARGET_STATIC_LIB
  ::init_plugin_heightmapland();
  ::init_plugin_csg();
  ::init_plugin_ivygen();
  ::init_plugin_occluders();
  ::init_plugin_staticgeom();
#if HAS_PLUGINS_WT
  ::init_plugin_missioned2();
  ::init_plugin_locationscene();
#endif
#endif
}
