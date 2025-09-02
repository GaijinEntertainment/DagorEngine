// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "settings.h"
#include "main.h"
#include "level.h"
#include "vromfs.h"
#include "gameLoad.h"
#include "render/renderer.h"
#include "ui/uiShared.h"
#include <startup/dag_globalSettings.h>
#include <startup/dag_loadSettings.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_simpleString.h>
#include <util/dag_oaHashNameMap.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <ioSys/dag_memIo.h>
#include <memory/dag_framemem.h>
#include <EASTL/unique_ptr.h>
#include <util/dag_string.h>
#include "appProfile.h"
#include <bindQuirrelEx/autoBind.h>
#include <sqrat.h>
#include <dataBlockUtils/blk2sqrat/blk2sqrat.h>
#include <net/dedicated.h>
#include <net/net.h>
#include <osApiWrappers/dag_fileIoErr.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_threads.h>
#include <perfMon/dag_perfTimer.h>
#include <folders/folders.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <ecs/core/entityManager.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_platform.h>
#include "crashFallbackHelper.h"
#include <main/settingsOverride.h>
#include <drv/hid/dag_hiGlobals.h>
#include "gameProjConfig.h"
#include <drv/3d/dag_info.h>

#if _TARGET_C1 | _TARGET_C2

#endif

#if _TARGET_C3

#endif

#if _TARGET_PC_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef GetObject
#endif

static eastl::unique_ptr<DataBlock> settings_override_blk;
static String settings_override_filename;


static void split_blk_path(String &path_buf, Tab<const char *> &parts)
{
  char *p = path_buf.data();
  parts.push_back(p);
  for (; *p; ++p)
  {
    if (*p == '/')
    {
      *p = 0;
      ++p;
      if (*p)
        parts.push_back(p);
    }
  }
}

String get_config_filename()
{
#if _TARGET_PC
  String settings_path(0, "%s.blk", gameproj::config_name_prefix());
#else
  String settings_path(0, "%s.%s.blk", gameproj::config_name_prefix(), get_platform_string_id());
#endif
  return settings_path;
}


static bool get_allowed_src_files_usage(String &stg_fn)
{
#if DAGOR_DBGLEVEL > 0 && (_TARGET_PC || _TARGET_C3)
  String config_blk_fn = get_config_filename();
  DataBlock cfg;
  if (config_blk_fn && dd_file_exist(config_blk_fn))
    dblk::load(cfg, config_blk_fn, dblk::ReadFlag::ROBUST);

  dgs_apply_command_line_to_config(&cfg);

  if (cfg.getBlockByNameEx("debug")->getBool("useAddonVromSrc", false))
  {
    stg_fn.setStrCat(cfg.getBlockByNameEx("debug")->getStr("vromSrcPath", "../prog/gameBase/"), gameproj::settings_fpath());
    return true;
  }
#endif
  stg_fn = gameproj::settings_fpath();
  return false;
}

static void prepare_before_loading_game_settings_blk(String &stg_fn)
{
  sceneload::GamePackage gameInfo;
  if (get_allowed_src_files_usage(stg_fn))
  {
    if (dblk::load(gameInfo.gameSettings, stg_fn, dblk::ReadFlag::ROBUST))
      sceneload::parse_addons(gameInfo, *gameInfo.gameSettings.getBlockByNameEx("addonBasePath"), true);
    else
      DAG_FATAL("failed to load settings: %s", stg_fn);
  }
  else
  {
    VromLoadInfo mainVrom = {gameproj::main_vromfs_fpath(), gameproj::main_vromfs_mount_dir(), "", false, false, ReqVromSign::Yes};
    apply_vrom_list_difference(DataBlock::emptyBlock, make_span_const(&mainVrom, 1));
    if (vromfs_check_file_exists(stg_fn) && dblk::load(gameInfo.gameSettings, stg_fn, dblk::ReadFlag::ROBUST))
      sceneload::parse_addons(gameInfo, *gameInfo.gameSettings.getBlockByNameEx("addonBasePath"), false,
        gameproj::main_vromfs_fpath());
    else
      DAG_FATAL("failed to find %s using signed vromfs %s (mounted to %s)", stg_fn, mainVrom.path, mainVrom.mount);
  }

  // load operator vromfs
  sceneload::add_operator_vromfs(gameInfo);

  // load vromfs and set base paths
  sceneload::load_package_files(gameInfo, /*load_game_res*/ false);
}

DataBlock *get_settings_override_blk() { return settings_override_blk.get(); }

#if DAGOR_DBGLEVEL > 0
static void apply_global_config(const SettingsHashMap *changed_settings = nullptr)
{
  DataBlock configBlk(framemem_ptr());
  const char *config_file = dedicated::is_dedicated() ? "server.config.blk" : "config.blk";
  if (dblk::load(configBlk, config_file, dblk::ReadFlag::ROBUST))
  {
    debug("apply config blk %s", config_file);
    OverrideFilter filter = gen_default_override_filter(changed_settings);
    dgs_apply_changes_to_config(configBlk, true, &filter);
    dgs_apply_config_blk(configBlk, true, false);
  }
  else
  {
    debug("failed to load config %s", config_file);
  }
}
#else
static void apply_global_config(const SettingsHashMap *changed_settings = nullptr) { G_UNUSED(changed_settings); }
#endif


static void apply_ime_to_video_mode()
{
  DataBlock overrideBlk;
  bool hasIme = HumanInput::keyboard_has_ime_layout();
  overrideBlk.addBlock("video")->setBool("forbidTrueFullscreen", hasIme);
  DEBUG_CTX("apply_ime_to_video_mode(), has IME = %d", hasIme);
  dgs_apply_config_blk(overrideBlk, true, false);
}


static void update_settings_from_reload_changes()
{
  const DataBlock *on_reload_params_blk = settings_override_blk->getBlockByNameEx("onReloadChanges");
  if (on_reload_params_blk->isEmpty())
    return;

  debug("have changes in onReloadChanges, merge them to settings first");
  merge_data_block(*settings_override_blk, *on_reload_params_blk);
  settings_override_blk->removeBlock("onReloadChanges");
}

void initial_load_settings()
{
  debug("load settings");
  if (!settings_override_blk)
  {
    settings_override_filename = get_config_filename();
    debug("Loading %s", settings_override_filename);
    settings_override_blk.reset(new DataBlock);
#if _TARGET_PC && !_TARGET_C4
    dblk::load(*settings_override_blk, settings_override_filename, dblk::ReadFlag::ROBUST);
#else
#if _TARGET_C1 | _TARGET_C2


#elif _TARGET_C3


#elif _TARGET_ANDROID
    settings_override_filename.printf(0, "%s/%s.blk", folders::get_game_dir(), gameproj::config_name_prefix());
#else
    settings_override_filename.printf(0, "%s/%s.blk", folders::get_gamedata_dir(), gameproj::config_name_prefix());
#endif

    DataBlock user_blk;
    if (dblk::load(user_blk, settings_override_filename, dblk::ReadFlag::ROBUST))
    {
      debug("Loading (savedata) %s", settings_override_filename);
      *settings_override_blk = user_blk;
    }
#endif

    update_settings_from_reload_changes();
  }

  String stg_fn;
  prepare_before_loading_game_settings_blk(stg_fn);

  {
    G_ASSERTF(dd_file_exists(stg_fn), "settings file %s missing", stg_fn);

#if _TARGET_XBOX
    String settings_path(0, "G:\\%s", get_config_filename().c_str());
#else
    String settings_path = get_config_filename();
#endif
#if _TARGET_PC && !_TARGET_C4
    dgs_load_settings_blk_ex(true, stg_fn, *settings_override_blk, false, true);
    dgs_apply_essential_pc_preset_params_to_config_from_cmd();
#else
    dgs_load_settings_blk(true, stg_fn, settings_path, false, true);
    dgs_apply_changes_to_config(*settings_override_blk, /*need_merge_cmd*/ false);
    dgs_apply_config_blk(*settings_override_blk, false, false);
#endif
  }
  apply_global_config();
  apply_ime_to_video_mode();

  load_gameparams();
  app_profile::apply_settings_blk(*dgs_get_settings());

#if CRASH_FALLBACK_ENABLED
  if (::dgs_get_settings()->getBlockByNameEx("dx12")->getBool("previous_run_crash_fallback_enabled", CRASH_FALLBACK_ENABLED_DEFAULT))
    crash_fallback_helper = eastl::make_unique<CrashFallbackHelper>();
#endif
}

namespace rendinst
{
extern int maxRiGenPerCell, maxRiExPerCell;
}
void load_gameparams()
{
  dgs_load_game_params_blk(gameproj::params_fpath());
  const_cast<DataBlock *>(::dgs_get_game_params())->setInt("rendinstExtraSubstFaceThres", 0);
  rendinst::maxRiGenPerCell = ::dgs_get_game_params()->getInt("maxRiGenPerCell", 0x10000);
  rendinst::maxRiExPerCell = ::dgs_get_game_params()->getInt("maxRiExPerCell", 0x10000);
}


void clear_settings()
{
  settings_override_blk.reset();
  clear_and_shrink(settings_override_filename);
  const_cast<DataBlock *>(dgs_get_settings())->reset();
  const_cast<DataBlock *>(dgs_get_game_params())->reset();
}


static SQInteger get_setting_by_blk_path(HSQUIRRELVM v)
{
  Sqrat::Var<const SQChar *> path(v, 2);
  String pathBuf(path.value);
  Tab<const char *> parts;
  split_blk_path(pathBuf, parts);

  if (parts.size() < 1)
    return sqstd_throwerrorf(v, "Bad setting blk path %s, should have at least one value", path.value);

  const DataBlock *blk = ::dgs_get_settings();

  for (int i = 0; i < (int)parts.size() - 1; ++i)
    blk = blk->getBlockByNameEx(parts[i]);

  DataBlock::ParamType type = DataBlock::TYPE_NONE;
  int paramIdx = blk->findParam(parts.back());
  if (paramIdx >= 0)
    type = (DataBlock::ParamType)blk->getParamType(paramIdx);

  switch (type)
  {
    case DataBlock::TYPE_NONE:
      if (const DataBlock *b = blk->getBlockByName(parts.back()))
      {
        sq_pushobject(v, blk_to_sqrat(v, *b).GetObject());
        break;
      }
      sq_pushnull(v);
      break;
    case DataBlock::TYPE_BOOL: sq_pushbool(v, blk->getBool(paramIdx)); break;
    case DataBlock::TYPE_INT: sq_pushinteger(v, blk->getInt(paramIdx)); break;
    case DataBlock::TYPE_REAL: sq_pushfloat(v, blk->getReal(paramIdx)); break;
    case DataBlock::TYPE_STRING: sq_pushstring(v, blk->getStr(paramIdx), -1); break;
    default:
      G_ASSERTF(0, "Unsupported DataBlock param type %d", type);
      sq_pushnull(v);
      break;
  }

  return 1;
}

static SQInteger set_setting_by_blk_path(HSQUIRRELVM v)
{
  Sqrat::Var<const SQChar *> path(v, 2);
  String pathBuf(path.value);
  Tab<const char *> parts;
  split_blk_path(pathBuf, parts);

  if (parts.size() < 1)
    return sqstd_throwerrorf(v, "Bad setting blk path %s, should have at least one value", path.value);

  DataBlock *blk = settings_override_blk.get();
  bool force_apply_all = ::dgs_get_settings()->getBool("__allow_force_apply_all", false);
  const DataBlock *aco = ::dgs_get_settings()->getBlockByName("__allowedConfigOverrides");

  bool allow_all_subfolder_params = false;
  for (int i = 0; i < (int)parts.size() - 1; ++i)
  {
    blk = blk->addBlock(parts[i]);
    aco = aco ? aco->getBlockByName(parts[i]) : nullptr;
    allow_all_subfolder_params =
      aco ? aco->getBool("__allow_all_subfolder_params", allow_all_subfolder_params) : allow_all_subfolder_params;
  }

  Sqrat::Var<Sqrat::Object> val(v, 3);
  const char *paramName = parts.back();

  if (!force_apply_all && !allow_all_subfolder_params &&
      (!aco || (!aco->getBool("__allow_all_params", false) && aco->findParam(paramName) < 0)))
    logmessage(DAGOR_DBGLEVEL > 0 ? LOGLEVEL_ERR : _MAKE4C('nEvt'), "settings.blk override for '%s' is not allowed!", path.value);

  switch (val.value.GetType())
  {
    case OT_BOOL: blk->setBool(paramName, val.value.Cast<bool>()); break;
    case OT_INTEGER: blk->setInt(paramName, val.value.Cast<int>()); break;
    case OT_FLOAT: blk->setReal(paramName, val.value.Cast<float>()); break;
    case OT_STRING: blk->setStr(paramName, val.value.GetVar<const SQChar *>().value); break;
    case OT_TABLE:
    {
      // use existed sub blk if it exist to keep sub blk order
      // to prevent changing of whole blk hash
      DataBlock *subBlk = blk->getBlockByName(paramName);
      if (subBlk)
        subBlk->clearData();
      else
        subBlk = blk->addNewBlock(paramName);
      sqrat_to_blk(*subBlk, val.value);
    }
    break;
    default: G_ASSERTF(0, "Unsupported setting script type %X for '%s'", val.value.GetType(), path.value);
  }

  return 0;
}

static SQInteger remove_setting_by_blk_path(HSQUIRRELVM v)
{
  Sqrat::Var<const SQChar *> path(v, 2);
  String pathBuf(path.value);
  Tab<const char *> parts;
  split_blk_path(pathBuf, parts);

  if (parts.size() < 1)
    return sqstd_throwerrorf(v, "Bad setting blk path %s, should have at least one value", path.value);

  DataBlock *blk = settings_override_blk.get();

  for (int i = 0; blk && i < (int)parts.size() - 1; ++i)
    blk = blk->getBlockByName(parts[i]);

  if (!blk)
    return 0;

  if (!blk->removeBlock(parts.back()))
    blk->removeParam(parts.back());

  return 0;
}

static SQInteger save_settings_sq(HSQUIRRELVM)
{
  save_settings(nullptr);
  return 0;
}
static void save_changed_settings_sq(Sqrat::Array changed_fields)
{
  SettingsHashMap nameMapChanges;
  for (SQInteger i = 0, n = changed_fields.Length(); i < n; ++i)
  {
    Sqrat::Object fieldNameObj = changed_fields.GetSlot(i);
    const char *fieldName = sq_objtostring(&fieldNameObj.GetObject());
    nameMapChanges.addNameId(fieldName ? fieldName : "<undefined>");
  }
  save_settings(&nameMapChanges);
}

static SQInteger set_setting_by_blk_path_and_save(HSQUIRRELVM v)
{
  SQInteger ret = set_setting_by_blk_path(v);
  save_settings_sq(v);
  return ret;
}

static int volatile settings_changed_generation = 0, settings_saved_generation = 0, settings_need_commit = 0;
static int save_stg_jobmgr = -1;
static int64_t save_stg_last_reft = 0;

struct SaveConfigBlkJob final : public cpujobs::IJob
{
  enum
  {
    MIN_INTERVAL_USEC = 30 * 1000000
  };
  DataBlock cfg;
  int gen = 0;

  SaveConfigBlkJob()
  {
    cfg = *settings_override_blk;
    gen = interlocked_acquire_load(settings_changed_generation);
  }
  const char *getJobName(bool &) const override { return "SaveConfigBlkJob"; }
  void doJob() override
  {
    for (;;) // eternal loop with explicit break
    {
      if (interlocked_acquire_load(settings_changed_generation) > gen)
      {
        debug("skip saving settings(async): (DISK=%d)", gen);
        return;
      }
      if (is_main_thread() || interlocked_relaxed_load(save_stg_jobmgr) == cpujobs::COREID_IMMEDIATE ||
          profile_usec_passed(save_stg_last_reft, MIN_INTERVAL_USEC) || interlocked_acquire_load(save_stg_jobmgr) < 0 ||
          interlocked_relaxed_load(settings_need_commit) > 0)
        break;
      sleep_msec(100);
    }

#if _TARGET_C1 | _TARGET_C2


#elif _TARGET_C3


#else
    const String &dest_fname = settings_override_filename;
#endif

    DataBlock existing_cfg;
    bool cfg_is_changed = true;
    if (dd_file_exists(dest_fname) && dblk::load(existing_cfg, dest_fname, dblk::ReadFlag::ROBUST))
      cfg_is_changed = !equalDataBlocks(cfg, existing_cfg);

    if (cfg_is_changed)
    {
      debug("saving settings(async): %s (DISK=%d)", dest_fname, gen);
      save_stg_last_reft = profile_ref_ticks();
      cfg.saveToTextFile(dest_fname);
    }
    else
      debug("saving settings(async): %s (DISK=%d), skipped writing identical file", dest_fname, gen);

    interlocked_release_store(settings_saved_generation, gen);
  }
  void releaseJob() override { delete this; }
};

void start_settings_async_saver_jobmgr()
{
  save_stg_last_reft = profile_ref_ticks();
  using namespace cpujobs;
  if (!dedicated::is_dedicated())
    save_stg_jobmgr = create_virtual_job_manager(192 << 10, WORKER_THREADS_AFFINITY_MASK, "SaveSettingsJobMgr",
      DEFAULT_THREAD_PRIORITY, DEFAULT_IDLE_TIME_SEC / 2);
  else // Dedicated is expected to be single-threaded
    save_stg_jobmgr = COREID_IMMEDIATE;
  if (save_stg_jobmgr < 0)
  {
    logerr("failed to create save settings jobmgr");
    save_stg_jobmgr = cpujobs::COREID_IMMEDIATE;
  }
}
void stop_settings_async_saver_jobmgr()
{
  if (save_stg_jobmgr != cpujobs::COREID_IMMEDIATE)
  {
    cpujobs::destroy_virtual_job_manager(save_stg_jobmgr, false);
    // destroy_virtual_job_manager does not wait for jobs to be fully finished
    // do it manually to avoid race
    const int checkJobMgr = interlocked_exchange(save_stg_jobmgr, -1);
    while (cpujobs::is_job_manager_busy(checkJobMgr))
      sleep_msec(1);
  }
  if (interlocked_acquire_load(settings_changed_generation) != interlocked_acquire_load(settings_saved_generation))
    cpujobs::add_job(cpujobs::COREID_IMMEDIATE, new SaveConfigBlkJob);
}
void get_settings_async_change_state(unsigned &out_change_gen, unsigned &out_saved_gen)
{
  out_change_gen = interlocked_acquire_load(settings_changed_generation);
  out_saved_gen = interlocked_acquire_load(settings_saved_generation);
}

void commit_settings_changes()
{
  debug("force comit settings changes");
  while (!are_settings_changes_committed() && interlocked_acquire_load(save_stg_jobmgr) != cpujobs::COREID_IMMEDIATE)
  {
    interlocked_increment(settings_need_commit);
    sleep_msec(100);
  }
  interlocked_release_store(settings_need_commit, 0);
  debug("settings changes commited");
}

void save_settings(const SettingsHashMap *changed_settings, bool apply_settings)
{
  interlocked_increment(settings_changed_generation);
  debug("saving settings: %s (MEM=%d DISK=%d)", settings_override_filename, interlocked_acquire_load(settings_changed_generation),
    interlocked_acquire_load(settings_saved_generation));
  const int jobMgrId = interlocked_acquire_load(save_stg_jobmgr);
  if (jobMgrId != cpujobs::COREID_IMMEDIATE)
    cpujobs::reset_job_queue(jobMgrId, false);
  cpujobs::add_job(jobMgrId, new SaveConfigBlkJob);

  if (apply_settings)
  {
#if _TARGET_PC
    dgs_load_settings_blk_ex(true, NULL, *settings_override_blk, false, true, true, changed_settings);
#else
    dgs_load_settings_blk(true, NULL, get_config_filename(), false, true);
    dgs_apply_config_blk(*settings_override_blk, false, false);
#endif
    apply_global_config(changed_settings);
    apply_ime_to_video_mode();
    apply_settings_override_entity();
  }
}

void commit_settings(const eastl::function<void(DataBlock &)> &mutate_cb)
{
  auto savedCb = dag_on_read_error_cb;
  dag_on_read_error_cb = NULL;

  static const char CONFIG_NAME[] = "settings.blk";
  DataBlock configBlk(framemem_ptr());
  if (dblk::load(configBlk, CONFIG_NAME, dblk::ReadFlag::ROBUST))
  {
    mutate_cb(configBlk);
    configBlk.saveToTextFile(CONFIG_NAME);
  }

  dag_on_read_error_cb = savedCb;
}

bool do_settings_changes_need_videomode_change(const FastNameMap &changed_fields)
{
  // clang-format off
  return changed_fields.getNameId("video/mode") >= 0
      || changed_fields.getNameId("video/resolution") >= 0
      || changed_fields.getNameId("video/staticResolutionScale") >= 0
      || changed_fields.getNameId("video/temporalUpsamplingRatio") >= 0
      || changed_fields.getNameId("video/vsync") >= 0
      || ( dgs_get_settings()->getBlockByNameEx("video")->getBool("vsync", false) &&
          (changed_fields.getNameId("video/nvidia_latency") >= 0 ||
           changed_fields.getNameId("video/amd_latency") >= 0 ||
           changed_fields.getNameId("video/intel_latency") >= 0))
      || changed_fields.getNameId("video/dlssQuality") >= 0
      || changed_fields.getNameId("video/dlssFrameGenerationCount") >= 0
      || changed_fields.getNameId("video/rayReconstruction") >= 0
      || changed_fields.getNameId("video/xessQuality") >= 0
      || changed_fields.getNameId("video/fsr2Quality") >= 0
      || changed_fields.getNameId("video/monitor") >= 0
      || changed_fields.getNameId("video/enableHdr") >= 0
      || changed_fields.getNameId("video/ssaa") >= 0
      || changed_fields.getNameId("video/fsr") >= 0
      || changed_fields.getNameId("video/antiAliasingMode") >= 0
      || changed_fields.getNameId("graphics/tsrQuality") >= 0;
  // clang-format on
}

void apply_settings_changes(const FastNameMap &changed_fields)
{
  bool applyAfterResetDevice = false;

  if (do_settings_changes_need_videomode_change(changed_fields))
  {
    d3d::update_window_mode();

    change_driver_reset_request(applyAfterResetDevice, true);
  }

#if _TARGET_C1 | _TARGET_C2


#elif _TARGET_SCARLETT
  if (changed_fields.getNameId("video/freqLevel") >= 0)
  {
    int freqLevel = ::dgs_get_settings()->getBlockByNameEx("video")->getInt("freqLevel", 2);
    d3d::driver_command(Drv3dCommand::SET_FREQ_LEVEL, &freqLevel);
  }
#endif

  if (changed_fields.getNameId("video/fpsLimit") >= 0)
  {
    int fpsLimit = g_entity_mgr->getOr(get_current_level_eid(), ECS_HASH("level__fpsLimit"), -1);
    set_corrected_fps_limit(fpsLimit);
  }

  if (get_world_renderer())
    get_world_renderer()->onSettingsChanged(changed_fields, applyAfterResetDevice);

  uishared::on_settings_changed(changed_fields, applyAfterResetDevice);
}

const char *get_platform_string()
{
  if (is_true_net_server())
    return nullptr;
  switch (get_platform_id()) // -V785
  {
    case TP_WIN32:
    case TP_WIN64:
    case TP_LINUX64:
    case TP_MACOSX: return "pc";
    default: return get_platform_string_id();
  }
}

int get_platformed_value(const DataBlock &blk, const char *key, int def)
{
  const char *pstring = get_platform_string();
  if (!pstring)
    return blk.getInt(key, def);
  size_t keyLen = strlen(key);
#if _TARGET_C4 && _TARGET_PC_WIN



#endif
  String pkey(keyLen + 16, "%s_%s", pstring, key);
  return blk.getInt(pkey, def);
}

const char *get_platformed_value(const DataBlock &blk, const char *key, const char *def)
{
  const char *pstring = get_platform_string();
  if (!pstring)
    return blk.getStr(key, def);
  size_t keyLen = strlen(key);
#if _TARGET_C4 && _TARGET_PC_WIN



#endif
  String pkey(keyLen + 16, "%s_%s", pstring, key);
  return blk.getStr(pkey, def);
}
///@module settings
SQ_DEF_AUTO_BINDING_MODULE_EX(bind_settings, "settings", sq::VM_ALL)
{
  Sqrat::Table tbl(vm);

  tbl //
    .SquirrelFunc("get_setting_by_blk_path", get_setting_by_blk_path, 2, ".s")
    .SquirrelFunc("set_setting_by_blk_path", set_setting_by_blk_path, 3, ".s.")
    .SquirrelFunc("set_setting_by_blk_path_and_save", set_setting_by_blk_path_and_save, 3, ".s.")
    .SquirrelFunc("remove_setting_by_blk_path", remove_setting_by_blk_path, 2, ".s.")
    .SquirrelFunc("save_settings", save_settings_sq, 1, NULL)
    .Func("save_changed_settings", save_changed_settings_sq)
    .Func("are_settings_changes_committed", are_settings_changes_committed)
    .Func("commit_settings_changes", commit_settings_changes)
    /**/;
  return tbl;
}

static void fill_changes_name_map(FastNameMap &name_map, const DataBlock &blk, const String &path_so_far)
{
  for (int i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock &childBlk = *blk.getBlock(i);
    fill_changes_name_map(name_map, childBlk, path_so_far + childBlk.getBlockName() + "/");
  }

  for (int i = 0; i < blk.paramCount(); i++)
    name_map.addNameId(path_so_far + blk.getParamName(i));
}

static void settings_explorer_window()
{
  // Ideas:
  // - search filter

  static bool editMode = false, autoApplyEdits = true, needCopySettings = true;
  bool applyEdits = false;

  ImGui::BeginMenuBar();
  if (ImGui::BeginMenu("Actions"))
  {
    if (ImGui::MenuItem("Dump to log"))
    {
      DynamicMemGeneralSaveCB dump(framemem_ptr());
      dgs_get_settings()->saveToTextStream(dump);
      dump.write("\0", 1);
      debug("Settings dump:\n%s", dump.data());
    }
    ImGui::EndMenu();
  }
  if (ImGui::BeginMenu("Modes"))
  {
    ImGui::Checkbox("Edit mode", &editMode);
    ImGui::Checkbox("Auto-apply edits", &autoApplyEdits);
    ImGui::EndMenu();
  }
  if (editMode)
  {
    ImGui::SameLine(ImGui::GetWindowWidth() - (autoApplyEdits ? 70 : 200)); // Align buttons to the right
    if (ImGui::Button(autoApplyEdits ? "Refresh" : "Revert and refresh"))
      needCopySettings = true;
    if (!autoApplyEdits)
      if (ImGui::Button("Apply"))
        applyEdits = true;
  }
  ImGui::EndMenuBar();

  if (editMode)
  {
    static DataBlock settingsCopy;
    static DataBlock settingsChanges;
    if (needCopySettings)
    {
      settingsCopy.setFrom(::dgs_get_settings());
      settingsChanges.clearData();
      needCopySettings = false;
    }
    DataBlock settingsChangesThisFrame(framemem_ptr());
    ImGuiDagor::BlkEdit(&settingsCopy, &settingsChangesThisFrame, true, "dgs_get_settings");
    if (!settingsChangesThisFrame.isEmpty())
    {
      merge_data_block_and_save_order(settingsCopy, settingsChangesThisFrame);
      merge_data_block(settingsChanges, settingsChangesThisFrame);
    }
    if ((applyEdits || autoApplyEdits) && !settingsChanges.isEmpty())
    {
      ::dgs_apply_config_blk(settingsChanges, true, false, true);
      FastNameMap changesNameMap;
      String pathSoFar;
      fill_changes_name_map(changesNameMap, settingsChanges, pathSoFar);
      apply_settings_changes(changesNameMap);
      needCopySettings = true;
    }
  }
  else
  {
    ImGuiDagor::Blk(::dgs_get_settings(), true, "dgs_get_settings");
  }
}

static void gameparams_explorer_window()
{
  ImGui::BeginMenuBar();
  if (ImGui::BeginMenu("Actions"))
  {
    if (ImGui::MenuItem("Dump to log"))
    {
      DynamicMemGeneralSaveCB dump(framemem_ptr());
      dgs_get_game_params()->saveToTextStream(dump);
      dump.write("\0", 1);
      debug("GameParams dump:\n%s", dump.data());
    }
    ImGui::EndMenu();
  }
  ImGui::EndMenuBar();

  ImGuiDagor::Blk(::dgs_get_game_params(), true, "dgs_get_game_params");
}

REGISTER_IMGUI_WINDOW_EX("Settings", "Settings explorer", nullptr, 100, ImGuiWindowFlags_MenuBar, settings_explorer_window);
REGISTER_IMGUI_WINDOW_EX("Settings", "GameParams explorer", nullptr, 100, ImGuiWindowFlags_MenuBar, gameparams_explorer_window);


void dgs_apply_changes_to_config(DataBlock &config_blk, bool need_merge_cmd, const OverrideFilter *override_filter)
{
  DataBlock cmdBlk;
  dgs_apply_command_line_to_config(&cmdBlk, override_filter);

  // check cmd block first
  const char *presetName = nullptr;
  presetName = cmdBlk.getBlockByNameEx("graphics")->getStr("consolePreset", nullptr);

  if (presetName == nullptr) // From saved data
    presetName = config_blk.getBlockByNameEx("graphics")->getStr("consolePreset", nullptr);

  if (presetName == nullptr) // From common settings for default
    presetName = ::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("consolePreset", nullptr);

  // first apply graphic preset settings
  if (presetName != nullptr)
    dgs_apply_console_preset_params(config_blk, presetName);

  // second apply cmd changes, overwrite all graphic preset settings if they are exist
  if (!cmdBlk.isEmpty())
    dgs_apply_config_blk(cmdBlk, false, false);
  if (need_merge_cmd)
    merge_data_block(config_blk, cmdBlk);
}

void dgs_apply_preset_params_impl(
  DataBlock &config_blk, const char *preset_name, const char *presets_list_block_name, const char *preset_type)
{
  const DataBlock *presetsBlk = ::dgs_get_settings()->getBlockByName(presets_list_block_name);
  if (!presetsBlk)
  {
    debug("Not exist block '%s' in settings.blk. Cannot read '%s' %s preset name.", presets_list_block_name, preset_name, preset_type);
    return;
  }

  const DataBlock *presetParamsBlk = presetsBlk->getBlockByName(preset_name);
  if (!presetParamsBlk)
  {
    debug("ERROR: Not found %s preset '%s' in '%s' block.", preset_type, preset_name, presets_list_block_name);
    return;
  }

  debug("Apply %s preset '%s'.", preset_type, preset_name);
  merge_data_block(config_blk, *presetParamsBlk);
}

void dgs_apply_console_preset_params(DataBlock &config_blk, const char *preset_name)
{
  dgs_apply_preset_params_impl(config_blk, preset_name, "consoleGraphicalPresets", "console");
}

void dgs_apply_essential_pc_preset_params(DataBlock &config_blk, const char *preset_name)
{
  dgs_apply_preset_params_impl(config_blk, preset_name, "pcGraphicalPresets", "pc (essential)");
}

void dgs_apply_essential_pc_preset_params_to_config_from_cmd()
{
  DataBlock cmdBlk;
  dgs_apply_command_line_to_config(&cmdBlk, nullptr);
  const char *presetName = cmdBlk.getBlockByNameEx("graphics")->getStr("preset", nullptr);
  if (presetName)
  {
    DataBlock config_blk;
    dgs_apply_essential_pc_preset_params(config_blk, presetName);
    dgs_apply_config_blk(config_blk, false, false);
  }
}

static bool get_gpu_score(int &gpu_score)
{
  DataBlock gpuDatabase;
  if (!gpuDatabase.load("config/gpu_database.blk"))
  {
    logerr("can't load gpu_database.blk, auto graphical settings will not be applied");
    return false;
  }

  const String rawDeviceName{d3d::get_device_name()};
  // skip:
  // - series of spaces
  // - info in parantheses (copyright, driver name, revision, chip name and other useless info)
  // and replace space with underscore
  String deviceName;
  int parenthesesCounter = 0; // there was no detected device names with nested parentheses, but use int for safety
  for (auto c : rawDeviceName)
  {
    if (c == ' ')
    {
      if (!deviceName.empty() && deviceName.back() != '_')
        deviceName.push_back('_');
    }
    else if (c == '(')
      ++parenthesesCounter;
    else if (c == ')')
      --parenthesesCounter;
    else if (parenthesesCounter == 0)
    {
      if (c == '-')
        c = '_';
      deviceName.push_back(c);
    }
  }

  // complete c-string, delete trailing underscore
  // AMD Radeon RX 570 (POLARIS10) -> AMD_Radeon_RX_570
  if (!deviceName.empty() && deviceName.back() == '_')
    deviceName.back() = '\0';
  else
    deviceName.push_back('\0');
  deviceName.updateSz();

  // NVIDIA_RTX_2080 -> nvidia_rtx_2080
  deviceName.toLower();

  if (auto underscoreBeforeGB = deviceName.find("_gb"); underscoreBeforeGB != nullptr)
    deviceName.erase(underscoreBeforeGB); // nvidia_rtx_3050_8_gb -> nvidia_rtx_3050_8gb

  // filter laptop GPUs:
  // mobile_something -> laptop_gpu_something
  // mobile_gpu_something -> laptop_gpu_something
  // laptop_something -> laptop_gpu_somethings
  if (deviceName.find("mobile") != nullptr)
    deviceName.replace("mobile", "laptop");
  const char LAPTOP_SUBSTR[] = "laptop";
  const size_t LAPTOP_SUBSTR_OFFSET = sizeof(LAPTOP_SUBSTR) - 1;
  if (auto laptopSubsstr = deviceName.find(LAPTOP_SUBSTR);
      laptopSubsstr && !deviceName.find("gpu", laptopSubsstr + LAPTOP_SUBSTR_OFFSET))
    deviceName.replace(LAPTOP_SUBSTR, "laptop_gpu");

  // replace ^radeon to ^amd_radeon
  if (deviceName.find("radeon") == deviceName.begin())
    deviceName.replace("radeon", "amd_radeon");

  // remove amd specific naming
  deviceName.replace("_series", "");

  // some intel and amd GPUs have "_graphics" suffix, remove it
  deviceName.replace("_graphics", "");

  gpu_score = gpuDatabase.getInt(deviceName.c_str(), 0);
  if (gpu_score == 0)
  {
    bool foundAlias = false;
    // try to find by memory size
    if (gpuDatabase.blockExists(deviceName.c_str()))
    {
      String gb;
      gb.aprintf(16, "_%dgb", d3d::get_dedicated_gpu_memory_size_kb() >> 20);
      deviceName += gb;

      gpu_score = gpuDatabase.getInt(deviceName.c_str(), 0);
      foundAlias = gpu_score != 0;
    }

    if (!foundAlias)
    {
      logwarn("can't find device '%s' in gpu_database.blk, auto graphical settings will not be applied", deviceName.c_str());
      return false;
    }
  }

  return true;
}

static const char *get_auto_preset(int gpu_score)
{
  auto lowerBoundCollections = ::dgs_get_settings()->getBlockByName("auto_preset_gpu_score_lower_bound");
  if (!lowerBoundCollections)
  {
    logerr("can't find 'auto_preset_gpu_score_lower_bound' block in settings.blk, auto graphical settings will not be applied");
    return nullptr;
  }

  int w, h;
  d3d::get_screen_size(w, h);
  const float scale = max(d3d::get_display_scale(), 1.f);
  float renderArea = static_cast<float>(w * h);
  // downgrade render area for high DPI displays (most likely laptops)
  if (renderArea > 1920 * 1080)
    renderArea /= scale;

  IPoint2 targetResoulution = {0, 0};
  int colletionId = -1;
  for (int i = 0; i < lowerBoundCollections->blockCount(); ++i)
  {
    const IPoint2 forResoultuion = lowerBoundCollections->getBlock(i)->getIPoint2("forResolution", IPoint2(0, 0));
    const float area = static_cast<float>(forResoultuion.x * forResoultuion.y);
    if (area == 0)
    {
      logerr("incorrect 'forResolution' value in 'auto_preset_gpu_score_lower_bound' block, area is zero");
      continue;
    }

    if (renderArea >= area && (targetResoulution.x < forResoultuion.x && targetResoulution.y < forResoultuion.y))
    {
      targetResoulution = forResoultuion;
      colletionId = i;
    }
  }

  if (colletionId < 0)
  {
    colletionId = 0;
    const IPoint2 forResoultuion = lowerBoundCollections->getBlock(colletionId)->getIPoint2("forResolution", IPoint2(0, 0));
    logwarn(
      "can't find suitable resolution in 'auto_preset_gpu_score_lower_bound' block, use first collection (for resolution = %dx%d)",
      forResoultuion.x, forResoultuion.y);
  }

  const DataBlock *lowerBounds = lowerBoundCollections->getBlock(colletionId);
  const char *presets[] = {"bareMinimum", "minimum", "low", "medium", "high", "ultra"};
  const char *appliedPreset = presets[0];
  for (auto preset : presets)
  {
    int lowerBound = lowerBounds->getInt(preset, 0);
    if (gpu_score >= lowerBound)
      appliedPreset = preset;
    else
      break;
  }

  debug("select %s as auto preset for resolution %dx%d (display resoultion %dx%d, scale %f)", appliedPreset, targetResoulution.x,
    targetResoulution.y, w, h, scale);
  return appliedPreset;
}

void try_apply_auto_graphical_settings()
{
#if _TARGET_PC
  G_ASSERT(d3d::is_inited());
  if (!::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("auto_preset", false))
  {
    debug("skipping auto graphical settings");
    return;
  }

  int gpuScore = 0;
  if (!get_gpu_score(gpuScore))
  {
    logwarn("failed to get GPU score (see reason above), auto graphical settings will not be applied");
    return;
  }

  if (auto preset = get_auto_preset(gpuScore); preset)
  {
    auto graphicsOverride = get_settings_override_blk()->addBlock("graphics");
    graphicsOverride->setStr("preset", preset);
    graphicsOverride->setBool("auto_preset", false);
    dgs_apply_essential_pc_preset_params(*get_settings_override_blk(), preset);
    save_settings(nullptr);
  }
#endif
}