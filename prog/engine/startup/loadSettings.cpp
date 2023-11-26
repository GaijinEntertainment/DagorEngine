#include <startup/dag_loadSettings.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_vromfs.h>
#include <util/dag_oaHashNameMap.h>

static DataBlock stg, gameParams;
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
  dgs_load_settings_blk_ex(apply_cmd, settings_blk_fn, cfg, force_apply_all, store_cfg_copy, resolve_tex_streaming);
}
void dgs_load_settings_blk_ex(bool apply_cmd, const char *settings_blk_fn, const DataBlock &config_blk, bool force_apply_all,
  bool store_cfg_copy, bool resolve_tex_streaming, const SettingsHashMap *changed_settings)
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
      fatal("failed to load texStreamingFile=%s", tsfn);
    else if (const DataBlock *b = tsblk.getBlockByName(blknm))
    {
      const_cast<DataBlock *>(dgs_get_settings())->removeBlock(blknm);
      const_cast<DataBlock *>(dgs_get_settings())->removeParam("texStreamingFile");
      const_cast<DataBlock *>(dgs_get_settings())->addNewBlock(b, blknm);
    }
    else
      fatal("failed to get %s block in texStreamingFile=%s", blknm, tsfn);
  }
  else if (!resolve_tex_streaming && stg.getStr("texStreamingFile", nullptr))
    const_cast<DataBlock *>(dgs_get_settings())->addBlock("texStreaming");

  // read and apply platform-specific patch (if exists)
  char patch_fn[DAGOR_MAX_PATH];

  for (bool mindHalfGenConsoles : {true, false})
  {
    memset(patch_fn, 0, sizeof(patch_fn));
    strncpy(patch_fn, settings_blk_fn, sizeof(patch_fn) - 1);
    if (char *p = strrchr(patch_fn, '.'))
      if (dd_stricmp(p, ".blk") == 0)
        p[1] = 0;
    if (mindHalfGenConsoles && get_console_model() == ConsoleModel::PS4_PRO)
      strcat(patch_fn, "ps4Pro");
    else if (mindHalfGenConsoles && get_console_model() == ConsoleModel::XBOXONE_X)
      strcat(patch_fn, "xboxOneX");
    else if (mindHalfGenConsoles && get_console_model() == ConsoleModel::XBOX_ANACONDA)
      strcat(patch_fn, "xboxScarlettX");
    else
      strcat(patch_fn, get_platform_string_id());
    strcat(patch_fn, ".patch");

    int read_flg = DF_READ | DF_IGNORE_MISSING;
#if DAGOR_DBGLEVEL == 0 // in release we require patch to be read from VROM if settings.blk is read from VROM
    if (vromfs_check_file_exists(settings_blk_fn))
      read_flg |= DF_VROM_ONLY;
#endif
    if (file_ptr_t fp = df_open(patch_fn, read_flg))
    {
      Tab<char> text(tmpmem);
      text.resize(df_length(fp) + 3);
      df_read(fp, text.data(), data_size(text) - 3);
      df_close(fp);
      strcpy(&text[text.size() - 3], "\n\n");
      bool ret = dblk::load_text(stg, text, dblk::ReadFlags(), ".patch");
      debug("%s: patching with %s: %d", "dgs_load_settings_blk", patch_fn, (int)ret);
      break;
    }
  }

  if (!config_blk.isEmpty() || apply_cmd)
  {
    DataBlock cfg;
    cfg = config_blk;

    if (apply_cmd)
    {
      OverrideFilter filter = gen_default_override_filter(changed_settings);
      dgs_apply_command_line_to_config(&cfg, &filter);
    }

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

static void copy_blk_params(DataBlock &dst, const DataBlock &src, const DataBlock &aco)
{
  bool allow_all = aco.getBool("__allow_all_params", false);
  const DataBlock *disallow = aco.getBlockByName("__exclude_allow");
  for (int i = 0; i < src.paramCount(); i++)
    if ((allow_all || aco.findParam(src.getParamName(i)) >= 0) && !(disallow && disallow->paramExists(src.getParamName(i))))
      addOverrideParam(dst, src, i, true);

  for (int i = 0; i < src.blockCount(); i++)
    if (const DataBlock *b = aco.getBlockByName(src.getBlock(i)->getBlockName()))
      copy_blk_params(*dst.addBlock(src.getBlock(i)->getBlockName()), *src.getBlock(i), *b);
}
static void remove_disallowed_params(DataBlock &dst, const DataBlock &aco)
{
  if (const DataBlock *disallow = aco.getBlockByName("__exclude_allow"))
    for (int i = 0; i < disallow->paramCount(); i++)
      dst.removeParam(disallow->getParamName(i));

  for (int i = 0; i < dst.blockCount(); i++)
    if (const DataBlock *b = aco.getBlockByName(dst.getBlock(i)->getBlockName()))
      remove_disallowed_params(*dst.getBlock(i), *b);
}

void dgs_apply_config_blk_ex(DataBlock &settings_blk, const DataBlock &config_blk, bool force_apply_all, bool store_cfg_copy,
  bool save_order)
{
  if (store_cfg_copy)
    settings_blk.addBlock("originalConfigBlk")->setFrom(&config_blk);
  force_apply_all = settings_blk.getBool("__allow_force_apply_all", force_apply_all);
#if DAGOR_DBGLEVEL > 0
  force_apply_all = config_blk.getBool("__allow_force_apply_all", true);
#endif
  const DataBlock *aco = settings_blk.getBlockByName("__allowedConfigOverrides");
  if (force_apply_all)
  {
#if DAGOR_DBGLEVEL > 0
    if (aco && !config_blk.getBool("__ignore_override_excludes", false))
#else
    if (aco)
#endif
      remove_disallowed_params(const_cast<DataBlock &>(config_blk), *aco);
    if (save_order)
      merge_data_block_and_save_order(settings_blk, config_blk);
    else
      merge_data_block(settings_blk, config_blk);
    return;
  }

  if (!aco)
    return;
  copy_blk_params(settings_blk, config_blk, *aco);
}
void dgs_apply_config_blk(const DataBlock &config_blk, bool force_apply_all, bool store_cfg_copy, bool save_order)
{
  dgs_apply_config_blk_ex(stg, config_blk, force_apply_all, store_cfg_copy, save_order);
}
