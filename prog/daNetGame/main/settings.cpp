// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "settings.h"
#include "main.h"
#include "level.h"
#include "vromfs.h"
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


namespace
{
static struct GameSettingsLoadCtx
{
  VirtualRomFsData *vrom = NULL;
  String vromMnt;
  String basePath;

  String vromSrcPath;
} gs_ctx;
} // namespace

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

String get_config_filename(const char *game_name)
{
#if _TARGET_PC
  String settings_path(0, "%s.config.blk", game_name);
#else
  String settings_path(0, "%s.config.%s.blk", game_name, get_platform_string_id());
#endif
  return settings_path;
}


const char *get_allowed_addon_src_files_prefix(const char *game_name)
{
#if DAGOR_DBGLEVEL > 0 && (_TARGET_PC || _TARGET_C3)
  String config_blk_fn = get_config_filename(game_name);
  DataBlock cfg;
  if (config_blk_fn && dd_file_exist(config_blk_fn))
    dblk::load(cfg, config_blk_fn, dblk::ReadFlag::ROBUST);

  dgs_apply_command_line_to_config(&cfg);

  if (cfg.getBlockByNameEx("debug")->getBool("useAddonVromSrc", false))
  {
    gs_ctx.vromSrcPath = cfg.getBlockByNameEx("debug")->getStr("vromSrcPath", "../prog/gameBase/");
    return gs_ctx.vromSrcPath;
  }
#endif
  G_UNUSED(game_name);
  return NULL;
}

void prepare_before_loading_game_settings_blk(const char *game_name)
{
  if (const char *path = get_allowed_addon_src_files_prefix(game_name))
  {
    if (dd_add_base_path(path, false))
      gs_ctx.basePath = path;
  }
  else
  {
    String vrom_fn(0, "content/%s/%s-game.vromfs.bin", game_name, game_name);
    if (!dd_file_exists(vrom_fn))
      vrom_fn.printf(0, "content/%s/%s.vromfs.bin", game_name, game_name);
    if (dd_file_exists(vrom_fn))
    {
      String eff_path(get_eff_vrom_fpath(vrom_fn));
      gs_ctx.vrom = ::load_vromfs_dump(eff_path, inimem);
      if (!gs_ctx.vrom && strcmp(eff_path, vrom_fn) != 0)
      {
        gs_ctx.vrom = load_vromfs_dump(vrom_fn, tmpmem);
        logwarn("removing broken %s", eff_path);
        dd_erase(eff_path);
      }
      gs_ctx.vromMnt.printf(0, "content/%s/", game_name);
      if (gs_ctx.vrom)
        add_vromfs(gs_ctx.vrom, true, gs_ctx.vromMnt);
    }
    if (!gs_ctx.vrom)
      DAG_FATAL("failed to load vromfs: %s", vrom_fn);

    String stg_fn(0, "content/%s/gamedata/%s.settings.blk", game_name, game_name);
    if (!vromfs_check_file_exists(stg_fn))
      DAG_FATAL("failed to find %s using signed vromfs %s (mounted to %s)", stg_fn, vrom_fn, gs_ctx.vromMnt);
  }
}
void restore_after_loading_game_settings_blk()
{
  if (gs_ctx.vrom)
  {
    remove_vromfs(gs_ctx.vrom);
    memfree(gs_ctx.vrom, inimem);
    clear_and_shrink(gs_ctx.vromMnt);
    gs_ctx.vrom = NULL;
  }
  else if (!gs_ctx.basePath.empty())
  {
    dd_remove_base_path(gs_ctx.basePath);
    clear_and_shrink(gs_ctx.basePath);
  }
  clear_and_shrink(gs_ctx.vromSrcPath);
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

void load_settings(const char *game_name, bool resolve_tex_streaming, bool use_on_reload_backup)
{
  debug("load settings for %s", game_name);
  if (!settings_override_blk)
  {
    settings_override_filename = get_config_filename(game_name);
    debug("Loading %s", settings_override_filename);
    settings_override_blk.reset(new DataBlock);
#if _TARGET_PC
    dblk::load(*settings_override_blk, settings_override_filename, dblk::ReadFlag::ROBUST);
#else
#if _TARGET_C1 | _TARGET_C2


#elif _TARGET_C3


#elif _TARGET_ANDROID
    settings_override_filename.printf(0, "%s/%s.config.blk", folders::get_game_dir(), game_name);
#else
    settings_override_filename.printf(0, "%s/%s.config.blk", folders::get_gamedata_dir(), game_name);
#endif

    DataBlock user_blk;
    if (dblk::load(user_blk, settings_override_filename, dblk::ReadFlag::ROBUST))
    {
      debug("Loading (savedata) %s", settings_override_filename);
      *settings_override_blk = user_blk;
    }
#endif

    if (use_on_reload_backup)
      update_settings_from_reload_changes();
  }

  prepare_before_loading_game_settings_blk(game_name);
  {
    String stg_fn(0, "content/%s/gamedata/%s.settings.blk", game_name, game_name);
    G_ASSERTF(dd_file_exists(stg_fn), "file %s", stg_fn);

#if _TARGET_XBOX
    String settings_path(0, "G:\\%s", game_name, get_config_filename(game_name).c_str());
#else
    String settings_path = get_config_filename(game_name);
#endif
#if _TARGET_PC
    dgs_load_settings_blk_ex(true, stg_fn, *settings_override_blk, false, true, resolve_tex_streaming);
#else
    dgs_load_settings_blk(true, stg_fn, settings_path, false, true, resolve_tex_streaming);
    dgs_apply_changes_to_config(*settings_override_blk, /*need_merge_cmd*/ true);
    dgs_apply_config_blk(*settings_override_blk, false, false);
#endif

    const_cast<DataBlock *>(::dgs_get_settings())->setStr("gameName", game_name);

    restore_after_loading_game_settings_blk();
  }
  apply_global_config();
  apply_ime_to_video_mode();

  load_gameparams(game_name);
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
void load_gameparams(const char *game_name)
{
  dgs_load_game_params_blk(String(0, "content/%s/gamedata/%s.gameparams.blk", game_name, game_name));
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
    save_stg_jobmgr = create_virtual_job_manager(256 << 10, WORKER_THREADS_AFFINITY_MASK, "SaveSettingsJobMgr",
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
    dgs_load_settings_blk(true, NULL, get_config_filename(get_game_name()), false, true);
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
      || (changed_fields.getNameId("video/latency") >= 0
        && dgs_get_settings()->getBlockByNameEx("video")->getBool("vsync", false))
      || changed_fields.getNameId("video/dlssQuality") >= 0
      || changed_fields.getNameId("video/dlssFrameGeneration") >= 0
      || changed_fields.getNameId("video/xessQuality") >= 0
      || changed_fields.getNameId("video/amdfsr") >= 0
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
    int freqLevel = ::dgs_get_settings()->getBlockByNameEx("video")->getInt("freqLevel", 1);
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
  String pkey(strlen(key) + 16, "%s_%s", pstring, key);
  return blk.getInt(pkey, def);
}

const char *get_platformed_value(const DataBlock &blk, const char *key, const char *def)
{
  const char *pstring = get_platform_string();
  if (!pstring)
    return blk.getStr(key, def);
  String pkey(strlen(key) + 16, "%s_%s", pstring, key);
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
  if (need_merge_cmd)
    dgs_apply_command_line_to_config(&cmdBlk, override_filter);

  // check cmd block first
  const char *presetName = nullptr;
  if (need_merge_cmd)
    presetName = cmdBlk.getBlockByNameEx("graphics")->getStr("consolePreset", nullptr);

  if (presetName == nullptr) // From saved data
    presetName = config_blk.getBlockByNameEx("graphics")->getStr("consolePreset", nullptr);

  if (presetName == nullptr) // From common settings for default
    presetName = ::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("consolePreset", nullptr);

  // first apply graphic preset settings
  if (presetName != nullptr)
    dgs_apply_console_preset_params(config_blk, presetName);

  // second apply cmd changes, overwrite all graphic preset settings if they are exist
  if (need_merge_cmd)
    merge_data_block(config_blk, cmdBlk);
}

void dgs_apply_console_preset_params(DataBlock &config_blk, const char *preset_name)
{
  const char *presetsListBlockName = "consoleGraphicalPresets";
  const DataBlock *presetsBlk = ::dgs_get_settings()->getBlockByName(presetsListBlockName);
  if (!presetsBlk)
  {
    debug("Not exist block '%s' in settings.blk. Cannot read '%s' console preset name.", presetsListBlockName, preset_name);
    return;
  }

  const DataBlock *presetParamsBlk = presetsBlk->getBlockByName(preset_name);
  if (!presetParamsBlk)
  {
    debug("ERROR: Not found console preset '%s' in '%s' block.", preset_name, presetsListBlockName);
    return;
  }

  debug("Apply console preset '%s'.", preset_name);
  merge_data_block(config_blk, *presetParamsBlk);
}