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
#include <startup/dag_fatalHandler.inc.cpp>
#include <startup/dag_globalSettings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_restart.h>
#include <startup/dag_startupTex.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_startupModules.h>
#include <workCycle/dag_workCycle.h>
#include <de3_dxpFactory.h>
#include <libTools/util/makeBindump.h>
#include <rendInst/rendInstGen.h>
#include <libTools/util/setupTexStreaming.h>
#include <assets/assetPlugin.h>

#include <startup/dag_winMain.inc.cpp>

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

namespace rendinstgen
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
} // namespace rendinstgen

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
    return false;
  char buf[1024];
  int w = snprintf(buf, sizeof(buf), "%s failed in %s:%d,%s() :\n\"%s\"%s%s\n", verify ? "verify" : "assert", file, line, func, cond,
    fmt ? "\n\n" : "", fmt ? fmt : "");
  if ((unsigned)w >= sizeof(buf))
    memcpy(buf + sizeof(buf) - 4, "...", 4);
  const char *string = &buf[0];
  engine->getConsoleLogWriter()->addMessage(ILogWriter::ERROR, string);
  return false;
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

  dd_add_base_path(df_get_real_folder_name("./"));
  if (!::symhlp_load("daKernel" DAGOR_DLL))
    debug_ctx("can't load sym for: %s", "daKernel" DAGOR_DLL);

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

  char fpath_buf[512];
  const char *app_dir = ::_fullpath(fpath_buf, path_to_blk.c_str(), 512);

  DataBlock *global_settings_blk = const_cast<DataBlock *>(::dgs_get_settings());

  global_settings_blk->addBlock("video")->setStr("mode", "windowed");
  global_settings_blk->addBlock("video")->setStr("resolution", "480x270");

  cpujobs::init();

  const DataBlock &blk = *appblk.getBlockByNameEx("assets");
  const DataBlock *exp_blk = blk.getBlockByName("export");
  const DataBlock *game_blk = appblk.getBlockByName("game");

  const char *sh_file = appblk.getStr("shaders", "compiledShaders/tools");
  appblk.setStr("appDir", app_dir);

  DataBlock texStreamingBlk;
  ::load_tex_streaming_settings(String(0, "%s/application.blk", appblk.getStr("appDir")), &texStreamingBlk);

  global_settings_blk->removeBlock("texStreaming");
  global_settings_blk->addNewBlock(&texStreamingBlk, "texStreaming")->setInt("forceGpuMemMB", 1);
  init_managed_textures_streaming_support(-1);

  ::dagor_init_video("DagorWClass", nCmdShow, NULL, "Loading...");
  dagor_install_dev_fatal_handler(NULL);

  ::startup_shaders(String(0, "%s/%s", app_dir, sh_file).c_str());
  ::startup_game(RESTART_ALL);

  CALL_AT_END_OF_SCOPE(flush_files());

  ::shaders_register_console(true);

  init_res_factories();

  ShaderGlobal::enableAutoBlockChange(true);
  ShaderGlobal::set_int(get_shader_variable_id("in_editor", true), 0);
  ShaderGlobal::set_vars_from_blk(*::dgs_get_settings()->getBlockByNameEx("shaderVar"), true);

  startup_game(RESTART_ALL);

  bool loadDDSxPacks = blk.getStr("ddsxPacks", nullptr) != nullptr;
  if (exp_blk)
  {
    ::set_gameres_sys_ver(2);
    if (!loadDDSxPacks)
      ::init_dxp_factory_service();
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

  rendinst::configurateRIGen(*appblk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getBlockByNameEx("config"));
  rendinst::initRIGen(true, 80, 8000.0f);

  int ret = 0;
  {
    ImpostorGenerator app{app_dir, appblk, &engine->getAssetManager(), engine->getConsoleLogWriter()};
    ret = app.run(options) ? 0 : 1;
    if (ret == 0)
      app.generateQualitySummary("impostor_baker_quality_summery.csv");
  }

  rendinst::clearRIGen();
  rendinst::termRIGen();
  reset_game_resources();
  ::term_dxp_factory_service();
  engine.demandDestroy();

  shutdown_game(RESTART_ALL);
  cpujobs::term(true);
  return ret;
}
