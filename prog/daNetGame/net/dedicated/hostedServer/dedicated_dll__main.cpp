// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/main.cpp"
#include <debug/dag_hwExcept.h>
#include <startup/dag_winCommons.h>
#include <startup/dag_addBasePathDef.h>
#include <locale.h>
#include <supp/dag_dllexport.h>
#include <landMesh/landRayTracer.h>
#include <heightmap/heightmapPhysHandler.h>
#include <fftWater/fftWater.h>
#include <osApiWrappers/dag_sharedMem.h>
#include <gameRes/dag_gameResProxyFactory.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResHooks.h>
#include <rendInst/rendInstGen.h>
#if _TARGET_PC
static void init_platform_specific(const DataBlock &) {}
#else
extern void init_platform_specific(const DataBlock &);
static int suppress_log_cb(int, const char *, const void *, int, const char *, int) { return 0; }
#endif

extern void default_crt_init_kernel_lib();
extern void default_crt_init_core_lib();
extern "C" const char *dagor_get_build_stamp_str(char *buf, size_t bufsz, const char *suffix);
namespace rendinst
{
extern bool allowOptimizeCollResOnLoad;
} // namespace rendinst

static GameResProxyTable gameres_proxy_table;
static void term_res_factories_for_hosted_internal_server();

DAG_DLL_EXPORT
void exit_game_exported(const char *reason_static_str)
{
  if (auto *shared_mem = LandRayTracer::sharedMem)
    shared_mem->dumpContents("HIS:beforeExit");
  debug("exit game exported %s", reason_static_str);
  exit_game(reason_static_str);
}

DAG_DLL_EXPORT
bool dng_is_app_terminating_exported() { return dng_is_app_terminating(); }

static bool quiet_fatal_handler(const char * /*msg*/, const char * /*stack_msg*/, const char * /*file*/, int /*line*/)
{
  exit_game("fatal");
  return false;
}

DAG_DLL_EXPORT
bool __cdecl start_internal_server(const DataBlock &inp, void(__cdecl *handler)(const char *))
{
  dgs_on_memory_leak_detected_handler = handler;
  dgs_fatal_handler = quiet_fatal_handler;
  g_destroy_init_on_demand_in_dtor = true; // to avoid memory leaks on DLL unload

  win_recover_systemroot_env();
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  setlocale(LC_NUMERIC, "C");
  default_crt_init_kernel_lib();
  default_crt_init_core_lib();

  delete[] new String[1]; // do this dummy new/delete to force linker to use OWR implementation of these (instead of RTL ones)

  Tab<const char *> argv_storage;
  argv_storage.reserve(inp.paramCountByName("arg"));
  dblk::iterate_params_by_name_and_type(inp, "arg", DataBlock::TYPE_STRING,
    [&inp, &argv_storage](int idx) { argv_storage.push_back(inp.getStr(idx)); });

  dgs_init_argv(argv_storage.size(), (char **)argv_storage.data());
  init_platform_specific(inp);
  if (auto *shared_mem = (GlobalSharedMemStorage *)(void *)inp.getInt64("sharedMemPtr", 0))
  {
    shared_mem->addRefLocal();
    LandRayTracer::sharedMem = shared_mem;
    HeightmapPhysHandler::sharedMem = shared_mem;
    HeightmapPhysHandler::dumpSharingReadOnly = inp.getBool("shareHmapRO", false);
    fft_water::WaterHeightmap::sharedMem = shared_mem;
  }

  ::dgs_execute_quiet = ::dgs_get_argv("quiet");

  static DagorSettingsBlkHolder stgBlkHolder;
  bool debugmode = ::dgs_get_argv("debug") != nullptr;
#if _TARGET_PC
  dagor_init_base_path();
  dagor_change_root_directory(::dgs_get_argv("rootdir"));
  start_classic_debug_system(dng_get_log_prefix(false));
#else
  debug_enable_thread_ids(true);
  char sbuf[256];
  debug(dagor_get_build_stamp_str(sbuf, sizeof(sbuf), "\n\n"));
  measure_cpu_freq();
#endif

  if (auto *tbl = (const GameResProxyTable *)(void *)inp.getInt64("gameResProxyTablePtr", 0))
  {
    unsigned tbl_sz = inp.getInt("gameResProxyTableSz", 0);
    if (tbl && tbl_sz == sizeof(gameres_proxy_table) &&
        reinterpret_cast<const GameResProxyTable *>(tbl)->apiVersion == GameResProxyTable::GRPT_API_VERSION)
    {
      memcpy(&gameres_proxy_table, tbl, tbl_sz);
      debug("%s(&ptable, %d): installed, apiVer=%d", "gameResProxyTable", sizeof(gameres_proxy_table), gameres_proxy_table.apiVersion);
    }
    else
    {
      memset(&gameres_proxy_table, 0, sizeof(gameres_proxy_table));
      logerr("%s(%p, %u): failed due to mismatched size (req.sz=%u) or apiVer=%u (req.ver=%u)", "gameResProxyTable", tbl, tbl_sz,
        sizeof(gameres_proxy_table),
        (tbl && tbl_sz > sizeof(gameres_proxy_table.apiVersion)) ? reinterpret_cast<const GameResProxyTable *>(tbl)->apiVersion : 0,
        GameResProxyTable::GRPT_API_VERSION);
    }
  }

  DagorWinMainInit(0, debugmode);
  int retcode = DagorWinMain(0, debugmode);

  if (auto *shared_mem = LandRayTracer::sharedMem)
  {
    LandRayTracer::sharedMem = nullptr;
    HeightmapPhysHandler::sharedMem = nullptr;
    fft_water::WaterHeightmap::sharedMem = nullptr;
    shared_mem->delRefLocal();
  }
  term_res_factories_for_hosted_internal_server();

  ::dgs_argc = 0;
  ::dgs_argv = nullptr;
  argv_storage.clear();
  flush_debug_file();
#if !_TARGET_PC
  debug_set_log_callback(&suppress_log_cb);
#endif
  return retcode == 0;
}

static Tab<GameResourceFactory *> proxy_gameres_factories;
static unsigned proxy_gameres_types[] = { //
  GeomNodeTreeGameResClassId, Anim2DataGameResClassId, CollisionGameResClassId, ImpostorDataGameResClassId, FastPhysDataGameResClassId
  /*, AnimBnlGameResClassId, AnimGraphGameResClassId*/};

static bool proxy_on_preload_all_required_res(gameres_rrl_ptr_t rrl)
{
  gameres_rrl_ptr_t proxy_rrl = nullptr;
  bool res = split_res_restriction_list(rrl, proxy_rrl, make_span_const(proxy_gameres_types));
  if (proxy_rrl)
  {
    gameres_proxy_table.preload_all_required_res(proxy_rrl);
    free_res_restriction_list(proxy_rrl);
  }
  return res;
}

bool init_res_factories_for_hosted_internal_server()
{
  if (!gameres_proxy_table.apiVersion)
    return false;

  for (unsigned &cls_id : proxy_gameres_types)
    if (auto *f = ProxyGameResFactory::create_and_add_proxy_factory(gameres_proxy_table, cls_id, false))
    {
      proxy_gameres_factories.push_back(f);
      debug("registered gameres proxy factory(0x%X, %s)", cls_id, f->getResClassName());
      if (cls_id == CollisionGameResClassId)
        rendinst::allowOptimizeCollResOnLoad = false;
    }
    else
      cls_id = 0;
  gamereshooks::on_preload_all_required_res = proxy_on_preload_all_required_res;
  ::register_animchar_gameres_factory();
  ::register_character_gameres_factory();
  rendinst::register_land_gameres_factory();
  return true;
}
static void term_res_factories_for_hosted_internal_server()
{
  for (auto *f : proxy_gameres_factories)
  {
    remove_factory(f);
    delete f;
  }
  clear_and_shrink(proxy_gameres_factories);
}

#if _TARGET_PC_MACOSX
class CGRect
{};
class NSWindow *macosx_create_dagor_window(const char *, int, int, class NSView *, CGRect) { return nullptr; }
#endif
