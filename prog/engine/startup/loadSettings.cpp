// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_loadSettings.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_vromfs.h>
#include <memory/dag_framemem.h>
#include <util/dag_oaHashNameMap.h>

static DataBlock settings_blk, gameParams;
#define stg settings_blk // TODO: compat alias, remove
static const DataBlock *get_settings_blk() { return &stg; }
static const DataBlock *get_game_params_blk() { return &gameParams; }

DagorSettingsBlkHolder::DagorSettingsBlkHolder()
{
  ::dgs_get_settings = &get_settings_blk;
  ::dgs_get_game_params = &get_game_params_blk;
}
DagorSettingsBlkHolder::~DagorSettingsBlkHolder()
{
  stg.resetAndReleaseRoNameMap();
  gameParams.resetAndReleaseRoNameMap();
  dblk::discard_unused_shared_name_maps();
  dblk::discard_unused_blk_ddict();
}

void dgs_load_settings_blk(bool apply_cmd, const char *settings_blk_fn, const char *config_blk_fn, bool force_apply_all,
  bool store_cfg_copy, bool resolve_tex_streaming)
{
  DataBlock cfg;
  if (config_blk_fn)
    if (dd_file_exist(config_blk_fn))
      dblk::load(cfg, config_blk_fn, dblk::ReadFlag::ROBUST);

  DataBlock cmdBlk;
  if (apply_cmd)
    dgs_apply_command_line_to_config(&cmdBlk);

  dgs_load_settings_blk_ex(cmdBlk, settings_blk_fn, cfg, force_apply_all, store_cfg_copy, resolve_tex_streaming);
}

static void strncat_platform_string_id_for_patch(char *dst, const size_t n)
{
  const char *stringId = get_platform_string_id();

#if DAGOR_DBGLEVEL > 0 && _TARGET_PC
  DataBlock cfg;
  dgs_apply_command_line_to_config(&cfg);

  stringId = cfg.getStr("usePlatformPatch", stringId);
#endif

  strncat(dst, stringId, n);
}

template <class... Args>
static void log_illegal_override(Args... argv)
{
#if DAGOR_DBGLEVEL > 0
  logerr(argv...);
#else
  debug(argv...);
#endif
}

void dgs_load_settings_blk_ex(const DataBlock &filtered_cmd_blk, const char *settings_blk_fn, const DataBlock &config_blk,
  bool force_apply_all, bool store_cfg_copy, bool resolve_tex_streaming)
{
  String stg_fn;
  if (!settings_blk_fn)
  {
    stg_fn = stg.getStr("__settings_blk_path", "settings.blk");
    settings_blk_fn = stg_fn;
  }
  dblk::load(stg, settings_blk_fn, dblk::ReadFlag::ROBUST);

  if (const char *tsfn = resolve_tex_streaming ? stg.getStr("texStreamingFile", nullptr) : nullptr)
  {
    const char *blknm = "texStreaming";
    DataBlock tsblk;
    if (!tsblk.load(tsfn))
      DAG_FATAL("failed to load texStreamingFile=%s", tsfn);
    else if (const DataBlock *b = tsblk.getBlockByName(blknm))
    {
      const_cast<DataBlock *>(dgs_get_settings())->removeBlock(blknm);
      const_cast<DataBlock *>(dgs_get_settings())->removeParam("texStreamingFile");
      const_cast<DataBlock *>(dgs_get_settings())->addNewBlock(b, blknm);
    }
    else
      DAG_FATAL("failed to get %s block in texStreamingFile=%s", blknm, tsfn);
  }
  else if (!resolve_tex_streaming && stg.getStr("texStreamingFile", nullptr))
    const_cast<DataBlock *>(dgs_get_settings())->addBlock("texStreaming");

  // read and apply platform-specific patch (if exists)
  char patch_fn[DAGOR_MAX_PATH];
  const char *consoleHalfGenPatchName = get_console_model() == ConsoleModel::PS4_PRO         ? "ps4Pro"
                                        : get_console_model() == ConsoleModel::PS5_PRO       ? "ps5Pro"
                                        : get_console_model() == ConsoleModel::XBOXONE_X     ? "xboxOneX"
                                        : get_console_model() == ConsoleModel::XBOX_ANACONDA ? "xboxScarlettX"
                                                                                             : nullptr;
  for (bool mindHalfGenConsoles : {true, false})
  {
    memset(patch_fn, 0, sizeof(patch_fn));
    strncpy(patch_fn, settings_blk_fn, sizeof(patch_fn) - 1);
    if (char *p = strrchr(patch_fn, '.'))
      if (dd_stricmp(p, ".blk") == 0)
        p[1] = 0;

    const auto patchFnFreeSize = [&patch_fn]() { return sizeof(patch_fn) - strlen(patch_fn) - 1; };
    if (mindHalfGenConsoles && consoleHalfGenPatchName)
      strncat(patch_fn, consoleHalfGenPatchName, patchFnFreeSize());
    else
      strncat_platform_string_id_for_patch(patch_fn, patchFnFreeSize());

    strncat(patch_fn, ".patch", patchFnFreeSize());

    int read_flg = DF_READ | DF_IGNORE_MISSING;
#if DAGOR_DBGLEVEL == 0 // in release we require patch to be read from VROM if settings.blk is read from VROM
    if (vromfs_check_file_exists(settings_blk_fn))
      read_flg |= DF_VROM_ONLY;
#endif
    if (file_ptr_t fp = df_open(patch_fn, read_flg))
    {
      Tab<char> text(framemem_ptr());
      text.resize(df_length(fp) + 3);
      df_read(fp, text.data(), data_size(text) - 3);
      df_close(fp);
      memcpy(&text[text.size() - 3], "\n\n", 3);
      bool loaded = dblk::load_text(stg, text, dblk::ReadFlags(), ".patch");
      if (!loaded) [[unlikely]]
        debug("Content of `%s`:\n%.*s%s", patch_fn, min(2048, (int)text.size() - 3), text.data(),
          ((int)text.size() - 3) <= 2048 ? "" : "...");
      int ll = loaded ? LOGLEVEL_DEBUG : LOGLEVEL_ERR;
      logmessage(ll, "%s: patching with %s - %s", "dgs_load_settings_blk", patch_fn, loaded ? "OK" : "Failed!");
      break;
    }
  }

  if (!config_blk.isEmpty() || !filtered_cmd_blk.isEmpty())
  {
    DataBlock cfg;
    cfg = config_blk;

    if (!filtered_cmd_blk.isEmpty())
      merge_data_block(cfg, filtered_cmd_blk);

    if (!cfg.isEmpty())
      dgs_apply_config_blk(cfg, force_apply_all, store_cfg_copy);
  }
  stg.setStr("__settings_blk_path", settings_blk_fn);
  dblk::clr_flag(stg, dblk::ReadFlag::ROBUST);
}

void dgs_load_game_params_blk(const char *game_params_blk_fn)
{
  dblk::load(gameParams, game_params_blk_fn, dblk::ReadFlag::ROBUST | dblk::ReadFlag::RESTORE_FLAGS);
}

// force_apply_all=false => keep listed in aco only
// force_apply_all=true => keep everything except __exclude_allow
// ignore_excludes=true => ignore __exclude_allow
static inline void remove_disallowed_params(DataBlock &dst, const DataBlock &aco, bool force_apply_all, bool ignore_excludes)
{
  bool allow_all = aco.getBool("__allow_all_params", false);
  const DataBlock *disallow = aco.getBlockByNameEx("__exclude_allow");
  force_apply_all = aco.getBool("__allow_force_apply_all", force_apply_all);
  for (int i = 0; i < dst.paramCount(); i++)
    if ((!ignore_excludes && disallow->paramExists(dst.getParamName(i))) ||
        (!force_apply_all && !allow_all && aco.findParam(dst.getParamName(i)) < 0))
    {
      log_illegal_override("Settings: '%s/%s' is not allowed for overwrite! check settings.blk", dst.getBlockName(),
        dst.getParamName(i));
      dst.removeParam(i);
      i--;
    }

  for (int i = 0; i < dst.blockCount(); i++)
    if (const DataBlock *b = aco.getBlockByName(dst.getBlock(i)->getBlockName()))
      remove_disallowed_params(*dst.getBlock(i), *b, force_apply_all, ignore_excludes);
    else if (!force_apply_all)
    {
      log_illegal_override("Settings: '%s/%s' is not allowed for overwrite! check settings.blk", dst.getBlockName(),
        dst.getBlock(i)->getBlockName());
      dst.removeBlock(i);
      i--;
    }
}

void dgs_filter_saved_config(DataBlock &config_blk)
{
  const DataBlock *aco = stg.getBlockByNameEx("__allowedSavedConfig");
  remove_disallowed_params(config_blk, *aco, true, false);
}

void dgs_apply_config_blk_ex(DataBlock &settings_blk, const DataBlock &config_blk, bool force_apply_all, bool store_cfg_copy,
  bool save_order)
{
  if (store_cfg_copy)
    settings_blk.addBlock("originalConfigBlk")->setFrom(&config_blk);
  force_apply_all = settings_blk.getBool("__allow_force_apply_all", force_apply_all);
  bool ignore_excludes = false;
#if DAGOR_DBGLEVEL > 0
  force_apply_all = config_blk.getBool("__allow_force_apply_all", true);
  ignore_excludes = config_blk.getBool("__ignore_override_excludes", ignore_excludes);
#endif
  const DataBlock *aco = settings_blk.getBlockByNameEx("__allowedConfigOverrides");
  remove_disallowed_params(const_cast<DataBlock &>(config_blk), *aco, force_apply_all, ignore_excludes);
  if (save_order)
    merge_data_block_and_save_order(settings_blk, config_blk);
  else
    merge_data_block(settings_blk, config_blk);
}
void dgs_apply_config_blk(const DataBlock &config_blk, bool force_apply_all, bool store_cfg_copy, bool save_order)
{
  dgs_apply_config_blk_ex(stg, config_blk, force_apply_all, store_cfg_copy, save_order);
}
