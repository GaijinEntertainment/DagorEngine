#include "de_appwnd.h"
#include "splashScreen.h"
#include "de_batch.h"
#include <de3_lightService.h>
#include <de3_interface.h>
#include <de3_editorEvents.h>
#include <de3_dynRenderService.h>
#include <de3_skiesService.h>
#include <de3_waterSrv.h>
#include <de3_dxpFactory.h>

#include <EditorCore/ec_startup.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <libTools/util/makeBinDump.h>
#include <libTools/util/setupTexStreaming.h>
#include <oldEditor/de_workspace.h>

#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_startupModules.h>

#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_dynSceneWithTreeRes.h>

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

#include <3d/dag_drv3dReset.h>
#include <3d/dag_texPackMgr2.h>
#include <libTools/util/strUtil.h>
#include <render/dynmodelRenderer.h>

#include <scene/dag_visibility.h>
#include <EditorCore/ec_IEditorCoreImpl.h>

static struct DagorEdReset3DCallback : public IDrv3DResetCB
{
  virtual void beforeReset(bool full_reset)
  {
    DAEDITOR3.conNote("notifying services (onBeforeReset3dDevice)...");
    ((DagorEdAppWindow *)DAGORED2)->onBeforeReset3dDevice();
  }

  virtual void afterReset(bool full_reset)
  {
    extern void rebuild_shaders_stateblocks();

    DAEDITOR3.conNote("notifying services (afterReset)...");
    rebuild_shaders_stateblocks();

    if (IDynRenderService *drSrv = EDITORCORE->queryEditorInterface<IDynRenderService>())
      drSrv->afterD3DReset(full_reset);
    if (ISkiesService *skiesSrv = EDITORCORE->queryEditorInterface<ISkiesService>())
      skiesSrv->afterD3DReset(full_reset);
    if (IWaterService *waterSrv = EDITORCORE->queryEditorInterface<IWaterService>())
      waterSrv->afterD3DReset(full_reset);

    if (full_reset)
    {
      DAEDITOR3.conNote("reloading textures...");
      ddsx::reload_active_textures(0);
      dxp_factory_after_reset();
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
#include <sepGui/wndGlobal.h>

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
  static const size_t suffix_len = strlen(".psXX.shdump.bin");
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
  int ver = 0;
  if (strstr(fn, ".ps50.shdump"))
    ver = FSHVER_50;
  else if (strstr(fn, ".ps40.shdump"))
    ver = FSHVER_40;
  else if (strstr(fn, ".ps30.shdump"))
    ver = FSHVER_30;
  if (::load_shaders_bindump(String(0, "%.*s", strlen(fn) - suffix_len, fn), ver, true))
    debug("loaded export-specific shader dump (%c%c%c%c): %s", _DUMP4C(tc), fn);
  else
    fatal("failed to load shaders: %s", fn);
}

void init3d()
{
  const char *appdir = DAGORED2->getWorkspace().getAppDir();
  DataBlock appblk(DAGORED2->getWorkspace().getAppPath());
  String sh_file;

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
  ::load_tex_streaming_settings(DAGORED2->getWorkspace().getAppPath(), &texStreamingBlk);
  texStreamingBlk.setBool("texLoadOnDemand", false);
  EDITORCORE->getWndManager()->init3d(de3_drv_name, &texStreamingBlk);
  if (int resv_tid_count = appblk.getInt("texMgrReserveTIDcount", 128 << 10))
  {
    enable_res_mgr_mt(false, 0);
    enable_res_mgr_mt(true, resv_tid_count);
  }

  if (const char *gp_file = appblk.getBlockByNameEx("game")->getStr("params", NULL))
  {
    DataBlock *b = const_cast<DataBlock *>(dgs_get_game_params());
    b->load(String(0, "%s/%s", appdir, gp_file));
    b->removeBlock("rendinstExtra");
    b->removeBlock("rendinstOpt");
  }
  const_cast<DataBlock *>(dgs_get_game_params())->setBool("rendinstExtraAutoSubst", false);
  const_cast<DataBlock *>(dgs_get_game_params())->setInt("rendinstExtraMaxCnt", 0);

  if (appblk.getStr("shaders", NULL))
    sh_file.printf(260, "%s/%s", appdir, appblk.getStr("shaders", NULL));
  else
    sh_file.printf(260, "%s/../commonData/compiledShaders/classic/tools", sgg::get_exe_path_full());

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
      String fn(0, "%s.%s.fexp.%s", sh_file, mkbindump::get_target_str(codes[i]), ver_suffix);
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
    full_sh_file.printf(260, "%s.%s", sh_file, ver_suffix = "ps50.shdump.bin");
    if (!dd_file_exist(full_sh_file))
      full_sh_file.printf(260, "%s.%s", sh_file, ver_suffix = "ps40.shdump.bin");
    if (!dd_file_exist(full_sh_file))
      full_sh_file.printf(260, "%s.%s", sh_file, ver_suffix = "ps30.shdump.bin");
    if (dd_file_exist(full_sh_file))
      shaderBinFname = full_sh_file;
  }

  ::shaders_register_console(true);
  ::register_common_tool_tex_factories();
  ::register_png_tex_load_factory();
  ::register_avif_tex_load_factory();

  ::startup_shaders(sh_file);
  ::startup_game(RESTART_ALL);
  ShaderGlobal::enableAutoBlockChange(true);

  DataBlock fontsBlk;
  fontsBlk.addBlock("fontbins")->addStr("name", String(260, "%s/../commonData/verdanab", sgg::get_exe_path_full()));
  StdGuiRender::init_dynamic_buffers(appblk.getInt("guiMaxQuad", 256 << 10), appblk.getInt("guiMaxVert", 64 << 10));
  StdGuiRender::init_fonts(fontsBlk);
  StdGuiRender::init_render();

  ShaderGlobal::set_int(get_shader_variable_id("in_editor"), 1);
  ShaderGlobal::set_int(get_shader_variable_id("fake_lighting_computations", true), 1);

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

  {
    DataBlock gameParamsBlk;

    // visibility finder
    G_ASSERT(!visibility_finder);
    visibility_finder = new VisibilityFinder;
  }

  ::set_3d_device_reset_callback(&drv_3d_reset_cb);
  load_exp_shaders_for_target(_MAKE4C('PC'));

  CfgReader cfr;
  ::create_visclipmesh(cfr);
}


extern void setup_water_service(const DataBlock &app_blk);
extern void setup_grass_service(const DataBlock &app_blk);
extern void setup_gpu_grass_service(const DataBlock &app_blk);

extern void init_plugin_environment();
extern void init_plugin_clipping();
extern void init_plugin_scn_export();
extern void init_plugin_ssview();
extern void init_plugin_bin_scn_view();

extern void init_rimgr_service(const DataBlock &app_blk);
extern void init_prefabmgr_service(const DataBlock &app_blk);
extern void init_fxmgr_service(const DataBlock &app_blk);
extern void init_efxmgr_service();
extern void init_physobj_mgr_service();
extern void init_gameobj_mgr_service();
extern void init_composit_mgr_service();
extern void init_entity_filter_service();
extern void init_built_scene_view_service();
extern void init_invalid_entity_service();
extern void init_animchar_mgr_service(const DataBlock &app_blk);
extern void init_dynmodel_mgr_service(const DataBlock &app_blk);
extern void init_csg_mgr_service();
extern void init_spline_gen_mgr_service();
extern void init_ecs_mgr_service(const DataBlock &app_blk, const char *app_dir);
extern void init_webui_service();

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
  INIT_SERVICE("composit", ::init_composit_mgr_service());
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

  ::init_plugin_environment();
  ::init_plugin_clipping();
  ::init_plugin_scn_export();
  ::init_plugin_ssview();
  ::init_plugin_bin_scn_view();
}

// required for ZLIB (that in turn is required by LIBPNG)
extern "C" void *zcalloc(void * /*opaque*/, unsigned items, unsigned size) { return memalloc_default(items * size); }

extern "C" void zcfree(void * /*opaque*/, void *ptr) { memfree_default(ptr); }
