// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <signal.h>
#include <regex>
#include <string>
#include <utility>

#include <3d/dag_texPackMgr2.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_stdGameResId.h>
#include <gameRes/dag_collisionResource.h>
#include <math/dag_bounds2.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_symHlp.h>
#include <perfMon/dag_cpuFreq.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <startup/dag_fatalHandler.inc.cpp>
#include <startup/dag_globalSettings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_restart.h>
#include <startup/dag_startupTex.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_startupModules.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameScene.h>
#include <libTools/util/makeBindump.h>
#include <shaders/dag_rendInstRes.h>
#include <rendInst/rendInstGen.h>
#include <libTools/util/setupTexStreaming.h>
#include <assets/assetPlugin.h>
#include <assets/texAssetBuilderTextureFactory.h>

#if _TARGET_PC_WIN
#include <startup/dag_winMain.inc.cpp>
#else
#include <startup/dag_mainCon.inc.cpp>
#endif

#include "../impostorUtil/impostorGenerator.h"
#include "engine.h"
#include "option_parser.h"
#include <generic/dag_initOnDemand.h>

extern "C" const char *dagor_get_build_stamp_str(char *buf, size_t bufsz, const char *suffix);

void __cdecl ctrl_break_handler(int) { ImpostorGenerator::interrupted = true; }

static void flush_files()
{
  flush_debug_file();
  fflush(stdout);
  fflush(stderr);
}

namespace rendinst::gen
{
float custom_max_trace_distance = 0;
bool custom_trace_ray(const Point3 &, const Point3 &, real &, Point3 *) { return false; }
bool custom_trace_ray_earth(const Point3 &, const Point3 &, real &) { return false; }
bool custom_get_height(Point3 &, Point3 *) { return false; }
vec3f custom_update_pregen_pos_y(vec4f pos, int16_t *, float, float) { return pos; }
void custom_get_land_min_max(BBox2, float &out_min, float &out_max)
{
  out_min = 0;
  out_max = 8192;
}
} // namespace rendinst::gen

class ITextureNameResolver *get_global_tex_name_resolver() { return nullptr; }

void DagorWinMainInit(int, bool)
{
  char stamp_buf[256];
  printf("--------------------------------------------\n"
         "DaImpostor v1.00\n"
         "Copyright (C) Gaijin Games KFT, 2023\n"
         "--------------------------------------------\n"
         "%s\n\n\n",
    dagor_get_build_stamp_str(stamp_buf, sizeof(stamp_buf), ""));
}

void init_res_factories()
{
  ::register_common_tool_tex_factories();
  ::register_svg_tex_load_factory();
  ::register_loadable_tex_create_factory();

  bool dynmodel_uvd = ::dgs_get_settings()->getBlockByNameEx("unitedVdata.dynModel")->getBool("use", false);
  bool dynmodel_uvd_can_rebuild = ::dgs_get_settings()->getBlockByNameEx("unitedVdata.dynModel")->getBool("canRebuild", false);
  debug("unitedVdata.dynModel: use=%d canRebuild=%d", dynmodel_uvd, dynmodel_uvd_can_rebuild);

  ::register_rendinst_gameres_factory();
  ::register_avif_tex_load_factory();
  ::register_png_tex_load_factory();
  ::register_tga_tex_load_factory();
  ::register_jpeg_tex_load_factory();
  ::register_any_vromfs_tex_create_factory("png|jpg|tga|ddsx");
  ::register_any_tex_load_factory();
  CollisionResource::registerFactory();
}

static InitOnDemand<DaEditor3Engine> engine;

static bool assertion_handler(bool verify, const char *file, int line, const char *func, const char *cond, const char *fmt,
  const DagorSafeArg *arg, int anum)
{
  if (!engine)
    return true;

  char fmtBuf[1024];
  DagorSafeArg::print_fmt(fmtBuf, sizeof(fmtBuf), fmt ? fmt : "", arg, anum);

  char buf[1024];
  int w = snprintf(buf, sizeof(buf), "%s failed in %s:%d,%s() :\n\"%s\"%s%s\n", verify ? "verify" : "assert", file, line, func, cond,
    fmtBuf[0] ? "\n\n" : "", fmtBuf[0] ? fmtBuf : "");
  if ((unsigned)w >= sizeof(buf))
    memcpy(buf + sizeof(buf) - 4, "...", 4);
  const char *string = &buf[0];
  engine->getConsoleLogWriter()->addMessage(ILogWriter::ERROR, string);
  logmessage(LOGLEVEL_FATAL, "FATAL ERROR:\n%s", string);
  debug_flush(true);
  _exit(13);
  return true;
}

static ImpostorGenerator::GenerateResponse impostor_export_callback(DagorAsset *asset, unsigned int ind, unsigned int count)
{
  if (ImpostorGenerator::interrupted)
    return ImpostorGenerator::GenerateResponse::ABORT;
  String title(0, "[%d/%d] Processing %s", ind + 1, count, asset->getName());
  ::win32_set_window_title(title);
  engine->conNote("%s", title.c_str());

  // clear references to already processed RIs, dump streaming state
  rendinst::clearRIGen();
  unitedvdata::riUnitedVdata.clear();
  free_unused_game_resources();
  dump_texture_streaming_memory_state();
  // largely increase frame# to allow aggressive textures discard when needed
  interlocked_add(dagor_frame_no_int, 600);
  return ImpostorGenerator::GenerateResponse::PROCESS;
}

int DagorWinMain(int nCmdShow, bool /*debugmode*/)
{
  signal(SIGINT, ctrl_break_handler);
  dgs_assertion_handler = assertion_handler;

  DataBlock::fatalOnMissingFile = false;
  DataBlock::fatalOnLoadFailed = false;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::fatalOnMissingVar = false;

  const ImpostorOptions options = parse_impostor_options();

  if (!options.valid)
  {
    print_impostor_options();
    return 1;
  }

  if (options.classic_dbg)
    start_classic_debug_system("debug.log");
  else
    start_debug_system(__argv[0]);
  char stamp_buf[256];
  debug(dagor_get_build_stamp_str(stamp_buf, sizeof(stamp_buf), "\n"));

  if (!::symhlp_load("daKernel" DAGOR_DLL))
    DEBUG_CTX("can't load sym for: %s", "daKernel" DAGOR_DLL);

  String path_to_blk;
  DataBlock appblk;
  if (!appblk.load(options.appBlk.c_str()))
  {
    logerr("cannot load %s", options.appBlk.c_str());
    return 1;
  }
  else
  {
    path_to_blk = String(options.appBlk.c_str());
    ::dd_get_fname_location(path_to_blk, options.appBlk.c_str());
  }

  engine.demandInit();

  char app_dir[512];
  dd_get_fname_location(app_dir, path_to_blk.c_str());
  if (!app_dir[0])
    strcpy(app_dir, "./");

  DataBlock *global_settings_blk = const_cast<DataBlock *>(::dgs_get_settings());

  global_settings_blk->addBlock("video")->setStr("mode", "windowed");
  global_settings_blk->addBlock("video")->setStr("resolution", "480x270");

#if _TARGET_PC_WIN
  if (strcmp(appblk.getStr("impostorbakerDriver", ""), "dx12") == 0)
  {
    global_settings_blk->addBlock("video")->setStr("driver", "dx12");
    global_settings_blk->addBlock("dx12")->setStr("executionMode", "immediate");
  }
  else
    global_settings_blk->addBlock("video")->setStr("driver", "dx11");
#endif

  cpujobs::init();

  const DataBlock &blk = *appblk.getBlockByNameEx("assets");
  const DataBlock *exp_blk = blk.getBlockByName("export");
  const DataBlock *game_blk = appblk.getBlockByName("game");

  const char *sh_file = appblk.getStr("impostorbakerShaders", appblk.getStr("shaders", "compiledShaders/tools"));
  appblk.setStr("appDir", app_dir);

  DataBlock texStreamingBlk;
  ::load_tex_streaming_settings(String(0, "%s/application.blk", appblk.getStr("appDir")), &texStreamingBlk);

  global_settings_blk->removeBlock("texStreaming");
  global_settings_blk->addNewBlock(&texStreamingBlk, "texStreaming")->setBool("texLoadOnDemand", true);
  init_managed_textures_streaming_support();
  if (int resv_tid_count = appblk.getInt("texMgrReserveTIDcount", 128 << 10))
    enable_res_mgr_mt(true, resv_tid_count);

  ::dagor_init_video("DagorWClass", nCmdShow, NULL, "Loading...");
  dagor_install_dev_fatal_handler(NULL);

  ::startup_shaders(String(0, "%s/%s", app_dir, sh_file).c_str());
  ::startup_game(RESTART_ALL);

  CALL_AT_END_OF_SCOPE(flush_files());

  ::shaders_register_console(true);

  init_res_factories();

  ShaderGlobal::enableAutoBlockChange(true);
  ShaderGlobal::set_int(get_shader_variable_id("in_editor", true), 0);

  DataBlock impostorShaderVarsBlk;
  const DataBlock *impostorBlock = blk.getBlockByName("impostor");
  String folder;
  if (impostorBlock)
  {
    G_ASSERTF(impostorBlock->paramExists("data_folder"), "Add data_folder:t to the assets/impostor block in application.blk");
    folder = String(0, "%s/%s/", app_dir, impostorBlock->getStr("data_folder"));
  }
  else
  {
    G_ASSERTF(blk.paramExists("impostor_data_folder"), "Add the assets/impostor block to application.blk");
    folder = String(0, "%s/%s/", app_dir, blk.getStr("impostor_data_folder"));
  }
  simplify_fname(folder);
  String impostorShaderVarsFile = String(0, "%simpostor_shader_vars.blk", folder.c_str());
  debug("impostorShaderVarsFile: looking for a file at <%s>", impostorShaderVarsFile);
  if (::dd_file_exists(impostorShaderVarsFile.c_str()))
  {
    debug("impostorShaderVarsFile: was found");
    impostorShaderVarsBlk.load(impostorShaderVarsFile.c_str());
  }
  else
  {
    debug("impostorShaderVarsFile: was not found");
  }

  ShaderGlobal::set_vars_from_blk(*::dgs_get_settings()->getBlockByNameEx("shaderVar"), true);
  if (impostorShaderVarsBlk != nullptr)
  {
    const DataBlock *shaderBlock = impostorShaderVarsBlk.getBlockByName("shaderVar");
    G_ASSERTF(shaderBlock != nullptr, "Add shaderVar{} block to the impostor_shader_vars.blk");
    ShaderGlobal::set_vars_from_blk(*shaderBlock, true);
  }

  startup_game(RESTART_ALL);

  bool loadDDSxPacks = blk.getStr("ddsxPacks", nullptr) != nullptr;
  if (exp_blk)
  {
    ::set_gameres_sys_ver(2);
    if (!loadDDSxPacks)
      texconvcache::init_build_on_demand_tex_factory(engine->getAssetManager(), engine->getConsoleLogWriter());
  }
  if (blk.getStr("gameRes", nullptr) || blk.getStr("ddsxPacks", nullptr))
  {
    int nid = blk.getNameId("prebuiltGameResFolder");
    String resDir;
    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamType(i) == DataBlock::TYPE_STRING && blk.getParamNameId(i) == nid)
      {
        resDir.printf(DAGOR_MAX_PATH, "%s/%s/", app_dir, blk.getStr(i));
        ::scan_for_game_resources(resDir, true, blk.getStr("ddsxPacks", nullptr));
      }
  }

  G_ASSERT(DAEDITOR3.initAssetBase(app_dir));
  if (get_max_managed_texture_id())
    debug("tex/res registered with AssetBase:   %d", get_max_managed_texture_id().index());

  rendinst::configurateRIGen(*appblk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getBlockByNameEx("config"));
  rendinst::initRIGen(true, 80, 8000.0f);

  struct SceneToEnablePresent : public DagorGameScene
  {
    ImpostorGenerator *app = nullptr;
    void actScene() override {}
    void drawScene() override
    {
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      if (app)
        app->getImpostorBaker()->drawLastExportedImage();
    }
  } scene;

  int ret = 0;
  {
    RenderableInstanceLodsResource::on_higher_lod_required = nullptr; // force lod0 streaming
    ImpostorGenerator app{app_dir, appblk, &engine->getAssetManager(), engine->getConsoleLogWriter(), true, impostor_export_callback};
    scene.app = &app;
    dagor_select_game_scene(&scene);
    dagor_reset_spent_work_time();
    ret = app.run(options) ? 0 : 1;
    if (ret == 0)
      app.generateQualitySummary("impostor_baker_quality_summery.csv");
    scene.app = nullptr;
  }

  dagor_select_game_scene(nullptr);
  rendinst::clearRIGen();
  rendinst::termRIGen();
  reset_game_resources();
  texconvcache::term_build_on_demand_tex_factory();
  engine.demandDestroy();

  shutdown_game(RESTART_ALL);
  cpujobs::term(true);
  return ret;
}

#if !_TARGET_PC_WIN
void DagorWinMainInit(bool /*debugmode*/) { DagorWinMainInit(0, false); }
int DagorWinMain(bool /*debugmode*/) { return DagorWinMain(0, false); }
#endif
